// Ports maplibre-native-ffi src/c_api/projection.cpp and the projection
// functions of src/map/map.cpp (map_projection_*, validate_map_view,
// transform_from_map_view, projected_meters_for_lat_lng,
// lat_lng_for_projected_meters) onto maplibre-native-next's TransformState.
//
// A projection converts for one `mln_map_view` on the calling thread,
// without its map: it holds its own transform state, and a mutex serializes
// the calls on one projection, as C++ `with_projection` does. It converts as
// `mln::MapProjection` does with the C++ library's core patch
// (patches/maplibre-native/0045), which puts a coordinate in the world copy
// nearest the camera's unwrapped center.

use crate::camera::{
    camera_options_at, constrain_mode_of, lat_lng_of, lat_lng_slice, lat_lng_to_c, north_orientation_of,
    screen_point_slice, validate_edge_insets, validate_lat_lng, validate_screen_point, validate_viewport_fields,
    viewport_mode_of,
};
use crate::diagnostics::{Failure, boundary};
use crate::handles::{self, Object};
use crate::map::camera_of;
use crate::types::*;
use maplibre_native_next::mln_map::transform::Transform;
use maplibre_native_next::mln_map::{ConstrainMode, TransformState};
use maplibre_native_next::mln_math::{deg2rad, rad2deg};
use maplibre_native_next::mln_util::constants::{DEGREES_MAX, MAX_ZOOM, MIN_ZOOM, PITCH_MAX, PITCH_MIN};
use maplibre_native_next::mln_util::geo::{EdgeInsets, LatLng, ScreenCoordinate, WrapMode};
use maplibre_native_next::mln_util::projection::{ProjectedMeters, Projection};
use maplibre_native_next::mln_util::size::Size;
use std::sync::{Arc, Mutex, MutexGuard};

/// C++ `MapProjectionObject`.
pub struct ProjectionObject {
    state: Mutex<TransformState>,
}

impl ProjectionObject {
    fn state(&self) -> MutexGuard<'_, TransformState> {
        self.state.lock().unwrap_or_else(|poisoned| poisoned.into_inner())
    }
}

/// C++ `with_projection`: runs `work` against a live projection.
fn with_projection<R>(
    projection: mln_map_projection,
    work: impl FnOnce(&mut TransformState) -> Result<R, Failure>,
) -> Result<R, Failure> {
    let object = handles::projection(projection)?;
    let mut state = object.state();
    work(&mut state)
}

/// C++ `validate_map_view`: a view's camera has finite members, and its zoom,
/// pitch, and field of view stay within MapLibre Native's absolute limits,
/// the only range in which a transform converts coordinates.
fn validate_map_view(view: &mln_map_view) -> Result<(), Failure> {
    let camera = &view.camera;
    validate_lat_lng(camera.center)?;
    let numeric =
        [camera.center_altitude, camera.zoom, camera.bearing, camera.pitch, camera.roll, camera.field_of_view];
    if numeric.iter().any(|value| !value.is_finite()) {
        return Err(Failure::invalid_argument("view camera numeric fields must be finite"));
    }
    validate_edge_insets(camera.padding)?;
    let limits = TransformState::default();
    if camera.zoom < MIN_ZOOM
        || camera.zoom > MAX_ZOOM
        || camera.pitch < rad2deg(PITCH_MIN)
        || camera.pitch > rad2deg(PITCH_MAX)
        || camera.field_of_view < rad2deg(limits.get_min_field_of_view())
        || camera.field_of_view > rad2deg(limits.get_max_field_of_view())
    {
        return Err(Failure::invalid_argument(
            "view camera zoom, pitch, and field of view must be within MapLibre Native's limits",
        ));
    }
    let extent = view.extent;
    if extent.width == 0 || extent.height == 0 || !(extent.scale_factor.is_finite() && extent.scale_factor > 0.0) {
        return Err(Failure::invalid_argument("view extent dimensions and scale_factor must be positive"));
    }
    if !view.projection_mode.x_skew.is_finite() || !view.projection_mode.y_skew.is_finite() {
        return Err(Failure::invalid_argument("view projection skews must be finite"));
    }
    let viewport = view.viewport;
    validate_viewport_fields(&mln_map_viewport_options {
        size: size_of::<mln_map_viewport_options>() as u32,
        fields: MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION
            | MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE
            | MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE
            | MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET,
        north_orientation: viewport.north_orientation,
        constrain_mode: viewport.constrain_mode,
        viewport_mode: viewport.viewport_mode,
        frustum_offset: viewport.frustum_offset,
    })
}

fn insets(insets: mln_edge_insets) -> Result<EdgeInsets, Failure> {
    EdgeInsets::new(insets.top, insets.left, insets.bottom, insets.right).map_err(Failure::invalid_argument)
}

/// C++ `transform_from_map_view`: assigns a validated view to a fresh
/// transform state. A camera jump would wrap the center into the first world
/// copy and clamp zoom and pitch to constraints that the view does not
/// carry, so the state is set directly, with MapLibre Native's widest limits
/// and no constraint until every member is in place.
fn transform_from_map_view(view: &mln_map_view) -> Result<TransformState, Failure> {
    let camera = &view.camera;
    let viewport = &view.viewport;
    let mut state = TransformState::new(ConstrainMode::None, viewport_mode_of(viewport.viewport_mode));
    state.set_max_zoom(MAX_ZOOM);
    state.set_max_pitch(PITCH_MAX);
    state.set_size(Size::new(view.extent.width, view.extent.height));
    state.set_north_orientation(north_orientation_of(viewport.north_orientation));
    state.set_frustum_offset(&insets(viewport.frustum_offset)?);
    state.set_edge_insets(&insets(camera.padding)?);
    state.set_lat_lng_zoom(&lat_lng_of(camera.center)?, camera.zoom);
    state.set_center_altitude(camera.center_altitude);
    state.set_bearing(-deg2rad(camera.bearing));
    state.set_pitch(deg2rad(camera.pitch));
    state.set_roll(deg2rad(camera.roll));
    state.set_field_of_view(deg2rad(camera.field_of_view));
    state.set_axonometric(view.projection_mode.axonometric);
    state.set_x_skew(view.projection_mode.x_skew);
    state.set_y_skew(view.projection_mode.y_skew);
    state.set_constrain_mode(constrain_mode_of(viewport.constrain_mode));
    Ok(state)
}

/// `MapProjection::pixelForLatLng`: the screen point of a coordinate in the
/// world copy nearest the camera's center.
fn pixel_for(state: &TransformState, coordinate: mln_lat_lng) -> Result<mln_screen_point, Failure> {
    let wrapped = lat_lng_of(coordinate)?.wrapped();
    let longitude = wrapped.longitude();
    let center = state.get_lat_lng(WrapMode::Unwrapped).longitude();
    let unwrapped =
        LatLng::new(wrapped.latitude(), longitude + DEGREES_MAX * ((center - longitude) / DEGREES_MAX).round())
            .map_err(|e| Failure::new(MLN_STATUS_NATIVE_ERROR, e))?;
    let point = state.lat_lng_to_screen_coordinate(&unwrapped);
    Ok(mln_screen_point { x: point.x, y: f64::from(state.get_size().height) - point.y })
}

/// `MapProjection::latLngForPixel`.
fn lat_lng_for(state: &TransformState, point: mln_screen_point, wrap_mode: WrapMode) -> mln_lat_lng {
    let flipped = ScreenCoordinate::new(point.x, f64::from(state.get_size().height) - point.y);
    lat_lng_to_c(&state.screen_coordinate_to_lat_lng(&flipped, wrap_mode))
}

/// # Safety
/// C API contract (projection.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_projection_create(
    view: *const mln_map_view,
    out_projection: *mut mln_map_projection,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let Some(view) = read_sized(view) else {
                return Err(Failure::invalid_argument("view must not be null and must have a valid size"));
            };
            if out_projection.is_null() {
                return Err(Failure::invalid_argument("out_projection must not be null"));
            }
            if (*out_projection).0 != 0 {
                return Err(Failure::invalid_argument("out_projection must point to the null handle"));
            }
            validate_map_view(&view)?;
            let state = transform_from_map_view(&view)?;
            let projection = Arc::new(ProjectionObject { state: Mutex::new(state) });
            *out_projection = handles::insert(Object::Projection(projection)).into();
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (projection.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_projection_release(projection: mln_map_projection) {
    // A call that holds the projection keeps it until it returns.
    if handles::projection(projection).is_ok() {
        handles::remove(projection);
    }
}

/// # Safety
/// C API contract (projection.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_projection_get_camera(
    projection: mln_map_projection,
    out_result: *mut mln_map_projection_camera_result,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if out_result.is_null() || ((*out_result).size as usize) < size_of::<mln_map_projection_camera_result>() {
                return Err(Failure::invalid_argument("out_result must not be null and must have a valid size"));
            }
            let camera = with_projection(projection, |state| Ok(camera_of(&state.get_camera_options(None))))?;
            (*out_result).camera = camera;
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (projection.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_projection_set_camera(
    projection: mln_map_projection,
    camera: *const mln_camera_options,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let camera = camera_options_at(camera)?;
            with_projection(projection, |state| {
                let mut transform = Transform::from_state(state.clone());
                transform.jump_to(&camera);
                *state = transform.get_state().clone();
                Ok(())
            })
        })
    }
}

/// # Safety
/// C API contract (projection.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_projection_pixel_for_lat_lng(
    projection: mln_map_projection,
    coordinate: mln_lat_lng,
    out_point: *mut mln_screen_point,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if out_point.is_null() {
                return Err(Failure::invalid_argument("out_point must not be null"));
            }
            validate_lat_lng(coordinate)?;
            *out_point = with_projection(projection, |state| pixel_for(state, coordinate))?;
            Ok(())
        })
    }
}

/// C++ `projection_lat_lng_for_pixel`.
///
/// # Safety
/// `out_coordinate` is null or writable.
unsafe fn lat_lng_for_pixel(
    projection: mln_map_projection,
    point: mln_screen_point,
    out_coordinate: *mut mln_lat_lng,
    wrap_mode: WrapMode,
) -> Result<(), Failure> {
    if out_coordinate.is_null() {
        return Err(Failure::invalid_argument("out_coordinate must not be null"));
    }
    validate_screen_point(point)?;
    let coordinate = with_projection(projection, |state| Ok(lat_lng_for(state, point, wrap_mode)))?;
    // SAFETY: the caller's contract.
    unsafe { *out_coordinate = coordinate };
    Ok(())
}

/// # Safety
/// C API contract (projection.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_projection_lat_lng_for_pixel(
    projection: mln_map_projection,
    point: mln_screen_point,
    out_coordinate: *mut mln_lat_lng,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe { boundary(out_diagnostic, || lat_lng_for_pixel(projection, point, out_coordinate, WrapMode::Wrapped)) }
}

/// # Safety
/// C API contract (projection.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_projection_lat_lng_for_pixel_unwrapped(
    projection: mln_map_projection,
    point: mln_screen_point,
    out_coordinate: *mut mln_lat_lng,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe { boundary(out_diagnostic, || lat_lng_for_pixel(projection, point, out_coordinate, WrapMode::Unwrapped)) }
}

/// # Safety
/// C API contract (projection.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_projection_pixels_for_lat_lngs(
    projection: mln_map_projection,
    coordinates: *const mln_lat_lng,
    coordinate_count: usize,
    out_points: *mut mln_screen_point,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if coordinate_count != 0 && out_points.is_null() {
                return Err(Failure::invalid_argument("out_points must not be null when coordinate_count is nonzero"));
            }
            let coordinates = lat_lng_slice(coordinates, coordinate_count, true)?;
            // A batch converts into scratch storage and copies the results
            // out afterwards, so a failure leaves the caller's output alone.
            let points = with_projection(projection, |state| {
                coordinates.iter().map(|coordinate| pixel_for(state, *coordinate)).collect::<Result<Vec<_>, _>>()
            })?;
            if !points.is_empty() {
                std::ptr::copy_nonoverlapping(points.as_ptr(), out_points, points.len());
            }
            Ok(())
        })
    }
}

/// C++ `projection_lat_lngs_for_pixels`.
///
/// # Safety
/// `points` and `out_coordinates` are null or have room for `point_count`.
unsafe fn lat_lngs_for_pixels(
    projection: mln_map_projection,
    points: *const mln_screen_point,
    point_count: usize,
    out_coordinates: *mut mln_lat_lng,
    wrap_mode: WrapMode,
) -> Result<(), Failure> {
    if point_count != 0 && out_coordinates.is_null() {
        return Err(Failure::invalid_argument("out_coordinates must not be null when point_count is nonzero"));
    }
    // SAFETY: the caller's contract.
    let points = unsafe { screen_point_slice(points, point_count) }?;
    let coordinates = with_projection(projection, |state| {
        Ok(points.iter().map(|point| lat_lng_for(state, *point, wrap_mode)).collect::<Vec<_>>())
    })?;
    if !coordinates.is_empty() {
        // SAFETY: the caller's contract.
        unsafe { std::ptr::copy_nonoverlapping(coordinates.as_ptr(), out_coordinates, coordinates.len()) };
    }
    Ok(())
}

/// # Safety
/// C API contract (projection.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_projection_lat_lngs_for_pixels(
    projection: mln_map_projection,
    points: *const mln_screen_point,
    point_count: usize,
    out_coordinates: *mut mln_lat_lng,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            lat_lngs_for_pixels(projection, points, point_count, out_coordinates, WrapMode::Wrapped)
        })
    }
}

/// # Safety
/// C API contract (projection.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_projection_lat_lngs_for_pixels_unwrapped(
    projection: mln_map_projection,
    points: *const mln_screen_point,
    point_count: usize,
    out_coordinates: *mut mln_lat_lng,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            lat_lngs_for_pixels(projection, points, point_count, out_coordinates, WrapMode::Unwrapped)
        })
    }
}

/// # Safety
/// C API contract (projection.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_projection_meters_per_pixel_at_latitude(
    projection: mln_map_projection,
    latitude: f64,
    out_meters_per_pixel: *mut f64,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if out_meters_per_pixel.is_null() {
                return Err(Failure::invalid_argument("out_meters_per_pixel must not be null"));
            }
            if !latitude.is_finite() || !(-90.0..=90.0).contains(&latitude) {
                return Err(Failure::invalid_argument("latitude must be finite and within [-90, 90]"));
            }
            *out_meters_per_pixel = with_projection(projection, |state| {
                Ok(Projection::get_meters_per_pixel_at_latitude(latitude, state.get_zoom()))
            })?;
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (projection.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_projected_meters_for_lat_lng(
    coordinate: mln_lat_lng,
    out_meters: *mut mln_projected_meters,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if out_meters.is_null() {
                return Err(Failure::invalid_argument("out_meters must not be null"));
            }
            validate_lat_lng(coordinate)?;
            let meters = Projection::projected_meters_for_lat_lng(&lat_lng_of(coordinate)?);
            *out_meters = mln_projected_meters { northing: meters.northing(), easting: meters.easting() };
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (projection.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_lat_lng_for_projected_meters(
    meters: mln_projected_meters,
    out_coordinate: *mut mln_lat_lng,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if out_coordinate.is_null() {
                return Err(Failure::invalid_argument("out_coordinate must not be null"));
            }
            if !meters.northing.is_finite() || !meters.easting.is_finite() {
                return Err(Failure::invalid_argument("projected meter values must be finite"));
            }
            let meters = ProjectedMeters::new(meters.northing, meters.easting).map_err(Failure::invalid_argument)?;
            *out_coordinate = lat_lng_to_c(&Projection::lat_lng_for_projected_meters(&meters));
            Ok(())
        })
    }
}
