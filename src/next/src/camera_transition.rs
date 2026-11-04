// Ports the camera transition end handlers of maplibre-native-ffi
// src/map/map.cpp: CameraTransitionEnd, CommandTransitions,
// end_camera_command, deliver_ended_transitions, close_camera_transitions,
// TransitionCancellationScope and TransitionReplacementScope.
//
// Each camera command with an end handler reports once how the MapLibre
// transitions that it started ended. maplibre-native-next's Transform runs
// one transition at a time, as upstream MapLibre Native does: a camera write
// finishes the running transition before it starts its own. The C++
// library's core patch that animates partial camera commands independently
// (patches/maplibre-native/0015) has no counterpart, so here a later camera
// change always ends a running command, as REPLACED, whichever fields it
// sets, and a cancellation by transition id stops the running command only
// when it has that id.
//
// A MapLibre transition calls its finish function inside a Transform call,
// so the functions here only record the end. A command reports the ends
// that its own run recorded with its generation, before its completion
// (map.rs `post_command`). An end outside a command, such as a transition
// that a rendered frame completes, takes a fresh generation, which the
// camera's did-change event that MapLibre raises next also carries, and a
// later task publishes the snapshot and runs the handler.

use crate::delivery::{UserData, deliver};
use crate::diagnostics::Failure;
use crate::handles;
use crate::map::{self, MapEntry, MapShared};
use crate::types::*;
use maplibre_native_next::mln_map::AnimationOptions;
use maplibre_native_next::mln_util::chrono::{Clock, Duration, TimePoint};
use std::cell::{Cell, RefCell};
use std::ffi::c_void;
use std::rc::{Rc, Weak};
use std::sync::Arc;
use std::sync::atomic::{AtomicBool, Ordering};

/// A validated `mln_camera_transition_handler`, which crosses to the runtime
/// thread with its command.
#[derive(Clone, Copy)]
pub struct HandlerSpec {
    callback: unsafe extern "C" fn(*mut c_void, *const mln_camera_transition_end),
    user_data: UserData,
    release_user_data: mln_user_data_release,
    queue: u64,
}

impl HandlerSpec {
    /// C++ `camera_transition_end`: the command's end handler, or `None` for
    /// a disabled one. `validate_animation_options` has checked that a
    /// disabled one retains no user data.
    pub fn of(handler: &mln_camera_transition_handler) -> Result<Option<HandlerSpec>, Failure> {
        if handler.callback.is_none() && handler.queue.0 == 0 {
            return Ok(None);
        }
        let Some(callback) = handler.callback else {
            return Err(Failure::invalid_argument("camera transition handler requires a callback"));
        };
        if handler.queue.0 != 0 {
            handles::queue(handler.queue)?;
        }
        Ok(Some(HandlerSpec {
            callback,
            user_data: UserData(handler.user_data),
            release_user_data: handler.release_user_data,
            queue: handler.queue.0,
        }))
    }

    /// Hands the handler to an accepted command. From here on it runs once:
    /// with the command's end, or as CLOSED when the runtime drops the
    /// command without ending it.
    pub fn accept(self) -> Arc<EndHandler> {
        Arc::new(EndHandler { spec: self, delivered: AtomicBool::new(false) })
    }
}

/// C++ `CameraTransitionEnd`: the end handler of one accepted command.
pub struct EndHandler {
    spec: HandlerSpec,
    delivered: AtomicBool,
}

impl EndHandler {
    /// Runs the callback, then the release, once, through the handler's
    /// queue when it has one.
    pub fn deliver(&self, outcome: u32, generation: u64) {
        if self.delivered.swap(true, Ordering::SeqCst) {
            return;
        }
        let spec = self.spec;
        deliver(
            spec.queue,
            Box::new(move || {
                let end = mln_camera_transition_end {
                    size: size_of::<mln_camera_transition_end>() as u32,
                    outcome,
                    generation,
                };
                // SAFETY: the caller registered this callback and user_data; `end`
                // lives until it returns.
                unsafe { (spec.callback)(spec.user_data.get(), &end) };
                if let Some(release) = spec.release_user_data {
                    // SAFETY: the registration's release, called once.
                    unsafe { release(spec.user_data.get()) };
                }
            }),
        );
    }
}

impl Drop for EndHandler {
    fn drop(&mut self) {
        self.deliver(MLN_CAMERA_TRANSITION_OUTCOME_CLOSED, 0);
    }
}

/// What the camera writes that run now do to the transitions they finish.
#[derive(Clone, Copy)]
enum Scope {
    /// Nothing: a transition that finishes ends on its own.
    None,
    /// C++ `TransitionReplacementScope`: a camera write of `command`
    /// replaces the other commands' transitions.
    Replacing { command: u64 },
    /// C++ `TransitionCancellationScope`.
    Cancelling { all: bool, transition_id: u64 },
}

/// The transition that the map's Transform runs.
#[derive(Clone, Copy)]
struct Running {
    token: u64,
    transition_id: Option<u64>,
}

/// A command that ended, whose handler hasn't run yet.
struct Ended {
    end: Arc<EndHandler>,
    outcome: u32,
    /// A fresh generation for an end outside a command; a command reports
    /// its ends with its own.
    generation: Option<u64>,
}

/// A map's camera transition state, on the runtime thread (C++
/// `MapEventState`'s transition members).
pub struct CameraState {
    shared: Arc<MapShared>,
    /// Set while a map command runs (map.rs `post_command`).
    pub in_command: Cell<bool>,
    scope: Cell<Scope>,
    running: Cell<Option<Running>>,
    ended: RefCell<Vec<Ended>>,
    delivery_posted: Cell<bool>,
    next_id: Cell<u64>,
    /// `mln_map_set_rendering_stats_view_enabled`. The core has no rendering
    /// statistics view yet, so the map reports the setting without drawing.
    pub rendering_stats_view: Cell<bool>,
}

impl CameraState {
    pub fn new(shared: Arc<MapShared>) -> Rc<CameraState> {
        Rc::new(CameraState {
            shared,
            in_command: Cell::new(false),
            scope: Cell::new(Scope::None),
            running: Cell::new(None),
            ended: RefCell::new(Vec::new()),
            delivery_posted: Cell::new(false),
            next_id: Cell::new(1),
            rendering_stats_view: Cell::new(false),
        })
    }

    fn next_id(&self) -> u64 {
        let id = self.next_id.get();
        self.next_id.set(id + 1);
        id
    }

    /// Whether a camera transition is running (C++ `Map::inTransition`,
    /// patches/maplibre-native/0046). Every transition starts through
    /// `transition`, so its finish function tracks it.
    pub fn in_transition(&self) -> bool {
        self.running.get().is_some()
    }

    /// The transition id of the running transition, if it has one.
    pub fn running_transition_id(&self) -> Option<u64> {
        self.running.get().and_then(|running| running.transition_id)
    }

    /// Starts tracking a command: its end handler, if any, and the number of
    /// MapLibre transitions that it starts.
    pub fn command(
        self: &Rc<Self>,
        end: Option<Arc<EndHandler>>,
        transition_id: Option<u64>,
        transitions: u32,
    ) -> Rc<CommandEnd> {
        Rc::new(CommandEnd {
            state: Rc::downgrade(self),
            end,
            id: self.next_id(),
            transition_id,
            remaining: Cell::new(transitions),
            cancelled: Cell::new(false),
            replaced: Cell::new(false),
        })
    }

    /// The `AnimationOptions` of one MapLibre transition of `command`, whose
    /// finish function ends that part of the command. `duration` is how long
    /// the transition runs, when it animates.
    pub fn transition(
        self: &Rc<Self>,
        command: &Rc<CommandEnd>,
        base: &AnimationOptions,
        duration: Option<Duration>,
    ) -> AnimationOptions {
        let token = self.next_id();
        self.running.set(Some(Running { token, transition_id: command.transition_id }));
        // A transition that a frame completes has run its whole duration;
        // MapLibre's float timing may round it up to the end slightly early.
        let deadline = duration
            .filter(|duration| *duration > Duration::zero())
            .map(|duration| Clock::now() + Duration::from_nanos(duration.count() - duration.count() / 100_000));
        let state = Rc::downgrade(self);
        let command = command.clone();
        let mut options = base.clone();
        options.transition_finish_fn = Some(Rc::new(move || {
            if let Some(state) = state.upgrade()
                && state.running.get().is_some_and(|running| running.token == token)
            {
                state.running.set(None);
            }
            command.finish(deadline);
        }));
        options
    }

    /// Runs `write`, a camera write of `command`, which replaces the
    /// transitions of other commands.
    pub fn replacing<R>(&self, command: &CommandEnd, write: impl FnOnce() -> R) -> R {
        let _scope = ScopeGuard::enter(self, Scope::Replacing { command: command.id });
        write()
    }

    /// Runs `write`, a camera write of no command, such as the jump of
    /// `mln_map_set_bounds`, which replaces the running transitions.
    pub fn replacing_any<R>(&self, write: impl FnOnce() -> R) -> R {
        let _scope = ScopeGuard::enter(self, Scope::Replacing { command: 0 });
        write()
    }

    /// Runs `cancel`, which cancels every running transition.
    pub fn cancelling<R>(&self, cancel: impl FnOnce() -> R) -> R {
        let _scope = ScopeGuard::enter(self, Scope::Cancelling { all: true, transition_id: 0 });
        cancel()
    }

    /// Runs `cancel`, which cancels the transitions with `transition_id`.
    pub fn cancelling_id<R>(&self, transition_id: u64, cancel: impl FnOnce() -> R) -> R {
        let _scope = ScopeGuard::enter(self, Scope::Cancelling { all: false, transition_id });
        cancel()
    }

    /// C++ `end_camera_command`: records that a command ended. Outside a
    /// map command, the camera change that ended it takes a fresh
    /// generation, and a later task publishes the snapshot that shows it.
    fn end_command(&self, end: Option<Arc<EndHandler>>, outcome: u32) {
        let in_command = self.in_command.get();
        let generation = (!in_command).then(|| self.shared.next_generation());
        if let Some(end) = end {
            self.ended.borrow_mut().push(Ended { end, outcome, generation });
        }
        if !in_command && !self.delivery_posted.replace(true) {
            let handle = self.shared.handle;
            self.shared.runtime.post(move || {
                if let Some(entry) = map::entry(handle) {
                    entry.publish_snapshot();
                    deliver_ended(&entry, entry.shared.generation.load(Ordering::SeqCst));
                }
            });
        }
    }
}

impl Drop for CameraState {
    /// C++ `close_camera_transitions`: a closing map runs the handlers of
    /// the commands that ended before it closed. The commands that it still
    /// runs report CLOSED as the map drops their transitions.
    fn drop(&mut self) {
        let generation = self.shared.generation.load(Ordering::SeqCst);
        for ended in self.ended.take() {
            ended.end.deliver(ended.outcome, ended.generation.unwrap_or(generation));
        }
    }
}

/// C++ `deliver_ended_transitions`: runs the handlers of the commands that
/// ended, with `generation` for those that ended in a command.
pub fn deliver_ended(entry: &MapEntry, generation: u64) {
    entry.camera.delivery_posted.set(false);
    let ended = entry.camera.ended.take();
    for ended in ended {
        ended.end.deliver(ended.outcome, ended.generation.unwrap_or(generation));
    }
}

struct ScopeGuard<'a> {
    state: &'a CameraState,
    previous: Scope,
}

impl<'a> ScopeGuard<'a> {
    fn enter(state: &'a CameraState, scope: Scope) -> ScopeGuard<'a> {
        ScopeGuard { state, previous: state.scope.replace(scope) }
    }
}

impl Drop for ScopeGuard<'_> {
    fn drop(&mut self) {
        self.state.scope.set(self.previous);
    }
}

/// C++ `CommandTransitions`: the MapLibre transitions that one camera
/// command started. The command ends once, after the last of them finishes:
/// as cancelled when a cancellation finished any of them, otherwise as
/// replaced when another command's camera write finished any of them,
/// otherwise as completed. A command whose transitions don't all finish, as
/// when it fails before it starts them, ends as cancelled.
pub struct CommandEnd {
    state: Weak<CameraState>,
    end: Option<Arc<EndHandler>>,
    id: u64,
    transition_id: Option<u64>,
    remaining: Cell<u32>,
    cancelled: Cell<bool>,
    replaced: Cell<bool>,
}

impl CommandEnd {
    /// Ends one transition of the command. `deadline` is when it completes,
    /// if it animates.
    pub fn finish(&self, deadline: Option<TimePoint>) {
        let Some(state) = self.state.upgrade() else {
            return;
        };
        if self.remaining.get() == 0 {
            return;
        }
        match state.scope.get() {
            Scope::Cancelling { all, transition_id } => {
                if all || self.transition_id == Some(transition_id) {
                    self.cancelled.set(true);
                }
            }
            Scope::Replacing { command } => {
                if command != self.id {
                    self.replaced.set(true);
                }
            }
            Scope::None => {
                // Outside a camera write, a transition ends when a frame
                // completes it, or when a resize that moves the camera back
                // inside its bounds cancels it. A still map completes its
                // transitions on its next update.
                let continuous = state.shared.mode == MLN_MAP_MODE_CONTINUOUS;
                if continuous && deadline.is_some_and(|deadline| Clock::now() < deadline) {
                    self.cancelled.set(true);
                }
            }
        }
        self.remaining.set(self.remaining.get() - 1);
        if self.remaining.get() != 0 {
            return;
        }
        let outcome = if self.cancelled.get() {
            MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED
        } else if self.replaced.get() {
            MLN_CAMERA_TRANSITION_OUTCOME_REPLACED
        } else {
            MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED
        };
        state.end_command(self.end.clone(), outcome);
    }
}

impl Drop for CommandEnd {
    fn drop(&mut self) {
        if self.remaining.get() == 0 {
            return;
        }
        // A closing map reports CLOSED instead, through the handler's drop.
        if let Some(state) = self.state.upgrade() {
            state.end_command(self.end.clone(), MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED);
        }
    }
}
