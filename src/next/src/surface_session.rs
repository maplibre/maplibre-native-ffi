// WebGPU surface sessions (surface.h `mln_map_attach_web_gpu_surface`,
// render_session.h `mln_render_session_service_driver_work`): the host owns
// the WebGPU instance, device and surface, which in a browser belong to the
// host's thread, so the session's renderer lives on that thread too and
// renders when the host services it (MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD).
//
// The map stays on the runtime thread. Its ProxyFrontend copies each
// UpdateParameters into the session (`CallerShared::update`) and wakes the
// host; the renderer's observer posts back to the runtime thread. The
// renderer's own actor replies (tile workers) arrive on the host thread's
// RunLoop, which each service runs, and they wake the host too.
//
// Deviations: a style image the renderer misses is reported to the map, but
// the renderer goes on without waiting for it; the session presents to the
// surface, so it reads no textures back.

use crate::delivery::{Completion, Wake};
use crate::diagnostics::{Failure, boundary};
use crate::handles::{self, Object};
use crate::map::{self, MapShared};
use crate::session::{SessionPublic, SessionShared};
use crate::types::*;
use maplibre_native_next::mln_actor::RunLoop;
use maplibre_native_next::mln_gfx::{BackendScope, RendererBackend, ScopeType};
use maplibre_native_next::mln_renderer::{RenderMode, Renderer, RendererObserver, UpdateParameters};
use maplibre_native_next::mln_util::exception::ExceptionPtr;
use maplibre_native_next::mln_util::size::Size;
use maplibre_native_next::mln_webgpu::SurfaceBackend;
use std::cell::RefCell;
use std::collections::HashMap;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::ptr::NonNull;
use std::rc::{Rc, Weak};
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::{Arc, Mutex};

/// The thread-safe part of a surface session (`SessionShared::caller`).
pub struct CallerShared {
    // The host's WGPUInstance, WGPUDevice and WGPUSurface, which only its
    // thread may use: wrapped there, at the first service.
    instance: usize,
    device: usize,
    surface: usize,
    /// A `WGPUTextureFormat` that `surface_texture_format` accepts.
    format: u32,
    /// The map's latest update and its generation, for the host thread.
    update: Mutex<Option<(UpdateParameters, u64)>>,
    /// A logical size to resize to at the next service.
    resize: Mutex<Option<(MlnLogicalSize, Option<Completion>)>>,
    driver_wake: Option<Wake>,
    driver_wake_armed: AtomicBool,
    /// Whether a service created the renderer (and graphics objects).
    created: AtomicBool,
    attach: Mutex<Option<Completion>>,
    /// Detach and dispose completions, which complete once a service freed
    /// the graphics objects.
    ending: Mutex<Vec<Completion>>,
    barriers: Mutex<Vec<Completion>>,
}

/// Wakes the host for a service, once until the next one.
pub fn wake_driver(shared: &SessionShared) {
    let Some(caller) = &shared.caller else {
        return;
    };
    if caller.driver_wake_armed.swap(false, Ordering::SeqCst) {
        if let Some(wake) = &caller.driver_wake {
            wake.signal();
        }
    }
}

/// The map's frontend hands each update to the session (runtime thread).
pub fn hand_off(shared: &SessionShared, parameters: &UpdateParameters, generation: u64) {
    if let Some(caller) = &shared.caller {
        *caller.update.lock().unwrap() = Some((parameters.clone(), generation));
        wake_driver(shared);
    }
}

// MARK: - The renderer's observer

/// Forwards the renderer's observer calls to the map's observer on the
/// runtime thread.
struct RemoteObserver {
    map: Arc<MapShared>,
}

impl RemoteObserver {
    fn forward(&self, call: impl FnOnce(&dyn RendererObserver) + Send + 'static) {
        let handle = self.map.handle;
        self.map.runtime.post(move || {
            if let Some(map) = map::entry(handle) {
                if let Some(observer) = map.frontend.observer() {
                    call(&*observer);
                }
            }
        });
    }
}

impl RendererObserver for RemoteObserver {
    fn on_invalidate(&self) {
        self.forward(|observer| observer.on_invalidate());
    }

    fn on_resource_error(&self, error: ExceptionPtr) {
        self.forward(move |observer| observer.on_resource_error(error));
    }

    fn on_will_start_rendering_map(&self) {
        self.forward(|observer| observer.on_will_start_rendering_map());
    }

    fn on_will_start_rendering_frame(&self) {
        self.forward(|observer| observer.on_will_start_rendering_frame());
    }

    fn on_did_finish_rendering_frame(&self, mode: RenderMode, repaint: bool, placement_changed: bool) {
        self.forward(move |observer| observer.on_did_finish_rendering_frame(mode, repaint, placement_changed));
    }

    fn on_did_finish_rendering_map(&self) {
        self.forward(|observer| observer.on_did_finish_rendering_map());
    }

    fn on_style_image_missing(&self, id: &str, done: &mut dyn FnMut()) {
        let id = id.to_owned();
        self.forward(move |observer| observer.on_style_image_missing(&id, &mut || {}));
        done();
    }

    fn on_remove_unused_style_images(&self, ids: &[String]) {
        let ids = ids.to_vec();
        self.forward(move |observer| observer.on_remove_unused_style_images(&ids));
    }

    fn on_render_error(&self, error: ExceptionPtr) {
        self.forward(move |observer| observer.on_render_error(error));
    }
}

// MARK: - The host thread's part

/// A session's renderer and graphics objects, on the host's thread.
struct CallerEntry {
    shared: Arc<SessionShared>,
    run_loop: RunLoop,
    backend: Rc<SurfaceBackend>,
    renderer: Renderer,
    _observer: Rc<RemoteObserver>,
    parameters: Option<Arc<UpdateParameters>>,
    update_generation: u64,
    rendered_generation: u64,
    waiting: Vec<MlnFrameDemand>,
}

thread_local! {
    static ENTRIES: RefCell<HashMap<u64, Rc<RefCell<CallerEntry>>>> = RefCell::new(HashMap::new());
}

fn physical(size: MlnLogicalSize, scale_factor: f64) -> Size {
    Size::new(
        (f64::from(size.width) * scale_factor).round().max(1.0) as u32,
        (f64::from(size.height) * scale_factor).round().max(1.0) as u32,
    )
}

/// Creates the session's renderer on this thread over the host's objects.
fn create_entry(session: u64, shared: &Arc<SessionShared>) -> Result<CallerEntry, String> {
    let caller = shared.caller.as_ref().expect("a surface session");
    let extent = shared.state.lock().unwrap().extent;
    let scale_factor = shared.map.scale_factor;
    let size = physical(MlnLogicalSize { width: extent.width, height: extent.height }, scale_factor);
    let instance = NonNull::new(caller.instance as *mut std::ffi::c_void).ok_or("no instance")?;
    let device = NonNull::new(caller.device as *mut std::ffi::c_void).ok_or("no device")?;
    let surface = NonNull::new(caller.surface as *mut std::ffi::c_void).ok_or("no surface")?;
    // SAFETY: the host passed these handles, created on this thread (the
    // thread that services the session), and keeps them until the session
    // lets go of them.
    let (instance, device, queue) =
        unsafe { maplibre_native_next::mln_wgpu_webgpu_h::wrap_host_objects(instance, device) };
    // SAFETY: as above.
    let surface = unsafe {
        instance.create_surface_unsafe(maplibre_native_next::mln_wgpu_webgpu_h::host_surface_target(surface))
    }
    .map_err(|error| format!("the WebGPU surface can't be used: {error}"))?;
    let format = maplibre_native_next::mln_wgpu_webgpu_h::surface_texture_format(caller.format)
        .ok_or("an unsupported surface format")?;
    let backend = Rc::new(SurfaceBackend::from_host(instance, device, queue, surface, format, size));

    let run_loop = RunLoop::get();
    let weak = Arc::downgrade(shared);
    run_loop.set_waker(Some(Arc::new(move || {
        if let Some(shared) = weak.upgrade() {
            wake_driver(&shared);
        }
    })));
    let renderer_backend: Rc<dyn RendererBackend> = backend.clone();
    let mut renderer = Renderer::new(renderer_backend, scale_factor as f32);
    let observer = Rc::new(RemoteObserver { map: shared.map.clone() });
    let weak_observer: Weak<dyn RendererObserver> = Rc::downgrade(&observer) as Weak<dyn RendererObserver>;
    renderer.set_observer(Some(weak_observer));
    let _ = session;
    Ok(CallerEntry {
        shared: shared.clone(),
        run_loop,
        backend,
        renderer,
        _observer: observer,
        parameters: None,
        update_generation: 0,
        rendered_generation: 0,
        waiting: Vec::new(),
    })
}

fn frame_result(entry: &CallerEntry, disposition: u32, token: u64) -> MlnRenderFrameResult {
    MlnRenderFrameResult {
        disposition,
        token,
        map_update_generation: entry.update_generation,
        frame_generation: entry.shared.frame_generation.load(Ordering::SeqCst),
        view: entry.shared.map.view(),
        mode: MLN_RENDER_MODE_FULL,
        ..Default::default()
    }
}

/// Renders the latest update to the surface; `false` when that failed.
fn render(entry: &mut CallerEntry) -> bool {
    let Some(parameters) = entry.parameters.clone() else {
        return false;
    };
    let rendered = catch_unwind(AssertUnwindSafe(|| {
        if !entry.backend.acquire_next_texture() {
            return Err("the surface has no texture to render to".to_owned());
        }
        let backend: &dyn RendererBackend = &*entry.backend;
        let _scope = BackendScope::new(backend, ScopeType::Explicit);
        entry.renderer.render(&parameters);
        Ok(())
    }));
    match rendered {
        Ok(Ok(())) => {
            entry.rendered_generation = entry.update_generation;
            entry.shared.frame_generation.fetch_add(1, Ordering::SeqCst);
            let handle = entry.shared.map.handle;
            entry.shared.map.runtime.post(move || {
                if let Some(map) = map::entry(handle) {
                    map.publish_snapshot();
                }
            });
            true
        }
        Ok(Err(message)) => {
            entry.shared.fail(message);
            false
        }
        Err(panic) => {
            entry.shared.fail(crate::diagnostics::panic_message(&panic));
            false
        }
    }
}

/// Runs the session's frame demands on this thread (`process_demands` of
/// the offscreen sessions).
fn process_demands(entry: &mut CallerEntry) {
    let incoming = std::mem::take(&mut *entry.shared.demands.lock().unwrap());
    let mut pending = std::mem::take(&mut entry.waiting);
    for demand in incoming {
        if demand.flags & MLN_FRAME_DEMAND_FORCE == 0 {
            pending.retain(|earlier| {
                if earlier.flags == demand.flags {
                    entry.shared.push_result(frame_result(entry, MLN_RENDER_RESULT_SUPERSEDED, earlier.token));
                    false
                } else {
                    true
                }
            });
        }
        pending.push(demand);
    }
    for demand in pending {
        if entry.shared.state() == MLN_RENDER_SESSION_STATE_FAILED {
            entry.shared.push_result(frame_result(entry, MLN_RENDER_RESULT_FAILED, demand.token));
            continue;
        }
        if entry.parameters.is_none() {
            let handle = entry.shared.map.handle;
            entry.shared.map.runtime.post(move || {
                if let Some(map) = map::entry(handle) {
                    map.map.borrow_mut().trigger_repaint();
                }
            });
        }
        let updated = entry.update_generation > entry.rendered_generation;
        let forced = demand.flags & MLN_FRAME_DEMAND_FORCE != 0;
        if entry.parameters.is_some() && (forced || updated) {
            let disposition = if render(entry) { MLN_RENDER_RESULT_RENDERED } else { MLN_RENDER_RESULT_FAILED };
            entry.shared.push_result(frame_result(entry, disposition, demand.token));
        } else if demand.flags & MLN_FRAME_DEMAND_WAIT_FOR_UPDATE != 0 {
            entry.waiting.push(demand);
        } else {
            entry.shared.push_result(frame_result(entry, MLN_RENDER_RESULT_NO_UPDATE, demand.token));
        }
    }
}

/// Ends a surface session on the runtime thread: the map lets go of it,
/// demands still queued finish with TARGET_NOT_READY, and `completions`
/// complete.
fn end(session: u64, shared: Arc<SessionShared>, state: u32, completions: Vec<Completion>) {
    let runtime = shared.map.runtime.clone();
    runtime.post(move || {
        if let Some(map) = map::entry(shared.map.handle) {
            map.frontend.detach_remote(session);
        }
        shared.map.session.compare_exchange(session, 0, Ordering::SeqCst, Ordering::SeqCst).ok();
        let demands = std::mem::take(&mut *shared.demands.lock().unwrap());
        for demand in demands {
            shared.push_result(MlnRenderFrameResult {
                disposition: MLN_RENDER_RESULT_TARGET_NOT_READY,
                token: demand.token,
                view: shared.map.view(),
                mode: MLN_RENDER_MODE_FULL,
                ..Default::default()
            });
        }
        shared.set_state(state);
        for completion in completions {
            completion.ok();
        }
    });
}

/// Frees the session's graphics objects, if this thread made them.
fn drop_entry(session: u64) -> Option<Rc<RefCell<CallerEntry>>> {
    ENTRIES.with(|entries| entries.borrow_mut().remove(&session))
}

// MARK: - Session calls of render_session.rs

/// `mln_render_session_detach` of a surface session.
pub fn detach(session: u64, shared: &Arc<SessionShared>, completion: Option<Completion>) {
    let caller = shared.caller.as_ref().expect("a surface session");
    shared.set_state(MLN_RENDER_SESSION_STATE_DETACHING);
    caller.ending.lock().unwrap().extend(completion);
    if !caller.created.load(Ordering::SeqCst) {
        let completions = std::mem::take(&mut *caller.ending.lock().unwrap());
        end(session, shared.clone(), MLN_RENDER_SESSION_STATE_DETACHED, completions);
        return;
    }
    wake_driver(shared);
}

/// `mln_render_session_abandon` and map disposal of a surface session: the
/// graphics objects go at the next service, if there is one.
pub fn abandon(session: u64, shared: &Arc<SessionShared>) {
    shared.set_state(MLN_RENDER_SESSION_STATE_ABANDONED);
    end(session, shared.clone(), MLN_RENDER_SESSION_STATE_ABANDONED, Vec::new());
    wake_driver(shared);
}

/// `mln_render_session_resize` of a surface session.
pub fn resize(shared: &Arc<SessionShared>, logical_size: MlnLogicalSize, completion: Option<Completion>) {
    {
        let mut public = shared.state.lock().unwrap();
        public.extent.width = logical_size.width;
        public.extent.height = logical_size.height;
    }
    *shared.caller.as_ref().expect("a surface session").resize.lock().unwrap() = Some((logical_size, completion));
    let handle = shared.map.handle;
    let size = Size::new(logical_size.width, logical_size.height);
    shared.map.runtime.post(move || {
        if let Some(map) = map::entry(handle) {
            let _ = map.map.borrow_mut().set_size(size);
            map.publish_snapshot();
        }
    });
    wake_driver(shared);
}

/// `mln_render_session_barrier` of a surface session.
pub fn barrier(shared: &Arc<SessionShared>, completion: Completion) {
    shared.caller.as_ref().expect("a surface session").barriers.lock().unwrap().push(completion);
    wake_driver(shared);
}

// MARK: - C API

/// # Safety
/// C API contract (surface.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_attach_web_gpu_surface(
    map: u64,
    descriptor: *const MlnWebGpuSurfaceDescriptor,
    options: *const MlnRenderSessionAttachOptions,
    out_session: *mut u64,
    completion: *const MlnCompletion,
    out_diagnostic: *mut MlnDiagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let map_shared = handles::map(map)?;
            let Some(descriptor) = read_sized(descriptor) else {
                return Err(Failure::invalid_argument("descriptor must be an mln_web_gpu_surface_descriptor"));
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
            if descriptor.context.device.is_null() || descriptor.surface.is_null() {
                return Err(Failure::invalid_argument("descriptor.context.device and descriptor.surface are required"));
            }
            if descriptor.context.instance.is_null() {
                return Err(Failure::invalid_argument(
                    "descriptor.context.instance must not be null: the session waits on it (create it with \
                     WGPUInstanceFeatureName_TimedWaitAny)",
                ));
            }
            if maplibre_native_next::mln_wgpu_webgpu_h::surface_texture_format(descriptor.format).is_none() {
                return Err(Failure::invalid_argument(
                    "descriptor.format must be BGRA8Unorm, RGBA8Unorm or RGBA16Float",
                ));
            }
            if options.driver != MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD {
                return Err(Failure::new(
                    MLN_STATUS_UNSUPPORTED,
                    "a WebGPU surface session renders on your thread: MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD",
                ));
            }
            if map_shared.mode != MLN_MAP_MODE_CONTINUOUS {
                return Err(Failure::new(MLN_STATUS_UNSUPPORTED, "only a continuous map renders to a WebGPU surface"));
            }
            if !cfg!(target_os = "emscripten") {
                return Err(Failure::new(MLN_STATUS_UNSUPPORTED, "WebGPU surfaces need the browser build"));
            }
            let completion = Completion::from_c(completion, false)?;
            let handle = handles::reserve(handles::Kind::RenderSession);
            if map_shared.session.compare_exchange(0, handle, Ordering::SeqCst, Ordering::SeqCst).is_err() {
                return Err(Failure::invalid_state("the map already has a render session"));
            }
            let shared = Arc::new(SessionShared {
                map: map_shared.clone(),
                state: Mutex::new(SessionPublic {
                    state: MLN_RENDER_SESSION_STATE_ATTACHING,
                    extent: MlnLogicalExtent {
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
                frame_wake_armed: Mutex::new(true),
                frame_generation: AtomicU64::new(0),
                driver: MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD,
                caller: Some(CallerShared {
                    instance: descriptor.context.instance as usize,
                    device: descriptor.context.device as usize,
                    surface: descriptor.surface as usize,
                    format: descriptor.format,
                    update: Mutex::new(None),
                    resize: Mutex::new(None),
                    driver_wake: Wake::from_c(&options.driver_work_wake),
                    driver_wake_armed: AtomicBool::new(true),
                    created: AtomicBool::new(false),
                    attach: Mutex::new(completion),
                    ending: Mutex::new(Vec::new()),
                    barriers: Mutex::new(Vec::new()),
                }),
            });
            handles::insert_at(handle, Object::Session(shared.clone()));
            *out_session = handle;

            map_shared.runtime.post(move || {
                let Some(map) = map::entry(shared.map.handle) else {
                    shared.set_state(MLN_RENDER_SESSION_STATE_DETACHED);
                    let completion = shared.caller.as_ref().and_then(|c| c.attach.lock().unwrap().take());
                    if let Some(completion) = completion {
                        completion.fail(MLN_STATUS_CANCELLED, "the map was released");
                    }
                    return;
                };
                let _ = map.map.borrow_mut().set_size(Size::new(size.width, size.height));
                map.frontend.attach_remote(handle, shared.clone());
                map.publish_snapshot();
                wake_driver(&shared);
            });
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (render_session.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_service_driver_work(
    session: u64,
    out_diagnostic: *mut MlnDiagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let existing = ENTRIES.with(|entries| entries.borrow().get(&session).cloned());
            let shared = match (&existing, handles::session(session)) {
                (_, Ok(shared)) => shared,
                // Disposed: the service after the disposal frees the
                // graphics objects.
                (Some(entry), Err(_)) => entry.borrow().shared.clone(),
                (None, Err(failure)) => return Err(failure),
            };
            let Some(caller) = &shared.caller else {
                return Err(Failure::invalid_state("a core worker drives the session"));
            };
            caller.driver_wake_armed.store(true, Ordering::SeqCst);

            match shared.state() {
                MLN_RENDER_SESSION_STATE_DETACHING => {
                    drop(existing);
                    drop_entry(session);
                    let completions = std::mem::take(&mut *caller.ending.lock().unwrap());
                    end(session, shared.clone(), MLN_RENDER_SESSION_STATE_DETACHED, completions);
                    return Ok(());
                }
                MLN_RENDER_SESSION_STATE_DETACHED | MLN_RENDER_SESSION_STATE_ABANDONED => {
                    drop(existing);
                    drop_entry(session);
                    return Ok(());
                }
                _ => {}
            }

            let entry = match existing {
                Some(entry) => entry,
                None => {
                    let created = catch_unwind(AssertUnwindSafe(|| create_entry(session, &shared)));
                    let attach = caller.attach.lock().unwrap().take();
                    let result = match created {
                        Ok(result) => result,
                        Err(panic) => Err(crate::diagnostics::panic_message(&panic)),
                    };
                    match result {
                        Ok(entry) => {
                            let entry = Rc::new(RefCell::new(entry));
                            ENTRIES.with(|entries| entries.borrow_mut().insert(session, entry.clone()));
                            caller.created.store(true, Ordering::SeqCst);
                            shared.set_state(MLN_RENDER_SESSION_STATE_ATTACHED);
                            if let Some(completion) = attach {
                                completion.ok();
                            }
                            entry
                        }
                        Err(message) => {
                            shared.fail(message.clone());
                            if let Some(completion) = attach {
                                completion.fail(MLN_STATUS_NATIVE_ERROR, message);
                            }
                            end(session, shared.clone(), MLN_RENDER_SESSION_STATE_FAILED, Vec::new());
                            return Ok(());
                        }
                    }
                }
            };

            let mut entry = entry.borrow_mut();
            entry.run_loop.run_once();
            if let Some((parameters, generation)) = caller.update.lock().unwrap().take() {
                entry.parameters = Some(Arc::new(parameters));
                entry.update_generation = generation;
            }
            if let Some((size, completion)) = caller.resize.lock().unwrap().take() {
                entry.backend.set_framebuffer_size(physical(size, shared.map.scale_factor));
                if let Some(completion) = completion {
                    completion.ok();
                }
            }
            process_demands(&mut entry);
            let barriers = std::mem::take(&mut *caller.barriers.lock().unwrap());
            if !barriers.is_empty() {
                for demand in std::mem::take(&mut entry.waiting) {
                    entry.shared.push_result(frame_result(&entry, MLN_RENDER_RESULT_NO_UPDATE, demand.token));
                }
                for completion in barriers {
                    completion.ok();
                }
            }
            Ok(())
        })
    }
}
