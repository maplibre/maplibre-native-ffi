// A runtime is one thread that owns an Orbis runtime (its GPU device and
// services) and the runtime's maps and render sessions, all of them `!Send`.
// Calls from any thread check their arguments on the caller's thread, then
// post the work to the runtime thread, and report through their completion.
// Synchronous calls (snapshots) wait for the runtime thread, or run in place
// on it.

use std::cell::RefCell;
use std::collections::HashMap;
use std::sync::mpsc::{self, Receiver, Sender};
use std::sync::{Arc, Mutex};
use std::thread::{JoinHandle, ThreadId};
use std::time::Instant;

use crate::base::{Failure, Result, UserData, boundary, put, sized, text};
use crate::delivery::{self, Completion, Value};
use crate::handles::{self, Object};
use crate::map::MapEntry;
use crate::session::SessionEntry;
use crate::types::*;

type Job = Box<dyn FnOnce(&mut State) + Send>;

pub enum Msg {
    Run(Job),
    /// Data arrived for a map, or a pacer ticked: look for frames to draw.
    Pump,
    Stop(Option<Completion>),
}

/// What the runtime thread owns.
pub struct State {
    pub runtime: orb_api::Runtime,
    pub maps: HashMap<u64, MapEntry>,
    pub sessions: HashMap<u64, SessionEntry>,
    pub handler: Option<EventHandler>,
    /// Frame and input times are milliseconds since this.
    pub epoch: Instant,
    pub id: u64,
    /// Events gathered while the state is busy, sent after.
    pub events: Vec<Event>,
}

thread_local! {
    static STATE: RefCell<Option<State>> = const { RefCell::new(None) };
}

/// The handle's side, shared with every thread.
pub struct RuntimeHandle {
    tx: Sender<Msg>,
    thread: ThreadId,
    join: Mutex<Option<JoinHandle<()>>>,
}

impl RuntimeHandle {
    /// Runs `job` on the runtime thread later, in call order.
    pub fn post(&self, job: impl FnOnce(&mut State) + Send + 'static) -> Result {
        self.tx
            .send(Msg::Run(Box::new(job)))
            .map_err(|_| Failure::state("the runtime was released"))
    }

    /// Runs `job` on the runtime thread and waits for its result. On the
    /// runtime thread itself (in a callback), it runs in place.
    pub fn call<R: Send + 'static>(
        &self,
        job: impl FnOnce(&mut State) -> Result<R> + Send + 'static,
    ) -> Result<R> {
        if std::thread::current().id() == self.thread {
            return STATE.with(|state| match state.try_borrow_mut() {
                Ok(mut state) => job(state.as_mut().expect("runtime state")),
                Err(_) => Err(Failure::state(
                    "the call can't run while the runtime is busy",
                )),
            });
        }
        let (tx, rx) = mpsc::sync_channel(1);
        self.post(move |state| {
            let _ = tx.send(job(state));
        })?;
        rx.recv()
            .map_err(|_| Failure::state("the runtime was released"))?
    }

    pub fn sender(&self) -> Sender<Msg> {
        self.tx.clone()
    }
}

impl State {
    pub fn now(&self) -> f64 {
        self.epoch.elapsed().as_secs_f64() * 1000.0
    }

    /// Queues an event of a map for the handler, if its mask takes it.
    pub fn event(
        &mut self,
        map: u64,
        mask: u64,
        kind: mln_runtime_event_type,
        payload: Payload,
        message: String,
    ) {
        if mask & (1u64 << kind) == 0 || self.handler.is_none() {
            return;
        }
        self.events.push(Event {
            kind,
            map,
            payload,
            message,
        });
    }
}

/// A runtime event, owned until its batch is delivered.
pub struct Event {
    pub kind: mln_runtime_event_type,
    pub map: u64,
    pub payload: Payload,
    pub message: String,
}

pub enum Payload {
    None,
    CameraChange(mln_camera_change_mode),
    LoadingFailed(mln_map_load_error),
    RenderFrame { needs_repaint: bool },
    StyleImage(String),
}

/// A registered `mln_runtime_event_handler`.
pub struct EventHandler {
    callback: unsafe extern "C" fn(*mut std::ffi::c_void, *const mln_event_batch_view),
    user_data: UserData,
    release_user_data: mln_user_data_release,
    queue: u64,
}

impl EventHandler {
    fn send(&self, events: Vec<Event>) {
        let (callback, user_data) = (self.callback, self.user_data);
        delivery::deliver(
            self.queue,
            Box::new(move || {
                let mut events = events;
                let c: Vec<mln_runtime_event> = events
                    .iter_mut()
                    .map(|e| {
                        let mut event = mln_runtime_event {
                            type_: e.kind,
                            source_type: MLN_RUNTIME_EVENT_SOURCE_MAP,
                            source: e.map,
                            message: crate::base::lend(&mut e.message),
                            ..Default::default()
                        };
                        match &mut e.payload {
                            Payload::None => {}
                            Payload::CameraChange(mode) => {
                                event.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_CHANGE;
                                event.payload.camera_change =
                                    mln_runtime_event_camera_change { mode: *mode };
                            }
                            Payload::LoadingFailed(error) => {
                                event.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_LOADING_FAILED;
                                event.payload.loading_failed =
                                    mln_runtime_event_loading_failed { error: *error };
                            }
                            Payload::RenderFrame { needs_repaint } => {
                                event.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME;
                                // SAFETY: plain C data.
                                let mut frame: mln_runtime_event_render_frame =
                                    unsafe { std::mem::zeroed() };
                                frame.mode = MLN_RENDER_MODE_FULL;
                                frame.needs_repaint = *needs_repaint;
                                event.payload.render_frame = frame;
                            }
                            Payload::StyleImage(id) => {
                                event.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE;
                                event.payload.style_image = mln_runtime_event_style_image {
                                    image_id: crate::base::lend(id),
                                };
                            }
                        }
                        event
                    })
                    .collect();
                let batch = mln_event_batch_view {
                    size: std::mem::size_of::<mln_event_batch_view>() as u32,
                    event_size: std::mem::size_of::<mln_runtime_event>() as u32,
                    events: c.as_ptr(),
                    event_count: c.len(),
                };
                // SAFETY: the caller's registration; the batch lives until it
                // returns.
                unsafe { callback(user_data.get(), &batch) };
            }),
        );
    }

    fn release(self) {
        if let Some(release) = self.release_user_data {
            let user_data = self.user_data;
            delivery::deliver(
                self.queue,
                Box::new(move || {
                    // SAFETY: the registration's release, called once.
                    unsafe { release(user_data.get()) }
                }),
            );
        }
    }
}

fn run(rx: Receiver<Msg>) {
    loop {
        let Ok(first) = rx.recv() else { return };
        let mut stop = None;
        for msg in std::iter::once(first).chain(rx.try_iter()) {
            match msg {
                Msg::Run(job) => {
                    STATE.with(|s| job(s.borrow_mut().as_mut().expect("runtime state")))
                }
                Msg::Pump => {}
                Msg::Stop(completion) => {
                    stop = Some(completion);
                    break;
                }
            }
            delivery::flush();
        }
        if let Some(completion) = stop {
            let state = STATE.with(|s| s.borrow_mut().take());
            if let Some(mut state) = state {
                // Sessions before maps before the runtime, as their devices
                // and surfaces need.
                state.sessions.clear();
                state.maps.clear();
                if let Some(handler) = state.handler.take() {
                    handler.release();
                }
            }
            if let Some(completion) = completion {
                completion.complete(Ok(Value::None), 0);
            }
            delivery::flush();
            return;
        }
        STATE.with(|s| {
            let mut s = s.borrow_mut();
            let state = s.as_mut().expect("runtime state");
            crate::session::pump(state);
            let events = std::mem::take(&mut state.events);
            if !events.is_empty()
                && let Some(handler) = &state.handler
            {
                handler.send(events);
            }
        });
        delivery::flush();
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_create(
    options: *const mln_runtime_options,
    out_runtime: *mut mln_runtime,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            if out_runtime.is_null() {
                return Err(Failure::argument("out_runtime is null"));
            }
            let mut state_dir = None;
            if !options.is_null() {
                let options = sized(options, "options")?;
                let cache = text(options.cache_path, "cache_path")?;
                if !cache.is_empty() {
                    // MapLibre Native's cache is one SQLite file; Orbis keeps a
                    // directory beside it.
                    state_dir = Some(std::path::PathBuf::from(format!("{cache}.orbis")));
                }
            }
            let (tx, rx) = mpsc::channel();
            let (ready_tx, ready_rx) = mpsc::sync_channel(1);
            let join = std::thread::Builder::new()
                .name("orbis-runtime".into())
                .spawn(move || {
                    delivery::start_outbox();
                    let options = orb_api::RuntimeOptions {
                        state_dir,
                        ..Default::default()
                    };
                    match orb_api::Runtime::new(&options) {
                        Ok(runtime) => {
                            STATE.with(|s| {
                                *s.borrow_mut() = Some(State {
                                    runtime,
                                    maps: HashMap::new(),
                                    sessions: HashMap::new(),
                                    handler: None,
                                    epoch: Instant::now(),
                                    id: 0,
                                    events: Vec::new(),
                                })
                            });
                            let _ = ready_tx.send(Ok(()));
                            run(rx);
                        }
                        Err(e) => {
                            let _ = ready_tx.send(Err(Failure::from(e)));
                        }
                    }
                })
                .map_err(|e| Failure::native(format!("can't start the runtime thread: {e}")))?;
            let thread = join.thread().id();
            ready_rx
                .recv()
                .map_err(|_| Failure::native("the runtime thread stopped"))??;
            let handle = Arc::new(RuntimeHandle {
                tx,
                thread,
                join: Mutex::new(Some(join)),
            });
            let id = handles::insert(Object::Runtime(handle.clone()));
            handle.post(move |state| state.id = id)?;
            put(out_runtime, mln_runtime(id), "out_runtime")
        })
    }
}

fn stop(runtime: mln_runtime, completion: Option<Completion>, wait: bool) -> Result {
    let handle = handles::runtime(runtime.0)?;
    let has_maps = handle.call(|state| Ok(!state.maps.is_empty()))?;
    if has_maps {
        return Err(Failure::state("the runtime still has maps"));
    }
    handles::remove(runtime.0);
    let _ = handle.tx.send(Msg::Stop(completion));
    if wait && let Some(join) = handle.join.lock().unwrap().take() {
        let _ = join.join();
    }
    Ok(())
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_release(
    runtime: mln_runtime,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            stop(runtime, Completion::from_c(completion)?, false)
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_dispose(
    runtime: mln_runtime,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointer.
    unsafe { boundary(out_diagnostic, || stop(runtime, None, true)) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_set_event_handler(
    runtime: mln_runtime,
    handler: *const mln_runtime_event_handler,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let runtime = handles::runtime(runtime.0)?;
            let h = sized(handler, "handler")?;
            let callback = h
                .callback
                .ok_or_else(|| Failure::argument("handler.callback is null"))?;
            let handler = EventHandler {
                callback,
                user_data: UserData(h.user_data),
                release_user_data: h.release_user_data,
                queue: h.queue.0,
            };
            let completion = Completion::from_c(completion)?;
            runtime.post(move |state| {
                if let Some(old) = state.handler.replace(handler) {
                    old.release();
                }
                if let Some(c) = completion {
                    c.complete(Ok(Value::None), 0);
                }
            })
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_clear_event_handler(
    runtime: mln_runtime,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let runtime = handles::runtime(runtime.0)?;
            let completion = Completion::from_c(completion)?;
            runtime.post(move |state| {
                if let Some(old) = state.handler.take() {
                    old.release();
                }
                if let Some(c) = completion {
                    c.complete(Ok(Value::None), 0);
                }
            })
        })
    }
}
