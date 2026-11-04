// Ports maplibre-native-ffi src/runtime and src/c_api/runtime.cpp (the parts
// this crate implements): a runtime is one thread with a RunLoop that owns the
// runtime's maps and render sessions, as each C++ runtime owns an executor
// thread. Calls from any thread validate their arguments on the caller's
// thread, then post the work to the runtime thread, and report through their
// completion. Map, frontend and renderer objects are `Rc`-based and never
// leave that thread.

use crate::delivery::{Completion, UserData, deliver};
use crate::diagnostics::{Failure, boundary, panic_message};
use crate::handles::{self, Object};
use crate::map::MapEntry;
use crate::resource_loader::{ResourceState, install_file_source_factories};
use crate::session::SessionEntry;
use crate::types::*;
use maplibre_native_next::mln_actor::{RunLoop, RunLoopHandle};
use maplibre_native_next::mln_storage::network_status::{NetworkStatus, Status};
use maplibre_native_next::mln_storage::{FileSource, ResourceOptions};
use std::cell::RefCell;
use std::collections::{HashMap, HashSet};
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::rc::Rc;
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::{Arc, Mutex, mpsc};
use std::thread::JoinHandle;

/// What the runtime thread owns.
#[derive(Default)]
pub struct RuntimeState {
    pub maps: HashMap<u64, Rc<MapEntry>>,
    pub sessions: HashMap<u64, Rc<SessionEntry>>,
    /// The database file source of the offline operations, from the first
    /// one until the runtime closes (offline.rs).
    pub database: Option<Arc<dyn FileSource>>,
}

thread_local! {
    pub static STATE: RefCell<RuntimeState> = RefCell::new(RuntimeState::default());
}

/// A registered `mln_runtime_event_handler`.
struct EventHandler {
    callback: mln_runtime_event_callback,
    user_data: UserData,
    release_user_data: mln_user_data_release,
    queue: u64,
}

impl EventHandler {
    fn release(&self) {
        if let Some(release) = self.release_user_data {
            let user_data = self.user_data;
            deliver(self.queue, Box::new(move || {
                // SAFETY: the registration's release, called once.
                unsafe { release(user_data.get()) };
            }));
        }
    }
}

/// The payloads of the events this crate raises.
pub enum Payload {
    None,
    RenderFrame(mln_runtime_event_render_frame),
    RenderMap(mln_runtime_event_render_map),
    OfflineRegionStatus(mln_runtime_event_offline_region_status),
    OfflineRegionResponseError(mln_runtime_event_offline_region_response_error),
    OfflineRegionTileCountLimit(mln_runtime_event_offline_region_tile_count_limit),
}

/// An event with the data it points at.
pub struct OwnedEvent {
    pub type_: u32,
    pub source_type: u32,
    pub source: u64,
    pub generation: u64,
    pub code: u32,
    pub message: String,
    pub payload: Payload,
}

impl OwnedEvent {
    pub fn map(map: u64, generation: u64, type_: u32) -> OwnedEvent {
        OwnedEvent {
            type_,
            source_type: MLN_RUNTIME_EVENT_SOURCE_MAP,
            source: map,
            generation,
            code: 0,
            message: String::new(),
            payload: Payload::None,
        }
    }
}

pub struct RuntimeShared {
    pub handle: u64,
    loop_handle: RunLoopHandle,
    thread: Mutex<Option<JoinHandle<()>>>,
    pub maps: Mutex<HashSet<u64>>,
    events: Mutex<Option<Arc<EventHandler>>>,
    event_mask: AtomicU64,
    released: AtomicBool,
    pub resource_options: ResourceOptions,
    /// The resource provider and transforms, and the observed offline
    /// regions, which the runtime's file sources read.
    pub resources: Arc<ResourceState>,
}

impl RuntimeShared {
    pub fn is_released(&self) -> bool {
        self.released.load(Ordering::SeqCst)
    }

    /// Runs `task` on the runtime thread. A panic in it is logged and
    /// swallowed: the C API has no caller to report it to there.
    pub fn post(&self, task: impl FnOnce() + Send + 'static) {
        self.loop_handle.invoke(move || {
            if let Err(panic) = catch_unwind(AssertUnwindSafe(task)) {
                eprintln!("maplibre-native-next ffi: {}", panic_message(&panic));
            }
        });
    }

    /// A map of the runtime was released. A disposed runtime finishes its
    /// teardown once its last map is gone.
    pub fn map_released(&self) {
        if self.is_released() && self.maps.lock().unwrap().is_empty() {
            self.loop_handle.stop();
        }
    }

    /// Delivers one event to the event handler, if its mask selects it. Map
    /// events are filtered by their map's mask before they get here.
    pub fn emit(&self, event: OwnedEvent) {
        if event.source_type == MLN_RUNTIME_EVENT_SOURCE_RUNTIME
            && self.event_mask.load(Ordering::SeqCst) & (1u64 << event.type_) == 0
        {
            return;
        }
        let Some(handler) = self.events.lock().unwrap().clone() else {
            return;
        };
        deliver(handler.queue, Box::new(move || {
            let message = LentText::new(event.message);
            let mut payload = mln_runtime_event_payload::default();
            let payload_type = match event.payload {
                Payload::None => MLN_RUNTIME_EVENT_PAYLOAD_NONE,
                Payload::RenderFrame(render_frame) => {
                    payload.render_frame = render_frame;
                    MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME
                }
                Payload::RenderMap(render_map) => {
                    payload.render_map = render_map;
                    MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP
                }
                Payload::OfflineRegionStatus(status) => {
                    payload.offline_region_status = status;
                    MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS
                }
                Payload::OfflineRegionResponseError(error) => {
                    payload.offline_region_response_error = error;
                    MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR
                }
                Payload::OfflineRegionTileCountLimit(limit) => {
                    payload.offline_region_tile_count_limit = limit;
                    MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT
                }
            };
            let c_event = mln_runtime_event {
                type_: event.type_,
                source_type: event.source_type,
                source: event.source,
                generation: event.generation,
                code: event.code,
                payload_type,
                message: mln_buffer_view::of_text(&message),
                payload,
            };
            let batch = mln_event_batch_view {
                size: size_of::<mln_event_batch_view>() as u32,
                event_size: size_of::<mln_runtime_event>() as u32,
                events: &c_event,
                event_count: 1,
            };
            if let Some(callback) = handler.callback {
                // SAFETY: the caller registered this callback and user_data;
                // the batch and what it points at live until it returns.
                unsafe { callback(handler.user_data.get(), &batch) };
            }
        }));
    }
}

fn runtime_thread(ready: mpsc::Sender<RunLoopHandle>, resources: Arc<ResourceState>) {
    let run_loop = RunLoop::new();
    if ready.send(run_loop.handle()).is_err() {
        return;
    }
    run_loop.run();
    // Teardown of whatever release left behind (dispose without release).
    let state = STATE.with(|state| std::mem::take(&mut *state.borrow_mut()));
    drop(state);
    // The resource provider and transforms retire last, on this thread, so
    // a host's release_user_data never runs on the thread that released
    // the runtime.
    resources.retire();
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_create(
    options: *const mln_runtime_options,
    out_runtime: *mut mln_runtime,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if out_runtime.is_null() || (*out_runtime).0 != 0 {
                return Err(Failure::invalid_argument("out_runtime must point to the null handle"));
            }
            if options.is_null() {
                return Err(Failure::invalid_argument("runtime options must not be null"));
            }
            let Some(options) = read_sized(options) else {
                return Err(Failure::invalid_argument("mln_runtime_options.size is too small"));
            };
            let asset_path = crate::url_rules::validate_c_string_view(options.asset_path, "asset_path")?;
            let cache_path = crate::url_rules::validate_c_string_view(options.cache_path, "cache_path")?;
            let asset_path = std::str::from_utf8(asset_path)
                .map_err(|_| Failure::invalid_argument("asset_path is not UTF-8"))?;
            let cache_path = std::str::from_utf8(cache_path)
                .map_err(|_| Failure::invalid_argument("cache_path is not UTF-8"))?;
            // C++ `resource_options_for_runtime`.
            let mut resource_options = ResourceOptions::default_options();
            if !asset_path.is_empty() {
                resource_options = resource_options.with_asset_path(asset_path);
            }
            if !cache_path.is_empty() {
                resource_options = resource_options.with_cache_path(cache_path);
            }
            install_file_source_factories();

            let handle = handles::reserve(handles::Kind::Runtime);
            let resources = ResourceState::new(handle);
            let resource_options = resource_options.with_platform_context(resources.token);
            let (ready_tx, ready_rx) = mpsc::channel();
            let thread_resources = resources.clone();
            let thread = std::thread::Builder::new()
                .name(format!("mln-runtime-{handle}"))
                .spawn(move || runtime_thread(ready_tx, thread_resources))
                .map_err(|e| Failure::new(MLN_STATUS_NATIVE_ERROR, format!("starting the runtime thread failed: {e}")))?;
            let loop_handle = ready_rx
                .recv()
                .map_err(|_| Failure::new(MLN_STATUS_NATIVE_ERROR, "the runtime thread failed to start"))?;
            let runtime = Arc::new(RuntimeShared {
                handle,
                loop_handle,
                thread: Mutex::new(Some(thread)),
                maps: Mutex::new(HashSet::new()),
                events: Mutex::new(None),
                event_mask: AtomicU64::new(MLN_RUNTIME_EVENT_FLAG_ALL),
                released: AtomicBool::new(false),
                resource_options,
                resources,
            });
            handles::insert_at(handle, Object::Runtime(runtime));
            *out_runtime = handle.into();
            Ok(())
        })
    }
}

fn stop_runtime(runtime: &Arc<RuntimeShared>) {
    runtime.released.store(true, Ordering::SeqCst);
    handles::remove(runtime.handle);
    // File sources no longer find the runtime; its registrations retire
    // with its thread.
    runtime.resources.unregister();
    if let Some(handler) = runtime.events.lock().unwrap().take() {
        handler.release();
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_release(
    runtime: mln_runtime,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::runtime(runtime)?;
            if !shared.maps.lock().unwrap().is_empty() {
                return Err(Failure::invalid_state("release the runtime's maps first"));
            }
            let completion = Completion::optional(completion)?;
            stop_runtime(&shared);
            let thread = shared.thread.lock().unwrap().take();
            let loop_handle = shared.loop_handle.clone();
            // The runtime thread can't join itself, so a helper waits for it
            // and then completes.
            std::thread::spawn(move || {
                loop_handle.stop();
                if let Some(thread) = thread {
                    let _ = thread.join();
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
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_dispose(runtime: mln_runtime, out_diagnostic: *mut mln_diagnostic) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::runtime(runtime)?;
            stop_runtime(&shared);
            // The teardown waits for the runtime's maps (map_released).
            if shared.maps.lock().unwrap().is_empty() {
                shared.loop_handle.stop();
            }
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_set_event_handler(
    runtime: mln_runtime,
    handler: *const mln_runtime_event_handler,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::runtime(runtime)?;
            let Some(handler) = read_sized(handler) else {
                return Err(Failure::invalid_argument("handler must be an mln_runtime_event_handler"));
            };
            if handler.callback.is_none() {
                return Err(Failure::invalid_argument("handler.callback must not be null"));
            }
            if handler.queue.0 != 0 {
                handles::queue(handler.queue)?;
            }
            let completion = Completion::optional(completion)?;
            let previous = shared.events.lock().unwrap().replace(Arc::new(EventHandler {
                callback: handler.callback,
                user_data: UserData(handler.user_data),
                release_user_data: handler.release_user_data,
                queue: handler.queue.0,
            }));
            if let Some(previous) = previous {
                previous.release();
            }
            if let Some(completion) = completion {
                completion.ok();
            }
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_clear_event_handler(
    runtime: mln_runtime,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::runtime(runtime)?;
            let completion = Completion::optional(completion)?;
            if let Some(previous) = shared.events.lock().unwrap().take() {
                previous.release();
            }
            if let Some(completion) = completion {
                completion.ok();
            }
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_set_event_mask(runtime: mln_runtime, mask: u64, out_diagnostic: *mut mln_diagnostic) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::runtime(runtime)?;
            if mask & !MLN_RUNTIME_EVENT_FLAG_ALL != 0 {
                return Err(Failure::invalid_argument("mask has unknown event bits"));
            }
            shared.event_mask.store(mask, Ordering::SeqCst);
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_get_event_mask(
    runtime: mln_runtime,
    out_mask: *mut u64,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::runtime(runtime)?;
            if out_mask.is_null() {
                return Err(Failure::invalid_argument("out_mask must not be null"));
            }
            *out_mask = shared.event_mask.load(Ordering::SeqCst);
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_network_get_status(out_status: *mut u32, out_diagnostic: *mut mln_diagnostic) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if out_status.is_null() {
                return Err(Failure::invalid_argument("out_status must not be null"));
            }
            *out_status = match NetworkStatus::get() {
                Status::Online => MLN_NETWORK_STATUS_ONLINE,
                Status::Offline => MLN_NETWORK_STATUS_OFFLINE,
            };
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_network_set_status(status: u32, out_diagnostic: *mut mln_diagnostic) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            match status {
                MLN_NETWORK_STATUS_ONLINE => NetworkStatus::set(Status::Online),
                MLN_NETWORK_STATUS_OFFLINE => NetworkStatus::set(Status::Offline),
                _ => return Err(Failure::invalid_argument("network status is invalid")),
            }
            Ok(())
        })
    }
}

