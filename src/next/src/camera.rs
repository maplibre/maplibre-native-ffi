// Ports maplibre-native-ffi src/c_api/camera.cpp and the camera commands of
// src/map/map.cpp (their validation, the conversions to and from MapLibre's
// camera types, map_update_camera, map_apply_camera_delta, the
// cancellations, the camera fits, map_set_bounds, the map options and the
// snapshot's camera members) onto `mln_map::Map`. camera_transition.rs
// ports the end handlers of camera commands.
//
// maplibre-native-next runs one camera transition at a time (see
// camera_transition.rs), so an animated camera delta that pans and also
// scales, rotates or pitches runs as one transition to the combined target,
// where the C++ library runs the pan apart.

use crate::camera_transition::{EndHandler, HandlerSpec};
use crate::delivery::Value;
use crate::diagnostics::{Failure, boundary};
use crate::map::{MapEntry, camera_of, command, command_target, post_command};
use crate::types::*;
use maplibre_native_next::geojson::{self, GeoJSON, Geometry};
use maplibre_native_next::maplibre_native_base::Point;
use maplibre_native_next::mln_map::{
    AnimationOptions, BoundOptions, CameraOptions, ConstrainMode, FreeCameraOptions, Map, MapDebugOptions,
    NorthOrientation, ProjectionMode, TileLodMode, ViewportMode,
};
use maplibre_native_next::mln_math::{deg2rad, rad2deg};
use maplibre_native_next::mln_util::chrono::Duration;
use maplibre_native_next::mln_util::constants::{MAX_ZOOM, MIN_ZOOM, TILE_SIZE_D};
use maplibre_native_next::mln_util::geo::{EdgeInsets, LatLng, LatLngBounds, ScreenCoordinate, WrapMode};
use maplibre_native_next::mln_util::logging::{Event, Log};
use maplibre_native_next::mln_util::projection::Projection;
use maplibre_native_next::mln_util::unitbezier::UnitBezier;
use std::rc::Rc;
use std::sync::Arc;

/// All zeros, for the plain C structs that this file fills.
fn zeroed<T: Copy>() -> T {
    // SAFETY: only used for the headers' plain-data structs, for which all
    // zeros is a valid value.
    unsafe { std::mem::zeroed() }
}

// MARK: - Validation (C++ map.cpp validate_*)

pub(crate) fn validate_lat_lng(coordinate: mln_lat_lng) -> Result<(), Failure> {
    let latitude = coordinate.latitude;
    if !latitude.is_finite() || !(-90.0..=90.0).contains(&latitude) || !coordinate.longitude.is_finite() {
        return Err(Failure::invalid_argument(
            "latitude must be finite and within [-90, 90], and longitude must be finite",
        ));
    }
    Ok(())
}

pub(crate) fn validate_lat_lng_bounds(bounds: mln_lat_lng_bounds) -> Result<(), Failure> {
    validate_lat_lng(bounds.southwest)?;
    validate_lat_lng(bounds.northeast)?;
    if bounds.southwest.latitude > bounds.northeast.latitude || bounds.southwest.longitude > bounds.northeast.longitude
    {
        return Err(Failure::invalid_argument("bounds southwest must be less than or equal to northeast"));
    }
    Ok(())
}

/// C++ `validate_lat_lng_array`: the caller's coordinates.
///
/// # Safety
/// `coordinates` is null or points at `count` readable coordinates.
pub(crate) unsafe fn lat_lng_slice<'a>(
    coordinates: *const mln_lat_lng,
    count: usize,
    allow_empty: bool,
) -> Result<&'a [mln_lat_lng], Failure> {
    if count == 0 {
        if allow_empty {
            return Ok(&[]);
        }
        return Err(Failure::invalid_argument("coordinate_count must be greater than 0"));
    }
    if coordinates.is_null() {
        return Err(Failure::invalid_argument("coordinates must not be null"));
    }
    // SAFETY: the caller's contract.
    let coordinates = unsafe { std::slice::from_raw_parts(coordinates, count) };
    for coordinate in coordinates {
        validate_lat_lng(*coordinate)?;
    }
    Ok(coordinates)
}

pub(crate) fn validate_screen_point(point: mln_screen_point) -> Result<(), Failure> {
    if !point.x.is_finite() || !point.y.is_finite() {
        return Err(Failure::invalid_argument("screen point values must be finite"));
    }
    Ok(())
}

/// C++ `validate_screen_point_array`: the caller's points.
///
/// # Safety
/// `points` is null or points at `count` readable points.
pub(crate) unsafe fn screen_point_slice<'a>(
    points: *const mln_screen_point,
    count: usize,
) -> Result<&'a [mln_screen_point], Failure> {
    if count == 0 {
        return Ok(&[]);
    }
    if points.is_null() {
        return Err(Failure::invalid_argument("points must not be null"));
    }
    // SAFETY: the caller's contract.
    let points = unsafe { std::slice::from_raw_parts(points, count) };
    for point in points {
        validate_screen_point(*point)?;
    }
    Ok(points)
}

pub(crate) fn validate_edge_insets(padding: mln_edge_insets) -> Result<(), Failure> {
    let values = [padding.top, padding.left, padding.bottom, padding.right];
    if values.iter().any(|value| !value.is_finite() || *value < 0.0) {
        return Err(Failure::invalid_argument("padding values must be finite and greater than or equal to 0"));
    }
    Ok(())
}

/// A validated coordinate.
pub(crate) fn lat_lng_of(coordinate: mln_lat_lng) -> Result<LatLng, Failure> {
    LatLng::new(coordinate.latitude, coordinate.longitude).map_err(Failure::invalid_argument)
}

pub(crate) fn lat_lng_to_c(coordinate: &LatLng) -> mln_lat_lng {
    mln_lat_lng { latitude: coordinate.latitude(), longitude: coordinate.longitude() }
}

/// Validated insets.
pub(crate) fn edge_insets_of(padding: mln_edge_insets) -> Result<EdgeInsets, Failure> {
    EdgeInsets::new(padding.top, padding.left, padding.bottom, padding.right).map_err(Failure::invalid_argument)
}

fn edge_insets_to_c(insets: &EdgeInsets) -> mln_edge_insets {
    mln_edge_insets { top: insets.top(), left: insets.left(), bottom: insets.bottom(), right: insets.right() }
}

fn lat_lng_bounds_to_c(bounds: &LatLngBounds) -> mln_lat_lng_bounds {
    mln_lat_lng_bounds {
        southwest: mln_lat_lng { latitude: bounds.south(), longitude: bounds.west() },
        northeast: mln_lat_lng { latitude: bounds.north(), longitude: bounds.east() },
    }
}

/// C++ `validate_camera_fields` + `to_native_camera`: the members of a
/// camera, whose size its container versions when it is embedded.
pub(crate) fn camera_options_of(camera: &mln_camera_options) -> Result<CameraOptions, Failure> {
    if camera.fields & !MLN_CAMERA_OPTION_ALL != 0 {
        return Err(Failure::invalid_argument("mln_camera_options.fields contains unknown bits"));
    }
    let has = |bit: u32| camera.fields & bit != 0;
    if has(MLN_CAMERA_OPTION_CENTER) {
        validate_lat_lng(camera.center)?;
    }
    let numeric = [
        (MLN_CAMERA_OPTION_CENTER_ALTITUDE, camera.center_altitude),
        (MLN_CAMERA_OPTION_ZOOM, camera.zoom),
        (MLN_CAMERA_OPTION_BEARING, camera.bearing),
        (MLN_CAMERA_OPTION_PITCH, camera.pitch),
        (MLN_CAMERA_OPTION_ROLL, camera.roll),
        (MLN_CAMERA_OPTION_FOV, camera.field_of_view),
    ];
    if numeric.iter().any(|(bit, value)| has(*bit) && !value.is_finite()) {
        return Err(Failure::invalid_argument("enabled camera numeric fields must be finite"));
    }
    if has(MLN_CAMERA_OPTION_PADDING) {
        validate_edge_insets(camera.padding)?;
    }
    if has(MLN_CAMERA_OPTION_ANCHOR) {
        validate_screen_point(camera.anchor)?;
    }

    let mut options = CameraOptions::default();
    if has(MLN_CAMERA_OPTION_CENTER) {
        options.center = Some(lat_lng_of(camera.center)?);
    }
    if has(MLN_CAMERA_OPTION_CENTER_ALTITUDE) {
        options.center_altitude = Some(camera.center_altitude);
    }
    if has(MLN_CAMERA_OPTION_PADDING) {
        options.padding = Some(edge_insets_of(camera.padding)?);
    }
    if has(MLN_CAMERA_OPTION_ANCHOR) {
        options.anchor = Some(ScreenCoordinate::new(camera.anchor.x, camera.anchor.y));
    }
    if has(MLN_CAMERA_OPTION_ZOOM) {
        options.zoom = Some(camera.zoom);
    }
    if has(MLN_CAMERA_OPTION_BEARING) {
        options.bearing = Some(camera.bearing);
    }
    if has(MLN_CAMERA_OPTION_PITCH) {
        options.pitch = Some(camera.pitch);
    }
    if has(MLN_CAMERA_OPTION_ROLL) {
        options.roll = Some(camera.roll);
    }
    if has(MLN_CAMERA_OPTION_FOV) {
        options.fov = Some(camera.field_of_view);
    }
    Ok(options)
}

/// C++ `validate_camera_options`: a camera passed by pointer, which versions
/// itself.
///
/// # Safety
/// `camera` is null or points at a readable `mln_camera_options`.
pub(crate) unsafe fn camera_options_at(camera: *const mln_camera_options) -> Result<CameraOptions, Failure> {
    if camera.is_null() {
        return Err(Failure::invalid_argument("camera must not be null"));
    }
    // SAFETY: the caller's contract.
    let Some(camera) = (unsafe { read_sized(camera) }) else {
        return Err(Failure::invalid_argument("mln_camera_options.size is too small"));
    };
    camera_options_of(&camera)
}

/// C++ `from_native_camera`: camera options with the members that MapLibre
/// set.
fn camera_options_to_c(camera: &CameraOptions) -> mln_camera_options {
    let mut result: mln_camera_options = zeroed();
    result.size = size_of::<mln_camera_options>() as u32;
    if let Some(center) = camera.center {
        result.fields |= MLN_CAMERA_OPTION_CENTER;
        result.center = lat_lng_to_c(&center);
    }
    if let Some(altitude) = camera.center_altitude {
        result.fields |= MLN_CAMERA_OPTION_CENTER_ALTITUDE;
        result.center_altitude = altitude;
    }
    if let Some(padding) = camera.padding {
        result.fields |= MLN_CAMERA_OPTION_PADDING;
        result.padding = edge_insets_to_c(&padding);
    }
    if let Some(anchor) = camera.anchor {
        result.fields |= MLN_CAMERA_OPTION_ANCHOR;
        result.anchor = mln_screen_point { x: anchor.x, y: anchor.y };
    }
    if let Some(zoom) = camera.zoom {
        result.fields |= MLN_CAMERA_OPTION_ZOOM;
        result.zoom = zoom;
    }
    if let Some(bearing) = camera.bearing {
        result.fields |= MLN_CAMERA_OPTION_BEARING;
        result.bearing = bearing;
    }
    if let Some(pitch) = camera.pitch {
        result.fields |= MLN_CAMERA_OPTION_PITCH;
        result.pitch = pitch;
    }
    if let Some(roll) = camera.roll {
        result.fields |= MLN_CAMERA_OPTION_ROLL;
        result.roll = roll;
    }
    if let Some(fov) = camera.fov {
        result.fields |= MLN_CAMERA_OPTION_FOV;
        result.field_of_view = fov;
    }
    result
}

/// `mln::Duration::max()` in milliseconds. A double can't hold it exactly,
/// so the accepted range ends below it (C++ `is_native_duration_ms`).
const MAX_DURATION_MS: f64 = i64::MAX as f64 / 1e6;

/// `mln_animation_options`, validated (C++ `validate_animation_options`).
/// `AnimationOptions` holds `Rc` callbacks, so the runtime thread builds it.
#[derive(Clone, Copy)]
struct Animation {
    duration: Option<Duration>,
    velocity: Option<f64>,
    min_zoom: Option<f64>,
    easing: Option<[f64; 4]>,
    transition_id: Option<u64>,
}

impl Animation {
    fn of(animation: &mln_animation_options) -> Result<Animation, Failure> {
        let known = MLN_ANIMATION_OPTION_DURATION
            | MLN_ANIMATION_OPTION_VELOCITY
            | MLN_ANIMATION_OPTION_MIN_ZOOM
            | MLN_ANIMATION_OPTION_EASING
            | MLN_ANIMATION_OPTION_TRANSITION_ID;
        let fields = animation.fields;
        if fields & !known != 0 {
            return Err(Failure::invalid_argument("mln_animation_options.fields contains unknown bits"));
        }
        let handler = &animation.end_handler;
        if handler.callback.is_none() && handler.release_user_data.is_some() {
            return Err(Failure::invalid_argument("a disabled camera transition handler must not retain user data"));
        }
        let has = |bit: u32| fields & bit != 0;
        let duration_ms = animation.duration_ms;
        if has(MLN_ANIMATION_OPTION_DURATION) && !(0.0..MAX_DURATION_MS).contains(&duration_ms) {
            return Err(Failure::invalid_argument("animation duration_ms must fit the native duration range"));
        }
        if has(MLN_ANIMATION_OPTION_VELOCITY) && !(animation.velocity.is_finite() && animation.velocity > 0.0) {
            return Err(Failure::invalid_argument("animation velocity must be positive and finite"));
        }
        if has(MLN_ANIMATION_OPTION_MIN_ZOOM) && !animation.min_zoom.is_finite() {
            return Err(Failure::invalid_argument("animation min_zoom must be finite"));
        }
        let easing = animation.easing;
        if has(MLN_ANIMATION_OPTION_EASING) {
            let values = [easing.x1, easing.y1, easing.x2, easing.y2];
            if values.iter().any(|value| !value.is_finite())
                || !(0.0..=1.0).contains(&easing.x1)
                || !(0.0..=1.0).contains(&easing.x2)
            {
                return Err(Failure::invalid_argument(
                    "animation easing x values must be within [0, 1] and all easing values must be finite",
                ));
            }
        }
        Ok(Animation {
            // C++ duration_cast<nanoseconds>(duration<double, milli>): truncation.
            duration: has(MLN_ANIMATION_OPTION_DURATION).then(|| Duration::from_nanos((duration_ms * 1e6) as i64)),
            velocity: has(MLN_ANIMATION_OPTION_VELOCITY).then_some(animation.velocity),
            min_zoom: has(MLN_ANIMATION_OPTION_MIN_ZOOM).then_some(animation.min_zoom),
            easing: has(MLN_ANIMATION_OPTION_EASING).then_some([easing.x1, easing.y1, easing.x2, easing.y2]),
            transition_id: has(MLN_ANIMATION_OPTION_TRANSITION_ID).then_some(animation.transition_id),
        })
    }

    /// Whether the change animates rather than applying at once.
    fn animates(&self) -> bool {
        self.duration.is_some_and(|duration| duration > Duration::zero())
    }

    fn options(&self) -> AnimationOptions {
        AnimationOptions {
            duration: self.duration,
            velocity: self.velocity,
            min_zoom: self.min_zoom,
            easing: self.easing.map(|[x1, y1, x2, y2]| UnitBezier::new(x1, y1, x2, y2)),
            ..Default::default()
        }
    }
}

/// `mln_camera_fit_options`, validated (C++ `validate_camera_fit_fields`).
#[derive(Clone, Copy, Default)]
struct Fit {
    padding: EdgeInsets,
    bearing: Option<f64>,
    pitch: Option<f64>,
}

impl Fit {
    /// Fit options whose size their container versions.
    fn of(options: &mln_camera_fit_options) -> Result<Fit, Failure> {
        let known = MLN_CAMERA_FIT_OPTION_PADDING | MLN_CAMERA_FIT_OPTION_BEARING | MLN_CAMERA_FIT_OPTION_PITCH;
        if options.fields & !known != 0 {
            return Err(Failure::invalid_argument("mln_camera_fit_options.fields contains unknown bits"));
        }
        let has = |bit: u32| options.fields & bit != 0;
        let mut fit = Fit::default();
        if has(MLN_CAMERA_FIT_OPTION_PADDING) {
            validate_edge_insets(options.padding)?;
            fit.padding = edge_insets_of(options.padding)?;
        }
        if (has(MLN_CAMERA_FIT_OPTION_BEARING) && !options.bearing.is_finite())
            || (has(MLN_CAMERA_FIT_OPTION_PITCH) && !options.pitch.is_finite())
        {
            return Err(Failure::invalid_argument("camera fit bearing and pitch must be finite"));
        }
        fit.bearing = has(MLN_CAMERA_FIT_OPTION_BEARING).then_some(options.bearing);
        fit.pitch = has(MLN_CAMERA_FIT_OPTION_PITCH).then_some(options.pitch);
        Ok(fit)
    }

    /// C++ `validate_camera_fit_options`: optional fit options passed by
    /// pointer.
    ///
    /// # Safety
    /// `options` is null or points at a readable `mln_camera_fit_options`.
    unsafe fn at(options: *const mln_camera_fit_options) -> Result<Fit, Failure> {
        if options.is_null() {
            return Ok(Fit::default());
        }
        // SAFETY: the caller's contract.
        let Some(options) = (unsafe { read_sized(options) }) else {
            return Err(Failure::invalid_argument("mln_camera_fit_options.size is too small"));
        };
        Fit::of(&options)
    }
}

/// C++ `CameraFitArea`: what a camera fit frames. A geometry fits as its
/// coordinates, as `Map::cameraForGeometry` does.
#[derive(Clone)]
enum FitArea {
    Bounds(LatLngBounds),
    Points(Vec<LatLng>),
}

/// C++ `camera_for_area`: the camera that fits `area` in the map's view.
/// MapLibre logs an error and keeps the zoom when the padding leaves no room
/// for the area, so the fit rejects that padding first.
fn camera_for_area(map: &Map, area: &FitArea, fit: &Fit) -> Result<CameraOptions, Failure> {
    let size = map.get_map_options().size();
    let padding = fit.padding;
    if padding.left() + padding.right() >= f64::from(size.width)
        || padding.top() + padding.bottom() >= f64::from(size.height)
    {
        return Err(Failure::invalid_argument("camera fit padding must be less than the map's width and height"));
    }
    Ok(match area {
        FitArea::Bounds(bounds) => map.camera_for_lat_lng_bounds(bounds, &padding, fit.bearing, fit.pitch),
        FitArea::Points(points) => map.camera_for_lat_lngs(points, &padding, fit.bearing, fit.pitch),
    })
}

fn bounds_of(bounds: mln_lat_lng_bounds) -> Result<LatLngBounds, Failure> {
    Ok(LatLngBounds::hull(lat_lng_of(bounds.southwest)?, lat_lng_of(bounds.northeast)?))
}

/// C++ `to_native_geometry` + `geometry_lat_lngs`: the coordinates of a
/// GeoJSON geometry, which must have at least one.
fn geometry_points(geometry: mln_buffer_view) -> Result<Vec<LatLng>, Failure> {
    if geometry.size == 0 {
        return Err(Failure::invalid_argument("geometry must not be empty"));
    }
    // SAFETY: C API contract for buffer views.
    let Some(bytes) = (unsafe { geometry.bytes() }) else {
        return Err(Failure::invalid_argument("geometry data must not be null"));
    };
    if bytes.contains(&0) {
        return Err(Failure::invalid_argument("geometry must not contain embedded NUL"));
    }
    let text = std::str::from_utf8(bytes)
        .map_err(|e| Failure::invalid_argument(format!("geometry is not valid JSON: {e}")))?;
    let parsed = geojson::parse(text).map_err(|e| Failure::invalid_argument(format!("geometry is invalid: {e}")))?;
    let GeoJSON::Geometry(geometry) = parsed else {
        return Err(Failure::invalid_argument("geometry must contain one GeoJSON Geometry"));
    };
    let mut points = Vec::new();
    for_each_point(&geometry, &mut |point| points.push(*point));
    if points.is_empty() {
        return Err(Failure::invalid_argument("geometry must contain at least one coordinate"));
    }
    points
        .iter()
        .map(|point| {
            let coordinate = mln_lat_lng { latitude: point.y, longitude: point.x };
            validate_lat_lng(coordinate)?;
            lat_lng_of(coordinate)
        })
        .collect()
}

/// `mln::forEachPoint`.
fn for_each_point(geometry: &Geometry<f64>, f: &mut impl FnMut(&Point<f64>)) {
    match geometry {
        Geometry::Empty => {}
        Geometry::Point(point) => f(point),
        Geometry::MultiPoint(points) | Geometry::LineString(points) => points.iter().for_each(f),
        Geometry::MultiLineString(lines) | Geometry::Polygon(lines) => lines.iter().flatten().for_each(f),
        Geometry::MultiPolygon(polygons) => polygons.iter().flatten().flatten().for_each(f),
        Geometry::GeometryCollection(geometries) => {
            for geometry in geometries {
                for_each_point(geometry, f);
            }
        }
    }
}

// MARK: - Moving the camera

fn cancel_all_camera_transitions(entry: &MapEntry) {
    entry.camera.cancelling(|| entry.map.borrow_mut().cancel_transitions());
}

/// C++ `apply_gesture_phase_before`: BEGIN cancels running transitions, and
/// BEGIN and UPDATE mark the gesture, before the camera write, so MapLibre
/// applies its gesture rules to that write.
fn apply_gesture_phase_before(entry: &MapEntry, phase: u32) {
    if phase == MLN_GESTURE_PHASE_BEGIN {
        cancel_all_camera_transitions(entry);
    }
    if phase == MLN_GESTURE_PHASE_BEGIN || phase == MLN_GESTURE_PHASE_UPDATE {
        entry.map.borrow_mut().set_gesture_in_progress(true);
    }
}

fn apply_gesture_phase_after(entry: &MapEntry, phase: u32) {
    if phase == MLN_GESTURE_PHASE_CANCEL {
        cancel_all_camera_transitions(entry);
    }
    if phase == MLN_GESTURE_PHASE_END || phase == MLN_GESTURE_PHASE_CANCEL {
        entry.map.borrow_mut().set_gesture_in_progress(false);
    }
}

/// The range of `Duration` in seconds; a double can't hold its end exactly.
const MAX_DURATION_SECONDS: f64 = i64::MAX as f64 / 1e9;

/// The duration that a flight without one derives from its speed, as
/// `Transform::flyTo` computes it. C++ rejects a duration that doesn't fit
/// `mln::Duration` (patches/maplibre-native/0044), where maplibre-native-next
/// would saturate it.
fn fly_duration(map: &Map, input: &CameraOptions, animation: &Animation) -> Result<Duration, Failure> {
    let state = map.get_transform_state();
    let mut camera = *input;
    let mut zoom = camera.zoom.unwrap_or(state.get_zoom());
    state.constrain_camera_and_zoom_to_bounds(&mut camera, &mut zoom);
    let size = state.get_size();
    if size.is_empty() {
        // The flight ends at once.
        return Ok(Duration::zero());
    }
    let padding = camera.padding.unwrap_or(state.get_edge_insets());
    let end = camera.center.unwrap_or(state.get_lat_lng(WrapMode::Unwrapped)).wrapped();
    let mut start = state.get_lat_lng(WrapMode::Unwrapped).wrapped();
    start.unwrap_for_shortest_path(&end);
    let start_point = Projection::project(&start, state.get_scale());
    let end_point = Projection::project(&end, state.get_scale());
    let zoom = zoom.clamp(state.get_min_zoom(), state.get_max_zoom());
    let start_zoom = state.scale_zoom(state.get_scale());

    let w0 = (f64::from(size.width) - padding.left() - padding.right())
        .max(f64::from(size.height) - padding.top() - padding.bottom());
    let w1 = w0 / state.zoom_scale(zoom - start_zoom);
    let u1 = (end_point.x - start_point.x).hypot(end_point.y - start_point.y);
    let mut rho: f64 = 1.42;
    if let Some(min_zoom) = animation.min_zoom {
        let min_zoom = min_zoom.min(start_zoom.min(zoom)).clamp(state.get_min_zoom(), state.get_max_zoom());
        let w_max = w0 / state.zoom_scale(min_zoom - start_zoom);
        rho = if u1 != 0.0 { (w_max / u1 * 2.0).sqrt() } else { 1.0 };
    }
    let rho2 = rho * rho;
    let r = |i: f64| {
        let b = (w1 * w1 - w0 * w0 + (if i != 0.0 { -1.0 } else { 1.0 }) * rho2 * rho2 * u1 * u1)
            / (2.0 * (if i != 0.0 { w1 } else { w0 }) * rho2 * u1);
        ((b * b + 1.0).sqrt() - b).ln()
    };
    let r0 = if u1 != 0.0 { r(0.0) } else { f64::INFINITY };
    let r1 = if u1 != 0.0 { r(1.0) } else { f64::INFINITY };
    let is_close = u1.abs() < 0.000001 || !r0.is_finite() || !r1.is_finite();
    let total = if is_close { (w1 / w0).ln().abs() / rho } else { (r1 - r0) / rho };
    let velocity = animation.velocity.map_or(1.2, |velocity| velocity / rho);
    let seconds = total / velocity;
    if !seconds.is_finite() || seconds < 0.0 || seconds >= MAX_DURATION_SECONDS {
        return Err(Failure::new(MLN_STATUS_NATIVE_ERROR, "flyTo duration must fit the native duration range"));
    }
    Ok(Duration::from_nanos((seconds * 1e9) as i64))
}

/// C++ `move_camera`: moves the camera for a camera command, to the camera
/// that `target` resolves on the runtime thread. The command's end is
/// tracked before `target` runs, so a target that fails ends the command as
/// cancelled.
fn move_camera(
    entry: &Rc<MapEntry>,
    mode: u32,
    animation: &Animation,
    gesture_phase: u32,
    end: Option<Arc<EndHandler>>,
    target: impl FnOnce(&Map) -> Result<CameraOptions, Failure>,
) -> Result<(), Failure> {
    let state = &entry.camera;
    let command = state.command(end, animation.transition_id, 1);
    let camera = target(&entry.map.borrow())?;
    apply_gesture_phase_before(entry, gesture_phase);
    let base = animation.options();
    match mode {
        MLN_CAMERA_UPDATE_MODE_JUMP => {
            let options = state.transition(&command, &base, None);
            state.replacing(&command, || {
                entry.map.borrow_mut().jump_to(&camera);
                // MapLibre gives a jump no animation options, so the jump ends
                // its command here, as soon as it applies.
                if let Some(finish) = &options.transition_finish_fn {
                    finish();
                }
            });
        }
        MLN_CAMERA_UPDATE_MODE_EASE => {
            let options = state.transition(&command, &base, animation.duration);
            state.replacing(&command, || entry.map.borrow_mut().ease_to(&camera, &options));
        }
        _ => {
            let duration = match animation.duration {
                Some(duration) => duration,
                None => fly_duration(&entry.map.borrow(), &camera, animation)?,
            };
            let options = state.transition(&command, &base, Some(duration));
            state.replacing(&command, || entry.map.borrow_mut().fly_to(&camera, &options));
        }
    }
    apply_gesture_phase_after(entry, gesture_phase);
    Ok(())
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
                return Err(Failure::invalid_argument("camera update must have a valid size"));
            };
            if update.mode > MLN_CAMERA_UPDATE_MODE_FLY || update.gesture_phase > MLN_GESTURE_PHASE_CANCEL {
                return Err(Failure::invalid_argument("camera update mode or gesture phase is invalid"));
            }
            let camera = camera_options_of(&update.camera)?;
            let animation = Animation::of(&update.animation)?;
            let handler = HandlerSpec::of(&update.animation.end_handler)?;
            let (shared, completion) = command_target(map, completion, true)?;
            let end = handler.map(HandlerSpec::accept);
            let (mode, gesture_phase) = (update.mode, update.gesture_phase);
            post_command(shared, completion, move |entry| {
                move_camera(entry, mode, &animation, gesture_phase, end, |_| Ok(camera))?;
                Ok(Value::None)
            });
            Ok(())
        })
    }
}

const DELTA_ANCHORED_FIELDS: u32 = MLN_CAMERA_DELTA_SCALE | MLN_CAMERA_DELTA_BEARING | MLN_CAMERA_DELTA_PITCH;

/// C++ `validate_camera_delta`.
fn validate_camera_delta(delta: &mln_camera_delta) -> Result<Animation, Failure> {
    let known = MLN_CAMERA_DELTA_OFFSET | DELTA_ANCHORED_FIELDS | MLN_CAMERA_DELTA_ANCHOR;
    let fields = delta.fields;
    if fields & !known != 0 {
        return Err(Failure::invalid_argument("mln_camera_delta.fields contains unknown bits"));
    }
    if delta.gesture_phase > MLN_GESTURE_PHASE_CANCEL {
        return Err(Failure::invalid_argument("camera delta gesture phase is invalid"));
    }
    let has = |bit: u32| fields & bit != 0;
    if has(MLN_CAMERA_DELTA_OFFSET) {
        validate_screen_point(delta.offset)?;
    }
    if has(MLN_CAMERA_DELTA_SCALE) && !(delta.scale.is_finite() && delta.scale > 0.0) {
        return Err(Failure::invalid_argument("camera delta scale must be finite and positive"));
    }
    if (has(MLN_CAMERA_DELTA_BEARING) && !delta.bearing.is_finite())
        || (has(MLN_CAMERA_DELTA_PITCH) && !delta.pitch.is_finite())
    {
        return Err(Failure::invalid_argument("camera delta bearing and pitch must be finite"));
    }
    if has(MLN_CAMERA_DELTA_ANCHOR) {
        if fields & DELTA_ANCHORED_FIELDS == 0 {
            return Err(Failure::invalid_argument("a camera delta anchor requires scale, bearing, or pitch"));
        }
        validate_screen_point(delta.anchor)?;
    }
    let animation = Animation::of(&delta.animation)?;
    // An anchored ease couples the center with its fields, so an animated one
    // would replace the animated pan. An immediate pan has already applied
    // when the ease reads the camera.
    let anchored_pan = MLN_CAMERA_DELTA_OFFSET | MLN_CAMERA_DELTA_ANCHOR;
    if fields & anchored_pan == anchored_pan
        && delta.animation.fields & MLN_ANIMATION_OPTION_DURATION != 0
        && delta.animation.duration_ms > 0.0
    {
        return Err(Failure::invalid_argument("a camera delta anchor combines with offset only in an immediate delta"));
    }
    Ok(animation)
}

/// C++ `camera_delta_target`: the camera that a delta's scale, bearing, and
/// pitch reach from the camera as it stands. `Map::scaleBy` adds log2(scale)
/// to the zoom the same way.
fn camera_delta_target(map: &Map, delta: &mln_camera_delta) -> CameraOptions {
    let current = map.get_camera_options(None);
    let mut camera = CameraOptions::default();
    if delta.fields & MLN_CAMERA_DELTA_SCALE != 0 {
        camera.zoom = Some(current.zoom.unwrap_or(0.0) + delta.scale.log2());
    }
    if delta.fields & MLN_CAMERA_DELTA_BEARING != 0 {
        camera.bearing = Some(current.bearing.unwrap_or(0.0) + delta.bearing);
    }
    if delta.fields & MLN_CAMERA_DELTA_PITCH != 0 {
        camera.pitch = Some(current.pitch.unwrap_or(0.0) + delta.pitch);
    }
    if delta.fields & MLN_CAMERA_DELTA_ANCHOR != 0 {
        camera.anchor = Some(ScreenCoordinate::new(delta.anchor.x, delta.anchor.y));
    }
    camera
}

/// The center that `Transform::moveBy` pans to: the coordinate at the
/// padded view's center less the offset, which it shortens to keep short of
/// the horizon.
fn pan_target(map: &Map, offset: ScreenCoordinate) -> LatLng {
    let state = map.get_transform_state();
    let mut center_offset = offset;
    let pitch = state.get_pitch();
    let length = offset.x.hypot(offset.y);
    if pitch > 0.0 && length > 0.0 {
        let camera_to_center =
            0.5 * f64::from(state.get_size().height) / (f64::from(state.get_field_of_view()) / 2.0).tan();
        let pixels_to_horizon = (camera_to_center / pitch.tan()).abs();
        let scale = pixels_to_horizon * 0.75 / length;
        if scale < 1.0 {
            center_offset = ScreenCoordinate::new(offset.x * scale, offset.y * scale);
        }
    }
    let size = state.get_size();
    let center = state.get_edge_insets().get_center(size.width as u16, size.height as u16);
    let point = ScreenCoordinate::new(center.x - center_offset.x, center.y - center_offset.y);
    map.lat_lng_for_pixel(&point, WrapMode::Unwrapped)
}

/// C++ `map_apply_camera_delta`'s work.
fn apply_camera_delta(
    entry: &Rc<MapEntry>,
    delta: &mln_camera_delta,
    animation: &Animation,
    end: Option<Arc<EndHandler>>,
) {
    let state = &entry.camera;
    let pans = delta.fields & MLN_CAMERA_DELTA_OFFSET != 0;
    let eases = delta.fields & DELTA_ANCHORED_FIELDS != 0;
    let offset = ScreenCoordinate::new(delta.offset.x, delta.offset.y);
    // An immediate pan applies before the ease reads the camera, so it moves
    // the camera that an anchor refers to. An animated delta that pans and
    // eases animates both as one transition, because MapLibre runs one at a
    // time.
    let combined = pans && eases && animation.animates();
    let transitions = if combined { 1 } else { u32::from(pans) + u32::from(eases) };
    let command = state.command(end, animation.transition_id, transitions.max(1));
    apply_gesture_phase_before(entry, delta.gesture_phase);
    let base = animation.options();
    if combined {
        let camera = {
            let map = entry.map.borrow();
            CameraOptions { center: Some(pan_target(&map, offset)), ..camera_delta_target(&map, delta) }
        };
        let options = state.transition(&command, &base, animation.duration);
        state.replacing(&command, || entry.map.borrow_mut().ease_to(&camera, &options));
    } else {
        if pans {
            let options = state.transition(&command, &base, animation.duration);
            state.replacing(&command, || entry.map.borrow_mut().move_by(&offset, &options));
        }
        if eases {
            let camera = camera_delta_target(&entry.map.borrow(), delta);
            let options = state.transition(&command, &base, animation.duration);
            state.replacing(&command, || entry.map.borrow_mut().ease_to(&camera, &options));
        }
    }
    // A delta without components starts no transition, so it ends its
    // command here, as an empty camera update does.
    if transitions == 0 {
        command.finish(None);
    }
    apply_gesture_phase_after(entry, delta.gesture_phase);
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_apply_camera_delta(
    map: mln_map,
    delta: *const mln_camera_delta,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let Some(delta) = read_sized(delta) else {
                return Err(Failure::invalid_argument("camera delta must have a valid size"));
            };
            let animation = validate_camera_delta(&delta)?;
            let handler = HandlerSpec::of(&delta.animation.end_handler)?;
            let (shared, completion) = command_target(map, completion, true)?;
            let end = handler.map(HandlerSpec::accept);
            let delta = SendDelta(delta);
            post_command(shared, completion, move |entry| {
                let delta = delta;
                apply_camera_delta(entry, &delta.0, &animation, end);
                Ok(Value::None)
            });
            Ok(())
        })
    }
}

/// A validated delta, whose animation's end handler pointers stay unread.
struct SendDelta(mln_camera_delta);
// SAFETY: the runtime thread reads only the delta's plain members; the end
// handler crosses separately as an `EndHandler`.
unsafe impl Send for SendDelta {}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_cancel_camera_transitions(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            command(map, completion, true, |entry| {
                cancel_all_camera_transitions(entry);
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_cancel_camera_transitions_with_id(
    map: mln_map,
    transition_id: u64,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            command(map, completion, true, move |entry| {
                // MapLibre runs one transition at a time, which this
                // cancels when its command has the id.
                if entry.camera.running_transition_id() == Some(transition_id) {
                    entry.camera.cancelling_id(transition_id, || entry.map.borrow_mut().cancel_transitions());
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
                    generation: entry.shared.generation.load(std::sync::atomic::Ordering::SeqCst),
                    camera,
                }))
            })
        })
    }
}

// MARK: - Camera fits

/// Runs a fit query (`mln_map_camera_for_*`).
///
/// # Safety
/// `completion` as for `Completion::required`.
unsafe fn fit_query(map: mln_map, area: FitArea, fit: Fit, completion: *const mln_completion) -> Result<(), Failure> {
    // SAFETY: the caller's contract.
    unsafe {
        command(map, completion, false, move |entry| {
            let camera = camera_for_area(&entry.map.borrow(), &area, &fit)?;
            Ok(Value::of_struct(camera_options_to_c(&camera)))
        })
    }
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_camera_for_lat_lng_bounds(
    map: mln_map,
    bounds: mln_lat_lng_bounds,
    fit_options: *const mln_camera_fit_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            validate_lat_lng_bounds(bounds)?;
            let fit = Fit::at(fit_options)?;
            fit_query(map, FitArea::Bounds(bounds_of(bounds)?), fit, completion)
        })
    }
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_camera_for_lat_lngs(
    map: mln_map,
    coordinates: *const mln_lat_lng,
    coordinate_count: usize,
    fit_options: *const mln_camera_fit_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let coordinates = lat_lng_slice(coordinates, coordinate_count, false)?;
            let fit = Fit::at(fit_options)?;
            let points = coordinates.iter().map(|c| lat_lng_of(*c)).collect::<Result<_, _>>()?;
            fit_query(map, FitArea::Points(points), fit, completion)
        })
    }
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_camera_for_geometry(
    map: mln_map,
    geometry: mln_buffer_view,
    fit_options: *const mln_camera_fit_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let points = geometry_points(geometry)?;
            let fit = Fit::at(fit_options)?;
            fit_query(map, FitArea::Points(points), fit, completion)
        })
    }
}

/// C++ `submit_camera_fit`: moves the camera to fit `area`, which the map
/// computes when it runs the command.
///
/// # Safety
/// `update` is null or points at a readable `mln_camera_fit_update`;
/// `completion` as for `Completion::optional`.
unsafe fn fit_command(
    map: mln_map,
    area: impl FnOnce() -> Result<FitArea, Failure>,
    update: *const mln_camera_fit_update,
    completion: *const mln_completion,
) -> Result<(), Failure> {
    let area = area()?;
    // SAFETY: the caller's contract.
    let Some(update) = (unsafe { read_sized(update) }) else {
        return Err(Failure::invalid_argument("camera fit update must have a valid size"));
    };
    if update.mode > MLN_CAMERA_UPDATE_MODE_FLY {
        return Err(Failure::invalid_argument("camera fit update mode is invalid"));
    }
    let fit = Fit::of(&update.fit)?;
    let animation = Animation::of(&update.animation)?;
    let handler = HandlerSpec::of(&update.animation.end_handler)?;
    // SAFETY: the caller's contract.
    let (shared, completion) = unsafe { command_target(map, completion, true) }?;
    let end = handler.map(HandlerSpec::accept);
    let mode = update.mode;
    post_command(shared, completion, move |entry| {
        move_camera(entry, mode, &animation, MLN_GESTURE_PHASE_NONE, end, |map| camera_for_area(map, &area, &fit))?;
        Ok(Value::None)
    });
    Ok(())
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_fit_camera_to_lat_lng_bounds(
    map: mln_map,
    bounds: mln_lat_lng_bounds,
    update: *const mln_camera_fit_update,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            fit_command(
                map,
                || {
                    validate_lat_lng_bounds(bounds)?;
                    Ok(FitArea::Bounds(bounds_of(bounds)?))
                },
                update,
                completion,
            )
        })
    }
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_fit_camera_to_lat_lngs(
    map: mln_map,
    coordinates: *const mln_lat_lng,
    coordinate_count: usize,
    update: *const mln_camera_fit_update,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            fit_command(
                map,
                || {
                    let coordinates = lat_lng_slice(coordinates, coordinate_count, false)?;
                    Ok(FitArea::Points(coordinates.iter().map(|c| lat_lng_of(*c)).collect::<Result<_, _>>()?))
                },
                update,
                completion,
            )
        })
    }
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_fit_camera_to_geometry(
    map: mln_map,
    geometry: mln_buffer_view,
    update: *const mln_camera_fit_update,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            fit_command(map, || Ok(FitArea::Points(geometry_points(geometry)?)), update, completion)
        })
    }
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_lat_lng_bounds_for_camera(
    map: mln_map,
    camera: *const mln_camera_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let camera = camera_options_at(camera)?;
            command(map, completion, false, move |entry| {
                // The engine's wrapped variant takes the hull of only two
                // screen corners, which misses rotated corners and flips
                // across the antimeridian.
                let bounds = entry.map.borrow().lat_lng_bounds_for_camera_unwrapped(&camera);
                Ok(Value::of_struct(lat_lng_bounds_to_c(&bounds)))
            })
        })
    }
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_camera_for_tile(
    tile: mln_canonical_tile_id,
    logical_size: mln_logical_size,
    out_camera: *mut mln_camera_options,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            // A vector source's tiles are 512 logical pixels wide at their own
            // zoom, so a wider map would show more than one.
            let side = logical_size.width;
            if side == 0 || side != logical_size.height || f64::from(side) > TILE_SIZE_D {
                return Err(Failure::invalid_argument("logical_size must be square, from 1 to 512 wide"));
            }
            // A narrower map shows a tile at a zoom offset by the ratio of the
            // widths. MapLibre's tile cover also counts a tile that only
            // touches the view's edge, so the camera zooms in by a sliver,
            // which moves the tile's edges by under 0.02 logical pixels.
            const EDGE_MARGIN_ZOOM: f64 = 1e-4;
            let fit_zoom = f64::from(tile.z) + (f64::from(side) / TILE_SIZE_D).log2();
            let zoom = fit_zoom + EDGE_MARGIN_ZOOM;
            if fit_zoom < MIN_ZOOM || zoom > MAX_ZOOM {
                return Err(Failure::invalid_argument(
                    "a map of logical_size can't show a tile at zoom tile.z from zoom 0 to 25.5",
                ));
            }
            // The zoom check bounds tile.z well below 64.
            let tile_count = 1u64 << tile.z;
            if u64::from(tile.x) >= tile_count || u64::from(tile.y) >= tile_count {
                return Err(Failure::invalid_argument("tile.x and tile.y must be less than 2^tile.z"));
            }
            if out_camera.is_null() || ((*out_camera).size as usize) < size_of::<mln_camera_options>() {
                return Err(Failure::invalid_argument("out_camera must not be null and must have a valid size"));
            }
            // The tile's center in Web Mercator, in world pixels at scale 2^z.
            let center = Projection::unproject(
                &Point::new((f64::from(tile.x) + 0.5) * TILE_SIZE_D, (f64::from(tile.y) + 0.5) * TILE_SIZE_D),
                tile_count as f64,
                WrapMode::Unwrapped,
            )
            .map_err(|e| Failure::new(MLN_STATUS_NATIVE_ERROR, e))?;
            let mut camera: mln_camera_options = zeroed();
            camera.size = size_of::<mln_camera_options>() as u32;
            camera.fields = MLN_CAMERA_OPTION_CENTER
                | MLN_CAMERA_OPTION_ZOOM
                | MLN_CAMERA_OPTION_BEARING
                | MLN_CAMERA_OPTION_PITCH
                | MLN_CAMERA_OPTION_ROLL
                | MLN_CAMERA_OPTION_PADDING
                | MLN_CAMERA_OPTION_CENTER_ALTITUDE;
            camera.center = lat_lng_to_c(&center);
            camera.zoom = zoom;
            *out_camera = camera;
            Ok(())
        })
    }
}

// MARK: - Limits and options

/// C++ `validate_bound_options` + `to_native_bound_options`.
///
/// # Safety
/// `options` is null or points at a readable `mln_bound_options`.
unsafe fn bound_options_at(options: *const mln_bound_options) -> Result<BoundOptions, Failure> {
    if options.is_null() {
        return Err(Failure::invalid_argument("bound options must not be null"));
    }
    // SAFETY: the caller's contract.
    let Some(options) = (unsafe { read_sized(options) }) else {
        return Err(Failure::invalid_argument("mln_bound_options.size is too small"));
    };
    let known = MLN_BOUND_OPTION_BOUNDS
        | MLN_BOUND_OPTION_MIN_ZOOM
        | MLN_BOUND_OPTION_MAX_ZOOM
        | MLN_BOUND_OPTION_MIN_PITCH
        | MLN_BOUND_OPTION_MAX_PITCH
        | MLN_BOUND_OPTION_UNBOUNDED;
    if options.fields & !known != 0 {
        return Err(Failure::invalid_argument("mln_bound_options.fields contains unknown bits"));
    }
    let has = |bit: u32| options.fields & bit != 0;
    if has(MLN_BOUND_OPTION_BOUNDS) && has(MLN_BOUND_OPTION_UNBOUNDED) {
        return Err(Failure::invalid_argument(
            "MLN_BOUND_OPTION_BOUNDS and MLN_BOUND_OPTION_UNBOUNDED are mutually exclusive",
        ));
    }
    if has(MLN_BOUND_OPTION_BOUNDS) {
        validate_lat_lng_bounds(options.bounds)?;
    }
    let numeric = [
        (MLN_BOUND_OPTION_MIN_ZOOM, options.min_zoom),
        (MLN_BOUND_OPTION_MAX_ZOOM, options.max_zoom),
        (MLN_BOUND_OPTION_MIN_PITCH, options.min_pitch),
        (MLN_BOUND_OPTION_MAX_PITCH, options.max_pitch),
    ];
    if numeric.iter().any(|(bit, value)| has(*bit) && !value.is_finite()) {
        return Err(Failure::invalid_argument("bound numeric fields must be finite"));
    }
    if has(MLN_BOUND_OPTION_MIN_ZOOM) && has(MLN_BOUND_OPTION_MAX_ZOOM) && options.min_zoom > options.max_zoom {
        return Err(Failure::invalid_argument("min_zoom must be less than or equal to max_zoom"));
    }
    if has(MLN_BOUND_OPTION_MIN_PITCH) && has(MLN_BOUND_OPTION_MAX_PITCH) && options.min_pitch > options.max_pitch {
        return Err(Failure::invalid_argument("min_pitch must be less than or equal to max_pitch"));
    }
    let mut result = BoundOptions::default();
    if has(MLN_BOUND_OPTION_BOUNDS) {
        result.bounds = Some(bounds_of(options.bounds)?);
    }
    if has(MLN_BOUND_OPTION_UNBOUNDED) {
        // The default LatLngBounds is MapLibre's unbounded constraint.
        result.bounds = Some(LatLngBounds::default());
    }
    result.min_zoom = has(MLN_BOUND_OPTION_MIN_ZOOM).then_some(options.min_zoom);
    result.max_zoom = has(MLN_BOUND_OPTION_MAX_ZOOM).then_some(options.max_zoom);
    result.min_pitch = has(MLN_BOUND_OPTION_MIN_PITCH).then_some(options.min_pitch);
    result.max_pitch = has(MLN_BOUND_OPTION_MAX_PITCH).then_some(options.max_pitch);
    Ok(result)
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_bounds(
    map: mln_map,
    options: *const mln_bound_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let options = bound_options_at(options)?;
            command(map, completion, true, move |entry| {
                // MapLibre applies only the limits that are set, and jumps to
                // move the camera inside the new ones.
                entry
                    .camera
                    .replacing_any(|| entry.map.borrow_mut().set_bounds(&options))
                    .map_err(|e| Failure::new(MLN_STATUS_NATIVE_ERROR, e))?;
                Ok(Value::None)
            })
        })
    }
}

/// C++ `validate_free_camera_options` + `to_native_free_camera`.
///
/// # Safety
/// `options` is null or points at a readable `mln_free_camera_options`.
unsafe fn free_camera_options_at(options: *const mln_free_camera_options) -> Result<FreeCameraOptions, Failure> {
    if options.is_null() {
        return Err(Failure::invalid_argument("free camera options must not be null"));
    }
    // SAFETY: the caller's contract.
    let Some(options) = (unsafe { read_sized(options) }) else {
        return Err(Failure::invalid_argument("mln_free_camera_options.size is too small"));
    };
    if options.fields & !(MLN_FREE_CAMERA_OPTION_POSITION | MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0 {
        return Err(Failure::invalid_argument("mln_free_camera_options.fields contains unknown bits"));
    }
    let mut result = FreeCameraOptions::default();
    if options.fields & MLN_FREE_CAMERA_OPTION_POSITION != 0 {
        let p = options.position;
        if !(p.x.is_finite() && p.y.is_finite() && p.z.is_finite()) {
            return Err(Failure::invalid_argument("free camera position values must be finite"));
        }
        result.position = Some([p.x, p.y, p.z]);
    }
    if options.fields & MLN_FREE_CAMERA_OPTION_ORIENTATION != 0 {
        let q = options.orientation;
        if !(q.x.is_finite() && q.y.is_finite() && q.z.is_finite() && q.w.is_finite()) {
            return Err(Failure::invalid_argument("free camera orientation values must be finite"));
        }
        if q.x == 0.0 && q.y == 0.0 && q.z == 0.0 && q.w == 0.0 {
            return Err(Failure::invalid_argument("free camera orientation must not be zero length"));
        }
        result.orientation = Some([q.x, q.y, q.z, q.w]);
    }
    Ok(result)
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_free_camera_options(
    map: mln_map,
    options: *const mln_free_camera_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let options = free_camera_options_at(options)?;
            command(map, completion, true, move |entry| {
                // maplibre-native-next's Map has no setFreeCameraOptions yet:
                // the camera that the transform state derives from the
                // options applies as a jump, after the cancellation that
                // Transform::setFreeCameraOptions makes.
                let camera = {
                    let mut state = entry.map.borrow().get_transform_state();
                    state.set_free_camera_options(&options);
                    state.get_camera_options(None)
                };
                entry.camera.cancelling(|| {
                    let mut map = entry.map.borrow_mut();
                    map.cancel_transitions();
                    map.jump_to(&camera);
                });
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_projection_mode(
    map: mln_map,
    options: *const mln_projection_mode_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let Some(options) = read_sized(options) else {
                return Err(Failure::invalid_argument("options must not be null and must have a valid size"));
            };
            let known = MLN_PROJECTION_MODE_OPTION_AXONOMETRIC
                | MLN_PROJECTION_MODE_OPTION_X_SKEW
                | MLN_PROJECTION_MODE_OPTION_Y_SKEW;
            if options.fields & !known != 0 {
                return Err(Failure::invalid_argument("projection mode fields contain unknown bits"));
            }
            let has = |bit: u32| options.fields & bit != 0;
            if (has(MLN_PROJECTION_MODE_OPTION_X_SKEW) && !options.x_skew.is_finite())
                || (has(MLN_PROJECTION_MODE_OPTION_Y_SKEW) && !options.y_skew.is_finite())
            {
                return Err(Failure::invalid_argument("projection skew values must be finite"));
            }
            let mode = ProjectionMode {
                axonometric: has(MLN_PROJECTION_MODE_OPTION_AXONOMETRIC).then_some(options.axonometric),
                x_skew: has(MLN_PROJECTION_MODE_OPTION_X_SKEW).then_some(options.x_skew),
                y_skew: has(MLN_PROJECTION_MODE_OPTION_Y_SKEW).then_some(options.y_skew),
            };
            command(map, completion, true, move |entry| {
                entry.map.borrow_mut().set_projection_mode(&mode);
                Ok(Value::None)
            })
        })
    }
}

const DEBUG_FLAGS: u32 = MLN_MAP_DEBUG_TILE_BORDERS
    | MLN_MAP_DEBUG_PARSE_STATUS
    | MLN_MAP_DEBUG_TIMESTAMPS
    | MLN_MAP_DEBUG_COLLISION
    | MLN_MAP_DEBUG_OVERDRAW
    | MLN_MAP_DEBUG_STENCIL_CLIP
    | MLN_MAP_DEBUG_DEPTH_BUFFER;

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_debug_flags(
    map: mln_map,
    flags: mln_map_debug_flags,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if flags & !DEBUG_FLAGS != 0 {
                return Err(Failure::invalid_argument("debug flags contain unknown bits"));
            }
            command(map, completion, true, move |entry| {
                entry.map.borrow_mut().set_debug(MapDebugOptions(flags));
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_rendering_stats_view_enabled(
    map: mln_map,
    enabled: bool,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            command(map, completion, true, move |entry| {
                // TODO(port): the core has no rendering statistics view
                // (Map::enableRenderingStatsView), so the map only reports
                // the setting.
                entry.camera.rendering_stats_view.set(enabled);
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_dump_debug_logs(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            command(map, completion, true, |entry| {
                // C++ Map::dumpDebugLogs.
                let rule = "-".repeat(80);
                Log::info(Event::General, &rule);
                entry.map.borrow().get_style().impl_.dump_debug_logs();
                Log::info(Event::General, &rule);
                Ok(Value::None)
            })
        })
    }
}

/// C++ `validate_frustum_offset`.
fn validate_frustum_offset(offset: mln_edge_insets) -> Result<(), Failure> {
    let values = [offset.top, offset.left, offset.bottom, offset.right];
    if values.iter().any(|value| !value.is_finite()) {
        return Err(Failure::invalid_argument("frustum offset values must be finite"));
    }
    if values.iter().any(|value| *value < 0.0) {
        return Err(Failure::invalid_argument("frustum offset values must be greater than or equal to 0"));
    }
    Ok(())
}

/// C++ `validate_viewport_options`, for options whose size is checked.
pub(crate) fn validate_viewport_fields(options: &mln_map_viewport_options) -> Result<(), Failure> {
    let known = MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION
        | MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE
        | MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE
        | MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
    if options.fields & !known != 0 {
        return Err(Failure::invalid_argument("mln_map_viewport_options.fields contains unknown bits"));
    }
    let has = |bit: u32| options.fields & bit != 0;
    if has(MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) && options.north_orientation > MLN_NORTH_ORIENTATION_LEFT {
        return Err(Failure::invalid_argument("north_orientation is invalid"));
    }
    if has(MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) && options.constrain_mode > MLN_CONSTRAIN_MODE_SCREEN {
        return Err(Failure::invalid_argument("constrain_mode is invalid"));
    }
    if has(MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) && options.viewport_mode > MLN_VIEWPORT_MODE_FLIPPED_Y {
        return Err(Failure::invalid_argument("viewport_mode is invalid"));
    }
    if has(MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) {
        validate_frustum_offset(options.frustum_offset)?;
    }
    Ok(())
}

pub(crate) fn north_orientation_of(orientation: u32) -> NorthOrientation {
    match orientation {
        MLN_NORTH_ORIENTATION_RIGHT => NorthOrientation::Rightwards,
        MLN_NORTH_ORIENTATION_DOWN => NorthOrientation::Downwards,
        MLN_NORTH_ORIENTATION_LEFT => NorthOrientation::Leftwards,
        _ => NorthOrientation::Upwards,
    }
}

fn north_orientation_to_c(orientation: NorthOrientation) -> u32 {
    match orientation {
        NorthOrientation::Upwards => MLN_NORTH_ORIENTATION_UP,
        NorthOrientation::Rightwards => MLN_NORTH_ORIENTATION_RIGHT,
        NorthOrientation::Downwards => MLN_NORTH_ORIENTATION_DOWN,
        NorthOrientation::Leftwards => MLN_NORTH_ORIENTATION_LEFT,
    }
}

pub(crate) fn constrain_mode_of(mode: u32) -> ConstrainMode {
    match mode {
        MLN_CONSTRAIN_MODE_NONE => ConstrainMode::None,
        MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT => ConstrainMode::WidthAndHeight,
        MLN_CONSTRAIN_MODE_SCREEN => ConstrainMode::Screen,
        _ => ConstrainMode::HeightOnly,
    }
}

fn constrain_mode_to_c(mode: ConstrainMode) -> u32 {
    match mode {
        ConstrainMode::None => MLN_CONSTRAIN_MODE_NONE,
        ConstrainMode::HeightOnly => MLN_CONSTRAIN_MODE_HEIGHT_ONLY,
        ConstrainMode::WidthAndHeight => MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT,
        ConstrainMode::Screen => MLN_CONSTRAIN_MODE_SCREEN,
    }
}

pub(crate) fn viewport_mode_of(mode: u32) -> ViewportMode {
    if mode == MLN_VIEWPORT_MODE_FLIPPED_Y { ViewportMode::FlippedY } else { ViewportMode::Default }
}

fn viewport_mode_to_c(mode: ViewportMode) -> u32 {
    match mode {
        ViewportMode::Default => MLN_VIEWPORT_MODE_DEFAULT,
        ViewportMode::FlippedY => MLN_VIEWPORT_MODE_FLIPPED_Y,
    }
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_viewport_options(
    map: mln_map,
    options: *const mln_map_viewport_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if options.is_null() {
                return Err(Failure::invalid_argument("viewport options must not be null"));
            }
            let Some(options) = read_sized(options) else {
                return Err(Failure::invalid_argument("mln_map_viewport_options.size is too small"));
            };
            validate_viewport_fields(&options)?;
            let frustum_offset = if options.fields & MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET != 0 {
                let offset = options.frustum_offset;
                Some(
                    EdgeInsets::new(offset.top, offset.left, offset.bottom, offset.right)
                        .map_err(Failure::invalid_argument)?,
                )
            } else {
                None
            };
            command(map, completion, true, move |entry| {
                let has = |bit: u32| options.fields & bit != 0;
                let mut map = entry.map.borrow_mut();
                if has(MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) {
                    map.set_north_orientation(north_orientation_of(options.north_orientation));
                }
                if has(MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) {
                    map.set_constrain_mode(constrain_mode_of(options.constrain_mode));
                }
                if has(MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) {
                    map.set_viewport_mode(viewport_mode_of(options.viewport_mode));
                }
                if let Some(frustum_offset) = frustum_offset {
                    map.set_frustum_offset(&frustum_offset);
                }
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (camera.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_tile_options(
    map: mln_map,
    options: *const mln_map_tile_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if options.is_null() {
                return Err(Failure::invalid_argument("tile options must not be null"));
            }
            let Some(options) = read_sized(options) else {
                return Err(Failure::invalid_argument("mln_map_tile_options.size is too small"));
            };
            let known = MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA
                | MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS
                | MLN_MAP_TILE_OPTION_LOD_SCALE
                | MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD
                | MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT
                | MLN_MAP_TILE_OPTION_LOD_MODE;
            if options.fields & !known != 0 {
                return Err(Failure::invalid_argument("mln_map_tile_options.fields contains unknown bits"));
            }
            let has = move |bit: u32| options.fields & bit != 0;
            if has(MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) && options.prefetch_zoom_delta > u32::from(u8::MAX) {
                return Err(Failure::invalid_argument("prefetch_zoom_delta must be at most 255"));
            }
            let numeric = [
                (MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS, options.lod_min_radius),
                (MLN_MAP_TILE_OPTION_LOD_SCALE, options.lod_scale),
                (MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD, options.lod_pitch_threshold),
                (MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT, options.lod_zoom_shift),
            ];
            if numeric.iter().any(|(bit, value)| has(*bit) && !value.is_finite()) {
                return Err(Failure::invalid_argument("tile LOD values must be finite"));
            }
            if has(MLN_MAP_TILE_OPTION_LOD_MODE) && options.lod_mode > MLN_TILE_LOD_MODE_DISTANCE {
                return Err(Failure::invalid_argument("lod_mode is invalid"));
            }
            command(map, completion, true, move |entry| {
                let mut map = entry.map.borrow_mut();
                if has(MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) {
                    map.set_prefetch_zoom_delta(options.prefetch_zoom_delta as u8);
                }
                if has(MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) {
                    map.set_tile_lod_min_radius(options.lod_min_radius);
                }
                if has(MLN_MAP_TILE_OPTION_LOD_SCALE) {
                    map.set_tile_lod_scale(options.lod_scale);
                }
                // Degrees don't round-trip through radians exactly, so the
                // snapshot's value converts back to a slightly different
                // angle. Keep the engine's value when a caller sends back what
                // the snapshot reported, or the engine would redraw.
                if has(MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD)
                    && options.lod_pitch_threshold != rad2deg(map.get_tile_lod_pitch_threshold())
                {
                    map.set_tile_lod_pitch_threshold(deg2rad(options.lod_pitch_threshold));
                }
                if has(MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) {
                    map.set_tile_lod_zoom_shift(options.lod_zoom_shift);
                }
                if has(MLN_MAP_TILE_OPTION_LOD_MODE) {
                    map.set_tile_lod_mode(if options.lod_mode == MLN_TILE_LOD_MODE_DISTANCE {
                        TileLodMode::Distance
                    } else {
                        TileLodMode::Default
                    });
                }
                Ok(Value::None)
            })
        })
    }
}

// MARK: - Snapshot

/// Fills the snapshot members that the camera and option commands change
/// (C++ `publish_snapshot_map`'s config members), for map.rs
/// `publish_snapshot`.
pub fn fill_snapshot(entry: &MapEntry, map: &Map, snapshot: &mut mln_map_snapshot) {
    snapshot.debug_flags = map.get_debug().0;
    snapshot.rendering_stats_view_enabled = entry.camera.rendering_stats_view.get();
    snapshot.gesture_in_progress = map.is_gesture_in_progress();
    snapshot.transition_in_progress = entry.camera.in_transition();
    snapshot.tile = mln_map_tile_settings {
        prefetch_zoom_delta: u32::from(map.get_prefetch_zoom_delta()),
        lod_min_radius: map.get_tile_lod_min_radius(),
        lod_scale: map.get_tile_lod_scale(),
        // The engine takes the threshold in radians.
        lod_pitch_threshold: rad2deg(map.get_tile_lod_pitch_threshold()),
        lod_zoom_shift: map.get_tile_lod_zoom_shift(),
        lod_mode: match map.get_tile_lod_mode() {
            TileLodMode::Default => MLN_TILE_LOD_MODE_DEFAULT,
            TileLodMode::Distance => MLN_TILE_LOD_MODE_DISTANCE,
        },
    };
    // MapLibre reports every bound, and an unbounded center as the default
    // LatLngBounds, whose corners cover the whole world.
    let bounds = map.get_bounds();
    let area = bounds.bounds.unwrap_or_default();
    snapshot.bounds = mln_camera_bounds {
        bounds: lat_lng_bounds_to_c(&area),
        unbounded: area == LatLngBounds::default(),
        min_zoom: bounds.min_zoom.unwrap_or(MIN_ZOOM),
        max_zoom: bounds.max_zoom.unwrap_or(MAX_ZOOM),
        min_pitch: bounds.min_pitch.unwrap_or(0.0),
        max_pitch: bounds.max_pitch.unwrap_or(0.0),
    };
    let free_camera = map.get_free_camera_options();
    let [x, y, z] = free_camera.position.unwrap_or_default();
    let [qx, qy, qz, qw] = free_camera.orientation.unwrap_or([0.0, 0.0, 0.0, 1.0]);
    snapshot.free_camera =
        mln_free_camera { position: mln_vec3 { x, y, z }, orientation: mln_quaternion { x: qx, y: qy, z: qz, w: qw } };
    let mode = map.get_projection_mode();
    snapshot.view.projection_mode = mln_projection_mode {
        axonometric: mode.axonometric.unwrap_or(false),
        x_skew: mode.x_skew.unwrap_or(0.0),
        y_skew: mode.y_skew.unwrap_or(1.0),
    };
    let options = map.get_map_options();
    snapshot.view.viewport = mln_map_viewport {
        north_orientation: north_orientation_to_c(options.north_orientation()),
        constrain_mode: constrain_mode_to_c(options.constrain_mode()),
        viewport_mode: viewport_mode_to_c(options.viewport_mode()),
        frustum_offset: edge_insets_to_c(&map.get_frustum_offset()),
    };
}
