// Ports the Metal surface sessions of maplibre-native-ffi
// (src/render/metal/metal_surface_session.mm, src/render/surface_session.cpp,
// src/c_api/surface.cpp, and the surface parts of render_session_common.cpp)
// onto mln_webgpu::SurfaceBackend: a wgpu surface on the host's CAMetalLayer.
//
// The session renders on its map's runtime thread, the "core worker" of
// MLN_RENDER_DRIVER_CORE_WORKER, as the offscreen sessions do (session.rs).
// Its frontend takes the map's updates from the map's ProxyFrontend and
// renders one frame per frame demand that has something to draw, and the
// frame presents to the layer.
//
// Deviations:
// - wgpu picks the Metal device (the system's default GPU), so the
//   descriptor's context.device goes unused, and wgpu configures the layer:
//   it makes it opaque, and turns off allowsNextDrawableTimeout.
// - A SurfaceBackend keeps its wgpu surface for life. So a session keeps the
//   layer that mln_render_session_release_target() releases, retained, and
//   draws nothing to it, until a new target replaces it or the session ends;
//   and a replacement with another layer creates a new renderer, whose caches
//   (tiles, glyphs, shaders) load again.
// - Only MLN_RENDER_DRIVER_CORE_WORKER, and only a continuous map.
// - An abandon lets go of the layer on the runtime thread, once the frame
//   that the thread may be rendering is done.

use crate::delivery::{Completion, Wake};
use crate::diagnostics::{Failure, boundary};
use crate::frame_observer::{FrameObserver, render_mode_code};
use crate::handles::{self, Object};
use crate::map::{self, MapEntry};
use crate::session::{SessionPublic, SessionShared};
use crate::types::*;
use maplibre_native_next::mln_actor::RunLoop;
use maplibre_native_next::mln_gfx::{BackendScope, RendererBackend, ScopeType};
use maplibre_native_next::mln_renderer::{
    RenderMode, Renderer, RendererFrontend, RendererObserver, UpdateParameters,
};
use maplibre_native_next::mln_util::size::Size;
use maplibre_native_next::mln_webgpu::SurfaceBackend;
use std::cell::{Cell, RefCell};
use std::collections::HashMap;
use std::ffi::c_void;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::rc::{Rc, Weak};
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::{Arc, Mutex};

/// The thread-safe part of a Metal surface session (`SessionShared::metal`).
#[derive(Default)]
pub struct MetalShared {
    /// From a completed mln_render_session_release_target() until the next
    /// target.
    released: AtomicBool,
}

impl MetalShared {
    pub fn released(&self) -> bool {
        self.released.load(Ordering::SeqCst)
    }
}

/// `physical_dimension`: the logical size at the scale factor, rounded up.
fn physical(size: mln_logical_size, scale_factor: f64) -> Size {
    Size::new(
        (f64::from(size.width) * scale_factor).ceil() as u32,
        (f64::from(size.height) * scale_factor).ceil() as u32,
    )
}

/// `validate_physical_size`.
fn validate_physical(
    size: mln_logical_size,
    scale_factor: f64,
    message: &str,
) -> Result<(), Failure> {
    let max = f64::from(u32::MAX);
    if (f64::from(size.width) * scale_factor).ceil() > max
        || (f64::from(size.height) * scale_factor).ceil() > max
    {
        return Err(Failure::invalid_argument(message));
    }
    Ok(())
}

// MARK: - Objective-C

/// A CAMetalLayer that the session retains, so that it may cross to the
/// runtime thread: Core Animation layers may be used from any thread. Only
/// Apple platforms make one.
#[cfg_attr(not(target_vendor = "apple"), allow(dead_code))]
struct Layer(std::ptr::NonNull<c_void>);

// SAFETY: see the type's comment; the session sends the layer only between
// its own threads, and only to create a surface on it.
unsafe impl Send for Layer {}

impl Layer {
    fn as_ptr(&self) -> *mut c_void {
        self.0.as_ptr()
    }
}

#[cfg(target_vendor = "apple")]
mod objc {
    use std::ffi::{c_char, c_void};

    #[link(name = "objc")]
    unsafe extern "C" {
        fn objc_retain(object: *mut c_void) -> *mut c_void;
        fn objc_release(object: *mut c_void);
        fn objc_getClass(name: *const c_char) -> *mut c_void;
        fn sel_registerName(name: *const c_char) -> *mut c_void;
        fn objc_msgSend();
    }

    /// Retains `object`.
    ///
    /// # Safety
    /// `object` is a live Objective-C object.
    pub unsafe fn retain(object: *mut c_void) {
        // SAFETY: the caller's contract.
        unsafe { objc_retain(object) };
    }

    /// Releases what `retain` retained.
    ///
    /// # Safety
    /// As for `retain`, once per `retain`.
    pub unsafe fn release(object: *mut c_void) {
        // SAFETY: the caller's contract.
        unsafe { objc_release(object) };
    }

    /// Whether `object` is a CAMetalLayer, or an instance of a subclass.
    ///
    /// # Safety
    /// `object` is a live Objective-C object.
    pub unsafe fn is_metal_layer(object: *mut c_void) -> bool {
        // SAFETY: the runtime's lookup functions take NUL-terminated names;
        // objc_msgSend is called through the signature of the method it
        // sends, -[NSObject isKindOfClass:], which returns a BOOL.
        unsafe {
            let class = objc_getClass(c"CAMetalLayer".as_ptr());
            if class.is_null() {
                return false;
            }
            let send: unsafe extern "C" fn(*mut c_void, *mut c_void, *mut c_void) -> i8 =
                std::mem::transmute(objc_msgSend as unsafe extern "C" fn());
            send(object, sel_registerName(c"isKindOfClass:".as_ptr()), class) != 0
        }
    }
}

impl Layer {
    /// Retains the host's layer.
    ///
    /// # Safety
    /// `layer` is a live CAMetalLayer.
    #[cfg(target_vendor = "apple")]
    unsafe fn retain(layer: *mut c_void) -> Option<Layer> {
        let layer = std::ptr::NonNull::new(layer)?;
        // SAFETY: the caller's contract.
        unsafe { objc::retain(layer.as_ptr()) };
        Some(Layer(layer))
    }
}

#[cfg(target_vendor = "apple")]
impl Drop for Layer {
    fn drop(&mut self) {
        // SAFETY: `retain` retained the layer once.
        unsafe { objc::release(self.0.as_ptr()) };
    }
}

/// Checks the layer that a descriptor or a replacement names, once
/// everything else about the call is valid: it is the one check that sends
/// the layer a message.
#[cfg(target_vendor = "apple")]
fn retain_layer(layer: *mut c_void) -> Result<Layer, Failure> {
    // SAFETY: the caller passes a live object (surface.h).
    if !unsafe { objc::is_metal_layer(layer) } {
        return Err(Failure::invalid_argument(
            "Metal surface layer must be a CAMetalLayer",
        ));
    }
    // SAFETY: as above.
    unsafe { Layer::retain(layer) }
        .ok_or_else(|| Failure::invalid_argument("Metal surface layer must not be null"))
}

#[cfg(not(target_vendor = "apple"))]
fn retain_layer(_layer: *mut c_void) -> Result<Layer, Failure> {
    Err(Failure::new(
        MLN_STATUS_UNSUPPORTED,
        "Metal surfaces are not supported by this build",
    ))
}

// MARK: - The runtime thread's part

/// The renderer and the wgpu surface on one layer.
struct Target {
    // Declared first, so it goes before the backend it renders with.
    renderer: Renderer,
    backend: Rc<SurfaceBackend>,
    /// Retained for the identity check of a replacement; the surface retains
    /// the layer too.
    layer: Layer,
}

/// Creates a renderer on a surface on `layer`, sized `size` in pixels.
#[cfg(target_vendor = "apple")]
fn create_target(
    layer: Layer,
    size: Size,
    scale_factor: f64,
    observer: &Rc<FrameObserver>,
) -> Result<Target, String> {
    let raw = layer.as_ptr();
    let backend = SurfaceBackend::new(
        |instance| {
            // SAFETY: `raw` is a CAMetalLayer (retain_layer checked it) that
            // `layer` keeps alive while the surface takes its own reference.
            unsafe {
                instance.create_surface_unsafe(wgpu::SurfaceTargetUnsafe::CoreAnimationLayer(raw))
            }
            .map_err(|error| format!("the Metal layer can't be used: {error}"))
        },
        size,
        true,
    )?;
    let backend = Rc::new(backend);
    let renderer_backend: Rc<dyn RendererBackend> = backend.clone();
    let mut renderer = Renderer::new(renderer_backend, scale_factor as f32);
    let weak: Weak<dyn RendererObserver> = Rc::downgrade(observer) as Weak<dyn RendererObserver>;
    renderer.set_observer(Some(weak));
    Ok(Target {
        renderer,
        backend,
        layer,
    })
}

#[cfg(not(target_vendor = "apple"))]
fn create_target(
    _layer: Layer,
    _size: Size,
    _scale_factor: f64,
    _observer: &Rc<FrameObserver>,
) -> Result<Target, String> {
    Err("Metal surfaces are not supported by this build".to_owned())
}

impl Target {
    /// Frees the renderer and the surface once the GPU is done with the work
    /// on the layer's drawables, so that the host may destroy the layer.
    fn retire(self) {
        let device = self.backend.get_device();
        drop(self);
        #[cfg(target_vendor = "apple")]
        let _ = device.poll(wgpu::PollType::wait_indefinitely());
        #[cfg(not(target_vendor = "apple"))]
        drop(device);
    }
}

/// A Metal surface session on the runtime thread: the map's ProxyFrontend
/// forwards its renderer observer and updates here.
struct Frontend {
    session: u64,
    shared: Arc<SessionShared>,
    observer: Rc<FrameObserver>,
    target: RefCell<Option<Target>>,
    update: RefCell<Option<Arc<UpdateParameters>>>,
    /// The ProxyFrontend's generation of `update`.
    update_generation: Cell<u64>,
    rendered_generation: Cell<u64>,
    /// The camera and extent of the last rendered frame.
    rendered_view: Cell<Option<mln_map_view>>,
    /// How the last rendered frame finished.
    rendered_mode: Cell<u32>,
    needs_repaint: Cell<bool>,
    waiting: RefCell<Vec<mln_frame_demand>>,
    feature_state: RefCell<crate::feature_state::Applied>,
}

thread_local! {
    static FRONTENDS: RefCell<HashMap<u64, Rc<Frontend>>> = RefCell::new(HashMap::new());
}

fn frontend(session: u64) -> Option<Rc<Frontend>> {
    FRONTENDS.with(|frontends| frontends.borrow().get(&session).cloned())
}

/// A Metal surface session's frontend, for the ProxyFrontend.
struct LocalFrontend(Rc<Frontend>);

impl RendererFrontend for LocalFrontend {
    fn reset(&self) {
        let target = self.0.target.borrow_mut().take();
        if let Some(target) = target {
            target.retire();
        }
    }

    fn set_observer(&self, observer: Weak<dyn RendererObserver>) {
        self.0.observer.set_delegate(observer);
    }

    fn update(&self, parameters: Arc<UpdateParameters>) {
        let generation = map::entry(self.0.shared.map.handle)
            .map(|map| map.frontend.update_generation())
            .unwrap_or(self.0.update_generation.get() + 1);
        *self.0.update.borrow_mut() = Some(parameters);
        self.0.update_generation.set(generation);
        let session = self.0.session;
        // Not now: the map is mid-update. Demands waiting for this update
        // render after it.
        RunLoop::get().invoke(move || process_demands(session));
    }
}

/// The view that `parameters` draws: the map's, with the camera and extent
/// of the frame rather than the map's latest, which may be ahead of it.
fn view_of(map: &MapEntry, parameters: &UpdateParameters, scale_factor: f64) -> mln_map_view {
    let size = parameters.transform_state.get_size();
    let mut view = map.view();
    view.camera = map::camera_of(&parameters.transform_state.get_camera_options(None));
    view.extent = mln_logical_extent {
        width: size.width,
        height: size.height,
        scale_factor,
    };
    view
}

fn frame_result(
    f: &Frontend,
    map: Option<&MapEntry>,
    disposition: u32,
    token: u64,
) -> mln_render_frame_result {
    let rendered = disposition == MLN_RENDER_RESULT_RENDERED;
    mln_render_frame_result {
        disposition,
        token,
        map_update_generation: f.update_generation.get(),
        frame_generation: f.shared.frame_generation.load(Ordering::SeqCst),
        needs_repaint: rendered && f.needs_repaint.get(),
        view: match (rendered, f.rendered_view.get()) {
            (true, Some(view)) => view,
            _ => map
                .map(MapEntry::view)
                .unwrap_or_else(|| f.shared.map.view()),
        },
        mode: if rendered {
            f.rendered_mode.get()
        } else {
            MLN_RENDER_MODE_PARTIAL
        },
        ..Default::default()
    }
}

/// What one render did.
enum Rendered {
    Frame,
    /// Nothing new to show: no frame, or an update for another size.
    Nothing,
    /// The layer has no drawable, or the session no layer.
    NotReady,
    Failed,
}

/// Renders `parameters` once: `Ok(true)` when the renderer drew a frame,
/// `Ok(false)` when it drew nothing, `Err(())` when the layer had no
/// drawable to draw it on.
fn render_once(
    f: &Frontend,
    target: &mut Target,
    parameters: &Arc<UpdateParameters>,
    size: Size,
) -> Result<bool, ()> {
    target.backend.set_framebuffer_size(size);
    if !target.backend.acquire_next_texture() {
        return Err(());
    }
    f.observer.begin();
    let backend: &dyn RendererBackend = &*target.backend;
    let _scope = BackendScope::new(backend, ScopeType::Explicit);
    target.renderer.render(parameters);
    Ok(f.observer.finished().is_some())
}

/// Renders the latest update to the layer (C++
/// `render_session_render_update_on_driver`).
fn render(f: &Frontend, map: &MapEntry) -> Rendered {
    let Some(parameters) = f.update.borrow().clone() else {
        return Rendered::Nothing;
    };
    // The render may publish the next update (as a transition's frame does),
    // so the frame shows this one.
    let generation = f.update_generation.get();
    let extent = f.shared.state.lock().unwrap().extent;
    let logical = parameters.transform_state.get_size();
    // An update made for the size before a resize would stretch; the map
    // publishes one at the new size.
    if logical.width != extent.width || logical.height != extent.height {
        return Rendered::Nothing;
    }
    if f.shared.metal.as_ref().is_some_and(MetalShared::released) {
        return Rendered::NotReady;
    }
    let mut target = f.target.borrow_mut();
    let Some(target) = target.as_mut() else {
        return Rendered::NotReady;
    };
    let scale_factor = f.shared.map.scale_factor;
    let size = physical(
        mln_logical_size {
            width: logical.width,
            height: logical.height,
        },
        scale_factor,
    );
    let rendered = catch_unwind(AssertUnwindSafe(|| {
        let mut applied = f.feature_state.borrow_mut();
        if map.feature_state.needs_warmup(&applied, &parameters) {
            // The renderer takes a source's state only once it has the
            // source, so a frame that the host never sees adds it first.
            f.observer.suppress_frame_callbacks(true);
            let warmup = render_once(f, target, &parameters, size);
            f.observer.suppress_frame_callbacks(false);
            warmup?;
        }
        map.feature_state
            .push(&mut applied, &mut target.renderer, &parameters);
        drop(applied);
        render_once(f, target, &parameters, size)
    }));
    match rendered {
        Ok(Ok(true)) => {
            let (mode, needs_repaint) = f
                .observer
                .finished()
                .unwrap_or((RenderMode::Partial, false));
            f.rendered_mode.set(render_mode_code(mode));
            f.needs_repaint.set(needs_repaint);
            f.rendered_generation.set(generation);
            f.rendered_view
                .set(Some(view_of(map, &parameters, scale_factor)));
            f.shared.frame_generation.fetch_add(1, Ordering::SeqCst);
            map.publish_snapshot();
            Rendered::Frame
        }
        Ok(Ok(false)) => Rendered::Nothing,
        Ok(Err(())) => Rendered::NotReady,
        Err(panic) => {
            f.shared.fail(crate::diagnostics::panic_message(&panic));
            Rendered::Failed
        }
    }
}

/// Runs the session's frame demands (on the runtime thread).
pub fn process_demands(session: u64) {
    let Some(f) = frontend(session) else {
        return;
    };
    let map = map::entry(f.shared.map.handle);
    let incoming = std::mem::take(&mut *f.shared.demands.lock().unwrap());
    let mut pending = std::mem::take(&mut *f.waiting.borrow_mut());
    for demand in incoming {
        // A demand without FORCE replaces each pending demand with the same
        // flags.
        if demand.flags & MLN_FRAME_DEMAND_FORCE == 0 {
            pending.retain(|earlier| {
                if earlier.flags == demand.flags {
                    f.shared.push_result(frame_result(
                        &f,
                        map.as_deref(),
                        MLN_RENDER_RESULT_SUPERSEDED,
                        earlier.token,
                    ));
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
        if f.shared.state() == MLN_RENDER_SESSION_STATE_FAILED {
            f.shared.push_result(frame_result(
                &f,
                map.as_deref(),
                MLN_RENDER_RESULT_FAILED,
                demand.token,
            ));
            continue;
        }
        let Some(map) = map.as_deref() else {
            f.shared.push_result(frame_result(
                &f,
                None,
                MLN_RENDER_RESULT_TARGET_NOT_READY,
                demand.token,
            ));
            continue;
        };
        if f.shared.metal.as_ref().is_some_and(MetalShared::released) || f.target.borrow().is_none()
        {
            f.shared.push_result(frame_result(
                &f,
                Some(map),
                MLN_RENDER_RESULT_TARGET_NOT_READY,
                demand.token,
            ));
            continue;
        }
        if f.update.borrow().is_none() {
            map.map.borrow_mut().trigger_repaint();
        }
        let updated = f.update_generation.get() > f.rendered_generation.get();
        let forced = demand.flags & MLN_FRAME_DEMAND_FORCE != 0;
        let disposition = if f.update.borrow().is_some() && (forced || updated) {
            match render(&f, map) {
                Rendered::Frame => Some(MLN_RENDER_RESULT_RENDERED),
                Rendered::NotReady => Some(MLN_RENDER_RESULT_TARGET_NOT_READY),
                Rendered::Failed => Some(MLN_RENDER_RESULT_FAILED),
                Rendered::Nothing => None,
            }
        } else {
            None
        };
        match disposition {
            Some(disposition) => {
                f.shared
                    .push_result(frame_result(&f, Some(map), disposition, demand.token))
            }
            None if demand.flags & MLN_FRAME_DEMAND_WAIT_FOR_UPDATE != 0 => waiting.push(demand),
            None => f.shared.push_result(frame_result(
                &f,
                Some(map),
                MLN_RENDER_RESULT_NO_UPDATE,
                demand.token,
            )),
        }
    }
    *f.waiting.borrow_mut() = waiting;
}

/// Runs `body` with a Metal surface session's renderer, on the runtime
/// thread. `None` when the session has none.
pub(crate) fn with_renderer<R>(
    session: u64,
    body: impl FnOnce(&mut Renderer, &dyn RendererBackend) -> R,
) -> Option<R> {
    let f = frontend(session)?;
    let mut target = f.target.borrow_mut();
    let target = target.as_mut()?;
    let backend: &dyn RendererBackend = &*target.backend;
    Some(body(&mut target.renderer, backend))
}

/// Forgets the feature state that the session's renderer holds, after its
/// data was cleared.
pub(crate) fn reset_feature_state(session: u64) {
    if let Some(f) = frontend(session) {
        *f.feature_state.borrow_mut() = Default::default();
    }
}

/// Whether `session` is a live Metal surface session on this thread.
pub(crate) fn is_live(session: u64) -> bool {
    frontend(session).is_some()
}

/// Ends a session on the runtime thread: demands still queued finish with
/// TARGET_NOT_READY, the map lets go of the frontend, and the layer goes.
fn end(session: u64, shared: &Arc<SessionShared>, state: u32) {
    let f = FRONTENDS.with(|frontends| frontends.borrow_mut().remove(&session));
    let map = map::entry(shared.map.handle);
    if let Some(f) = &f {
        let mut demands = std::mem::take(&mut *shared.demands.lock().unwrap());
        demands.extend(f.waiting.borrow_mut().drain(..));
        for demand in demands {
            shared.push_result(frame_result(
                f,
                map.as_deref(),
                MLN_RENDER_RESULT_TARGET_NOT_READY,
                demand.token,
            ));
        }
        if let Some(map) = &map {
            map.frontend.detach_local();
        }
        let target = f.target.borrow_mut().take();
        if let Some(target) = target {
            target.retire();
        }
    }
    shared
        .map
        .session
        .compare_exchange(session, 0, Ordering::SeqCst, Ordering::SeqCst)
        .ok();
    if state == MLN_RENDER_SESSION_STATE_DETACHED {
        shared.generation.fetch_add(1, Ordering::SeqCst);
    }
    shared.set_state(state);
}

// MARK: - Session calls of session.rs

/// `mln_render_session_detach` of a Metal surface session.
pub fn detach(session: u64, shared: &Arc<SessionShared>, completion: Option<Completion>) {
    shared.set_state(MLN_RENDER_SESSION_STATE_DETACHING);
    let shared = shared.clone();
    shared.map.runtime.clone().post(move || {
        end(session, &shared, MLN_RENDER_SESSION_STATE_DETACHED);
        if let Some(completion) = completion {
            completion.ok();
        }
    });
}

/// `mln_render_session_abandon` and map disposal of a Metal surface session.
pub fn abandon(session: u64, shared: &Arc<SessionShared>) {
    shared.set_state(MLN_RENDER_SESSION_STATE_ABANDONED);
    shared
        .map
        .session
        .compare_exchange(session, 0, Ordering::SeqCst, Ordering::SeqCst)
        .ok();
    let shared = shared.clone();
    shared
        .map
        .runtime
        .clone()
        .post(move || end(session, &shared, MLN_RENDER_SESSION_STATE_ABANDONED));
}

/// `mln_render_session_dispose` of a Metal surface session, which session.rs
/// already let go of its map.
pub fn dispose(
    session: u64,
    shared: &Arc<SessionShared>,
    attached: bool,
    completion: Option<Completion>,
) {
    let runtime = shared.map.runtime.clone();
    if runtime.is_released() {
        shared.release_wakes();
        if let Some(completion) = completion {
            completion.ok();
        }
        return;
    }
    let shared = shared.clone();
    runtime.post(move || {
        if attached {
            end(session, &shared, MLN_RENDER_SESSION_STATE_DETACHED);
        }
        shared.release_wakes();
        if let Some(completion) = completion {
            completion.ok();
        }
    });
}

/// `mln_render_session_resize` of a Metal surface session: the layer takes
/// the size of the first frame at the new size.
pub fn resize(
    session: u64,
    shared: &Arc<SessionShared>,
    logical_size: mln_logical_size,
    completion: Option<Completion>,
) {
    let shared = shared.clone();
    shared.map.runtime.clone().post(move || {
        let (Some(_), Some(map)) = (frontend(session), map::entry(shared.map.handle)) else {
            if let Some(completion) = completion {
                completion.fail(MLN_STATUS_TARGET_LOST, "the session is detached");
            }
            return;
        };
        {
            let extent = &mut shared.state.lock().unwrap().extent;
            extent.width = logical_size.width;
            extent.height = logical_size.height;
        }
        shared.generation.fetch_add(1, Ordering::SeqCst);
        let _ = map
            .map
            .borrow_mut()
            .set_size(Size::new(logical_size.width, logical_size.height));
        map.publish_snapshot();
        if let Some(completion) = completion {
            completion.ok();
        }
    });
}

/// `mln_render_session_barrier` of a Metal surface session.
pub fn barrier(session: u64, shared: &Arc<SessionShared>, completion: Completion) {
    shared.map.runtime.clone().post(move || {
        if let Some(f) = frontend(session) {
            let map = map::entry(f.shared.map.handle);
            let waiting = std::mem::take(&mut *f.waiting.borrow_mut());
            for demand in waiting {
                f.shared.push_result(frame_result(
                    &f,
                    map.as_deref(),
                    MLN_RENDER_RESULT_NO_UPDATE,
                    demand.token,
                ));
            }
        }
        completion.ok();
    });
}

/// `mln_render_session_release_target` of a Metal surface session: the
/// session draws nothing until a new target, and demands that wait for an
/// update finish now.
pub fn release_target(session: u64, completion: Completion) -> Result<(), Failure> {
    let shared = handles::session(session)?;
    shared.map.runtime.clone().post(move || {
        let Some(f) = frontend(session) else {
            completion.fail(MLN_STATUS_INVALID_STATE, "render session is detached");
            return;
        };
        if let Some(metal) = &f.shared.metal {
            metal.released.store(true, Ordering::SeqCst);
        }
        f.rendered_generation.set(0);
        let map = map::entry(f.shared.map.handle);
        let waiting = std::mem::take(&mut *f.waiting.borrow_mut());
        for demand in waiting {
            f.shared.push_result(frame_result(
                &f,
                map.as_deref(),
                MLN_RENDER_RESULT_TARGET_NOT_READY,
                demand.token,
            ));
        }
        completion.ok();
    });
    Ok(())
}

/// Replaces the session's target with `layer` at `logical_size` (C++
/// `render_session_set_target`), on the runtime thread.
fn set_target(
    f: &Frontend,
    map: &MapEntry,
    layer: Layer,
    logical_size: mln_logical_size,
) -> Result<(), String> {
    let scale_factor = f.shared.map.scale_factor;
    let size = physical(logical_size, scale_factor);
    let released = f.shared.metal.as_ref().is_some_and(MetalShared::released);
    let extent = f.shared.state.lock().unwrap().extent;
    let same_layer = f
        .target
        .borrow()
        .as_ref()
        .is_some_and(|target| target.layer.as_ptr() == layer.as_ptr());
    // A host that hands over the current target again, such as on every
    // frame, must not make the session render a frame it already has.
    if same_layer
        && !released
        && extent.width == logical_size.width
        && extent.height == logical_size.height
    {
        return Ok(());
    }
    if same_layer {
        if let Some(target) = f.target.borrow().as_ref() {
            target.backend.set_framebuffer_size(size);
        }
    } else {
        let target = create_target(layer, size, scale_factor, &f.observer)?;
        let old = f.target.borrow_mut().replace(target);
        if let Some(old) = old {
            old.retire();
        }
        // The new renderer holds no feature state yet.
        *f.feature_state.borrow_mut() = Default::default();
    }
    if let Some(metal) = &f.shared.metal {
        metal.released.store(false, Ordering::SeqCst);
    }
    {
        let extent = &mut f.shared.state.lock().unwrap().extent;
        extent.width = logical_size.width;
        extent.height = logical_size.height;
    }
    f.shared.generation.fetch_add(1, Ordering::SeqCst);
    f.rendered_generation.set(0);
    let _ = map
        .map
        .borrow_mut()
        .set_size(Size::new(logical_size.width, logical_size.height));
    map.publish_snapshot();
    Ok(())
}

// MARK: - C API

/// Validates what every Metal call validates of its logical size.
fn validate_logical_size(size: mln_logical_size) -> Result<(), Failure> {
    if size.width == 0 || size.height == 0 {
        return Err(Failure::invalid_argument(
            "surface dimensions must be positive",
        ));
    }
    Ok(())
}

/// Validates an `mln_wake`'s queue.
fn validate_wake(wake: &mln_wake) -> Result<(), Failure> {
    if wake.callback.is_some() && wake.queue.0 != 0 {
        handles::queue(wake.queue)?;
    }
    Ok(())
}

/// # Safety
/// C API contract (surface.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_attach_metal_surface(
    map: mln_map,
    descriptor: *const mln_metal_surface_descriptor,
    options: *const mln_render_session_attach_options,
    out_session: *mut mln_render_session,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let map_shared = handles::map(map)?;
            if descriptor.is_null() {
                return Err(Failure::invalid_argument(
                    "surface descriptor must not be null",
                ));
            }
            let Some(descriptor) = read_sized(descriptor) else {
                return Err(Failure::invalid_argument(
                    "mln_metal_surface_descriptor.size is too small",
                ));
            };
            let size = descriptor.logical_size;
            validate_logical_size(size)?;
            if descriptor.layer.is_null() {
                return Err(Failure::invalid_argument(
                    "Metal surface layer must not be null",
                ));
            }
            validate_physical(
                size,
                map_shared.scale_factor,
                "scaled target dimensions are too large",
            )?;
            if options.is_null() {
                return Err(Failure::invalid_argument(
                    "render session attach options must not be null",
                ));
            }
            let Some(options) = read_sized(options) else {
                return Err(Failure::invalid_argument(
                    "render session attach options size is too small",
                ));
            };
            if out_session.is_null() || (*out_session).0 != 0 {
                return Err(Failure::invalid_argument(
                    "out_session must point to a null handle",
                ));
            }
            let completion = Completion::required(completion)?;
            if options.driver != MLN_RENDER_DRIVER_CORE_WORKER
                && options.driver != MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
            {
                return Err(Failure::invalid_argument("render driver kind is invalid"));
            }
            let known_sync_kinds = MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE
                | MLN_GPU_SYNC_KIND_FLAG_QUEUE_ORDER
                | MLN_GPU_SYNC_KIND_FLAG_OPEN_GL_FENCE
                | MLN_GPU_SYNC_KIND_FLAG_VULKAN_TIMELINE_SEMAPHORE
                | MLN_GPU_SYNC_KIND_FLAG_METAL_SHARED_EVENT
                | MLN_GPU_SYNC_KIND_FLAG_D3D12_FENCE
                | MLN_GPU_SYNC_KIND_FLAG_SYNC_FD;
            if options.producer_sync_kinds & !known_sync_kinds != 0 {
                return Err(Failure::invalid_argument(
                    "producer synchronization kinds are invalid",
                ));
            }
            validate_wake(&options.frame_wake)?;
            validate_wake(&options.driver_work_wake)?;
            if options.queue_lock.lock.is_some() {
                return Err(Failure::new(
                    MLN_STATUS_UNSUPPORTED,
                    "this render target does not take a queue lock",
                ));
            }
            // A surface's frames finish on the CPU timeline.
            if options.producer_sync_kinds != 0
                && options.producer_sync_kinds & MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE == 0
            {
                return Err(Failure::new(
                    MLN_STATUS_UNSUPPORTED,
                    "render target supports none of the requested producer sync kinds",
                ));
            }
            if !cfg!(target_vendor = "apple") {
                return Err(Failure::new(
                    MLN_STATUS_UNSUPPORTED,
                    "Metal surfaces are not supported by this build",
                ));
            }
            if options.driver != MLN_RENDER_DRIVER_CORE_WORKER {
                return Err(Failure::new(
                    MLN_STATUS_UNSUPPORTED,
                    "a Metal surface session renders on a core worker: MLN_RENDER_DRIVER_CORE_WORKER",
                ));
            }
            if map_shared.mode != MLN_MAP_MODE_CONTINUOUS {
                return Err(Failure::new(
                    MLN_STATUS_UNSUPPORTED,
                    "only a continuous map renders to a Metal surface",
                ));
            }
            if map_shared.session.load(Ordering::SeqCst) != 0 {
                return Err(Failure::invalid_state(
                    "map already has an attached render session",
                ));
            }
            let layer = retain_layer(descriptor.layer)?;
            let handle = handles::reserve(handles::Kind::RenderSession);
            if map_shared
                .session
                .compare_exchange(0, handle, Ordering::SeqCst, Ordering::SeqCst)
                .is_err()
            {
                return Err(Failure::invalid_state(
                    "map already has an attached render session",
                ));
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
                generation: AtomicU64::new(1),
                driver: MLN_RENDER_DRIVER_CORE_WORKER,
                caller: None,
                metal: Some(MetalShared::default()),
            });
            handles::insert_at(handle, Object::Session(shared.clone()));
            *out_session = handle.into();

            map_shared
                .runtime
                .post(move || attach(handle, shared, layer, size, completion));
            Ok(())
        })
    }
}

/// The attach's work on the runtime thread.
fn attach(
    handle: u64,
    shared: Arc<SessionShared>,
    layer: Layer,
    size: mln_logical_size,
    completion: Completion,
) {
    let Some(map) = map::entry(shared.map.handle) else {
        shared.set_state(MLN_RENDER_SESSION_STATE_DETACHED);
        completion.fail(MLN_STATUS_CANCELLED, "the map was released");
        return;
    };
    if shared.state() != MLN_RENDER_SESSION_STATE_ATTACHING {
        // Abandoned or disposed before it attached.
        completion.fail(MLN_STATUS_CANCELLED, "the session ended before it attached");
        return;
    }
    let scale_factor = shared.map.scale_factor;
    let observer = Rc::new(FrameObserver::default());
    let created = catch_unwind(AssertUnwindSafe(|| {
        create_target(layer, physical(size, scale_factor), scale_factor, &observer)
    }))
    .unwrap_or_else(|panic| Err(crate::diagnostics::panic_message(&panic)));
    let target = match created {
        Ok(target) => target,
        Err(message) => {
            shared.fail(message.clone());
            shared
                .map
                .session
                .compare_exchange(handle, 0, Ordering::SeqCst, Ordering::SeqCst)
                .ok();
            completion.fail(MLN_STATUS_NATIVE_ERROR, message);
            // Demands made while it attached end with it.
            process_failed(&shared);
            return;
        }
    };
    let f = Rc::new(Frontend {
        session: handle,
        shared: shared.clone(),
        observer,
        target: RefCell::new(Some(target)),
        update: RefCell::new(None),
        update_generation: Cell::new(0),
        rendered_generation: Cell::new(0),
        rendered_view: Cell::new(None),
        rendered_mode: Cell::new(MLN_RENDER_MODE_PARTIAL),
        needs_repaint: Cell::new(false),
        waiting: RefCell::new(Vec::new()),
        feature_state: RefCell::new(Default::default()),
    });
    FRONTENDS.with(|frontends| frontends.borrow_mut().insert(handle, f.clone()));
    let _ = map
        .map
        .borrow_mut()
        .set_size(Size::new(size.width, size.height));
    map.frontend.attach_local(Rc::new(LocalFrontend(f)));
    shared.set_state(MLN_RENDER_SESSION_STATE_ATTACHED);
    map.publish_snapshot();
    completion.ok();
    process_demands(handle);
}

/// Ends the demands of a session that failed to attach.
fn process_failed(shared: &SessionShared) {
    let demands = std::mem::take(&mut *shared.demands.lock().unwrap());
    for demand in demands {
        shared.push_result(mln_render_frame_result {
            disposition: MLN_RENDER_RESULT_FAILED,
            token: demand.token,
            view: shared.map.view(),
            mode: MLN_RENDER_MODE_PARTIAL,
            ..Default::default()
        });
    }
}

/// # Safety
/// C API contract (surface.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_set_metal_surface_target(
    session: mln_render_session,
    replacement: *const mln_metal_surface_replacement,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if replacement.is_null() {
                return Err(Failure::invalid_argument(
                    "surface replacement must not be null",
                ));
            }
            let Some(replacement) = read_sized(replacement) else {
                return Err(Failure::invalid_argument(
                    "mln_metal_surface_replacement.size is too small",
                ));
            };
            let size = replacement.logical_size;
            validate_logical_size(size)?;
            if replacement.layer.is_null() {
                return Err(Failure::invalid_argument(
                    "Metal surface layer must not be null",
                ));
            }
            let completion = Completion::required(completion)?;
            let shared = handles::session(session)?;
            if shared.state() != MLN_RENDER_SESSION_STATE_ATTACHED {
                return Err(Failure::invalid_state("render session is not attached"));
            }
            if shared.metal.is_none() {
                return Err(Failure::new(
                    MLN_STATUS_UNSUPPORTED,
                    "session does not render through a native surface",
                ));
            }
            validate_physical(
                size,
                shared.map.scale_factor,
                "scaled surface dimensions are too large",
            )?;
            let layer = retain_layer(replacement.layer)?;
            let session = session.0;
            shared.map.runtime.clone().post(move || {
                let (Some(f), Some(map)) = (frontend(session), map::entry(shared.map.handle))
                else {
                    completion.fail(MLN_STATUS_INVALID_STATE, "render session is detached");
                    return;
                };
                let replaced = catch_unwind(AssertUnwindSafe(|| set_target(&f, &map, layer, size)))
                    .unwrap_or_else(|panic| Err(crate::diagnostics::panic_message(&panic)));
                match replaced {
                    Ok(()) => {
                        completion.ok();
                        // The new target holds no frame of the latest update,
                        // so a demand that waits for one has one to render.
                        process_demands(session);
                    }
                    Err(message) => completion.fail(MLN_STATUS_NATIVE_ERROR, message),
                }
            });
            Ok(())
        })
    }
}
