use maplibre_native_sys as sys;

use crate::{EdgeInsets, LatLng, LatLngBounds, Quaternion, ScreenPoint, UnitBezier, Vec3};

/// Camera fields used for snapshots and camera commands.
#[derive(Debug, Clone, PartialEq, Default)]
#[non_exhaustive]
pub struct CameraOptions {
    pub center: Option<LatLng>,
    pub zoom: Option<f64>,
    pub bearing: Option<f64>,
    pub pitch: Option<f64>,
    pub center_altitude: Option<f64>,
    pub padding: Option<EdgeInsets>,
    pub anchor: Option<ScreenPoint>,
    pub roll: Option<f64>,
    pub field_of_view: Option<f64>,
}

impl CameraOptions {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_center(mut self, center: LatLng) -> Self {
        self.center = Some(center);
        self
    }

    pub fn with_zoom(mut self, zoom: f64) -> Self {
        self.zoom = Some(zoom);
        self
    }

    pub fn with_bearing(mut self, bearing: f64) -> Self {
        self.bearing = Some(bearing);
        self
    }

    pub fn with_pitch(mut self, pitch: f64) -> Self {
        self.pitch = Some(pitch);
        self
    }

    pub fn with_center_altitude(mut self, center_altitude: f64) -> Self {
        self.center_altitude = Some(center_altitude);
        self
    }

    pub fn with_padding(mut self, padding: EdgeInsets) -> Self {
        self.padding = Some(padding);
        self
    }

    pub fn with_anchor(mut self, anchor: ScreenPoint) -> Self {
        self.anchor = Some(anchor);
        self
    }

    pub fn with_roll(mut self, roll: f64) -> Self {
        self.roll = Some(roll);
        self
    }

    pub fn with_field_of_view(mut self, field_of_view: f64) -> Self {
        self.field_of_view = Some(field_of_view);
        self
    }

    pub(crate) fn to_native(&self) -> sys::mln_camera_options {
        // SAFETY: Default constructor takes no arguments and initializes size
        // and default values for this C ABI version.
        let mut raw = unsafe { sys::mln_camera_options_default() };
        if let Some(center) = self.center {
            raw.fields |= sys::MLN_CAMERA_OPTION_CENTER;
            raw.latitude = center.latitude;
            raw.longitude = center.longitude;
        }
        if let Some(zoom) = self.zoom {
            raw.fields |= sys::MLN_CAMERA_OPTION_ZOOM;
            raw.zoom = zoom;
        }
        if let Some(bearing) = self.bearing {
            raw.fields |= sys::MLN_CAMERA_OPTION_BEARING;
            raw.bearing = bearing;
        }
        if let Some(pitch) = self.pitch {
            raw.fields |= sys::MLN_CAMERA_OPTION_PITCH;
            raw.pitch = pitch;
        }
        if let Some(center_altitude) = self.center_altitude {
            raw.fields |= sys::MLN_CAMERA_OPTION_CENTER_ALTITUDE;
            raw.center_altitude = center_altitude;
        }
        if let Some(padding) = self.padding {
            raw.fields |= sys::MLN_CAMERA_OPTION_PADDING;
            raw.padding = padding.to_native();
        }
        if let Some(anchor) = self.anchor {
            raw.fields |= sys::MLN_CAMERA_OPTION_ANCHOR;
            raw.anchor = anchor.to_native();
        }
        if let Some(roll) = self.roll {
            raw.fields |= sys::MLN_CAMERA_OPTION_ROLL;
            raw.roll = roll;
        }
        if let Some(field_of_view) = self.field_of_view {
            raw.fields |= sys::MLN_CAMERA_OPTION_FOV;
            raw.field_of_view = field_of_view;
        }
        raw
    }

    pub(crate) fn from_native(raw: sys::mln_camera_options) -> Self {
        Self {
            center: has(raw.fields, sys::MLN_CAMERA_OPTION_CENTER)
                .then(|| LatLng::new(raw.latitude, raw.longitude)),
            zoom: has(raw.fields, sys::MLN_CAMERA_OPTION_ZOOM).then_some(raw.zoom),
            bearing: has(raw.fields, sys::MLN_CAMERA_OPTION_BEARING).then_some(raw.bearing),
            pitch: has(raw.fields, sys::MLN_CAMERA_OPTION_PITCH).then_some(raw.pitch),
            center_altitude: has(raw.fields, sys::MLN_CAMERA_OPTION_CENTER_ALTITUDE)
                .then_some(raw.center_altitude),
            padding: has(raw.fields, sys::MLN_CAMERA_OPTION_PADDING)
                .then(|| EdgeInsets::from_native(raw.padding)),
            anchor: has(raw.fields, sys::MLN_CAMERA_OPTION_ANCHOR)
                .then(|| ScreenPoint::from_native(raw.anchor)),
            roll: has(raw.fields, sys::MLN_CAMERA_OPTION_ROLL).then_some(raw.roll),
            field_of_view: has(raw.fields, sys::MLN_CAMERA_OPTION_FOV).then_some(raw.field_of_view),
        }
    }
}

/// Optional animation controls for camera transitions.
#[derive(Debug, Clone, PartialEq, Default)]
#[non_exhaustive]
pub struct AnimationOptions {
    pub duration_ms: Option<f64>,
    pub velocity: Option<f64>,
    pub min_zoom: Option<f64>,
    pub easing: Option<UnitBezier>,
}

impl AnimationOptions {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_duration_ms(mut self, duration_ms: f64) -> Self {
        self.duration_ms = Some(duration_ms);
        self
    }

    pub fn with_velocity(mut self, velocity: f64) -> Self {
        self.velocity = Some(velocity);
        self
    }

    pub fn with_min_zoom(mut self, min_zoom: f64) -> Self {
        self.min_zoom = Some(min_zoom);
        self
    }

    pub fn with_easing(mut self, easing: UnitBezier) -> Self {
        self.easing = Some(easing);
        self
    }

    pub(crate) fn to_native(&self) -> sys::mln_animation_options {
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_animation_options_default() };
        if let Some(duration_ms) = self.duration_ms {
            raw.fields |= sys::MLN_ANIMATION_OPTION_DURATION;
            raw.duration_ms = duration_ms;
        }
        if let Some(velocity) = self.velocity {
            raw.fields |= sys::MLN_ANIMATION_OPTION_VELOCITY;
            raw.velocity = velocity;
        }
        if let Some(min_zoom) = self.min_zoom {
            raw.fields |= sys::MLN_ANIMATION_OPTION_MIN_ZOOM;
            raw.min_zoom = min_zoom;
        }
        if let Some(easing) = self.easing {
            raw.fields |= sys::MLN_ANIMATION_OPTION_EASING;
            raw.easing = easing.to_native();
        }
        raw
    }
}

/// Optional fitting controls for camera-for-viewport queries.
#[derive(Debug, Clone, PartialEq, Default)]
#[non_exhaustive]
pub struct CameraFitOptions {
    pub padding: Option<EdgeInsets>,
    pub bearing: Option<f64>,
    pub pitch: Option<f64>,
}

impl CameraFitOptions {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_padding(mut self, padding: EdgeInsets) -> Self {
        self.padding = Some(padding);
        self
    }

    pub fn with_bearing(mut self, bearing: f64) -> Self {
        self.bearing = Some(bearing);
        self
    }

    pub fn with_pitch(mut self, pitch: f64) -> Self {
        self.pitch = Some(pitch);
        self
    }

    pub(crate) fn to_native(&self) -> sys::mln_camera_fit_options {
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_camera_fit_options_default() };
        if let Some(padding) = self.padding {
            raw.fields |= sys::MLN_CAMERA_FIT_OPTION_PADDING;
            raw.padding = padding.to_native();
        }
        if let Some(bearing) = self.bearing {
            raw.fields |= sys::MLN_CAMERA_FIT_OPTION_BEARING;
            raw.bearing = bearing;
        }
        if let Some(pitch) = self.pitch {
            raw.fields |= sys::MLN_CAMERA_FIT_OPTION_PITCH;
            raw.pitch = pitch;
        }
        raw
    }
}

/// Optional map camera constraint fields.
#[derive(Debug, Clone, PartialEq, Default)]
#[non_exhaustive]
pub struct BoundOptions {
    pub bounds: Option<LatLngBounds>,
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
    pub min_pitch: Option<f64>,
    pub max_pitch: Option<f64>,
}

impl BoundOptions {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_bounds(mut self, bounds: LatLngBounds) -> Self {
        self.bounds = Some(bounds);
        self
    }

    pub fn with_min_zoom(mut self, min_zoom: f64) -> Self {
        self.min_zoom = Some(min_zoom);
        self
    }

    pub fn with_max_zoom(mut self, max_zoom: f64) -> Self {
        self.max_zoom = Some(max_zoom);
        self
    }

    pub fn with_min_pitch(mut self, min_pitch: f64) -> Self {
        self.min_pitch = Some(min_pitch);
        self
    }

    pub fn with_max_pitch(mut self, max_pitch: f64) -> Self {
        self.max_pitch = Some(max_pitch);
        self
    }

    pub(crate) fn to_native(&self) -> sys::mln_bound_options {
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_bound_options_default() };
        if let Some(bounds) = self.bounds {
            raw.fields |= sys::MLN_BOUND_OPTION_BOUNDS;
            raw.bounds = bounds.to_native();
        }
        if let Some(min_zoom) = self.min_zoom {
            raw.fields |= sys::MLN_BOUND_OPTION_MIN_ZOOM;
            raw.min_zoom = min_zoom;
        }
        if let Some(max_zoom) = self.max_zoom {
            raw.fields |= sys::MLN_BOUND_OPTION_MAX_ZOOM;
            raw.max_zoom = max_zoom;
        }
        if let Some(min_pitch) = self.min_pitch {
            raw.fields |= sys::MLN_BOUND_OPTION_MIN_PITCH;
            raw.min_pitch = min_pitch;
        }
        if let Some(max_pitch) = self.max_pitch {
            raw.fields |= sys::MLN_BOUND_OPTION_MAX_PITCH;
            raw.max_pitch = max_pitch;
        }
        raw
    }

    pub(crate) fn from_native(raw: sys::mln_bound_options) -> Self {
        Self {
            bounds: has(raw.fields, sys::MLN_BOUND_OPTION_BOUNDS)
                .then(|| LatLngBounds::from_native(raw.bounds)),
            min_zoom: has(raw.fields, sys::MLN_BOUND_OPTION_MIN_ZOOM).then_some(raw.min_zoom),
            max_zoom: has(raw.fields, sys::MLN_BOUND_OPTION_MAX_ZOOM).then_some(raw.max_zoom),
            min_pitch: has(raw.fields, sys::MLN_BOUND_OPTION_MIN_PITCH).then_some(raw.min_pitch),
            max_pitch: has(raw.fields, sys::MLN_BOUND_OPTION_MAX_PITCH).then_some(raw.max_pitch),
        }
    }
}

/// Free camera position and orientation in MapLibre Native camera space.
#[derive(Debug, Clone, PartialEq, Default)]
#[non_exhaustive]
pub struct FreeCameraOptions {
    pub position: Option<Vec3>,
    pub orientation: Option<Quaternion>,
}

impl FreeCameraOptions {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_position(mut self, position: Vec3) -> Self {
        self.position = Some(position);
        self
    }

    pub fn with_orientation(mut self, orientation: Quaternion) -> Self {
        self.orientation = Some(orientation);
        self
    }

    pub(crate) fn to_native(&self) -> sys::mln_free_camera_options {
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_free_camera_options_default() };
        if let Some(position) = self.position {
            raw.fields |= sys::MLN_FREE_CAMERA_OPTION_POSITION;
            raw.position = position.to_native();
        }
        if let Some(orientation) = self.orientation {
            raw.fields |= sys::MLN_FREE_CAMERA_OPTION_ORIENTATION;
            raw.orientation = orientation.to_native();
        }
        raw
    }

    pub(crate) fn from_native(raw: sys::mln_free_camera_options) -> Self {
        Self {
            position: has(raw.fields, sys::MLN_FREE_CAMERA_OPTION_POSITION)
                .then(|| Vec3::from_native(raw.position)),
            orientation: has(raw.fields, sys::MLN_FREE_CAMERA_OPTION_ORIENTATION)
                .then(|| Quaternion::from_native(raw.orientation)),
        }
    }
}

/// Axonometric rendering options for the live map render transform.
#[derive(Debug, Clone, PartialEq, Default)]
#[non_exhaustive]
pub struct ProjectionMode {
    pub axonometric: Option<bool>,
    pub x_skew: Option<f64>,
    pub y_skew: Option<f64>,
}

impl ProjectionMode {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_axonometric(mut self, axonometric: bool) -> Self {
        self.axonometric = Some(axonometric);
        self
    }

    pub fn with_x_skew(mut self, x_skew: f64) -> Self {
        self.x_skew = Some(x_skew);
        self
    }

    pub fn with_y_skew(mut self, y_skew: f64) -> Self {
        self.y_skew = Some(y_skew);
        self
    }

    pub(crate) fn to_native(&self) -> sys::mln_projection_mode {
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_projection_mode_default() };
        if let Some(axonometric) = self.axonometric {
            raw.fields |= sys::MLN_PROJECTION_MODE_AXONOMETRIC;
            raw.axonometric = axonometric;
        }
        if let Some(x_skew) = self.x_skew {
            raw.fields |= sys::MLN_PROJECTION_MODE_X_SKEW;
            raw.x_skew = x_skew;
        }
        if let Some(y_skew) = self.y_skew {
            raw.fields |= sys::MLN_PROJECTION_MODE_Y_SKEW;
            raw.y_skew = y_skew;
        }
        raw
    }

    pub(crate) fn from_native(raw: sys::mln_projection_mode) -> Self {
        Self {
            axonometric: has(raw.fields, sys::MLN_PROJECTION_MODE_AXONOMETRIC)
                .then_some(raw.axonometric),
            x_skew: has(raw.fields, sys::MLN_PROJECTION_MODE_X_SKEW).then_some(raw.x_skew),
            y_skew: has(raw.fields, sys::MLN_PROJECTION_MODE_Y_SKEW).then_some(raw.y_skew),
        }
    }
}

fn has(fields: u32, flag: u32) -> bool {
    fields & flag != 0
}
