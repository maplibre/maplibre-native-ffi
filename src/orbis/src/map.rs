// Maps: an Orbis map on its runtime's thread, with the state the C API
// reports about it (mode, extent, event mask, style URL).

use std::sync::{Arc, OnceLock};

use orb_api::{CameraTarget, Change, EaseOptions, Easing, FitOptions, FlyOptions, Geodetic, Phase};

use crate::base::{Failure, Result, boundary, put, sized, text};
use crate::delivery::{Completion, Value};
use crate::handles::{self, Object};
use crate::runtime::{Payload, RuntimeHandle, State};
use crate::types::*;

/// Every event a map raises unless its options set a mask.
const DEFAULT_EVENT_MASK: u64 = u64::MAX;

pub struct MapHandle {
    pub runtime: Arc<RuntimeHandle>,
    pub id: OnceLock<u64>,
}

impl MapHandle {
    fn id(&self) -> u64 {
        *self.id.get().expect("map id")
    }

    /// Runs `job` with the map on the runtime thread, then completes.
    fn command(
        &self,
        completion: Option<Completion>,
        job: impl FnOnce(&mut State, u64) -> Result<Value> + Send + 'static,
    ) -> Result {
        let id = self.id();
        self.runtime.post(move |state| {
            let result = if state.maps.contains_key(&id) {
                job(state, id)
            } else {
                Err(Failure::state("the map was released"))
            };
            let generation = state.maps.get(&id).map_or(0, |m| m.map.generation());
            if let Some(c) = completion {
                c.complete(result, generation);
            }
        })
    }

    /// Runs `job` with the map on the runtime thread and waits.
    fn query<R: Send + 'static>(
        &self,
        job: impl FnOnce(&mut MapEntry) -> Result<R> + Send + 'static,
    ) -> Result<R> {
        let id = self.id();
        self.runtime.call(move |state| {
            let entry = state
                .maps
                .get_mut(&id)
                .ok_or_else(|| Failure::state("the map was released"))?;
            job(entry)
        })
    }
}

pub struct MapEntry {
    pub map: orb_api::Map,
    pub mode: mln_map_mode,
    pub extent: mln_logical_extent,
    pub event_mask: u64,
    pub style_url: String,
    pub session: Option<u64>,
    pub style_loaded: bool,
}

impl MapEntry {
    pub fn camera(&self) -> mln_camera {
        let c = self.map.camera();
        mln_camera {
            center: mln_lat_lng {
                latitude: c.target.lat,
                longitude: c.target.lon,
            },
            center_altitude: c.elevation,
            padding: mln_edge_insets {
                top: 0.0,
                left: 0.0,
                bottom: 0.0,
                right: 0.0,
            },
            zoom: self.map.zoom(),
            bearing: c.bearing,
            pitch: c.pitch,
            roll: c.roll,
            field_of_view: c.fov,
        }
    }

    pub fn view(&self) -> mln_map_view {
        mln_map_view {
            size: std::mem::size_of::<mln_map_view>() as u32,
            camera: self.camera(),
            extent: self.extent,
            ..Default::default()
        }
    }
}

/// Moves Orbis's events for a map into the runtime's event batch.
pub fn take_events(state: &mut State, id: u64) {
    let Some(entry) = state.maps.get_mut(&id) else {
        return;
    };
    let mut out = Vec::new();
    while let Some(event) = entry.map.poll_event() {
        let orb_api::Event::Map(event) = event else {
            continue;
        };
        use orb_api::MapEvent as E;
        match event {
            E::StyleLoad => {
                entry.style_loaded = true;
                out.push((
                    MLN_RUNTIME_EVENT_MAP_STYLE_LOADED,
                    Payload::None,
                    String::new(),
                ));
            }
            E::Load => out.push((
                MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED,
                Payload::None,
                String::new(),
            )),
            E::Idle => out.push((MLN_RUNTIME_EVENT_MAP_IDLE, Payload::None, String::new())),
            E::StyleImageMissing { name } => out.push((
                MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING,
                Payload::StyleImage(name),
                String::new(),
            )),
            // A failure before the style loaded is the style's; later ones are
            // a source's, a tile's or a sprite's, which MapLibre Native logs.
            E::Error { message } if !entry.style_loaded => out.push((
                MLN_RUNTIME_EVENT_MAP_LOADING_FAILED,
                Payload::LoadingFailed(MLN_MAP_LOAD_ERROR_STYLE_LOAD),
                message,
            )),
            E::Error { message } | E::Warning { message } => crate::logging::warn(&message),
            E::Camera(orb_api::CameraEvent::Changed {
                change: Change::Move,
                phase,
                user,
            }) => {
                let mode = if user {
                    MLN_CAMERA_CHANGE_MODE_IMMEDIATE
                } else {
                    MLN_CAMERA_CHANGE_MODE_ANIMATED
                };
                let kind = match phase {
                    Phase::Start => MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE,
                    Phase::Progress => MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING,
                    Phase::End => MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE,
                };
                out.push((kind, Payload::CameraChange(mode), String::new()));
            }
            _ => {}
        }
    }
    let mask = entry.event_mask;
    for (kind, payload, message) in out {
        state.event(id, mask, kind, payload, message);
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_create_map(
    runtime: mln_runtime,
    options: *const mln_map_options,
    out_map: *mut mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let runtime = handles::runtime(runtime.0)?;
            let options = *sized(options, "options")?;
            crate::base::physical(options.initial_extent)?;
            if options.map_mode > MLN_MAP_MODE_TILE {
                return Err(Failure::argument("unknown map mode"));
            }
            if out_map.is_null() {
                return Err(Failure::argument("out_map is null"));
            }
            let completion = Completion::from_c(completion)?;
            let handle = Arc::new(MapHandle {
                runtime: runtime.clone(),
                id: OnceLock::new(),
            });
            let id = handles::insert(Object::Map(handle.clone()));
            let _ = handle.id.set(id);
            let event_mask = if options.fields & MLN_MAP_OPTION_EVENT_MASK != 0 {
                options.event_mask
            } else {
                DEFAULT_EVENT_MASK
            };
            let wake = runtime.sender();
            runtime.post(move |state| {
                let mut map = orb_api::Map::new(&state.runtime, orb_api::MapOptions::default());
                let wake = std::sync::Mutex::new(wake);
                map.set_wake(move || {
                    let _ = wake.lock().unwrap().send(crate::runtime::Msg::Pump);
                });
                state.maps.insert(
                    id,
                    MapEntry {
                        map,
                        mode: options.map_mode,
                        extent: options.initial_extent,
                        event_mask,
                        style_url: String::new(),
                        session: None,
                        style_loaded: false,
                    },
                );
                if let Some(c) = completion {
                    c.complete(Ok(Value::None), 0);
                }
            })?;
            put(out_map, mln_map(id), "out_map")
        })
    }
}

fn release(map: mln_map, completion: Option<Completion>, wait: bool) -> Result {
    let handle = handles::map(map.0)?;
    let id = handle.id();
    let has_session = handle.query(|entry| Ok(entry.session.is_some()))?;
    if has_session {
        return Err(Failure::state("the map still has a render session"));
    }
    handles::remove(map.0);
    let job = move |state: &mut State| {
        state.maps.remove(&id);
        Ok(())
    };
    if wait {
        handle.runtime.call(job)
    } else {
        handle.runtime.post(move |state| {
            let _ = job(state);
            if let Some(c) = completion {
                c.complete(Ok(Value::None), 0);
            }
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_release(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            release(map, Completion::from_c(completion)?, false)
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_dispose(
    map: mln_map,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointer.
    unsafe { boundary(out_diagnostic, || release(map, None, true)) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_style_url(
    map: mln_map,
    url: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::map(map.0)?;
            let url = text(url, "url")?;
            if url.is_empty() {
                return Err(Failure::argument("the URL is empty"));
            }
            handle.command(Completion::from_c(completion)?, move |state, id| {
                let entry = state.maps.get_mut(&id).expect("map");
                entry.map.load_style_url(&url);
                entry.style_url = url;
                entry.style_loaded = false;
                let mask = entry.event_mask;
                state.event(
                    id,
                    mask,
                    MLN_RUNTIME_EVENT_MAP_LOADING_STARTED,
                    Payload::None,
                    String::new(),
                );
                Ok(Value::None)
            })
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_style_json(
    map: mln_map,
    json: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::map(map.0)?;
            let json = text(json, "json")?;
            handle.command(Completion::from_c(completion)?, move |state, id| {
                let entry = state.maps.get_mut(&id).expect("map");
                entry.style_url.clear();
                entry.style_loaded = false;
                let mask = entry.event_mask;
                let result = entry.map.set_style_json(&json);
                state.event(
                    id,
                    mask,
                    MLN_RUNTIME_EVENT_MAP_LOADING_STARTED,
                    Payload::None,
                    String::new(),
                );
                if let Err(e) = &result {
                    state.event(
                        id,
                        mask,
                        MLN_RUNTIME_EVENT_MAP_LOADING_FAILED,
                        Payload::LoadingFailed(MLN_MAP_LOAD_ERROR_STYLE_PARSE),
                        e.message().into(),
                    );
                }
                result.map(|()| Value::None).map_err(Failure::from)
            })
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_style_url(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::map(map.0)?;
            handle.command(Completion::from_c(completion)?, |state, id| {
                Ok(Value::Text(state.maps[&id].style_url.clone()))
            })
        })
    }
}

/// An Orbis camera move from the options' fields.
fn target(entry: &MapEntry, camera: &mln_camera_options) -> Result<CameraTarget> {
    let f = camera.fields;
    let has = |bit| f & bit != 0;
    if has(MLN_CAMERA_OPTION_FOV) {
        return Err(Failure::unsupported("Orbis keeps a fixed field of view"));
    }
    let mut t = CameraTarget::default();
    if has(MLN_CAMERA_OPTION_CENTER) {
        t.center = Some(Geodetic::new(
            camera.center.longitude,
            camera.center.latitude,
        ));
    }
    if has(MLN_CAMERA_OPTION_ZOOM) {
        t.zoom = Some(camera.zoom);
    }
    if has(MLN_CAMERA_OPTION_BEARING) {
        t.bearing = Some(camera.bearing);
    }
    if has(MLN_CAMERA_OPTION_PITCH) {
        t.pitch = Some(camera.pitch);
    }
    if has(MLN_CAMERA_OPTION_ROLL) {
        t.roll = Some(camera.roll);
    }
    if has(MLN_CAMERA_OPTION_CENTER_ALTITUDE) {
        t.elevation = Some(camera.center_altitude);
    }
    if has(MLN_CAMERA_OPTION_PADDING) {
        // The center shifts to the middle of the padded area.
        let p = camera.padding;
        t.offset = glam::DVec2::new((p.left - p.right) / 2.0, (p.top - p.bottom) / 2.0);
    }
    if has(MLN_CAMERA_OPTION_ANCHOR) && t.center.is_none() {
        let a = camera.anchor;
        t.around = entry.map.unproject(glam::DVec2::new(a.x, a.y));
    }
    Ok(t)
}

/// The animation fields Orbis reads, without the end handler's pointers.
#[derive(Clone, Copy)]
struct Animation {
    fields: mln_animation_option_fields,
    duration_ms: f64,
    velocity: f64,
    min_zoom: f64,
    easing: mln_unit_bezier,
}

impl From<&mln_animation_options> for Animation {
    fn from(a: &mln_animation_options) -> Self {
        Self {
            fields: a.fields,
            duration_ms: a.duration_ms,
            velocity: a.velocity,
            min_zoom: a.min_zoom,
            easing: a.easing,
        }
    }
}

fn easing(animation: &Animation) -> Option<Easing> {
    (animation.fields & MLN_ANIMATION_OPTION_EASING != 0).then(|| {
        let e = animation.easing;
        Easing::Bezier([e.x1, e.y1, e.x2, e.y2])
    })
}

/// Moves the camera as an update says, at once or animated.
fn update_camera(
    entry: &mut MapEntry,
    mode: mln_camera_update_mode,
    t: &CameraTarget,
    animation: &Animation,
) -> Result {
    let duration =
        (animation.fields & MLN_ANIMATION_OPTION_DURATION != 0).then_some(animation.duration_ms);
    match mode {
        MLN_CAMERA_UPDATE_MODE_JUMP => entry.map.jump_to(t),
        MLN_CAMERA_UPDATE_MODE_EASE => {
            let mut options = EaseOptions::default();
            if let Some(d) = duration {
                options.duration = d;
            }
            if let Some(e) = easing(animation) {
                options.easing = e;
            }
            entry.map.ease_to(t, &options);
        }
        MLN_CAMERA_UPDATE_MODE_FLY => {
            let mut options = FlyOptions::default();
            options.duration = duration;
            if animation.fields & MLN_ANIMATION_OPTION_MIN_ZOOM != 0 {
                options.min_zoom = Some(animation.min_zoom);
            }
            if animation.fields & MLN_ANIMATION_OPTION_VELOCITY != 0 {
                options.screen_speed = Some(animation.velocity);
            }
            if let Some(e) = easing(animation) {
                options.easing = e;
            }
            entry.map.fly_to(t, &options);
        }
        _ => return Err(Failure::argument("unknown camera update mode")),
    }
    Ok(())
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_update_camera(
    map: mln_map,
    update: *const mln_camera_update,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::map(map.0)?;
            let update = *sized(update, "update")?;
            if update.animation.fields & MLN_ANIMATION_OPTION_TRANSITION_ID != 0
                || update.animation.end_handler.callback.is_some()
            {
                return Err(Failure::unsupported(
                    "camera transition handlers aren't implemented on Orbis",
                ));
            }
            let (mode, camera, animation) = (
                update.mode,
                update.camera,
                Animation::from(&update.animation),
            );
            handle.command(Completion::from_c(completion)?, move |state, id| {
                let entry = state.maps.get_mut(&id).expect("map");
                let t = target(entry, &camera)?;
                update_camera(entry, mode, &t, &animation)?;
                Ok(Value::None)
            })
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_apply_camera_delta(
    map: mln_map,
    delta: *const mln_camera_delta,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::map(map.0)?;
            let d = *sized(delta, "delta")?;
            let delta = Delta {
                fields: d.fields,
                offset: d.offset,
                scale: d.scale,
                bearing: d.bearing,
                pitch: d.pitch,
                anchor: d.anchor,
                animation: Animation::from(&d.animation),
                gesture_phase: d.gesture_phase,
            };
            handle.command(Completion::from_c(completion)?, move |state, id| {
                let entry = state.maps.get_mut(&id).expect("map");
                apply_delta(entry, &delta)?;
                Ok(Value::None)
            })
        })
    }
}

/// An `mln_camera_delta`'s fields, without the end handler's pointers.
struct Delta {
    fields: mln_camera_delta_fields,
    offset: mln_screen_point,
    scale: f64,
    bearing: f64,
    pitch: f64,
    anchor: mln_screen_point,
    animation: Animation,
    gesture_phase: mln_gesture_phase,
}

/// A host's gesture step on Orbis's camera: the pan moves the point under the
/// offset's start to its end, then zoom and rotation turn about the anchor.
fn apply_delta(entry: &mut MapEntry, d: &Delta) -> Result {
    let has = |bit| d.fields & bit != 0;
    if d.gesture_phase == MLN_GESTURE_PHASE_BEGIN {
        entry.map.stop();
    }
    let viewport = entry.map.viewport();
    let middle = glam::DVec2::new(viewport.width / 2.0, viewport.height / 2.0);
    if has(MLN_CAMERA_DELTA_OFFSET) && (d.offset.x != 0.0 || d.offset.y != 0.0) {
        let offset = glam::DVec2::new(d.offset.x, d.offset.y);
        if let Some(center) = entry.map.unproject(middle - offset) {
            entry.map.jump_to(&CameraTarget {
                center: Some(center),
                ..Default::default()
            });
        }
    }
    let camera = entry.map.camera();
    let mut t = CameraTarget::default();
    if has(MLN_CAMERA_DELTA_SCALE) && d.scale > 0.0 && d.scale.is_finite() {
        t.zoom = Some(entry.map.zoom() + d.scale.log2());
    }
    if has(MLN_CAMERA_DELTA_BEARING) {
        t.bearing = Some(camera.bearing + d.bearing);
    }
    if has(MLN_CAMERA_DELTA_PITCH) {
        t.pitch = Some(camera.pitch + d.pitch);
    }
    if t.zoom.is_none() && t.bearing.is_none() && t.pitch.is_none() {
        return Ok(());
    }
    if has(MLN_CAMERA_DELTA_ANCHOR) {
        t.around = entry
            .map
            .unproject(glam::DVec2::new(d.anchor.x, d.anchor.y));
    }
    let mode = if d.animation.fields & MLN_ANIMATION_OPTION_DURATION != 0 {
        MLN_CAMERA_UPDATE_MODE_EASE
    } else {
        MLN_CAMERA_UPDATE_MODE_JUMP
    };
    update_camera(entry, mode, &t, &d.animation)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_camera(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::map(map.0)?;
            handle.command(Completion::from_c(completion)?, |state, id| {
                let entry = &state.maps[&id];
                Ok(Value::of(mln_camera_query_result {
                    generation: entry.map.generation(),
                    camera: entry.camera(),
                }))
            })
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_cancel_camera_transitions(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::map(map.0)?;
            handle.command(Completion::from_c(completion)?, |state, id| {
                state.maps.get_mut(&id).expect("map").map.stop();
                Ok(Value::None)
            })
        })
    }
}

fn bounds(b: mln_lat_lng_bounds) -> orb_api::Bounds {
    orb_api::Bounds {
        west: b.southwest.longitude,
        south: b.southwest.latitude,
        east: b.northeast.longitude,
        north: b.northeast.latitude,
    }
}

fn fit_options(options: *const mln_camera_fit_options) -> Result<FitOptions> {
    let mut fit = FitOptions::default();
    if options.is_null() {
        return Ok(fit);
    }
    // SAFETY: the caller's pointer.
    let o = unsafe { sized(options, "fit_options")? };
    if o.fields & MLN_CAMERA_FIT_OPTION_PADDING != 0 {
        let p = o.padding;
        fit.padding = orb_api::Padding {
            top: p.top,
            right: p.right,
            bottom: p.bottom,
            left: p.left,
        };
    }
    if o.fields & MLN_CAMERA_FIT_OPTION_BEARING != 0 {
        fit.bearing = Some(o.bearing);
    }
    if o.fields & MLN_CAMERA_FIT_OPTION_PITCH != 0 {
        fit.pitch = Some(o.pitch);
    }
    Ok(fit)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_camera_for_lat_lng_bounds(
    map: mln_map,
    lat_lng_bounds: mln_lat_lng_bounds,
    fit: *const mln_camera_fit_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::map(map.0)?;
            let fit = fit_options(fit)?;
            handle.command(Completion::from_c(completion)?, move |state, id| {
                let entry = &state.maps[&id];
                let t = entry
                    .map
                    .camera_for_bounds(bounds(lat_lng_bounds), &fit)
                    .ok_or_else(|| Failure::argument("no camera shows these bounds"))?;
                let camera = entry.map.camera();
                let center = t.center.unwrap_or(camera.target);
                let mut options = mln_camera_options {
                    size: std::mem::size_of::<mln_camera_options>() as u32,
                    fields: MLN_CAMERA_OPTION_CENTER
                        | MLN_CAMERA_OPTION_ZOOM
                        | MLN_CAMERA_OPTION_BEARING
                        | MLN_CAMERA_OPTION_PITCH,
                    ..Default::default()
                };
                options.center = mln_lat_lng {
                    latitude: center.lat,
                    longitude: center.lon,
                };
                options.zoom = t.zoom.unwrap_or_else(|| entry.map.zoom());
                options.bearing = t.bearing.unwrap_or(camera.bearing);
                options.pitch = t.pitch.unwrap_or(camera.pitch);
                Ok(Value::of(options))
            })
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_snapshot(
    map: mln_map,
    out_snapshot: *mut mln_map_snapshot,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::map(map.0)?;
            if out_snapshot.is_null() {
                return Err(Failure::argument("out_snapshot is null"));
            }
            let size = (*out_snapshot).size;
            let snapshot = handle.query(|entry| {
                Ok(mln_map_snapshot {
                    size: std::mem::size_of::<mln_map_snapshot>() as u32,
                    generation: entry.map.generation(),
                    fully_loaded: entry.style_loaded && !entry.map.needs_frame(),
                    transition_in_progress: entry.map.needs_frame(),
                    event_mask: entry.event_mask,
                    latest_render_update_generation: entry.map.generation(),
                    view: entry.view(),
                    ..Default::default()
                })
            })?;
            if (size as usize) < std::mem::size_of::<mln_map_snapshot>() {
                return Err(Failure::argument("out_snapshot.size is too small"));
            }
            out_snapshot.write(snapshot);
            Ok(())
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_event_mask(
    map: mln_map,
    mask: mln_runtime_event_flags,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::map(map.0)?;
            handle.command(Completion::from_c(completion)?, move |state, id| {
                state.maps.get_mut(&id).expect("map").event_mask = mask;
                Ok(Value::None)
            })
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_request_repaint(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::map(map.0)?;
            handle.command(Completion::from_c(completion)?, |state, id| {
                if let Some(session) = state.maps[&id].session {
                    crate::session::force_redraw(state, session);
                }
                Ok(Value::None)
            })
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_camera_for_tile(
    tile: mln_canonical_tile_id,
    logical_size: mln_logical_size,
    out_camera: *mut mln_camera_options,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            if tile.z > 30 || u64::from(tile.x) >= 1 << tile.z || u64::from(tile.y) >= 1 << tile.z {
                return Err(Failure::argument("no such tile"));
            }
            if logical_size.width == 0 || logical_size.width != logical_size.height {
                return Err(Failure::argument("a tile's map is square"));
            }
            let n = f64::from(1u32 << tile.z);
            let lon = (f64::from(tile.x) + 0.5) / n * 360.0 - 180.0;
            let y = std::f64::consts::PI * (1.0 - 2.0 * (f64::from(tile.y) + 0.5) / n);
            let lat = y.sinh().atan().to_degrees();
            if out_camera.is_null() {
                return Err(Failure::argument("out_camera is null"));
            }
            let size = (*out_camera).size;
            let mut camera = mln_camera_options {
                size,
                fields: MLN_CAMERA_OPTION_CENTER
                    | MLN_CAMERA_OPTION_ZOOM
                    | MLN_CAMERA_OPTION_BEARING
                    | MLN_CAMERA_OPTION_PITCH,
                ..Default::default()
            };
            camera.center = mln_lat_lng {
                latitude: lat,
                longitude: lon,
            };
            camera.zoom = f64::from(tile.z) + (f64::from(logical_size.width) / 512.0).log2();
            out_camera.write(camera);
            Ok(())
        })
    }
}
