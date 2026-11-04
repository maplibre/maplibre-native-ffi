// Ports the map half of maplibre-native-ffi src/map/map.cpp and
// src/c_api/map.cpp, camera.cpp (the parts this crate implements) onto
// `mln_map::Map`.
//
// A map lives on its runtime's thread. `MapShared` is the thread-safe part
// the handle table holds (state that `mln_map_get_snapshot` reads without
// waiting); `MapEntry` is the runtime thread's part (the Map itself).

use crate::delivery::{Completion, Value};
use crate::diagnostics::{CallResult, Failure, boundary};
use crate::handles::{self, Object};
use crate::runtime::{OwnedEvent, Payload, RuntimeShared, STATE};
use crate::session::ProxyFrontend;
use crate::types::*;
use maplibre_native_next::mln_map::{
    AnimationOptions, CameraChangeMode, CameraOptions, Map, MapLoadError, MapMode, MapObserver, MapOptions,
    RenderFrameStatus, RenderMode,
};
use maplibre_native_next::mln_util::geo::{EdgeInsets, LatLng, ScreenCoordinate};
use maplibre_native_next::mln_util::size::Size;
use std::cell::RefCell;
use std::rc::Rc;
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::{Arc, Mutex};

pub struct MapShared {
    pub handle: u64,
    pub runtime: Arc<RuntimeShared>,
    pub mode: u32,
    pub scale_factor: f64,
    pub generation: AtomicU64,
    pub created: AtomicBool,
    pub event_mask: AtomicU64,
    pub session: AtomicU64,
    snapshot: Mutex<mln_map_snapshot>,
}

impl MapShared {
    /// The view of the last published snapshot, from any thread.
    pub fn view(&self) -> mln_map_view {
        self.snapshot.lock().unwrap().view
    }

    pub fn next_generation(&self) -> u64 {
        self.generation.fetch_add(1, Ordering::SeqCst) + 1
    }

    pub fn emit(&self, type_: u32, fill: impl FnOnce(&mut OwnedEvent)) {
        if self.event_mask.load(Ordering::SeqCst) & (1u64 << type_) == 0 {
            return;
        }
        let mut event = OwnedEvent::map(self.handle, self.generation.load(Ordering::SeqCst), type_);
        fill(&mut event);
        self.runtime.emit(event);
    }
}

/// The runtime thread's part of a map.
pub struct MapEntry {
    pub shared: Arc<MapShared>,
    pub map: RefCell<Map>,
    pub frontend: ProxyFrontend,
    pub observer: Rc<FfiMapObserver>,
}

impl MapEntry {
    pub fn map_mode(&self) -> MapMode {
        mode_of(self.shared.mode)
    }

    /// Publishes the state `mln_map_get_snapshot` reads.
    pub fn publish_snapshot(&self) {
        let map = self.map.borrow();
        let camera = camera_of(&map.get_camera_options(None));
        let size = map.get_map_options().size();
        let mut snapshot = self.shared.snapshot.lock().unwrap();
        snapshot.size = size_of::<mln_map_snapshot>() as u32;
        snapshot.generation = self.shared.generation.load(Ordering::SeqCst);
        snapshot.fully_loaded = map.is_fully_loaded();
        snapshot.event_mask = self.shared.event_mask.load(Ordering::SeqCst);
        snapshot.view.size = size_of::<mln_map_view>() as u32;
        snapshot.view.camera = camera;
        snapshot.view.extent =
            mln_logical_extent { width: size.width, height: size.height, scale_factor: self.shared.scale_factor };
        snapshot.latest_render_update_generation = self.frontend.update_generation();
    }

    pub fn view(&self) -> mln_map_view {
        self.shared.snapshot.lock().unwrap().view
    }
}

fn mode_of(mode: u32) -> MapMode {
    match mode {
        MLN_MAP_MODE_STATIC => MapMode::Static,
        MLN_MAP_MODE_TILE => MapMode::Tile,
        _ => MapMode::Continuous,
    }
}

pub fn camera_of(options: &CameraOptions) -> mln_camera {
    let center = options.center.unwrap_or_default();
    let padding = options.padding.unwrap_or_default();
    mln_camera {
        center: mln_lat_lng { latitude: center.latitude(), longitude: center.longitude() },
        center_altitude: options.center_altitude.unwrap_or(0.0),
        padding: mln_edge_insets {
            top: padding.top(),
            left: padding.left(),
            bottom: padding.bottom(),
            right: padding.right(),
        },
        zoom: options.zoom.unwrap_or(0.0),
        bearing: options.bearing.unwrap_or(0.0),
        pitch: options.pitch.unwrap_or(0.0),
        roll: options.roll.unwrap_or(0.0),
        field_of_view: options.fov.unwrap_or(0.0),
    }
}

/// `mln_camera_options` -> `CameraOptions`, with the C++ validation messages
/// for out-of-range values.
fn camera_options_of(c: &mln_camera_options) -> Result<CameraOptions, Failure> {
    if c.fields & !MLN_CAMERA_OPTION_ALL != 0 {
        return Err(Failure::invalid_argument("camera.fields has unknown bits"));
    }
    let has = |bit: u32| c.fields & bit != 0;
    let finite = |name: &str, value: f64| {
        if value.is_finite() { Ok(value) } else { Err(Failure::invalid_argument(format!("camera.{name} must be finite"))) }
    };
    let mut options = CameraOptions::default();
    if has(MLN_CAMERA_OPTION_CENTER) {
        let center = LatLng::new(c.center.latitude, c.center.longitude)
            .map_err(|e| Failure::invalid_argument(format!("camera.center: {e}")))?;
        options.center = Some(center);
    }
    if has(MLN_CAMERA_OPTION_CENTER_ALTITUDE) {
        options.center_altitude = Some(finite("center_altitude", c.center_altitude)?);
    }
    if has(MLN_CAMERA_OPTION_PADDING) {
        let p = c.padding;
        options.padding = Some(
            EdgeInsets::new(p.top, p.left, p.bottom, p.right)
                .map_err(|e| Failure::invalid_argument(format!("camera.padding: {e}")))?,
        );
    }
    if has(MLN_CAMERA_OPTION_ANCHOR) {
        options.anchor = Some(ScreenCoordinate::new(finite("anchor.x", c.anchor.x)?, finite("anchor.y", c.anchor.y)?));
    }
    if has(MLN_CAMERA_OPTION_ZOOM) {
        options.zoom = Some(finite("zoom", c.zoom)?);
    }
    if has(MLN_CAMERA_OPTION_BEARING) {
        options.bearing = Some(finite("bearing", c.bearing)?);
    }
    if has(MLN_CAMERA_OPTION_PITCH) {
        options.pitch = Some(finite("pitch", c.pitch)?);
    }
    if has(MLN_CAMERA_OPTION_ROLL) {
        options.roll = Some(finite("roll", c.roll)?);
    }
    if has(MLN_CAMERA_OPTION_FOV) {
        options.fov = Some(finite("field_of_view", c.field_of_view)?);
    }
    Ok(options)
}

fn animation_options_of(a: &mln_animation_options) -> Result<AnimationOptions, Failure> {
    let mut options = AnimationOptions::default();
    if a.fields & MLN_ANIMATION_OPTION_DURATION != 0 {
        if !(a.duration_ms.is_finite() && a.duration_ms >= 0.0) {
            return Err(Failure::invalid_argument("animation.duration_ms must be finite and not negative"));
        }
        options.duration = Some(maplibre_native_next::mln_util::chrono::Duration::from_nanos((a.duration_ms * 1e6) as i64));
    }
    if a.fields & MLN_ANIMATION_OPTION_VELOCITY != 0 {
        options.velocity = Some(a.velocity);
    }
    if a.fields & MLN_ANIMATION_OPTION_MIN_ZOOM != 0 {
        options.min_zoom = Some(a.min_zoom);
    }
    if a.fields & MLN_ANIMATION_OPTION_EASING != 0 {
        let e = a.easing;
        options.easing = Some(maplibre_native_next::mln_util::unitbezier::UnitBezier::new(e.x1, e.y1, e.x2, e.y2));
    }
    // TODO(ffi): transition_id, end_handler (camera transition callbacks).
    Ok(options)
}

/// The map's observer: turns MapObserver calls into runtime events.
pub struct FfiMapObserver {
    shared: Arc<MapShared>,
    /// The last style load failure, for set_style_json's completion.
    pub load_error: RefCell<Option<String>>,
}

impl FfiMapObserver {
    fn continuous(&self) -> bool {
        self.shared.mode == MLN_MAP_MODE_CONTINUOUS
    }
}

fn change_code(mode: CameraChangeMode) -> u32 {
    match mode {
        CameraChangeMode::Immediate => 0,
        CameraChangeMode::Animated => 1,
    }
}

fn render_mode_code(mode: RenderMode) -> u32 {
    match mode {
        RenderMode::Partial => MLN_RENDER_MODE_PARTIAL,
        RenderMode::Full => MLN_RENDER_MODE_FULL,
    }
}

impl MapObserver for FfiMapObserver {
    fn on_camera_will_change(&self, mode: CameraChangeMode) {
        self.shared.emit(MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE, |e| e.code = change_code(mode));
    }
    fn on_camera_is_changing(&self) {
        self.shared.emit(MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING, |_| {});
    }
    fn on_camera_did_change(&self, mode: CameraChangeMode) {
        self.shared.emit(MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE, |e| e.code = change_code(mode));
    }
    fn on_will_start_loading_map(&self) {
        self.shared.emit(MLN_RUNTIME_EVENT_MAP_LOADING_STARTED, |_| {});
    }
    fn on_did_finish_loading_map(&self) {
        if self.continuous() {
            self.shared.emit(MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED, |_| {});
        }
    }
    fn on_did_fail_loading_map(&self, _error: MapLoadError, what: &str) {
        *self.load_error.borrow_mut() = Some(what.to_owned());
        self.shared.emit(MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, |e| e.message = what.to_owned());
    }
    fn on_will_start_rendering_frame(&self) {
        if self.continuous() {
            self.shared.emit(MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED, |_| {});
        }
    }
    fn on_did_finish_rendering_frame(&self, status: &RenderFrameStatus) {
        if self.continuous() {
            self.shared.emit(MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED, |e| {
                e.payload = Payload::RenderFrame(mln_runtime_event_render_frame {
                    mode: render_mode_code(status.mode),
                    needs_repaint: status.needs_repaint,
                    placement_changed: status.placement_changed,
                    // TODO(port): gfx::RenderingStats.
                    stats: mln_rendering_stats::default(),
                });
            });
        }
    }
    fn on_will_start_rendering_map(&self) {
        if self.continuous() {
            self.shared.emit(MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED, |_| {});
        }
    }
    fn on_did_finish_rendering_map(&self, mode: RenderMode) {
        if self.continuous() {
            self.shared.emit(MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED, |e| {
                e.payload = Payload::RenderMap(mln_runtime_event_render_map { mode: render_mode_code(mode) });
            });
        }
    }
    fn on_did_finish_loading_style(&self) {
        self.shared.emit(MLN_RUNTIME_EVENT_MAP_STYLE_LOADED, |_| {});
    }
    fn on_did_become_idle(&self) {
        if self.continuous() {
            self.shared.emit(MLN_RUNTIME_EVENT_MAP_IDLE, |_| {});
        }
    }
    fn on_style_image_missing(&self, id: &str) {
        self.shared.emit(MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING, |e| e.message = id.to_owned());
    }
}

/// The runtime thread's entry of `handle`. The `STATE` borrow ends before it
/// returns, so the caller may call into the map (which may call back here).
pub fn entry(handle: u64) -> Option<Rc<MapEntry>> {
    STATE.with(|state| state.borrow().maps.get(&handle).cloned())
}

/// Validates `map` and `completion` on the caller's thread, then runs `body`
/// on the runtime thread and completes with its result and the command's
/// generation.
///
/// # Safety
/// `completion` as for `Completion::optional`.
pub unsafe fn command(
    map: mln_map,
    completion: *const mln_completion,
    optional_completion: bool,
    body: impl FnOnce(&Rc<MapEntry>) -> Result<Value, Failure> + Send + 'static,
) -> CallResult {
    let shared = handles::map(map)?;
    // SAFETY: the caller's contract.
    let completion = if optional_completion {
        unsafe { Completion::optional(completion) }?
    } else {
        Some(unsafe { Completion::required(completion) }?)
    };
    let generation = shared.next_generation();
    let runtime = shared.runtime.clone();
    runtime.post(move || {
        let Some(entry) = entry(shared.handle) else {
            if let Some(completion) = completion {
                completion.fail(MLN_STATUS_CANCELLED, "the map was released");
            }
            return;
        };
        let result = body(&entry);
        entry.publish_snapshot();
        if let Some(completion) = completion {
            match result {
                Ok(value) => completion.ok_with(generation, value),
                Err(failure) => completion.finish(failure.status, generation, failure.message, Value::None),
            }
        }
    });
    Ok(())
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_create_map(
    runtime: mln_runtime,
    options: *const mln_map_options,
    out_map: *mut mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let runtime = handles::runtime(runtime)?;
            let Some(options) = read_sized(options) else {
                return Err(Failure::invalid_argument("options must be an mln_map_options"));
            };
            if out_map.is_null() {
                return Err(Failure::invalid_argument("out_map must not be null"));
            }
            let extent = options.initial_extent;
            if extent.width == 0 || extent.height == 0 {
                return Err(Failure::invalid_argument("initial_extent must not be empty"));
            }
            if !(extent.scale_factor.is_finite() && extent.scale_factor > 0.0) {
                return Err(Failure::invalid_argument("initial_extent.scale_factor must be finite and positive"));
            }
            if options.map_mode > MLN_MAP_MODE_TILE {
                return Err(Failure::invalid_argument("map_mode is not an mln_map_mode"));
            }
            if options.fields & !(MLN_MAP_OPTION_EVENT_MASK | MLN_MAP_OPTION_TILE_CACHE_ENABLED) != 0 {
                return Err(Failure::invalid_argument("options.fields has unknown bits"));
            }
            let event_mask = if options.fields & MLN_MAP_OPTION_EVENT_MASK != 0 {
                if options.event_mask & !MLN_RUNTIME_EVENT_FLAG_ALL_MAP_EVENTS != 0 {
                    return Err(Failure::invalid_argument("event_mask has bits that aren't map events"));
                }
                options.event_mask
            } else {
                MLN_RUNTIME_EVENT_FLAG_ALL_MAP_EVENTS
            };
            let completion = Some(Completion::required(completion)?);

            let handle = handles::reserve(handles::Kind::Map);
            let shared = Arc::new(MapShared {
                handle,
                runtime: runtime.clone(),
                mode: options.map_mode,
                scale_factor: extent.scale_factor,
                generation: AtomicU64::new(0),
                created: AtomicBool::new(false),
                event_mask: AtomicU64::new(event_mask),
                session: AtomicU64::new(0),
                snapshot: Mutex::new(mln_map_snapshot::default()),
            });
            handles::insert_at(handle, Object::Map(shared.clone()));
            runtime.maps.lock().unwrap().insert(handle);
            *out_map = handle.into();

            let fast_pfor = options.fast_pfor_enabled;
            runtime.clone().post(move || {
                let observer = Rc::new(FfiMapObserver { shared: shared.clone(), load_error: RefCell::new(None) });
                let size = Size::new(extent.width, extent.height);
                let frontend = ProxyFrontend::new(mode_of(shared.mode), extent.scale_factor as f32);
                let map = Map::with_resource_options(
                    &frontend,
                    observer.clone(),
                    MapOptions::new()
                        .with_map_mode(mode_of(shared.mode))
                        .with_size(size)
                        .with_pixel_ratio(extent.scale_factor as f32)
                        .with_fast_pfor_enabled(fast_pfor),
                    &runtime.resource_options,
                    &maplibre_native_next::mln_storage::ClientOptions::default(),
                );
                let entry = Rc::new(MapEntry { shared: shared.clone(), map: RefCell::new(map), frontend, observer });
                STATE.with(|state| state.borrow_mut().maps.insert(handle, entry.clone()));
                entry.publish_snapshot();
                shared.created.store(true, Ordering::SeqCst);
                if let Some(completion) = completion {
                    completion.ok_at(shared.generation.load(Ordering::SeqCst));
                }
            });
            Ok(())
        })
    }
}

fn release_map(shared: &Arc<MapShared>, completion: Option<Completion>) {
    handles::remove(shared.handle);
    shared.runtime.maps.lock().unwrap().remove(&shared.handle);
    let handle = shared.handle;
    shared.runtime.post(move || {
        let entry = STATE.with(|state| state.borrow_mut().maps.remove(&handle));
        drop(entry);
        if let Some(completion) = completion {
            completion.ok();
        }
    });
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_release(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::map(map)?;
            if shared.session.load(Ordering::SeqCst) != 0 {
                return Err(Failure::invalid_state("detach the map's render session first"));
            }
            let completion = Completion::optional(completion)?;
            release_map(&shared, completion);
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_dispose(map: mln_map, out_diagnostic: *mut mln_diagnostic) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::map(map)?;
            let session = shared.session.swap(0, Ordering::SeqCst);
            if session != 0 {
                crate::session::abandon(session);
            }
            release_map(&shared, None);
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_snapshot(
    map: mln_map,
    out_snapshot: *mut mln_map_snapshot,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::map(map)?;
            if !shared.created.load(Ordering::SeqCst) {
                return Err(Failure::invalid_state("the map is still being created"));
            }
            let snapshot = *shared.snapshot.lock().unwrap();
            if !write_sized(out_snapshot, snapshot) {
                return Err(Failure::invalid_argument("out_snapshot must be an mln_map_snapshot"));
            }
            Ok(())
        })
    }
}

fn utf8_view(view: mln_buffer_view, name: &str) -> Result<String, Failure> {
    // SAFETY: C API contract for buffer views.
    let Some(bytes) = (unsafe { view.bytes() }) else {
        return Err(Failure::invalid_argument(format!("{name} must be a valid buffer view")));
    };
    String::from_utf8(bytes.to_vec()).map_err(|_| Failure::invalid_argument(format!("{name} must be UTF-8")))
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_style_json(
    map: mln_map,
    json: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let json = utf8_view(json, "json")?;
            command(map, completion, true, move |entry| {
                *entry.observer.load_error.borrow_mut() = None;
                entry.map.borrow_mut().get_style_mut().load_json(&json);
                match entry.observer.load_error.borrow_mut().take() {
                    Some(error) => Err(Failure::new(MLN_STATUS_NATIVE_ERROR, error)),
                    None => Ok(Value::None),
                }
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_style_url(
    map: mln_map,
    url: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let url = utf8_view(url, "url")?;
            if url.is_empty() {
                return Err(Failure::invalid_argument("url must not be empty"));
            }
            command(map, completion, true, move |entry| {
                entry.map.borrow_mut().get_style_mut().load_url(&url);
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_style_url(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            command(map, completion, false, |entry| {
                Ok(Value::BufferView(entry.map.borrow().get_style().get_url().into_bytes()))
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_loaded_style_json(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            command(map, completion, false, |entry| {
                Ok(Value::BufferView(entry.map.borrow().get_style().get_json().into_bytes()))
            })
        })
    }
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_update_camera(
    map: mln_map,
    update: *const mln_camera_update,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let Some(update) = read_sized(update) else {
                return Err(Failure::invalid_argument("update must be an mln_camera_update"));
            };
            let Some(camera) = read_sized(&update.camera) else {
                return Err(Failure::invalid_argument("update.camera must be an mln_camera_options"));
            };
            let camera = camera_options_of(&camera)?;
            let animation = animation_options_of(&update.animation)?;
            if update.animation.end_handler.callback.is_some() {
                return Err(Failure::new(MLN_STATUS_UNSUPPORTED, "camera transition end handlers are not implemented yet"));
            }
            let mode = update.mode;
            if mode > MLN_CAMERA_UPDATE_MODE_FLY {
                return Err(Failure::invalid_argument("update.mode is not an mln_camera_update_mode"));
            }
            // TODO(ffi): gesture_phase.
            struct SendAnimation(AnimationOptions);
            // SAFETY: animation options from C carry no Rc callbacks.
            unsafe impl Send for SendAnimation {}
            let animation = SendAnimation(animation);
            command(map, completion, true, move |entry| {
                let animation = animation;
                let mut map = entry.map.borrow_mut();
                match mode {
                    MLN_CAMERA_UPDATE_MODE_JUMP => map.jump_to(&camera),
                    MLN_CAMERA_UPDATE_MODE_EASE => map.ease_to(&camera, &animation.0),
                    _ => map.fly_to(&camera, &animation.0),
                }
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_camera(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            command(map, completion, false, |entry| {
                let camera = camera_of(&entry.map.borrow().get_camera_options(None));
                Ok(Value::Camera(mln_camera_query_result {
                    generation: entry.shared.generation.load(Ordering::SeqCst),
                    camera,
                }))
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_request_repaint(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            command(map, completion, true, |entry| {
                entry.map.borrow_mut().trigger_repaint();
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_resize(
    map: mln_map,
    logical_size: mln_logical_size,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::map(map)?;
            if shared.session.load(Ordering::SeqCst) != 0 {
                return Err(Failure::invalid_state("the map has a render session; resize the session instead"));
            }
            if logical_size.width == 0 || logical_size.height == 0 {
                return Err(Failure::invalid_argument("logical_size must not be empty"));
            }
            command(map, completion, true, move |entry| {
                entry
                    .map
                    .borrow_mut()
                    .set_size(Size::new(logical_size.width, logical_size.height))
                    .map_err(|e| Failure::invalid_argument(e))?;
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_event_mask(
    map: mln_map,
    mask: u64,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if mask & !MLN_RUNTIME_EVENT_FLAG_ALL_MAP_EVENTS != 0 {
                return Err(Failure::invalid_argument("mask has bits that aren't map events"));
            }
            command(map, completion, true, move |entry| {
                entry.shared.event_mask.store(mask, Ordering::SeqCst);
                Ok(Value::None)
            })
        })
    }
}
