// Ports the offscreen render sessions of maplibre-native-ffi
// (src/render/offscreen_device.cpp, render_session_common.cpp,
// texture_session.cpp, src/c_api/offscreen.cpp, render_session.cpp,
// texture.cpp: the parts this crate implements) onto `mln_platform_default`'s
// HeadlessFrontend.
//
// A map's renderer frontend is a `ProxyFrontend`: it exists from the map's
// creation, keeps the map's last UpdateParameters, and forwards them to the
// HeadlessFrontend of the render session once one attaches. The session's
// "core worker" is the runtime thread. A continuous map renders on frame
// demands; a static or tile map renders still images as its HeadlessFrontend
// invalidates, as in C++. A WebGPU surface session renders on the host's
// thread instead (surface_session.rs): the frontend hands it each update.

use crate::delivery::{Completion, Value, Wake};
use crate::diagnostics::{Failure, boundary};
use crate::handles::{self, Object};
use crate::map::{self, MapEntry, MapShared};
use crate::runtime::STATE;
use crate::types::*;
use maplibre_native_next::mln_actor::RunLoop;
use maplibre_native_next::mln_gfx::{ContextMode, SwapBehaviour};
use maplibre_native_next::mln_map::MapMode;
use maplibre_native_next::mln_platform_default::HeadlessFrontend;
use maplibre_native_next::mln_renderer::{RendererFrontend, RendererObserver, UpdateParameters};
use maplibre_native_next::mln_util::size::Size;
use std::cell::{Cell, RefCell};
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::rc::{Rc, Weak};
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::{Arc, Mutex};

// MARK: - ProxyFrontend

struct ProxyInner {
    mode: MapMode,
    pixel_ratio: f32,
    attached: RefCell<Option<HeadlessFrontend>>,
    observer: RefCell<Option<Weak<dyn RendererObserver>>>,
    last_update: RefCell<Option<Arc<UpdateParameters>>>,
    update_generation: Cell<u64>,
    session: Cell<u64>,
    /// A surface session, which renders on the host's thread.
    remote: RefCell<Option<(u64, Arc<SessionShared>)>>,
    /// The newest update that a running command or an open command group
    /// holds back (C++ HeadlessFrontend's `held_update_`).
    held: RefCell<Option<Arc<UpdateParameters>>>,
    in_command: Cell<bool>,
    group_depth: Cell<u32>,
}

/// The map's `RendererFrontend` (see the file comment).
#[derive(Clone)]
pub struct ProxyFrontend {
    inner: Rc<ProxyInner>,
}

impl ProxyFrontend {
    pub fn new(mode: MapMode, pixel_ratio: f32) -> ProxyFrontend {
        ProxyFrontend {
            inner: Rc::new(ProxyInner {
                mode,
                pixel_ratio,
                attached: RefCell::new(None),
                observer: RefCell::new(None),
                last_update: RefCell::new(None),
                update_generation: Cell::new(0),
                session: Cell::new(0),
                remote: RefCell::new(None),
                held: RefCell::new(None),
                in_command: Cell::new(false),
                group_depth: Cell::new(0),
            }),
        }
    }

    // A map command runs as one transaction, so the updates that MapLibre
    // makes while it runs (a gesture flag and a camera move, or each step of
    // a layer move) publish once, as the last of them, when it ends. A
    // command group holds them until its outermost end (C++ HeadlessFrontend
    // `store_held_update`, `begin_command_group`, `end_command_group`).

    pub fn begin_command(&self) {
        self.inner.in_command.set(true);
    }

    /// Publishes the update that the command held, unless a group is open.
    pub fn end_command(&self) {
        self.inner.in_command.set(false);
        if self.inner.group_depth.get() == 0 {
            let held = self.inner.held.borrow_mut().take();
            if let Some(update) = held {
                RendererFrontend::update(self, update);
            }
        }
    }

    pub fn begin_command_group(&self) {
        self.inner.group_depth.set(self.inner.group_depth.get() + 1);
    }

    /// `false` when no group is open.
    pub fn end_command_group(&self) -> bool {
        let depth = self.inner.group_depth.get();
        if depth == 0 {
            return false;
        }
        self.inner.group_depth.set(depth - 1);
        true
    }

    /// Whether a group holds the map's updates, so a still image would bind
    /// to a stale one (C++ map_request_still_image_start fails then).
    #[allow(dead_code)] // TODO(ffi): mln_map_request_still_image.
    pub fn command_group_open(&self) -> bool {
        self.inner.group_depth.get() > 0
    }

    /// The generation of the first update that includes the map's current
    /// state: the held update publishes as the next one.
    pub fn pending_update_generation(&self) -> u64 {
        self.inner.update_generation.get() + u64::from(self.inner.held.borrow().is_some())
    }

    pub fn update_generation(&self) -> u64 {
        self.inner.update_generation.get()
    }

    pub fn has_update(&self) -> bool {
        self.inner.last_update.borrow().is_some()
    }

    /// The map's renderer observer.
    pub fn observer(&self) -> Option<Rc<dyn RendererObserver>> {
        self.inner.observer.borrow().as_ref().and_then(Weak::upgrade)
    }

    /// Hands this and every later update to a surface session.
    pub fn attach_remote(&self, session: u64, shared: Arc<SessionShared>) {
        if let Some(update) = self.inner.last_update.borrow().clone() {
            crate::surface_session::hand_off(&shared, &update, self.inner.update_generation.get());
        }
        *self.inner.remote.borrow_mut() = Some((session, shared));
    }

    pub fn detach_remote(&self, session: u64) {
        let mut remote = self.inner.remote.borrow_mut();
        if remote.as_ref().is_some_and(|(handle, _)| *handle == session) {
            *remote = None;
        }
    }

    /// Creates the session's HeadlessFrontend (and with it the wgpu device)
    /// and hands it the map's renderer observer and last update.
    fn attach(&self, session: u64, size: Size) -> HeadlessFrontend {
        // A continuous map renders on frame demands only, so its frontend
        // doesn't render on update; a still image renders as it invalidates.
        let invalidate_on_update = self.inner.mode != MapMode::Continuous;
        let frontend = HeadlessFrontend::with_options(
            size,
            self.inner.pixel_ratio,
            SwapBehaviour::NoFlush,
            ContextMode::Unique,
            invalidate_on_update,
        );
        if let Some(observer) = self.inner.observer.borrow().clone() {
            frontend.set_observer(observer);
        }
        *self.inner.attached.borrow_mut() = Some(frontend.clone());
        self.inner.session.set(session);
        if self.inner.mode == MapMode::Continuous {
            if let Some(update) = self.inner.last_update.borrow().clone() {
                frontend.update(update);
            }
        }
        frontend
    }

    fn detach(&self) {
        self.inner.session.set(0);
        let frontend = self.inner.attached.borrow_mut().take();
        if let Some(frontend) = frontend {
            frontend.reset();
        }
    }
}

impl RendererFrontend for ProxyFrontend {
    fn reset(&self) {
        let frontend = self.inner.attached.borrow().clone();
        if let Some(frontend) = frontend {
            frontend.reset();
        }
    }

    fn set_observer(&self, observer: Weak<dyn RendererObserver>) {
        *self.inner.observer.borrow_mut() = Some(observer.clone());
        let frontend = self.inner.attached.borrow().clone();
        if let Some(frontend) = frontend {
            frontend.set_observer(observer);
        }
    }

    fn update(&self, parameters: Arc<UpdateParameters>) {
        if self.inner.in_command.get() || self.inner.group_depth.get() > 0 {
            // Each update is a complete snapshot of the map, so only the
            // newest held one is kept.
            *self.inner.held.borrow_mut() = Some(parameters);
            return;
        }
        *self.inner.last_update.borrow_mut() = Some(parameters.clone());
        self.inner.update_generation.set(self.inner.update_generation.get() + 1);
        let frontend = self.inner.attached.borrow().clone();
        if let Some(frontend) = frontend {
            frontend.update(parameters.clone());
        }
        if let Some((_, shared)) = self.inner.remote.borrow().as_ref() {
            crate::surface_session::hand_off(shared, &parameters, self.inner.update_generation.get());
        }
        let session = self.inner.session.get();
        if session != 0 && self.inner.mode == MapMode::Continuous {
            // Not now: the map is mid-update. Demands waiting for this
            // update render after it.
            RunLoop::get().invoke(move || process_demands(session));
        }
    }
}

// MARK: - Sessions

/// The thread-safe part of a session, which the handle table holds.
pub struct SessionShared {
    pub map: Arc<MapShared>,
    pub(crate) state: Mutex<SessionPublic>,
    pub(crate) demands: Mutex<Vec<mln_frame_demand>>,
    pub(crate) results: Mutex<Vec<mln_render_frame_result>>,
    pub(crate) frame_wake: Option<Wake>,
    pub(crate) frame_wake_armed: AtomicBool,
    pub(crate) frame_generation: AtomicU64,
    /// The thread that renders: MLN_RENDER_DRIVER_CORE_WORKER (offscreen)
    /// or MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD (a WebGPU surface).
    pub(crate) driver: u32,
    pub(crate) caller: Option<crate::surface_session::CallerShared>,
}

pub(crate) struct SessionPublic {
    pub(crate) state: u32,
    pub(crate) extent: mln_logical_extent,
    pub(crate) failure_status: i32,
    pub(crate) failure: String,
}

impl SessionShared {
    pub(crate) fn set_state(&self, state: u32) {
        self.state.lock().unwrap().state = state;
    }

    pub(crate) fn state(&self) -> u32 {
        self.state.lock().unwrap().state
    }

    fn is_caller(&self) -> bool {
        self.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
    }

    pub(crate) fn fail(&self, message: String) {
        let mut public = self.state.lock().unwrap();
        public.state = MLN_RENDER_SESSION_STATE_FAILED;
        public.failure_status = MLN_STATUS_NATIVE_ERROR;
        public.failure = message;
    }

    /// Queues a frame result and wakes the caller, once per drain.
    pub(crate) fn push_result(&self, mut result: mln_render_frame_result) {
        result.size = size_of::<mln_render_frame_result>() as u32;
        result.generation = self.map.generation.load(Ordering::SeqCst);
        self.results.lock().unwrap().push(result);
        if self.frame_wake_armed.swap(false, Ordering::SeqCst) {
            if let Some(wake) = &self.frame_wake {
                wake.signal();
            }
        }
    }

    fn check_live(&self) -> Result<(), Failure> {
        let public = self.state.lock().unwrap();
        match public.state {
            MLN_RENDER_SESSION_STATE_FAILED => Err(Failure::new(public.failure_status, public.failure.clone())),
            MLN_RENDER_SESSION_STATE_ATTACHING | MLN_RENDER_SESSION_STATE_ATTACHED => Ok(()),
            _ => Err(Failure::invalid_state("the session is detached")),
        }
    }
}

/// The runtime thread's part of a session.
pub struct SessionEntry {
    pub shared: Arc<SessionShared>,
    pub frontend: HeadlessFrontend,
    rendered_update_generation: Cell<u64>,
    frames_rendered: Cell<u64>,
    waiting: RefCell<Vec<mln_frame_demand>>,
}

pub struct FrameBatch {
    results: Vec<mln_render_frame_result>,
}

fn session_entry(handle: u64) -> Option<Rc<SessionEntry>> {
    STATE.with(|state| state.borrow().sessions.get(&handle).cloned())
}

fn frame_result(entry: &SessionEntry, map: Option<&MapEntry>, disposition: u32, token: u64) -> mln_render_frame_result {
    mln_render_frame_result {
        disposition,
        token,
        map_update_generation: map.map(|m| m.frontend.update_generation()).unwrap_or(0),
        frame_generation: entry.shared.frame_generation.load(Ordering::SeqCst),
        view: map.map(|m| m.view()).unwrap_or_default(),
        mode: MLN_RENDER_MODE_FULL,
        ..Default::default()
    }
}

/// Renders one frame of a continuous map; `false` when the render panicked
/// (the session then fails).
fn render_frame(entry: &SessionEntry, map: &MapEntry) -> bool {
    let generation = map.frontend.update_generation();
    match catch_unwind(AssertUnwindSafe(|| entry.frontend.render_frame())) {
        Ok(()) => {
            entry.rendered_update_generation.set(generation);
            entry.frames_rendered.set(entry.frames_rendered.get() + 1);
            entry.shared.frame_generation.fetch_add(1, Ordering::SeqCst);
            map.publish_snapshot();
            true
        }
        Err(panic) => {
            entry.shared.fail(crate::diagnostics::panic_message(&panic));
            false
        }
    }
}

/// Runs the session's pending frame demands (on the runtime thread).
fn process_demands(session: u64) {
    let Some(entry) = session_entry(session) else {
        return;
    };
    let map = map::entry(entry.shared.map.handle);
    let incoming = std::mem::take(&mut *entry.shared.demands.lock().unwrap());
    let mut pending = std::mem::take(&mut *entry.waiting.borrow_mut());
    for demand in incoming {
        // A demand without FORCE replaces each pending demand with the same
        // flags.
        if demand.flags & MLN_FRAME_DEMAND_FORCE == 0 {
            pending.retain(|earlier| {
                if earlier.flags == demand.flags {
                    entry.shared.push_result(frame_result(&entry, map.as_deref(), MLN_RENDER_RESULT_SUPERSEDED, earlier.token));
                    false
                } else {
                    true
                }
            });
        }
        pending.push(demand);
    }
    let mut waiting = Vec::new();
    for demand in pending {
        if entry.shared.state() == MLN_RENDER_SESSION_STATE_FAILED {
            entry.shared.push_result(frame_result(&entry, map.as_deref(), MLN_RENDER_RESULT_FAILED, demand.token));
            continue;
        }
        let Some(map) = map.as_deref() else {
            entry.shared.push_result(frame_result(&entry, None, MLN_RENDER_RESULT_TARGET_NOT_READY, demand.token));
            continue;
        };
        if map.map_mode() == MapMode::Continuous && !map.frontend.has_update() {
            map.map.borrow_mut().trigger_repaint();
        }
        let updated = map.frontend.update_generation() > entry.rendered_update_generation.get();
        let forced = demand.flags & MLN_FRAME_DEMAND_FORCE != 0;
        if map.frontend.has_update() && (forced || updated) {
            let disposition =
                if render_frame(&entry, map) { MLN_RENDER_RESULT_RENDERED } else { MLN_RENDER_RESULT_FAILED };
            entry.shared.push_result(frame_result(&entry, Some(map), disposition, demand.token));
        } else if demand.flags & MLN_FRAME_DEMAND_WAIT_FOR_UPDATE != 0 {
            waiting.push(demand);
        } else {
            entry.shared.push_result(frame_result(&entry, Some(map), MLN_RENDER_RESULT_NO_UPDATE, demand.token));
        }
    }
    *entry.waiting.borrow_mut() = waiting;
}

/// Ends a session on the runtime thread: pending demands finish with
/// TARGET_NOT_READY, and the map lets go of the frontend.
fn end_session(session: u64, state: u32) {
    let entry = STATE.with(|s| s.borrow_mut().sessions.remove(&session));
    let Some(entry) = entry else {
        return;
    };
    let map = map::entry(entry.shared.map.handle);
    let mut demands = std::mem::take(&mut *entry.shared.demands.lock().unwrap());
    demands.extend(entry.waiting.borrow_mut().drain(..));
    for demand in demands {
        entry.shared.push_result(frame_result(&entry, map.as_deref(), MLN_RENDER_RESULT_TARGET_NOT_READY, demand.token));
    }
    if let Some(map) = &map {
        map.frontend.detach();
    }
    entry.shared.map.session.compare_exchange(session, 0, Ordering::SeqCst, Ordering::SeqCst).ok();
    entry.shared.set_state(state);
}

/// Abandons `session` without waiting (map disposal).
pub fn abandon(session: u64) {
    if let Ok(shared) = handles::session(session) {
        if shared.is_caller() {
            crate::surface_session::abandon(session, &shared);
            return;
        }
        shared.set_state(MLN_RENDER_SESSION_STATE_ABANDONED);
        shared.map.session.compare_exchange(session, 0, Ordering::SeqCst, Ordering::SeqCst).ok();
        shared.map.runtime.post(move || end_session(session, MLN_RENDER_SESSION_STATE_ABANDONED));
    }
}

// MARK: - Offscreen devices

/// # Safety
/// C API contract (offscreen.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_offscreen_device_create(
    options: *const mln_offscreen_device_options,
    out_device: *mut mln_offscreen_device,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if !options.is_null() {
                let Some(options) = read_sized(options) else {
                    return Err(Failure::invalid_argument("options must be an mln_offscreen_device_options"));
                };
                let known = MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX
                    | MLN_OFFSCREEN_DEVICE_OPTION_SINGLE_CONTEXT
                    | MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API;
                if options.fields & !known != 0 {
                    return Err(Failure::invalid_argument("options.fields has unknown bits"));
                }
                if options.fields & MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API != 0 {
                    if options.client_api == MLN_OPEN_GL_CLIENT_API_UNSPECIFIED || options.client_api > 2 {
                        return Err(Failure::invalid_argument("options.client_api is not an OpenGL client API"));
                    }
                    return Err(Failure::new(MLN_STATUS_UNSUPPORTED, "a WebGPU device renders with no OpenGL API"));
                }
            }
            if out_device.is_null() || (*out_device).0 != 0 {
                return Err(Failure::invalid_argument("out_device must point at MLN_HANDLE_NULL"));
            }
            // TODO(ffi): share one wgpu device among the device's sessions
            // (needs a HeadlessBackend constructor that takes a device); each
            // session creates its own for now.
            *out_device = handles::insert(Object::OffscreenDevice).into();
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (offscreen.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_offscreen_device_release(device: mln_offscreen_device) {
    if let Some(Object::OffscreenDevice) = handles::get(device) {
        handles::remove(device);
    }
}

/// # Safety
/// C API contract (offscreen.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_attach_offscreen(
    map: mln_map,
    descriptor: *const mln_offscreen_descriptor,
    options: *const mln_render_session_attach_options,
    device: mln_offscreen_device,
    out_session: *mut mln_render_session,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let map_shared = handles::map(map)?;
            let Some(descriptor) = read_sized(descriptor) else {
                return Err(Failure::invalid_argument("descriptor must be an mln_offscreen_descriptor"));
            };
            let Some(options) = read_sized(options) else {
                return Err(Failure::invalid_argument("options must be an mln_render_session_attach_options"));
            };
            if out_session.is_null() {
                return Err(Failure::invalid_argument("out_session must not be null"));
            }
            let size = descriptor.logical_size;
            if size.width == 0 || size.height == 0 {
                return Err(Failure::invalid_argument("descriptor.logical_size must not be empty"));
            }
            if options.driver != MLN_RENDER_DRIVER_CORE_WORKER {
                return Err(Failure::new(MLN_STATUS_UNSUPPORTED, "an offscreen session renders on a core worker"));
            }
            if device.0 != 0 {
                handles::resolve(handles::Kind::OffscreenDevice, device)?;
            }
            let completion = Some(Completion::required(completion)?);
            let handle = handles::reserve(handles::Kind::RenderSession);
            if map_shared.session.compare_exchange(0, handle, Ordering::SeqCst, Ordering::SeqCst).is_err() {
                return Err(Failure::invalid_state("the map already has a render session"));
            }
            let shared = Arc::new(SessionShared {
                map: map_shared.clone(),
                state: Mutex::new(SessionPublic {
                    state: MLN_RENDER_SESSION_STATE_ATTACHING,
                    extent: mln_logical_extent {
                        width: size.width,
                        height: size.height,
                        scale_factor: map_shared.scale_factor,
                    },
                    failure_status: MLN_STATUS_OK,
                    failure: String::new(),
                }),
                demands: Mutex::new(Vec::new()),
                results: Mutex::new(Vec::new()),
                frame_wake: Wake::from_c(&options.frame_wake),
                frame_wake_armed: AtomicBool::new(true),
                frame_generation: AtomicU64::new(0),
                driver: MLN_RENDER_DRIVER_CORE_WORKER,
                caller: None,
            });
            handles::insert_at(handle, Object::Session(shared.clone()));
            *out_session = handle.into();

            map_shared.runtime.post(move || {
                let Some(map) = map::entry(shared.map.handle) else {
                    shared.set_state(MLN_RENDER_SESSION_STATE_DETACHED);
                    if let Some(completion) = completion {
                        completion.fail(MLN_STATUS_CANCELLED, "the map was released");
                    }
                    return;
                };
                let logical = Size::new(size.width, size.height);
                let attached = catch_unwind(AssertUnwindSafe(|| {
                    let _ = map.map.borrow_mut().set_size(logical);
                    map.frontend.attach(handle, logical)
                }));
                match attached {
                    Ok(frontend) => {
                        let entry = Rc::new(SessionEntry {
                            shared: shared.clone(),
                            frontend,
                            rendered_update_generation: Cell::new(0),
                            frames_rendered: Cell::new(0),
                            waiting: RefCell::new(Vec::new()),
                        });
                        STATE.with(|state| state.borrow_mut().sessions.insert(handle, entry));
                        shared.set_state(MLN_RENDER_SESSION_STATE_ATTACHED);
                        map.publish_snapshot();
                        if let Some(completion) = completion {
                            completion.ok();
                        }
                        process_demands(handle);
                    }
                    Err(panic) => {
                        let message = crate::diagnostics::panic_message(&panic);
                        shared.fail(message.clone());
                        shared.map.session.compare_exchange(handle, 0, Ordering::SeqCst, Ordering::SeqCst).ok();
                        if let Some(completion) = completion {
                            completion.fail(MLN_STATUS_NATIVE_ERROR, message);
                        }
                    }
                }
            });
            Ok(())
        })
    }
}

// MARK: - Session calls

/// # Safety
/// C API contract (render_session.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_get_capabilities(
    session: mln_render_session,
    out_capabilities: *mut mln_render_session_capabilities,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::session(session)?;
            let capabilities = mln_render_session_capabilities {
                size: 0,
                driver: shared.driver,
                texture_ring_depth: 0,
                flags: if shared.is_caller() { 0 } else { MLN_RENDER_SESSION_CAPABILITY_READBACK },
                producer_sync_kind: MLN_GPU_SYNC_CPU_COMPLETE,
                consumer_sync_kinds: MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE,
            };
            if !write_sized(out_capabilities, capabilities) {
                return Err(Failure::invalid_argument("out_capabilities must be an mln_render_session_capabilities"));
            }
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (render_session.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_get_snapshot(
    session: mln_render_session,
    out_snapshot: *mut mln_render_session_snapshot,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::session(session)?;
            let snapshot = {
                let public = shared.state.lock().unwrap();
                mln_render_session_snapshot {
                    size: 0,
                    state: public.state,
                    extent: public.extent,
                    generation: shared.map.generation.load(Ordering::SeqCst),
                    frame_generation: shared.frame_generation.load(Ordering::SeqCst),
                    acquired_frame_count: 0,
                    failure_status: public.failure_status,
                    has_target: public.state == MLN_RENDER_SESSION_STATE_ATTACHED,
                }
            };
            if !write_sized(out_snapshot, snapshot) {
                return Err(Failure::invalid_argument("out_snapshot must be an mln_render_session_snapshot"));
            }
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (render_session.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_get_failure(
    session: mln_render_session,
    out_message: *mut std::ffi::c_char,
    message_capacity: usize,
    out_message_length: *mut usize,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::session(session)?;
            if out_message_length.is_null() {
                return Err(Failure::invalid_argument("out_message_length must not be null"));
            }
            let message = shared.state.lock().unwrap().failure.clone();
            *out_message_length = message.len();
            if !out_message.is_null() && message_capacity > message.len() {
                std::ptr::copy_nonoverlapping(message.as_ptr(), out_message.cast::<u8>(), message.len());
                *out_message.add(message.len()) = 0;
            }
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (render_session.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_demand_frame(
    session: mln_render_session,
    demand: *const mln_frame_demand,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::session(session)?;
            let Some(demand) = read_sized(demand) else {
                return Err(Failure::invalid_argument("demand must be an mln_frame_demand"));
            };
            if demand.flags & !(MLN_FRAME_DEMAND_FORCE | MLN_FRAME_DEMAND_WAIT_FOR_UPDATE) != 0 {
                return Err(Failure::invalid_argument("demand.flags has unknown bits"));
            }
            if demand.flags == MLN_FRAME_DEMAND_FORCE | MLN_FRAME_DEMAND_WAIT_FOR_UPDATE {
                return Err(Failure::invalid_argument("FORCE can't be combined with WAIT_FOR_UPDATE"));
            }
            if demand.token == MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE {
                return Err(Failure::invalid_argument("the still image token is reserved"));
            }
            shared.check_live()?;
            shared.demands.lock().unwrap().push(demand);
            if shared.is_caller() {
                crate::surface_session::wake_driver(&shared);
            } else {
                shared.map.runtime.post(move || process_demands(session.0));
            }
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (render_session.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_drain_frame_results(
    session: mln_render_session,
    out_batch: *mut mln_render_frame_batch,
    out_view: *mut mln_render_frame_batch_view,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::session(session)?;
            if out_batch.is_null() || out_view.is_null() {
                return Err(Failure::invalid_argument("out_batch and out_view must not be null"));
            }
            if ((*out_view).size as usize) < size_of::<mln_render_frame_batch_view>() {
                return Err(Failure::invalid_argument("out_view must be an mln_render_frame_batch_view"));
            }
            let results = {
                let mut results = shared.results.lock().unwrap();
                shared.frame_wake_armed.store(true, Ordering::SeqCst);
                std::mem::take(&mut *results)
            };
            let view_size = (*out_view).size;
            if results.is_empty() {
                *out_batch = mln_render_frame_batch(0);
                *out_view = mln_render_frame_batch_view {
                    size: view_size,
                    result_size: size_of::<mln_render_frame_result>() as u32,
                    results: std::ptr::null(),
                    result_count: 0,
                };
                return Ok(());
            }
            let batch = Arc::new(FrameBatch { results });
            *out_view = mln_render_frame_batch_view {
                size: view_size,
                result_size: size_of::<mln_render_frame_result>() as u32,
                results: batch.results.as_ptr(),
                result_count: batch.results.len(),
            };
            *out_batch = handles::insert(Object::FrameBatch(batch)).into();
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (render_session.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_frame_batch_release(batch: mln_render_frame_batch) {
    if let Some(Object::FrameBatch(_)) = handles::get(batch) {
        handles::remove(batch);
    }
}

/// # Safety
/// C API contract (texture.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_read_texture(
    session: mln_render_session,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::session(session)?;
            shared.check_live()?;
            if shared.is_caller() {
                return Err(Failure::new(
                    MLN_STATUS_UNSUPPORTED,
                    "a WebGPU surface session presents its frames to the surface",
                ));
            }
            let completion = Completion::required(completion)?;
            shared.map.runtime.clone().post(move || {
                let Some(entry) = session_entry(session.0) else {
                    completion.fail(MLN_STATUS_TARGET_LOST, "the session is detached");
                    return;
                };
                if entry.frames_rendered.get() == 0 {
                    completion.fail(MLN_STATUS_INVALID_STATE, "the session hasn't rendered a frame");
                    return;
                }
                let image = entry.frontend.read_still_image();
                let info = mln_texture_image_info {
                    width: image.size.width,
                    height: image.size.height,
                    stride: image.size.width * 4,
                    byte_length: image.data.len(),
                };
                completion.ok_with(0, Value::Readback { pixels: image.data, info });
            });
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_request_still_image(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::map(map)?;
            if shared.mode == MLN_MAP_MODE_CONTINUOUS {
                return Err(Failure::invalid_state("only a static or tile map renders still images"));
            }
            let completion = Completion::required(completion)?;
            shared.runtime.clone().post(move || {
                let Some(entry) = map::entry(map.0) else {
                    completion.fail(MLN_STATUS_CANCELLED, "the map was released");
                    return;
                };
                let session = entry.shared.session.load(Ordering::SeqCst);
                if session_entry(session).is_none() {
                    completion.fail(MLN_STATUS_INVALID_STATE, "attach a render session first");
                    return;
                }
                let runtime = entry.shared.runtime.clone();
                entry.map.borrow_mut().render_still(Box::new(move |error| {
                    let error = error.map(|e| e.to_string());
                    // Not now: this runs at the end of the frame, inside the
                    // renderer.
                    runtime.post(move || still_image_done(session, error, completion));
                }));
            });
            Ok(())
        })
    }
}

fn still_image_done(session: u64, error: Option<String>, completion: Completion) {
    let Some(entry) = session_entry(session) else {
        completion.fail(MLN_STATUS_CANCELLED, "the session ended before the image was done");
        return;
    };
    let map = map::entry(entry.shared.map.handle);
    match error {
        None => {
            entry.frames_rendered.set(entry.frames_rendered.get() + 1);
            entry.shared.frame_generation.fetch_add(1, Ordering::SeqCst);
            if let Some(map) = &map {
                map.publish_snapshot();
            }
            entry.shared.push_result(frame_result(
                &entry,
                map.as_deref(),
                MLN_RENDER_RESULT_RENDERED,
                MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE,
            ));
            completion.ok_at(entry.shared.map.generation.load(Ordering::SeqCst));
        }
        Some(error) => completion.fail(MLN_STATUS_NATIVE_ERROR, error),
    }
}

/// # Safety
/// C API contract (render_session.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_resize(
    session: mln_render_session,
    logical_size: mln_logical_size,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::session(session)?;
            if logical_size.width == 0 || logical_size.height == 0 {
                return Err(Failure::invalid_argument("logical_size must not be empty"));
            }
            shared.check_live()?;
            let completion = Completion::optional(completion)?;
            if shared.is_caller() {
                crate::surface_session::resize(&shared, logical_size, completion);
                return Ok(());
            }
            shared.map.runtime.clone().post(move || {
                let (Some(entry), Some(map)) = (session_entry(session.0), map::entry(shared.map.handle)) else {
                    if let Some(completion) = completion {
                        completion.fail(MLN_STATUS_TARGET_LOST, "the session is detached");
                    }
                    return;
                };
                let size = Size::new(logical_size.width, logical_size.height);
                let mut frontend = entry.frontend.clone();
                frontend.set_size(size);
                let _ = map.map.borrow_mut().set_size(size);
                entry.frames_rendered.set(0);
                {
                    let extent = &mut shared.state.lock().unwrap().extent;
                    extent.width = logical_size.width;
                    extent.height = logical_size.height;
                }
                map.publish_snapshot();
                if let Some(completion) = completion {
                    completion.ok();
                }
            });
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (render_session.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_barrier(
    session: mln_render_session,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::session(session)?;
            shared.check_live()?;
            let completion = Completion::required(completion)?;
            if shared.is_caller() {
                crate::surface_session::barrier(&shared, completion);
                return Ok(());
            }
            shared.map.runtime.clone().post(move || {
                if let Some(entry) = session_entry(session.0) {
                    let map = map::entry(entry.shared.map.handle);
                    for demand in entry.waiting.borrow_mut().drain(..) {
                        entry.shared.push_result(frame_result(
                            &entry,
                            map.as_deref(),
                            MLN_RENDER_RESULT_NO_UPDATE,
                            demand.token,
                        ));
                    }
                }
                completion.ok();
            });
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (render_session.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_detach(
    session: mln_render_session,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::session(session)?;
            match shared.state() {
                MLN_RENDER_SESSION_STATE_ATTACHING | MLN_RENDER_SESSION_STATE_ATTACHED => {}
                _ => return Err(Failure::invalid_state("the session is not attached")),
            }
            let completion = Completion::optional(completion)?;
            if shared.is_caller() {
                crate::surface_session::detach(session.0, &shared, completion);
                return Ok(());
            }
            shared.set_state(MLN_RENDER_SESSION_STATE_DETACHING);
            shared.map.runtime.post(move || {
                end_session(session.0, MLN_RENDER_SESSION_STATE_DETACHED);
                if let Some(completion) = completion {
                    completion.ok();
                }
            });
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (render_session.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_dispose(
    session: mln_render_session,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::session(session)?;
            let completion = Completion::optional(completion)?;
            handles::remove(session);
            let attached =
                matches!(shared.state(), MLN_RENDER_SESSION_STATE_ATTACHING | MLN_RENDER_SESSION_STATE_ATTACHED);
            if shared.is_caller() {
                // The next service frees the graphics objects, then this
                // completes.
                if attached {
                    crate::surface_session::detach(session.0, &shared, completion);
                } else if let Some(completion) = completion {
                    completion.ok();
                }
                return Ok(());
            }
            // Abandoned as mln_render_session_abandon() does: the map is free
            // for another session, or its release, once this returns.
            if attached {
                shared.map.session.compare_exchange(session.0, 0, Ordering::SeqCst, Ordering::SeqCst).ok();
            }
            let runtime = shared.map.runtime.clone();
            if runtime.is_released() {
                if let Some(completion) = completion {
                    completion.ok();
                }
                return Ok(());
            }
            runtime.post(move || {
                if attached {
                    end_session(session.0, MLN_RENDER_SESSION_STATE_DETACHED);
                }
                if let Some(completion) = completion {
                    completion.ok();
                }
            });
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (render_session.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_abandon(
    session: mln_render_session,
    out_result: *mut mln_render_abandon_result,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            handles::session(session)?;
            abandon(session.0);
            let result = mln_render_abandon_result::default();
            if !out_result.is_null() && !write_sized(out_result, result) {
                return Err(Failure::invalid_argument("out_result must be an mln_render_abandon_result"));
            }
            Ok(())
        })
    }
}
