// Generated from C headers by tools/bindgen. Do not edit.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum AmbientCacheOperation {
    Clear,
    Invalidate,
    PackDatabase,
    ResetDatabase,
    Unknown(u32),
}
impl Default for AmbientCacheOperation {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl AmbientCacheOperation {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            4 => Self::Clear,
            3 => Self::Invalidate,
            2 => Self::PackDatabase,
            1 => Self::ResetDatabase,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Clear => 4,
            Self::Invalidate => 3,
            Self::PackDatabase => 2,
            Self::ResetDatabase => 1,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct AnimationOptionField: u32 {
        const DURATION = 1;
        const VELOCITY = 2;
        const MIN_ZOOM = 4;
        const EASING = 8;
        const TRANSITION_ID = 16;
        const _ = !0;
} }
impl AnimationOptionField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct AnimationOptions {
    pub duration_ms: Option<f64>,
    pub velocity: Option<f64>,
    pub min_zoom: Option<f64>,
    pub easing: Option<crate::generated::UnitBezier>,
    pub transition_id: Option<u64>,
}
impl Default for AnimationOptions {
    fn default() -> Self {
        Self::from_native(unsafe { maplibre_native_ffi_sys::mln_animation_options_default() })
    }
}
impl AnimationOptions {
    #[doc(hidden)]
    pub fn to_native(&self) -> maplibre_native_ffi_sys::mln_animation_options {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_animation_options_default() };
        raw.fields = 0;
        if let Some(value) = self.duration_ms {
            raw.fields |= maplibre_native_ffi_sys::MLN_ANIMATION_OPTION_DURATION;
            raw.duration_ms = value;
        }
        if let Some(value) = self.velocity {
            raw.fields |= maplibre_native_ffi_sys::MLN_ANIMATION_OPTION_VELOCITY;
            raw.velocity = value;
        }
        if let Some(value) = self.min_zoom {
            raw.fields |= maplibre_native_ffi_sys::MLN_ANIMATION_OPTION_MIN_ZOOM;
            raw.min_zoom = value;
        }
        if let Some(value) = self.easing {
            raw.fields |= maplibre_native_ffi_sys::MLN_ANIMATION_OPTION_EASING;
            raw.easing = value.to_native();
        }
        if let Some(value) = self.transition_id {
            raw.fields |= maplibre_native_ffi_sys::MLN_ANIMATION_OPTION_TRANSITION_ID;
            raw.transition_id = value;
        }
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_animation_options) -> Self {
        Self {
            duration_ms: (raw.fields & maplibre_native_ffi_sys::MLN_ANIMATION_OPTION_DURATION != 0)
                .then(|| raw.duration_ms),
            velocity: (raw.fields & maplibre_native_ffi_sys::MLN_ANIMATION_OPTION_VELOCITY != 0)
                .then(|| raw.velocity),
            min_zoom: (raw.fields & maplibre_native_ffi_sys::MLN_ANIMATION_OPTION_MIN_ZOOM != 0)
                .then(|| raw.min_zoom),
            easing: (raw.fields & maplibre_native_ffi_sys::MLN_ANIMATION_OPTION_EASING != 0)
                .then(|| crate::generated::UnitBezier::from_native(raw.easing)),
            transition_id: (raw.fields
                & maplibre_native_ffi_sys::MLN_ANIMATION_OPTION_TRANSITION_ID
                != 0)
                .then(|| raw.transition_id),
        }
    }
}
impl crate::values::NativeValue for AnimationOptions {
    type Raw = maplibre_native_ffi_sys::mln_animation_options;
    fn to_native(self) -> Self::Raw {
        AnimationOptions::to_native(&self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct BoundOptionField: u32 {
        const BOUNDS = 1;
        const MIN_ZOOM = 2;
        const MAX_ZOOM = 4;
        const MIN_PITCH = 8;
        const MAX_PITCH = 16;
        const UNBOUNDED = 32;
        const _ = !0;
} }
impl BoundOptionField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct BoundOptions {
    pub unbounded: bool,
    pub bounds: Option<crate::generated::LatLngBounds>,
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
    pub min_pitch: Option<f64>,
    pub max_pitch: Option<f64>,
}
impl Default for BoundOptions {
    fn default() -> Self {
        Self::from_native(unsafe { maplibre_native_ffi_sys::mln_bound_options_default() })
    }
}
impl BoundOptions {
    #[doc(hidden)]
    pub fn to_native(&self) -> maplibre_native_ffi_sys::mln_bound_options {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_bound_options_default() };
        raw.fields = 0;
        if self.unbounded {
            raw.fields |= maplibre_native_ffi_sys::MLN_BOUND_OPTION_UNBOUNDED;
        }
        if let Some(value) = self.bounds {
            raw.fields |= maplibre_native_ffi_sys::MLN_BOUND_OPTION_BOUNDS;
            raw.bounds = value.to_native();
        }
        if let Some(value) = self.min_zoom {
            raw.fields |= maplibre_native_ffi_sys::MLN_BOUND_OPTION_MIN_ZOOM;
            raw.min_zoom = value;
        }
        if let Some(value) = self.max_zoom {
            raw.fields |= maplibre_native_ffi_sys::MLN_BOUND_OPTION_MAX_ZOOM;
            raw.max_zoom = value;
        }
        if let Some(value) = self.min_pitch {
            raw.fields |= maplibre_native_ffi_sys::MLN_BOUND_OPTION_MIN_PITCH;
            raw.min_pitch = value;
        }
        if let Some(value) = self.max_pitch {
            raw.fields |= maplibre_native_ffi_sys::MLN_BOUND_OPTION_MAX_PITCH;
            raw.max_pitch = value;
        }
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_bound_options) -> Self {
        Self {
            unbounded: raw.fields & maplibre_native_ffi_sys::MLN_BOUND_OPTION_UNBOUNDED != 0,
            bounds: (raw.fields & maplibre_native_ffi_sys::MLN_BOUND_OPTION_BOUNDS != 0)
                .then(|| crate::generated::LatLngBounds::from_native(raw.bounds)),
            min_zoom: (raw.fields & maplibre_native_ffi_sys::MLN_BOUND_OPTION_MIN_ZOOM != 0)
                .then(|| raw.min_zoom),
            max_zoom: (raw.fields & maplibre_native_ffi_sys::MLN_BOUND_OPTION_MAX_ZOOM != 0)
                .then(|| raw.max_zoom),
            min_pitch: (raw.fields & maplibre_native_ffi_sys::MLN_BOUND_OPTION_MIN_PITCH != 0)
                .then(|| raw.min_pitch),
            max_pitch: (raw.fields & maplibre_native_ffi_sys::MLN_BOUND_OPTION_MAX_PITCH != 0)
                .then(|| raw.max_pitch),
        }
    }
}
impl crate::values::NativeValue for BoundOptions {
    type Raw = maplibre_native_ffi_sys::mln_bound_options;
    fn to_native(self) -> Self::Raw {
        BoundOptions::to_native(&self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum CameraChangeMode {
    Animated,
    Immediate,
    Unknown(u32),
}
impl Default for CameraChangeMode {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl CameraChangeMode {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::Animated,
            0 => Self::Immediate,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Animated => 1,
            Self::Immediate => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct CameraDelta {
    pub kind: crate::generated::CameraDeltaKind,
    pub offset: crate::generated::ScreenPoint,
    pub amount: f64,
    pub anchor: Option<crate::generated::ScreenPoint>,
    pub animation: crate::generated::AnimationOptions,
}
impl Default for CameraDelta {
    fn default() -> Self {
        Self::from_native(unsafe { maplibre_native_ffi_sys::mln_camera_delta_default() })
    }
}
impl CameraDelta {
    #[doc(hidden)]
    pub fn to_native(&self) -> maplibre_native_ffi_sys::mln_camera_delta {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_camera_delta_default() };
        raw.has_anchor = false;
        raw.kind = self.kind.to_native();
        raw.offset = self.offset.to_native();
        raw.amount = self.amount;
        if let Some(value) = self.anchor {
            raw.has_anchor = true;
            raw.anchor = value.to_native();
        }
        raw.animation = self.animation.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_camera_delta) -> Self {
        Self {
            kind: crate::generated::CameraDeltaKind::from_native(raw.kind),
            offset: crate::generated::ScreenPoint::from_native(raw.offset),
            amount: raw.amount,
            anchor: (raw.has_anchor)
                .then(|| crate::generated::ScreenPoint::from_native(raw.anchor)),
            animation: crate::generated::AnimationOptions::from_native(raw.animation),
        }
    }
}
impl crate::values::NativeValue for CameraDelta {
    type Raw = maplibre_native_ffi_sys::mln_camera_delta;
    fn to_native(self) -> Self::Raw {
        CameraDelta::to_native(&self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum CameraDeltaKind {
    Pitch,
    Bearing,
    Scale,
    Move,
    Unknown(u32),
}
impl Default for CameraDeltaKind {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl CameraDeltaKind {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            3 => Self::Pitch,
            2 => Self::Bearing,
            1 => Self::Scale,
            0 => Self::Move,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Pitch => 3,
            Self::Bearing => 2,
            Self::Scale => 1,
            Self::Move => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct CameraFitOptionField: u32 {
        const PADDING = 1;
        const BEARING = 2;
        const PITCH = 4;
        const _ = !0;
} }
impl CameraFitOptionField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct CameraFitOptions {
    pub padding: Option<crate::generated::EdgeInsets>,
    pub bearing: Option<f64>,
    pub pitch: Option<f64>,
}
impl Default for CameraFitOptions {
    fn default() -> Self {
        Self::from_native(unsafe { maplibre_native_ffi_sys::mln_camera_fit_options_default() })
    }
}
impl CameraFitOptions {
    #[doc(hidden)]
    pub fn to_native(&self) -> maplibre_native_ffi_sys::mln_camera_fit_options {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_camera_fit_options_default() };
        raw.fields = 0;
        if let Some(value) = self.padding {
            raw.fields |= maplibre_native_ffi_sys::MLN_CAMERA_FIT_OPTION_PADDING;
            raw.padding = value.to_native();
        }
        if let Some(value) = self.bearing {
            raw.fields |= maplibre_native_ffi_sys::MLN_CAMERA_FIT_OPTION_BEARING;
            raw.bearing = value;
        }
        if let Some(value) = self.pitch {
            raw.fields |= maplibre_native_ffi_sys::MLN_CAMERA_FIT_OPTION_PITCH;
            raw.pitch = value;
        }
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_camera_fit_options) -> Self {
        Self {
            padding: (raw.fields & maplibre_native_ffi_sys::MLN_CAMERA_FIT_OPTION_PADDING != 0)
                .then(|| crate::generated::EdgeInsets::from_native(raw.padding)),
            bearing: (raw.fields & maplibre_native_ffi_sys::MLN_CAMERA_FIT_OPTION_BEARING != 0)
                .then(|| raw.bearing),
            pitch: (raw.fields & maplibre_native_ffi_sys::MLN_CAMERA_FIT_OPTION_PITCH != 0)
                .then(|| raw.pitch),
        }
    }
}
impl crate::values::NativeValue for CameraFitOptions {
    type Raw = maplibre_native_ffi_sys::mln_camera_fit_options;
    fn to_native(self) -> Self::Raw {
        CameraFitOptions::to_native(&self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct CameraOptionField: u32 {
        const CENTER = 1;
        const ZOOM = 2;
        const BEARING = 4;
        const PITCH = 8;
        const CENTER_ALTITUDE = 16;
        const PADDING = 32;
        const ANCHOR = 64;
        const ROLL = 128;
        const FOV = 256;
        const _ = !0;
} }
impl CameraOptionField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct CameraOptions {
    pub center: Option<crate::generated::LatLng>,
    pub center_altitude: Option<f64>,
    pub padding: Option<crate::generated::EdgeInsets>,
    pub anchor: Option<crate::generated::ScreenPoint>,
    pub zoom: Option<f64>,
    pub bearing: Option<f64>,
    pub pitch: Option<f64>,
    pub roll: Option<f64>,
    pub field_of_view: Option<f64>,
}
impl Default for CameraOptions {
    fn default() -> Self {
        Self::from_native(unsafe { maplibre_native_ffi_sys::mln_camera_options_default() })
    }
}
impl CameraOptions {
    #[doc(hidden)]
    pub fn to_native(&self) -> maplibre_native_ffi_sys::mln_camera_options {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_camera_options_default() };
        raw.fields = 0;
        if let Some(value) = self.center {
            raw.fields |= maplibre_native_ffi_sys::MLN_CAMERA_OPTION_CENTER;
            raw.latitude = value.latitude;
            raw.longitude = value.longitude;
        }
        if let Some(value) = self.center_altitude {
            raw.fields |= maplibre_native_ffi_sys::MLN_CAMERA_OPTION_CENTER_ALTITUDE;
            raw.center_altitude = value;
        }
        if let Some(value) = self.padding {
            raw.fields |= maplibre_native_ffi_sys::MLN_CAMERA_OPTION_PADDING;
            raw.padding = value.to_native();
        }
        if let Some(value) = self.anchor {
            raw.fields |= maplibre_native_ffi_sys::MLN_CAMERA_OPTION_ANCHOR;
            raw.anchor = value.to_native();
        }
        if let Some(value) = self.zoom {
            raw.fields |= maplibre_native_ffi_sys::MLN_CAMERA_OPTION_ZOOM;
            raw.zoom = value;
        }
        if let Some(value) = self.bearing {
            raw.fields |= maplibre_native_ffi_sys::MLN_CAMERA_OPTION_BEARING;
            raw.bearing = value;
        }
        if let Some(value) = self.pitch {
            raw.fields |= maplibre_native_ffi_sys::MLN_CAMERA_OPTION_PITCH;
            raw.pitch = value;
        }
        if let Some(value) = self.roll {
            raw.fields |= maplibre_native_ffi_sys::MLN_CAMERA_OPTION_ROLL;
            raw.roll = value;
        }
        if let Some(value) = self.field_of_view {
            raw.fields |= maplibre_native_ffi_sys::MLN_CAMERA_OPTION_FOV;
            raw.field_of_view = value;
        }
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_camera_options) -> Self {
        Self {
            center: (raw.fields & maplibre_native_ffi_sys::MLN_CAMERA_OPTION_CENTER != 0).then(
                || crate::generated::LatLng {
                    latitude: raw.latitude,
                    longitude: raw.longitude,
                },
            ),
            center_altitude: (raw.fields
                & maplibre_native_ffi_sys::MLN_CAMERA_OPTION_CENTER_ALTITUDE
                != 0)
                .then(|| raw.center_altitude),
            padding: (raw.fields & maplibre_native_ffi_sys::MLN_CAMERA_OPTION_PADDING != 0)
                .then(|| crate::generated::EdgeInsets::from_native(raw.padding)),
            anchor: (raw.fields & maplibre_native_ffi_sys::MLN_CAMERA_OPTION_ANCHOR != 0)
                .then(|| crate::generated::ScreenPoint::from_native(raw.anchor)),
            zoom: (raw.fields & maplibre_native_ffi_sys::MLN_CAMERA_OPTION_ZOOM != 0)
                .then(|| raw.zoom),
            bearing: (raw.fields & maplibre_native_ffi_sys::MLN_CAMERA_OPTION_BEARING != 0)
                .then(|| raw.bearing),
            pitch: (raw.fields & maplibre_native_ffi_sys::MLN_CAMERA_OPTION_PITCH != 0)
                .then(|| raw.pitch),
            roll: (raw.fields & maplibre_native_ffi_sys::MLN_CAMERA_OPTION_ROLL != 0)
                .then(|| raw.roll),
            field_of_view: (raw.fields & maplibre_native_ffi_sys::MLN_CAMERA_OPTION_FOV != 0)
                .then(|| raw.field_of_view),
        }
    }
}
impl crate::values::NativeValue for CameraOptions {
    type Raw = maplibre_native_ffi_sys::mln_camera_options;
    fn to_native(self) -> Self::Raw {
        CameraOptions::to_native(&self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct CameraQueryResult {
    pub generation: u64,
    pub camera: crate::generated::CameraOptions,
}

impl CameraQueryResult {
    pub const fn new(generation: u64, camera: crate::generated::CameraOptions) -> Self {
        Self { generation, camera }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_camera_query_result {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_camera_query_result>() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_camera_query_result>() as _;
        raw.generation = self.generation;
        raw.camera = self.camera.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_camera_query_result) -> Self {
        Self {
            generation: raw.generation,
            camera: crate::generated::CameraOptions::from_native(raw.camera),
        }
    }
}
impl crate::values::NativeValue for CameraQueryResult {
    type Raw = maplibre_native_ffi_sys::mln_camera_query_result;
    fn to_native(self) -> Self::Raw {
        CameraQueryResult::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct CameraUpdate {
    pub mode: crate::generated::CameraUpdateMode,
    pub camera: crate::generated::CameraOptions,
    pub animation: crate::generated::AnimationOptions,
    pub gesture_phase: crate::generated::GesturePhase,
}
impl Default for CameraUpdate {
    fn default() -> Self {
        Self::from_native(unsafe { maplibre_native_ffi_sys::mln_camera_update_default() })
    }
}
impl CameraUpdate {
    pub const fn new(
        mode: crate::generated::CameraUpdateMode,
        camera: crate::generated::CameraOptions,
        animation: crate::generated::AnimationOptions,
        gesture_phase: crate::generated::GesturePhase,
    ) -> Self {
        Self {
            mode,
            camera,
            animation,
            gesture_phase,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_camera_update {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_camera_update_default() };
        raw.mode = self.mode.to_native();
        raw.camera = self.camera.to_native();
        raw.animation = self.animation.to_native();
        raw.gesture_phase = self.gesture_phase.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_camera_update) -> Self {
        Self {
            mode: crate::generated::CameraUpdateMode::from_native(raw.mode),
            camera: crate::generated::CameraOptions::from_native(raw.camera),
            animation: crate::generated::AnimationOptions::from_native(raw.animation),
            gesture_phase: crate::generated::GesturePhase::from_native(raw.gesture_phase),
        }
    }
}
impl crate::values::NativeValue for CameraUpdate {
    type Raw = maplibre_native_ffi_sys::mln_camera_update;
    fn to_native(self) -> Self::Raw {
        CameraUpdate::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum CameraUpdateMode {
    Fly,
    Ease,
    Jump,
    Unknown(u32),
}
impl Default for CameraUpdateMode {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl CameraUpdateMode {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            2 => Self::Fly,
            1 => Self::Ease,
            0 => Self::Jump,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Fly => 2,
            Self::Ease => 1,
            Self::Jump => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct CanonicalTileId {
    pub z: u32,
    pub x: u32,
    pub y: u32,
}

impl CanonicalTileId {
    pub const fn new(z: u32, x: u32, y: u32) -> Self {
        Self { z, x, y }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_canonical_tile_id {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_canonical_tile_id>() };
        raw.z = self.z;
        raw.x = self.x;
        raw.y = self.y;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_canonical_tile_id) -> Self {
        Self {
            z: raw.z,
            x: raw.x,
            y: raw.y,
        }
    }
}
impl crate::values::NativeValue for CanonicalTileId {
    type Raw = maplibre_native_ffi_sys::mln_canonical_tile_id;
    fn to_native(self) -> Self::Raw {
        CanonicalTileId::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum CommandDisposition {
    Cancelled,
    Failed,
    Superseded,
    Committed,
    Unknown(u32),
}
impl Default for CommandDisposition {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl CommandDisposition {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            3 => Self::Cancelled,
            2 => Self::Failed,
            1 => Self::Superseded,
            0 => Self::Committed,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Cancelled => 3,
            Self::Failed => 2,
            Self::Superseded => 1,
            Self::Committed => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum ConstrainMode {
    Screen,
    WidthAndHeight,
    HeightOnly,
    None,
    Unknown(u32),
}
impl Default for ConstrainMode {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl ConstrainMode {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            3 => Self::Screen,
            2 => Self::WidthAndHeight,
            1 => Self::HeightOnly,
            0 => Self::None,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Screen => 3,
            Self::WidthAndHeight => 2,
            Self::HeightOnly => 1,
            Self::None => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct CustomGeometrySourceOptionField: u32 {
        const MIN_ZOOM = 1;
        const MAX_ZOOM = 2;
        const TOLERANCE = 4;
        const TILE_SIZE = 8;
        const BUFFER = 16;
        const CLIP = 32;
        const WRAP = 64;
        const _ = !0;
} }
impl CustomGeometrySourceOptionField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Clone)]
pub struct CustomGeometrySourceOptions {
    pub fetch_tile: Option<
        std::sync::Arc<dyn Fn(crate::generated::CanonicalTileId) -> () + Send + Sync + 'static>,
    >,
    pub cancel_tile: Option<
        std::sync::Arc<dyn Fn(crate::generated::CanonicalTileId) -> () + Send + Sync + 'static>,
    >,
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
    pub tolerance: Option<f64>,
    pub tile_size: Option<u32>,
    pub buffer: Option<u32>,
    pub clip: Option<bool>,
    pub wrap: Option<bool>,
}
impl Default for CustomGeometrySourceOptions {
    fn default() -> Self {
        unsafe {
            Self::from_native(maplibre_native_ffi_sys::mln_custom_geometry_source_options_default())
                .expect("native default has no callbacks")
        }
    }
}
impl std::fmt::Debug for CustomGeometrySourceOptions {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("CustomGeometrySourceOptions")
            .finish_non_exhaustive()
    }
}
impl CustomGeometrySourceOptions {
    pub fn with_fetch_tile<F>(mut self, callback: F) -> Self
    where
        F: Fn(crate::generated::CanonicalTileId) -> () + Send + Sync + 'static,
    {
        self.fetch_tile = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(crate::generated::CanonicalTileId) -> () + Send + Sync + 'static,
    {
        Self::default().with_fetch_tile(callback)
    }
    pub fn with_cancel_tile<F>(mut self, callback: F) -> Self
    where
        F: Fn(crate::generated::CanonicalTileId) -> () + Send + Sync + 'static,
    {
        self.cancel_tile = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_custom_geometry_source_options> {
        let mut raw: maplibre_native_ffi_sys::mln_custom_geometry_source_options =
            unsafe { maplibre_native_ffi_sys::mln_custom_geometry_source_options_default() };
        raw.fields = 0;
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_custom_geometry_source_options>() as _;
        raw.fetch_tile = self
            .fetch_tile
            .as_ref()
            .map(|_| Self::fetch_tile_trampoline as _);
        raw.cancel_tile = self
            .cancel_tile
            .as_ref()
            .map(|_| Self::cancel_tile_trampoline as _);
        if let Some(item) = &self.min_zoom {
            raw.fields |= maplibre_native_ffi_sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
            raw.min_zoom = *(item);
        }
        if let Some(item) = &self.max_zoom {
            raw.fields |= maplibre_native_ffi_sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
            raw.max_zoom = *(item);
        }
        if let Some(item) = &self.tolerance {
            raw.fields |= maplibre_native_ffi_sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE;
            raw.tolerance = *(item);
        }
        if let Some(item) = &self.tile_size {
            raw.fields |= maplibre_native_ffi_sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE;
            raw.tile_size = *(item);
        }
        if let Some(item) = &self.buffer {
            raw.fields |= maplibre_native_ffi_sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER;
            raw.buffer = *(item);
        }
        if let Some(item) = &self.clip {
            raw.fields |= maplibre_native_ffi_sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP;
            raw.clip = *(item);
        }
        if let Some(item) = &self.wrap {
            raw.fields |= maplibre_native_ffi_sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
            raw.wrap = *(item);
        }
        if !(raw.fetch_tile.is_none() && raw.cancel_tile.is_none()) {
            raw.user_data = unsafe { arena.registration(self.clone(), Self::release_callback) };
            raw.release_user_data = Some(Self::release_callback);
        }
        Ok(raw)
    }
    /// Captures an empty callback descriptor returned by its native constructor.
    ///
    /// # Safety
    /// Borrowed fields must remain valid for this conversion.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_custom_geometry_source_options,
    ) -> crate::Result<Self> {
        if !(raw.fetch_tile.is_none() && raw.cancel_tile.is_none()) {
            return Err(crate::Error::invalid_argument(
                "foreign callbacks cannot be adopted",
            ));
        }
        Ok(Self {
            fetch_tile: None,
            cancel_tile: None,
            min_zoom: if raw.fields
                & maplibre_native_ffi_sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM
                != 0
            {
                Some(raw.min_zoom)
            } else {
                None
            },
            max_zoom: if raw.fields
                & maplibre_native_ffi_sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM
                != 0
            {
                Some(raw.max_zoom)
            } else {
                None
            },
            tolerance: if raw.fields
                & maplibre_native_ffi_sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE
                != 0
            {
                Some(raw.tolerance)
            } else {
                None
            },
            tile_size: if raw.fields
                & maplibre_native_ffi_sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE
                != 0
            {
                Some(raw.tile_size)
            } else {
                None
            },
            buffer: if raw.fields
                & maplibre_native_ffi_sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER
                != 0
            {
                Some(raw.buffer)
            } else {
                None
            },
            clip: if raw.fields & maplibre_native_ffi_sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP
                != 0
            {
                Some(raw.clip)
            } else {
                None
            },
            wrap: if raw.fields & maplibre_native_ffi_sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP
                != 0
            {
                Some(raw.wrap)
            } else {
                None
            },
        })
    }
    unsafe extern "C" fn release_callback(user_data: *mut std::ffi::c_void) {
        let _policy = crate::callback::PolicyScope::enter(&[], 0);
        let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| unsafe {
            drop(Box::from_raw(user_data.cast::<Self>()))
        }));
    }
    unsafe extern "C" fn fetch_tile_trampoline(
        binding_arg_0: *mut std::ffi::c_void,
        binding_arg_1: maplibre_native_ffi_sys::mln_canonical_tile_id,
    ) -> () {
        let result =
            std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| -> crate::Result<()> {
                let state = unsafe { &*binding_arg_0.cast::<Self>() };
                let callback = state
                    .fetch_tile
                    .as_ref()
                    .ok_or_else(|| crate::Error::invalid_argument("missing callback"))?;
                callback(crate::generated::CanonicalTileId::from_native(
                    binding_arg_1,
                ));
                Ok(())
            }));
        match result {
            Ok(Ok(value)) => value,
            _ => Default::default(),
        }
    }
    unsafe extern "C" fn cancel_tile_trampoline(
        binding_arg_0: *mut std::ffi::c_void,
        binding_arg_1: maplibre_native_ffi_sys::mln_canonical_tile_id,
    ) -> () {
        let result =
            std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| -> crate::Result<()> {
                let state = unsafe { &*binding_arg_0.cast::<Self>() };
                let callback = state
                    .cancel_tile
                    .as_ref()
                    .ok_or_else(|| crate::Error::invalid_argument("missing callback"))?;
                callback(crate::generated::CanonicalTileId::from_native(
                    binding_arg_1,
                ));
                Ok(())
            }));
        match result {
            Ok(Ok(value)) => value,
            _ => Default::default(),
        }
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct CustomMvtVectorSourceOptionField: u32 {
        const MIN_ZOOM = 1;
        const MAX_ZOOM = 2;
        const _ = !0;
} }
impl CustomMvtVectorSourceOptionField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Clone)]
pub struct CustomMvtVectorSourceOptions {
    pub fetch_tile: Option<
        std::sync::Arc<dyn Fn(crate::generated::CanonicalTileId) -> () + Send + Sync + 'static>,
    >,
    pub cancel_tile: Option<
        std::sync::Arc<dyn Fn(crate::generated::CanonicalTileId) -> () + Send + Sync + 'static>,
    >,
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
}
impl Default for CustomMvtVectorSourceOptions {
    fn default() -> Self {
        unsafe {
            Self::from_native(
                maplibre_native_ffi_sys::mln_custom_mvt_vector_source_options_default(),
            )
            .expect("native default has no callbacks")
        }
    }
}
impl std::fmt::Debug for CustomMvtVectorSourceOptions {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("CustomMvtVectorSourceOptions")
            .finish_non_exhaustive()
    }
}
impl CustomMvtVectorSourceOptions {
    pub fn with_fetch_tile<F>(mut self, callback: F) -> Self
    where
        F: Fn(crate::generated::CanonicalTileId) -> () + Send + Sync + 'static,
    {
        self.fetch_tile = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(crate::generated::CanonicalTileId) -> () + Send + Sync + 'static,
    {
        Self::default().with_fetch_tile(callback)
    }
    pub fn with_cancel_tile<F>(mut self, callback: F) -> Self
    where
        F: Fn(crate::generated::CanonicalTileId) -> () + Send + Sync + 'static,
    {
        self.cancel_tile = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_custom_mvt_vector_source_options> {
        let mut raw: maplibre_native_ffi_sys::mln_custom_mvt_vector_source_options =
            unsafe { maplibre_native_ffi_sys::mln_custom_mvt_vector_source_options_default() };
        raw.fields = 0;
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_custom_mvt_vector_source_options>()
                as _;
        raw.fetch_tile = self
            .fetch_tile
            .as_ref()
            .map(|_| Self::fetch_tile_trampoline as _);
        raw.cancel_tile = self
            .cancel_tile
            .as_ref()
            .map(|_| Self::cancel_tile_trampoline as _);
        if let Some(item) = &self.min_zoom {
            raw.fields |= maplibre_native_ffi_sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM;
            raw.min_zoom = *(item);
        }
        if let Some(item) = &self.max_zoom {
            raw.fields |= maplibre_native_ffi_sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
            raw.max_zoom = *(item);
        }
        if !(raw.fetch_tile.is_none() && raw.cancel_tile.is_none()) {
            raw.user_data = unsafe { arena.registration(self.clone(), Self::release_callback) };
            raw.release_user_data = Some(Self::release_callback);
        }
        Ok(raw)
    }
    /// Captures an empty callback descriptor returned by its native constructor.
    ///
    /// # Safety
    /// Borrowed fields must remain valid for this conversion.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_custom_mvt_vector_source_options,
    ) -> crate::Result<Self> {
        if !(raw.fetch_tile.is_none() && raw.cancel_tile.is_none()) {
            return Err(crate::Error::invalid_argument(
                "foreign callbacks cannot be adopted",
            ));
        }
        Ok(Self {
            fetch_tile: None,
            cancel_tile: None,
            min_zoom: if raw.fields
                & maplibre_native_ffi_sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM
                != 0
            {
                Some(raw.min_zoom)
            } else {
                None
            },
            max_zoom: if raw.fields
                & maplibre_native_ffi_sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM
                != 0
            {
                Some(raw.max_zoom)
            } else {
                None
            },
        })
    }
    unsafe extern "C" fn release_callback(user_data: *mut std::ffi::c_void) {
        let _policy = crate::callback::PolicyScope::enter(&[], 0);
        let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| unsafe {
            drop(Box::from_raw(user_data.cast::<Self>()))
        }));
    }
    unsafe extern "C" fn fetch_tile_trampoline(
        binding_arg_0: *mut std::ffi::c_void,
        binding_arg_1: maplibre_native_ffi_sys::mln_canonical_tile_id,
    ) -> () {
        let result =
            std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| -> crate::Result<()> {
                let state = unsafe { &*binding_arg_0.cast::<Self>() };
                let callback = state
                    .fetch_tile
                    .as_ref()
                    .ok_or_else(|| crate::Error::invalid_argument("missing callback"))?;
                callback(crate::generated::CanonicalTileId::from_native(
                    binding_arg_1,
                ));
                Ok(())
            }));
        match result {
            Ok(Ok(value)) => value,
            _ => Default::default(),
        }
    }
    unsafe extern "C" fn cancel_tile_trampoline(
        binding_arg_0: *mut std::ffi::c_void,
        binding_arg_1: maplibre_native_ffi_sys::mln_canonical_tile_id,
    ) -> () {
        let result =
            std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| -> crate::Result<()> {
                let state = unsafe { &*binding_arg_0.cast::<Self>() };
                let callback = state
                    .cancel_tile
                    .as_ref()
                    .ok_or_else(|| crate::Error::invalid_argument("missing callback"))?;
                callback(crate::generated::CanonicalTileId::from_native(
                    binding_arg_1,
                ));
                Ok(())
            }));
        match result {
            Ok(Ok(value)) => value,
            _ => Default::default(),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct EdgeInsets {
    pub top: f64,
    pub left: f64,
    pub bottom: f64,
    pub right: f64,
}

impl EdgeInsets {
    pub const fn new(top: f64, left: f64, bottom: f64, right: f64) -> Self {
        Self {
            top,
            left,
            bottom,
            right,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_edge_insets {
        let mut raw = unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_edge_insets>() };
        raw.top = self.top;
        raw.left = self.left;
        raw.bottom = self.bottom;
        raw.right = self.right;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_edge_insets) -> Self {
        Self {
            top: raw.top,
            left: raw.left,
            bottom: raw.bottom,
            right: raw.right,
        }
    }
}
impl crate::values::NativeValue for EdgeInsets {
    type Raw = maplibre_native_ffi_sys::mln_edge_insets;
    fn to_native(self) -> Self::Raw {
        EdgeInsets::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct EglContextDescriptor {
    pub display: *mut std::ffi::c_void,
    pub config: *mut std::ffi::c_void,
    pub share_context: *mut std::ffi::c_void,
    pub client_api: crate::generated::OpenglClientApi,
    pub get_proc_address: *mut std::ffi::c_void,
}

impl EglContextDescriptor {
    pub const fn new(
        display: *mut std::ffi::c_void,
        config: *mut std::ffi::c_void,
        share_context: *mut std::ffi::c_void,
        client_api: crate::generated::OpenglClientApi,
        get_proc_address: *mut std::ffi::c_void,
    ) -> Self {
        Self {
            display,
            config,
            share_context,
            client_api,
            get_proc_address,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_egl_context_descriptor {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_egl_context_descriptor>() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_egl_context_descriptor>() as _;
        raw.display = self.display;
        raw.config = self.config;
        raw.share_context = self.share_context;
        raw.client_api = self.client_api.to_native();
        raw.get_proc_address = self.get_proc_address;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_egl_context_descriptor) -> Self {
        Self {
            display: raw.display,
            config: raw.config,
            share_context: raw.share_context,
            client_api: crate::generated::OpenglClientApi::from_native(raw.client_api),
            get_proc_address: raw.get_proc_address,
        }
    }
}
impl crate::values::NativeValue for EglContextDescriptor {
    type Raw = maplibre_native_ffi_sys::mln_egl_context_descriptor;
    fn to_native(self) -> Self::Raw {
        EglContextDescriptor::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct FeatureStateSelector {
    pub source_id: String,
    pub source_layer_id: Option<String>,
    pub feature_id: Option<String>,
    pub state_key: Option<String>,
}

impl FeatureStateSelector {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_feature_state_selector> {
        let mut raw: maplibre_native_ffi_sys::mln_feature_state_selector =
            unsafe { std::mem::zeroed() };
        raw.fields = 0;
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_feature_state_selector>() as _;
        raw.source_id = maplibre_native_ffi_sys::mln_buffer_view {
            data: (&self.source_id).as_bytes().as_ptr().cast(),
            size: (&self.source_id).as_bytes().len(),
        };
        if let Some(item) = &self.source_layer_id {
            raw.fields |= maplibre_native_ffi_sys::MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID;
            raw.source_layer_id = maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            };
        }
        if let Some(item) = &self.feature_id {
            raw.fields |= maplibre_native_ffi_sys::MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
            raw.feature_id = maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            };
        }
        if let Some(item) = &self.state_key {
            raw.fields |= maplibre_native_ffi_sys::MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
            raw.state_key = maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            };
        }
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_feature_state_selector,
    ) -> crate::Result<Self> {
        Ok(Self {
            source_id: unsafe { crate::string::copy_string_view(raw.source_id) }?,
            source_layer_id: if raw.fields
                & maplibre_native_ffi_sys::MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
                != 0
            {
                Some(unsafe { crate::string::copy_string_view(raw.source_layer_id) }?)
            } else {
                None
            },
            feature_id: if raw.fields
                & maplibre_native_ffi_sys::MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
                != 0
            {
                Some(unsafe { crate::string::copy_string_view(raw.feature_id) }?)
            } else {
                None
            },
            state_key: if raw.fields & maplibre_native_ffi_sys::MLN_FEATURE_STATE_SELECTOR_STATE_KEY
                != 0
            {
                Some(unsafe { crate::string::copy_string_view(raw.state_key) }?)
            } else {
                None
            },
        })
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct FeatureStateSelectorField: u32 {
        const SOURCE_LAYER_ID = 1;
        const FEATURE_ID = 2;
        const STATE_KEY = 4;
        const _ = !0;
} }
impl FeatureStateSelectorField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct FrameDemand {
    pub flags: crate::generated::FrameDemandFlag,
    pub token: u64,
    pub coalescing_boundary: u64,
    pub timeout_ns: u64,
}
impl Default for FrameDemand {
    fn default() -> Self {
        Self::from_native(unsafe { maplibre_native_ffi_sys::mln_frame_demand_default() })
    }
}
impl FrameDemand {
    pub const fn new(
        flags: crate::generated::FrameDemandFlag,
        token: u64,
        coalescing_boundary: u64,
        timeout_ns: u64,
    ) -> Self {
        Self {
            flags,
            token,
            coalescing_boundary,
            timeout_ns,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_frame_demand {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_frame_demand_default() };
        raw.flags = self.flags.to_native();
        raw.token = self.token;
        raw.coalescing_boundary = self.coalescing_boundary;
        raw.timeout_ns = self.timeout_ns;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_frame_demand) -> Self {
        Self {
            flags: crate::generated::FrameDemandFlag::from_native(raw.flags),
            token: raw.token,
            coalescing_boundary: raw.coalescing_boundary,
            timeout_ns: raw.timeout_ns,
        }
    }
}
impl crate::values::NativeValue for FrameDemand {
    type Raw = maplibre_native_ffi_sys::mln_frame_demand;
    fn to_native(self) -> Self::Raw {
        FrameDemand::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct FrameDemandFlag: u32 {
        const IF_NEEDED = 1;
        const PRESENT = 2;
        const _ = !0;
} }
impl FrameDemandFlag {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct FreeCameraOptionField: u32 {
        const POSITION = 1;
        const ORIENTATION = 2;
        const _ = !0;
} }
impl FreeCameraOptionField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct FreeCameraOptions {
    pub position: Option<crate::generated::Vec3>,
    pub orientation: Option<crate::generated::Quaternion>,
}
impl Default for FreeCameraOptions {
    fn default() -> Self {
        Self::from_native(unsafe { maplibre_native_ffi_sys::mln_free_camera_options_default() })
    }
}
impl FreeCameraOptions {
    #[doc(hidden)]
    pub fn to_native(&self) -> maplibre_native_ffi_sys::mln_free_camera_options {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_free_camera_options_default() };
        raw.fields = 0;
        if let Some(value) = self.position {
            raw.fields |= maplibre_native_ffi_sys::MLN_FREE_CAMERA_OPTION_POSITION;
            raw.position = value.to_native();
        }
        if let Some(value) = self.orientation {
            raw.fields |= maplibre_native_ffi_sys::MLN_FREE_CAMERA_OPTION_ORIENTATION;
            raw.orientation = value.to_native();
        }
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_free_camera_options) -> Self {
        Self {
            position: (raw.fields & maplibre_native_ffi_sys::MLN_FREE_CAMERA_OPTION_POSITION != 0)
                .then(|| crate::generated::Vec3::from_native(raw.position)),
            orientation: (raw.fields & maplibre_native_ffi_sys::MLN_FREE_CAMERA_OPTION_ORIENTATION
                != 0)
                .then(|| crate::generated::Quaternion::from_native(raw.orientation)),
        }
    }
}
impl crate::values::NativeValue for FreeCameraOptions {
    type Raw = maplibre_native_ffi_sys::mln_free_camera_options;
    fn to_native(self) -> Self::Raw {
        FreeCameraOptions::to_native(&self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct GeojsonSourceOptionField: u32 {
        const MIN_ZOOM = 1;
        const MAX_ZOOM = 2;
        const TOLERANCE = 4;
        const CLUSTER_MAX_ZOOM = 8;
        const CLUSTER_PROPERTIES = 16;
        const TILE_SIZE = 32;
        const BUFFER = 64;
        const CLUSTER_RADIUS = 128;
        const CLUSTER_MIN_POINTS = 256;
        const LINE_METRICS = 512;
        const CLUSTER = 1024;
        const SYNCHRONOUS_TILING = 2048;
        const _ = !0;
} }
impl GeojsonSourceOptionField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct GeojsonSourceOptions {
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
    pub tolerance: Option<f64>,
    pub cluster_max_zoom: Option<f64>,
    pub cluster_properties: Option<Vec<u8>>,
    pub tile_size: Option<u32>,
    pub buffer: Option<u32>,
    pub cluster_radius: Option<u32>,
    pub cluster_min_points: Option<u32>,
    pub line_metrics: Option<bool>,
    pub cluster: Option<bool>,
    pub synchronous_tiling: Option<bool>,
}
impl Default for GeojsonSourceOptions {
    fn default() -> Self {
        unsafe {
            Self::from_native(maplibre_native_ffi_sys::mln_geojson_source_options_default())
                .expect("native default must be valid")
        }
    }
}
impl GeojsonSourceOptions {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_geojson_source_options> {
        let mut raw: maplibre_native_ffi_sys::mln_geojson_source_options =
            unsafe { maplibre_native_ffi_sys::mln_geojson_source_options_default() };
        raw.fields = 0;
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_geojson_source_options>() as _;
        if let Some(item) = &self.min_zoom {
            raw.fields |= maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM;
            raw.min_zoom = *(item);
        }
        if let Some(item) = &self.max_zoom {
            raw.fields |= maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM;
            raw.max_zoom = *(item);
        }
        if let Some(item) = &self.tolerance {
            raw.fields |= maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_TOLERANCE;
            raw.tolerance = *(item);
        }
        if let Some(item) = &self.cluster_max_zoom {
            raw.fields |= maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM;
            raw.cluster_max_zoom = *(item);
        }
        if let Some(item) = &self.cluster_properties {
            raw.fields |= maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
            raw.cluster_properties = maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_ptr().cast(),
                size: (item).len(),
            };
        }
        if let Some(item) = &self.tile_size {
            raw.fields |= maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE;
            raw.tile_size = *(item);
        }
        if let Some(item) = &self.buffer {
            raw.fields |= maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_BUFFER;
            raw.buffer = *(item);
        }
        if let Some(item) = &self.cluster_radius {
            raw.fields |= maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS;
            raw.cluster_radius = *(item);
        }
        if let Some(item) = &self.cluster_min_points {
            raw.fields |= maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS;
            raw.cluster_min_points = *(item);
        }
        if let Some(item) = &self.line_metrics {
            raw.fields |= maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS;
            raw.line_metrics = *(item);
        }
        if let Some(item) = &self.cluster {
            raw.fields |= maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
            raw.cluster = *(item);
        }
        if let Some(item) = &self.synchronous_tiling {
            raw.fields |= maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING;
            raw.synchronous_tiling = *(item);
        }
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_geojson_source_options,
    ) -> crate::Result<Self> {
        Ok(Self {
            min_zoom: if raw.fields & maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM
                != 0
            {
                Some(raw.min_zoom)
            } else {
                None
            },
            max_zoom: if raw.fields & maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM
                != 0
            {
                Some(raw.max_zoom)
            } else {
                None
            },
            tolerance: if raw.fields & maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_TOLERANCE
                != 0
            {
                Some(raw.tolerance)
            } else {
                None
            },
            cluster_max_zoom: if raw.fields
                & maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM
                != 0
            {
                Some(raw.cluster_max_zoom)
            } else {
                None
            },
            cluster_properties: if raw.fields
                & maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES
                != 0
            {
                Some(unsafe { crate::string::copy_string_view_bytes(raw.cluster_properties) }?)
            } else {
                None
            },
            tile_size: if raw.fields & maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE
                != 0
            {
                Some(raw.tile_size)
            } else {
                None
            },
            buffer: if raw.fields & maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_BUFFER != 0 {
                Some(raw.buffer)
            } else {
                None
            },
            cluster_radius: if raw.fields
                & maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS
                != 0
            {
                Some(raw.cluster_radius)
            } else {
                None
            },
            cluster_min_points: if raw.fields
                & maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
                != 0
            {
                Some(raw.cluster_min_points)
            } else {
                None
            },
            line_metrics: if raw.fields
                & maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS
                != 0
            {
                Some(raw.line_metrics)
            } else {
                None
            },
            cluster: if raw.fields & maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER != 0
            {
                Some(raw.cluster)
            } else {
                None
            },
            synchronous_tiling: if raw.fields
                & maplibre_native_ffi_sys::MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING
                != 0
            {
                Some(raw.synchronous_tiling)
            } else {
                None
            },
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum GesturePhase {
    Cancel,
    End,
    Update,
    Begin,
    None,
    Unknown(u32),
}
impl Default for GesturePhase {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl GesturePhase {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            4 => Self::Cancel,
            3 => Self::End,
            2 => Self::Update,
            1 => Self::Begin,
            0 => Self::None,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Cancel => 4,
            Self::End => 3,
            Self::Update => 2,
            Self::Begin => 1,
            Self::None => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct GpuSync {
    pub kind: crate::generated::GpuSyncKind,
    pub object: u64,
    pub value: u64,
}
impl Default for GpuSync {
    fn default() -> Self {
        Self::from_native(unsafe { maplibre_native_ffi_sys::mln_gpu_sync_default() })
    }
}
impl GpuSync {
    pub const fn new(kind: crate::generated::GpuSyncKind, object: u64, value: u64) -> Self {
        Self {
            kind,
            object,
            value,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_gpu_sync {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_gpu_sync_default() };
        raw.kind = self.kind.to_native();
        raw.object = self.object;
        raw.value = self.value;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_gpu_sync) -> Self {
        Self {
            kind: crate::generated::GpuSyncKind::from_native(raw.kind),
            object: raw.object,
            value: raw.value,
        }
    }
}
impl crate::values::NativeValue for GpuSync {
    type Raw = maplibre_native_ffi_sys::mln_gpu_sync;
    fn to_native(self) -> Self::Raw {
        GpuSync::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum GpuSyncKind {
    WebgpuToken,
    OpenglFence,
    VulkanTimelineSemaphore,
    MetalSharedEvent,
    CpuComplete,
    Unknown(u32),
}
impl Default for GpuSyncKind {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl GpuSyncKind {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            4 => Self::WebgpuToken,
            3 => Self::OpenglFence,
            2 => Self::VulkanTimelineSemaphore,
            1 => Self::MetalSharedEvent,
            0 => Self::CpuComplete,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::WebgpuToken => 4,
            Self::OpenglFence => 3,
            Self::VulkanTimelineSemaphore => 2,
            Self::MetalSharedEvent => 1,
            Self::CpuComplete => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Clone, Default)]
pub struct HttpHeaderTransform {
    pub callback: Option<
        std::sync::Arc<
            dyn Fn(
                    crate::generated::ResourceKind,
                    String,
                    &mut crate::generated::HttpHeaderTransformResponse<'_>,
                ) -> crate::Result<()>
                + Send
                + Sync
                + 'static,
        >,
    >,
}

impl std::fmt::Debug for HttpHeaderTransform {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("HttpHeaderTransform")
            .finish_non_exhaustive()
    }
}
impl HttpHeaderTransform {
    pub fn with_callback<F>(mut self, callback: F) -> Self
    where
        F: Fn(
                crate::generated::ResourceKind,
                String,
                &mut crate::generated::HttpHeaderTransformResponse<'_>,
            ) -> crate::Result<()>
            + Send
            + Sync
            + 'static,
    {
        self.callback = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(
                crate::generated::ResourceKind,
                String,
                &mut crate::generated::HttpHeaderTransformResponse<'_>,
            ) -> crate::Result<()>
            + Send
            + Sync
            + 'static,
    {
        Self::default().with_callback(callback)
    }
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_http_header_transform> {
        let mut raw: maplibre_native_ffi_sys::mln_http_header_transform =
            unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_http_header_transform>() as _;
        raw.callback = self
            .callback
            .as_ref()
            .map(|_| Self::callback_trampoline as _);
        if !(raw.callback.is_none()) {
            raw.user_data = unsafe { arena.registration(self.clone(), Self::release_callback) };
            raw.release_user_data = Some(Self::release_callback);
        }
        Ok(raw)
    }
    /// Captures an empty callback descriptor returned by its native constructor.
    ///
    /// # Safety
    /// Borrowed fields must remain valid for this conversion.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_http_header_transform,
    ) -> crate::Result<Self> {
        if !(raw.callback.is_none()) {
            return Err(crate::Error::invalid_argument(
                "foreign callbacks cannot be adopted",
            ));
        }
        Ok(Self { callback: None })
    }
    unsafe extern "C" fn release_callback(user_data: *mut std::ffi::c_void) {
        let _policy = crate::callback::PolicyScope::enter(&[], 0);
        let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| unsafe {
            drop(Box::from_raw(user_data.cast::<Self>()))
        }));
    }
    unsafe extern "C" fn callback_trampoline(
        binding_arg_0: *mut std::ffi::c_void,
        binding_arg_1: u32,
        binding_arg_2: *const std::ffi::c_char,
        binding_arg_3: *mut maplibre_native_ffi_sys::mln_http_header_transform_response,
    ) -> maplibre_native_ffi_sys::mln_status {
        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(
            || -> crate::Result<maplibre_native_ffi_sys::mln_status> {
                let _policy = crate::callback::PolicyScope::enter(
                    &["mln_http_header_transform_response_set"],
                    binding_arg_3 as usize as u64,
                );
                let state = unsafe { &*binding_arg_0.cast::<Self>() };
                let callback = state
                    .callback
                    .as_ref()
                    .ok_or_else(|| crate::Error::invalid_argument("missing callback"))?;
                callback(
                    crate::generated::ResourceKind::from_native(binding_arg_1),
                    unsafe { crate::string::copy_c_string(binding_arg_2) }?,
                    &mut HttpHeaderTransformResponse {
                        raw: std::ptr::NonNull::new(binding_arg_3).ok_or_else(|| {
                            crate::Error::invalid_argument("null callback response")
                        })?,
                        lifetime: std::marker::PhantomData,
                    },
                )?;
                Ok(maplibre_native_ffi_sys::MLN_STATUS_OK)
            },
        ));
        match result {
            Ok(Ok(value)) => value,
            Ok(Err(error)) => crate::resource::status_for_error(&error),
            _ => maplibre_native_ffi_sys::MLN_STATUS_NATIVE_ERROR,
        }
    }
}

/// A native response borrowed only for one host callback.
#[derive(Debug)]
pub struct HttpHeaderTransformResponse<'a> {
    raw: std::ptr::NonNull<maplibre_native_ffi_sys::mln_http_header_transform_response>,
    lifetime: std::marker::PhantomData<
        &'a mut maplibre_native_ffi_sys::mln_http_header_transform_response,
    >,
}
impl HttpHeaderTransformResponse<'_> {
    pub fn set(&mut self, name: &str, value: &str) -> crate::Result<()> {
        crate::callback::check(
            "mln_http_header_transform_response_set",
            self.raw.as_ptr() as usize as u64,
        )?;
        let name_size = name
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
        let value_size = value
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
        crate::check(|diagnostic| unsafe {
            maplibre_native_ffi_sys::mln_http_header_transform_response_set(
                self.raw.as_ptr(),
                (name).as_bytes().as_ptr().cast(),
                name_size,
                (value).as_bytes().as_ptr().cast(),
                value_size,
                diagnostic,
            )
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct ImageContent {
    pub left: f32,
    pub top: f32,
    pub right: f32,
    pub bottom: f32,
}

impl ImageContent {
    pub const fn new(left: f32, top: f32, right: f32, bottom: f32) -> Self {
        Self {
            left,
            top,
            right,
            bottom,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_image_content {
        let mut raw = unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_image_content>() };
        raw.left = self.left;
        raw.top = self.top;
        raw.right = self.right;
        raw.bottom = self.bottom;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_image_content) -> Self {
        Self {
            left: raw.left,
            top: raw.top,
            right: raw.right,
            bottom: raw.bottom,
        }
    }
}
impl crate::values::NativeValue for ImageContent {
    type Raw = maplibre_native_ffi_sys::mln_image_content;
    fn to_native(self) -> Self::Raw {
        ImageContent::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct ImageStretch {
    pub from: f32,
    pub to: f32,
}

impl ImageStretch {
    pub const fn new(from: f32, to: f32) -> Self {
        Self { from, to }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_image_stretch {
        let mut raw = unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_image_stretch>() };
        raw.from = self.from;
        raw.to = self.to;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_image_stretch) -> Self {
        Self {
            from: raw.from,
            to: raw.to,
        }
    }
}
impl crate::values::NativeValue for ImageStretch {
    type Raw = maplibre_native_ffi_sys::mln_image_stretch;
    fn to_native(self) -> Self::Raw {
        ImageStretch::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct LatLng {
    pub latitude: f64,
    pub longitude: f64,
}

impl LatLng {
    pub const fn new(latitude: f64, longitude: f64) -> Self {
        Self {
            latitude,
            longitude,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_lat_lng {
        let mut raw = unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_lat_lng>() };
        raw.latitude = self.latitude;
        raw.longitude = self.longitude;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_lat_lng) -> Self {
        Self {
            latitude: raw.latitude,
            longitude: raw.longitude,
        }
    }
}
impl crate::values::NativeValue for LatLng {
    type Raw = maplibre_native_ffi_sys::mln_lat_lng;
    fn to_native(self) -> Self::Raw {
        LatLng::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct LatLngBounds {
    pub southwest: crate::generated::LatLng,
    pub northeast: crate::generated::LatLng,
}

impl LatLngBounds {
    pub const fn new(
        southwest: crate::generated::LatLng,
        northeast: crate::generated::LatLng,
    ) -> Self {
        Self {
            southwest,
            northeast,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_lat_lng_bounds {
        let mut raw = unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_lat_lng_bounds>() };
        raw.southwest = self.southwest.to_native();
        raw.northeast = self.northeast.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_lat_lng_bounds) -> Self {
        Self {
            southwest: crate::generated::LatLng::from_native(raw.southwest),
            northeast: crate::generated::LatLng::from_native(raw.northeast),
        }
    }
}
impl crate::values::NativeValue for LatLngBounds {
    type Raw = maplibre_native_ffi_sys::mln_lat_lng_bounds;
    fn to_native(self) -> Self::Raw {
        LatLngBounds::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum LocationIndicatorImageKind {
    Shadow,
    Bearing,
    Top,
    Unknown(u32),
}
impl Default for LocationIndicatorImageKind {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl LocationIndicatorImageKind {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            2 => Self::Shadow,
            1 => Self::Bearing,
            0 => Self::Top,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Shadow => 2,
            Self::Bearing => 1,
            Self::Top => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum LogEvent {
    Timing,
    Glyph,
    Crash,
    Android,
    Jni,
    GraphicsBackend,
    Image,
    Sprite,
    HttpRequest,
    Database,
    Style,
    Render,
    ParseTile,
    ParseStyle,
    Shader,
    Setup,
    General,
    Unknown(u32),
}
impl Default for LogEvent {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl LogEvent {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            16 => Self::Timing,
            15 => Self::Glyph,
            14 => Self::Crash,
            13 => Self::Android,
            12 => Self::Jni,
            11 => Self::GraphicsBackend,
            10 => Self::Image,
            9 => Self::Sprite,
            8 => Self::HttpRequest,
            7 => Self::Database,
            6 => Self::Style,
            5 => Self::Render,
            4 => Self::ParseTile,
            3 => Self::ParseStyle,
            2 => Self::Shader,
            1 => Self::Setup,
            0 => Self::General,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Timing => 16,
            Self::Glyph => 15,
            Self::Crash => 14,
            Self::Android => 13,
            Self::Jni => 12,
            Self::GraphicsBackend => 11,
            Self::Image => 10,
            Self::Sprite => 9,
            Self::HttpRequest => 8,
            Self::Database => 7,
            Self::Style => 6,
            Self::Render => 5,
            Self::ParseTile => 4,
            Self::ParseStyle => 3,
            Self::Shader => 2,
            Self::Setup => 1,
            Self::General => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum LogSeverity {
    Error,
    Warning,
    Info,
    Unknown(u32),
}
impl Default for LogSeverity {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl LogSeverity {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            3 => Self::Error,
            2 => Self::Warning,
            1 => Self::Info,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Error => 3,
            Self::Warning => 2,
            Self::Info => 1,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct LogSeverityMask: u32 {
        const INFO = 2;
        const WARNING = 4;
        const ERROR = 8;
        const DEFAULT = 6;
        const ALL = 14;
        const _ = !0;
} }
impl LogSeverityMask {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct LogicalExtent {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
}

impl LogicalExtent {
    pub const fn new(width: u32, height: u32, scale_factor: f64) -> Self {
        Self {
            width,
            height,
            scale_factor,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_logical_extent {
        let mut raw = unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_logical_extent>() };
        raw.width = self.width;
        raw.height = self.height;
        raw.scale_factor = self.scale_factor;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_logical_extent) -> Self {
        Self {
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
        }
    }
}
impl crate::values::NativeValue for LogicalExtent {
    type Raw = maplibre_native_ffi_sys::mln_logical_extent;
    fn to_native(self) -> Self::Raw {
        LogicalExtent::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct MapDebugOption: u32 {
        const TILE_BORDERS = 2;
        const PARSE_STATUS = 4;
        const TIMESTAMPS = 8;
        const COLLISION = 16;
        const OVERDRAW = 32;
        const STENCIL_CLIP = 64;
        const DEPTH_BUFFER = 128;
        const _ = !0;
} }
impl MapDebugOption {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum MapMode {
    Tile,
    Static,
    Continuous,
    Unknown(u32),
}
impl Default for MapMode {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl MapMode {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            2 => Self::Tile,
            1 => Self::Static,
            0 => Self::Continuous,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Tile => 2,
            Self::Static => 1,
            Self::Continuous => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MapOptions {
    pub initial_extent: crate::generated::LogicalExtent,
    pub map_mode: crate::generated::MapMode,
    pub fast_pfor_enabled: bool,
    pub event_mask: crate::generated::RuntimeEventMask,
}
impl Default for MapOptions {
    fn default() -> Self {
        Self::from_native(unsafe { maplibre_native_ffi_sys::mln_map_options_default() })
    }
}
impl MapOptions {
    pub const fn new(
        initial_extent: crate::generated::LogicalExtent,
        map_mode: crate::generated::MapMode,
        fast_pfor_enabled: bool,
        event_mask: crate::generated::RuntimeEventMask,
    ) -> Self {
        Self {
            initial_extent,
            map_mode,
            fast_pfor_enabled,
            event_mask,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_map_options {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_map_options_default() };
        raw.initial_extent = self.initial_extent.to_native();
        raw.map_mode = self.map_mode.to_native();
        raw.fast_pfor_enabled = self.fast_pfor_enabled;
        raw.event_mask = self.event_mask.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_map_options) -> Self {
        Self {
            initial_extent: crate::generated::LogicalExtent::from_native(raw.initial_extent),
            map_mode: crate::generated::MapMode::from_native(raw.map_mode),
            fast_pfor_enabled: raw.fast_pfor_enabled,
            event_mask: crate::generated::RuntimeEventMask::from_native(raw.event_mask),
        }
    }
}
impl crate::values::NativeValue for MapOptions {
    type Raw = maplibre_native_ffi_sys::mln_map_options;
    fn to_native(self) -> Self::Raw {
        MapOptions::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MapSnapshot {
    pub debug_options: crate::generated::MapDebugOption,
    pub generation: u64,
    pub camera: crate::generated::CameraOptions,
    pub logical_extent: crate::generated::LogicalExtent,
    pub projection_mode: crate::generated::ProjectionMode,
    pub viewport: crate::generated::MapViewportOptions,
    pub fully_loaded: bool,
    pub rendering_stats_view_enabled: bool,
    pub repaint_demand: bool,
    pub gesture_in_progress: bool,
    pub event_mask: crate::generated::RuntimeEventMask,
    pub latest_render_update_generation: u64,
    pub tile: crate::generated::MapTileOptions,
    pub bounds: crate::generated::BoundOptions,
    pub free_camera: crate::generated::FreeCameraOptions,
}

impl MapSnapshot {
    pub const fn new(
        debug_options: crate::generated::MapDebugOption,
        generation: u64,
        camera: crate::generated::CameraOptions,
        logical_extent: crate::generated::LogicalExtent,
        projection_mode: crate::generated::ProjectionMode,
        viewport: crate::generated::MapViewportOptions,
        fully_loaded: bool,
        rendering_stats_view_enabled: bool,
        repaint_demand: bool,
        gesture_in_progress: bool,
        event_mask: crate::generated::RuntimeEventMask,
        latest_render_update_generation: u64,
        tile: crate::generated::MapTileOptions,
        bounds: crate::generated::BoundOptions,
        free_camera: crate::generated::FreeCameraOptions,
    ) -> Self {
        Self {
            debug_options,
            generation,
            camera,
            logical_extent,
            projection_mode,
            viewport,
            fully_loaded,
            rendering_stats_view_enabled,
            repaint_demand,
            gesture_in_progress,
            event_mask,
            latest_render_update_generation,
            tile,
            bounds,
            free_camera,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_map_snapshot {
        let mut raw = unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_map_snapshot>() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_map_snapshot>() as _;
        raw.debug_options = self.debug_options.to_native();
        raw.generation = self.generation;
        raw.camera = self.camera.to_native();
        raw.logical_extent = self.logical_extent.to_native();
        raw.projection_mode = self.projection_mode.to_native();
        raw.viewport = self.viewport.to_native();
        raw.fully_loaded = self.fully_loaded;
        raw.rendering_stats_view_enabled = self.rendering_stats_view_enabled;
        raw.repaint_demand = self.repaint_demand;
        raw.gesture_in_progress = self.gesture_in_progress;
        raw.event_mask = self.event_mask.to_native();
        raw.latest_render_update_generation = self.latest_render_update_generation;
        raw.tile = self.tile.to_native();
        raw.bounds = self.bounds.to_native();
        raw.free_camera = self.free_camera.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_map_snapshot) -> Self {
        Self {
            debug_options: crate::generated::MapDebugOption::from_native(raw.debug_options),
            generation: raw.generation,
            camera: crate::generated::CameraOptions::from_native(raw.camera),
            logical_extent: crate::generated::LogicalExtent::from_native(raw.logical_extent),
            projection_mode: crate::generated::ProjectionMode::from_native(raw.projection_mode),
            viewport: crate::generated::MapViewportOptions::from_native(raw.viewport),
            fully_loaded: raw.fully_loaded,
            rendering_stats_view_enabled: raw.rendering_stats_view_enabled,
            repaint_demand: raw.repaint_demand,
            gesture_in_progress: raw.gesture_in_progress,
            event_mask: crate::generated::RuntimeEventMask::from_native(raw.event_mask),
            latest_render_update_generation: raw.latest_render_update_generation,
            tile: crate::generated::MapTileOptions::from_native(raw.tile),
            bounds: crate::generated::BoundOptions::from_native(raw.bounds),
            free_camera: crate::generated::FreeCameraOptions::from_native(raw.free_camera),
        }
    }
}
impl crate::values::NativeValue for MapSnapshot {
    type Raw = maplibre_native_ffi_sys::mln_map_snapshot;
    fn to_native(self) -> Self::Raw {
        MapSnapshot::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct MapTileOptionField: u32 {
        const PREFETCH_ZOOM_DELTA = 1;
        const LOD_MIN_RADIUS = 2;
        const LOD_SCALE = 4;
        const LOD_PITCH_THRESHOLD = 8;
        const LOD_ZOOM_SHIFT = 16;
        const LOD_MODE = 32;
        const _ = !0;
} }
impl MapTileOptionField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MapTileOptions {
    pub prefetch_zoom_delta: Option<u32>,
    pub lod_min_radius: Option<f64>,
    pub lod_scale: Option<f64>,
    pub lod_pitch_threshold: Option<f64>,
    pub lod_zoom_shift: Option<f64>,
    pub lod_mode: Option<crate::generated::TileLodMode>,
}
impl Default for MapTileOptions {
    fn default() -> Self {
        Self::from_native(unsafe { maplibre_native_ffi_sys::mln_map_tile_options_default() })
    }
}
impl MapTileOptions {
    #[doc(hidden)]
    pub fn to_native(&self) -> maplibre_native_ffi_sys::mln_map_tile_options {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_map_tile_options_default() };
        raw.fields = 0;
        if let Some(value) = self.prefetch_zoom_delta {
            raw.fields |= maplibre_native_ffi_sys::MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
            raw.prefetch_zoom_delta = value;
        }
        if let Some(value) = self.lod_min_radius {
            raw.fields |= maplibre_native_ffi_sys::MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS;
            raw.lod_min_radius = value;
        }
        if let Some(value) = self.lod_scale {
            raw.fields |= maplibre_native_ffi_sys::MLN_MAP_TILE_OPTION_LOD_SCALE;
            raw.lod_scale = value;
        }
        if let Some(value) = self.lod_pitch_threshold {
            raw.fields |= maplibre_native_ffi_sys::MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD;
            raw.lod_pitch_threshold = value;
        }
        if let Some(value) = self.lod_zoom_shift {
            raw.fields |= maplibre_native_ffi_sys::MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT;
            raw.lod_zoom_shift = value;
        }
        if let Some(value) = self.lod_mode {
            raw.fields |= maplibre_native_ffi_sys::MLN_MAP_TILE_OPTION_LOD_MODE;
            raw.lod_mode = value.to_native();
        }
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_map_tile_options) -> Self {
        Self {
            prefetch_zoom_delta: (raw.fields
                & maplibre_native_ffi_sys::MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA
                != 0)
                .then(|| raw.prefetch_zoom_delta),
            lod_min_radius: (raw.fields
                & maplibre_native_ffi_sys::MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS
                != 0)
                .then(|| raw.lod_min_radius),
            lod_scale: (raw.fields & maplibre_native_ffi_sys::MLN_MAP_TILE_OPTION_LOD_SCALE != 0)
                .then(|| raw.lod_scale),
            lod_pitch_threshold: (raw.fields
                & maplibre_native_ffi_sys::MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD
                != 0)
                .then(|| raw.lod_pitch_threshold),
            lod_zoom_shift: (raw.fields
                & maplibre_native_ffi_sys::MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT
                != 0)
                .then(|| raw.lod_zoom_shift),
            lod_mode: (raw.fields & maplibre_native_ffi_sys::MLN_MAP_TILE_OPTION_LOD_MODE != 0)
                .then(|| crate::generated::TileLodMode::from_native(raw.lod_mode)),
        }
    }
}
impl crate::values::NativeValue for MapTileOptions {
    type Raw = maplibre_native_ffi_sys::mln_map_tile_options;
    fn to_native(self) -> Self::Raw {
        MapTileOptions::to_native(&self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct MapViewportOptionField: u32 {
        const NORTH_ORIENTATION = 1;
        const CONSTRAIN_MODE = 2;
        const VIEWPORT_MODE = 4;
        const FRUSTUM_OFFSET = 8;
        const _ = !0;
} }
impl MapViewportOptionField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MapViewportOptions {
    pub north_orientation: Option<crate::generated::NorthOrientation>,
    pub constrain_mode: Option<crate::generated::ConstrainMode>,
    pub viewport_mode: Option<crate::generated::ViewportMode>,
    pub frustum_offset: Option<crate::generated::EdgeInsets>,
}
impl Default for MapViewportOptions {
    fn default() -> Self {
        Self::from_native(unsafe { maplibre_native_ffi_sys::mln_map_viewport_options_default() })
    }
}
impl MapViewportOptions {
    #[doc(hidden)]
    pub fn to_native(&self) -> maplibre_native_ffi_sys::mln_map_viewport_options {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_map_viewport_options_default() };
        raw.fields = 0;
        if let Some(value) = self.north_orientation {
            raw.fields |= maplibre_native_ffi_sys::MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
            raw.north_orientation = value.to_native();
        }
        if let Some(value) = self.constrain_mode {
            raw.fields |= maplibre_native_ffi_sys::MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
            raw.constrain_mode = value.to_native();
        }
        if let Some(value) = self.viewport_mode {
            raw.fields |= maplibre_native_ffi_sys::MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
            raw.viewport_mode = value.to_native();
        }
        if let Some(value) = self.frustum_offset {
            raw.fields |= maplibre_native_ffi_sys::MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
            raw.frustum_offset = value.to_native();
        }
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_map_viewport_options) -> Self {
        Self {
            north_orientation: (raw.fields
                & maplibre_native_ffi_sys::MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION
                != 0)
                .then(|| crate::generated::NorthOrientation::from_native(raw.north_orientation)),
            constrain_mode: (raw.fields
                & maplibre_native_ffi_sys::MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE
                != 0)
                .then(|| crate::generated::ConstrainMode::from_native(raw.constrain_mode)),
            viewport_mode: (raw.fields
                & maplibre_native_ffi_sys::MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE
                != 0)
                .then(|| crate::generated::ViewportMode::from_native(raw.viewport_mode)),
            frustum_offset: (raw.fields
                & maplibre_native_ffi_sys::MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET
                != 0)
                .then(|| crate::generated::EdgeInsets::from_native(raw.frustum_offset)),
        }
    }
}
impl crate::values::NativeValue for MapViewportOptions {
    type Raw = maplibre_native_ffi_sys::mln_map_viewport_options;
    fn to_native(self) -> Self::Raw {
        MapViewportOptions::to_native(&self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalBorrowedTextureDescriptor {
    pub extent: crate::generated::RenderTargetExtent,
    pub physical_width: u32,
    pub physical_height: u32,
    pub texture: *mut std::ffi::c_void,
}
impl Default for MetalBorrowedTextureDescriptor {
    fn default() -> Self {
        Self::from_native(unsafe {
            maplibre_native_ffi_sys::mln_metal_borrowed_texture_descriptor_default()
        })
    }
}
impl MetalBorrowedTextureDescriptor {
    pub const fn new(
        extent: crate::generated::RenderTargetExtent,
        physical_width: u32,
        physical_height: u32,
        texture: *mut std::ffi::c_void,
    ) -> Self {
        Self {
            extent,
            physical_width,
            physical_height,
            texture,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_metal_borrowed_texture_descriptor {
        let mut raw =
            unsafe { maplibre_native_ffi_sys::mln_metal_borrowed_texture_descriptor_default() };
        raw.extent = self.extent.to_native();
        raw.physical_width = self.physical_width;
        raw.physical_height = self.physical_height;
        raw.texture = self.texture;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(
        raw: maplibre_native_ffi_sys::mln_metal_borrowed_texture_descriptor,
    ) -> Self {
        Self {
            extent: crate::generated::RenderTargetExtent::from_native(raw.extent),
            physical_width: raw.physical_width,
            physical_height: raw.physical_height,
            texture: raw.texture,
        }
    }
}
impl crate::values::NativeValue for MetalBorrowedTextureDescriptor {
    type Raw = maplibre_native_ffi_sys::mln_metal_borrowed_texture_descriptor;
    fn to_native(self) -> Self::Raw {
        MetalBorrowedTextureDescriptor::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MetalContextDescriptor {
    pub device: *mut std::ffi::c_void,
}

impl MetalContextDescriptor {
    pub const fn new(device: *mut std::ffi::c_void) -> Self {
        Self { device }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_metal_context_descriptor {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_metal_context_descriptor>() };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_metal_context_descriptor>() as _;
        raw.device = self.device;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_metal_context_descriptor) -> Self {
        Self { device: raw.device }
    }
}
impl crate::values::NativeValue for MetalContextDescriptor {
    type Raw = maplibre_native_ffi_sys::mln_metal_context_descriptor;
    fn to_native(self) -> Self::Raw {
        MetalContextDescriptor::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalOwnedTextureDescriptor {
    pub extent: crate::generated::RenderTargetExtent,
    pub context: crate::generated::MetalContextDescriptor,
}
impl Default for MetalOwnedTextureDescriptor {
    fn default() -> Self {
        Self::from_native(unsafe {
            maplibre_native_ffi_sys::mln_metal_owned_texture_descriptor_default()
        })
    }
}
impl MetalOwnedTextureDescriptor {
    pub const fn new(
        extent: crate::generated::RenderTargetExtent,
        context: crate::generated::MetalContextDescriptor,
    ) -> Self {
        Self { extent, context }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_metal_owned_texture_descriptor {
        let mut raw =
            unsafe { maplibre_native_ffi_sys::mln_metal_owned_texture_descriptor_default() };
        raw.extent = self.extent.to_native();
        raw.context = self.context.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_metal_owned_texture_descriptor) -> Self {
        Self {
            extent: crate::generated::RenderTargetExtent::from_native(raw.extent),
            context: crate::generated::MetalContextDescriptor::from_native(raw.context),
        }
    }
}
impl crate::values::NativeValue for MetalOwnedTextureDescriptor {
    type Raw = maplibre_native_ffi_sys::mln_metal_owned_texture_descriptor;
    fn to_native(self) -> Self::Raw {
        MetalOwnedTextureDescriptor::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MetalOwnedTextureFrame {
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub texture: *mut std::ffi::c_void,
    pub device: *mut std::ffi::c_void,
    pub pixel_format: u64,
}

impl MetalOwnedTextureFrame {
    pub const fn new(
        generation: u64,
        width: u32,
        height: u32,
        scale_factor: f64,
        frame_id: u64,
        texture: *mut std::ffi::c_void,
        device: *mut std::ffi::c_void,
        pixel_format: u64,
    ) -> Self {
        Self {
            generation,
            width,
            height,
            scale_factor,
            frame_id,
            texture,
            device,
            pixel_format,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_metal_owned_texture_frame {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_metal_owned_texture_frame>() };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_metal_owned_texture_frame>() as _;
        raw.generation = self.generation;
        raw.width = self.width;
        raw.height = self.height;
        raw.scale_factor = self.scale_factor;
        raw.frame_id = self.frame_id;
        raw.texture = self.texture;
        raw.device = self.device;
        raw.pixel_format = self.pixel_format;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_metal_owned_texture_frame) -> Self {
        Self {
            generation: raw.generation,
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: raw.frame_id,
            texture: raw.texture,
            device: raw.device,
            pixel_format: raw.pixel_format,
        }
    }
}
impl crate::values::NativeValue for MetalOwnedTextureFrame {
    type Raw = maplibre_native_ffi_sys::mln_metal_owned_texture_frame;
    fn to_native(self) -> Self::Raw {
        MetalOwnedTextureFrame::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalSurfaceDescriptor {
    pub extent: crate::generated::RenderTargetExtent,
    pub context: crate::generated::MetalContextDescriptor,
    pub layer: *mut std::ffi::c_void,
}
impl Default for MetalSurfaceDescriptor {
    fn default() -> Self {
        Self::from_native(unsafe {
            maplibre_native_ffi_sys::mln_metal_surface_descriptor_default()
        })
    }
}
impl MetalSurfaceDescriptor {
    pub const fn new(
        extent: crate::generated::RenderTargetExtent,
        context: crate::generated::MetalContextDescriptor,
        layer: *mut std::ffi::c_void,
    ) -> Self {
        Self {
            extent,
            context,
            layer,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_metal_surface_descriptor {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_metal_surface_descriptor_default() };
        raw.extent = self.extent.to_native();
        raw.context = self.context.to_native();
        raw.layer = self.layer;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_metal_surface_descriptor) -> Self {
        Self {
            extent: crate::generated::RenderTargetExtent::from_native(raw.extent),
            context: crate::generated::MetalContextDescriptor::from_native(raw.context),
            layer: raw.layer,
        }
    }
}
impl crate::values::NativeValue for MetalSurfaceDescriptor {
    type Raw = maplibre_native_ffi_sys::mln_metal_surface_descriptor;
    fn to_native(self) -> Self::Raw {
        MetalSurfaceDescriptor::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum NetworkStatus {
    Offline,
    Online,
    Unknown(u32),
}
impl Default for NetworkStatus {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl NetworkStatus {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            2 => Self::Offline,
            1 => Self::Online,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Offline => 2,
            Self::Online => 1,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum NorthOrientation {
    Left,
    Down,
    Right,
    Up,
    Unknown(u32),
}
impl Default for NorthOrientation {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl NorthOrientation {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            3 => Self::Left,
            2 => Self::Down,
            1 => Self::Right,
            0 => Self::Up,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Left => 3,
            Self::Down => 2,
            Self::Right => 1,
            Self::Up => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct OfflineGeometryRegionDefinition {
    pub style_url: String,
    pub geometry: Vec<u8>,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub pixel_ratio: f32,
    pub include_ideographs: bool,
}

impl OfflineGeometryRegionDefinition {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_offline_geometry_region_definition> {
        let mut raw: maplibre_native_ffi_sys::mln_offline_geometry_region_definition =
            unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<
            maplibre_native_ffi_sys::mln_offline_geometry_region_definition,
        >() as _;
        raw.style_url = arena.c_string((&self.style_url))?;
        raw.geometry = maplibre_native_ffi_sys::mln_buffer_view {
            data: (&self.geometry).as_ptr().cast(),
            size: (&self.geometry).len(),
        };
        raw.min_zoom = *(&self.min_zoom);
        raw.max_zoom = *(&self.max_zoom);
        raw.pixel_ratio = *(&self.pixel_ratio);
        raw.include_ideographs = *(&self.include_ideographs);
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_offline_geometry_region_definition,
    ) -> crate::Result<Self> {
        Ok(Self {
            style_url: unsafe { crate::string::copy_c_string(raw.style_url) }?,
            geometry: unsafe { crate::string::copy_string_view_bytes(raw.geometry) }?,
            min_zoom: raw.min_zoom,
            max_zoom: raw.max_zoom,
            pixel_ratio: raw.pixel_ratio,
            include_ideographs: raw.include_ideographs,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct OfflineRegionDefinition {
    pub data: crate::generated::OfflineRegionDefinitionData,
}

impl OfflineRegionDefinition {
    pub fn tile_pyramid(value: crate::generated::OfflineTilePyramidRegionDefinition) -> Self {
        Self {
            data: crate::generated::OfflineRegionDefinitionData::TilePyramid(value),
        }
    }
    pub fn geometry(value: crate::generated::OfflineGeometryRegionDefinition) -> Self {
        Self {
            data: crate::generated::OfflineRegionDefinitionData::Geometry(value),
        }
    }
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_offline_region_definition> {
        let mut raw: maplibre_native_ffi_sys::mln_offline_region_definition =
            unsafe { std::mem::zeroed() };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_offline_region_definition>() as _;
        match &self.data {
            crate::generated::OfflineRegionDefinitionData::TilePyramid(item) => {
                raw.type_ = maplibre_native_ffi_sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID;
                raw.data.tile_pyramid = (item).to_native(arena)?;
            }
            crate::generated::OfflineRegionDefinitionData::Geometry(item) => {
                raw.type_ = maplibre_native_ffi_sys::MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
                raw.data.geometry = (item).to_native(arena)?;
            }
            crate::generated::OfflineRegionDefinitionData::Unknown(_) => {
                return Err(crate::Error::invalid_argument(
                    "unknown union variant cannot be submitted",
                ));
            }
        }
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_offline_region_definition,
    ) -> crate::Result<Self> {
        Ok(Self {
            data: match raw.type_ {
                maplibre_native_ffi_sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID => {
                    crate::generated::OfflineRegionDefinitionData::TilePyramid(unsafe {
                        crate::generated::OfflineTilePyramidRegionDefinition::from_native(unsafe {
                            raw.data.tile_pyramid
                        })
                    }?)
                }
                maplibre_native_ffi_sys::MLN_OFFLINE_REGION_DEFINITION_GEOMETRY => {
                    crate::generated::OfflineRegionDefinitionData::Geometry(unsafe {
                        crate::generated::OfflineGeometryRegionDefinition::from_native(unsafe {
                            raw.data.geometry
                        })
                    }?)
                }
                tag => crate::generated::OfflineRegionDefinitionData::Unknown(tag as u32),
            },
        })
    }
}

#[derive(Debug, Clone, PartialEq)]
pub enum OfflineRegionDefinitionData {
    TilePyramid(crate::generated::OfflineTilePyramidRegionDefinition),
    Geometry(crate::generated::OfflineGeometryRegionDefinition),
    Unknown(u32),
}
impl Default for OfflineRegionDefinitionData {
    fn default() -> Self {
        Self::Unknown(0)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum OfflineRegionDefinitionType {
    Geometry,
    TilePyramid,
    Unknown(u32),
}
impl Default for OfflineRegionDefinitionType {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl OfflineRegionDefinitionType {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            2 => Self::Geometry,
            1 => Self::TilePyramid,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Geometry => 2,
            Self::TilePyramid => 1,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum OfflineRegionDownloadState {
    Active,
    Inactive,
    Unknown(u32),
}
impl Default for OfflineRegionDownloadState {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl OfflineRegionDownloadState {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::Active,
            0 => Self::Inactive,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Active => 1,
            Self::Inactive => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct OfflineRegionInfo {
    pub id: i64,
    pub definition: crate::generated::OfflineRegionDefinition,
    pub metadata: Vec<u8>,
}

impl OfflineRegionInfo {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_offline_region_info> {
        let mut raw: maplibre_native_ffi_sys::mln_offline_region_info =
            unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_offline_region_info>() as _;
        raw.id = *(&self.id);
        raw.definition = (&self.definition).to_native(arena)?;
        raw.metadata = (&self.metadata).as_ptr().cast();
        raw.metadata_size = self
            .metadata
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_offline_region_info,
    ) -> crate::Result<Self> {
        Ok(Self {
            id: raw.id,
            definition: unsafe {
                crate::generated::OfflineRegionDefinition::from_native(raw.definition)
            }?,
            metadata: unsafe {
                crate::string::copy_string_view_bytes(maplibre_native_ffi_sys::mln_buffer_view {
                    data: raw.metadata.cast(),
                    size: raw.metadata_size as usize,
                })
            }?,
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct OfflineRegionStatus {
    pub download_state: crate::generated::OfflineRegionDownloadState,
    pub completed_resource_count: u64,
    pub completed_resource_size: u64,
    pub completed_tile_count: u64,
    pub required_tile_count: u64,
    pub completed_tile_size: u64,
    pub required_resource_count: u64,
    pub required_resource_count_is_precise: bool,
    pub complete: bool,
}

impl OfflineRegionStatus {
    pub const fn new(
        download_state: crate::generated::OfflineRegionDownloadState,
        completed_resource_count: u64,
        completed_resource_size: u64,
        completed_tile_count: u64,
        required_tile_count: u64,
        completed_tile_size: u64,
        required_resource_count: u64,
        required_resource_count_is_precise: bool,
        complete: bool,
    ) -> Self {
        Self {
            download_state,
            completed_resource_count,
            completed_resource_size,
            completed_tile_count,
            required_tile_count,
            completed_tile_size,
            required_resource_count,
            required_resource_count_is_precise,
            complete,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_offline_region_status {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_offline_region_status>() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_offline_region_status>() as _;
        raw.download_state = self.download_state.to_native();
        raw.completed_resource_count = self.completed_resource_count;
        raw.completed_resource_size = self.completed_resource_size;
        raw.completed_tile_count = self.completed_tile_count;
        raw.required_tile_count = self.required_tile_count;
        raw.completed_tile_size = self.completed_tile_size;
        raw.required_resource_count = self.required_resource_count;
        raw.required_resource_count_is_precise = self.required_resource_count_is_precise;
        raw.complete = self.complete;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_offline_region_status) -> Self {
        Self {
            download_state: crate::generated::OfflineRegionDownloadState::from_native(
                raw.download_state,
            ),
            completed_resource_count: raw.completed_resource_count,
            completed_resource_size: raw.completed_resource_size,
            completed_tile_count: raw.completed_tile_count,
            required_tile_count: raw.required_tile_count,
            completed_tile_size: raw.completed_tile_size,
            required_resource_count: raw.required_resource_count,
            required_resource_count_is_precise: raw.required_resource_count_is_precise,
            complete: raw.complete,
        }
    }
}
impl crate::values::NativeValue for OfflineRegionStatus {
    type Raw = maplibre_native_ffi_sys::mln_offline_region_status;
    fn to_native(self) -> Self::Raw {
        OfflineRegionStatus::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct OfflineTilePyramidRegionDefinition {
    pub style_url: String,
    pub bounds: crate::generated::LatLngBounds,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub pixel_ratio: f32,
    pub include_ideographs: bool,
}

impl OfflineTilePyramidRegionDefinition {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_offline_tile_pyramid_region_definition> {
        let mut raw: maplibre_native_ffi_sys::mln_offline_tile_pyramid_region_definition =
            unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<
            maplibre_native_ffi_sys::mln_offline_tile_pyramid_region_definition,
        >() as _;
        raw.style_url = arena.c_string((&self.style_url))?;
        raw.bounds = (&self.bounds).to_native();
        raw.min_zoom = *(&self.min_zoom);
        raw.max_zoom = *(&self.max_zoom);
        raw.pixel_ratio = *(&self.pixel_ratio);
        raw.include_ideographs = *(&self.include_ideographs);
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_offline_tile_pyramid_region_definition,
    ) -> crate::Result<Self> {
        Ok(Self {
            style_url: unsafe { crate::string::copy_c_string(raw.style_url) }?,
            bounds: crate::generated::LatLngBounds::from_native(raw.bounds),
            min_zoom: raw.min_zoom,
            max_zoom: raw.max_zoom,
            pixel_ratio: raw.pixel_ratio,
            include_ideographs: raw.include_ideographs,
        })
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct OpenglBorrowedTextureDescriptor {
    pub extent: crate::generated::RenderTargetExtent,
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: crate::generated::OpenglContextDescriptor,
    pub texture: u32,
    pub target: u32,
}
impl Default for OpenglBorrowedTextureDescriptor {
    fn default() -> Self {
        unsafe {
            Self::from_native(
                maplibre_native_ffi_sys::mln_opengl_borrowed_texture_descriptor_default(),
            )
            .expect("native default must be valid")
        }
    }
}
impl OpenglBorrowedTextureDescriptor {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_opengl_borrowed_texture_descriptor> {
        let mut raw: maplibre_native_ffi_sys::mln_opengl_borrowed_texture_descriptor =
            unsafe { maplibre_native_ffi_sys::mln_opengl_borrowed_texture_descriptor_default() };
        raw.size = std::mem::size_of::<
            maplibre_native_ffi_sys::mln_opengl_borrowed_texture_descriptor,
        >() as _;
        raw.extent = (&self.extent).to_native();
        raw.physical_width = *(&self.physical_width);
        raw.physical_height = *(&self.physical_height);
        raw.context = (&self.context).to_native(arena)?;
        raw.texture = *(&self.texture);
        raw.target = *(&self.target);
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_opengl_borrowed_texture_descriptor,
    ) -> crate::Result<Self> {
        Ok(Self {
            extent: crate::generated::RenderTargetExtent::from_native(raw.extent),
            physical_width: raw.physical_width,
            physical_height: raw.physical_height,
            context: unsafe {
                crate::generated::OpenglContextDescriptor::from_native(raw.context)
            }?,
            texture: raw.texture,
            target: raw.target,
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum OpenglClientApi {
    Gles,
    Gl,
    Unspecified,
    Unknown(u32),
}
impl Default for OpenglClientApi {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl OpenglClientApi {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            2 => Self::Gles,
            1 => Self::Gl,
            0 => Self::Unspecified,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Gles => 2,
            Self::Gl => 1,
            Self::Unspecified => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct OpenglContextDescriptor {
    pub ownership: crate::generated::OpenglContextOwnership,
    pub data: crate::generated::OpenglContextDescriptorData,
}

impl OpenglContextDescriptor {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_opengl_context_descriptor> {
        let mut raw: maplibre_native_ffi_sys::mln_opengl_context_descriptor =
            unsafe { std::mem::zeroed() };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_opengl_context_descriptor>() as _;
        raw.ownership = (&self.ownership).to_native();
        match &self.data {
            crate::generated::OpenglContextDescriptorData::Wgl(item) => {
                raw.platform = maplibre_native_ffi_sys::MLN_OPENGL_CONTEXT_PLATFORM_WGL;
                raw.data.wgl = (item).to_native();
            }
            crate::generated::OpenglContextDescriptorData::Egl(item) => {
                raw.platform = maplibre_native_ffi_sys::MLN_OPENGL_CONTEXT_PLATFORM_EGL;
                raw.data.egl = (item).to_native();
            }
            crate::generated::OpenglContextDescriptorData::Webgl(item) => {
                raw.platform = maplibre_native_ffi_sys::MLN_OPENGL_CONTEXT_PLATFORM_WEBGL;
                raw.data.webgl = (item).to_native(arena)?;
            }
            crate::generated::OpenglContextDescriptorData::Unknown(_) => {
                return Err(crate::Error::invalid_argument(
                    "unknown union variant cannot be submitted",
                ));
            }
        }
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_opengl_context_descriptor,
    ) -> crate::Result<Self> {
        Ok(Self {
            ownership: crate::generated::OpenglContextOwnership::from_native(raw.ownership),
            data: match raw.platform {
                maplibre_native_ffi_sys::MLN_OPENGL_CONTEXT_PLATFORM_WGL => {
                    crate::generated::OpenglContextDescriptorData::Wgl(
                        crate::generated::WglContextDescriptor::from_native(unsafe {
                            raw.data.wgl
                        }),
                    )
                }
                maplibre_native_ffi_sys::MLN_OPENGL_CONTEXT_PLATFORM_EGL => {
                    crate::generated::OpenglContextDescriptorData::Egl(
                        crate::generated::EglContextDescriptor::from_native(unsafe {
                            raw.data.egl
                        }),
                    )
                }
                maplibre_native_ffi_sys::MLN_OPENGL_CONTEXT_PLATFORM_WEBGL => {
                    crate::generated::OpenglContextDescriptorData::Webgl(unsafe {
                        crate::generated::WebglContextDescriptor::from_native(unsafe {
                            raw.data.webgl
                        })
                    }?)
                }
                tag => crate::generated::OpenglContextDescriptorData::Unknown(tag as u32),
            },
        })
    }
}

#[derive(Debug, Clone, PartialEq)]
pub enum OpenglContextDescriptorData {
    Wgl(crate::generated::WglContextDescriptor),
    Egl(crate::generated::EglContextDescriptor),
    Webgl(crate::generated::WebglContextDescriptor),
    Unknown(u32),
}
impl Default for OpenglContextDescriptorData {
    fn default() -> Self {
        Self::Unknown(0)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum OpenglContextOwnership {
    Dedicated,
    Shared,
    Unknown(u32),
}
impl Default for OpenglContextOwnership {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl OpenglContextOwnership {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::Dedicated,
            0 => Self::Shared,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Dedicated => 1,
            Self::Shared => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum OpenglContextPlatform {
    Webgl,
    Egl,
    Wgl,
    Unspecified,
    Unknown(u32),
}
impl Default for OpenglContextPlatform {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl OpenglContextPlatform {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            3 => Self::Webgl,
            2 => Self::Egl,
            1 => Self::Wgl,
            0 => Self::Unspecified,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Webgl => 3,
            Self::Egl => 2,
            Self::Wgl => 1,
            Self::Unspecified => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct OpenglContextProviderFlag: u32 {
        const WGL = 1;
        const EGL = 2;
        const WEBGL = 4;
        const _ = !0;
} }
impl OpenglContextProviderFlag {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct OpenglOwnedTextureDescriptor {
    pub extent: crate::generated::RenderTargetExtent,
    pub context: crate::generated::OpenglContextDescriptor,
}
impl Default for OpenglOwnedTextureDescriptor {
    fn default() -> Self {
        unsafe {
            Self::from_native(
                maplibre_native_ffi_sys::mln_opengl_owned_texture_descriptor_default(),
            )
            .expect("native default must be valid")
        }
    }
}
impl OpenglOwnedTextureDescriptor {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_opengl_owned_texture_descriptor> {
        let mut raw: maplibre_native_ffi_sys::mln_opengl_owned_texture_descriptor =
            unsafe { maplibre_native_ffi_sys::mln_opengl_owned_texture_descriptor_default() };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_opengl_owned_texture_descriptor>()
                as _;
        raw.extent = (&self.extent).to_native();
        raw.context = (&self.context).to_native(arena)?;
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_opengl_owned_texture_descriptor,
    ) -> crate::Result<Self> {
        Ok(Self {
            extent: crate::generated::RenderTargetExtent::from_native(raw.extent),
            context: unsafe {
                crate::generated::OpenglContextDescriptor::from_native(raw.context)
            }?,
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct OpenglOwnedTextureFrame {
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub texture: u32,
    pub target: u32,
    pub internal_format: u32,
    pub format: u32,
    pub r#type: u32,
}

impl OpenglOwnedTextureFrame {
    pub const fn new(
        generation: u64,
        width: u32,
        height: u32,
        scale_factor: f64,
        frame_id: u64,
        texture: u32,
        target: u32,
        internal_format: u32,
        format: u32,
        r#type: u32,
    ) -> Self {
        Self {
            generation,
            width,
            height,
            scale_factor,
            frame_id,
            texture,
            target,
            internal_format,
            format,
            r#type,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_opengl_owned_texture_frame {
        let mut raw = unsafe {
            std::mem::zeroed::<maplibre_native_ffi_sys::mln_opengl_owned_texture_frame>()
        };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_opengl_owned_texture_frame>() as _;
        raw.generation = self.generation;
        raw.width = self.width;
        raw.height = self.height;
        raw.scale_factor = self.scale_factor;
        raw.frame_id = self.frame_id;
        raw.texture = self.texture;
        raw.target = self.target;
        raw.internal_format = self.internal_format;
        raw.format = self.format;
        raw.type_ = self.r#type;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_opengl_owned_texture_frame) -> Self {
        Self {
            generation: raw.generation,
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: raw.frame_id,
            texture: raw.texture,
            target: raw.target,
            internal_format: raw.internal_format,
            format: raw.format,
            r#type: raw.type_,
        }
    }
}
impl crate::values::NativeValue for OpenglOwnedTextureFrame {
    type Raw = maplibre_native_ffi_sys::mln_opengl_owned_texture_frame;
    fn to_native(self) -> Self::Raw {
        OpenglOwnedTextureFrame::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct OpenglSurfaceDescriptor {
    pub extent: crate::generated::RenderTargetExtent,
    pub context: crate::generated::OpenglContextDescriptor,
    pub surface: *mut std::ffi::c_void,
}
impl Default for OpenglSurfaceDescriptor {
    fn default() -> Self {
        unsafe {
            Self::from_native(maplibre_native_ffi_sys::mln_opengl_surface_descriptor_default())
                .expect("native default must be valid")
        }
    }
}
impl OpenglSurfaceDescriptor {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_opengl_surface_descriptor> {
        let mut raw: maplibre_native_ffi_sys::mln_opengl_surface_descriptor =
            unsafe { maplibre_native_ffi_sys::mln_opengl_surface_descriptor_default() };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_opengl_surface_descriptor>() as _;
        raw.extent = (&self.extent).to_native();
        raw.context = (&self.context).to_native(arena)?;
        raw.surface = *(&self.surface);
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_opengl_surface_descriptor,
    ) -> crate::Result<Self> {
        Ok(Self {
            extent: crate::generated::RenderTargetExtent::from_native(raw.extent),
            context: unsafe {
                crate::generated::OpenglContextDescriptor::from_native(raw.context)
            }?,
            surface: raw.surface,
        })
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct PremultipliedRgba8Image {
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub pixels: Vec<u8>,
}
impl Default for PremultipliedRgba8Image {
    fn default() -> Self {
        unsafe {
            Self::from_native(maplibre_native_ffi_sys::mln_premultiplied_rgba8_image_default())
                .expect("native default must be valid")
        }
    }
}
impl PremultipliedRgba8Image {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_premultiplied_rgba8_image> {
        let mut raw: maplibre_native_ffi_sys::mln_premultiplied_rgba8_image =
            unsafe { maplibre_native_ffi_sys::mln_premultiplied_rgba8_image_default() };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_premultiplied_rgba8_image>() as _;
        raw.width = *(&self.width);
        raw.height = *(&self.height);
        raw.stride = *(&self.stride);
        raw.pixels = (&self.pixels).as_ptr().cast();
        raw.byte_length = self
            .pixels
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_premultiplied_rgba8_image,
    ) -> crate::Result<Self> {
        Ok(Self {
            width: raw.width,
            height: raw.height,
            stride: raw.stride,
            pixels: unsafe {
                crate::string::copy_string_view_bytes(maplibre_native_ffi_sys::mln_buffer_view {
                    data: raw.pixels.cast(),
                    size: raw.byte_length as usize,
                })
            }?,
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct ProjectedMeters {
    pub northing: f64,
    pub easting: f64,
}

impl ProjectedMeters {
    pub const fn new(northing: f64, easting: f64) -> Self {
        Self { northing, easting }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_projected_meters {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_projected_meters>() };
        raw.northing = self.northing;
        raw.easting = self.easting;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_projected_meters) -> Self {
        Self {
            northing: raw.northing,
            easting: raw.easting,
        }
    }
}
impl crate::values::NativeValue for ProjectedMeters {
    type Raw = maplibre_native_ffi_sys::mln_projected_meters;
    fn to_native(self) -> Self::Raw {
        ProjectedMeters::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct ProjectionMode {
    pub axonometric: Option<bool>,
    pub x_skew: Option<f64>,
    pub y_skew: Option<f64>,
}
impl Default for ProjectionMode {
    fn default() -> Self {
        Self::from_native(unsafe { maplibre_native_ffi_sys::mln_projection_mode_default() })
    }
}
impl ProjectionMode {
    #[doc(hidden)]
    pub fn to_native(&self) -> maplibre_native_ffi_sys::mln_projection_mode {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_projection_mode_default() };
        raw.fields = 0;
        if let Some(value) = self.axonometric {
            raw.fields |= maplibre_native_ffi_sys::MLN_PROJECTION_MODE_AXONOMETRIC;
            raw.axonometric = value;
        }
        if let Some(value) = self.x_skew {
            raw.fields |= maplibre_native_ffi_sys::MLN_PROJECTION_MODE_X_SKEW;
            raw.x_skew = value;
        }
        if let Some(value) = self.y_skew {
            raw.fields |= maplibre_native_ffi_sys::MLN_PROJECTION_MODE_Y_SKEW;
            raw.y_skew = value;
        }
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_projection_mode) -> Self {
        Self {
            axonometric: (raw.fields & maplibre_native_ffi_sys::MLN_PROJECTION_MODE_AXONOMETRIC
                != 0)
                .then(|| raw.axonometric),
            x_skew: (raw.fields & maplibre_native_ffi_sys::MLN_PROJECTION_MODE_X_SKEW != 0)
                .then(|| raw.x_skew),
            y_skew: (raw.fields & maplibre_native_ffi_sys::MLN_PROJECTION_MODE_Y_SKEW != 0)
                .then(|| raw.y_skew),
        }
    }
}
impl crate::values::NativeValue for ProjectionMode {
    type Raw = maplibre_native_ffi_sys::mln_projection_mode;
    fn to_native(self) -> Self::Raw {
        ProjectionMode::to_native(&self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct ProjectionModeField: u32 {
        const AXONOMETRIC = 1;
        const X_SKEW = 2;
        const Y_SKEW = 4;
        const _ = !0;
} }
impl ProjectionModeField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Quaternion {
    pub x: f64,
    pub y: f64,
    pub z: f64,
    pub w: f64,
}

impl Quaternion {
    pub const fn new(x: f64, y: f64, z: f64, w: f64) -> Self {
        Self { x, y, z, w }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_quaternion {
        let mut raw = unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_quaternion>() };
        raw.x = self.x;
        raw.y = self.y;
        raw.z = self.z;
        raw.w = self.w;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_quaternion) -> Self {
        Self {
            x: raw.x,
            y: raw.y,
            z: raw.z,
            w: raw.w,
        }
    }
}
impl crate::values::NativeValue for Quaternion {
    type Raw = maplibre_native_ffi_sys::mln_quaternion;
    fn to_native(self) -> Self::Raw {
        Quaternion::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct QueriedFeature {
    pub feature: Vec<u8>,
    pub source_id: Option<String>,
    pub source_layer_id: Option<String>,
    pub state: Option<Vec<u8>>,
}

impl QueriedFeature {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_queried_feature> {
        let mut raw: maplibre_native_ffi_sys::mln_queried_feature = unsafe { std::mem::zeroed() };
        raw.fields = 0;
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_queried_feature>() as _;
        raw.feature = maplibre_native_ffi_sys::mln_buffer_view {
            data: (&self.feature).as_ptr().cast(),
            size: (&self.feature).len(),
        };
        if let Some(item) = &self.source_id {
            raw.fields |= maplibre_native_ffi_sys::MLN_QUERIED_FEATURE_SOURCE_ID;
            raw.source_id = maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            };
        }
        if let Some(item) = &self.source_layer_id {
            raw.fields |= maplibre_native_ffi_sys::MLN_QUERIED_FEATURE_SOURCE_LAYER_ID;
            raw.source_layer_id = maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            };
        }
        if let Some(item) = &self.state {
            raw.fields |= maplibre_native_ffi_sys::MLN_QUERIED_FEATURE_STATE;
            raw.state = maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_ptr().cast(),
                size: (item).len(),
            };
        }
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_queried_feature,
    ) -> crate::Result<Self> {
        Ok(Self {
            feature: unsafe { crate::string::copy_string_view_bytes(raw.feature) }?,
            source_id: if raw.fields & maplibre_native_ffi_sys::MLN_QUERIED_FEATURE_SOURCE_ID != 0 {
                Some(unsafe { crate::string::copy_string_view(raw.source_id) }?)
            } else {
                None
            },
            source_layer_id: if raw.fields
                & maplibre_native_ffi_sys::MLN_QUERIED_FEATURE_SOURCE_LAYER_ID
                != 0
            {
                Some(unsafe { crate::string::copy_string_view(raw.source_layer_id) }?)
            } else {
                None
            },
            state: if raw.fields & maplibre_native_ffi_sys::MLN_QUERIED_FEATURE_STATE != 0 {
                Some(unsafe { crate::string::copy_string_view_bytes(raw.state) }?)
            } else {
                None
            },
        })
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct QueriedFeatureField: u32 {
        const SOURCE_ID = 1;
        const SOURCE_LAYER_ID = 2;
        const STATE = 4;
        const _ = !0;
} }
impl QueriedFeatureField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum RenderAbandonDisposition {
    Quarantined,
    Clean,
    Unknown(u32),
}
impl Default for RenderAbandonDisposition {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl RenderAbandonDisposition {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::Quarantined,
            0 => Self::Clean,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Quarantined => 1,
            Self::Clean => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RenderAbandonResult {
    pub disposition: crate::generated::RenderAbandonDisposition,
    pub quarantined_resource_count: u32,
}

impl RenderAbandonResult {
    pub const fn new(
        disposition: crate::generated::RenderAbandonDisposition,
        quarantined_resource_count: u32,
    ) -> Self {
        Self {
            disposition,
            quarantined_resource_count,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_render_abandon_result {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_render_abandon_result>() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_render_abandon_result>() as _;
        raw.disposition = self.disposition.to_native();
        raw.quarantined_resource_count = self.quarantined_resource_count;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_render_abandon_result) -> Self {
        Self {
            disposition: crate::generated::RenderAbandonDisposition::from_native(raw.disposition),
            quarantined_resource_count: raw.quarantined_resource_count,
        }
    }
}
impl crate::values::NativeValue for RenderAbandonResult {
    type Raw = maplibre_native_ffi_sys::mln_render_abandon_result;
    fn to_native(self) -> Self::Raw {
        RenderAbandonResult::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct RenderBackendFlag: u32 {
        const METAL = 1;
        const VULKAN = 2;
        const OPENGL = 4;
        const WEBGPU = 8;
        const _ = !0;
} }
impl RenderBackendFlag {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum RenderDriverKind {
    CallerGraphicsThread,
    CoreWorker,
    Unknown(u32),
}
impl Default for RenderDriverKind {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl RenderDriverKind {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            2 => Self::CallerGraphicsThread,
            1 => Self::CoreWorker,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::CallerGraphicsThread => 2,
            Self::CoreWorker => 1,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RenderFrameResult {
    pub disposition: crate::generated::RenderResult,
    pub token: u64,
    pub map_update_generation: u64,
    pub extent_generation: u64,
    pub frame_generation: u64,
    pub needs_repaint: bool,
}

impl RenderFrameResult {
    pub const fn new(
        disposition: crate::generated::RenderResult,
        token: u64,
        map_update_generation: u64,
        extent_generation: u64,
        frame_generation: u64,
        needs_repaint: bool,
    ) -> Self {
        Self {
            disposition,
            token,
            map_update_generation,
            extent_generation,
            frame_generation,
            needs_repaint,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_render_frame_result {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_render_frame_result>() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_render_frame_result>() as _;
        raw.disposition = self.disposition.to_native();
        raw.token = self.token;
        raw.map_update_generation = self.map_update_generation;
        raw.extent_generation = self.extent_generation;
        raw.frame_generation = self.frame_generation;
        raw.needs_repaint = self.needs_repaint;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_render_frame_result) -> Self {
        Self {
            disposition: crate::generated::RenderResult::from_native(raw.disposition),
            token: raw.token,
            map_update_generation: raw.map_update_generation,
            extent_generation: raw.extent_generation,
            frame_generation: raw.frame_generation,
            needs_repaint: raw.needs_repaint,
        }
    }
}
impl crate::values::NativeValue for RenderFrameResult {
    type Raw = maplibre_native_ffi_sys::mln_render_frame_result;
    fn to_native(self) -> Self::Raw {
        RenderFrameResult::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum RenderMode {
    Full,
    Partial,
    Unknown(u32),
}
impl Default for RenderMode {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl RenderMode {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::Full,
            0 => Self::Partial,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Full => 1,
            Self::Partial => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum RenderResult {
    DeadlineMissed,
    Superseded,
    TargetNotReady,
    SizePending,
    NoUpdate,
    Rendered,
    Unknown(u32),
}
impl Default for RenderResult {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl RenderResult {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            5 => Self::DeadlineMissed,
            4 => Self::Superseded,
            3 => Self::TargetNotReady,
            2 => Self::SizePending,
            1 => Self::NoUpdate,
            0 => Self::Rendered,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::DeadlineMissed => 5,
            Self::Superseded => 4,
            Self::TargetNotReady => 3,
            Self::SizePending => 2,
            Self::NoUpdate => 1,
            Self::Rendered => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone)]
pub struct RenderSessionAttachOptions {
    pub driver: crate::generated::RenderDriverKind,
    pub requested_texture_ring_depth: u32,
    pub frame_wake: crate::generated::Wake,
    pub driver_work_wake: crate::generated::Wake,
}
impl Default for RenderSessionAttachOptions {
    fn default() -> Self {
        unsafe {
            Self::from_native(maplibre_native_ffi_sys::mln_render_session_attach_options_default())
                .expect("native default must be valid")
        }
    }
}
impl RenderSessionAttachOptions {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_render_session_attach_options> {
        let mut raw: maplibre_native_ffi_sys::mln_render_session_attach_options =
            unsafe { maplibre_native_ffi_sys::mln_render_session_attach_options_default() };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_render_session_attach_options>() as _;
        raw.driver = (&self.driver).to_native();
        raw.requested_texture_ring_depth = *(&self.requested_texture_ring_depth);
        raw.frame_wake = (&self.frame_wake).to_native(arena)?;
        raw.driver_work_wake = (&self.driver_work_wake).to_native(arena)?;
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_render_session_attach_options,
    ) -> crate::Result<Self> {
        Ok(Self {
            driver: crate::generated::RenderDriverKind::from_native(raw.driver),
            requested_texture_ring_depth: raw.requested_texture_ring_depth,
            frame_wake: unsafe { crate::generated::Wake::from_native(raw.frame_wake) }?,
            driver_work_wake: unsafe { crate::generated::Wake::from_native(raw.driver_work_wake) }?,
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RenderSessionCapabilities {
    pub driver: crate::generated::RenderDriverKind,
    pub texture_ring_depth: u32,
    pub flags: crate::generated::RenderSessionCapabilityFlag,
}

impl RenderSessionCapabilities {
    pub const fn new(
        driver: crate::generated::RenderDriverKind,
        texture_ring_depth: u32,
        flags: crate::generated::RenderSessionCapabilityFlag,
    ) -> Self {
        Self {
            driver,
            texture_ring_depth,
            flags,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_render_session_capabilities {
        let mut raw = unsafe {
            std::mem::zeroed::<maplibre_native_ffi_sys::mln_render_session_capabilities>()
        };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_render_session_capabilities>() as _;
        raw.driver = self.driver.to_native();
        raw.texture_ring_depth = self.texture_ring_depth;
        raw.flags = self.flags.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_render_session_capabilities) -> Self {
        Self {
            driver: crate::generated::RenderDriverKind::from_native(raw.driver),
            texture_ring_depth: raw.texture_ring_depth,
            flags: crate::generated::RenderSessionCapabilityFlag::from_native(raw.flags),
        }
    }
}
impl crate::values::NativeValue for RenderSessionCapabilities {
    type Raw = maplibre_native_ffi_sys::mln_render_session_capabilities;
    fn to_native(self) -> Self::Raw {
        RenderSessionCapabilities::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct RenderSessionCapabilityFlag: u32 {
        const FRAME_ACQUISITION = 1;
        const READBACK = 2;
        const CONSUMER_SYNC = 4;
        const PRESENTATION = 8;
        const _ = !0;
} }
impl RenderSessionCapabilityFlag {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RenderSessionSnapshot {
    pub state: crate::generated::RenderSessionState,
    pub driver: crate::generated::RenderDriverKind,
    pub latest_result: crate::generated::RenderResult,
    pub extent: crate::generated::RenderTargetExtent,
    pub generation: u64,
    pub map_update_generation: u64,
    pub rendered_update_generation: u64,
    pub extent_generation: u64,
    pub frame_generation: u64,
    pub latest_demand_token: u64,
    pub pending_demand_count: u32,
    pub acquired_frame_count: u32,
    pub target_ready: bool,
    pub pending_changes: bool,
}

impl RenderSessionSnapshot {
    pub const fn new(
        state: crate::generated::RenderSessionState,
        driver: crate::generated::RenderDriverKind,
        latest_result: crate::generated::RenderResult,
        extent: crate::generated::RenderTargetExtent,
        generation: u64,
        map_update_generation: u64,
        rendered_update_generation: u64,
        extent_generation: u64,
        frame_generation: u64,
        latest_demand_token: u64,
        pending_demand_count: u32,
        acquired_frame_count: u32,
        target_ready: bool,
        pending_changes: bool,
    ) -> Self {
        Self {
            state,
            driver,
            latest_result,
            extent,
            generation,
            map_update_generation,
            rendered_update_generation,
            extent_generation,
            frame_generation,
            latest_demand_token,
            pending_demand_count,
            acquired_frame_count,
            target_ready,
            pending_changes,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_render_session_snapshot {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_render_session_snapshot>() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_render_session_snapshot>() as _;
        raw.state = self.state.to_native();
        raw.driver = self.driver.to_native();
        raw.latest_result = self.latest_result.to_native();
        raw.extent = self.extent.to_native();
        raw.generation = self.generation;
        raw.map_update_generation = self.map_update_generation;
        raw.rendered_update_generation = self.rendered_update_generation;
        raw.extent_generation = self.extent_generation;
        raw.frame_generation = self.frame_generation;
        raw.latest_demand_token = self.latest_demand_token;
        raw.pending_demand_count = self.pending_demand_count;
        raw.acquired_frame_count = self.acquired_frame_count;
        raw.target_ready = self.target_ready;
        raw.pending_changes = self.pending_changes;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_render_session_snapshot) -> Self {
        Self {
            state: crate::generated::RenderSessionState::from_native(raw.state),
            driver: crate::generated::RenderDriverKind::from_native(raw.driver),
            latest_result: crate::generated::RenderResult::from_native(raw.latest_result),
            extent: crate::generated::RenderTargetExtent::from_native(raw.extent),
            generation: raw.generation,
            map_update_generation: raw.map_update_generation,
            rendered_update_generation: raw.rendered_update_generation,
            extent_generation: raw.extent_generation,
            frame_generation: raw.frame_generation,
            latest_demand_token: raw.latest_demand_token,
            pending_demand_count: raw.pending_demand_count,
            acquired_frame_count: raw.acquired_frame_count,
            target_ready: raw.target_ready,
            pending_changes: raw.pending_changes,
        }
    }
}
impl crate::values::NativeValue for RenderSessionSnapshot {
    type Raw = maplibre_native_ffi_sys::mln_render_session_snapshot;
    fn to_native(self) -> Self::Raw {
        RenderSessionSnapshot::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum RenderSessionState {
    Abandoned,
    TargetLost,
    Detached,
    Detaching,
    Attached,
    Attaching,
    Unknown(u32),
}
impl Default for RenderSessionState {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl RenderSessionState {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            6 => Self::Abandoned,
            5 => Self::TargetLost,
            4 => Self::Detached,
            3 => Self::Detaching,
            2 => Self::Attached,
            1 => Self::Attaching,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Abandoned => 6,
            Self::TargetLost => 5,
            Self::Detached => 4,
            Self::Detaching => 3,
            Self::Attached => 2,
            Self::Attaching => 1,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RenderTargetExtent {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
}

impl RenderTargetExtent {
    pub const fn new(width: u32, height: u32, scale_factor: f64) -> Self {
        Self {
            width,
            height,
            scale_factor,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_render_target_extent {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_render_target_extent>() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_render_target_extent>() as _;
        raw.width = self.width;
        raw.height = self.height;
        raw.scale_factor = self.scale_factor;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_render_target_extent) -> Self {
        Self {
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
        }
    }
}
impl crate::values::NativeValue for RenderTargetExtent {
    type Raw = maplibre_native_ffi_sys::mln_render_target_extent;
    fn to_native(self) -> Self::Raw {
        RenderTargetExtent::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct RenderedFeatureQueryOptionField: u32 {
        const IDS = 1;
        const _ = !0;
} }
impl RenderedFeatureQueryOptionField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct RenderedFeatureQueryOptions {
    pub layer_ids: Option<Vec<String>>,
    pub filter: Option<Vec<u8>>,
}
impl Default for RenderedFeatureQueryOptions {
    fn default() -> Self {
        unsafe {
            Self::from_native(maplibre_native_ffi_sys::mln_rendered_feature_query_options_default())
                .expect("native default must be valid")
        }
    }
}
impl RenderedFeatureQueryOptions {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_rendered_feature_query_options> {
        let mut raw: maplibre_native_ffi_sys::mln_rendered_feature_query_options =
            unsafe { maplibre_native_ffi_sys::mln_rendered_feature_query_options_default() };
        raw.fields = 0;
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_rendered_feature_query_options>() as _;
        if let Some(item) = &self.layer_ids {
            raw.fields |= maplibre_native_ffi_sys::MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
            raw.layer_ids = {
                let items = (item)
                    .iter()
                    .map(|item| -> crate::Result<_> {
                        Ok(maplibre_native_ffi_sys::mln_buffer_view {
                            data: (item).as_bytes().as_ptr().cast(),
                            size: (item).as_bytes().len(),
                        })
                    })
                    .collect::<crate::Result<Vec<_>>>()?;
                arena.array(items)
            };
        }
        raw.layer_id_count = self
            .layer_ids
            .as_ref()
            .map_or(0, |items| items.len())
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;
        raw.filter = match (&self.filter).as_ref() {
            Some(item) => {
                let item = maplibre_native_ffi_sys::mln_buffer_view {
                    data: (item).as_ptr().cast(),
                    size: (item).len(),
                };
                arena.store(item)
            }
            None => std::ptr::null(),
        };
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_rendered_feature_query_options,
    ) -> crate::Result<Self> {
        Ok(Self {
            layer_ids: if raw.fields
                & maplibre_native_ffi_sys::MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS
                != 0
            {
                Some(
                    unsafe { crate::input::slice(raw.layer_ids, raw.layer_id_count as usize) }?
                        .iter()
                        .map(|item| -> crate::Result<_> {
                            Ok(unsafe { crate::string::copy_string_view(*item) }?)
                        })
                        .collect::<crate::Result<Vec<_>>>()?,
                )
            } else {
                None
            },
            filter: if raw.filter.is_null() {
                None
            } else {
                Some({
                    let item = unsafe { raw.filter.as_ref() }
                        .ok_or_else(|| crate::Error::invalid_argument("null record value"))?;
                    unsafe { crate::string::copy_string_view_bytes(*item) }?
                })
            },
        })
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct RenderedQueryGeometry {
    pub data: crate::generated::RenderedQueryGeometryData,
}

impl RenderedQueryGeometry {
    pub fn point(value: crate::generated::ScreenPoint) -> Self {
        Self {
            data: crate::generated::RenderedQueryGeometryData::Point(value),
        }
    }
    pub fn box_(value: crate::generated::ScreenBox) -> Self {
        Self {
            data: crate::generated::RenderedQueryGeometryData::Box(value),
        }
    }
    pub fn line_string(value: crate::generated::ScreenLineString) -> Self {
        Self {
            data: crate::generated::RenderedQueryGeometryData::LineString(value),
        }
    }
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_rendered_query_geometry> {
        let mut raw: maplibre_native_ffi_sys::mln_rendered_query_geometry =
            unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_rendered_query_geometry>() as _;
        match &self.data {
            crate::generated::RenderedQueryGeometryData::Point(item) => {
                raw.type_ = maplibre_native_ffi_sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT;
                raw.data.point = (item).to_native();
            }
            crate::generated::RenderedQueryGeometryData::Box(item) => {
                raw.type_ = maplibre_native_ffi_sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX;
                raw.data.box_ = (item).to_native();
            }
            crate::generated::RenderedQueryGeometryData::LineString(item) => {
                raw.type_ = maplibre_native_ffi_sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING;
                raw.data.line_string = (item).to_native(arena)?;
            }
            crate::generated::RenderedQueryGeometryData::Unknown(_) => {
                return Err(crate::Error::invalid_argument(
                    "unknown union variant cannot be submitted",
                ));
            }
        }
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_rendered_query_geometry,
    ) -> crate::Result<Self> {
        Ok(Self {
            data: match raw.type_ {
                maplibre_native_ffi_sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT => {
                    crate::generated::RenderedQueryGeometryData::Point(
                        crate::generated::ScreenPoint::from_native(unsafe { raw.data.point }),
                    )
                }
                maplibre_native_ffi_sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX => {
                    crate::generated::RenderedQueryGeometryData::Box(
                        crate::generated::ScreenBox::from_native(unsafe { raw.data.box_ }),
                    )
                }
                maplibre_native_ffi_sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING => {
                    crate::generated::RenderedQueryGeometryData::LineString(unsafe {
                        crate::generated::ScreenLineString::from_native(unsafe {
                            raw.data.line_string
                        })
                    }?)
                }
                tag => crate::generated::RenderedQueryGeometryData::Unknown(tag as u32),
            },
        })
    }
}

#[derive(Debug, Clone, PartialEq)]
pub enum RenderedQueryGeometryData {
    Point(crate::generated::ScreenPoint),
    Box(crate::generated::ScreenBox),
    LineString(crate::generated::ScreenLineString),
    Unknown(u32),
}
impl Default for RenderedQueryGeometryData {
    fn default() -> Self {
        Self::Unknown(0)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum RenderedQueryGeometryType {
    LineString,
    Box,
    Point,
    Unknown(u32),
}
impl Default for RenderedQueryGeometryType {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl RenderedQueryGeometryType {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            3 => Self::LineString,
            2 => Self::Box,
            1 => Self::Point,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::LineString => 3,
            Self::Box => 2,
            Self::Point => 1,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RenderingStats {
    pub encoding_time: f64,
    pub rendering_time: f64,
    pub frame_count: i64,
    pub draw_call_count: i64,
    pub total_draw_call_count: i64,
}

impl RenderingStats {
    pub const fn new(
        encoding_time: f64,
        rendering_time: f64,
        frame_count: i64,
        draw_call_count: i64,
        total_draw_call_count: i64,
    ) -> Self {
        Self {
            encoding_time,
            rendering_time,
            frame_count,
            draw_call_count,
            total_draw_call_count,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_rendering_stats {
        let mut raw = unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_rendering_stats>() };
        raw.encoding_time = self.encoding_time;
        raw.rendering_time = self.rendering_time;
        raw.frame_count = self.frame_count;
        raw.draw_call_count = self.draw_call_count;
        raw.total_draw_call_count = self.total_draw_call_count;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_rendering_stats) -> Self {
        Self {
            encoding_time: raw.encoding_time,
            rendering_time: raw.rendering_time,
            frame_count: raw.frame_count,
            draw_call_count: raw.draw_call_count,
            total_draw_call_count: raw.total_draw_call_count,
        }
    }
}
impl crate::values::NativeValue for RenderingStats {
    type Raw = maplibre_native_ffi_sys::mln_rendering_stats;
    fn to_native(self) -> Self::Raw {
        RenderingStats::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum ResourceErrorReason {
    Other,
    RateLimit,
    Connection,
    Server,
    NotFound,
    None,
    Unknown(u32),
}
impl Default for ResourceErrorReason {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl ResourceErrorReason {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            5 => Self::Other,
            4 => Self::RateLimit,
            3 => Self::Connection,
            2 => Self::Server,
            1 => Self::NotFound,
            0 => Self::None,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Other => 5,
            Self::RateLimit => 4,
            Self::Connection => 3,
            Self::Server => 2,
            Self::NotFound => 1,
            Self::None => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum ResourceKind {
    Image,
    SpriteJson,
    SpriteImage,
    Glyphs,
    Tile,
    Source,
    Style,
    Unknown,
    Unrecognized(u32),
}
impl Default for ResourceKind {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl ResourceKind {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            7 => Self::Image,
            6 => Self::SpriteJson,
            5 => Self::SpriteImage,
            4 => Self::Glyphs,
            3 => Self::Tile,
            2 => Self::Source,
            1 => Self::Style,
            0 => Self::Unknown,
            value => Self::Unrecognized(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Image => 7,
            Self::SpriteJson => 6,
            Self::SpriteImage => 5,
            Self::Glyphs => 4,
            Self::Tile => 3,
            Self::Source => 2,
            Self::Style => 1,
            Self::Unknown => 0,
            Self::Unrecognized(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum ResourceLoadingMethod {
    NetworkOnly,
    CacheOnly,
    All,
    Unknown(u32),
}
impl Default for ResourceLoadingMethod {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl ResourceLoadingMethod {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            2 => Self::NetworkOnly,
            1 => Self::CacheOnly,
            0 => Self::All,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::NetworkOnly => 2,
            Self::CacheOnly => 1,
            Self::All => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum ResourcePriority {
    Low,
    Regular,
    Unknown(u32),
}
impl Default for ResourcePriority {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl ResourcePriority {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::Low,
            0 => Self::Regular,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Low => 1,
            Self::Regular => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Clone, Default)]
pub struct ResourceProvider {
    pub callback: Option<
        std::sync::Arc<
            dyn Fn(
                    crate::generated::ResourceRequest,
                    crate::generated::ResourceRequestHandle,
                ) -> crate::generated::ResourceProviderDecision
                + Send
                + Sync
                + 'static,
        >,
    >,
}

impl std::fmt::Debug for ResourceProvider {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("ResourceProvider").finish_non_exhaustive()
    }
}
impl ResourceProvider {
    pub fn with_callback<F>(mut self, callback: F) -> Self
    where
        F: Fn(
                crate::generated::ResourceRequest,
                crate::generated::ResourceRequestHandle,
            ) -> crate::generated::ResourceProviderDecision
            + Send
            + Sync
            + 'static,
    {
        self.callback = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(
                crate::generated::ResourceRequest,
                crate::generated::ResourceRequestHandle,
            ) -> crate::generated::ResourceProviderDecision
            + Send
            + Sync
            + 'static,
    {
        Self::default().with_callback(callback)
    }
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_resource_provider> {
        let mut raw: maplibre_native_ffi_sys::mln_resource_provider = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_resource_provider>() as _;
        raw.callback = self
            .callback
            .as_ref()
            .map(|_| Self::callback_trampoline as _);
        if !(raw.callback.is_none()) {
            raw.user_data = unsafe { arena.registration(self.clone(), Self::release_callback) };
            raw.release_user_data = Some(Self::release_callback);
        }
        Ok(raw)
    }
    /// Captures an empty callback descriptor returned by its native constructor.
    ///
    /// # Safety
    /// Borrowed fields must remain valid for this conversion.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_resource_provider,
    ) -> crate::Result<Self> {
        if !(raw.callback.is_none()) {
            return Err(crate::Error::invalid_argument(
                "foreign callbacks cannot be adopted",
            ));
        }
        Ok(Self { callback: None })
    }
    unsafe extern "C" fn release_callback(user_data: *mut std::ffi::c_void) {
        let _policy = crate::callback::PolicyScope::enter(&[], 0);
        let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| unsafe {
            drop(Box::from_raw(user_data.cast::<Self>()))
        }));
    }
    unsafe extern "C" fn callback_trampoline(
        binding_arg_0: *mut std::ffi::c_void,
        binding_arg_1: *const maplibre_native_ffi_sys::mln_resource_request,
        binding_arg_2: maplibre_native_ffi_sys::mln_resource_request_handle,
    ) -> u32 {
        let _policy = crate::callback::PolicyScope::enter(
            &[
                "mln_resource_request_complete",
                "mln_resource_request_cancelled",
                "mln_resource_request_set_cancel_callback",
                "mln_resource_request_release",
            ],
            binding_arg_2.0,
        );
        let request_state = match unsafe {
            crate::resource::ResourceRequestHandleState::new(
                binding_arg_2,
                crate::generated::RESOURCE_REQUEST_HANDLE_FUNCTIONS,
            )
        } {
            Ok(state) => state,
            Err(_) => return maplibre_native_ffi_sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH,
        };
        let result =
            std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| -> crate::Result<u32> {
                let state = unsafe { &*binding_arg_0.cast::<Self>() };
                let callback = state
                    .callback
                    .as_ref()
                    .ok_or_else(|| crate::Error::invalid_argument("missing callback"))?;
                let result = callback(
                    {
                        let item = unsafe { binding_arg_1.as_ref() }
                            .ok_or_else(|| crate::Error::invalid_argument("null record value"))?;
                        unsafe { crate::generated::ResourceRequest::from_native(*item) }?
                    },
                    ResourceRequestHandle {
                        state: std::sync::Arc::clone(&request_state),
                        not_sync: std::marker::PhantomData,
                    },
                );
                Ok(result.to_native())
            }));
        match result {
            Ok(Ok(value))
                if value == maplibre_native_ffi_sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE =>
            {
                request_state
                    .finish_provider_decision(crate::resource::ResourceProviderDecision::Handle)
            }
            Ok(Ok(value))
                if value
                    == maplibre_native_ffi_sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH =>
            {
                request_state.finish_provider_decision(
                    crate::resource::ResourceProviderDecision::PassThrough,
                )
            }
            _ => request_state.finish_provider_exception(),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum ResourceProviderDecision {
    Handle,
    PassThrough,
    Unknown(u32),
}
impl Default for ResourceProviderDecision {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl ResourceProviderDecision {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::Handle,
            0 => Self::PassThrough,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Handle => 1,
            Self::PassThrough => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct ResourceRequestRange {
    pub range_start: u64,
    pub range_end: u64,
}
#[derive(Debug, Clone, PartialEq, Default)]
pub struct ResourceRequest {
    pub range: Option<ResourceRequestRange>,
    pub requested_url: Option<String>,
    pub resolved_url: Option<String>,
    pub kind: crate::generated::ResourceKind,
    pub loading_method: crate::generated::ResourceLoadingMethod,
    pub priority: crate::generated::ResourcePriority,
    pub usage: crate::generated::ResourceUsage,
    pub storage_policy: crate::generated::ResourceStoragePolicy,
    pub prior_modified_unix_ms: Option<i64>,
    pub prior_expires_unix_ms: Option<i64>,
    pub prior_etag: Option<String>,
    pub prior_data: Vec<u8>,
}

impl ResourceRequest {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_resource_request> {
        let mut raw: maplibre_native_ffi_sys::mln_resource_request = unsafe { std::mem::zeroed() };
        raw.has_prior_expires = false;
        raw.has_prior_modified = false;
        raw.has_range = false;
        if let Some(item) = &self.range {
            raw.has_range = true;
            raw.range_start = *(&item.range_start);
            raw.range_end = *(&item.range_end);
        }
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_resource_request>() as _;
        raw.requested_url = match (&self.requested_url).as_ref() {
            Some(item) => arena.c_string((item))?,
            None => std::ptr::null(),
        };
        raw.resolved_url = match (&self.resolved_url).as_ref() {
            Some(item) => arena.c_string((item))?,
            None => std::ptr::null(),
        };
        raw.kind = (&self.kind).to_native();
        raw.loading_method = (&self.loading_method).to_native();
        raw.priority = (&self.priority).to_native();
        raw.usage = (&self.usage).to_native();
        raw.storage_policy = (&self.storage_policy).to_native();
        if let Some(item) = &self.prior_modified_unix_ms {
            raw.has_prior_modified = true;
            raw.prior_modified_unix_ms = *(item);
        }
        if let Some(item) = &self.prior_expires_unix_ms {
            raw.has_prior_expires = true;
            raw.prior_expires_unix_ms = *(item);
        }
        raw.prior_etag = match (&self.prior_etag).as_ref() {
            Some(item) => arena.c_string((item))?,
            None => std::ptr::null(),
        };
        raw.prior_data = (&self.prior_data).as_ptr().cast();
        raw.prior_data_size = self
            .prior_data
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_resource_request,
    ) -> crate::Result<Self> {
        Ok(Self {
            range: if raw.has_range {
                Some(ResourceRequestRange {
                    range_start: raw.range_start,
                    range_end: raw.range_end,
                })
            } else {
                None
            },
            requested_url: if raw.requested_url.is_null() {
                None
            } else {
                Some(unsafe { crate::string::copy_c_string(raw.requested_url) }?)
            },
            resolved_url: if raw.resolved_url.is_null() {
                None
            } else {
                Some(unsafe { crate::string::copy_c_string(raw.resolved_url) }?)
            },
            kind: crate::generated::ResourceKind::from_native(raw.kind),
            loading_method: crate::generated::ResourceLoadingMethod::from_native(
                raw.loading_method,
            ),
            priority: crate::generated::ResourcePriority::from_native(raw.priority),
            usage: crate::generated::ResourceUsage::from_native(raw.usage),
            storage_policy: crate::generated::ResourceStoragePolicy::from_native(
                raw.storage_policy,
            ),
            prior_modified_unix_ms: if raw.has_prior_modified {
                Some(raw.prior_modified_unix_ms)
            } else {
                None
            },
            prior_expires_unix_ms: if raw.has_prior_expires {
                Some(raw.prior_expires_unix_ms)
            } else {
                None
            },
            prior_etag: if raw.prior_etag.is_null() {
                None
            } else {
                Some(unsafe { crate::string::copy_c_string(raw.prior_etag) }?)
            },
            prior_data: unsafe {
                crate::string::copy_string_view_bytes(maplibre_native_ffi_sys::mln_buffer_view {
                    data: raw.prior_data.cast(),
                    size: raw.prior_data_size as usize,
                })
            }?,
        })
    }
}

#[derive(Debug)]
pub struct ResourceRequestHandle {
    state: std::sync::Arc<crate::resource::ResourceRequestHandleState>,
    not_sync: std::marker::PhantomData<std::cell::Cell<()>>,
}
impl ResourceRequestHandle {
    pub fn complete(&self, response: &crate::generated::ResourceResponse) -> crate::Result<()> {
        let native = self.state.native_for_call()?;
        crate::callback::check("mln_resource_request_complete", native.0)?;
        let mut arena = crate::input::InputArena::default();
        let response = response.to_native(&mut arena)?;
        self.state.complete_with(|handle| {
            crate::check(|diagnostic| unsafe {
                maplibre_native_ffi_sys::mln_resource_request_complete(
                    handle, &response, diagnostic,
                )
            })
        })
    }
    pub fn cancelled(&self) -> crate::Result<bool> {
        let native = self.state.native_for_call()?;
        crate::callback::check("mln_resource_request_cancelled", native.0)?;
        let mut cancelled = false;
        crate::check(|diagnostic| unsafe {
            maplibre_native_ffi_sys::mln_resource_request_cancelled(
                native,
                &mut cancelled,
                diagnostic,
            )
        })?;
        Ok(cancelled)
    }
    pub fn set_cancel_callback(
        &self,
        callback: impl FnOnce() + Send + 'static,
    ) -> crate::Result<bool> {
        let native = self.state.native_for_call()?;
        crate::callback::check("mln_resource_request_set_cancel_callback", native.0)?;
        self.state.set_cancel_callback(Box::new(callback))
    }
    pub fn wait_until_retired(&self) -> crate::Result<()> {
        let native = self.state.issued_handle();
        crate::callback::check("mln_resource_request_wait_until_retired", native.0)?;
        crate::check(|diagnostic| unsafe {
            maplibre_native_ffi_sys::mln_resource_request_wait_until_retired(native, diagnostic)
        })
    }
    pub fn close(&self) -> crate::Result<()> {
        crate::callback::check("mln_resource_request_release", self.state.issued_handle().0)?;
        self.state.close();
        Ok(())
    }
}
impl crate::resource::ResourceRequestHandleState {
    /// Registers a callback that runs at most once when MapLibre cancels the
    /// request, returning whether the request was already cancelled.
    ///
    /// An accepted registration transfers the callback to the C API, which
    /// releases it once it can no longer run. A rejected registration or an
    /// already cancelled request drops the callback unrun before returning.
    pub fn set_cancel_callback(
        &self,
        callback: Box<dyn FnOnce() + Send + 'static>,
    ) -> crate::Result<bool> {
        type Registration = (u64, Option<Box<dyn FnOnce() + Send + 'static>>);
        unsafe extern "C" fn invoke(user_data: *mut std::ffi::c_void) {
            // SAFETY: native passes the registration it owns and invokes it at
            // most once, before its release.
            let (owner, callback) = unsafe { &mut *user_data.cast::<Registration>() };
            let _policy = crate::callback::PolicyScope::enter(
                &[
                    "mln_resource_request_complete",
                    "mln_resource_request_cancelled",
                    "mln_resource_request_set_cancel_callback",
                    "mln_resource_request_release",
                ],
                *owner,
            );
            if let Some(callback) = callback.take() {
                let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(callback));
            }
        }
        unsafe extern "C" fn release(user_data: *mut std::ffi::c_void) {
            let _policy = crate::callback::PolicyScope::enter(&[], 0);
            // SAFETY: native or the rejecting arena releases each registration once.
            let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| unsafe {
                drop(Box::from_raw(user_data.cast::<Registration>()))
            }));
        }
        let handle = self.native_for_call()?;
        let mut arena = crate::input::InputArena::default();
        // SAFETY: release reclaims exactly this box without unwinding.
        let user_data =
            unsafe { arena.registration::<Registration>((handle.0, Some(callback)), release) };
        let mut cancelled = false;
        crate::check(|diagnostic| unsafe {
            maplibre_native_ffi_sys::mln_resource_request_set_cancel_callback(
                handle,
                Some(invoke),
                user_data,
                Some(release),
                &mut cancelled,
                diagnostic,
            )
        })?;
        if !cancelled {
            arena.accept_registrations();
        }
        Ok(cancelled)
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct ResourceResponse {
    pub status: crate::generated::ResourceResponseStatus,
    pub error_reason: crate::generated::ResourceErrorReason,
    pub bytes: Vec<u8>,
    pub error_message: Option<String>,
    pub must_revalidate: bool,
    pub modified_unix_ms: Option<i64>,
    pub expires_unix_ms: Option<i64>,
    pub etag: Option<String>,
    pub retry_after_unix_ms: Option<i64>,
}

impl ResourceResponse {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_resource_response> {
        let mut raw: maplibre_native_ffi_sys::mln_resource_response = unsafe { std::mem::zeroed() };
        raw.has_retry_after = false;
        raw.has_expires = false;
        raw.has_modified = false;
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_resource_response>() as _;
        raw.status = (&self.status).to_native();
        raw.error_reason = (&self.error_reason).to_native();
        raw.bytes = (&self.bytes).as_ptr().cast();
        raw.byte_count = self
            .bytes
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;
        raw.error_message = match (&self.error_message).as_ref() {
            Some(item) => arena.c_string((item))?,
            None => std::ptr::null(),
        };
        raw.must_revalidate = *(&self.must_revalidate);
        if let Some(item) = &self.modified_unix_ms {
            raw.has_modified = true;
            raw.modified_unix_ms = *(item);
        }
        if let Some(item) = &self.expires_unix_ms {
            raw.has_expires = true;
            raw.expires_unix_ms = *(item);
        }
        raw.etag = match (&self.etag).as_ref() {
            Some(item) => arena.c_string((item))?,
            None => std::ptr::null(),
        };
        if let Some(item) = &self.retry_after_unix_ms {
            raw.has_retry_after = true;
            raw.retry_after_unix_ms = *(item);
        }
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_resource_response,
    ) -> crate::Result<Self> {
        Ok(Self {
            status: crate::generated::ResourceResponseStatus::from_native(raw.status),
            error_reason: crate::generated::ResourceErrorReason::from_native(raw.error_reason),
            bytes: unsafe {
                crate::string::copy_string_view_bytes(maplibre_native_ffi_sys::mln_buffer_view {
                    data: raw.bytes.cast(),
                    size: raw.byte_count as usize,
                })
            }?,
            error_message: if raw.error_message.is_null() {
                None
            } else {
                Some(unsafe { crate::string::copy_c_string(raw.error_message) }?)
            },
            must_revalidate: raw.must_revalidate,
            modified_unix_ms: if raw.has_modified {
                Some(raw.modified_unix_ms)
            } else {
                None
            },
            expires_unix_ms: if raw.has_expires {
                Some(raw.expires_unix_ms)
            } else {
                None
            },
            etag: if raw.etag.is_null() {
                None
            } else {
                Some(unsafe { crate::string::copy_c_string(raw.etag) }?)
            },
            retry_after_unix_ms: if raw.has_retry_after {
                Some(raw.retry_after_unix_ms)
            } else {
                None
            },
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum ResourceResponseStatus {
    NotModified,
    NoContent,
    Error,
    Ok,
    Unknown(u32),
}
impl Default for ResourceResponseStatus {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl ResourceResponseStatus {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            3 => Self::NotModified,
            2 => Self::NoContent,
            1 => Self::Error,
            0 => Self::Ok,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::NotModified => 3,
            Self::NoContent => 2,
            Self::Error => 1,
            Self::Ok => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum ResourceStoragePolicy {
    Volatile,
    Permanent,
    Unknown(u32),
}
impl Default for ResourceStoragePolicy {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl ResourceStoragePolicy {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::Volatile,
            0 => Self::Permanent,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Volatile => 1,
            Self::Permanent => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Clone, Default)]
pub struct ResourceTransform {
    pub callback: Option<
        std::sync::Arc<
            dyn Fn(
                    crate::generated::ResourceKind,
                    String,
                    &mut crate::generated::ResourceTransformResponse<'_>,
                ) -> crate::Result<()>
                + Send
                + Sync
                + 'static,
        >,
    >,
}

impl std::fmt::Debug for ResourceTransform {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("ResourceTransform").finish_non_exhaustive()
    }
}
impl ResourceTransform {
    pub fn with_callback<F>(mut self, callback: F) -> Self
    where
        F: Fn(
                crate::generated::ResourceKind,
                String,
                &mut crate::generated::ResourceTransformResponse<'_>,
            ) -> crate::Result<()>
            + Send
            + Sync
            + 'static,
    {
        self.callback = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(
                crate::generated::ResourceKind,
                String,
                &mut crate::generated::ResourceTransformResponse<'_>,
            ) -> crate::Result<()>
            + Send
            + Sync
            + 'static,
    {
        Self::default().with_callback(callback)
    }
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_resource_transform> {
        let mut raw: maplibre_native_ffi_sys::mln_resource_transform =
            unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_resource_transform>() as _;
        raw.callback = self
            .callback
            .as_ref()
            .map(|_| Self::callback_trampoline as _);
        if !(raw.callback.is_none()) {
            raw.user_data = unsafe { arena.registration(self.clone(), Self::release_callback) };
            raw.release_user_data = Some(Self::release_callback);
        }
        Ok(raw)
    }
    /// Captures an empty callback descriptor returned by its native constructor.
    ///
    /// # Safety
    /// Borrowed fields must remain valid for this conversion.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_resource_transform,
    ) -> crate::Result<Self> {
        if !(raw.callback.is_none()) {
            return Err(crate::Error::invalid_argument(
                "foreign callbacks cannot be adopted",
            ));
        }
        Ok(Self { callback: None })
    }
    unsafe extern "C" fn release_callback(user_data: *mut std::ffi::c_void) {
        let _policy = crate::callback::PolicyScope::enter(&[], 0);
        let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| unsafe {
            drop(Box::from_raw(user_data.cast::<Self>()))
        }));
    }
    unsafe extern "C" fn callback_trampoline(
        binding_arg_0: *mut std::ffi::c_void,
        binding_arg_1: u32,
        binding_arg_2: *const std::ffi::c_char,
        binding_arg_3: *mut maplibre_native_ffi_sys::mln_resource_transform_response,
    ) -> maplibre_native_ffi_sys::mln_status {
        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(
            || -> crate::Result<maplibre_native_ffi_sys::mln_status> {
                let _policy = crate::callback::PolicyScope::enter(
                    &["mln_resource_transform_response_set_url"],
                    binding_arg_3 as usize as u64,
                );
                let state = unsafe { &*binding_arg_0.cast::<Self>() };
                let callback = state
                    .callback
                    .as_ref()
                    .ok_or_else(|| crate::Error::invalid_argument("missing callback"))?;
                callback(
                    crate::generated::ResourceKind::from_native(binding_arg_1),
                    unsafe { crate::string::copy_c_string(binding_arg_2) }?,
                    &mut ResourceTransformResponse {
                        raw: std::ptr::NonNull::new(binding_arg_3).ok_or_else(|| {
                            crate::Error::invalid_argument("null callback response")
                        })?,
                        lifetime: std::marker::PhantomData,
                    },
                )?;
                Ok(maplibre_native_ffi_sys::MLN_STATUS_OK)
            },
        ));
        match result {
            Ok(Ok(value)) => value,
            Ok(Err(error)) => crate::resource::status_for_error(&error),
            _ => maplibre_native_ffi_sys::MLN_STATUS_NATIVE_ERROR,
        }
    }
}

/// A native response borrowed only for one host callback.
#[derive(Debug)]
pub struct ResourceTransformResponse<'a> {
    raw: std::ptr::NonNull<maplibre_native_ffi_sys::mln_resource_transform_response>,
    lifetime:
        std::marker::PhantomData<&'a mut maplibre_native_ffi_sys::mln_resource_transform_response>,
}
impl ResourceTransformResponse<'_> {
    pub fn set_url(&mut self, url: &str) -> crate::Result<()> {
        crate::callback::check(
            "mln_resource_transform_response_set_url",
            self.raw.as_ptr() as usize as u64,
        )?;
        let url_size = url
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
        crate::check(|diagnostic| unsafe {
            maplibre_native_ffi_sys::mln_resource_transform_response_set_url(
                self.raw.as_ptr(),
                (url).as_bytes().as_ptr().cast(),
                url_size,
                diagnostic,
            )
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum ResourceUsage {
    Offline,
    Online,
    Unknown(u32),
}
impl Default for ResourceUsage {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl ResourceUsage {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::Offline,
            0 => Self::Online,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Offline => 1,
            Self::Online => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct RuntimeEvent {
    pub message: String,
    pub r#type: crate::generated::RuntimeEventType,
    pub source_type: crate::generated::RuntimeEventSourceType,
    pub source: u64,
    pub code: i32,
    pub payload: crate::generated::RuntimeEventPayload,
}

impl RuntimeEvent {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_runtime_event> {
        let mut raw: maplibre_native_ffi_sys::mln_runtime_event = unsafe { std::mem::zeroed() };
        raw.type_ = (&self.r#type).to_native();
        raw.source_type = (&self.source_type).to_native();
        raw.source = *(&self.source);
        raw.code = *(&self.code);
        match &self.payload {
            crate::generated::RuntimeEventPayload::RenderFrame(item) => {
                raw.payload_type = maplibre_native_ffi_sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME;
                raw.payload.render_frame = (item).to_native();
            }
            crate::generated::RuntimeEventPayload::RenderMap(item) => {
                raw.payload_type = maplibre_native_ffi_sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP;
                raw.payload.render_map = (item).to_native();
            }
            crate::generated::RuntimeEventPayload::TileAction(item) => {
                raw.payload_type = maplibre_native_ffi_sys::MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION;
                raw.payload.tile_action = (item).to_native();
            }
            crate::generated::RuntimeEventPayload::OfflineRegionStatus(item) => {
                raw.payload_type =
                    maplibre_native_ffi_sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS;
                raw.payload.offline_region_status = (item).to_native();
            }
            crate::generated::RuntimeEventPayload::OfflineRegionResponseError(item) => {
                raw.payload_type = maplibre_native_ffi_sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR;
                raw.payload.offline_region_response_error = (item).to_native();
            }
            crate::generated::RuntimeEventPayload::OfflineRegionTileCountLimit(item) => {
                raw.payload_type = maplibre_native_ffi_sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT;
                raw.payload.offline_region_tile_count_limit = (item).to_native();
            }
            crate::generated::RuntimeEventPayload::CameraTransitionFinished(item) => {
                raw.payload_type =
                    maplibre_native_ffi_sys::MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED;
                raw.payload.camera_transition_finished = (item).to_native();
            }
            crate::generated::RuntimeEventPayload::Empty => {
                raw.payload_type = maplibre_native_ffi_sys::MLN_RUNTIME_EVENT_PAYLOAD_NONE;
            }
            crate::generated::RuntimeEventPayload::Unknown(_) => {
                return Err(crate::Error::invalid_argument(
                    "unknown union variant cannot be submitted",
                ));
            }
        }
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_runtime_event,
    ) -> crate::Result<Self> {
        Ok(Self {
            message: String::new(),
            r#type: crate::generated::RuntimeEventType::from_native(raw.type_),
            source_type: crate::generated::RuntimeEventSourceType::from_native(raw.source_type),
            source: raw.source,
            code: raw.code,
            payload: match raw.payload_type { maplibre_native_ffi_sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME => crate::generated::RuntimeEventPayload::RenderFrame(crate::generated::RuntimeEventRenderFrame::from_native(unsafe { raw.payload.render_frame })), maplibre_native_ffi_sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP => crate::generated::RuntimeEventPayload::RenderMap(crate::generated::RuntimeEventRenderMap::from_native(unsafe { raw.payload.render_map })), maplibre_native_ffi_sys::MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION => crate::generated::RuntimeEventPayload::TileAction(crate::generated::RuntimeEventTileAction::from_native(unsafe { raw.payload.tile_action })), maplibre_native_ffi_sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS => crate::generated::RuntimeEventPayload::OfflineRegionStatus(crate::generated::RuntimeEventOfflineRegionStatus::from_native(unsafe { raw.payload.offline_region_status })), maplibre_native_ffi_sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR => crate::generated::RuntimeEventPayload::OfflineRegionResponseError(crate::generated::RuntimeEventOfflineRegionResponseError::from_native(unsafe { raw.payload.offline_region_response_error })), maplibre_native_ffi_sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT => crate::generated::RuntimeEventPayload::OfflineRegionTileCountLimit(crate::generated::RuntimeEventOfflineRegionTileCountLimit::from_native(unsafe { raw.payload.offline_region_tile_count_limit })), maplibre_native_ffi_sys::MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED => crate::generated::RuntimeEventPayload::CameraTransitionFinished(crate::generated::RuntimeEventCameraTransitionFinished::from_native(unsafe { raw.payload.camera_transition_finished })), maplibre_native_ffi_sys::MLN_RUNTIME_EVENT_PAYLOAD_NONE => crate::generated::RuntimeEventPayload::Empty, tag => crate::generated::RuntimeEventPayload::Unknown(tag as u32) },
        })
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct RuntimeEventBatchView {
    pub events: Vec<crate::generated::RuntimeEvent>,
}

impl RuntimeEventBatchView {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_runtime_event_batch_view> {
        let mut raw: maplibre_native_ffi_sys::mln_runtime_event_batch_view =
            unsafe { std::mem::zeroed() };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_runtime_event_batch_view>() as _;
        raw.events = {
            let items = (&self.events)
                .iter()
                .map(|item| -> crate::Result<_> { Ok((item).to_native(arena)?) })
                .collect::<crate::Result<Vec<_>>>()?;
            arena.array(items)
        };
        raw.event_count = self
            .events
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_runtime_event_batch_view,
    ) -> crate::Result<Self> {
        Ok(Self {
            events: unsafe {
                crate::input::strided_values(
                    raw.events,
                    raw.event_count as usize,
                    raw.event_size as usize,
                )
            }?
            .iter()
            .map(|item| -> crate::Result<_> {
                Ok({
                    let mut value = unsafe { crate::generated::RuntimeEvent::from_native(*item) }?;
                    value.message = unsafe {
                        crate::input::arena_string(
                            raw.messages.cast(),
                            raw.messages_size as usize,
                            item.message_offset as usize,
                            item.message_size as usize,
                        )
                    }?;
                    value
                })
            })
            .collect::<crate::Result<Vec<_>>>()?,
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventCameraTransitionFinished {
    pub transition_id: u64,
}

impl RuntimeEventCameraTransitionFinished {
    pub const fn new(transition_id: u64) -> Self {
        Self { transition_id }
    }
    #[doc(hidden)]
    pub fn to_native(
        self,
    ) -> maplibre_native_ffi_sys::mln_runtime_event_camera_transition_finished {
        let mut raw = unsafe {
            std::mem::zeroed::<maplibre_native_ffi_sys::mln_runtime_event_camera_transition_finished>(
            )
        };
        raw.transition_id = self.transition_id;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(
        raw: maplibre_native_ffi_sys::mln_runtime_event_camera_transition_finished,
    ) -> Self {
        Self {
            transition_id: raw.transition_id,
        }
    }
}
impl crate::values::NativeValue for RuntimeEventCameraTransitionFinished {
    type Raw = maplibre_native_ffi_sys::mln_runtime_event_camera_transition_finished;
    fn to_native(self) -> Self::Raw {
        RuntimeEventCameraTransitionFinished::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct RuntimeEventMask: u64 {
        const NONE = 0;
        const MAP_CAMERA_WILL_CHANGE = 2;
        const MAP_CAMERA_IS_CHANGING = 4;
        const MAP_CAMERA_DID_CHANGE = 8;
        const MAP_STYLE_LOADED = 16;
        const MAP_LOADING_STARTED = 32;
        const MAP_LOADING_FINISHED = 64;
        const MAP_LOADING_FAILED = 128;
        const MAP_IDLE = 256;
        const MAP_RENDER_UPDATE_AVAILABLE = 512;
        const MAP_RENDER_ERROR = 1024;
        const MAP_STILL_IMAGE_FINISHED = 2048;
        const MAP_STILL_IMAGE_FAILED = 4096;
        const MAP_RENDER_FRAME_STARTED = 8192;
        const MAP_RENDER_FRAME_FINISHED = 16384;
        const MAP_RENDER_MAP_STARTED = 32768;
        const MAP_RENDER_MAP_FINISHED = 65536;
        const MAP_STYLE_IMAGE_MISSING = 131072;
        const MAP_TILE_ACTION = 262144;
        const MAP_CAMERA_TRANSITION_FINISHED = 4194304;
        const OFFLINE_REGION_STATUS_CHANGED = 524288;
        const OFFLINE_REGION_RESPONSE_ERROR = 1048576;
        const OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 2097152;
        const ALL_MAP_EVENTS = 4718590;
        const ALL_RUNTIME_EVENTS = 3670016;
        const ALL = 8388606;
        const _ = !0;
} }
impl RuntimeEventMask {
    pub fn from_native(raw: u64) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u64 {
        self.bits()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventOfflineRegionResponseError {
    pub region_id: i64,
    pub reason: crate::generated::ResourceErrorReason,
}

impl RuntimeEventOfflineRegionResponseError {
    pub const fn new(region_id: i64, reason: crate::generated::ResourceErrorReason) -> Self {
        Self { region_id, reason }
    }
    #[doc(hidden)]
    pub fn to_native(
        self,
    ) -> maplibre_native_ffi_sys::mln_runtime_event_offline_region_response_error {
        let mut raw = unsafe {
            std::mem::zeroed::<
                maplibre_native_ffi_sys::mln_runtime_event_offline_region_response_error,
            >()
        };
        raw.region_id = self.region_id;
        raw.reason = self.reason.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(
        raw: maplibre_native_ffi_sys::mln_runtime_event_offline_region_response_error,
    ) -> Self {
        Self {
            region_id: raw.region_id,
            reason: crate::generated::ResourceErrorReason::from_native(raw.reason),
        }
    }
}
impl crate::values::NativeValue for RuntimeEventOfflineRegionResponseError {
    type Raw = maplibre_native_ffi_sys::mln_runtime_event_offline_region_response_error;
    fn to_native(self) -> Self::Raw {
        RuntimeEventOfflineRegionResponseError::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventOfflineRegionStatus {
    pub region_id: i64,
    pub status: crate::generated::OfflineRegionStatus,
}

impl RuntimeEventOfflineRegionStatus {
    pub const fn new(region_id: i64, status: crate::generated::OfflineRegionStatus) -> Self {
        Self { region_id, status }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_runtime_event_offline_region_status {
        let mut raw = unsafe {
            std::mem::zeroed::<maplibre_native_ffi_sys::mln_runtime_event_offline_region_status>()
        };
        raw.region_id = self.region_id;
        raw.status = self.status.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(
        raw: maplibre_native_ffi_sys::mln_runtime_event_offline_region_status,
    ) -> Self {
        Self {
            region_id: raw.region_id,
            status: crate::generated::OfflineRegionStatus::from_native(raw.status),
        }
    }
}
impl crate::values::NativeValue for RuntimeEventOfflineRegionStatus {
    type Raw = maplibre_native_ffi_sys::mln_runtime_event_offline_region_status;
    fn to_native(self) -> Self::Raw {
        RuntimeEventOfflineRegionStatus::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventOfflineRegionTileCountLimit {
    pub region_id: i64,
    pub limit: u64,
}

impl RuntimeEventOfflineRegionTileCountLimit {
    pub const fn new(region_id: i64, limit: u64) -> Self {
        Self { region_id, limit }
    }
    #[doc(hidden)]
    pub fn to_native(
        self,
    ) -> maplibre_native_ffi_sys::mln_runtime_event_offline_region_tile_count_limit {
        let mut raw = unsafe {
            std::mem::zeroed::<
                maplibre_native_ffi_sys::mln_runtime_event_offline_region_tile_count_limit,
            >()
        };
        raw.region_id = self.region_id;
        raw.limit = self.limit;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(
        raw: maplibre_native_ffi_sys::mln_runtime_event_offline_region_tile_count_limit,
    ) -> Self {
        Self {
            region_id: raw.region_id,
            limit: raw.limit,
        }
    }
}
impl crate::values::NativeValue for RuntimeEventOfflineRegionTileCountLimit {
    type Raw = maplibre_native_ffi_sys::mln_runtime_event_offline_region_tile_count_limit;
    fn to_native(self) -> Self::Raw {
        RuntimeEventOfflineRegionTileCountLimit::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, PartialEq)]
pub enum RuntimeEventPayload {
    RenderFrame(crate::generated::RuntimeEventRenderFrame),
    RenderMap(crate::generated::RuntimeEventRenderMap),
    TileAction(crate::generated::RuntimeEventTileAction),
    OfflineRegionStatus(crate::generated::RuntimeEventOfflineRegionStatus),
    OfflineRegionResponseError(crate::generated::RuntimeEventOfflineRegionResponseError),
    OfflineRegionTileCountLimit(crate::generated::RuntimeEventOfflineRegionTileCountLimit),
    CameraTransitionFinished(crate::generated::RuntimeEventCameraTransitionFinished),
    Empty,
    Unknown(u32),
}
impl Default for RuntimeEventPayload {
    fn default() -> Self {
        Self::Unknown(0)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum RuntimeEventPayloadType {
    CameraTransitionFinished,
    OfflineRegionTileCountLimit,
    OfflineRegionResponseError,
    OfflineRegionStatus,
    TileAction,
    RenderMap,
    RenderFrame,
    None,
    Unknown(u32),
}
impl Default for RuntimeEventPayloadType {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl RuntimeEventPayloadType {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            9 => Self::CameraTransitionFinished,
            7 => Self::OfflineRegionTileCountLimit,
            6 => Self::OfflineRegionResponseError,
            5 => Self::OfflineRegionStatus,
            4 => Self::TileAction,
            2 => Self::RenderMap,
            1 => Self::RenderFrame,
            0 => Self::None,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::CameraTransitionFinished => 9,
            Self::OfflineRegionTileCountLimit => 7,
            Self::OfflineRegionResponseError => 6,
            Self::OfflineRegionStatus => 5,
            Self::TileAction => 4,
            Self::RenderMap => 2,
            Self::RenderFrame => 1,
            Self::None => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventRenderFrame {
    pub mode: crate::generated::RenderMode,
    pub needs_repaint: bool,
    pub placement_changed: bool,
    pub stats: crate::generated::RenderingStats,
}

impl RuntimeEventRenderFrame {
    pub const fn new(
        mode: crate::generated::RenderMode,
        needs_repaint: bool,
        placement_changed: bool,
        stats: crate::generated::RenderingStats,
    ) -> Self {
        Self {
            mode,
            needs_repaint,
            placement_changed,
            stats,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_runtime_event_render_frame {
        let mut raw = unsafe {
            std::mem::zeroed::<maplibre_native_ffi_sys::mln_runtime_event_render_frame>()
        };
        raw.mode = self.mode.to_native();
        raw.needs_repaint = self.needs_repaint;
        raw.placement_changed = self.placement_changed;
        raw.stats = self.stats.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_runtime_event_render_frame) -> Self {
        Self {
            mode: crate::generated::RenderMode::from_native(raw.mode),
            needs_repaint: raw.needs_repaint,
            placement_changed: raw.placement_changed,
            stats: crate::generated::RenderingStats::from_native(raw.stats),
        }
    }
}
impl crate::values::NativeValue for RuntimeEventRenderFrame {
    type Raw = maplibre_native_ffi_sys::mln_runtime_event_render_frame;
    fn to_native(self) -> Self::Raw {
        RuntimeEventRenderFrame::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventRenderMap {
    pub mode: crate::generated::RenderMode,
}

impl RuntimeEventRenderMap {
    pub const fn new(mode: crate::generated::RenderMode) -> Self {
        Self { mode }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_runtime_event_render_map {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_runtime_event_render_map>() };
        raw.mode = self.mode.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_runtime_event_render_map) -> Self {
        Self {
            mode: crate::generated::RenderMode::from_native(raw.mode),
        }
    }
}
impl crate::values::NativeValue for RuntimeEventRenderMap {
    type Raw = maplibre_native_ffi_sys::mln_runtime_event_render_map;
    fn to_native(self) -> Self::Raw {
        RuntimeEventRenderMap::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum RuntimeEventSourceType {
    Map,
    Runtime,
    Unknown(u32),
}
impl Default for RuntimeEventSourceType {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl RuntimeEventSourceType {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::Map,
            0 => Self::Runtime,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Map => 1,
            Self::Runtime => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventTileAction {
    pub operation: crate::generated::TileOperation,
    pub tile_id: crate::generated::TileId,
}

impl RuntimeEventTileAction {
    pub const fn new(
        operation: crate::generated::TileOperation,
        tile_id: crate::generated::TileId,
    ) -> Self {
        Self { operation, tile_id }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_runtime_event_tile_action {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_runtime_event_tile_action>() };
        raw.operation = self.operation.to_native();
        raw.tile_id = self.tile_id.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_runtime_event_tile_action) -> Self {
        Self {
            operation: crate::generated::TileOperation::from_native(raw.operation),
            tile_id: crate::generated::TileId::from_native(raw.tile_id),
        }
    }
}
impl crate::values::NativeValue for RuntimeEventTileAction {
    type Raw = maplibre_native_ffi_sys::mln_runtime_event_tile_action;
    fn to_native(self) -> Self::Raw {
        RuntimeEventTileAction::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum RuntimeEventType {
    MapCameraTransitionFinished,
    OfflineRegionTileCountLimitExceeded,
    OfflineRegionResponseError,
    OfflineRegionStatusChanged,
    MapTileAction,
    MapStyleImageMissing,
    MapRenderMapFinished,
    MapRenderMapStarted,
    MapRenderFrameFinished,
    MapRenderFrameStarted,
    MapStillImageFailed,
    MapStillImageFinished,
    MapRenderError,
    MapRenderUpdateAvailable,
    MapIdle,
    MapLoadingFailed,
    MapLoadingFinished,
    MapLoadingStarted,
    MapStyleLoaded,
    MapCameraDidChange,
    MapCameraIsChanging,
    MapCameraWillChange,
    Unknown(u32),
}
impl Default for RuntimeEventType {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl RuntimeEventType {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            22 => Self::MapCameraTransitionFinished,
            21 => Self::OfflineRegionTileCountLimitExceeded,
            20 => Self::OfflineRegionResponseError,
            19 => Self::OfflineRegionStatusChanged,
            18 => Self::MapTileAction,
            17 => Self::MapStyleImageMissing,
            16 => Self::MapRenderMapFinished,
            15 => Self::MapRenderMapStarted,
            14 => Self::MapRenderFrameFinished,
            13 => Self::MapRenderFrameStarted,
            12 => Self::MapStillImageFailed,
            11 => Self::MapStillImageFinished,
            10 => Self::MapRenderError,
            9 => Self::MapRenderUpdateAvailable,
            8 => Self::MapIdle,
            7 => Self::MapLoadingFailed,
            6 => Self::MapLoadingFinished,
            5 => Self::MapLoadingStarted,
            4 => Self::MapStyleLoaded,
            3 => Self::MapCameraDidChange,
            2 => Self::MapCameraIsChanging,
            1 => Self::MapCameraWillChange,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::MapCameraTransitionFinished => 22,
            Self::OfflineRegionTileCountLimitExceeded => 21,
            Self::OfflineRegionResponseError => 20,
            Self::OfflineRegionStatusChanged => 19,
            Self::MapTileAction => 18,
            Self::MapStyleImageMissing => 17,
            Self::MapRenderMapFinished => 16,
            Self::MapRenderMapStarted => 15,
            Self::MapRenderFrameFinished => 14,
            Self::MapRenderFrameStarted => 13,
            Self::MapStillImageFailed => 12,
            Self::MapStillImageFinished => 11,
            Self::MapRenderError => 10,
            Self::MapRenderUpdateAvailable => 9,
            Self::MapIdle => 8,
            Self::MapLoadingFailed => 7,
            Self::MapLoadingFinished => 6,
            Self::MapLoadingStarted => 5,
            Self::MapStyleLoaded => 4,
            Self::MapCameraDidChange => 3,
            Self::MapCameraIsChanging => 2,
            Self::MapCameraWillChange => 1,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone)]
pub struct RuntimeOptions {
    pub flags: u32,
    pub asset_path: Option<String>,
    pub cache_path: Option<String>,
    pub event_mask: crate::generated::RuntimeEventMask,
    pub event_wake: crate::generated::Wake,
}
impl Default for RuntimeOptions {
    fn default() -> Self {
        unsafe {
            Self::from_native(maplibre_native_ffi_sys::mln_runtime_options_default())
                .expect("native default must be valid")
        }
    }
}
impl RuntimeOptions {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_runtime_options> {
        let mut raw: maplibre_native_ffi_sys::mln_runtime_options =
            unsafe { maplibre_native_ffi_sys::mln_runtime_options_default() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_runtime_options>() as _;
        raw.flags = *(&self.flags);
        raw.asset_path = match (&self.asset_path).as_ref() {
            Some(item) => arena.c_string((item))?,
            None => std::ptr::null(),
        };
        raw.cache_path = match (&self.cache_path).as_ref() {
            Some(item) => arena.c_string((item))?,
            None => std::ptr::null(),
        };
        raw.event_mask = (&self.event_mask).to_native();
        raw.event_wake = (&self.event_wake).to_native(arena)?;
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_runtime_options,
    ) -> crate::Result<Self> {
        Ok(Self {
            flags: raw.flags,
            asset_path: if raw.asset_path.is_null() {
                None
            } else {
                Some(unsafe { crate::string::copy_c_string(raw.asset_path) }?)
            },
            cache_path: if raw.cache_path.is_null() {
                None
            } else {
                Some(unsafe { crate::string::copy_c_string(raw.cache_path) }?)
            },
            event_mask: crate::generated::RuntimeEventMask::from_native(raw.event_mask),
            event_wake: unsafe { crate::generated::Wake::from_native(raw.event_wake) }?,
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct ScreenBox {
    pub min: crate::generated::ScreenPoint,
    pub max: crate::generated::ScreenPoint,
}

impl ScreenBox {
    pub const fn new(
        min: crate::generated::ScreenPoint,
        max: crate::generated::ScreenPoint,
    ) -> Self {
        Self { min, max }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_screen_box {
        let mut raw = unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_screen_box>() };
        raw.min = self.min.to_native();
        raw.max = self.max.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_screen_box) -> Self {
        Self {
            min: crate::generated::ScreenPoint::from_native(raw.min),
            max: crate::generated::ScreenPoint::from_native(raw.max),
        }
    }
}
impl crate::values::NativeValue for ScreenBox {
    type Raw = maplibre_native_ffi_sys::mln_screen_box;
    fn to_native(self) -> Self::Raw {
        ScreenBox::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct ScreenLineString {
    pub points: Vec<crate::generated::ScreenPoint>,
}

impl ScreenLineString {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_screen_line_string> {
        let mut raw: maplibre_native_ffi_sys::mln_screen_line_string =
            unsafe { std::mem::zeroed() };
        raw.points = {
            let items = (&self.points)
                .iter()
                .map(|item| -> crate::Result<_> { Ok((item).to_native()) })
                .collect::<crate::Result<Vec<_>>>()?;
            arena.array(items)
        };
        raw.point_count = self
            .points
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_screen_line_string,
    ) -> crate::Result<Self> {
        Ok(Self {
            points: unsafe { crate::input::slice(raw.points, raw.point_count as usize) }?
                .iter()
                .map(|item| -> crate::Result<_> {
                    Ok(crate::generated::ScreenPoint::from_native(*item))
                })
                .collect::<crate::Result<Vec<_>>>()?,
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct ScreenPoint {
    pub x: f64,
    pub y: f64,
}

impl ScreenPoint {
    pub const fn new(x: f64, y: f64) -> Self {
        Self { x, y }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_screen_point {
        let mut raw = unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_screen_point>() };
        raw.x = self.x;
        raw.y = self.y;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_screen_point) -> Self {
        Self { x: raw.x, y: raw.y }
    }
}
impl crate::values::NativeValue for ScreenPoint {
    type Raw = maplibre_native_ffi_sys::mln_screen_point;
    fn to_native(self) -> Self::Raw {
        ScreenPoint::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct SourceFeatureQueryOptionField: u32 {
        const IDS = 1;
        const _ = !0;
} }
impl SourceFeatureQueryOptionField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct SourceFeatureQueryOptions {
    pub source_layer_ids: Option<Vec<String>>,
    pub filter: Option<Vec<u8>>,
}
impl Default for SourceFeatureQueryOptions {
    fn default() -> Self {
        unsafe {
            Self::from_native(maplibre_native_ffi_sys::mln_source_feature_query_options_default())
                .expect("native default must be valid")
        }
    }
}
impl SourceFeatureQueryOptions {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_source_feature_query_options> {
        let mut raw: maplibre_native_ffi_sys::mln_source_feature_query_options =
            unsafe { maplibre_native_ffi_sys::mln_source_feature_query_options_default() };
        raw.fields = 0;
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_source_feature_query_options>() as _;
        if let Some(item) = &self.source_layer_ids {
            raw.fields |= maplibre_native_ffi_sys::MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
            raw.source_layer_ids = {
                let items = (item)
                    .iter()
                    .map(|item| -> crate::Result<_> {
                        Ok(maplibre_native_ffi_sys::mln_buffer_view {
                            data: (item).as_bytes().as_ptr().cast(),
                            size: (item).as_bytes().len(),
                        })
                    })
                    .collect::<crate::Result<Vec<_>>>()?;
                arena.array(items)
            };
        }
        raw.source_layer_id_count = self
            .source_layer_ids
            .as_ref()
            .map_or(0, |items| items.len())
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;
        raw.filter = match (&self.filter).as_ref() {
            Some(item) => {
                let item = maplibre_native_ffi_sys::mln_buffer_view {
                    data: (item).as_ptr().cast(),
                    size: (item).len(),
                };
                arena.store(item)
            }
            None => std::ptr::null(),
        };
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_source_feature_query_options,
    ) -> crate::Result<Self> {
        Ok(Self {
            source_layer_ids: if raw.fields
                & maplibre_native_ffi_sys::MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
                != 0
            {
                Some(
                    unsafe {
                        crate::input::slice(
                            raw.source_layer_ids,
                            raw.source_layer_id_count as usize,
                        )
                    }?
                    .iter()
                    .map(|item| -> crate::Result<_> {
                        Ok(unsafe { crate::string::copy_string_view(*item) }?)
                    })
                    .collect::<crate::Result<Vec<_>>>()?,
                )
            } else {
                None
            },
            filter: if raw.filter.is_null() {
                None
            } else {
                Some({
                    let item = unsafe { raw.filter.as_ref() }
                        .ok_or_else(|| crate::Error::invalid_argument("null record value"))?;
                    unsafe { crate::string::copy_string_view_bytes(*item) }?
                })
            },
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Status {
    NotFound,
    NotReady,
    TargetLost,
    Busy,
    Cancelled,
    NativeError,
    Unsupported,
    WrongThread,
    InvalidState,
    InvalidArgument,
    Ok,
    Unknown(i32),
}
impl Default for Status {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl Status {
    pub fn from_raw(raw: i32) -> Self {
        match raw {
            -10 => Self::NotFound,
            -9 => Self::NotReady,
            -8 => Self::TargetLost,
            -7 => Self::Busy,
            -6 => Self::Cancelled,
            -5 => Self::NativeError,
            -4 => Self::Unsupported,
            -3 => Self::WrongThread,
            -2 => Self::InvalidState,
            -1 => Self::InvalidArgument,
            0 => Self::Ok,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> i32 {
        match self {
            Self::NotFound => -10,
            Self::NotReady => -9,
            Self::TargetLost => -8,
            Self::Busy => -7,
            Self::Cancelled => -6,
            Self::NativeError => -5,
            Self::Unsupported => -4,
            Self::WrongThread => -3,
            Self::InvalidState => -2,
            Self::InvalidArgument => -1,
            Self::Ok => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: i32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> i32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct StyleImageInfo {
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub byte_length: usize,
    pub stretch_x_count: usize,
    pub stretch_y_count: usize,
    pub content: Option<crate::generated::ImageContent>,
    pub text_fit_width: Option<crate::generated::StyleImageTextFit>,
    pub text_fit_height: Option<crate::generated::StyleImageTextFit>,
    pub pixel_ratio: f32,
    pub sdf: bool,
}
impl Default for StyleImageInfo {
    fn default() -> Self {
        Self::from_native(unsafe { maplibre_native_ffi_sys::mln_style_image_info_default() })
    }
}
impl StyleImageInfo {
    #[doc(hidden)]
    pub fn to_native(&self) -> maplibre_native_ffi_sys::mln_style_image_info {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_style_image_info_default() };
        raw.has_content = false;
        raw.has_text_fit_width = false;
        raw.has_text_fit_height = false;
        raw.width = self.width;
        raw.height = self.height;
        raw.stride = self.stride;
        raw.byte_length = self.byte_length;
        raw.stretch_x_count = self.stretch_x_count;
        raw.stretch_y_count = self.stretch_y_count;
        if let Some(value) = self.content {
            raw.has_content = true;
            raw.content = value.to_native();
        }
        if let Some(value) = self.text_fit_width {
            raw.has_text_fit_width = true;
            raw.text_fit_width = value.to_native();
        }
        if let Some(value) = self.text_fit_height {
            raw.has_text_fit_height = true;
            raw.text_fit_height = value.to_native();
        }
        raw.pixel_ratio = self.pixel_ratio;
        raw.sdf = self.sdf;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_style_image_info) -> Self {
        Self {
            width: raw.width,
            height: raw.height,
            stride: raw.stride,
            byte_length: raw.byte_length,
            stretch_x_count: raw.stretch_x_count,
            stretch_y_count: raw.stretch_y_count,
            content: (raw.has_content)
                .then(|| crate::generated::ImageContent::from_native(raw.content)),
            text_fit_width: (raw.has_text_fit_width)
                .then(|| crate::generated::StyleImageTextFit::from_native(raw.text_fit_width)),
            text_fit_height: (raw.has_text_fit_height)
                .then(|| crate::generated::StyleImageTextFit::from_native(raw.text_fit_height)),
            pixel_ratio: raw.pixel_ratio,
            sdf: raw.sdf,
        }
    }
}
impl crate::values::NativeValue for StyleImageInfo {
    type Raw = maplibre_native_ffi_sys::mln_style_image_info;
    fn to_native(self) -> Self::Raw {
        StyleImageInfo::to_native(&self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct StyleImageOptionField: u32 {
        const PIXEL_RATIO = 1;
        const SDF = 2;
        const STRETCH_X = 4;
        const STRETCH_Y = 8;
        const CONTENT = 16;
        const TEXT_FIT_WIDTH = 32;
        const TEXT_FIT_HEIGHT = 64;
        const _ = !0;
} }
impl StyleImageOptionField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct StyleImageOptions {
    pub stretch_x: Option<Vec<crate::generated::ImageStretch>>,
    pub stretch_y: Option<Vec<crate::generated::ImageStretch>>,
    pub content: Option<crate::generated::ImageContent>,
    pub text_fit_width: Option<crate::generated::StyleImageTextFit>,
    pub text_fit_height: Option<crate::generated::StyleImageTextFit>,
    pub pixel_ratio: Option<f32>,
    pub sdf: Option<bool>,
}
impl Default for StyleImageOptions {
    fn default() -> Self {
        unsafe {
            Self::from_native(maplibre_native_ffi_sys::mln_style_image_options_default())
                .expect("native default must be valid")
        }
    }
}
impl StyleImageOptions {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_style_image_options> {
        let mut raw: maplibre_native_ffi_sys::mln_style_image_options =
            unsafe { maplibre_native_ffi_sys::mln_style_image_options_default() };
        raw.fields = 0;
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_style_image_options>() as _;
        if let Some(item) = &self.stretch_x {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_IMAGE_OPTION_STRETCH_X;
            raw.stretch_x = {
                let items = (item)
                    .iter()
                    .map(|item| -> crate::Result<_> { Ok((item).to_native()) })
                    .collect::<crate::Result<Vec<_>>>()?;
                arena.array(items)
            };
        }
        raw.stretch_x_count = self
            .stretch_x
            .as_ref()
            .map_or(0, |items| items.len())
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;
        if let Some(item) = &self.stretch_y {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
            raw.stretch_y = {
                let items = (item)
                    .iter()
                    .map(|item| -> crate::Result<_> { Ok((item).to_native()) })
                    .collect::<crate::Result<Vec<_>>>()?;
                arena.array(items)
            };
        }
        raw.stretch_y_count = self
            .stretch_y
            .as_ref()
            .map_or(0, |items| items.len())
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;
        if let Some(item) = &self.content {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_IMAGE_OPTION_CONTENT;
            raw.content = (item).to_native();
        }
        if let Some(item) = &self.text_fit_width {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH;
            raw.text_fit_width = (item).to_native();
        }
        if let Some(item) = &self.text_fit_height {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
            raw.text_fit_height = (item).to_native();
        }
        if let Some(item) = &self.pixel_ratio {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
            raw.pixel_ratio = *(item);
        }
        if let Some(item) = &self.sdf {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_IMAGE_OPTION_SDF;
            raw.sdf = *(item);
        }
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_style_image_options,
    ) -> crate::Result<Self> {
        Ok(Self {
            stretch_x: if raw.fields & maplibre_native_ffi_sys::MLN_STYLE_IMAGE_OPTION_STRETCH_X
                != 0
            {
                Some(
                    unsafe { crate::input::slice(raw.stretch_x, raw.stretch_x_count as usize) }?
                        .iter()
                        .map(|item| -> crate::Result<_> {
                            Ok(crate::generated::ImageStretch::from_native(*item))
                        })
                        .collect::<crate::Result<Vec<_>>>()?,
                )
            } else {
                None
            },
            stretch_y: if raw.fields & maplibre_native_ffi_sys::MLN_STYLE_IMAGE_OPTION_STRETCH_Y
                != 0
            {
                Some(
                    unsafe { crate::input::slice(raw.stretch_y, raw.stretch_y_count as usize) }?
                        .iter()
                        .map(|item| -> crate::Result<_> {
                            Ok(crate::generated::ImageStretch::from_native(*item))
                        })
                        .collect::<crate::Result<Vec<_>>>()?,
                )
            } else {
                None
            },
            content: if raw.fields & maplibre_native_ffi_sys::MLN_STYLE_IMAGE_OPTION_CONTENT != 0 {
                Some(crate::generated::ImageContent::from_native(raw.content))
            } else {
                None
            },
            text_fit_width: if raw.fields
                & maplibre_native_ffi_sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH
                != 0
            {
                Some(crate::generated::StyleImageTextFit::from_native(
                    raw.text_fit_width,
                ))
            } else {
                None
            },
            text_fit_height: if raw.fields
                & maplibre_native_ffi_sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT
                != 0
            {
                Some(crate::generated::StyleImageTextFit::from_native(
                    raw.text_fit_height,
                ))
            } else {
                None
            },
            pixel_ratio: if raw.fields & maplibre_native_ffi_sys::MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO
                != 0
            {
                Some(raw.pixel_ratio)
            } else {
                None
            },
            sdf: if raw.fields & maplibre_native_ffi_sys::MLN_STYLE_IMAGE_OPTION_SDF != 0 {
                Some(raw.sdf)
            } else {
                None
            },
        })
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleImageResult {
    pub info: crate::generated::StyleImageInfo,
    pub pixels: Vec<u8>,
    pub stretch_x: Vec<crate::generated::ImageStretch>,
    pub stretch_y: Vec<crate::generated::ImageStretch>,
}

impl StyleImageResult {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_style_image_result> {
        let mut raw: maplibre_native_ffi_sys::mln_style_image_result =
            unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_style_image_result>() as _;
        raw.info = (&self.info).to_native();
        raw.pixels = maplibre_native_ffi_sys::mln_buffer_view {
            data: (&self.pixels).as_ptr().cast(),
            size: (&self.pixels).len(),
        };
        raw.stretch_x = {
            let items = (&self.stretch_x)
                .iter()
                .map(|item| -> crate::Result<_> { Ok((item).to_native()) })
                .collect::<crate::Result<Vec<_>>>()?;
            arena.array(items)
        };
        raw.stretch_x_count = self
            .stretch_x
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;
        raw.stretch_y = {
            let items = (&self.stretch_y)
                .iter()
                .map(|item| -> crate::Result<_> { Ok((item).to_native()) })
                .collect::<crate::Result<Vec<_>>>()?;
            arena.array(items)
        };
        raw.stretch_y_count = self
            .stretch_y
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_style_image_result,
    ) -> crate::Result<Self> {
        Ok(Self {
            info: crate::generated::StyleImageInfo::from_native(raw.info),
            pixels: unsafe { crate::string::copy_string_view_bytes(raw.pixels) }?,
            stretch_x: unsafe { crate::input::slice(raw.stretch_x, raw.stretch_x_count as usize) }?
                .iter()
                .map(|item| -> crate::Result<_> {
                    Ok(crate::generated::ImageStretch::from_native(*item))
                })
                .collect::<crate::Result<Vec<_>>>()?,
            stretch_y: unsafe { crate::input::slice(raw.stretch_y, raw.stretch_y_count as usize) }?
                .iter()
                .map(|item| -> crate::Result<_> {
                    Ok(crate::generated::ImageStretch::from_native(*item))
                })
                .collect::<crate::Result<Vec<_>>>()?,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleImageStretchesResult {
    pub stretch_x: Vec<crate::generated::ImageStretch>,
    pub stretch_y: Vec<crate::generated::ImageStretch>,
}

impl StyleImageStretchesResult {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_style_image_stretches_result> {
        let mut raw: maplibre_native_ffi_sys::mln_style_image_stretches_result =
            unsafe { std::mem::zeroed() };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_style_image_stretches_result>() as _;
        raw.stretch_x = {
            let items = (&self.stretch_x)
                .iter()
                .map(|item| -> crate::Result<_> { Ok((item).to_native()) })
                .collect::<crate::Result<Vec<_>>>()?;
            arena.array(items)
        };
        raw.stretch_x_count = self
            .stretch_x
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;
        raw.stretch_y = {
            let items = (&self.stretch_y)
                .iter()
                .map(|item| -> crate::Result<_> { Ok((item).to_native()) })
                .collect::<crate::Result<Vec<_>>>()?;
            arena.array(items)
        };
        raw.stretch_y_count = self
            .stretch_y
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_style_image_stretches_result,
    ) -> crate::Result<Self> {
        Ok(Self {
            stretch_x: unsafe { crate::input::slice(raw.stretch_x, raw.stretch_x_count as usize) }?
                .iter()
                .map(|item| -> crate::Result<_> {
                    Ok(crate::generated::ImageStretch::from_native(*item))
                })
                .collect::<crate::Result<Vec<_>>>()?,
            stretch_y: unsafe { crate::input::slice(raw.stretch_y, raw.stretch_y_count as usize) }?
                .iter()
                .map(|item| -> crate::Result<_> {
                    Ok(crate::generated::ImageStretch::from_native(*item))
                })
                .collect::<crate::Result<Vec<_>>>()?,
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum StyleImageTextFit {
    Proportional,
    StretchOnly,
    StretchOrShrink,
    Unknown(u32),
}
impl Default for StyleImageTextFit {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl StyleImageTextFit {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            2 => Self::Proportional,
            1 => Self::StretchOnly,
            0 => Self::StretchOrShrink,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Proportional => 2,
            Self::StretchOnly => 1,
            Self::StretchOrShrink => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleLayerEntry {
    pub id: String,
    pub r#type: String,
    pub source_id: Option<String>,
    pub source_layer: Option<String>,
}

impl StyleLayerEntry {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_style_layer_entry> {
        let mut raw: maplibre_native_ffi_sys::mln_style_layer_entry = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_style_layer_entry>() as _;
        raw.id = maplibre_native_ffi_sys::mln_buffer_view {
            data: (&self.id).as_bytes().as_ptr().cast(),
            size: (&self.id).as_bytes().len(),
        };
        raw.type_ = maplibre_native_ffi_sys::mln_buffer_view {
            data: (&self.r#type).as_bytes().as_ptr().cast(),
            size: (&self.r#type).as_bytes().len(),
        };
        raw.source_id = match (&self.source_id).as_ref() {
            Some(item) => maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            },
            None => maplibre_native_ffi_sys::mln_buffer_view {
                data: std::ptr::null(),
                size: 0,
            },
        };
        raw.source_layer = match (&self.source_layer).as_ref() {
            Some(item) => maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            },
            None => maplibre_native_ffi_sys::mln_buffer_view {
                data: std::ptr::null(),
                size: 0,
            },
        };
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_style_layer_entry,
    ) -> crate::Result<Self> {
        Ok(Self {
            id: unsafe { crate::string::copy_string_view(raw.id) }?,
            r#type: unsafe { crate::string::copy_string_view(raw.type_) }?,
            source_id: if raw.source_id.size == 0 {
                None
            } else {
                Some(unsafe { crate::string::copy_string_view(raw.source_id) }?)
            },
            source_layer: if raw.source_layer.size == 0 {
                None
            } else {
                Some(unsafe { crate::string::copy_string_view(raw.source_layer) }?)
            },
        })
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleLayerInfo {
    pub r#type: String,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub visibility: crate::generated::StyleLayerVisibility,
}

impl StyleLayerInfo {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_style_layer_info> {
        let mut raw: maplibre_native_ffi_sys::mln_style_layer_info = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_style_layer_info>() as _;
        raw.type_ = maplibre_native_ffi_sys::mln_buffer_view {
            data: (&self.r#type).as_bytes().as_ptr().cast(),
            size: (&self.r#type).as_bytes().len(),
        };
        raw.min_zoom = *(&self.min_zoom);
        raw.max_zoom = *(&self.max_zoom);
        raw.visibility = (&self.visibility).to_native();
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_style_layer_info,
    ) -> crate::Result<Self> {
        Ok(Self {
            r#type: unsafe { crate::string::copy_string_view(raw.type_) }?,
            min_zoom: raw.min_zoom,
            max_zoom: raw.max_zoom,
            visibility: crate::generated::StyleLayerVisibility::from_native(raw.visibility),
        })
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleLayerResult {
    pub info: crate::generated::StyleLayerInfo,
    pub source_id: Option<String>,
    pub source_layer: Option<String>,
}

impl StyleLayerResult {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_style_layer_result> {
        let mut raw: maplibre_native_ffi_sys::mln_style_layer_result =
            unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_style_layer_result>() as _;
        raw.info = (&self.info).to_native(arena)?;
        raw.source_id = match (&self.source_id).as_ref() {
            Some(item) => maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            },
            None => maplibre_native_ffi_sys::mln_buffer_view {
                data: std::ptr::null(),
                size: 0,
            },
        };
        raw.source_layer = match (&self.source_layer).as_ref() {
            Some(item) => maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            },
            None => maplibre_native_ffi_sys::mln_buffer_view {
                data: std::ptr::null(),
                size: 0,
            },
        };
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_style_layer_result,
    ) -> crate::Result<Self> {
        Ok(Self {
            info: unsafe { crate::generated::StyleLayerInfo::from_native(raw.info) }?,
            source_id: if raw.source_id.size == 0 {
                None
            } else {
                Some(unsafe { crate::string::copy_string_view(raw.source_id) }?)
            },
            source_layer: if raw.source_layer.size == 0 {
                None
            } else {
                Some(unsafe { crate::string::copy_string_view(raw.source_layer) }?)
            },
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum StyleLayerVisibility {
    None,
    Visible,
    Unknown(u32),
}
impl Default for StyleLayerVisibility {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl StyleLayerVisibility {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::None,
            0 => Self::Visible,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::None => 1,
            Self::Visible => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum StyleRasterDemEncoding {
    Terrarium,
    Mapbox,
    Unknown(u32),
}
impl Default for StyleRasterDemEncoding {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl StyleRasterDemEncoding {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::Terrarium,
            0 => Self::Mapbox,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Terrarium => 1,
            Self::Mapbox => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct StyleSourceInfo {
    pub tilejson: Option<crate::generated::StyleSourceTileInfo>,
    pub r#type: crate::generated::StyleSourceType,
    pub id_size: usize,
    pub is_volatile: bool,
    pub attribution_size: Option<usize>,
    pub url_size: Option<usize>,
    pub bounds: Option<crate::generated::LatLngBounds>,
    pub tile_size: Option<u32>,
    pub vector_encoding: Option<crate::generated::StyleVectorTileEncoding>,
    pub raster_encoding: Option<crate::generated::StyleRasterDemEncoding>,
}

impl StyleSourceInfo {
    #[doc(hidden)]
    pub fn to_native(&self) -> maplibre_native_ffi_sys::mln_style_source_info {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_style_source_info>() };
        raw.fields = 0;
        raw.has_attribution = false;
        if let Some(value) = self.tilejson {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_SOURCE_INFO_TILEJSON;
            raw.tile_count = value.tile_count;
            raw.min_zoom = value.min_zoom;
            raw.max_zoom = value.max_zoom;
            raw.scheme = value.scheme.to_native();
        }
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_style_source_info>() as _;
        raw.type_ = self.r#type.to_native();
        raw.id_size = self.id_size;
        raw.is_volatile = self.is_volatile;
        if let Some(value) = self.attribution_size {
            raw.has_attribution = true;
            raw.attribution_size = value;
        }
        if let Some(value) = self.url_size {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_SOURCE_INFO_URL;
            raw.url_size = value;
        }
        if let Some(value) = self.bounds {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_SOURCE_INFO_BOUNDS;
            raw.bounds = value.to_native();
        }
        if let Some(value) = self.tile_size {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_SOURCE_INFO_TILE_SIZE;
            raw.tile_size = value;
        }
        if let Some(value) = self.vector_encoding {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING;
            raw.vector_encoding = value.to_native();
        }
        if let Some(value) = self.raster_encoding {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_SOURCE_INFO_RASTER_ENCODING;
            raw.raster_encoding = value.to_native();
        }
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_style_source_info) -> Self {
        Self {
            tilejson: (raw.fields & maplibre_native_ffi_sys::MLN_STYLE_SOURCE_INFO_TILEJSON != 0)
                .then(|| crate::generated::StyleSourceTileInfo {
                    tile_count: raw.tile_count,
                    min_zoom: raw.min_zoom,
                    max_zoom: raw.max_zoom,
                    scheme: crate::generated::StyleTileScheme::from_native(raw.scheme),
                }),
            r#type: crate::generated::StyleSourceType::from_native(raw.type_),
            id_size: raw.id_size,
            is_volatile: raw.is_volatile,
            attribution_size: (raw.has_attribution).then(|| raw.attribution_size),
            url_size: (raw.fields & maplibre_native_ffi_sys::MLN_STYLE_SOURCE_INFO_URL != 0)
                .then(|| raw.url_size),
            bounds: (raw.fields & maplibre_native_ffi_sys::MLN_STYLE_SOURCE_INFO_BOUNDS != 0)
                .then(|| crate::generated::LatLngBounds::from_native(raw.bounds)),
            tile_size: (raw.fields & maplibre_native_ffi_sys::MLN_STYLE_SOURCE_INFO_TILE_SIZE != 0)
                .then(|| raw.tile_size),
            vector_encoding: (raw.fields
                & maplibre_native_ffi_sys::MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING
                != 0)
                .then(|| {
                    crate::generated::StyleVectorTileEncoding::from_native(raw.vector_encoding)
                }),
            raster_encoding: (raw.fields
                & maplibre_native_ffi_sys::MLN_STYLE_SOURCE_INFO_RASTER_ENCODING
                != 0)
                .then(|| {
                    crate::generated::StyleRasterDemEncoding::from_native(raw.raster_encoding)
                }),
        }
    }
}
impl crate::values::NativeValue for StyleSourceInfo {
    type Raw = maplibre_native_ffi_sys::mln_style_source_info;
    fn to_native(self) -> Self::Raw {
        StyleSourceInfo::to_native(&self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct StyleSourceInfoField: u32 {
        const URL = 1;
        const TILEJSON = 2;
        const BOUNDS = 4;
        const TILE_SIZE = 8;
        const VECTOR_ENCODING = 16;
        const RASTER_ENCODING = 32;
        const _ = !0;
} }
impl StyleSourceInfoField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleSourceResult {
    pub info: crate::generated::StyleSourceInfo,
    pub attribution: Option<String>,
    pub url: Option<String>,
    pub tile_urls: Option<Vec<String>>,
}

impl StyleSourceResult {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_style_source_result> {
        let mut raw: maplibre_native_ffi_sys::mln_style_source_result =
            unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_style_source_result>() as _;
        raw.info = (&self.info).to_native();
        if let Some(item) = &self.attribution {
            raw.info.has_attribution = true;
            raw.attribution = maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            };
        }
        if let Some(item) = &self.url {
            raw.info.fields |= maplibre_native_ffi_sys::MLN_STYLE_SOURCE_INFO_URL;
            raw.url = maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            };
        }
        if let Some(item) = &self.tile_urls {
            raw.info.fields |= maplibre_native_ffi_sys::MLN_STYLE_SOURCE_INFO_TILEJSON;
            raw.tile_urls = {
                let items = (item)
                    .iter()
                    .map(|item| -> crate::Result<_> {
                        Ok(maplibre_native_ffi_sys::mln_buffer_view {
                            data: (item).as_bytes().as_ptr().cast(),
                            size: (item).as_bytes().len(),
                        })
                    })
                    .collect::<crate::Result<Vec<_>>>()?;
                arena.array(items)
            };
        }
        raw.tile_url_count = self
            .tile_urls
            .as_ref()
            .map_or(0, |items| items.len())
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_style_source_result,
    ) -> crate::Result<Self> {
        Ok(Self {
            info: crate::generated::StyleSourceInfo::from_native(raw.info),
            attribution: if raw.info.has_attribution {
                Some(unsafe { crate::string::copy_string_view(raw.attribution) }?)
            } else {
                None
            },
            url: if raw.info.fields & maplibre_native_ffi_sys::MLN_STYLE_SOURCE_INFO_URL != 0 {
                Some(unsafe { crate::string::copy_string_view(raw.url) }?)
            } else {
                None
            },
            tile_urls: if raw.info.fields & maplibre_native_ffi_sys::MLN_STYLE_SOURCE_INFO_TILEJSON
                != 0
            {
                Some(
                    unsafe { crate::input::slice(raw.tile_urls, raw.tile_url_count as usize) }?
                        .iter()
                        .map(|item| -> crate::Result<_> {
                            Ok(unsafe { crate::string::copy_string_view(*item) }?)
                        })
                        .collect::<crate::Result<Vec<_>>>()?,
                )
            } else {
                None
            },
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct StyleSourceTileInfo {
    pub tile_count: usize,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub scheme: crate::generated::StyleTileScheme,
}

impl StyleSourceTileInfo {
    pub const fn new(
        tile_count: usize,
        min_zoom: f64,
        max_zoom: f64,
        scheme: crate::generated::StyleTileScheme,
    ) -> Self {
        Self {
            tile_count,
            min_zoom,
            max_zoom,
            scheme,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_style_source_tile_info {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_style_source_tile_info>() };
        raw.tile_count = self.tile_count;
        raw.min_zoom = self.min_zoom;
        raw.max_zoom = self.max_zoom;
        raw.scheme = self.scheme.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_style_source_tile_info) -> Self {
        Self {
            tile_count: raw.tile_count,
            min_zoom: raw.min_zoom,
            max_zoom: raw.max_zoom,
            scheme: crate::generated::StyleTileScheme::from_native(raw.scheme),
        }
    }
}
impl crate::values::NativeValue for StyleSourceTileInfo {
    type Raw = maplibre_native_ffi_sys::mln_style_source_tile_info;
    fn to_native(self) -> Self::Raw {
        StyleSourceTileInfo::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleSourceTileUrlsResult {
    pub tile_urls: Vec<String>,
}

impl StyleSourceTileUrlsResult {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_style_source_tile_urls_result> {
        let mut raw: maplibre_native_ffi_sys::mln_style_source_tile_urls_result =
            unsafe { std::mem::zeroed() };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_style_source_tile_urls_result>() as _;
        raw.tile_urls = {
            let items = (&self.tile_urls)
                .iter()
                .map(|item| -> crate::Result<_> {
                    Ok(maplibre_native_ffi_sys::mln_buffer_view {
                        data: (item).as_bytes().as_ptr().cast(),
                        size: (item).as_bytes().len(),
                    })
                })
                .collect::<crate::Result<Vec<_>>>()?;
            arena.array(items)
        };
        raw.tile_url_count = self
            .tile_urls
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_style_source_tile_urls_result,
    ) -> crate::Result<Self> {
        Ok(Self {
            tile_urls: unsafe { crate::input::slice(raw.tile_urls, raw.tile_url_count as usize) }?
                .iter()
                .map(|item| -> crate::Result<_> {
                    Ok(unsafe { crate::string::copy_string_view(*item) }?)
                })
                .collect::<crate::Result<Vec<_>>>()?,
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum StyleSourceType {
    CustomMvtVector,
    CustomVector,
    Annotations,
    Video,
    Image,
    Geojson,
    RasterDem,
    Raster,
    Vector,
    Unknown,
    Unrecognized(u32),
}
impl Default for StyleSourceType {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl StyleSourceType {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            9 => Self::CustomMvtVector,
            8 => Self::CustomVector,
            7 => Self::Annotations,
            6 => Self::Video,
            5 => Self::Image,
            4 => Self::Geojson,
            3 => Self::RasterDem,
            2 => Self::Raster,
            1 => Self::Vector,
            0 => Self::Unknown,
            value => Self::Unrecognized(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::CustomMvtVector => 9,
            Self::CustomVector => 8,
            Self::Annotations => 7,
            Self::Video => 6,
            Self::Image => 5,
            Self::Geojson => 4,
            Self::RasterDem => 3,
            Self::Raster => 2,
            Self::Vector => 1,
            Self::Unknown => 0,
            Self::Unrecognized(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum StyleTileScheme {
    Tms,
    Xyz,
    Unknown(u32),
}
impl Default for StyleTileScheme {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl StyleTileScheme {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::Tms,
            0 => Self::Xyz,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Tms => 1,
            Self::Xyz => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct StyleTileSourceOptionField: u32 {
        const MIN_ZOOM = 1;
        const MAX_ZOOM = 2;
        const ATTRIBUTION = 4;
        const SCHEME = 8;
        const BOUNDS = 16;
        const TILE_SIZE = 32;
        const VECTOR_ENCODING = 64;
        const RASTER_ENCODING = 128;
        const _ = !0;
} }
impl StyleTileSourceOptionField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct StyleTileSourceOptions {
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
    pub attribution: Option<String>,
    pub scheme: Option<crate::generated::StyleTileScheme>,
    pub bounds: Option<crate::generated::LatLngBounds>,
    pub tile_size: Option<u32>,
    pub vector_encoding: Option<crate::generated::StyleVectorTileEncoding>,
    pub raster_encoding: Option<crate::generated::StyleRasterDemEncoding>,
}
impl Default for StyleTileSourceOptions {
    fn default() -> Self {
        unsafe {
            Self::from_native(maplibre_native_ffi_sys::mln_style_tile_source_options_default())
                .expect("native default must be valid")
        }
    }
}
impl StyleTileSourceOptions {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_style_tile_source_options> {
        let mut raw: maplibre_native_ffi_sys::mln_style_tile_source_options =
            unsafe { maplibre_native_ffi_sys::mln_style_tile_source_options_default() };
        raw.fields = 0;
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_style_tile_source_options>() as _;
        if let Some(item) = &self.min_zoom {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM;
            raw.min_zoom = *(item);
        }
        if let Some(item) = &self.max_zoom {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM;
            raw.max_zoom = *(item);
        }
        if let Some(item) = &self.attribution {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
            raw.attribution = maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            };
        }
        if let Some(item) = &self.scheme {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_TILE_SOURCE_OPTION_SCHEME;
            raw.scheme = (item).to_native();
        }
        if let Some(item) = &self.bounds {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS;
            raw.bounds = (item).to_native();
        }
        if let Some(item) = &self.tile_size {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE;
            raw.tile_size = *(item);
        }
        if let Some(item) = &self.vector_encoding {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
            raw.vector_encoding = (item).to_native();
        }
        if let Some(item) = &self.raster_encoding {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
            raw.raster_encoding = (item).to_native();
        }
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_style_tile_source_options,
    ) -> crate::Result<Self> {
        Ok(Self {
            min_zoom: if raw.fields & maplibre_native_ffi_sys::MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM
                != 0
            {
                Some(raw.min_zoom)
            } else {
                None
            },
            max_zoom: if raw.fields & maplibre_native_ffi_sys::MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM
                != 0
            {
                Some(raw.max_zoom)
            } else {
                None
            },
            attribution: if raw.fields
                & maplibre_native_ffi_sys::MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION
                != 0
            {
                Some(unsafe { crate::string::copy_string_view(raw.attribution) }?)
            } else {
                None
            },
            scheme: if raw.fields & maplibre_native_ffi_sys::MLN_STYLE_TILE_SOURCE_OPTION_SCHEME
                != 0
            {
                Some(crate::generated::StyleTileScheme::from_native(raw.scheme))
            } else {
                None
            },
            bounds: if raw.fields & maplibre_native_ffi_sys::MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS
                != 0
            {
                Some(crate::generated::LatLngBounds::from_native(raw.bounds))
            } else {
                None
            },
            tile_size: if raw.fields
                & maplibre_native_ffi_sys::MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE
                != 0
            {
                Some(raw.tile_size)
            } else {
                None
            },
            vector_encoding: if raw.fields
                & maplibre_native_ffi_sys::MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING
                != 0
            {
                Some(crate::generated::StyleVectorTileEncoding::from_native(
                    raw.vector_encoding,
                ))
            } else {
                None
            },
            raster_encoding: if raw.fields
                & maplibre_native_ffi_sys::MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
                != 0
            {
                Some(crate::generated::StyleRasterDemEncoding::from_native(
                    raw.raster_encoding,
                ))
            } else {
                None
            },
        })
    }
}

bitflags::bitflags! { #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct StyleTransitionOptionField: u32 {
        const DURATION = 1;
        const DELAY = 2;
        const ENABLE_PLACEMENT_TRANSITIONS = 4;
        const _ = !0;
} }
impl StyleTransitionOptionField {
    pub fn from_native(raw: u32) -> Self {
        Self::from_bits_retain(raw)
    }
    pub fn to_native(self) -> u32 {
        self.bits()
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct StyleTransitionOptions {
    pub duration_ms: Option<f64>,
    pub delay_ms: Option<f64>,
    pub enable_placement_transitions: Option<bool>,
}
impl Default for StyleTransitionOptions {
    fn default() -> Self {
        Self::from_native(unsafe {
            maplibre_native_ffi_sys::mln_style_transition_options_default()
        })
    }
}
impl StyleTransitionOptions {
    #[doc(hidden)]
    pub fn to_native(&self) -> maplibre_native_ffi_sys::mln_style_transition_options {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_style_transition_options_default() };
        raw.fields = 0;
        if let Some(value) = self.duration_ms {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_TRANSITION_OPTION_DURATION;
            raw.duration_ms = value;
        }
        if let Some(value) = self.delay_ms {
            raw.fields |= maplibre_native_ffi_sys::MLN_STYLE_TRANSITION_OPTION_DELAY;
            raw.delay_ms = value;
        }
        if let Some(value) = self.enable_placement_transitions {
            raw.fields |=
                maplibre_native_ffi_sys::MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
            raw.enable_placement_transitions = value;
        }
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_style_transition_options) -> Self {
        Self {
            duration_ms: (raw.fields
                & maplibre_native_ffi_sys::MLN_STYLE_TRANSITION_OPTION_DURATION
                != 0)
                .then(|| raw.duration_ms),
            delay_ms: (raw.fields & maplibre_native_ffi_sys::MLN_STYLE_TRANSITION_OPTION_DELAY
                != 0)
                .then(|| raw.delay_ms),
            enable_placement_transitions: (raw.fields
                & maplibre_native_ffi_sys::MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS
                != 0)
                .then(|| raw.enable_placement_transitions),
        }
    }
}
impl crate::values::NativeValue for StyleTransitionOptions {
    type Raw = maplibre_native_ffi_sys::mln_style_transition_options;
    fn to_native(self) -> Self::Raw {
        StyleTransitionOptions::to_native(&self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum StyleVectorTileEncoding {
    Mlt,
    Mvt,
    Unknown(u32),
}
impl Default for StyleVectorTileEncoding {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl StyleVectorTileEncoding {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::Mlt,
            0 => Self::Mvt,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Mlt => 1,
            Self::Mvt => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct TextureImageInfo {
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub byte_length: usize,
}
impl Default for TextureImageInfo {
    fn default() -> Self {
        Self::from_native(unsafe { maplibre_native_ffi_sys::mln_texture_image_info_default() })
    }
}
impl TextureImageInfo {
    pub const fn new(width: u32, height: u32, stride: u32, byte_length: usize) -> Self {
        Self {
            width,
            height,
            stride,
            byte_length,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_texture_image_info {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_texture_image_info_default() };
        raw.width = self.width;
        raw.height = self.height;
        raw.stride = self.stride;
        raw.byte_length = self.byte_length;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_texture_image_info) -> Self {
        Self {
            width: raw.width,
            height: raw.height,
            stride: raw.stride,
            byte_length: raw.byte_length,
        }
    }
}
impl crate::values::NativeValue for TextureImageInfo {
    type Raw = maplibre_native_ffi_sys::mln_texture_image_info;
    fn to_native(self) -> Self::Raw {
        TextureImageInfo::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct TextureReadbackResult {
    pub data: Vec<u8>,
    pub info: crate::generated::TextureImageInfo,
}

impl TextureReadbackResult {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_texture_readback_result> {
        let mut raw: maplibre_native_ffi_sys::mln_texture_readback_result =
            unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_texture_readback_result>() as _;
        raw.data = maplibre_native_ffi_sys::mln_buffer_view {
            data: (&self.data).as_ptr().cast(),
            size: (&self.data).len(),
        };
        raw.info = (&self.info).to_native();
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_texture_readback_result,
    ) -> crate::Result<Self> {
        Ok(Self {
            data: unsafe { crate::string::copy_string_view_bytes(raw.data) }?,
            info: crate::generated::TextureImageInfo::from_native(raw.info),
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct TileId {
    pub overscaled_z: u32,
    pub wrap: i32,
    pub canonical_z: u32,
    pub canonical_x: u32,
    pub canonical_y: u32,
}

impl TileId {
    pub const fn new(
        overscaled_z: u32,
        wrap: i32,
        canonical_z: u32,
        canonical_x: u32,
        canonical_y: u32,
    ) -> Self {
        Self {
            overscaled_z,
            wrap,
            canonical_z,
            canonical_x,
            canonical_y,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_tile_id {
        let mut raw = unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_tile_id>() };
        raw.overscaled_z = self.overscaled_z;
        raw.wrap = self.wrap;
        raw.canonical_z = self.canonical_z;
        raw.canonical_x = self.canonical_x;
        raw.canonical_y = self.canonical_y;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_tile_id) -> Self {
        Self {
            overscaled_z: raw.overscaled_z,
            wrap: raw.wrap,
            canonical_z: raw.canonical_z,
            canonical_x: raw.canonical_x,
            canonical_y: raw.canonical_y,
        }
    }
}
impl crate::values::NativeValue for TileId {
    type Raw = maplibre_native_ffi_sys::mln_tile_id;
    fn to_native(self) -> Self::Raw {
        TileId::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum TileLodMode {
    Distance,
    Default,
    Unknown(u32),
}
impl Default for TileLodMode {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl TileLodMode {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::Distance,
            0 => Self::Default,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Distance => 1,
            Self::Default => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum TileOperation {
    Null,
    Cancelled,
    Error,
    EndParse,
    StartParse,
    LoadFromCache,
    LoadFromNetwork,
    RequestedFromNetwork,
    RequestedFromCache,
    Unknown(u32),
}
impl Default for TileOperation {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl TileOperation {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            8 => Self::Null,
            7 => Self::Cancelled,
            6 => Self::Error,
            5 => Self::EndParse,
            4 => Self::StartParse,
            3 => Self::LoadFromCache,
            2 => Self::LoadFromNetwork,
            1 => Self::RequestedFromNetwork,
            0 => Self::RequestedFromCache,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Null => 8,
            Self::Cancelled => 7,
            Self::Error => 6,
            Self::EndParse => 5,
            Self::StartParse => 4,
            Self::LoadFromCache => 3,
            Self::LoadFromNetwork => 2,
            Self::RequestedFromNetwork => 1,
            Self::RequestedFromCache => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct UnitBezier {
    pub x1: f64,
    pub y1: f64,
    pub x2: f64,
    pub y2: f64,
}

impl UnitBezier {
    pub const fn new(x1: f64, y1: f64, x2: f64, y2: f64) -> Self {
        Self { x1, y1, x2, y2 }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_unit_bezier {
        let mut raw = unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_unit_bezier>() };
        raw.x1 = self.x1;
        raw.y1 = self.y1;
        raw.x2 = self.x2;
        raw.y2 = self.y2;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_unit_bezier) -> Self {
        Self {
            x1: raw.x1,
            y1: raw.y1,
            x2: raw.x2,
            y2: raw.y2,
        }
    }
}
impl crate::values::NativeValue for UnitBezier {
    type Raw = maplibre_native_ffi_sys::mln_unit_bezier;
    fn to_native(self) -> Self::Raw {
        UnitBezier::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Vec3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

impl Vec3 {
    pub const fn new(x: f64, y: f64, z: f64) -> Self {
        Self { x, y, z }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_vec3 {
        let mut raw = unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_vec3>() };
        raw.x = self.x;
        raw.y = self.y;
        raw.z = self.z;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_vec3) -> Self {
        Self {
            x: raw.x,
            y: raw.y,
            z: raw.z,
        }
    }
}
impl crate::values::NativeValue for Vec3 {
    type Raw = maplibre_native_ffi_sys::mln_vec3;
    fn to_native(self) -> Self::Raw {
        Vec3::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum ViewportMode {
    FlippedY,
    Default,
    Unknown(u32),
}
impl Default for ViewportMode {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl ViewportMode {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::FlippedY,
            0 => Self::Default,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::FlippedY => 1,
            Self::Default => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanBorrowedTextureDescriptor {
    pub extent: crate::generated::RenderTargetExtent,
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: crate::generated::VulkanContextDescriptor,
    pub image: u64,
    pub image_view: u64,
    pub format: u32,
    pub initial_layout: u32,
    pub final_layout: u32,
}
impl Default for VulkanBorrowedTextureDescriptor {
    fn default() -> Self {
        Self::from_native(unsafe {
            maplibre_native_ffi_sys::mln_vulkan_borrowed_texture_descriptor_default()
        })
    }
}
impl VulkanBorrowedTextureDescriptor {
    pub const fn new(
        extent: crate::generated::RenderTargetExtent,
        physical_width: u32,
        physical_height: u32,
        context: crate::generated::VulkanContextDescriptor,
        image: u64,
        image_view: u64,
        format: u32,
        initial_layout: u32,
        final_layout: u32,
    ) -> Self {
        Self {
            extent,
            physical_width,
            physical_height,
            context,
            image,
            image_view,
            format,
            initial_layout,
            final_layout,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_vulkan_borrowed_texture_descriptor {
        let mut raw =
            unsafe { maplibre_native_ffi_sys::mln_vulkan_borrowed_texture_descriptor_default() };
        raw.extent = self.extent.to_native();
        raw.physical_width = self.physical_width;
        raw.physical_height = self.physical_height;
        raw.context = self.context.to_native();
        raw.image = self.image;
        raw.image_view = self.image_view;
        raw.format = self.format;
        raw.initial_layout = self.initial_layout;
        raw.final_layout = self.final_layout;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(
        raw: maplibre_native_ffi_sys::mln_vulkan_borrowed_texture_descriptor,
    ) -> Self {
        Self {
            extent: crate::generated::RenderTargetExtent::from_native(raw.extent),
            physical_width: raw.physical_width,
            physical_height: raw.physical_height,
            context: crate::generated::VulkanContextDescriptor::from_native(raw.context),
            image: raw.image,
            image_view: raw.image_view,
            format: raw.format,
            initial_layout: raw.initial_layout,
            final_layout: raw.final_layout,
        }
    }
}
impl crate::values::NativeValue for VulkanBorrowedTextureDescriptor {
    type Raw = maplibre_native_ffi_sys::mln_vulkan_borrowed_texture_descriptor;
    fn to_native(self) -> Self::Raw {
        VulkanBorrowedTextureDescriptor::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct VulkanContextDescriptor {
    pub instance: *mut std::ffi::c_void,
    pub physical_device: *mut std::ffi::c_void,
    pub device: *mut std::ffi::c_void,
    pub graphics_queue: *mut std::ffi::c_void,
    pub graphics_queue_family_index: u32,
    pub get_instance_proc_addr: *mut std::ffi::c_void,
    pub get_device_proc_addr: *mut std::ffi::c_void,
}

impl VulkanContextDescriptor {
    pub const fn new(
        instance: *mut std::ffi::c_void,
        physical_device: *mut std::ffi::c_void,
        device: *mut std::ffi::c_void,
        graphics_queue: *mut std::ffi::c_void,
        graphics_queue_family_index: u32,
        get_instance_proc_addr: *mut std::ffi::c_void,
        get_device_proc_addr: *mut std::ffi::c_void,
    ) -> Self {
        Self {
            instance,
            physical_device,
            device,
            graphics_queue,
            graphics_queue_family_index,
            get_instance_proc_addr,
            get_device_proc_addr,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_vulkan_context_descriptor {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_vulkan_context_descriptor>() };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_vulkan_context_descriptor>() as _;
        raw.instance = self.instance;
        raw.physical_device = self.physical_device;
        raw.device = self.device;
        raw.graphics_queue = self.graphics_queue;
        raw.graphics_queue_family_index = self.graphics_queue_family_index;
        raw.get_instance_proc_addr = self.get_instance_proc_addr;
        raw.get_device_proc_addr = self.get_device_proc_addr;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_vulkan_context_descriptor) -> Self {
        Self {
            instance: raw.instance,
            physical_device: raw.physical_device,
            device: raw.device,
            graphics_queue: raw.graphics_queue,
            graphics_queue_family_index: raw.graphics_queue_family_index,
            get_instance_proc_addr: raw.get_instance_proc_addr,
            get_device_proc_addr: raw.get_device_proc_addr,
        }
    }
}
impl crate::values::NativeValue for VulkanContextDescriptor {
    type Raw = maplibre_native_ffi_sys::mln_vulkan_context_descriptor;
    fn to_native(self) -> Self::Raw {
        VulkanContextDescriptor::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanOwnedTextureDescriptor {
    pub extent: crate::generated::RenderTargetExtent,
    pub context: crate::generated::VulkanContextDescriptor,
}
impl Default for VulkanOwnedTextureDescriptor {
    fn default() -> Self {
        Self::from_native(unsafe {
            maplibre_native_ffi_sys::mln_vulkan_owned_texture_descriptor_default()
        })
    }
}
impl VulkanOwnedTextureDescriptor {
    pub const fn new(
        extent: crate::generated::RenderTargetExtent,
        context: crate::generated::VulkanContextDescriptor,
    ) -> Self {
        Self { extent, context }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_vulkan_owned_texture_descriptor {
        let mut raw =
            unsafe { maplibre_native_ffi_sys::mln_vulkan_owned_texture_descriptor_default() };
        raw.extent = self.extent.to_native();
        raw.context = self.context.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_vulkan_owned_texture_descriptor) -> Self {
        Self {
            extent: crate::generated::RenderTargetExtent::from_native(raw.extent),
            context: crate::generated::VulkanContextDescriptor::from_native(raw.context),
        }
    }
}
impl crate::values::NativeValue for VulkanOwnedTextureDescriptor {
    type Raw = maplibre_native_ffi_sys::mln_vulkan_owned_texture_descriptor;
    fn to_native(self) -> Self::Raw {
        VulkanOwnedTextureDescriptor::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct VulkanOwnedTextureFrame {
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub image: u64,
    pub image_view: u64,
    pub device: *mut std::ffi::c_void,
    pub format: u32,
    pub layout: u32,
}

impl VulkanOwnedTextureFrame {
    pub const fn new(
        generation: u64,
        width: u32,
        height: u32,
        scale_factor: f64,
        frame_id: u64,
        image: u64,
        image_view: u64,
        device: *mut std::ffi::c_void,
        format: u32,
        layout: u32,
    ) -> Self {
        Self {
            generation,
            width,
            height,
            scale_factor,
            frame_id,
            image,
            image_view,
            device,
            format,
            layout,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_vulkan_owned_texture_frame {
        let mut raw = unsafe {
            std::mem::zeroed::<maplibre_native_ffi_sys::mln_vulkan_owned_texture_frame>()
        };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_vulkan_owned_texture_frame>() as _;
        raw.generation = self.generation;
        raw.width = self.width;
        raw.height = self.height;
        raw.scale_factor = self.scale_factor;
        raw.frame_id = self.frame_id;
        raw.image = self.image;
        raw.image_view = self.image_view;
        raw.device = self.device;
        raw.format = self.format;
        raw.layout = self.layout;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_vulkan_owned_texture_frame) -> Self {
        Self {
            generation: raw.generation,
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: raw.frame_id,
            image: raw.image,
            image_view: raw.image_view,
            device: raw.device,
            format: raw.format,
            layout: raw.layout,
        }
    }
}
impl crate::values::NativeValue for VulkanOwnedTextureFrame {
    type Raw = maplibre_native_ffi_sys::mln_vulkan_owned_texture_frame;
    fn to_native(self) -> Self::Raw {
        VulkanOwnedTextureFrame::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanSurfaceDescriptor {
    pub extent: crate::generated::RenderTargetExtent,
    pub context: crate::generated::VulkanContextDescriptor,
    pub surface: u64,
}
impl Default for VulkanSurfaceDescriptor {
    fn default() -> Self {
        Self::from_native(unsafe {
            maplibre_native_ffi_sys::mln_vulkan_surface_descriptor_default()
        })
    }
}
impl VulkanSurfaceDescriptor {
    pub const fn new(
        extent: crate::generated::RenderTargetExtent,
        context: crate::generated::VulkanContextDescriptor,
        surface: u64,
    ) -> Self {
        Self {
            extent,
            context,
            surface,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_vulkan_surface_descriptor {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_vulkan_surface_descriptor_default() };
        raw.extent = self.extent.to_native();
        raw.context = self.context.to_native();
        raw.surface = self.surface;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_vulkan_surface_descriptor) -> Self {
        Self {
            extent: crate::generated::RenderTargetExtent::from_native(raw.extent),
            context: crate::generated::VulkanContextDescriptor::from_native(raw.context),
            surface: raw.surface,
        }
    }
}
impl crate::values::NativeValue for VulkanSurfaceDescriptor {
    type Raw = maplibre_native_ffi_sys::mln_vulkan_surface_descriptor;
    fn to_native(self) -> Self::Raw {
        VulkanSurfaceDescriptor::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Clone, Default)]
pub struct Wake {
    pub callback: Option<std::sync::Arc<dyn Fn() -> () + Send + Sync + 'static>>,
}

impl std::fmt::Debug for Wake {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Wake").finish_non_exhaustive()
    }
}
impl Wake {
    pub fn with_callback<F>(mut self, callback: F) -> Self
    where
        F: Fn() -> () + Send + Sync + 'static,
    {
        self.callback = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn() -> () + Send + Sync + 'static,
    {
        Self::default().with_callback(callback)
    }
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_wake> {
        let mut raw: maplibre_native_ffi_sys::mln_wake = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_wake>() as _;
        raw.callback = self
            .callback
            .as_ref()
            .map(|_| Self::callback_trampoline as _);
        if !(raw.callback.is_none()) {
            raw.user_data = unsafe { arena.registration(self.clone(), Self::release_callback) };
            raw.release_user_data = Some(Self::release_callback);
        }
        Ok(raw)
    }
    /// Captures an empty callback descriptor returned by its native constructor.
    ///
    /// # Safety
    /// Borrowed fields must remain valid for this conversion.
    pub unsafe fn from_native(raw: maplibre_native_ffi_sys::mln_wake) -> crate::Result<Self> {
        if !(raw.callback.is_none()) {
            return Err(crate::Error::invalid_argument(
                "foreign callbacks cannot be adopted",
            ));
        }
        Ok(Self { callback: None })
    }
    unsafe extern "C" fn release_callback(user_data: *mut std::ffi::c_void) {
        let _policy = crate::callback::PolicyScope::enter(&[], 0);
        let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| unsafe {
            drop(Box::from_raw(user_data.cast::<Self>()))
        }));
    }
    unsafe extern "C" fn callback_trampoline(binding_arg_0: *mut std::ffi::c_void) -> () {
        let result =
            std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| -> crate::Result<()> {
                let state = unsafe { &*binding_arg_0.cast::<Self>() };
                let callback = state
                    .callback
                    .as_ref()
                    .ok_or_else(|| crate::Error::invalid_argument("missing callback"))?;
                callback();
                Ok(())
            }));
        match result {
            Ok(Ok(value)) => value,
            _ => Default::default(),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct WebglContextDescriptor {
    pub kind: crate::generated::WebglContextKind,
    pub context: i32,
    pub canvas_selector: String,
}

impl WebglContextDescriptor {
    pub fn to_native(
        &self,
        arena: &mut crate::input::InputArena,
    ) -> crate::Result<maplibre_native_ffi_sys::mln_webgl_context_descriptor> {
        let mut raw: maplibre_native_ffi_sys::mln_webgl_context_descriptor =
            unsafe { std::mem::zeroed() };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_webgl_context_descriptor>() as _;
        raw.kind = (&self.kind).to_native();
        raw.context = *(&self.context);
        raw.canvas_selector = maplibre_native_ffi_sys::mln_buffer_view {
            data: (&self.canvas_selector).as_bytes().as_ptr().cast(),
            size: (&self.canvas_selector).as_bytes().len(),
        };
        Ok(raw)
    }
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(
        raw: maplibre_native_ffi_sys::mln_webgl_context_descriptor,
    ) -> crate::Result<Self> {
        Ok(Self {
            kind: crate::generated::WebglContextKind::from_native(raw.kind),
            context: raw.context,
            canvas_selector: unsafe { crate::string::copy_string_view(raw.canvas_selector) }?,
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum WebglContextKind {
    TransferredCanvas,
    Existing,
    Unknown(u32),
}
impl Default for WebglContextKind {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl WebglContextKind {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            1 => Self::TransferredCanvas,
            0 => Self::Existing,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> u32 {
        match self {
            Self::TransferredCanvas => 1,
            Self::Existing => 0,
            Self::Unknown(value) => value,
        }
    }
    pub fn from_native(raw: u32) -> Self {
        Self::from_raw(raw)
    }
    pub fn to_native(self) -> u32 {
        self.as_raw()
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WebgpuBorrowedTextureDescriptor {
    pub extent: crate::generated::RenderTargetExtent,
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: crate::generated::WebgpuContextDescriptor,
    pub texture: *mut std::ffi::c_void,
    pub texture_view: *mut std::ffi::c_void,
    pub format: u32,
}
impl Default for WebgpuBorrowedTextureDescriptor {
    fn default() -> Self {
        Self::from_native(unsafe {
            maplibre_native_ffi_sys::mln_webgpu_borrowed_texture_descriptor_default()
        })
    }
}
impl WebgpuBorrowedTextureDescriptor {
    pub const fn new(
        extent: crate::generated::RenderTargetExtent,
        physical_width: u32,
        physical_height: u32,
        context: crate::generated::WebgpuContextDescriptor,
        texture: *mut std::ffi::c_void,
        texture_view: *mut std::ffi::c_void,
        format: u32,
    ) -> Self {
        Self {
            extent,
            physical_width,
            physical_height,
            context,
            texture,
            texture_view,
            format,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_webgpu_borrowed_texture_descriptor {
        let mut raw =
            unsafe { maplibre_native_ffi_sys::mln_webgpu_borrowed_texture_descriptor_default() };
        raw.extent = self.extent.to_native();
        raw.physical_width = self.physical_width;
        raw.physical_height = self.physical_height;
        raw.context = self.context.to_native();
        raw.texture = self.texture;
        raw.texture_view = self.texture_view;
        raw.format = self.format;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(
        raw: maplibre_native_ffi_sys::mln_webgpu_borrowed_texture_descriptor,
    ) -> Self {
        Self {
            extent: crate::generated::RenderTargetExtent::from_native(raw.extent),
            physical_width: raw.physical_width,
            physical_height: raw.physical_height,
            context: crate::generated::WebgpuContextDescriptor::from_native(raw.context),
            texture: raw.texture,
            texture_view: raw.texture_view,
            format: raw.format,
        }
    }
}
impl crate::values::NativeValue for WebgpuBorrowedTextureDescriptor {
    type Raw = maplibre_native_ffi_sys::mln_webgpu_borrowed_texture_descriptor;
    fn to_native(self) -> Self::Raw {
        WebgpuBorrowedTextureDescriptor::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct WebgpuContextDescriptor {
    pub instance: *mut std::ffi::c_void,
    pub device: *mut std::ffi::c_void,
    pub queue: *mut std::ffi::c_void,
}

impl WebgpuContextDescriptor {
    pub const fn new(
        instance: *mut std::ffi::c_void,
        device: *mut std::ffi::c_void,
        queue: *mut std::ffi::c_void,
    ) -> Self {
        Self {
            instance,
            device,
            queue,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_webgpu_context_descriptor {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_webgpu_context_descriptor>() };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_webgpu_context_descriptor>() as _;
        raw.instance = self.instance;
        raw.device = self.device;
        raw.queue = self.queue;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_webgpu_context_descriptor) -> Self {
        Self {
            instance: raw.instance,
            device: raw.device,
            queue: raw.queue,
        }
    }
}
impl crate::values::NativeValue for WebgpuContextDescriptor {
    type Raw = maplibre_native_ffi_sys::mln_webgpu_context_descriptor;
    fn to_native(self) -> Self::Raw {
        WebgpuContextDescriptor::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WebgpuOwnedTextureDescriptor {
    pub extent: crate::generated::RenderTargetExtent,
    pub context: crate::generated::WebgpuContextDescriptor,
}
impl Default for WebgpuOwnedTextureDescriptor {
    fn default() -> Self {
        Self::from_native(unsafe {
            maplibre_native_ffi_sys::mln_webgpu_owned_texture_descriptor_default()
        })
    }
}
impl WebgpuOwnedTextureDescriptor {
    pub const fn new(
        extent: crate::generated::RenderTargetExtent,
        context: crate::generated::WebgpuContextDescriptor,
    ) -> Self {
        Self { extent, context }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_webgpu_owned_texture_descriptor {
        let mut raw =
            unsafe { maplibre_native_ffi_sys::mln_webgpu_owned_texture_descriptor_default() };
        raw.extent = self.extent.to_native();
        raw.context = self.context.to_native();
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_webgpu_owned_texture_descriptor) -> Self {
        Self {
            extent: crate::generated::RenderTargetExtent::from_native(raw.extent),
            context: crate::generated::WebgpuContextDescriptor::from_native(raw.context),
        }
    }
}
impl crate::values::NativeValue for WebgpuOwnedTextureDescriptor {
    type Raw = maplibre_native_ffi_sys::mln_webgpu_owned_texture_descriptor;
    fn to_native(self) -> Self::Raw {
        WebgpuOwnedTextureDescriptor::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct WebgpuOwnedTextureFrame {
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub texture: *mut std::ffi::c_void,
    pub texture_view: *mut std::ffi::c_void,
    pub device: *mut std::ffi::c_void,
    pub format: u32,
}

impl WebgpuOwnedTextureFrame {
    pub const fn new(
        generation: u64,
        width: u32,
        height: u32,
        scale_factor: f64,
        frame_id: u64,
        texture: *mut std::ffi::c_void,
        texture_view: *mut std::ffi::c_void,
        device: *mut std::ffi::c_void,
        format: u32,
    ) -> Self {
        Self {
            generation,
            width,
            height,
            scale_factor,
            frame_id,
            texture,
            texture_view,
            device,
            format,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_webgpu_owned_texture_frame {
        let mut raw = unsafe {
            std::mem::zeroed::<maplibre_native_ffi_sys::mln_webgpu_owned_texture_frame>()
        };
        raw.size =
            std::mem::size_of::<maplibre_native_ffi_sys::mln_webgpu_owned_texture_frame>() as _;
        raw.generation = self.generation;
        raw.width = self.width;
        raw.height = self.height;
        raw.scale_factor = self.scale_factor;
        raw.frame_id = self.frame_id;
        raw.texture = self.texture;
        raw.texture_view = self.texture_view;
        raw.device = self.device;
        raw.format = self.format;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_webgpu_owned_texture_frame) -> Self {
        Self {
            generation: raw.generation,
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: raw.frame_id,
            texture: raw.texture,
            texture_view: raw.texture_view,
            device: raw.device,
            format: raw.format,
        }
    }
}
impl crate::values::NativeValue for WebgpuOwnedTextureFrame {
    type Raw = maplibre_native_ffi_sys::mln_webgpu_owned_texture_frame;
    fn to_native(self) -> Self::Raw {
        WebgpuOwnedTextureFrame::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WebgpuSurfaceDescriptor {
    pub extent: crate::generated::RenderTargetExtent,
    pub context: crate::generated::WebgpuContextDescriptor,
    pub surface: *mut std::ffi::c_void,
    pub format: u32,
}
impl Default for WebgpuSurfaceDescriptor {
    fn default() -> Self {
        Self::from_native(unsafe {
            maplibre_native_ffi_sys::mln_webgpu_surface_descriptor_default()
        })
    }
}
impl WebgpuSurfaceDescriptor {
    pub const fn new(
        extent: crate::generated::RenderTargetExtent,
        context: crate::generated::WebgpuContextDescriptor,
        surface: *mut std::ffi::c_void,
        format: u32,
    ) -> Self {
        Self {
            extent,
            context,
            surface,
            format,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_webgpu_surface_descriptor {
        let mut raw = unsafe { maplibre_native_ffi_sys::mln_webgpu_surface_descriptor_default() };
        raw.extent = self.extent.to_native();
        raw.context = self.context.to_native();
        raw.surface = self.surface;
        raw.format = self.format;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_webgpu_surface_descriptor) -> Self {
        Self {
            extent: crate::generated::RenderTargetExtent::from_native(raw.extent),
            context: crate::generated::WebgpuContextDescriptor::from_native(raw.context),
            surface: raw.surface,
            format: raw.format,
        }
    }
}
impl crate::values::NativeValue for WebgpuSurfaceDescriptor {
    type Raw = maplibre_native_ffi_sys::mln_webgpu_surface_descriptor;
    fn to_native(self) -> Self::Raw {
        WebgpuSurfaceDescriptor::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct WglContextDescriptor {
    pub device_context: *mut std::ffi::c_void,
    pub share_context: *mut std::ffi::c_void,
    pub get_proc_address: *mut std::ffi::c_void,
}

impl WglContextDescriptor {
    pub const fn new(
        device_context: *mut std::ffi::c_void,
        share_context: *mut std::ffi::c_void,
        get_proc_address: *mut std::ffi::c_void,
    ) -> Self {
        Self {
            device_context,
            share_context,
            get_proc_address,
        }
    }
    #[doc(hidden)]
    pub fn to_native(self) -> maplibre_native_ffi_sys::mln_wgl_context_descriptor {
        let mut raw =
            unsafe { std::mem::zeroed::<maplibre_native_ffi_sys::mln_wgl_context_descriptor>() };
        raw.size = std::mem::size_of::<maplibre_native_ffi_sys::mln_wgl_context_descriptor>() as _;
        raw.device_context = self.device_context;
        raw.share_context = self.share_context;
        raw.get_proc_address = self.get_proc_address;
        raw
    }
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::mln_wgl_context_descriptor) -> Self {
        Self {
            device_context: raw.device_context,
            share_context: raw.share_context,
            get_proc_address: raw.get_proc_address,
        }
    }
}
impl crate::values::NativeValue for WglContextDescriptor {
    type Raw = maplibre_native_ffi_sys::mln_wgl_context_descriptor;
    fn to_native(self) -> Self::Raw {
        WglContextDescriptor::to_native(self)
    }
    fn from_native(value: Self::Raw) -> Self {
        Self::from_native(value)
    }
}

#[doc(hidden)]
pub unsafe fn acquired_frame_dispose(
    native: maplibre_native_ffi_sys::mln_acquired_frame,
) -> crate::Result<()> {
    crate::check(|diagnostic| unsafe {
        maplibre_native_ffi_sys::mln_acquired_frame_dispose(native, diagnostic)
    })
}

#[doc(hidden)]
pub unsafe fn map_dispose(native: maplibre_native_ffi_sys::mln_map) -> crate::Result<()> {
    crate::check(|diagnostic| unsafe {
        maplibre_native_ffi_sys::mln_map_dispose(native, diagnostic)
    })
}

#[doc(hidden)]
pub unsafe fn map_projection_dispose(
    native: maplibre_native_ffi_sys::mln_map_projection,
) -> crate::Result<()> {
    crate::check(|diagnostic| unsafe {
        maplibre_native_ffi_sys::mln_map_projection_close(native, diagnostic)
    })
}

#[doc(hidden)]
pub unsafe fn render_session_dispose(
    native: maplibre_native_ffi_sys::mln_render_session,
) -> crate::Result<()> {
    crate::check(|diagnostic| unsafe {
        maplibre_native_ffi_sys::mln_render_session_dispose(native, diagnostic)
    })
}

#[doc(hidden)]
pub unsafe fn runtime_dispose(native: maplibre_native_ffi_sys::mln_runtime) -> crate::Result<()> {
    crate::check(|diagnostic| unsafe {
        maplibre_native_ffi_sys::mln_runtime_dispose(native, diagnostic)
    })
}

pub const RESOURCE_REQUEST_HANDLE_FUNCTIONS: crate::resource::ResourceRequestHandleFns = unsafe {
    crate::resource::ResourceRequestHandleFns::new(
        maplibre_native_ffi_sys::mln_resource_request_complete,
        maplibre_native_ffi_sys::mln_resource_request_release,
    )
};

pub type LogCallback = std::sync::Arc<
    dyn Fn(crate::generated::LogSeverity, crate::generated::LogEvent, i64, String) -> u32
        + Send
        + Sync
        + 'static,
>;
pub fn log_callback_registration(
    callback: Option<LogCallback>,
    arena: &mut crate::input::InputArena,
) -> (
    maplibre_native_ffi_sys::mln_log_callback,
    *mut std::ffi::c_void,
    maplibre_native_ffi_sys::mln_log_callback_release,
) {
    unsafe extern "C" fn invoke(
        binding_arg_0: *mut std::ffi::c_void,
        binding_arg_1: u32,
        binding_arg_2: u32,
        binding_arg_3: i64,
        binding_arg_4: *const std::ffi::c_char,
    ) -> u32 {
        let _policy = crate::callback::PolicyScope::enter(&[], 0);
        let result =
            std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| -> crate::Result<u32> {
                let callback = unsafe { &*binding_arg_0.cast::<LogCallback>() };
                let value = callback(
                    crate::generated::LogSeverity::from_native(binding_arg_1),
                    crate::generated::LogEvent::from_native(binding_arg_2),
                    binding_arg_3,
                    unsafe { crate::string::copy_c_string(binding_arg_4) }?,
                );
                Ok(value)
            }));
        match result {
            Ok(Ok(value)) => value,
            _ => 0,
        }
    }
    unsafe extern "C" fn release(context: *mut std::ffi::c_void) {
        let _policy = crate::callback::PolicyScope::enter(&[], 0);
        let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| unsafe {
            drop(Box::from_raw(context.cast::<LogCallback>()));
        }));
    }
    match callback {
        Some(callback) => (
            Some(invoke),
            unsafe { arena.registration(callback, release) },
            Some(release),
        ),
        None => (None, std::ptr::null_mut(), None),
    }
}
