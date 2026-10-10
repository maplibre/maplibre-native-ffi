// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_enum! {
pub enum AmbientCacheOperation: u32 {
    ResetDatabase = 1,
    PackDatabase = 2,
    Invalidate = 3,
    Clear = 4,
} Unknown
}

native_flags! {
/// Field mask values for `mln_animation_options`.
///
/// See `mln_animation_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub struct AnimationOptionField: u32 {
    const DURATION = 1;
    const VELOCITY = 2;
    const MIN_ZOOM = 4;
    const EASING = 8;
    const TRANSITION_ID = 16;
}
}

/// Optional animation controls for camera transitions.
///
/// See `mln_animation_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone)]
pub struct AnimationOptions {
    /// Duration in milliseconds. Must be finite and non-negative. Values that
    /// would overflow MapLibre Native's internal duration are invalid.
    pub duration_ms: Option<f64>,
    /// Average fly velocity in screenfuls per second. Must be positive and
    /// defaults to 1.2 when omitted.
    pub velocity: Option<f64>,
    /// Peak zoom for flyTo transitions.
    pub min_zoom: Option<f64>,
    pub easing: Option<UnitBezier>,
    /// Caller-chosen identity of the command that these options animate, which
    /// `mln_map_cancel_camera_transition()` matches.
    pub transition_id: Option<u64>,
    /// Reports the end of the command's transitions. Disabled by default.
    pub end_handler: CameraTransitionHandler,
}
impl Default for AnimationOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_animation_options_default() })
    }
}
impl ToNative<sys::mln_animation_options> for AnimationOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_animation_options> {
        let mut raw: sys::mln_animation_options = unsafe { sys::mln_animation_options_default() };
        raw.fields = 0;
        if let Some(item) = &self.duration_ms {
            raw.fields |= sys::MLN_ANIMATION_OPTION_DURATION;
            raw.duration_ms = *item;
        }
        if let Some(item) = &self.velocity {
            raw.fields |= sys::MLN_ANIMATION_OPTION_VELOCITY;
            raw.velocity = *item;
        }
        if let Some(item) = &self.min_zoom {
            raw.fields |= sys::MLN_ANIMATION_OPTION_MIN_ZOOM;
            raw.min_zoom = *item;
        }
        if let Some(item) = &self.easing {
            raw.fields |= sys::MLN_ANIMATION_OPTION_EASING;
            raw.easing = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.transition_id {
            raw.fields |= sys::MLN_ANIMATION_OPTION_TRANSITION_ID;
            raw.transition_id = *item;
        }
        raw.end_handler = to_native(&self.end_handler, arena)?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_animation_options> for AnimationOptions {
    unsafe fn from_native(raw: sys::mln_animation_options) -> Result<Self> {
        Ok(Self {
            duration_ms: (raw.fields & sys::MLN_ANIMATION_OPTION_DURATION != 0)
                .then_some(raw.duration_ms),
            velocity: (raw.fields & sys::MLN_ANIMATION_OPTION_VELOCITY != 0)
                .then_some(raw.velocity),
            min_zoom: (raw.fields & sys::MLN_ANIMATION_OPTION_MIN_ZOOM != 0)
                .then_some(raw.min_zoom),
            easing: unsafe {
                convert::present(raw.fields, sys::MLN_ANIMATION_OPTION_EASING, raw.easing)
            }?,
            transition_id: (raw.fields & sys::MLN_ANIMATION_OPTION_TRANSITION_ID != 0)
                .then_some(raw.transition_id),
            end_handler: Default::default(),
        })
    }
}

native_flags! {
/// Field mask values for `mln_bound_options`.
///
/// See `mln_bound_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub struct BoundOptionField: u32 {
    /// Selects `mln_bound_options.bounds` as a geographic constraint that the
    /// camera center stays inside. Mutually exclusive with
    /// `MLN_BOUND_OPTION_UNBOUNDED`.
    const BOUNDS = 1;
    const MIN_ZOOM = 2;
    const MAX_ZOOM = 4;
    const MIN_PITCH = 8;
    const MAX_PITCH = 16;
    /// Selects the unbounded geographic constraint, which leaves every camera
    /// center unconstrained and lets the map pan freely across the
    /// antimeridian. This differs from world bounds of -90/-180 to 90/180,
    /// which clamp longitude to that range. Mutually exclusive with
    /// `MLN_BOUND_OPTION_BOUNDS`, and leaves `mln_bound_options.bounds` unread.
    const UNBOUNDED = 32;
}
}

/// Optional map camera constraint fields.
///
/// See `mln_bound_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct BoundOptions {
    /// Selects the unbounded geographic constraint, which leaves every camera
    /// center unconstrained and lets the map pan freely across the
    /// antimeridian. This differs from world bounds of -90/-180 to 90/180,
    /// which clamp longitude to that range. Mutually exclusive with
    /// `MLN_BOUND_OPTION_BOUNDS`, and leaves `mln_bound_options.bounds` unread.
    pub unbounded: bool,
    /// Read when fields contains `MLN_BOUND_OPTION_BOUNDS`.
    pub bounds: Option<LatLngBounds>,
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
    pub min_pitch: Option<f64>,
    pub max_pitch: Option<f64>,
}
impl Default for BoundOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_bound_options_default() })
    }
}
impl ToNative<sys::mln_bound_options> for BoundOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_bound_options> {
        let mut raw: sys::mln_bound_options = unsafe { sys::mln_bound_options_default() };
        raw.size = std::mem::size_of::<sys::mln_bound_options>() as _;
        raw.fields = 0;
        convert::set_flag(
            &mut raw.fields,
            sys::MLN_BOUND_OPTION_UNBOUNDED,
            self.unbounded,
        );
        if let Some(item) = &self.bounds {
            raw.fields |= sys::MLN_BOUND_OPTION_BOUNDS;
            raw.bounds = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.min_zoom {
            raw.fields |= sys::MLN_BOUND_OPTION_MIN_ZOOM;
            raw.min_zoom = *item;
        }
        if let Some(item) = &self.max_zoom {
            raw.fields |= sys::MLN_BOUND_OPTION_MAX_ZOOM;
            raw.max_zoom = *item;
        }
        if let Some(item) = &self.min_pitch {
            raw.fields |= sys::MLN_BOUND_OPTION_MIN_PITCH;
            raw.min_pitch = *item;
        }
        if let Some(item) = &self.max_pitch {
            raw.fields |= sys::MLN_BOUND_OPTION_MAX_PITCH;
            raw.max_pitch = *item;
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_bound_options> for BoundOptions {
    unsafe fn from_native(raw: sys::mln_bound_options) -> Result<Self> {
        Ok(Self {
            unbounded: raw.fields & sys::MLN_BOUND_OPTION_UNBOUNDED != 0,
            bounds: unsafe {
                convert::present(raw.fields, sys::MLN_BOUND_OPTION_BOUNDS, raw.bounds)
            }?,
            min_zoom: (raw.fields & sys::MLN_BOUND_OPTION_MIN_ZOOM != 0).then_some(raw.min_zoom),
            max_zoom: (raw.fields & sys::MLN_BOUND_OPTION_MAX_ZOOM != 0).then_some(raw.max_zoom),
            min_pitch: (raw.fields & sys::MLN_BOUND_OPTION_MIN_PITCH != 0).then_some(raw.min_pitch),
            max_pitch: (raw.fields & sys::MLN_BOUND_OPTION_MAX_PITCH != 0).then_some(raw.max_pitch),
        })
    }
}

native_enum! {
/// Camera change kinds reported by camera will-change and did-change events.
///
/// See `mln_camera_change_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub enum CameraChangeMode: u32 {
    /// The camera reached its new value without an animated transition.
    Immediate = 0,
    /// The camera moved as part of an animated transition.
    Animated = 1,
} Unknown
}

/// One atomic relative camera update.
///
/// See `mln_camera_delta` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone)]
pub struct CameraDelta {
    /// Pan in logical map pixels; the content moves by this offset.
    pub offset: Option<ScreenPoint>,
    /// Positive zoom factor; 2 zooms in one level.
    pub scale: Option<f64>,
    /// Degrees added to the bearing.
    pub bearing: Option<f64>,
    /// Degrees added to the pitch; positive tilts further from straight down.
    pub pitch: Option<f64>,
    /// Screen point in logical map pixels that scale, bearing, and pitch keep
    /// fixed.
    pub anchor: Option<ScreenPoint>,
    pub animation: AnimationOptions,
    pub gesture_phase: GesturePhase,
}
impl Default for CameraDelta {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_camera_delta_default() })
    }
}
impl ToNative<sys::mln_camera_delta> for CameraDelta {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_camera_delta> {
        let mut raw: sys::mln_camera_delta = unsafe { sys::mln_camera_delta_default() };
        raw.size = std::mem::size_of::<sys::mln_camera_delta>() as _;
        raw.fields = 0;
        if let Some(item) = &self.offset {
            raw.fields |= sys::MLN_CAMERA_DELTA_OFFSET;
            raw.offset = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.scale {
            raw.fields |= sys::MLN_CAMERA_DELTA_SCALE;
            raw.scale = *item;
        }
        if let Some(item) = &self.bearing {
            raw.fields |= sys::MLN_CAMERA_DELTA_BEARING;
            raw.bearing = *item;
        }
        if let Some(item) = &self.pitch {
            raw.fields |= sys::MLN_CAMERA_DELTA_PITCH;
            raw.pitch = *item;
        }
        if let Some(item) = &self.anchor {
            raw.fields |= sys::MLN_CAMERA_DELTA_ANCHOR;
            raw.anchor = to_native(&*item, arena)?;
        }
        raw.animation = to_native(&self.animation, arena)?;
        raw.gesture_phase = to_native(&self.gesture_phase, arena)?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_camera_delta> for CameraDelta {
    unsafe fn from_native(raw: sys::mln_camera_delta) -> Result<Self> {
        Ok(Self {
            offset: unsafe {
                convert::present(raw.fields, sys::MLN_CAMERA_DELTA_OFFSET, raw.offset)
            }?,
            scale: (raw.fields & sys::MLN_CAMERA_DELTA_SCALE != 0).then_some(raw.scale),
            bearing: (raw.fields & sys::MLN_CAMERA_DELTA_BEARING != 0).then_some(raw.bearing),
            pitch: (raw.fields & sys::MLN_CAMERA_DELTA_PITCH != 0).then_some(raw.pitch),
            anchor: unsafe {
                convert::present(raw.fields, sys::MLN_CAMERA_DELTA_ANCHOR, raw.anchor)
            }?,
            animation: unsafe { from_native(raw.animation) }?,
            gesture_phase: unsafe { from_native(raw.gesture_phase) }?,
        })
    }
}

native_flags! {
/// Field mask values for `mln_camera_delta`.
///
/// See `mln_camera_delta_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub struct CameraDeltaField: u32 {
    const OFFSET = 1;
    const SCALE = 2;
    const BEARING = 4;
    const PITCH = 8;
    const ANCHOR = 16;
}
}

native_flags! {
/// Field mask values for `mln_camera_fit_options`.
///
/// See `mln_camera_fit_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub struct CameraFitOptionField: u32 {
    const PADDING = 1;
    const BEARING = 2;
    const PITCH = 4;
}
}

/// Optional fitting controls for camera-for-viewport queries.
///
/// See `mln_camera_fit_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct CameraFitOptions {
    pub padding: Option<EdgeInsets>,
    pub bearing: Option<f64>,
    pub pitch: Option<f64>,
}
impl Default for CameraFitOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_camera_fit_options_default() })
    }
}
impl ToNative<sys::mln_camera_fit_options> for CameraFitOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_camera_fit_options> {
        let mut raw: sys::mln_camera_fit_options = unsafe { sys::mln_camera_fit_options_default() };
        raw.size = std::mem::size_of::<sys::mln_camera_fit_options>() as _;
        raw.fields = 0;
        if let Some(item) = &self.padding {
            raw.fields |= sys::MLN_CAMERA_FIT_OPTION_PADDING;
            raw.padding = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.bearing {
            raw.fields |= sys::MLN_CAMERA_FIT_OPTION_BEARING;
            raw.bearing = *item;
        }
        if let Some(item) = &self.pitch {
            raw.fields |= sys::MLN_CAMERA_FIT_OPTION_PITCH;
            raw.pitch = *item;
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_camera_fit_options> for CameraFitOptions {
    unsafe fn from_native(raw: sys::mln_camera_fit_options) -> Result<Self> {
        Ok(Self {
            padding: unsafe {
                convert::present(raw.fields, sys::MLN_CAMERA_FIT_OPTION_PADDING, raw.padding)
            }?,
            bearing: (raw.fields & sys::MLN_CAMERA_FIT_OPTION_BEARING != 0).then_some(raw.bearing),
            pitch: (raw.fields & sys::MLN_CAMERA_FIT_OPTION_PITCH != 0).then_some(raw.pitch),
        })
    }
}

native_flags! {
/// Field mask values for `mln_camera_options`.
///
/// See `mln_camera_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub struct CameraOptionField: u32 {
    const CENTER = 1;
    const ZOOM = 2;
    const BEARING = 4;
    const PITCH = 8;
    const CENTER_ALTITUDE = 16;
    const PADDING = 32;
    const ANCHOR = 64;
    const ROLL = 128;
    const FOV = 256;
}
}

/// Camera fields used by snapshots and camera updates.
///
/// See `mln_camera_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct CameraOptions {
    pub center: Option<LatLng>,
    pub center_altitude: Option<f64>,
    pub padding: Option<EdgeInsets>,
    /// Optional screen-space focal point in logical map pixels.
    pub anchor: Option<ScreenPoint>,
    pub zoom: Option<f64>,
    pub bearing: Option<f64>,
    pub pitch: Option<f64>,
    pub roll: Option<f64>,
    pub field_of_view: Option<f64>,
}
impl Default for CameraOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_camera_options_default() })
    }
}
impl ToNative<sys::mln_camera_options> for CameraOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_camera_options> {
        let mut raw: sys::mln_camera_options = unsafe { sys::mln_camera_options_default() };
        raw.size = std::mem::size_of::<sys::mln_camera_options>() as _;
        raw.fields = 0;
        if let Some(item) = &self.center {
            raw.fields |= sys::MLN_CAMERA_OPTION_CENTER;
            raw.center = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.center_altitude {
            raw.fields |= sys::MLN_CAMERA_OPTION_CENTER_ALTITUDE;
            raw.center_altitude = *item;
        }
        if let Some(item) = &self.padding {
            raw.fields |= sys::MLN_CAMERA_OPTION_PADDING;
            raw.padding = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.anchor {
            raw.fields |= sys::MLN_CAMERA_OPTION_ANCHOR;
            raw.anchor = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.zoom {
            raw.fields |= sys::MLN_CAMERA_OPTION_ZOOM;
            raw.zoom = *item;
        }
        if let Some(item) = &self.bearing {
            raw.fields |= sys::MLN_CAMERA_OPTION_BEARING;
            raw.bearing = *item;
        }
        if let Some(item) = &self.pitch {
            raw.fields |= sys::MLN_CAMERA_OPTION_PITCH;
            raw.pitch = *item;
        }
        if let Some(item) = &self.roll {
            raw.fields |= sys::MLN_CAMERA_OPTION_ROLL;
            raw.roll = *item;
        }
        if let Some(item) = &self.field_of_view {
            raw.fields |= sys::MLN_CAMERA_OPTION_FOV;
            raw.field_of_view = *item;
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_camera_options> for CameraOptions {
    unsafe fn from_native(raw: sys::mln_camera_options) -> Result<Self> {
        Ok(Self {
            center: unsafe {
                convert::present(raw.fields, sys::MLN_CAMERA_OPTION_CENTER, raw.center)
            }?,
            center_altitude: (raw.fields & sys::MLN_CAMERA_OPTION_CENTER_ALTITUDE != 0)
                .then_some(raw.center_altitude),
            padding: unsafe {
                convert::present(raw.fields, sys::MLN_CAMERA_OPTION_PADDING, raw.padding)
            }?,
            anchor: unsafe {
                convert::present(raw.fields, sys::MLN_CAMERA_OPTION_ANCHOR, raw.anchor)
            }?,
            zoom: (raw.fields & sys::MLN_CAMERA_OPTION_ZOOM != 0).then_some(raw.zoom),
            bearing: (raw.fields & sys::MLN_CAMERA_OPTION_BEARING != 0).then_some(raw.bearing),
            pitch: (raw.fields & sys::MLN_CAMERA_OPTION_PITCH != 0).then_some(raw.pitch),
            roll: (raw.fields & sys::MLN_CAMERA_OPTION_ROLL != 0).then_some(raw.roll),
            field_of_view: (raw.fields & sys::MLN_CAMERA_OPTION_FOV != 0)
                .then_some(raw.field_of_view),
        })
    }
}

/// Camera result borrowed for an ordered camera-query completion.
///
/// See `mln_camera_query_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct CameraQueryResult {
    pub generation: u64,
    pub camera: CameraOptions,
}
impl CameraQueryResult {
    pub const fn new(generation: u64, camera: CameraOptions) -> Self {
        Self { generation, camera }
    }
}
impl FromNative<sys::mln_camera_query_result> for CameraQueryResult {
    unsafe fn from_native(raw: sys::mln_camera_query_result) -> Result<Self> {
        Ok(Self {
            generation: raw.generation,
            camera: unsafe { from_native(raw.camera) }?,
        })
    }
}

/// The end of one camera command's transitions, borrowed for the callback.
///
/// See `mln_camera_transition_end` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct CameraTransitionEnd {
    /// One of `mln_camera_transition_outcome`.
    pub outcome: CameraTransitionOutcome,
    /// Generation of the published snapshot that shows the camera where the
    /// transitions left it. The map events of the change that ended them carry
    /// this generation and are queued before the callback runs. Zero for
    /// `MLN_CAMERA_TRANSITION_OUTCOME_CLOSED`.
    pub generation: u64,
}
impl CameraTransitionEnd {
    pub const fn new(outcome: CameraTransitionOutcome, generation: u64) -> Self {
        Self {
            outcome,
            generation,
        }
    }
}
impl FromNative<sys::mln_camera_transition_end> for CameraTransitionEnd {
    unsafe fn from_native(raw: sys::mln_camera_transition_end) -> Result<Self> {
        Ok(Self {
            outcome: unsafe { from_native(raw.outcome) }?,
            generation: raw.generation,
        })
    }
}

/// Callback state that one camera command copies to report the end of its
/// transitions.
///
/// See `mln_camera_transition_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Clone, Default)]
pub struct CameraTransitionHandler {
    pub callback: Option<std::sync::Arc<dyn Fn(CameraTransitionEnd) -> () + Send + Sync + 'static>>,
}
impl std::fmt::Debug for CameraTransitionHandler {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("CameraTransitionHandler")
            .finish_non_exhaustive()
    }
}
impl CameraTransitionHandler {
    pub fn with_callback<F>(mut self, callback: F) -> Self
    where
        F: Fn(CameraTransitionEnd) -> () + Send + Sync + 'static,
    {
        self.callback = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(CameraTransitionEnd) -> () + Send + Sync + 'static,
    {
        Self::default().with_callback(callback)
    }
    unsafe extern "C" fn callback_trampoline(
        user_data: *mut std::ffi::c_void,
        end: *const sys::mln_camera_transition_end,
    ) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_camera_transition_end_callback", None, (), || {
            callback::require(&state.callback)?(unsafe { convert::copy_reference(end) }?);
            Ok(())
        })
    }
}
impl ToNative<sys::mln_camera_transition_handler> for CameraTransitionHandler {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_camera_transition_handler> {
        let mut raw: sys::mln_camera_transition_handler = unsafe { std::mem::zeroed() };
        raw.callback = self
            .callback
            .as_ref()
            .map(|_| Self::callback_trampoline as _);
        if !(raw.callback.is_none()) {
            // SAFETY: the release reclaims exactly this state.
            raw.user_data = unsafe { arena.registration(self.clone(), callback::release::<Self>) };
            raw.release_user_data = Some(callback::release::<Self>);
        }
        Ok(raw)
    }
}

native_enum! {
/// How the transitions of one camera command ended.
///
/// See `mln_camera_transition_outcome` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub enum CameraTransitionOutcome: u32 {
    /// Every property of the command reached its target, or a later camera
    /// write replaced it.
    Completed = 0,
    /// `mln_map_cancel_transitions()`, `mln_map_cancel_camera_transition()`, or
    /// `MLN_GESTURE_PHASE_CANCEL` ended the transitions, or the command failed
    /// before it started them.
    Cancelled = 1,
    /// The map closed before the transitions ended.
    Closed = 2,
} Unknown
}

/// One atomic absolute camera update.
///
/// See `mln_camera_update` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone)]
pub struct CameraUpdate {
    pub mode: CameraUpdateMode,
    pub camera: CameraOptions,
    pub animation: AnimationOptions,
    pub gesture_phase: GesturePhase,
}
impl Default for CameraUpdate {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_camera_update_default() })
    }
}
impl ToNative<sys::mln_camera_update> for CameraUpdate {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_camera_update> {
        let mut raw: sys::mln_camera_update = unsafe { sys::mln_camera_update_default() };
        raw.size = std::mem::size_of::<sys::mln_camera_update>() as _;
        raw.mode = to_native(&self.mode, arena)?;
        raw.camera = to_native(&self.camera, arena)?;
        raw.animation = to_native(&self.animation, arena)?;
        raw.gesture_phase = to_native(&self.gesture_phase, arena)?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_camera_update> for CameraUpdate {
    unsafe fn from_native(raw: sys::mln_camera_update) -> Result<Self> {
        Ok(Self {
            mode: unsafe { from_native(raw.mode) }?,
            camera: unsafe { from_native(raw.camera) }?,
            animation: unsafe { from_native(raw.animation) }?,
            gesture_phase: unsafe { from_native(raw.gesture_phase) }?,
        })
    }
}

native_enum! {
/// Camera transition behavior for `mln_camera_update`.
///
/// See `mln_camera_update_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub enum CameraUpdateMode: u32 {
    Jump = 0,
    Ease = 1,
    Fly = 2,
} Unknown
}

/// Canonical tile identity used by custom geometry and custom MVT vector source
/// callbacks.
///
/// See `mln_canonical_tile_id` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
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
}
impl ToNative<sys::mln_canonical_tile_id> for CanonicalTileId {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_canonical_tile_id> {
        let mut raw: sys::mln_canonical_tile_id = unsafe { std::mem::zeroed() };
        raw.z = self.z;
        raw.x = self.x;
        raw.y = self.y;
        Ok(raw)
    }
}
impl FromNative<sys::mln_canonical_tile_id> for CanonicalTileId {
    unsafe fn from_native(raw: sys::mln_canonical_tile_id) -> Result<Self> {
        Ok(Self {
            z: raw.z,
            x: raw.x,
            y: raw.y,
        })
    }
}

native_enum! {
/// Terminal dispositions reported by command completions.
///
/// See `mln_command_disposition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/completion_8h.html).
pub enum CommandDisposition: u32 {
    Committed = 0,
    Superseded = 1,
    Failed = 2,
    Cancelled = 3,
} Unknown
}

native_enum! {
/// Map constraint modes used by `mln_map_viewport_options`.
///
/// See `mln_constrain_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub enum ConstrainMode: u32 {
    None = 0,
    HeightOnly = 1,
    WidthAndHeight = 2,
    Screen = 3,
} Unknown
}

native_flags! {
/// Field mask values for `mln_custom_geometry_source_options`.
///
/// See `mln_custom_geometry_source_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub struct CustomGeometrySourceOptionField: u32 {
    const MIN_ZOOM = 1;
    const MAX_ZOOM = 2;
    const TOLERANCE = 4;
    const TILE_SIZE = 8;
    const BUFFER = 16;
    const CLIP = 32;
    const WRAP = 64;
}
}

/// Options for custom geometry sources.
///
/// See `mln_custom_geometry_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Clone)]
pub struct CustomGeometrySourceOptions {
    /// Required tile fetch callback.
    pub fetch_tile: Option<std::sync::Arc<dyn Fn(CanonicalTileId) -> () + Send + Sync + 'static>>,
    /// Optional best-effort tile cancel callback.
    pub cancel_tile: Option<std::sync::Arc<dyn Fn(CanonicalTileId) -> () + Send + Sync + 'static>>,
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
        convert::native_default(unsafe { sys::mln_custom_geometry_source_options_default() })
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
        F: Fn(CanonicalTileId) -> () + Send + Sync + 'static,
    {
        self.fetch_tile = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(CanonicalTileId) -> () + Send + Sync + 'static,
    {
        Self::default().with_fetch_tile(callback)
    }
    pub fn with_cancel_tile<F>(mut self, callback: F) -> Self
    where
        F: Fn(CanonicalTileId) -> () + Send + Sync + 'static,
    {
        self.cancel_tile = Some(std::sync::Arc::new(callback));
        self
    }
    unsafe extern "C" fn fetch_tile_trampoline(
        user_data: *mut std::ffi::c_void,
        tile_id: sys::mln_canonical_tile_id,
    ) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_custom_source_tile_callback", None, (), || {
            callback::require(&state.fetch_tile)?(unsafe { from_native(tile_id) }?);
            Ok(())
        })
    }
    unsafe extern "C" fn cancel_tile_trampoline(
        user_data: *mut std::ffi::c_void,
        tile_id: sys::mln_canonical_tile_id,
    ) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_custom_source_tile_callback", None, (), || {
            callback::require(&state.cancel_tile)?(unsafe { from_native(tile_id) }?);
            Ok(())
        })
    }
}
impl ToNative<sys::mln_custom_geometry_source_options> for CustomGeometrySourceOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_custom_geometry_source_options> {
        let mut raw: sys::mln_custom_geometry_source_options =
            unsafe { sys::mln_custom_geometry_source_options_default() };
        raw.fields = 0;
        raw.size = std::mem::size_of::<sys::mln_custom_geometry_source_options>() as _;
        raw.fetch_tile = self
            .fetch_tile
            .as_ref()
            .map(|_| Self::fetch_tile_trampoline as _);
        raw.cancel_tile = self
            .cancel_tile
            .as_ref()
            .map(|_| Self::cancel_tile_trampoline as _);
        if let Some(item) = &self.min_zoom {
            raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
            raw.min_zoom = *item;
        }
        if let Some(item) = &self.max_zoom {
            raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
            raw.max_zoom = *item;
        }
        if let Some(item) = &self.tolerance {
            raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE;
            raw.tolerance = *item;
        }
        if let Some(item) = &self.tile_size {
            raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE;
            raw.tile_size = *item;
        }
        if let Some(item) = &self.buffer {
            raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER;
            raw.buffer = *item;
        }
        if let Some(item) = &self.clip {
            raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP;
            raw.clip = *item;
        }
        if let Some(item) = &self.wrap {
            raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
            raw.wrap = *item;
        }
        if !(raw.fetch_tile.is_none() && raw.cancel_tile.is_none()) {
            // SAFETY: the release reclaims exactly this state.
            raw.user_data = unsafe { arena.registration(self.clone(), callback::release::<Self>) };
            raw.release_user_data = Some(callback::release::<Self>);
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_custom_geometry_source_options> for CustomGeometrySourceOptions {
    /// Copies a native default, whose callbacks are unset.
    unsafe fn from_native(raw: sys::mln_custom_geometry_source_options) -> Result<Self> {
        Ok(Self {
            fetch_tile: None,
            cancel_tile: None,
            min_zoom: (raw.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM != 0)
                .then_some(raw.min_zoom),
            max_zoom: (raw.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM != 0)
                .then_some(raw.max_zoom),
            tolerance: (raw.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE != 0)
                .then_some(raw.tolerance),
            tile_size: (raw.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE != 0)
                .then_some(raw.tile_size),
            buffer: (raw.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER != 0)
                .then_some(raw.buffer),
            clip: (raw.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP != 0)
                .then_some(raw.clip),
            wrap: (raw.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP != 0)
                .then_some(raw.wrap),
        })
    }
}

native_flags! {
/// Field mask values for `mln_custom_mvt_vector_source_options`.
///
/// See `mln_custom_mvt_vector_source_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub struct CustomMvtVectorSourceOptionField: u32 {
    const MIN_ZOOM = 1;
    const MAX_ZOOM = 2;
}
}

/// Options for custom MVT vector sources.
///
/// See `mln_custom_mvt_vector_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Clone)]
pub struct CustomMvtVectorSourceOptions {
    /// Required tile fetch callback.
    pub fetch_tile: Option<std::sync::Arc<dyn Fn(CanonicalTileId) -> () + Send + Sync + 'static>>,
    /// Optional best-effort tile cancel callback.
    pub cancel_tile: Option<std::sync::Arc<dyn Fn(CanonicalTileId) -> () + Send + Sync + 'static>>,
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
}
impl Default for CustomMvtVectorSourceOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_custom_mvt_vector_source_options_default() })
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
        F: Fn(CanonicalTileId) -> () + Send + Sync + 'static,
    {
        self.fetch_tile = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(CanonicalTileId) -> () + Send + Sync + 'static,
    {
        Self::default().with_fetch_tile(callback)
    }
    pub fn with_cancel_tile<F>(mut self, callback: F) -> Self
    where
        F: Fn(CanonicalTileId) -> () + Send + Sync + 'static,
    {
        self.cancel_tile = Some(std::sync::Arc::new(callback));
        self
    }
    unsafe extern "C" fn fetch_tile_trampoline(
        user_data: *mut std::ffi::c_void,
        tile_id: sys::mln_canonical_tile_id,
    ) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_custom_source_tile_callback", None, (), || {
            callback::require(&state.fetch_tile)?(unsafe { from_native(tile_id) }?);
            Ok(())
        })
    }
    unsafe extern "C" fn cancel_tile_trampoline(
        user_data: *mut std::ffi::c_void,
        tile_id: sys::mln_canonical_tile_id,
    ) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_custom_source_tile_callback", None, (), || {
            callback::require(&state.cancel_tile)?(unsafe { from_native(tile_id) }?);
            Ok(())
        })
    }
}
impl ToNative<sys::mln_custom_mvt_vector_source_options> for CustomMvtVectorSourceOptions {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_custom_mvt_vector_source_options> {
        let mut raw: sys::mln_custom_mvt_vector_source_options =
            unsafe { sys::mln_custom_mvt_vector_source_options_default() };
        raw.fields = 0;
        raw.size = std::mem::size_of::<sys::mln_custom_mvt_vector_source_options>() as _;
        raw.fetch_tile = self
            .fetch_tile
            .as_ref()
            .map(|_| Self::fetch_tile_trampoline as _);
        raw.cancel_tile = self
            .cancel_tile
            .as_ref()
            .map(|_| Self::cancel_tile_trampoline as _);
        if let Some(item) = &self.min_zoom {
            raw.fields |= sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM;
            raw.min_zoom = *item;
        }
        if let Some(item) = &self.max_zoom {
            raw.fields |= sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
            raw.max_zoom = *item;
        }
        if !(raw.fetch_tile.is_none() && raw.cancel_tile.is_none()) {
            // SAFETY: the release reclaims exactly this state.
            raw.user_data = unsafe { arena.registration(self.clone(), callback::release::<Self>) };
            raw.release_user_data = Some(callback::release::<Self>);
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_custom_mvt_vector_source_options> for CustomMvtVectorSourceOptions {
    /// Copies a native default, whose callbacks are unset.
    unsafe fn from_native(raw: sys::mln_custom_mvt_vector_source_options) -> Result<Self> {
        Ok(Self {
            fetch_tile: None,
            cancel_tile: None,
            min_zoom: (raw.fields & sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM != 0)
                .then_some(raw.min_zoom),
            max_zoom: (raw.fields & sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM != 0)
                .then_some(raw.max_zoom),
        })
    }
}

/// Screen-space inset in logical map pixels.
///
/// See `mln_edge_insets` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
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
}
impl ToNative<sys::mln_edge_insets> for EdgeInsets {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_edge_insets> {
        let mut raw: sys::mln_edge_insets = unsafe { std::mem::zeroed() };
        raw.top = self.top;
        raw.left = self.left;
        raw.bottom = self.bottom;
        raw.right = self.right;
        Ok(raw)
    }
}
impl FromNative<sys::mln_edge_insets> for EdgeInsets {
    unsafe fn from_native(raw: sys::mln_edge_insets) -> Result<Self> {
        Ok(Self {
            top: raw.top,
            left: raw.left,
            bottom: raw.bottom,
            right: raw.right,
        })
    }
}

/// EGL context fields shared by OpenGL render targets.
///
/// See `mln_egl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct EglContextDescriptor {
    /// Borrowed EGLDisplay. Required and kept initialized through teardown.
    pub display: *mut std::ffi::c_void,
    /// Borrowed EGLConfig used to create the session context. Required. OpenGL
    /// texture targets require EGL_SURFACE_TYPE to include EGL_PBUFFER_BIT.
    pub config: *mut std::ffi::c_void,
    /// Borrowed EGLContext whose share group the session context joins.
    /// Required under shared ownership, where the session also takes its client
    /// API from this context. A dedicated session joins no share group, so it
    /// must be null there and names client_api instead.
    pub share_context: *mut std::ffi::c_void,
    /// Client API the session creates its context for. Required under dedicated
    /// ownership. A shared session queries share_context for it, so this is
    /// ignored there.
    pub client_api: OpenglClientApi,
    /// Optional eglGetProcAddress-compatible function for the host loader.
    pub get_proc_address: *mut std::ffi::c_void,
}
impl EglContextDescriptor {
    pub const fn new(
        display: *mut std::ffi::c_void,
        config: *mut std::ffi::c_void,
        share_context: *mut std::ffi::c_void,
        client_api: OpenglClientApi,
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
}
impl ToNative<sys::mln_egl_context_descriptor> for EglContextDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_egl_context_descriptor> {
        let mut raw: sys::mln_egl_context_descriptor = unsafe { std::mem::zeroed() };
        raw.display = self.display;
        raw.config = self.config;
        raw.share_context = self.share_context;
        raw.client_api = to_native(&self.client_api, arena)?;
        raw.get_proc_address = self.get_proc_address;
        Ok(raw)
    }
}
impl FromNative<sys::mln_egl_context_descriptor> for EglContextDescriptor {
    unsafe fn from_native(raw: sys::mln_egl_context_descriptor) -> Result<Self> {
        Ok(Self {
            display: raw.display,
            config: raw.config,
            share_context: raw.share_context,
            client_api: unsafe { from_native(raw.client_api) }?,
            get_proc_address: raw.get_proc_address,
        })
    }
}

/// A borrowed view of one owned runtime-event batch.
///
/// See `mln_event_batch_view` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct EventBatchView {
    /// Borrowed array of event_count events in queue order.
    pub events: Vec<RuntimeEvent>,
}
impl FromNative<sys::mln_event_batch_view> for EventBatchView {
    unsafe fn from_native(raw: sys::mln_event_batch_view) -> Result<Self> {
        Ok(Self {
            events: unsafe { convert::strided_items(raw.events, raw.event_count, raw.event_size) }?
                .into_iter()
                .map(|item| -> Result<RuntimeEvent> {
                    let mut value: RuntimeEvent = unsafe { from_native(item) }?;
                    value.message = unsafe {
                        convert::arena_string(
                            raw.messages,
                            raw.messages_size,
                            item.message_offset,
                            item.message_size,
                        )
                    }?;
                    Ok(value)
                })
                .collect::<Result<Vec<_>>>()?,
        })
    }
}

/// Feature-state source, feature, and key selector.
///
/// See `mln_feature_state_selector` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct FeatureStateSelector {
    /// Source ID. Required and borrowed for the duration of the call.
    pub source_id: String,
    /// Optional source layer ID. Required for vector-source disambiguation.
    pub source_layer_id: Option<String>,
    /// Optional feature ID string. Required by set/get and optional for remove.
    pub feature_id: Option<String>,
    /// Optional state key. Used only by remove and requires feature_id.
    pub state_key: Option<String>,
}
impl ToNative<sys::mln_feature_state_selector> for FeatureStateSelector {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_feature_state_selector> {
        let mut raw: sys::mln_feature_state_selector = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_feature_state_selector>() as _;
        raw.fields = 0;
        raw.source_id = to_native(&self.source_id, arena)?;
        if let Some(item) = &self.source_layer_id {
            raw.fields |= sys::MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID;
            raw.source_layer_id = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.feature_id {
            raw.fields |= sys::MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
            raw.feature_id = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.state_key {
            raw.fields |= sys::MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
            raw.state_key = to_native(&*item, arena)?;
        }
        Ok(raw)
    }
}

native_flags! {
/// Optional fields for `mln_feature_state_selector`.
///
/// See `mln_feature_state_selector_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub struct FeatureStateSelectorField: u32 {
    const SOURCE_LAYER_ID = 1;
    const FEATURE_ID = 2;
    const STATE_KEY = 4;
}
}

/// One nonblocking request for a frame.
///
/// See `mln_frame_demand` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct FrameDemand {
    /// A bitwise OR of `mln_frame_demand_flag` values. Defaults to
    /// `MLN_FRAME_DEMAND_IF_NEEDED`.
    pub flags: FrameDemandFlag,
    /// Host identity returned with the terminal frame result.
    pub token: u64,
    /// Demands coalesce only when this value and their flags match.
    pub coalescing_boundary: u64,
    /// Positive time allowed before driver work begins, in nanoseconds; zero
    /// has no limit. A demand that waits, for a free texture slot or for a map
    /// update, is checked against its timeout when it runs again; a wait has no
    /// timer of its own.
    pub timeout_ns: u64,
}
impl Default for FrameDemand {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_frame_demand_default() })
    }
}
impl FrameDemand {
    pub const fn new(
        flags: FrameDemandFlag,
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
}
impl ToNative<sys::mln_frame_demand> for FrameDemand {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_frame_demand> {
        let mut raw: sys::mln_frame_demand = unsafe { sys::mln_frame_demand_default() };
        raw.size = std::mem::size_of::<sys::mln_frame_demand>() as _;
        raw.flags = to_native(&self.flags, arena)?;
        raw.token = self.token;
        raw.coalescing_boundary = self.coalescing_boundary;
        raw.timeout_ns = self.timeout_ns;
        Ok(raw)
    }
}
impl FromNative<sys::mln_frame_demand> for FrameDemand {
    unsafe fn from_native(raw: sys::mln_frame_demand) -> Result<Self> {
        Ok(Self {
            flags: unsafe { from_native(raw.flags) }?,
            token: raw.token,
            coalescing_boundary: raw.coalescing_boundary,
            timeout_ns: raw.timeout_ns,
        })
    }
}

native_flags! {
/// Frame-demand policy bits.
///
/// See `mln_frame_demand_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
pub struct FrameDemandFlag: u32 {
    /// Render only when a newer map update exists.
    const IF_NEEDED = 1;
    /// Present the rendered frame on a target that supports presentation. A
    /// presenting target whose demand clears this bit still renders and keeps
    /// whatever it presented last. Ignored by targets without presentation.
    const PRESENT = 2;
    /// With `MLN_FRAME_DEMAND_IF_NEEDED`, a demand that would finish with
    /// `MLN_RENDER_RESULT_NO_UPDATE` or `MLN_RENDER_RESULT_SIZE_PENDING` waits
    /// instead, and runs again after the map's next update, a target
    /// replacement, or an applied resize. A waiting demand holds no ring slot.
    /// A later demand with the same flags and coalescing boundary supersedes
    /// it, a barrier ends its wait with `MLN_RENDER_RESULT_NO_UPDATE`, and
    /// detach, abandon, or the quarantine of the ring's last usable slot end it
    /// with `MLN_RENDER_RESULT_TARGET_NOT_READY`. A waiting demand's result can
    /// follow the results of demands accepted after it. The flag does not pace:
    /// a host that re-arms a waiting demand as each result arrives renders
    /// every update the map publishes.
    const WAIT_FOR_UPDATE = 4;
}
}

native_flags! {
/// Field mask values for `mln_free_camera_options`.
///
/// See `mln_free_camera_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub struct FreeCameraOptionField: u32 {
    const POSITION = 1;
    const ORIENTATION = 2;
}
}

/// Free camera position and orientation in MapLibre Native camera space.
///
/// See `mln_free_camera_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct FreeCameraOptions {
    pub position: Option<Vec3>,
    pub orientation: Option<Quaternion>,
}
impl Default for FreeCameraOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_free_camera_options_default() })
    }
}
impl ToNative<sys::mln_free_camera_options> for FreeCameraOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_free_camera_options> {
        let mut raw: sys::mln_free_camera_options =
            unsafe { sys::mln_free_camera_options_default() };
        raw.size = std::mem::size_of::<sys::mln_free_camera_options>() as _;
        raw.fields = 0;
        if let Some(item) = &self.position {
            raw.fields |= sys::MLN_FREE_CAMERA_OPTION_POSITION;
            raw.position = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.orientation {
            raw.fields |= sys::MLN_FREE_CAMERA_OPTION_ORIENTATION;
            raw.orientation = to_native(&*item, arena)?;
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_free_camera_options> for FreeCameraOptions {
    unsafe fn from_native(raw: sys::mln_free_camera_options) -> Result<Self> {
        Ok(Self {
            position: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_FREE_CAMERA_OPTION_POSITION,
                    raw.position,
                )
            }?,
            orientation: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_FREE_CAMERA_OPTION_ORIENTATION,
                    raw.orientation,
                )
            }?,
        })
    }
}

native_flags! {
/// Field mask values for `mln_geojson_source_options`.
///
/// See `mln_geojson_source_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub struct GeojsonSourceOptionField: u32 {
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
}
}

/// Options for GeoJSON sources.
///
/// See `mln_geojson_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct GeojsonSourceOptions {
    /// Minimum tiling zoom. Defaults to 0.
    pub min_zoom: Option<f64>,
    /// Maximum tiling zoom. Defaults to 18.
    pub max_zoom: Option<f64>,
    /// Douglas-Peucker simplification tolerance. Defaults to 0.375.
    pub tolerance: Option<f64>,
    /// Highest zoom that clusters points. Defaults to 17.
    pub cluster_max_zoom: Option<f64>,
    /// Cluster aggregation expressions keyed by property name, as a JSON object
    /// whose members follow the MapLibre Style Spec clusterProperties form. The
    /// UTF-8 bytes are borrowed for the call.
    pub cluster_properties: Option<Vec<u8>>,
    /// Tile extent in pixels. Defaults to 512.
    pub tile_size: Option<u32>,
    /// Tile buffer in pixels. Defaults to 128.
    pub buffer: Option<u32>,
    /// Cluster radius in pixels. Defaults to 50.
    pub cluster_radius: Option<u32>,
    /// Points required to form a cluster. Defaults to 2.
    pub cluster_min_points: Option<u32>,
    /// Adds line distance metrics to line features. Defaults to false.
    pub line_metrics: Option<bool>,
    /// Clusters point features. Defaults to false.
    pub cluster: Option<bool>,
    /// Slices requested tiles inline during the update pass. Defaults to false.
    pub synchronous_tiling: Option<bool>,
}
impl Default for GeojsonSourceOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_geojson_source_options_default() })
    }
}
impl ToNative<sys::mln_geojson_source_options> for GeojsonSourceOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_geojson_source_options> {
        let mut raw: sys::mln_geojson_source_options =
            unsafe { sys::mln_geojson_source_options_default() };
        raw.size = std::mem::size_of::<sys::mln_geojson_source_options>() as _;
        raw.fields = 0;
        if let Some(item) = &self.min_zoom {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM;
            raw.min_zoom = *item;
        }
        if let Some(item) = &self.max_zoom {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM;
            raw.max_zoom = *item;
        }
        if let Some(item) = &self.tolerance {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_TOLERANCE;
            raw.tolerance = *item;
        }
        if let Some(item) = &self.cluster_max_zoom {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM;
            raw.cluster_max_zoom = *item;
        }
        if let Some(item) = &self.cluster_properties {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
            raw.cluster_properties = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.tile_size {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE;
            raw.tile_size = *item;
        }
        if let Some(item) = &self.buffer {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_BUFFER;
            raw.buffer = *item;
        }
        if let Some(item) = &self.cluster_radius {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS;
            raw.cluster_radius = *item;
        }
        if let Some(item) = &self.cluster_min_points {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS;
            raw.cluster_min_points = *item;
        }
        if let Some(item) = &self.line_metrics {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS;
            raw.line_metrics = *item;
        }
        if let Some(item) = &self.cluster {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
            raw.cluster = *item;
        }
        if let Some(item) = &self.synchronous_tiling {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING;
            raw.synchronous_tiling = *item;
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_geojson_source_options> for GeojsonSourceOptions {
    unsafe fn from_native(raw: sys::mln_geojson_source_options) -> Result<Self> {
        Ok(Self {
            min_zoom: (raw.fields & sys::MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM != 0)
                .then_some(raw.min_zoom),
            max_zoom: (raw.fields & sys::MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM != 0)
                .then_some(raw.max_zoom),
            tolerance: (raw.fields & sys::MLN_GEOJSON_SOURCE_OPTION_TOLERANCE != 0)
                .then_some(raw.tolerance),
            cluster_max_zoom: (raw.fields & sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM != 0)
                .then_some(raw.cluster_max_zoom),
            cluster_properties: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES,
                    raw.cluster_properties,
                )
            }?,
            tile_size: (raw.fields & sys::MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE != 0)
                .then_some(raw.tile_size),
            buffer: (raw.fields & sys::MLN_GEOJSON_SOURCE_OPTION_BUFFER != 0).then_some(raw.buffer),
            cluster_radius: (raw.fields & sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS != 0)
                .then_some(raw.cluster_radius),
            cluster_min_points: (raw.fields & sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
                != 0)
                .then_some(raw.cluster_min_points),
            line_metrics: (raw.fields & sys::MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS != 0)
                .then_some(raw.line_metrics),
            cluster: (raw.fields & sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER != 0)
                .then_some(raw.cluster),
            synchronous_tiling: (raw.fields & sys::MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING
                != 0)
                .then_some(raw.synchronous_tiling),
        })
    }
}

native_enum! {
/// Gesture boundary carried atomically with a camera update or delta.
///
/// See `mln_gesture_phase` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub enum GesturePhase: u32 {
    /// The update carries no gesture boundary and leaves the flag as it is.
    None = 0,
    /// Marks a gesture as in progress before the camera write. It does not
    /// cancel running transitions; use `mln_map_cancel_transitions()` for that.
    Begin = 1,
    /// Keeps the gesture marked as in progress before the camera write.
    Update = 2,
    /// Clears the gesture flag after the camera write.
    End = 3,
    /// Cancels transitions running after the camera write, then clears the
    /// gesture flag.
    Cancel = 4,
} Unknown
}

/// Backend synchronization copied by frame access and release calls.
///
/// See `mln_gpu_sync` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct GpuSync {
    /// One `mln_gpu_sync_kind` value.
    pub kind: GpuSyncKind,
    /// Bit pattern of the backend object that kind names: the
    /// `id<MTLSharedEvent>` pointer, the VkSemaphore handle, the GLsync
    /// pointer, or the WebGPU token.
    pub object: u64,
    pub value: u64,
}
impl Default for GpuSync {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_gpu_sync_default() })
    }
}
impl GpuSync {
    pub const fn new(kind: GpuSyncKind, object: u64, value: u64) -> Self {
        Self {
            kind,
            object,
            value,
        }
    }
}
impl ToNative<sys::mln_gpu_sync> for GpuSync {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_gpu_sync> {
        let mut raw: sys::mln_gpu_sync = unsafe { sys::mln_gpu_sync_default() };
        raw.size = std::mem::size_of::<sys::mln_gpu_sync>() as _;
        raw.kind = to_native(&self.kind, arena)?;
        raw.object = self.object;
        raw.value = self.value;
        Ok(raw)
    }
}
impl FromNative<sys::mln_gpu_sync> for GpuSync {
    unsafe fn from_native(raw: sys::mln_gpu_sync) -> Result<Self> {
        Ok(Self {
            kind: unsafe { from_native(raw.kind) }?,
            object: raw.object,
            value: raw.value,
        })
    }
}

native_enum! {
/// Synchronization payload kind for acquired texture frames.
///
/// See `mln_gpu_sync_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub enum GpuSyncKind: u32 {
    /// The host needs no synchronization object. The work completed, or on
    /// WebGPU was submitted to the device's queue, before the frame became
    /// acquirable or before the release call.
    CpuComplete = 0,
    /// `id<MTLSharedEvent>` plus a monotonically increasing signal value.
    MetalSharedEvent = 1,
    /// VkSemaphore plus a timeline value.
    VulkanTimelineSemaphore = 2,
    /// GLsync, used only by a caller-graphics-thread driver.
    OpenglFence = 3,
    /// A backend-defined WebGPU completion token.
    WebgpuToken = 4,
} Unknown
}

#[derive(Clone, Default)]
pub struct HttpHeaderTransform {
    pub callback: Option<
        std::sync::Arc<
            dyn Fn(ResourceKind, String, &mut HttpHeaderTransformResponse<'_>) -> Result<()>
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
        F: Fn(ResourceKind, String, &mut HttpHeaderTransformResponse<'_>) -> Result<()>
            + Send
            + Sync
            + 'static,
    {
        self.callback = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(ResourceKind, String, &mut HttpHeaderTransformResponse<'_>) -> Result<()>
            + Send
            + Sync
            + 'static,
    {
        Self::default().with_callback(callback)
    }
    unsafe extern "C" fn callback_trampoline(
        user_data: *mut std::ffi::c_void,
        kind: u32,
        url: *const std::ffi::c_char,
        out_response: *mut sys::mln_http_header_transform_response,
    ) -> sys::mln_status {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke_status(
            "mln_http_header_transform_callback",
            Some((
                &["mln_http_header_transform_response_set_header"],
                out_response as usize as u64,
            )),
            sys::MLN_STATUS_NATIVE_ERROR,
            || {
                callback::require(&state.callback)?(
                    unsafe { from_native(kind) }?,
                    unsafe { from_native(url) }?,
                    &mut HttpHeaderTransformResponse::new(out_response)?,
                )
            },
        )
    }
}
impl ToNative<sys::mln_http_header_transform> for HttpHeaderTransform {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_http_header_transform> {
        let mut raw: sys::mln_http_header_transform = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_http_header_transform>() as _;
        raw.callback = self
            .callback
            .as_ref()
            .map(|_| Self::callback_trampoline as _);
        if !(raw.callback.is_none()) {
            // SAFETY: the release reclaims exactly this state.
            raw.user_data = unsafe { arena.registration(self.clone(), callback::release::<Self>) };
            raw.release_user_data = Some(callback::release::<Self>);
        }
        Ok(raw)
    }
}

/// A native response borrowed only for one host callback.
#[derive(Debug)]
pub struct HttpHeaderTransformResponse<'a> {
    raw: std::ptr::NonNull<sys::mln_http_header_transform_response>,
    lifetime: std::marker::PhantomData<&'a mut sys::mln_http_header_transform_response>,
}
impl HttpHeaderTransformResponse<'_> {
    fn new(raw: *mut sys::mln_http_header_transform_response) -> Result<Self> {
        let raw = std::ptr::NonNull::new(raw)
            .ok_or_else(|| Error::invalid_argument("null callback response"))?;
        Ok(Self {
            raw,
            lifetime: std::marker::PhantomData,
        })
    }
    /// Sets one outgoing HTTP request header for the current transform
    /// invocation.
    ///
    /// See `mln_http_header_transform_response_set_header` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_header(&mut self, name: &str, value: &str) -> Result<()> {
        maplibre_core::callback::check(
            "mln_http_header_transform_response_set_header",
            self.raw.as_ptr() as usize as u64,
        )?;
        let name_size = convert::count(name.len())?;
        let value_size = convert::count(value.len())?;
        maplibre_core::check(|out_diagnostic| unsafe {
            sys::mln_http_header_transform_response_set_header(
                self.raw.as_ptr(),
                name.as_ptr().cast(),
                name_size,
                value.as_ptr().cast(),
                value_size,
                out_diagnostic,
            )
        })
    }
}

/// Content-box insets in image pixels, measured from the image's top-left.
///
/// See `mln_image_content` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
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
}
impl ToNative<sys::mln_image_content> for ImageContent {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_image_content> {
        let mut raw: sys::mln_image_content = unsafe { std::mem::zeroed() };
        raw.left = self.left;
        raw.top = self.top;
        raw.right = self.right;
        raw.bottom = self.bottom;
        Ok(raw)
    }
}
impl FromNative<sys::mln_image_content> for ImageContent {
    unsafe fn from_native(raw: sys::mln_image_content) -> Result<Self> {
        Ok(Self {
            left: raw.left,
            top: raw.top,
            right: raw.right,
            bottom: raw.bottom,
        })
    }
}

/// One stretchable interval along an image axis, in image pixels.
///
/// See `mln_image_stretch` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct ImageStretch {
    pub from: f32,
    pub to: f32,
}
impl ImageStretch {
    pub const fn new(from: f32, to: f32) -> Self {
        Self { from, to }
    }
}
impl ToNative<sys::mln_image_stretch> for ImageStretch {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_image_stretch> {
        let mut raw: sys::mln_image_stretch = unsafe { std::mem::zeroed() };
        raw.from = self.from;
        raw.to = self.to;
        Ok(raw)
    }
}
impl FromNative<sys::mln_image_stretch> for ImageStretch {
    unsafe fn from_native(raw: sys::mln_image_stretch) -> Result<Self> {
        Ok(Self {
            from: raw.from,
            to: raw.to,
        })
    }
}

/// Geographic coordinate in degrees used by map and projection APIs.
///
/// See `mln_lat_lng` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct LatLng {
    /// Latitude in degrees. Input latitude must be finite and within \[-90,
    /// 90\].
    pub latitude: f64,
    /// Longitude in degrees. Input longitude must be finite.
    pub longitude: f64,
}
impl LatLng {
    pub const fn new(latitude: f64, longitude: f64) -> Self {
        Self {
            latitude,
            longitude,
        }
    }
}
impl ToNative<sys::mln_lat_lng> for LatLng {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_lat_lng> {
        let mut raw: sys::mln_lat_lng = unsafe { std::mem::zeroed() };
        raw.latitude = self.latitude;
        raw.longitude = self.longitude;
        Ok(raw)
    }
}
impl FromNative<sys::mln_lat_lng> for LatLng {
    unsafe fn from_native(raw: sys::mln_lat_lng) -> Result<Self> {
        Ok(Self {
            latitude: raw.latitude,
            longitude: raw.longitude,
        })
    }
}

/// Geographic bounds in degrees.
///
/// See `mln_lat_lng_bounds` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct LatLngBounds {
    pub southwest: LatLng,
    pub northeast: LatLng,
}
impl LatLngBounds {
    pub const fn new(southwest: LatLng, northeast: LatLng) -> Self {
        Self {
            southwest,
            northeast,
        }
    }
}
impl ToNative<sys::mln_lat_lng_bounds> for LatLngBounds {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_lat_lng_bounds> {
        let mut raw: sys::mln_lat_lng_bounds = unsafe { std::mem::zeroed() };
        raw.southwest = to_native(&self.southwest, arena)?;
        raw.northeast = to_native(&self.northeast, arena)?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_lat_lng_bounds> for LatLngBounds {
    unsafe fn from_native(raw: sys::mln_lat_lng_bounds) -> Result<Self> {
        Ok(Self {
            southwest: unsafe { from_native(raw.southwest) }?,
            northeast: unsafe { from_native(raw.northeast) }?,
        })
    }
}

native_enum! {
/// Location indicator image-name properties.
///
/// See `mln_location_indicator_image_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub enum LocationIndicatorImageKind: u32 {
    Top = 0,
    Bearing = 1,
    Shadow = 2,
} Unknown
}

native_enum! {
/// Log event categories emitted by MapLibre Native.
///
/// See `mln_log_event` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
pub enum LogEvent: u32 {
    General = 0,
    Setup = 1,
    Shader = 2,
    ParseStyle = 3,
    ParseTile = 4,
    Render = 5,
    Style = 6,
    Database = 7,
    HttpRequest = 8,
    Sprite = 9,
    Image = 10,
    GraphicsBackend = 11,
    Jni = 12,
    Android = 13,
    Crash = 14,
    Glyph = 15,
    Timing = 16,
} Unknown
}

/// Process-global log callback state.
///
/// See `mln_log_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
#[derive(Clone, Default)]
pub struct LogHandler {
    pub callback: Option<
        std::sync::Arc<dyn Fn(LogSeverity, LogEvent, i64, String) -> u32 + Send + Sync + 'static>,
    >,
}
impl std::fmt::Debug for LogHandler {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("LogHandler").finish_non_exhaustive()
    }
}
impl LogHandler {
    pub fn with_callback<F>(mut self, callback: F) -> Self
    where
        F: Fn(LogSeverity, LogEvent, i64, String) -> u32 + Send + Sync + 'static,
    {
        self.callback = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(LogSeverity, LogEvent, i64, String) -> u32 + Send + Sync + 'static,
    {
        Self::default().with_callback(callback)
    }
    unsafe extern "C" fn callback_trampoline(
        user_data: *mut std::ffi::c_void,
        severity: u32,
        event: u32,
        code: i64,
        message: *const std::ffi::c_char,
    ) -> u32 {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_log_callback", Some((&[], 0)), 0, || {
            let value = callback::require(&state.callback)?(
                unsafe { from_native(severity) }?,
                unsafe { from_native(event) }?,
                code,
                unsafe { from_native(message) }?,
            );
            Ok(value)
        })
    }
}
impl ToNative<sys::mln_log_handler> for LogHandler {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_log_handler> {
        let mut raw: sys::mln_log_handler = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_log_handler>() as _;
        raw.callback = self
            .callback
            .as_ref()
            .map(|_| Self::callback_trampoline as _);
        if !(raw.callback.is_none()) {
            // SAFETY: the release reclaims exactly this state.
            raw.user_data = unsafe { arena.registration(self.clone(), callback::release::<Self>) };
            raw.release_user_data = Some(callback::release::<Self>);
        }
        Ok(raw)
    }
}

native_enum! {
/// Log severity values emitted by MapLibre Native.
///
/// See `mln_log_severity` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
pub enum LogSeverity: u32 {
    Info = 1,
    Warning = 2,
    Error = 3,
} Unknown
}

native_flags! {
/// Bitmask values for log severities dispatched asynchronously.
///
/// See `mln_log_severity_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
pub struct LogSeverityMask: u32 {
    const INFO = 2;
    const WARNING = 4;
    const ERROR = 8;
    const DEFAULT = 6;
    const ALL = 14;
}
}

/// Logical extent in UI pixels and the device-pixel scale.
///
/// See `mln_logical_extent` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct LogicalExtent {
    /// Width in UI pixels. Defaults to 256.
    pub width: u32,
    /// Height in UI pixels. Defaults to 256.
    pub height: u32,
    /// Device pixels per UI pixel. Defaults to 1.0.
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
}
impl ToNative<sys::mln_logical_extent> for LogicalExtent {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_logical_extent> {
        let mut raw: sys::mln_logical_extent = unsafe { std::mem::zeroed() };
        raw.width = self.width;
        raw.height = self.height;
        raw.scale_factor = self.scale_factor;
        Ok(raw)
    }
}
impl FromNative<sys::mln_logical_extent> for LogicalExtent {
    unsafe fn from_native(raw: sys::mln_logical_extent) -> Result<Self> {
        Ok(Self {
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
        })
    }
}

native_flags! {
/// Debug overlay mask values for `mln_map_set_debug_options()`.
///
/// See `mln_map_debug_option` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub struct MapDebugOption: u32 {
    const TILE_BORDERS = 2;
    const PARSE_STATUS = 4;
    const TIMESTAMPS = 8;
    const COLLISION = 16;
    const OVERDRAW = 32;
    const STENCIL_CLIP = 64;
    const DEPTH_BUFFER = 128;
}
}

native_enum! {
/// Map rendering modes used when creating a map.
///
/// See `mln_map_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub enum MapMode: u32 {
    /// Continuously updates as data arrives and map state changes.
    Continuous = 0,
    /// Produces one-off still images of an arbitrary viewport.
    Static = 1,
    /// Produces one-off still images for a single tile.
    Tile = 2,
} Unknown
}

/// Options used when creating a map.
///
/// See `mln_map_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MapOptions {
    /// Initial logical extent. Width and height must be nonzero, and
    /// scale_factor must be finite and positive. scale_factor is fixed for the
    /// map's lifetime.
    pub initial_extent: LogicalExtent,
    /// One of `mln_map_mode`. Defaults to `MLN_MAP_MODE_CONTINUOUS`.
    pub map_mode: MapMode,
    /// Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR
    /// encodings. Defaults to false.
    pub fast_pfor_enabled: bool,
    /// Map-originated event types this map queues, as a bitwise OR of
    /// `mln_runtime_event_mask` values.
    pub event_mask: RuntimeEventMask,
}
impl Default for MapOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_map_options_default() })
    }
}
impl MapOptions {
    pub const fn new(
        initial_extent: LogicalExtent,
        map_mode: MapMode,
        fast_pfor_enabled: bool,
        event_mask: RuntimeEventMask,
    ) -> Self {
        Self {
            initial_extent,
            map_mode,
            fast_pfor_enabled,
            event_mask,
        }
    }
}
impl ToNative<sys::mln_map_options> for MapOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_map_options> {
        let mut raw: sys::mln_map_options = unsafe { sys::mln_map_options_default() };
        raw.size = std::mem::size_of::<sys::mln_map_options>() as _;
        raw.initial_extent = to_native(&self.initial_extent, arena)?;
        raw.map_mode = to_native(&self.map_mode, arena)?;
        raw.fast_pfor_enabled = self.fast_pfor_enabled;
        raw.event_mask = to_native(&self.event_mask, arena)?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_map_options> for MapOptions {
    unsafe fn from_native(raw: sys::mln_map_options) -> Result<Self> {
        Ok(Self {
            initial_extent: unsafe { from_native(raw.initial_extent) }?,
            map_mode: unsafe { from_native(raw.map_mode) }?,
            fast_pfor_enabled: raw.fast_pfor_enabled,
            event_mask: unsafe { from_native(raw.event_mask) }?,
        })
    }
}

/// Immutable map state copied from the latest published generation.
///
/// See `mln_map_snapshot` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MapSnapshot {
    /// Debug overlay mask of `mln_map_debug_option` values.
    pub debug_options: MapDebugOption,
    pub generation: u64,
    pub camera: CameraOptions,
    pub logical_extent: LogicalExtent,
    pub projection_mode: ProjectionMode,
    pub viewport: MapViewportOptions,
    /// True once every requested style and tile resource finished loading.
    pub fully_loaded: bool,
    pub rendering_stats_view_enabled: bool,
    pub repaint_demand: bool,
    /// True while the map is inside a gesture.
    pub gesture_in_progress: bool,
    pub event_mask: RuntimeEventMask,
    /// Generation of the latest render update the map published. A rendered
    /// frame at or past it draws map state that includes every command this
    /// snapshot observes, though animations and resource loads finish in later
    /// frames.
    pub latest_render_update_generation: u64,
    pub tile: MapTileOptions,
    pub bounds: BoundOptions,
    pub free_camera: FreeCameraOptions,
}
impl MapSnapshot {
    pub const fn new(
        debug_options: MapDebugOption,
        generation: u64,
        camera: CameraOptions,
        logical_extent: LogicalExtent,
        projection_mode: ProjectionMode,
        viewport: MapViewportOptions,
        fully_loaded: bool,
        rendering_stats_view_enabled: bool,
        repaint_demand: bool,
        gesture_in_progress: bool,
        event_mask: RuntimeEventMask,
        latest_render_update_generation: u64,
        tile: MapTileOptions,
        bounds: BoundOptions,
        free_camera: FreeCameraOptions,
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
}
impl FromNative<sys::mln_map_snapshot> for MapSnapshot {
    unsafe fn from_native(raw: sys::mln_map_snapshot) -> Result<Self> {
        Ok(Self {
            debug_options: unsafe { from_native(raw.debug_options) }?,
            generation: raw.generation,
            camera: unsafe { from_native(raw.camera) }?,
            logical_extent: unsafe { from_native(raw.logical_extent) }?,
            projection_mode: unsafe { from_native(raw.projection_mode) }?,
            viewport: unsafe { from_native(raw.viewport) }?,
            fully_loaded: raw.fully_loaded,
            rendering_stats_view_enabled: raw.rendering_stats_view_enabled,
            repaint_demand: raw.repaint_demand,
            gesture_in_progress: raw.gesture_in_progress,
            event_mask: unsafe { from_native(raw.event_mask) }?,
            latest_render_update_generation: raw.latest_render_update_generation,
            tile: unsafe { from_native(raw.tile) }?,
            bounds: unsafe { from_native(raw.bounds) }?,
            free_camera: unsafe { from_native(raw.free_camera) }?,
        })
    }
}

native_flags! {
/// Field mask values for `mln_map_tile_options`.
///
/// See `mln_map_tile_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub struct MapTileOptionField: u32 {
    const PREFETCH_ZOOM_DELTA = 1;
    const LOD_MIN_RADIUS = 2;
    const LOD_SCALE = 4;
    const LOD_PITCH_THRESHOLD = 8;
    const LOD_ZOOM_SHIFT = 16;
    const LOD_MODE = 32;
}
}

/// Tile prefetch and LOD tuning controls.
///
/// See `mln_map_tile_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MapTileOptions {
    /// Native uint8_t prefetch zoom delta.
    pub prefetch_zoom_delta: Option<u32>,
    pub lod_min_radius: Option<f64>,
    pub lod_scale: Option<f64>,
    pub lod_pitch_threshold: Option<f64>,
    pub lod_zoom_shift: Option<f64>,
    /// One of `mln_tile_lod_mode`.
    pub lod_mode: Option<TileLodMode>,
}
impl Default for MapTileOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_map_tile_options_default() })
    }
}
impl ToNative<sys::mln_map_tile_options> for MapTileOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_map_tile_options> {
        let mut raw: sys::mln_map_tile_options = unsafe { sys::mln_map_tile_options_default() };
        raw.size = std::mem::size_of::<sys::mln_map_tile_options>() as _;
        raw.fields = 0;
        if let Some(item) = &self.prefetch_zoom_delta {
            raw.fields |= sys::MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
            raw.prefetch_zoom_delta = *item;
        }
        if let Some(item) = &self.lod_min_radius {
            raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS;
            raw.lod_min_radius = *item;
        }
        if let Some(item) = &self.lod_scale {
            raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_SCALE;
            raw.lod_scale = *item;
        }
        if let Some(item) = &self.lod_pitch_threshold {
            raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD;
            raw.lod_pitch_threshold = *item;
        }
        if let Some(item) = &self.lod_zoom_shift {
            raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT;
            raw.lod_zoom_shift = *item;
        }
        if let Some(item) = &self.lod_mode {
            raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_MODE;
            raw.lod_mode = to_native(&*item, arena)?;
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_map_tile_options> for MapTileOptions {
    unsafe fn from_native(raw: sys::mln_map_tile_options) -> Result<Self> {
        Ok(Self {
            prefetch_zoom_delta: (raw.fields & sys::MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA != 0)
                .then_some(raw.prefetch_zoom_delta),
            lod_min_radius: (raw.fields & sys::MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS != 0)
                .then_some(raw.lod_min_radius),
            lod_scale: (raw.fields & sys::MLN_MAP_TILE_OPTION_LOD_SCALE != 0)
                .then_some(raw.lod_scale),
            lod_pitch_threshold: (raw.fields & sys::MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD != 0)
                .then_some(raw.lod_pitch_threshold),
            lod_zoom_shift: (raw.fields & sys::MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT != 0)
                .then_some(raw.lod_zoom_shift),
            lod_mode: unsafe {
                convert::present(raw.fields, sys::MLN_MAP_TILE_OPTION_LOD_MODE, raw.lod_mode)
            }?,
        })
    }
}

native_flags! {
/// Field mask values for `mln_map_viewport_options`.
///
/// See `mln_map_viewport_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub struct MapViewportOptionField: u32 {
    const NORTH_ORIENTATION = 1;
    const CONSTRAIN_MODE = 2;
    const VIEWPORT_MODE = 4;
    const FRUSTUM_OFFSET = 8;
}
}

/// Live map viewport and render-transform controls.
///
/// See `mln_map_viewport_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MapViewportOptions {
    /// One of `mln_north_orientation`.
    pub north_orientation: Option<NorthOrientation>,
    /// One of `mln_constrain_mode`.
    pub constrain_mode: Option<ConstrainMode>,
    /// One of `mln_viewport_mode`.
    pub viewport_mode: Option<ViewportMode>,
    pub frustum_offset: Option<EdgeInsets>,
}
impl Default for MapViewportOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_map_viewport_options_default() })
    }
}
impl ToNative<sys::mln_map_viewport_options> for MapViewportOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_map_viewport_options> {
        let mut raw: sys::mln_map_viewport_options =
            unsafe { sys::mln_map_viewport_options_default() };
        raw.size = std::mem::size_of::<sys::mln_map_viewport_options>() as _;
        raw.fields = 0;
        if let Some(item) = &self.north_orientation {
            raw.fields |= sys::MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
            raw.north_orientation = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.constrain_mode {
            raw.fields |= sys::MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
            raw.constrain_mode = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.viewport_mode {
            raw.fields |= sys::MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
            raw.viewport_mode = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.frustum_offset {
            raw.fields |= sys::MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
            raw.frustum_offset = to_native(&*item, arena)?;
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_map_viewport_options> for MapViewportOptions {
    unsafe fn from_native(raw: sys::mln_map_viewport_options) -> Result<Self> {
        Ok(Self {
            north_orientation: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION,
                    raw.north_orientation,
                )
            }?,
            constrain_mode: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE,
                    raw.constrain_mode,
                )
            }?,
            viewport_mode: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE,
                    raw.viewport_mode,
                )
            }?,
            frustum_offset: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET,
                    raw.frustum_offset,
                )
            }?,
        })
    }
}

/// One caller-owned Metal texture of a borrowed texture ring.
///
/// See `mln_metal_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MetalBorrowedTexture {
    /// Borrowed `id<MTLTexture>` / `MTL::Texture*`. Required.
    pub texture: *mut std::ffi::c_void,
}
impl MetalBorrowedTexture {
    pub const fn new(texture: *mut std::ffi::c_void) -> Self {
        Self { texture }
    }
}
impl ToNative<sys::mln_metal_borrowed_texture> for MetalBorrowedTexture {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_metal_borrowed_texture> {
        let mut raw: sys::mln_metal_borrowed_texture = unsafe { std::mem::zeroed() };
        raw.texture = self.texture;
        Ok(raw)
    }
}
impl FromNative<sys::mln_metal_borrowed_texture> for MetalBorrowedTexture {
    unsafe fn from_native(raw: sys::mln_metal_borrowed_texture) -> Result<Self> {
        Ok(Self {
            texture: raw.texture,
        })
    }
}

/// Metal attachment options for a borrowed texture target.
///
/// See `mln_metal_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct MetalBorrowedTextureDescriptor {
    /// Logical texture extent. The map viewport uses width and height and the
    /// renderer uses scale_factor; the physical size is stated separately
    /// below. A scale_factor that differs from the map's is accepted and logged
    /// as a warning.
    pub extent: LogicalExtent,
    /// Physical texture width in device pixels. Must be positive. Defaults to
    /// 256.
    pub physical_width: u32,
    /// Physical texture height in device pixels. Must be positive. Defaults to
    /// 256.
    pub physical_height: u32,
    /// The ring's textures, one per slot, in slot order. Required.
    pub textures: Vec<MetalBorrowedTexture>,
}
impl Default for MetalBorrowedTextureDescriptor {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_metal_borrowed_texture_descriptor_default() })
    }
}
impl ToNative<sys::mln_metal_borrowed_texture_descriptor> for MetalBorrowedTextureDescriptor {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_metal_borrowed_texture_descriptor> {
        let mut raw: sys::mln_metal_borrowed_texture_descriptor =
            unsafe { sys::mln_metal_borrowed_texture_descriptor_default() };
        raw.size = std::mem::size_of::<sys::mln_metal_borrowed_texture_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.physical_width = self.physical_width;
        raw.physical_height = self.physical_height;
        raw.textures = convert::array(&self.textures, arena)?;
        raw.texture_count = convert::count(self.textures.len())?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_metal_borrowed_texture_descriptor> for MetalBorrowedTextureDescriptor {
    unsafe fn from_native(raw: sys::mln_metal_borrowed_texture_descriptor) -> Result<Self> {
        Ok(Self {
            extent: unsafe { from_native(raw.extent) }?,
            physical_width: raw.physical_width,
            physical_height: raw.physical_height,
            textures: unsafe { convert::copy_array(raw.textures, raw.texture_count) }?,
        })
    }
}

/// Metal backend context fields shared by Metal render targets.
///
/// See `mln_metal_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MetalContextDescriptor {
    /// `id<MTLDevice>` / `MTL::Device*`. Retained when the target requires it.
    pub device: *mut std::ffi::c_void,
}
impl MetalContextDescriptor {
    pub const fn new(device: *mut std::ffi::c_void) -> Self {
        Self { device }
    }
}
impl ToNative<sys::mln_metal_context_descriptor> for MetalContextDescriptor {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_metal_context_descriptor> {
        let mut raw: sys::mln_metal_context_descriptor = unsafe { std::mem::zeroed() };
        raw.device = self.device;
        Ok(raw)
    }
}
impl FromNative<sys::mln_metal_context_descriptor> for MetalContextDescriptor {
    unsafe fn from_native(raw: sys::mln_metal_context_descriptor) -> Result<Self> {
        Ok(Self { device: raw.device })
    }
}

/// Metal attachment options for an owned texture target.
///
/// See `mln_metal_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalOwnedTextureDescriptor {
    /// Logical texture extent. A scale_factor that differs from the map's is
    /// accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Metal backend context. device is required.
    pub context: MetalContextDescriptor,
}
impl Default for MetalOwnedTextureDescriptor {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_metal_owned_texture_descriptor_default() })
    }
}
impl MetalOwnedTextureDescriptor {
    pub const fn new(extent: LogicalExtent, context: MetalContextDescriptor) -> Self {
        Self { extent, context }
    }
}
impl ToNative<sys::mln_metal_owned_texture_descriptor> for MetalOwnedTextureDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_metal_owned_texture_descriptor> {
        let mut raw: sys::mln_metal_owned_texture_descriptor =
            unsafe { sys::mln_metal_owned_texture_descriptor_default() };
        raw.size = std::mem::size_of::<sys::mln_metal_owned_texture_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.context = to_native(&self.context, arena)?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_metal_owned_texture_descriptor> for MetalOwnedTextureDescriptor {
    unsafe fn from_native(raw: sys::mln_metal_owned_texture_descriptor) -> Result<Self> {
        Ok(Self {
            extent: unsafe { from_native(raw.extent) }?,
            context: unsafe { from_native(raw.context) }?,
        })
    }
}

/// Metal attachment options for a native surface.
///
/// See `mln_metal_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalSurfaceDescriptor {
    /// Logical surface extent. A scale_factor that differs from the map's is
    /// accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Metal backend context. device is optional for Metal surfaces.
    pub context: MetalContextDescriptor,
    /// `CAMetalLayer*` / `CA::MetalLayer*` retained by the session. Required.
    pub layer: *mut std::ffi::c_void,
}
impl Default for MetalSurfaceDescriptor {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_metal_surface_descriptor_default() })
    }
}
impl MetalSurfaceDescriptor {
    pub const fn new(
        extent: LogicalExtent,
        context: MetalContextDescriptor,
        layer: *mut std::ffi::c_void,
    ) -> Self {
        Self {
            extent,
            context,
            layer,
        }
    }
}
impl ToNative<sys::mln_metal_surface_descriptor> for MetalSurfaceDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_metal_surface_descriptor> {
        let mut raw: sys::mln_metal_surface_descriptor =
            unsafe { sys::mln_metal_surface_descriptor_default() };
        raw.size = std::mem::size_of::<sys::mln_metal_surface_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.context = to_native(&self.context, arena)?;
        raw.layer = self.layer;
        Ok(raw)
    }
}
impl FromNative<sys::mln_metal_surface_descriptor> for MetalSurfaceDescriptor {
    unsafe fn from_native(raw: sys::mln_metal_surface_descriptor) -> Result<Self> {
        Ok(Self {
            extent: unsafe { from_native(raw.extent) }?,
            context: unsafe { from_native(raw.context) }?,
            layer: raw.layer,
        })
    }
}

/// Metal frame acquired from a texture ring.
///
/// See `mln_metal_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MetalTextureFrame {
    /// Session generation that produced this frame.
    pub generation: u64,
    /// Physical Metal texture width in device pixels.
    pub width: u32,
    /// Physical Metal texture height in device pixels.
    pub height: u32,
    /// UI-to-device pixel scale used for this frame.
    pub scale_factor: f64,
    /// Opaque frame identity used to reject stale releases.
    pub frame_id: u64,
    /// Ring slot that holds this frame. For a borrowed target, the index of its
    /// texture in the descriptor's textures array.
    pub slot: u32,
    /// Borrowed `id<MTLTexture>` / `MTL::Texture*`. Valid until frame release.
    pub texture: *mut std::ffi::c_void,
    /// Borrowed `id<MTLDevice>` / `MTL::Device*`. Valid until frame release.
    pub device: *mut std::ffi::c_void,
    /// Backend-native pixel format value. Metal uses MTLPixelFormat.
    pub pixel_format: u64,
}
impl MetalTextureFrame {
    pub const fn new(
        generation: u64,
        width: u32,
        height: u32,
        scale_factor: f64,
        frame_id: u64,
        slot: u32,
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
            slot,
            texture,
            device,
            pixel_format,
        }
    }
}
impl FromNative<sys::mln_metal_texture_frame> for MetalTextureFrame {
    unsafe fn from_native(raw: sys::mln_metal_texture_frame) -> Result<Self> {
        Ok(Self {
            generation: raw.generation,
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: raw.frame_id,
            slot: raw.slot,
            texture: raw.texture,
            device: raw.device,
            pixel_format: raw.pixel_format,
        })
    }
}

native_enum! {
pub enum NetworkStatus: u32 {
    Online = 1,
    Offline = 2,
} Unknown
}

native_enum! {
/// Map north orientation values used by `mln_map_viewport_options`.
///
/// See `mln_north_orientation` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub enum NorthOrientation: u32 {
    Up = 0,
    Right = 1,
    Down = 2,
    Left = 3,
} Unknown
}

/// Geometry offline region definition.
///
/// See `mln_offline_geometry_region_definition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct OfflineGeometryRegionDefinition {
    /// Style URL. Copied during region creation.
    pub style_url: String,
    /// UTF-8 GeoJSON Geometry bytes. Borrowed during region creation.
    pub geometry: Vec<u8>,
    pub min_zoom: f64,
    /// Maximum zoom. Positive infinity follows MapLibre Native behavior and
    /// lets each tile source use its own maximum zoom.
    pub max_zoom: f64,
    pub pixel_ratio: f32,
    pub include_ideographs: bool,
}
impl ToNative<sys::mln_offline_geometry_region_definition> for OfflineGeometryRegionDefinition {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_offline_geometry_region_definition> {
        let mut raw: sys::mln_offline_geometry_region_definition = unsafe { std::mem::zeroed() };
        raw.style_url = to_native(&self.style_url, arena)?;
        raw.geometry = to_native(&self.geometry, arena)?;
        raw.min_zoom = self.min_zoom;
        raw.max_zoom = self.max_zoom;
        raw.pixel_ratio = self.pixel_ratio;
        raw.include_ideographs = self.include_ideographs;
        Ok(raw)
    }
}
impl FromNative<sys::mln_offline_geometry_region_definition> for OfflineGeometryRegionDefinition {
    unsafe fn from_native(raw: sys::mln_offline_geometry_region_definition) -> Result<Self> {
        Ok(Self {
            style_url: unsafe { from_native(raw.style_url) }?,
            geometry: unsafe { from_native(raw.geometry) }?,
            min_zoom: raw.min_zoom,
            max_zoom: raw.max_zoom,
            pixel_ratio: raw.pixel_ratio,
            include_ideographs: raw.include_ideographs,
        })
    }
}

/// Tagged offline region definition.
///
/// See `mln_offline_region_definition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct OfflineRegionDefinition {
    pub data: OfflineRegionDefinitionData,
}
impl OfflineRegionDefinition {
    pub fn tile_pyramid(value: OfflineTilePyramidRegionDefinition) -> Self {
        Self {
            data: OfflineRegionDefinitionData::TilePyramid(value),
        }
    }
    pub fn geometry(value: OfflineGeometryRegionDefinition) -> Self {
        Self {
            data: OfflineRegionDefinitionData::Geometry(value),
        }
    }
}
impl ToNative<sys::mln_offline_region_definition> for OfflineRegionDefinition {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_offline_region_definition> {
        let mut raw: sys::mln_offline_region_definition = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_offline_region_definition>() as _;
        match &self.data {
            OfflineRegionDefinitionData::TilePyramid(item) => {
                raw.type_ = sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID;
                raw.data.tile_pyramid = to_native(&(*item), arena)?;
            }
            OfflineRegionDefinitionData::Geometry(item) => {
                raw.type_ = sys::MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
                raw.data.geometry = to_native(&(*item), arena)?;
            }
            OfflineRegionDefinitionData::Unknown(_) => {
                return Err(Error::invalid_argument(
                    "unknown union variant cannot be submitted",
                ));
            }
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_offline_region_definition> for OfflineRegionDefinition {
    unsafe fn from_native(raw: sys::mln_offline_region_definition) -> Result<Self> {
        Ok(Self {
            data: match raw.type_ {
                sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID => {
                    OfflineRegionDefinitionData::TilePyramid(unsafe {
                        from_native(raw.data.tile_pyramid)
                    }?)
                }
                sys::MLN_OFFLINE_REGION_DEFINITION_GEOMETRY => {
                    OfflineRegionDefinitionData::Geometry(unsafe {
                        from_native(raw.data.geometry)
                    }?)
                }
                tag => OfflineRegionDefinitionData::Unknown(tag as u32),
            },
        })
    }
}

/// Offline region definition data.
///
/// See `mln_offline_region_definition_data` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub enum OfflineRegionDefinitionData {
    TilePyramid(OfflineTilePyramidRegionDefinition),
    Geometry(OfflineGeometryRegionDefinition),
    Unknown(u32),
}
impl Default for OfflineRegionDefinitionData {
    fn default() -> Self {
        Self::Unknown(0)
    }
}

native_enum! {
pub enum OfflineRegionDefinitionType: u32 {
    TilePyramid = 1,
    Geometry = 2,
} Unknown
}

native_enum! {
pub enum OfflineRegionDownloadState: u32 {
    Inactive = 0,
    Active = 1,
} Unknown
}

/// Region data delivered by an offline completion.
///
/// See `mln_offline_region_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct OfflineRegionInfo {
    pub id: i64,
    pub definition: OfflineRegionDefinition,
    /// Metadata bytes.
    pub metadata: Vec<u8>,
}
impl FromNative<sys::mln_offline_region_info> for OfflineRegionInfo {
    unsafe fn from_native(raw: sys::mln_offline_region_info) -> Result<Self> {
        Ok(Self {
            id: raw.id,
            definition: unsafe { from_native(raw.definition) }?,
            metadata: unsafe { convert::counted(raw.metadata, raw.metadata_size) }?,
        })
    }
}

/// Offline region status snapshot.
///
/// See `mln_offline_region_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct OfflineRegionStatus {
    /// One of `mln_offline_region_download_state`.
    pub download_state: OfflineRegionDownloadState,
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
        download_state: OfflineRegionDownloadState,
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
}
impl FromNative<sys::mln_offline_region_status> for OfflineRegionStatus {
    unsafe fn from_native(raw: sys::mln_offline_region_status) -> Result<Self> {
        Ok(Self {
            download_state: unsafe { from_native(raw.download_state) }?,
            completed_resource_count: raw.completed_resource_count,
            completed_resource_size: raw.completed_resource_size,
            completed_tile_count: raw.completed_tile_count,
            required_tile_count: raw.required_tile_count,
            completed_tile_size: raw.completed_tile_size,
            required_resource_count: raw.required_resource_count,
            required_resource_count_is_precise: raw.required_resource_count_is_precise,
            complete: raw.complete,
        })
    }
}

/// Tile-pyramid offline region definition.
///
/// See `mln_offline_tile_pyramid_region_definition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct OfflineTilePyramidRegionDefinition {
    /// Style URL. Copied during region creation.
    pub style_url: String,
    pub bounds: LatLngBounds,
    pub min_zoom: f64,
    /// Maximum zoom. Positive infinity follows MapLibre Native behavior and
    /// lets each tile source use its own maximum zoom.
    pub max_zoom: f64,
    pub pixel_ratio: f32,
    pub include_ideographs: bool,
}
impl ToNative<sys::mln_offline_tile_pyramid_region_definition>
    for OfflineTilePyramidRegionDefinition
{
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_offline_tile_pyramid_region_definition> {
        let mut raw: sys::mln_offline_tile_pyramid_region_definition =
            unsafe { std::mem::zeroed() };
        raw.style_url = to_native(&self.style_url, arena)?;
        raw.bounds = to_native(&self.bounds, arena)?;
        raw.min_zoom = self.min_zoom;
        raw.max_zoom = self.max_zoom;
        raw.pixel_ratio = self.pixel_ratio;
        raw.include_ideographs = self.include_ideographs;
        Ok(raw)
    }
}
impl FromNative<sys::mln_offline_tile_pyramid_region_definition>
    for OfflineTilePyramidRegionDefinition
{
    unsafe fn from_native(raw: sys::mln_offline_tile_pyramid_region_definition) -> Result<Self> {
        Ok(Self {
            style_url: unsafe { from_native(raw.style_url) }?,
            bounds: unsafe { from_native(raw.bounds) }?,
            min_zoom: raw.min_zoom,
            max_zoom: raw.max_zoom,
            pixel_ratio: raw.pixel_ratio,
            include_ideographs: raw.include_ideographs,
        })
    }
}

/// One caller-owned OpenGL texture of a borrowed texture ring.
///
/// See `mln_opengl_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct OpenglBorrowedTexture {
    /// Borrowed OpenGL texture object name. Required.
    pub texture: u32,
}
impl OpenglBorrowedTexture {
    pub const fn new(texture: u32) -> Self {
        Self { texture }
    }
}
impl ToNative<sys::mln_opengl_borrowed_texture> for OpenglBorrowedTexture {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_opengl_borrowed_texture> {
        let mut raw: sys::mln_opengl_borrowed_texture = unsafe { std::mem::zeroed() };
        raw.texture = self.texture;
        Ok(raw)
    }
}
impl FromNative<sys::mln_opengl_borrowed_texture> for OpenglBorrowedTexture {
    unsafe fn from_native(raw: sys::mln_opengl_borrowed_texture) -> Result<Self> {
        Ok(Self {
            texture: raw.texture,
        })
    }
}

/// OpenGL attachment options for a borrowed texture target.
///
/// See `mln_opengl_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct OpenglBorrowedTextureDescriptor {
    /// Logical texture extent. The map viewport uses width and height and the
    /// renderer uses scale_factor; the physical size is stated separately
    /// below. A scale_factor that differs from the map's is accepted and logged
    /// as a warning.
    pub extent: LogicalExtent,
    /// Physical texture width in device pixels. Must be positive. Defaults to
    /// 256.
    pub physical_width: u32,
    /// Physical texture height in device pixels. Must be positive. Defaults to
    /// 256.
    pub physical_height: u32,
    /// Borrowed OpenGL context provider data. The textures must belong to this
    /// context or a context in the same share group.
    pub context: OpenglContextDescriptor,
    /// The ring's textures, one per slot, in slot order. Required.
    pub textures: Vec<OpenglBorrowedTexture>,
    /// OpenGL texture target of every texture. Must be GL_TEXTURE_2D.
    pub target: u32,
}
impl Default for OpenglBorrowedTextureDescriptor {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_opengl_borrowed_texture_descriptor_default() })
    }
}
impl ToNative<sys::mln_opengl_borrowed_texture_descriptor> for OpenglBorrowedTextureDescriptor {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_opengl_borrowed_texture_descriptor> {
        let mut raw: sys::mln_opengl_borrowed_texture_descriptor =
            unsafe { sys::mln_opengl_borrowed_texture_descriptor_default() };
        raw.size = std::mem::size_of::<sys::mln_opengl_borrowed_texture_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.physical_width = self.physical_width;
        raw.physical_height = self.physical_height;
        raw.context = to_native(&self.context, arena)?;
        raw.textures = convert::array(&self.textures, arena)?;
        raw.texture_count = convert::count(self.textures.len())?;
        raw.target = self.target;
        Ok(raw)
    }
}
impl FromNative<sys::mln_opengl_borrowed_texture_descriptor> for OpenglBorrowedTextureDescriptor {
    unsafe fn from_native(raw: sys::mln_opengl_borrowed_texture_descriptor) -> Result<Self> {
        Ok(Self {
            extent: unsafe { from_native(raw.extent) }?,
            physical_width: raw.physical_width,
            physical_height: raw.physical_height,
            context: unsafe { from_native(raw.context) }?,
            textures: unsafe { convert::copy_array(raw.textures, raw.texture_count) }?,
            target: raw.target,
        })
    }
}

native_enum! {
/// OpenGL client API a dedicated EGL session creates its context for.
///
/// See `mln_opengl_client_api` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub enum OpenglClientApi: u32 {
    /// No client API is named.
    Unspecified = 0,
    /// Desktop OpenGL, as EGL_OPENGL_API names it.
    Gl = 1,
    /// OpenGL ES, as EGL_OPENGL_ES_API names it.
    Gles = 2,
} Unknown
}

/// OpenGL backend context fields shared by OpenGL render targets.
///
/// See `mln_opengl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct OpenglContextDescriptor {
    /// Whether the session shares its driver thread and graphics objects with
    /// the host. A private EGL owned texture and a transferred WebGL canvas are
    /// dedicated to their core worker.
    pub ownership: OpenglContextOwnership,
    pub data: OpenglContextDescriptorData,
}
impl ToNative<sys::mln_opengl_context_descriptor> for OpenglContextDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_opengl_context_descriptor> {
        let mut raw: sys::mln_opengl_context_descriptor = unsafe { std::mem::zeroed() };
        raw.ownership = to_native(&self.ownership, arena)?;
        match &self.data {
            OpenglContextDescriptorData::Wgl(item) => {
                raw.platform = sys::MLN_OPENGL_CONTEXT_PLATFORM_WGL;
                raw.data.wgl = to_native(&(*item), arena)?;
            }
            OpenglContextDescriptorData::Egl(item) => {
                raw.platform = sys::MLN_OPENGL_CONTEXT_PLATFORM_EGL;
                raw.data.egl = to_native(&(*item), arena)?;
            }
            OpenglContextDescriptorData::Webgl(item) => {
                raw.platform = sys::MLN_OPENGL_CONTEXT_PLATFORM_WEBGL;
                raw.data.webgl = to_native(&(*item), arena)?;
            }
            OpenglContextDescriptorData::Unknown(_) => {
                return Err(Error::invalid_argument(
                    "unknown union variant cannot be submitted",
                ));
            }
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_opengl_context_descriptor> for OpenglContextDescriptor {
    unsafe fn from_native(raw: sys::mln_opengl_context_descriptor) -> Result<Self> {
        Ok(Self {
            ownership: unsafe { from_native(raw.ownership) }?,
            data: match raw.platform {
                sys::MLN_OPENGL_CONTEXT_PLATFORM_WGL => {
                    OpenglContextDescriptorData::Wgl(unsafe { from_native(raw.data.wgl) }?)
                }
                sys::MLN_OPENGL_CONTEXT_PLATFORM_EGL => {
                    OpenglContextDescriptorData::Egl(unsafe { from_native(raw.data.egl) }?)
                }
                sys::MLN_OPENGL_CONTEXT_PLATFORM_WEBGL => {
                    OpenglContextDescriptorData::Webgl(unsafe { from_native(raw.data.webgl) }?)
                }
                tag => OpenglContextDescriptorData::Unknown(tag as u32),
            },
        })
    }
}

/// Backend-specific OpenGL context data.
///
/// See `mln_opengl_context_descriptor_data` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub enum OpenglContextDescriptorData {
    Wgl(WglContextDescriptor),
    Egl(EglContextDescriptor),
    Webgl(WebglContextDescriptor),
    Unknown(u32),
}
impl Default for OpenglContextDescriptorData {
    fn default() -> Self {
        Self::Unknown(0)
    }
}

native_enum! {
/// How a session's OpenGL context relates to its driver thread and host
/// graphics state.
///
/// See `mln_opengl_context_ownership` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub enum OpenglContextOwnership: u32 {
    /// The session shares its thread with host graphics work.
    Shared = 0,
    /// The session owns its thread's OpenGL context.
    Dedicated = 1,
} Unknown
}

native_enum! {
/// OpenGL platform context provider used by a context descriptor.
///
/// See `mln_opengl_context_platform` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub enum OpenglContextPlatform: u32 {
    /// No OpenGL context provider is selected.
    Unspecified = 0,
    Wgl = 1,
    Egl = 2,
    /// Emscripten WebGL context handle.
    Webgl = 3,
} Unknown
}

native_flags! {
/// OpenGL context providers supported by this build.
///
/// See `mln_opengl_context_provider_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub struct OpenglContextProviderFlag: u32 {
    const WGL = 1;
    const EGL = 2;
    /// Browser WebGL context imported into an Emscripten module.
    const WEBGL = 4;
}
}

/// OpenGL attachment options for an owned texture target.
///
/// See `mln_opengl_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct OpenglOwnedTextureDescriptor {
    /// Logical texture extent. A scale_factor that differs from the map's is
    /// accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Borrowed OpenGL context provider data. Shared ownership creates a
    /// context whose texture frames the host can acquire. Dedicated EGL or
    /// transferred WebGL ownership creates a private core-worker context for
    /// CPU readback.
    pub context: OpenglContextDescriptor,
}
impl Default for OpenglOwnedTextureDescriptor {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_opengl_owned_texture_descriptor_default() })
    }
}
impl ToNative<sys::mln_opengl_owned_texture_descriptor> for OpenglOwnedTextureDescriptor {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_opengl_owned_texture_descriptor> {
        let mut raw: sys::mln_opengl_owned_texture_descriptor =
            unsafe { sys::mln_opengl_owned_texture_descriptor_default() };
        raw.size = std::mem::size_of::<sys::mln_opengl_owned_texture_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.context = to_native(&self.context, arena)?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_opengl_owned_texture_descriptor> for OpenglOwnedTextureDescriptor {
    unsafe fn from_native(raw: sys::mln_opengl_owned_texture_descriptor) -> Result<Self> {
        Ok(Self {
            extent: unsafe { from_native(raw.extent) }?,
            context: unsafe { from_native(raw.context) }?,
        })
    }
}

/// OpenGL attachment options for a native surface.
///
/// See `mln_opengl_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct OpenglSurfaceDescriptor {
    /// Logical surface extent. A scale_factor that differs from the map's is
    /// accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Borrowed OpenGL context provider data.
    pub context: OpenglContextDescriptor,
    /// Borrowed platform surface handle: an HDC for WGL and an EGLSurface for
    /// EGL, both required. Null for WebGL, whose context carries its canvas
    /// binding.
    pub surface: *mut std::ffi::c_void,
}
impl Default for OpenglSurfaceDescriptor {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_opengl_surface_descriptor_default() })
    }
}
impl ToNative<sys::mln_opengl_surface_descriptor> for OpenglSurfaceDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_opengl_surface_descriptor> {
        let mut raw: sys::mln_opengl_surface_descriptor =
            unsafe { sys::mln_opengl_surface_descriptor_default() };
        raw.size = std::mem::size_of::<sys::mln_opengl_surface_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.context = to_native(&self.context, arena)?;
        raw.surface = self.surface;
        Ok(raw)
    }
}
impl FromNative<sys::mln_opengl_surface_descriptor> for OpenglSurfaceDescriptor {
    unsafe fn from_native(raw: sys::mln_opengl_surface_descriptor) -> Result<Self> {
        Ok(Self {
            extent: unsafe { from_native(raw.extent) }?,
            context: unsafe { from_native(raw.context) }?,
            surface: raw.surface,
        })
    }
}

/// OpenGL frame acquired from a texture ring.
///
/// See `mln_opengl_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct OpenglTextureFrame {
    /// Session generation that produced this frame.
    pub generation: u64,
    /// Physical OpenGL texture width in device pixels.
    pub width: u32,
    /// Physical OpenGL texture height in device pixels.
    pub height: u32,
    /// UI-to-device pixel scale used for this frame.
    pub scale_factor: f64,
    /// Opaque frame identity used to reject stale releases.
    pub frame_id: u64,
    /// Ring slot that holds this frame. For a borrowed target, the index of its
    /// texture in the descriptor's textures array.
    pub slot: u32,
    /// Borrowed OpenGL texture object name. Valid until frame release.
    pub texture: u32,
    /// OpenGL texture target. GL_TEXTURE_2D is the expected target.
    pub target: u32,
    /// OpenGL internal format, such as GL_RGBA8. Zero for a borrowed texture,
    /// whose format the host chose.
    pub internal_format: u32,
    /// OpenGL pixel format, such as GL_RGBA. Zero for a borrowed texture.
    pub format: u32,
    /// OpenGL pixel type, such as GL_UNSIGNED_BYTE. Zero for a borrowed
    /// texture.
    pub r#type: u32,
}
impl OpenglTextureFrame {
    pub const fn new(
        generation: u64,
        width: u32,
        height: u32,
        scale_factor: f64,
        frame_id: u64,
        slot: u32,
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
            slot,
            texture,
            target,
            internal_format,
            format,
            r#type,
        }
    }
}
impl FromNative<sys::mln_opengl_texture_frame> for OpenglTextureFrame {
    unsafe fn from_native(raw: sys::mln_opengl_texture_frame) -> Result<Self> {
        Ok(Self {
            generation: raw.generation,
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: raw.frame_id,
            slot: raw.slot,
            texture: raw.texture,
            target: raw.target,
            internal_format: raw.internal_format,
            format: raw.format,
            r#type: raw.type_,
        })
    }
}

/// Caller-owned premultiplied RGBA8 image pixels.
///
/// See `mln_premultiplied_rgba8_image` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct PremultipliedRgba8Image {
    pub width: u32,
    pub height: u32,
    /// Bytes per image row. Must be at least width \* 4.
    pub stride: u32,
    /// Premultiplied RGBA8 pixels. Must not be null for a non-empty image.
    pub pixels: Vec<u8>,
}
impl Default for PremultipliedRgba8Image {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_premultiplied_rgba8_image_default() })
    }
}
impl ToNative<sys::mln_premultiplied_rgba8_image> for PremultipliedRgba8Image {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_premultiplied_rgba8_image> {
        let mut raw: sys::mln_premultiplied_rgba8_image =
            unsafe { sys::mln_premultiplied_rgba8_image_default() };
        raw.size = std::mem::size_of::<sys::mln_premultiplied_rgba8_image>() as _;
        raw.width = self.width;
        raw.height = self.height;
        raw.stride = self.stride;
        raw.pixels = self.pixels.as_ptr().cast();
        raw.byte_length = convert::count(self.pixels.len())?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_premultiplied_rgba8_image> for PremultipliedRgba8Image {
    unsafe fn from_native(raw: sys::mln_premultiplied_rgba8_image) -> Result<Self> {
        Ok(Self {
            width: raw.width,
            height: raw.height,
            stride: raw.stride,
            pixels: unsafe { convert::counted(raw.pixels, raw.byte_length) }?,
        })
    }
}

/// Lower-level Spherical Mercator projected-meter coordinate.
///
/// See `mln_projected_meters` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct ProjectedMeters {
    /// Distance measured northward from the equator, in meters.
    pub northing: f64,
    /// Distance measured eastward from the prime meridian, in meters.
    pub easting: f64,
}
impl ProjectedMeters {
    pub const fn new(northing: f64, easting: f64) -> Self {
        Self { northing, easting }
    }
}
impl ToNative<sys::mln_projected_meters> for ProjectedMeters {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_projected_meters> {
        let mut raw: sys::mln_projected_meters = unsafe { std::mem::zeroed() };
        raw.northing = self.northing;
        raw.easting = self.easting;
        Ok(raw)
    }
}
impl FromNative<sys::mln_projected_meters> for ProjectedMeters {
    unsafe fn from_native(raw: sys::mln_projected_meters) -> Result<Self> {
        Ok(Self {
            northing: raw.northing,
            easting: raw.easting,
        })
    }
}

/// MapLibre axonometric rendering options used for snapshots and commands.
///
/// See `mln_projection_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct ProjectionMode {
    /// Enables a non-perspective axonometric render transform.
    pub axonometric: Option<bool>,
    /// Native x-skew factor used by the axonometric transform.
    pub x_skew: Option<f64>,
    /// Native y-skew factor used by the axonometric transform.
    pub y_skew: Option<f64>,
}
impl Default for ProjectionMode {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_projection_mode_default() })
    }
}
impl ToNative<sys::mln_projection_mode> for ProjectionMode {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_projection_mode> {
        let mut raw: sys::mln_projection_mode = unsafe { sys::mln_projection_mode_default() };
        raw.size = std::mem::size_of::<sys::mln_projection_mode>() as _;
        raw.fields = 0;
        if let Some(item) = &self.axonometric {
            raw.fields |= sys::MLN_PROJECTION_MODE_AXONOMETRIC;
            raw.axonometric = *item;
        }
        if let Some(item) = &self.x_skew {
            raw.fields |= sys::MLN_PROJECTION_MODE_X_SKEW;
            raw.x_skew = *item;
        }
        if let Some(item) = &self.y_skew {
            raw.fields |= sys::MLN_PROJECTION_MODE_Y_SKEW;
            raw.y_skew = *item;
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_projection_mode> for ProjectionMode {
    unsafe fn from_native(raw: sys::mln_projection_mode) -> Result<Self> {
        Ok(Self {
            axonometric: (raw.fields & sys::MLN_PROJECTION_MODE_AXONOMETRIC != 0)
                .then_some(raw.axonometric),
            x_skew: (raw.fields & sys::MLN_PROJECTION_MODE_X_SKEW != 0).then_some(raw.x_skew),
            y_skew: (raw.fields & sys::MLN_PROJECTION_MODE_Y_SKEW != 0).then_some(raw.y_skew),
        })
    }
}

native_flags! {
/// Field mask values for MapLibre axonometric rendering options.
///
/// See `mln_projection_mode_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub struct ProjectionModeField: u32 {
    const AXONOMETRIC = 1;
    const X_SKEW = 2;
    const Y_SKEW = 4;
}
}

/// Quaternion stored as x, y, z, w components.
///
/// See `mln_quaternion` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
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
}
impl ToNative<sys::mln_quaternion> for Quaternion {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_quaternion> {
        let mut raw: sys::mln_quaternion = unsafe { std::mem::zeroed() };
        raw.x = self.x;
        raw.y = self.y;
        raw.z = self.z;
        raw.w = self.w;
        Ok(raw)
    }
}
impl FromNative<sys::mln_quaternion> for Quaternion {
    unsafe fn from_native(raw: sys::mln_quaternion) -> Result<Self> {
        Ok(Self {
            x: raw.x,
            y: raw.y,
            z: raw.z,
            w: raw.w,
        })
    }
}

/// One query hit borrowed for a completion callback.
///
/// See `mln_queried_feature` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct QueriedFeature {
    pub feature: Vec<u8>,
    pub source_id: Option<String>,
    pub source_layer_id: Option<String>,
    pub state: Option<Vec<u8>>,
}
impl FromNative<sys::mln_queried_feature> for QueriedFeature {
    unsafe fn from_native(raw: sys::mln_queried_feature) -> Result<Self> {
        Ok(Self {
            feature: unsafe { from_native(raw.feature) }?,
            source_id: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_QUERIED_FEATURE_SOURCE_ID,
                    raw.source_id,
                )
            }?,
            source_layer_id: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_QUERIED_FEATURE_SOURCE_LAYER_ID,
                    raw.source_layer_id,
                )
            }?,
            state: unsafe {
                convert::present(raw.fields, sys::MLN_QUERIED_FEATURE_STATE, raw.state)
            }?,
        })
    }
}

native_flags! {
/// Optional fields for `mln_queried_feature`.
///
/// See `mln_queried_feature_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
pub struct QueriedFeatureField: u32 {
    const SOURCE_ID = 1;
    const SOURCE_LAYER_ID = 2;
    const STATE = 4;
}
}

/// Host lock on the graphics queue that a session shares with its host, copied
/// by a successful attach.
///
/// See `mln_queue_lock` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Clone, Default)]
pub struct QueueLock {
    pub lock: Option<std::sync::Arc<dyn Fn() -> () + Send + Sync + 'static>>,
    pub unlock: Option<std::sync::Arc<dyn Fn() -> () + Send + Sync + 'static>>,
}
impl std::fmt::Debug for QueueLock {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("QueueLock").finish_non_exhaustive()
    }
}
impl QueueLock {
    pub fn with_lock<F>(mut self, callback: F) -> Self
    where
        F: Fn() -> () + Send + Sync + 'static,
    {
        self.lock = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn() -> () + Send + Sync + 'static,
    {
        Self::default().with_lock(callback)
    }
    pub fn with_unlock<F>(mut self, callback: F) -> Self
    where
        F: Fn() -> () + Send + Sync + 'static,
    {
        self.unlock = Some(std::sync::Arc::new(callback));
        self
    }
    unsafe extern "C" fn lock_trampoline(user_data: *mut std::ffi::c_void) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_queue_lock_callback", Some((&[], 0)), (), || {
            callback::require(&state.lock)?();
            Ok(())
        })
    }
    unsafe extern "C" fn unlock_trampoline(user_data: *mut std::ffi::c_void) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_queue_lock_callback", Some((&[], 0)), (), || {
            callback::require(&state.unlock)?();
            Ok(())
        })
    }
}
impl ToNative<sys::mln_queue_lock> for QueueLock {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_queue_lock> {
        let mut raw: sys::mln_queue_lock = unsafe { std::mem::zeroed() };
        raw.lock = self.lock.as_ref().map(|_| Self::lock_trampoline as _);
        raw.unlock = self.unlock.as_ref().map(|_| Self::unlock_trampoline as _);
        if !(raw.lock.is_none() && raw.unlock.is_none()) {
            // SAFETY: the release reclaims exactly this state.
            raw.user_data = unsafe { arena.registration(self.clone(), callback::release::<Self>) };
            raw.release_user_data = Some(callback::release::<Self>);
        }
        Ok(raw)
    }
}

native_enum! {
/// What abandon did with a session's graphics resources.
///
/// See `mln_render_abandon_disposition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
pub enum RenderAbandonDisposition: u32 {
    /// Abandon destroyed every graphics resource, or none remained.
    Clean = 0,
    /// Abandon kept graphics resources that it could not safely destroy.
    Quarantined = 1,
} Unknown
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RenderAbandonResult {
    /// One `mln_render_abandon_disposition` value.
    pub disposition: RenderAbandonDisposition,
    /// Backend resource groups intentionally retained until process exit.
    pub quarantined_resource_count: u32,
}
impl RenderAbandonResult {
    pub const fn new(
        disposition: RenderAbandonDisposition,
        quarantined_resource_count: u32,
    ) -> Self {
        Self {
            disposition,
            quarantined_resource_count,
        }
    }
}
impl FromNative<sys::mln_render_abandon_result> for RenderAbandonResult {
    unsafe fn from_native(raw: sys::mln_render_abandon_result) -> Result<Self> {
        Ok(Self {
            disposition: unsafe { from_native(raw.disposition) }?,
            quarantined_resource_count: raw.quarantined_resource_count,
        })
    }
}

native_flags! {
/// Render backend support flags reported by this native library build.
///
/// See `mln_render_backend_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
pub struct RenderBackendFlag: u32 {
    const METAL = 1;
    const VULKAN = 2;
    const OPENGL = 4;
    const WEBGPU = 8;
}
}

native_enum! {
/// Execution placement for one render session.
///
/// See `mln_render_driver_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub enum RenderDriverKind: u32 {
    /// Native code owns a serial worker that initializes, drives, and tears
    /// down transferable graphics state.
    CoreWorker = 1,
    /// The host explicitly calls the narrow driver API from the thread or realm
    /// where its graphics context is current.
    CallerGraphicsThread = 2,
} Unknown
}

/// A borrowed view of one owned frame-result batch.
///
/// See `mln_render_frame_batch_view` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct RenderFrameBatchView {
    /// Borrowed array of result_count terminal frame results in completion
    /// order.
    pub results: Vec<RenderFrameResult>,
}
impl FromNative<sys::mln_render_frame_batch_view> for RenderFrameBatchView {
    unsafe fn from_native(raw: sys::mln_render_frame_batch_view) -> Result<Self> {
        Ok(Self {
            results: unsafe {
                convert::copy_strided(raw.results, raw.result_count, raw.result_size)
            }?,
        })
    }
}

/// Terminal result of one frame demand, held by an owned frame-result batch and
/// copied by `mln_acquired_frame_get_result()`.
///
/// See `mln_render_frame_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RenderFrameResult {
    /// One `mln_render_result` value.
    pub disposition: RenderResult,
    pub token: u64,
    /// Generation of the map render update the demand evaluated. When
    /// disposition is `MLN_RENDER_RESULT_RENDERED`, the frame drew that update;
    /// compare it with `mln_map_snapshot.latest_render_update_generation` to
    /// find the first frame that includes a command.
    pub map_update_generation: u64,
    pub extent_generation: u64,
    /// Zero unless disposition is `MLN_RENDER_RESULT_RENDERED`.
    pub frame_generation: u64,
    /// Whether the map asked for another frame while it rendered this one, as
    /// during an ongoing paint transition. Set only when disposition is
    /// `MLN_RENDER_RESULT_RENDERED`, and false for every other outcome. This is
    /// the same signal that `MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED`
    /// carries in its needs_repaint field, delivered with the frame result so a
    /// host can re-arm its frame loop without the runtime event round trip. A
    /// camera transition does not set it by itself: the map publishes a new
    /// update after each of the transition's frames instead, which a
    /// render-if-needed demand renders. A demand with
    /// `MLN_FRAME_DEMAND_WAIT_FOR_UPDATE` renders each transition update
    /// without a runtime-event round trip; the host re-arms the demand as each
    /// result arrives.
    pub needs_repaint: bool,
}
impl RenderFrameResult {
    pub const fn new(
        disposition: RenderResult,
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
}
impl FromNative<sys::mln_render_frame_result> for RenderFrameResult {
    unsafe fn from_native(raw: sys::mln_render_frame_result) -> Result<Self> {
        Ok(Self {
            disposition: unsafe { from_native(raw.disposition) }?,
            token: raw.token,
            map_update_generation: raw.map_update_generation,
            extent_generation: raw.extent_generation,
            frame_generation: raw.frame_generation,
            needs_repaint: raw.needs_repaint,
        })
    }
}

native_enum! {
/// Render modes reported by render observer events.
///
/// See `mln_render_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub enum RenderMode: u32 {
    Partial = 0,
    Full = 1,
} Unknown
}

native_enum! {
/// Terminal disposition of one accepted frame demand.
///
/// See `mln_render_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
pub enum RenderResult: u32 {
    /// A frame was rendered for acquisition, presentation, or ordered readback.
    Rendered = 0,
    /// No newer map update was available, or the map had no complete frame to
    /// draw yet. The map publishes another update when it has one. A demand
    /// with `MLN_FRAME_DEMAND_WAIT_FOR_UPDATE` waits for that update instead,
    /// and finishes with this result only when a barrier ends its wait.
    NoUpdate = 1,
    /// An ordered extent change had not reached the map. The map publishes an
    /// update at the new extent, which a demand with
    /// `MLN_FRAME_DEMAND_WAIT_FOR_UPDATE` waits for instead of finishing with
    /// this result.
    SizePending = 2,
    /// The target could not produce a frame. The attempt consumes nothing, so a
    /// later demand with the same flags renders what this one would have. This
    /// result does not cause a map update, so the host demands again when the
    /// target can be ready, such as after a paced delay.
    TargetNotReady = 3,
    /// A newer demand in the same coalescing boundary replaced this demand.
    Superseded = 4,
    /// The demand's timeout elapsed before driver work began.
    DeadlineMissed = 5,
} Unknown
}

/// Common attachment policy copied before an attach call returns.
///
/// See `mln_render_session_attach_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone)]
pub struct RenderSessionAttachOptions {
    /// One `mln_render_driver_kind` value. Defaults to
    /// `MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD`.
    pub driver: RenderDriverKind,
    /// Requested slot count of a session-owned texture ring, from one to three.
    /// Private targets grant one slot regardless of this value. A borrowed
    /// texture ring's depth is its texture count, so borrowed and other targets
    /// ignore this value. Defaults to 1.
    pub requested_texture_ring_depth: u32,
    /// Wakes the receiver when the frame-result queue becomes nonempty.
    pub frame_wake: Wake,
    /// Wakes the graphics receiver when caller-driver work is available.
    pub driver_work_wake: Wake,
    /// Host lock on the graphics queue, disabled by default. Only Vulkan
    /// targets accept an enabled lock; other backends fail the attach with
    /// `MLN_STATUS_UNSUPPORTED`.
    pub queue_lock: QueueLock,
}
impl Default for RenderSessionAttachOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_render_session_attach_options_default() })
    }
}
impl ToNative<sys::mln_render_session_attach_options> for RenderSessionAttachOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_render_session_attach_options> {
        let mut raw: sys::mln_render_session_attach_options =
            unsafe { sys::mln_render_session_attach_options_default() };
        raw.size = std::mem::size_of::<sys::mln_render_session_attach_options>() as _;
        raw.driver = to_native(&self.driver, arena)?;
        raw.requested_texture_ring_depth = self.requested_texture_ring_depth;
        raw.frame_wake = to_native(&self.frame_wake, arena)?;
        raw.driver_work_wake = to_native(&self.driver_work_wake, arena)?;
        raw.queue_lock = to_native(&self.queue_lock, arena)?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_render_session_attach_options> for RenderSessionAttachOptions {
    unsafe fn from_native(raw: sys::mln_render_session_attach_options) -> Result<Self> {
        Ok(Self {
            driver: unsafe { from_native(raw.driver) }?,
            requested_texture_ring_depth: raw.requested_texture_ring_depth,
            frame_wake: Default::default(),
            driver_work_wake: Default::default(),
            queue_lock: Default::default(),
        })
    }
}

/// Driver and target capabilities fixed for one attached render session.
///
/// See `mln_render_session_capabilities` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RenderSessionCapabilities {
    /// One `mln_render_driver_kind` value.
    pub driver: RenderDriverKind,
    /// Granted texture ring depth: the slot count of a session-owned ring, or
    /// the texture count of a borrowed one. Zero for a surface.
    pub texture_ring_depth: u32,
    /// A bitwise OR of `mln_render_session_capability_flag` values.
    pub flags: RenderSessionCapabilityFlag,
}
impl RenderSessionCapabilities {
    pub const fn new(
        driver: RenderDriverKind,
        texture_ring_depth: u32,
        flags: RenderSessionCapabilityFlag,
    ) -> Self {
        Self {
            driver,
            texture_ring_depth,
            flags,
        }
    }
}
impl FromNative<sys::mln_render_session_capabilities> for RenderSessionCapabilities {
    unsafe fn from_native(raw: sys::mln_render_session_capabilities) -> Result<Self> {
        Ok(Self {
            driver: unsafe { from_native(raw.driver) }?,
            texture_ring_depth: raw.texture_ring_depth,
            flags: unsafe { from_native(raw.flags) }?,
        })
    }
}

native_flags! {
/// Optional render-session capabilities.
///
/// See `mln_render_session_capability_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub struct RenderSessionCapabilityFlag: u32 {
    const FRAME_ACQUISITION = 1;
    const READBACK = 2;
    const CONSUMER_SYNC = 4;
    const PRESENTATION = 8;
}
}

/// Any-thread render-session snapshot.
///
/// See `mln_render_session_snapshot` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RenderSessionSnapshot {
    /// One `mln_render_session_state` value.
    pub state: RenderSessionState,
    /// One `mln_render_driver_kind` value.
    pub driver: RenderDriverKind,
    /// Most recent terminal `mln_render_result` value.
    pub latest_result: RenderResult,
    /// Logical extent, including a resize the driver has not applied yet.
    pub extent: LogicalExtent,
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
        state: RenderSessionState,
        driver: RenderDriverKind,
        latest_result: RenderResult,
        extent: LogicalExtent,
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
}
impl FromNative<sys::mln_render_session_snapshot> for RenderSessionSnapshot {
    unsafe fn from_native(raw: sys::mln_render_session_snapshot) -> Result<Self> {
        Ok(Self {
            state: unsafe { from_native(raw.state) }?,
            driver: unsafe { from_native(raw.driver) }?,
            latest_result: unsafe { from_native(raw.latest_result) }?,
            extent: unsafe { from_native(raw.extent) }?,
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
        })
    }
}

native_enum! {
/// Render-session lifecycle visible in snapshots.
///
/// See `mln_render_session_state` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
pub enum RenderSessionState: u32 {
    Attaching = 1,
    Attached = 2,
    Detaching = 3,
    Detached = 4,
    TargetLost = 5,
    Abandoned = 6,
} Unknown
}

native_flags! {
/// Optional fields for `mln_rendered_feature_query_options`.
///
/// See `mln_rendered_feature_query_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
pub struct RenderedFeatureQueryOptionField: u32 {
    const LAYER_IDS = 1;
    const FILTER = 2;
}
}

/// Options for rendered feature queries.
///
/// See `mln_rendered_feature_query_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct RenderedFeatureQueryOptions {
    /// Optional style layer IDs. When absent, all rendered layers are queried.
    pub layer_ids: Option<Vec<String>>,
    /// Optional UTF-8 MapLibre style-spec filter JSON. When absent, no filter.
    pub filter: Option<Vec<u8>>,
}
impl Default for RenderedFeatureQueryOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_rendered_feature_query_options_default() })
    }
}
impl ToNative<sys::mln_rendered_feature_query_options> for RenderedFeatureQueryOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_rendered_feature_query_options> {
        let mut raw: sys::mln_rendered_feature_query_options =
            unsafe { sys::mln_rendered_feature_query_options_default() };
        raw.size = std::mem::size_of::<sys::mln_rendered_feature_query_options>() as _;
        raw.fields = 0;
        if let Some(item) = &self.layer_ids {
            raw.fields |= sys::MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
            raw.layer_ids = convert::array(&*item, arena)?;
        }
        raw.layer_id_count =
            convert::count(self.layer_ids.as_ref().map_or(0, |items| items.len()))?;
        if let Some(item) = &self.filter {
            raw.fields |= sys::MLN_RENDERED_FEATURE_QUERY_OPTION_FILTER;
            raw.filter = to_native(&*item, arena)?;
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_rendered_feature_query_options> for RenderedFeatureQueryOptions {
    unsafe fn from_native(raw: sys::mln_rendered_feature_query_options) -> Result<Self> {
        Ok(Self {
            layer_ids: if raw.fields & sys::MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS != 0 {
                Some(unsafe { convert::copy_array(raw.layer_ids, raw.layer_id_count) }?)
            } else {
                None
            },
            filter: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_RENDERED_FEATURE_QUERY_OPTION_FILTER,
                    raw.filter,
                )
            }?,
        })
    }
}

/// Rendered feature query geometry descriptor.
///
/// See `mln_rendered_query_geometry` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct RenderedQueryGeometry {
    pub data: RenderedQueryGeometryData,
}
impl RenderedQueryGeometry {
    pub fn point(value: ScreenPoint) -> Self {
        Self {
            data: RenderedQueryGeometryData::Point(value),
        }
    }
    pub fn r#box(value: ScreenBox) -> Self {
        Self {
            data: RenderedQueryGeometryData::Box(value),
        }
    }
    pub fn line_string(value: ScreenLineString) -> Self {
        Self {
            data: RenderedQueryGeometryData::LineString(value),
        }
    }
}
impl ToNative<sys::mln_rendered_query_geometry> for RenderedQueryGeometry {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_rendered_query_geometry> {
        let mut raw: sys::mln_rendered_query_geometry = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_rendered_query_geometry>() as _;
        match &self.data {
            RenderedQueryGeometryData::Point(item) => {
                raw.type_ = sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT;
                raw.data.point = to_native(&(*item), arena)?;
            }
            RenderedQueryGeometryData::Box(item) => {
                raw.type_ = sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX;
                raw.data.box_ = to_native(&(*item), arena)?;
            }
            RenderedQueryGeometryData::LineString(item) => {
                raw.type_ = sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING;
                raw.data.line_string = to_native(&(*item), arena)?;
            }
            RenderedQueryGeometryData::Unknown(_) => {
                return Err(Error::invalid_argument(
                    "unknown union variant cannot be submitted",
                ));
            }
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_rendered_query_geometry> for RenderedQueryGeometry {
    unsafe fn from_native(raw: sys::mln_rendered_query_geometry) -> Result<Self> {
        Ok(Self {
            data: match raw.type_ {
                sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT => {
                    RenderedQueryGeometryData::Point(unsafe { from_native(raw.data.point) }?)
                }
                sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX => {
                    RenderedQueryGeometryData::Box(unsafe { from_native(raw.data.box_) }?)
                }
                sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING => {
                    RenderedQueryGeometryData::LineString(unsafe {
                        from_native(raw.data.line_string)
                    }?)
                }
                tag => RenderedQueryGeometryData::Unknown(tag as u32),
            },
        })
    }
}

/// Screen-space query geometry data.
///
/// See `mln_rendered_query_geometry_data` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub enum RenderedQueryGeometryData {
    Point(ScreenPoint),
    Box(ScreenBox),
    LineString(ScreenLineString),
    Unknown(u32),
}
impl Default for RenderedQueryGeometryData {
    fn default() -> Self {
        Self::Unknown(0)
    }
}

native_enum! {
/// Rendered feature query geometry variants.
///
/// See `mln_rendered_query_geometry_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
pub enum RenderedQueryGeometryType: u32 {
    Point = 1,
    Box = 2,
    LineString = 3,
} Unknown
}

/// Rendering statistics reported in `MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME`.
///
/// See `mln_rendering_stats` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RenderingStats {
    /// Frame CPU encoding time in seconds.
    pub encoding_time: f64,
    /// Frame CPU rendering time in seconds.
    pub rendering_time: f64,
    /// Number of frames rendered by the native renderer.
    pub frame_count: i64,
    /// Draw calls executed during the most recent frame.
    pub draw_call_count: i64,
    /// Total draw calls executed by the native renderer.
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
}
impl FromNative<sys::mln_rendering_stats> for RenderingStats {
    unsafe fn from_native(raw: sys::mln_rendering_stats) -> Result<Self> {
        Ok(Self {
            encoding_time: raw.encoding_time,
            rendering_time: raw.rendering_time,
            frame_count: raw.frame_count,
            draw_call_count: raw.draw_call_count,
            total_draw_call_count: raw.total_draw_call_count,
        })
    }
}

native_enum! {
pub enum ResourceErrorReason: u32 {
    None = 0,
    NotFound = 1,
    Server = 2,
    Connection = 3,
    RateLimit = 4,
    Other = 5,
} Unknown
}

native_enum! {
pub enum ResourceKind: u32 {
    Unknown = 0,
    Style = 1,
    Source = 2,
    Tile = 3,
    Glyphs = 4,
    SpriteImage = 5,
    SpriteJson = 6,
    Image = 7,
} Unrecognized
}

native_enum! {
pub enum ResourceLoadingMethod: u32 {
    All = 0,
    CacheOnly = 1,
    NetworkOnly = 2,
} Unknown
}

native_enum! {
pub enum ResourcePriority: u32 {
    Regular = 0,
    Low = 1,
} Unknown
}

#[derive(Clone, Default)]
pub struct ResourceProvider {
    pub callback: Option<
        std::sync::Arc<
            dyn Fn(ResourceRequest, ResourceRequestHandle) -> ResourceProviderDecision
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
        F: Fn(ResourceRequest, ResourceRequestHandle) -> ResourceProviderDecision
            + Send
            + Sync
            + 'static,
    {
        self.callback = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(ResourceRequest, ResourceRequestHandle) -> ResourceProviderDecision
            + Send
            + Sync
            + 'static,
    {
        Self::default().with_callback(callback)
    }
    unsafe extern "C" fn callback_trampoline(
        user_data: *mut std::ffi::c_void,
        request: *const sys::mln_resource_request,
        handle: sys::mln_resource_request_handle,
    ) -> u32 {
        let _policy = callback::enter(Some((
            &[
                "mln_resource_request_complete",
                "mln_resource_request_is_cancelled",
                "mln_resource_request_set_cancel_callback",
                "mln_resource_request_release",
            ],
            handle.0,
        )));
        // SAFETY: native lends the callback this live decision handle.
        let Ok(request_state) = (unsafe {
            maplibre_core::decision::DecisionHandleState::new(handle, RESOURCE_REQUEST_DECISION)
        }) else {
            return sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
        };
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        match callback::invoke("mln_resource_provider_callback", None, None, || {
            Ok(Some(
                callback::require(&state.callback)?(
                    unsafe { convert::copy_reference(request) }?,
                    ResourceRequestHandle {
                        state: std::sync::Arc::clone(&request_state),
                        not_sync: std::marker::PhantomData,
                    },
                )
                .to_native(),
            ))
        }) {
            Some(sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE) => request_state.finish_decision(true),
            Some(sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH) => {
                request_state.finish_decision(false)
            }
            _ => request_state.finish_exception(),
        }
    }
}
impl ToNative<sys::mln_resource_provider> for ResourceProvider {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_resource_provider> {
        let mut raw: sys::mln_resource_provider = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_resource_provider>() as _;
        raw.callback = self
            .callback
            .as_ref()
            .map(|_| Self::callback_trampoline as _);
        if !(raw.callback.is_none()) {
            // SAFETY: the release reclaims exactly this state.
            raw.user_data = unsafe { arena.registration(self.clone(), callback::release::<Self>) };
            raw.release_user_data = Some(callback::release::<Self>);
        }
        Ok(raw)
    }
}

native_enum! {
pub enum ResourceProviderDecision: u32 {
    PassThrough = 0,
    Handle = 1,
} Unknown
}

/// Inclusive byte range of a resource request.
///
/// See `mln_resource_range` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct ResourceRange {
    /// First byte offset of the requested range.
    pub start: u64,
    /// Last byte offset of the requested range, inclusive.
    pub end: u64,
}
impl ResourceRange {
    pub const fn new(start: u64, end: u64) -> Self {
        Self { start, end }
    }
}
impl FromNative<sys::mln_resource_range> for ResourceRange {
    unsafe fn from_native(raw: sys::mln_resource_range) -> Result<Self> {
        Ok(Self {
            start: raw.start,
            end: raw.end,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct ResourceRequest {
    /// URL entering the network layer, before tile server normalization.
    pub requested_url: Option<String>,
    /// URL to fetch, after resource-kind normalization against the runtime's
    /// tile server options and API key.
    pub resolved_url: Option<String>,
    pub kind: ResourceKind,
    pub loading_method: ResourceLoadingMethod,
    pub priority: ResourcePriority,
    pub usage: ResourceUsage,
    pub storage_policy: ResourceStoragePolicy,
    pub range: Option<ResourceRange>,
    pub prior_modified_unix_ms: Option<i64>,
    pub prior_expires_unix_ms: Option<i64>,
    pub prior_etag: Option<String>,
    pub prior_data: Vec<u8>,
}
impl FromNative<sys::mln_resource_request> for ResourceRequest {
    unsafe fn from_native(raw: sys::mln_resource_request) -> Result<Self> {
        Ok(Self {
            requested_url: unsafe { from_native(raw.requested_url) }?,
            resolved_url: unsafe { from_native(raw.resolved_url) }?,
            kind: unsafe { from_native(raw.kind) }?,
            loading_method: unsafe { from_native(raw.loading_method) }?,
            priority: unsafe { from_native(raw.priority) }?,
            usage: unsafe { from_native(raw.usage) }?,
            storage_policy: unsafe { from_native(raw.storage_policy) }?,
            range: unsafe {
                convert::present(raw.fields, sys::MLN_RESOURCE_REQUEST_RANGE, raw.range)
            }?,
            prior_modified_unix_ms: (raw.fields & sys::MLN_RESOURCE_REQUEST_PRIOR_MODIFIED != 0)
                .then_some(raw.prior_modified_unix_ms),
            prior_expires_unix_ms: (raw.fields & sys::MLN_RESOURCE_REQUEST_PRIOR_EXPIRES != 0)
                .then_some(raw.prior_expires_unix_ms),
            prior_etag: unsafe { from_native(raw.prior_etag) }?,
            prior_data: unsafe { convert::counted(raw.prior_data, raw.prior_data_size) }?,
        })
    }
}

/// Cancel callback state for one handled resource request.
///
/// See `mln_resource_request_cancel_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Default)]
pub struct ResourceRequestCancelHandler {
    pub callback: Option<Box<dyn FnOnce() + Send + 'static>>,
}
impl std::fmt::Debug for ResourceRequestCancelHandler {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("ResourceRequestCancelHandler")
            .finish_non_exhaustive()
    }
}
impl ResourceRequestCancelHandler {
    pub fn with_callback<F>(mut self, callback: F) -> Self
    where
        F: FnOnce() + Send + 'static,
    {
        self.callback = Some(Box::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: FnOnce() + Send + 'static,
    {
        Self::default().with_callback(callback)
    }
}

native_flags! {
/// Field mask values for `mln_resource_request`.
///
/// See `mln_resource_request_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub struct ResourceRequestField: u32 {
    /// The request asks only for the bytes in range.
    const RANGE = 1;
    /// The cached copy being revalidated carries a modification time.
    const PRIOR_MODIFIED = 2;
    /// The cached copy being revalidated carries an expiration time.
    const PRIOR_EXPIRES = 4;
}
}

/// A resource request that a resource provider handles.
///
/// See `mln_resource_request_handle` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
#[derive(Debug)]
pub struct ResourceRequestHandle {
    state: std::sync::Arc<
        maplibre_core::decision::DecisionHandleState<sys::mln_resource_request_handle>,
    >,
    not_sync: std::marker::PhantomData<std::cell::Cell<()>>,
}
impl ResourceRequestHandle {
    /// Completes a C API resource provider request.
    ///
    /// See `mln_resource_request_complete` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn complete(&self, response: &ResourceResponse) -> Result<()> {
        let native = self.state.native_for_call()?;
        maplibre_core::callback::check("mln_resource_request_complete", native.0)?;
        let mut arena = InputArena::default();
        let response = response.to_native(&mut arena)?;
        self.state.complete_with(|handle| {
            maplibre_core::check(|out_diagnostic| unsafe {
                sys::mln_resource_request_complete(handle, &response, out_diagnostic)
            })
        })
    }
    /// Reports whether MapLibre has cancelled a C API resource provider
    /// request.
    ///
    /// See `mln_resource_request_is_cancelled` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn is_cancelled(&self) -> Result<bool> {
        let native = self.state.native_for_call()?;
        maplibre_core::callback::check("mln_resource_request_is_cancelled", native.0)?;
        let mut cancelled = false;
        maplibre_core::check(|out_diagnostic| unsafe {
            sys::mln_resource_request_is_cancelled(native, &mut cancelled, out_diagnostic)
        })?;
        Ok(cancelled)
    }
    /// Registers a callback that runs when MapLibre cancels a C API resource
    /// provider request.
    ///
    /// See `mln_resource_request_set_cancel_callback` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_cancel_callback(&self, handler: ResourceRequestCancelHandler) -> Result<bool> {
        let native = self.state.native_for_call()?;
        maplibre_core::callback::check("mln_resource_request_set_cancel_callback", native.0)?;
        self.state.register_cancel(handler.callback)
    }
    /// Blocks until a resource request is released and its cancel callback
    /// registration has retired: the callback, if it ran, and release_user_data
    /// have both returned. Completing a request does not release its owner.
    ///
    /// See `mln_resource_request_wait_until_retired` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn wait_until_retired(&self) -> Result<()> {
        let native = self.state.issued_handle();
        maplibre_core::callback::check("mln_resource_request_wait_until_retired", native.0)?;
        maplibre_core::check(|out_diagnostic| unsafe {
            sys::mln_resource_request_wait_until_retired(native, out_diagnostic)
        })
    }
    pub fn close(&self) -> Result<()> {
        maplibre_core::callback::check(
            "mln_resource_request_release",
            self.state.issued_handle().0,
        )?;
        self.state.close();
        Ok(())
    }
}

/// A resource provider's answer to one request.
///
/// See `mln_resource_response` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct ResourceResponse {
    pub status: ResourceResponseStatus,
    pub error_reason: ResourceErrorReason,
    /// Response bytes. May be null only when byte_count is 0.
    pub bytes: Vec<u8>,
    pub error_message: Option<String>,
    pub must_revalidate: bool,
    pub modified_unix_ms: Option<i64>,
    pub expires_unix_ms: Option<i64>,
    pub etag: Option<String>,
    pub retry_after_unix_ms: Option<i64>,
}
impl ToNative<sys::mln_resource_response> for ResourceResponse {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_resource_response> {
        let mut raw: sys::mln_resource_response = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_resource_response>() as _;
        raw.fields = 0;
        raw.status = to_native(&self.status, arena)?;
        raw.error_reason = to_native(&self.error_reason, arena)?;
        raw.bytes = self.bytes.as_ptr().cast();
        raw.byte_count = convert::count(self.bytes.len())?;
        raw.error_message = to_native(&self.error_message, arena)?;
        raw.must_revalidate = self.must_revalidate;
        if let Some(item) = &self.modified_unix_ms {
            raw.fields |= sys::MLN_RESOURCE_RESPONSE_MODIFIED;
            raw.modified_unix_ms = *item;
        }
        if let Some(item) = &self.expires_unix_ms {
            raw.fields |= sys::MLN_RESOURCE_RESPONSE_EXPIRES;
            raw.expires_unix_ms = *item;
        }
        raw.etag = to_native(&self.etag, arena)?;
        if let Some(item) = &self.retry_after_unix_ms {
            raw.fields |= sys::MLN_RESOURCE_RESPONSE_RETRY_AFTER;
            raw.retry_after_unix_ms = *item;
        }
        Ok(raw)
    }
}

native_flags! {
/// Field mask values for `mln_resource_response`.
///
/// See `mln_resource_response_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub struct ResourceResponseField: u32 {
    /// The response carries a modification time.
    const MODIFIED = 1;
    /// The response carries an expiration time.
    const EXPIRES = 2;
    /// An ERROR response carries the earliest time to retry the request.
    const RETRY_AFTER = 4;
}
}

native_enum! {
/// How a resource provider answered a request.
///
/// See `mln_resource_response_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub enum ResourceResponseStatus: u32 {
    Ok = 0,
    Error = 1,
    NoContent = 2,
    NotModified = 3,
} Unknown
}

native_enum! {
pub enum ResourceStoragePolicy: u32 {
    Permanent = 0,
    Volatile = 1,
} Unknown
}

#[derive(Clone, Default)]
pub struct ResourceTransform {
    pub callback: Option<
        std::sync::Arc<
            dyn Fn(ResourceKind, String, &mut ResourceTransformResponse<'_>) -> Result<()>
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
        F: Fn(ResourceKind, String, &mut ResourceTransformResponse<'_>) -> Result<()>
            + Send
            + Sync
            + 'static,
    {
        self.callback = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(ResourceKind, String, &mut ResourceTransformResponse<'_>) -> Result<()>
            + Send
            + Sync
            + 'static,
    {
        Self::default().with_callback(callback)
    }
    unsafe extern "C" fn callback_trampoline(
        user_data: *mut std::ffi::c_void,
        kind: u32,
        url: *const std::ffi::c_char,
        out_response: *mut sys::mln_resource_transform_response,
    ) -> sys::mln_status {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke_status(
            "mln_resource_transform_callback",
            Some((
                &["mln_resource_transform_response_set_url"],
                out_response as usize as u64,
            )),
            sys::MLN_STATUS_NATIVE_ERROR,
            || {
                callback::require(&state.callback)?(
                    unsafe { from_native(kind) }?,
                    unsafe { from_native(url) }?,
                    &mut ResourceTransformResponse::new(out_response)?,
                )
            },
        )
    }
}
impl ToNative<sys::mln_resource_transform> for ResourceTransform {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_resource_transform> {
        let mut raw: sys::mln_resource_transform = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_resource_transform>() as _;
        raw.callback = self
            .callback
            .as_ref()
            .map(|_| Self::callback_trampoline as _);
        if !(raw.callback.is_none()) {
            // SAFETY: the release reclaims exactly this state.
            raw.user_data = unsafe { arena.registration(self.clone(), callback::release::<Self>) };
            raw.release_user_data = Some(callback::release::<Self>);
        }
        Ok(raw)
    }
}

/// A native response borrowed only for one host callback.
#[derive(Debug)]
pub struct ResourceTransformResponse<'a> {
    raw: std::ptr::NonNull<sys::mln_resource_transform_response>,
    lifetime: std::marker::PhantomData<&'a mut sys::mln_resource_transform_response>,
}
impl ResourceTransformResponse<'_> {
    fn new(raw: *mut sys::mln_resource_transform_response) -> Result<Self> {
        let raw = std::ptr::NonNull::new(raw)
            .ok_or_else(|| Error::invalid_argument("null callback response"))?;
        Ok(Self {
            raw,
            lifetime: std::marker::PhantomData,
        })
    }
    /// Copies a replacement URL into C API-managed storage for the current
    /// callback.
    ///
    /// See `mln_resource_transform_response_set_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_url(&mut self, url: &str) -> Result<()> {
        maplibre_core::callback::check(
            "mln_resource_transform_response_set_url",
            self.raw.as_ptr() as usize as u64,
        )?;
        let url_size = convert::count(url.len())?;
        maplibre_core::check(|out_diagnostic| unsafe {
            sys::mln_resource_transform_response_set_url(
                self.raw.as_ptr(),
                url.as_ptr().cast(),
                url_size,
                out_diagnostic,
            )
        })
    }
}

native_enum! {
pub enum ResourceUsage: u32 {
    Online = 0,
    Offline = 1,
} Unknown
}

/// One drained runtime event.
///
/// See `mln_runtime_event` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct RuntimeEvent {
    pub message: String,
    /// One of `mln_runtime_event_type`.
    pub r#type: RuntimeEventType,
    /// One of `mln_runtime_event_source_type`.
    pub source_type: RuntimeEventSourceType,
    /// Source handle selected by source_type: an `mln_runtime` or an `mln_map`.
    /// Every handle type is uint64_t, so this needs no cast.
    pub source: u64,
    /// Map snapshot generation that the event reports, or zero when source_type
    /// is `MLN_RUNTIME_EVENT_SOURCE_RUNTIME`.
    pub generation: u64,
    /// Secondary event detail whose meaning type selects. Depending on type it
    /// carries an `mln_camera_change_mode`, an `mln_status`, a MapLibre Native
    /// error ordinal, or 0. See `mln_runtime_event_type` for the per-type
    /// meaning.
    pub code: i32,
    /// Typed payload selected by payload_type.
    pub payload: RuntimeEventPayload,
}
impl FromNative<sys::mln_runtime_event> for RuntimeEvent {
    unsafe fn from_native(raw: sys::mln_runtime_event) -> Result<Self> {
        Ok(Self {
            message: String::new(),
            r#type: unsafe { from_native(raw.type_) }?,
            source_type: unsafe { from_native(raw.source_type) }?,
            source: raw.source,
            generation: raw.generation,
            code: raw.code,
            payload: match raw.payload_type {
                sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME => {
                    RuntimeEventPayload::RenderFrame(unsafe {
                        from_native(raw.payload.render_frame)
                    }?)
                }
                sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP => {
                    RuntimeEventPayload::RenderMap(unsafe { from_native(raw.payload.render_map) }?)
                }
                sys::MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION => {
                    RuntimeEventPayload::TileAction(unsafe {
                        from_native(raw.payload.tile_action)
                    }?)
                }
                sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS => {
                    RuntimeEventPayload::OfflineRegionStatus(unsafe {
                        from_native(raw.payload.offline_region_status)
                    }?)
                }
                sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR => {
                    RuntimeEventPayload::OfflineRegionResponseError(unsafe {
                        from_native(raw.payload.offline_region_response_error)
                    }?)
                }
                sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT => {
                    RuntimeEventPayload::OfflineRegionTileCountLimit(unsafe {
                        from_native(raw.payload.offline_region_tile_count_limit)
                    }?)
                }
                sys::MLN_RUNTIME_EVENT_PAYLOAD_NONE => RuntimeEventPayload::Empty,
                tag => RuntimeEventPayload::Unknown(tag as u32),
            },
        })
    }
}

native_flags! {
/// Bit values for the map and runtime event subscription masks.
///
/// See `mln_runtime_event_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub struct RuntimeEventMask: u64 {
    /// Selects no event type.
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
    const OFFLINE_REGION_STATUS_CHANGED = 524288;
    const OFFLINE_REGION_RESPONSE_ERROR = 1048576;
    const OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 2097152;
    /// Selects every map-originated event type this version defines.
    const ALL_MAP_EVENTS = 524286;
    /// Selects every runtime-originated event type this version defines.
    const ALL_RUNTIME_EVENTS = 3670016;
    /// Selects every event type this version defines.
    const ALL = 4194302;
}
}

/// Payload for `MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR`.
///
/// See `mln_runtime_event_offline_region_response_error` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventOfflineRegionResponseError {
    pub region_id: i64,
    /// One of `mln_resource_error_reason`.
    pub reason: ResourceErrorReason,
}
impl RuntimeEventOfflineRegionResponseError {
    pub const fn new(region_id: i64, reason: ResourceErrorReason) -> Self {
        Self { region_id, reason }
    }
}
impl FromNative<sys::mln_runtime_event_offline_region_response_error>
    for RuntimeEventOfflineRegionResponseError
{
    unsafe fn from_native(
        raw: sys::mln_runtime_event_offline_region_response_error,
    ) -> Result<Self> {
        Ok(Self {
            region_id: raw.region_id,
            reason: unsafe { from_native(raw.reason) }?,
        })
    }
}

/// Payload for `MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED`.
///
/// See `mln_runtime_event_offline_region_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventOfflineRegionStatus {
    pub region_id: i64,
    pub status: OfflineRegionStatus,
}
impl RuntimeEventOfflineRegionStatus {
    pub const fn new(region_id: i64, status: OfflineRegionStatus) -> Self {
        Self { region_id, status }
    }
}
impl FromNative<sys::mln_runtime_event_offline_region_status> for RuntimeEventOfflineRegionStatus {
    unsafe fn from_native(raw: sys::mln_runtime_event_offline_region_status) -> Result<Self> {
        Ok(Self {
            region_id: raw.region_id,
            status: unsafe { from_native(raw.status) }?,
        })
    }
}

/// Payload for `MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED`.
///
/// See `mln_runtime_event_offline_region_tile_count_limit` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventOfflineRegionTileCountLimit {
    pub region_id: i64,
    pub limit: u64,
}
impl RuntimeEventOfflineRegionTileCountLimit {
    pub const fn new(region_id: i64, limit: u64) -> Self {
        Self { region_id, limit }
    }
}
impl FromNative<sys::mln_runtime_event_offline_region_tile_count_limit>
    for RuntimeEventOfflineRegionTileCountLimit
{
    unsafe fn from_native(
        raw: sys::mln_runtime_event_offline_region_tile_count_limit,
    ) -> Result<Self> {
        Ok(Self {
            region_id: raw.region_id,
            limit: raw.limit,
        })
    }
}

/// Typed event payload carried inline by every event.
///
/// See `mln_runtime_event_payload` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub enum RuntimeEventPayload {
    RenderFrame(RuntimeEventRenderFrame),
    RenderMap(RuntimeEventRenderMap),
    TileAction(RuntimeEventTileAction),
    OfflineRegionStatus(RuntimeEventOfflineRegionStatus),
    OfflineRegionResponseError(RuntimeEventOfflineRegionResponseError),
    OfflineRegionTileCountLimit(RuntimeEventOfflineRegionTileCountLimit),
    Empty,
    Unknown(u32),
}
impl Default for RuntimeEventPayload {
    fn default() -> Self {
        Self::Unknown(0)
    }
}

native_enum! {
/// Payload kinds used by `mln_runtime_event.payload_type`.
///
/// See `mln_runtime_event_payload_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub enum RuntimeEventPayloadType: u32 {
    None = 0,
    RenderFrame = 1,
    RenderMap = 2,
    TileAction = 4,
    OfflineRegionStatus = 5,
    OfflineRegionResponseError = 6,
    OfflineRegionTileCountLimit = 7,
} Unknown
}

/// Payload for `MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED`.
///
/// See `mln_runtime_event_render_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventRenderFrame {
    /// One of `mln_render_mode`.
    pub mode: RenderMode,
    /// Whether MapLibre needs another frame after this one.
    pub needs_repaint: bool,
    /// Whether symbol placement changed during this frame.
    pub placement_changed: bool,
    pub stats: RenderingStats,
}
impl RuntimeEventRenderFrame {
    pub const fn new(
        mode: RenderMode,
        needs_repaint: bool,
        placement_changed: bool,
        stats: RenderingStats,
    ) -> Self {
        Self {
            mode,
            needs_repaint,
            placement_changed,
            stats,
        }
    }
}
impl FromNative<sys::mln_runtime_event_render_frame> for RuntimeEventRenderFrame {
    unsafe fn from_native(raw: sys::mln_runtime_event_render_frame) -> Result<Self> {
        Ok(Self {
            mode: unsafe { from_native(raw.mode) }?,
            needs_repaint: raw.needs_repaint,
            placement_changed: raw.placement_changed,
            stats: unsafe { from_native(raw.stats) }?,
        })
    }
}

/// Payload for `MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED`.
///
/// See `mln_runtime_event_render_map` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventRenderMap {
    /// One of `mln_render_mode`.
    pub mode: RenderMode,
}
impl RuntimeEventRenderMap {
    pub const fn new(mode: RenderMode) -> Self {
        Self { mode }
    }
}
impl FromNative<sys::mln_runtime_event_render_map> for RuntimeEventRenderMap {
    unsafe fn from_native(raw: sys::mln_runtime_event_render_map) -> Result<Self> {
        Ok(Self {
            mode: unsafe { from_native(raw.mode) }?,
        })
    }
}

native_enum! {
/// Source kinds used by `mln_runtime_event.source_type`.
///
/// See `mln_runtime_event_source_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub enum RuntimeEventSourceType: u32 {
    Runtime = 0,
    Map = 1,
} Unknown
}

/// Payload for `MLN_RUNTIME_EVENT_MAP_TILE_ACTION`.
///
/// See `mln_runtime_event_tile_action` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventTileAction {
    /// One of `mln_tile_operation`.
    pub operation: TileOperation,
    pub tile_id: TileId,
}
impl RuntimeEventTileAction {
    pub const fn new(operation: TileOperation, tile_id: TileId) -> Self {
        Self { operation, tile_id }
    }
}
impl FromNative<sys::mln_runtime_event_tile_action> for RuntimeEventTileAction {
    unsafe fn from_native(raw: sys::mln_runtime_event_tile_action) -> Result<Self> {
        Ok(Self {
            operation: unsafe { from_native(raw.operation) }?,
            tile_id: unsafe { from_native(raw.tile_id) }?,
        })
    }
}

native_enum! {
/// Runtime event types carried by `mln_runtime_event.type`.
///
/// See `mln_runtime_event_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub enum RuntimeEventType: u32 {
    MapCameraWillChange = 1,
    MapCameraIsChanging = 2,
    MapCameraDidChange = 3,
    MapStyleLoaded = 4,
    MapLoadingStarted = 5,
    MapLoadingFinished = 6,
    MapLoadingFailed = 7,
    MapIdle = 8,
    MapRenderUpdateAvailable = 9,
    MapRenderError = 10,
    MapStillImageFinished = 11,
    MapStillImageFailed = 12,
    MapRenderFrameStarted = 13,
    MapRenderFrameFinished = 14,
    MapRenderMapStarted = 15,
    MapRenderMapFinished = 16,
    MapStyleImageMissing = 17,
    MapTileAction = 18,
    OfflineRegionStatusChanged = 19,
    OfflineRegionResponseError = 20,
    OfflineRegionTileCountLimitExceeded = 21,
} Unknown
}

/// Options used when creating a runtime.
///
/// See `mln_runtime_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone)]
pub struct RuntimeOptions {
    /// Directory root for asset:// URLs. Copied during runtime creation. Null
    /// or empty selects `/android_asset` on Android and `.` elsewhere.
    pub asset_path: Option<String>,
    /// Cache database path. Copied during runtime creation.
    pub cache_path: Option<String>,
    /// Runtime-scoped event types this runtime queues, as a bitwise OR of
    /// `mln_runtime_event_mask` values.
    pub event_mask: RuntimeEventMask,
    /// Wakes the receiver when the runtime event queue becomes nonempty.
    pub event_wake: Wake,
}
impl Default for RuntimeOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_runtime_options_default() })
    }
}
impl ToNative<sys::mln_runtime_options> for RuntimeOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_runtime_options> {
        let mut raw: sys::mln_runtime_options = unsafe { sys::mln_runtime_options_default() };
        raw.size = std::mem::size_of::<sys::mln_runtime_options>() as _;
        raw.asset_path = to_native(&self.asset_path, arena)?;
        raw.cache_path = to_native(&self.cache_path, arena)?;
        raw.event_mask = to_native(&self.event_mask, arena)?;
        raw.event_wake = to_native(&self.event_wake, arena)?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_runtime_options> for RuntimeOptions {
    unsafe fn from_native(raw: sys::mln_runtime_options) -> Result<Self> {
        Ok(Self {
            asset_path: unsafe { from_native(raw.asset_path) }?,
            cache_path: unsafe { from_native(raw.cache_path) }?,
            event_mask: unsafe { from_native(raw.event_mask) }?,
            event_wake: Default::default(),
        })
    }
}

/// Screen-space box in logical map pixels.
///
/// See `mln_screen_box` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct ScreenBox {
    pub min: ScreenPoint,
    pub max: ScreenPoint,
}
impl ScreenBox {
    pub const fn new(min: ScreenPoint, max: ScreenPoint) -> Self {
        Self { min, max }
    }
}
impl ToNative<sys::mln_screen_box> for ScreenBox {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_screen_box> {
        let mut raw: sys::mln_screen_box = unsafe { std::mem::zeroed() };
        raw.min = to_native(&self.min, arena)?;
        raw.max = to_native(&self.max, arena)?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_screen_box> for ScreenBox {
    unsafe fn from_native(raw: sys::mln_screen_box) -> Result<Self> {
        Ok(Self {
            min: unsafe { from_native(raw.min) }?,
            max: unsafe { from_native(raw.max) }?,
        })
    }
}

/// Screen-space line string in logical map pixels.
///
/// See `mln_screen_line_string` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct ScreenLineString {
    /// Points. Null only when point_count is 0.
    pub points: Vec<ScreenPoint>,
}
impl ToNative<sys::mln_screen_line_string> for ScreenLineString {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_screen_line_string> {
        let mut raw: sys::mln_screen_line_string = unsafe { std::mem::zeroed() };
        raw.points = convert::array(&self.points, arena)?;
        raw.point_count = convert::count(self.points.len())?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_screen_line_string> for ScreenLineString {
    unsafe fn from_native(raw: sys::mln_screen_line_string) -> Result<Self> {
        Ok(Self {
            points: unsafe { convert::copy_array(raw.points, raw.point_count) }?,
        })
    }
}

/// Screen-space point in logical map pixels.
///
/// See `mln_screen_point` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct ScreenPoint {
    pub x: f64,
    pub y: f64,
}
impl ScreenPoint {
    pub const fn new(x: f64, y: f64) -> Self {
        Self { x, y }
    }
}
impl ToNative<sys::mln_screen_point> for ScreenPoint {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_screen_point> {
        let mut raw: sys::mln_screen_point = unsafe { std::mem::zeroed() };
        raw.x = self.x;
        raw.y = self.y;
        Ok(raw)
    }
}
impl FromNative<sys::mln_screen_point> for ScreenPoint {
    unsafe fn from_native(raw: sys::mln_screen_point) -> Result<Self> {
        Ok(Self { x: raw.x, y: raw.y })
    }
}

native_flags! {
/// Optional fields for `mln_source_feature_query_options`.
///
/// See `mln_source_feature_query_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
pub struct SourceFeatureQueryOptionField: u32 {
    const SOURCE_LAYER_IDS = 1;
    const FILTER = 2;
}
}

/// Options for source feature queries.
///
/// See `mln_source_feature_query_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct SourceFeatureQueryOptions {
    /// Optional source-layer IDs. Required by vector sources; ignored by
    /// GeoJSON.
    pub source_layer_ids: Option<Vec<String>>,
    /// Optional UTF-8 MapLibre style-spec filter JSON. When absent, no filter.
    pub filter: Option<Vec<u8>>,
}
impl Default for SourceFeatureQueryOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_source_feature_query_options_default() })
    }
}
impl ToNative<sys::mln_source_feature_query_options> for SourceFeatureQueryOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_source_feature_query_options> {
        let mut raw: sys::mln_source_feature_query_options =
            unsafe { sys::mln_source_feature_query_options_default() };
        raw.size = std::mem::size_of::<sys::mln_source_feature_query_options>() as _;
        raw.fields = 0;
        if let Some(item) = &self.source_layer_ids {
            raw.fields |= sys::MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
            raw.source_layer_ids = convert::array(&*item, arena)?;
        }
        raw.source_layer_id_count = convert::count(
            self.source_layer_ids
                .as_ref()
                .map_or(0, |items| items.len()),
        )?;
        if let Some(item) = &self.filter {
            raw.fields |= sys::MLN_SOURCE_FEATURE_QUERY_OPTION_FILTER;
            raw.filter = to_native(&*item, arena)?;
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_source_feature_query_options> for SourceFeatureQueryOptions {
    unsafe fn from_native(raw: sys::mln_source_feature_query_options) -> Result<Self> {
        Ok(Self {
            source_layer_ids: if raw.fields & sys::MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
                != 0
            {
                Some(unsafe {
                    convert::copy_array(raw.source_layer_ids, raw.source_layer_id_count)
                }?)
            } else {
                None
            },
            filter: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_SOURCE_FEATURE_QUERY_OPTION_FILTER,
                    raw.filter,
                )
            }?,
        })
    }
}

native_enum! {
/// Status values returned by status-returning functions.
///
/// See `mln_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
pub enum Status: i32 {
    Ok = 0,
    /// A pointer, size field, mask, or handle argument was invalid.
    InvalidArgument = -1,
    /// The object is valid but not currently in a state that permits the call.
    InvalidState = -2,
    /// The handle is thread-affine and the call was made from the wrong thread.
    WrongThread = -3,
    /// The entry point or requested behavior is unavailable in this build.
    Unsupported = -4,
    /// A native MapLibre error or C++ exception was converted to status.
    NativeError = -5,
    /// The operation reached its terminal cancelled disposition.
    Cancelled = -6,
    /// A conflicting driver call or lifecycle transition is in flight.
    Busy = -7,
    /// The render target or graphics receiver was irreversibly lost.
    TargetLost = -8,
    /// A nonblocking acquisition or service call has no result yet.
    NotReady = -9,
    /// A call named an ID with no live object behind it.
    NotFound = -10,
} Unknown
}

/// One complete runtime style image, borrowed for a completion callback.
///
/// See `mln_style_image_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleImageInfo {
    /// Image width in pixels.
    pub width: u32,
    /// Image height in pixels.
    pub height: u32,
    /// Premultiplied RGBA8 pixels in tightly packed rows of width \* 4 bytes,
    /// top row first.
    pub pixels: Vec<u8>,
    /// Horizontally stretchable intervals.
    pub stretch_x: Vec<ImageStretch>,
    /// Vertically stretchable intervals.
    pub stretch_y: Vec<ImageStretch>,
    /// Content box, when the image sets one.
    pub content: Option<ImageContent>,
    /// How the image fits text horizontally, when it sets this.
    pub text_fit_width: Option<StyleImageTextFit>,
    /// How the image fits text vertically, when it sets this.
    pub text_fit_height: Option<StyleImageTextFit>,
    /// Sprite pixel ratio.
    pub pixel_ratio: f32,
    /// Whether the image is a signed distance field icon.
    pub sdf: bool,
}
impl FromNative<sys::mln_style_image_info> for StyleImageInfo {
    unsafe fn from_native(raw: sys::mln_style_image_info) -> Result<Self> {
        Ok(Self {
            width: raw.width,
            height: raw.height,
            pixels: unsafe { from_native(raw.pixels) }?,
            stretch_x: unsafe { convert::copy_array(raw.stretch_x, raw.stretch_x_count) }?,
            stretch_y: unsafe { convert::copy_array(raw.stretch_y, raw.stretch_y_count) }?,
            content: unsafe {
                convert::present(raw.fields, sys::MLN_STYLE_IMAGE_INFO_CONTENT, raw.content)
            }?,
            text_fit_width: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH,
                    raw.text_fit_width,
                )
            }?,
            text_fit_height: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT,
                    raw.text_fit_height,
                )
            }?,
            pixel_ratio: raw.pixel_ratio,
            sdf: raw.sdf,
        })
    }
}

native_flags! {
/// Field mask values for `mln_style_image_info`.
///
/// See `mln_style_image_info_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub struct StyleImageInfoField: u32 {
    /// The image declares a content box.
    const CONTENT = 1;
    /// The image declares a horizontal text-fit mode.
    const TEXT_FIT_WIDTH = 2;
    /// The image declares a vertical text-fit mode.
    const TEXT_FIT_HEIGHT = 4;
}
}

native_flags! {
/// Field mask values for `mln_style_image_options`.
///
/// See `mln_style_image_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub struct StyleImageOptionField: u32 {
    const PIXEL_RATIO = 1;
    const SDF = 2;
    const STRETCH_X = 4;
    const STRETCH_Y = 8;
    const CONTENT = 16;
    const TEXT_FIT_WIDTH = 32;
    const TEXT_FIT_HEIGHT = 64;
}
}

/// Options for runtime style images.
///
/// See `mln_style_image_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct StyleImageOptions {
    /// Horizontally stretchable intervals. Borrowed for the call and copied
    /// before return. May be null only when stretch_x_count is 0.
    pub stretch_x: Option<Vec<ImageStretch>>,
    /// Vertically stretchable intervals. Borrowed for the call and copied
    /// before return. May be null only when stretch_y_count is 0.
    pub stretch_y: Option<Vec<ImageStretch>>,
    /// Content box used when icon-text-fit applies.
    pub content: Option<ImageContent>,
    /// One of `mln_style_image_text_fit`. Defaults to STRETCH_OR_SHRINK.
    pub text_fit_width: Option<StyleImageTextFit>,
    /// One of `mln_style_image_text_fit`. Defaults to STRETCH_OR_SHRINK.
    pub text_fit_height: Option<StyleImageTextFit>,
    /// Sprite pixel ratio. Defaults to 1.
    pub pixel_ratio: Option<f32>,
    /// Whether the image is a signed distance field icon. Defaults to false.
    pub sdf: Option<bool>,
}
impl Default for StyleImageOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_style_image_options_default() })
    }
}
impl ToNative<sys::mln_style_image_options> for StyleImageOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_style_image_options> {
        let mut raw: sys::mln_style_image_options =
            unsafe { sys::mln_style_image_options_default() };
        raw.size = std::mem::size_of::<sys::mln_style_image_options>() as _;
        raw.fields = 0;
        if let Some(item) = &self.stretch_x {
            raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_STRETCH_X;
            raw.stretch_x = convert::array(&*item, arena)?;
        }
        raw.stretch_x_count =
            convert::count(self.stretch_x.as_ref().map_or(0, |items| items.len()))?;
        if let Some(item) = &self.stretch_y {
            raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
            raw.stretch_y = convert::array(&*item, arena)?;
        }
        raw.stretch_y_count =
            convert::count(self.stretch_y.as_ref().map_or(0, |items| items.len()))?;
        if let Some(item) = &self.content {
            raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_CONTENT;
            raw.content = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.text_fit_width {
            raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH;
            raw.text_fit_width = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.text_fit_height {
            raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
            raw.text_fit_height = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.pixel_ratio {
            raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
            raw.pixel_ratio = *item;
        }
        if let Some(item) = &self.sdf {
            raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_SDF;
            raw.sdf = *item;
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_style_image_options> for StyleImageOptions {
    unsafe fn from_native(raw: sys::mln_style_image_options) -> Result<Self> {
        Ok(Self {
            stretch_x: if raw.fields & sys::MLN_STYLE_IMAGE_OPTION_STRETCH_X != 0 {
                Some(unsafe { convert::copy_array(raw.stretch_x, raw.stretch_x_count) }?)
            } else {
                None
            },
            stretch_y: if raw.fields & sys::MLN_STYLE_IMAGE_OPTION_STRETCH_Y != 0 {
                Some(unsafe { convert::copy_array(raw.stretch_y, raw.stretch_y_count) }?)
            } else {
                None
            },
            content: unsafe {
                convert::present(raw.fields, sys::MLN_STYLE_IMAGE_OPTION_CONTENT, raw.content)
            }?,
            text_fit_width: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH,
                    raw.text_fit_width,
                )
            }?,
            text_fit_height: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT,
                    raw.text_fit_height,
                )
            }?,
            pixel_ratio: (raw.fields & sys::MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO != 0)
                .then_some(raw.pixel_ratio),
            sdf: (raw.fields & sys::MLN_STYLE_IMAGE_OPTION_SDF != 0).then_some(raw.sdf),
        })
    }
}

native_enum! {
/// How a stretchable image fits text along one axis.
///
/// See `mln_style_image_text_fit` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub enum StyleImageTextFit: u32 {
    StretchOrShrink = 0,
    StretchOnly = 1,
    Proportional = 2,
} Unknown
}

/// One style layer, borrowed for a completion callback.
///
/// See `mln_style_layer_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleLayerInfo {
    /// Layer ID.
    pub id: String,
    /// The style-spec layer type string.
    pub r#type: String,
    /// Source ID. Empty for a layer type that takes no source.
    pub source_id: Option<String>,
    /// Source-layer ID. Empty when the layer sets none.
    pub source_layer: Option<String>,
    /// Lowest zoom at which the layer draws; -INFINITY with no lower bound.
    pub min_zoom: f64,
    /// Highest zoom at which the layer draws; INFINITY with no upper bound.
    pub max_zoom: f64,
    /// One of `mln_style_layer_visibility`.
    pub visibility: StyleLayerVisibility,
}
impl FromNative<sys::mln_style_layer_info> for StyleLayerInfo {
    unsafe fn from_native(raw: sys::mln_style_layer_info) -> Result<Self> {
        Ok(Self {
            id: unsafe { from_native(raw.id) }?,
            r#type: unsafe { from_native(raw.type_) }?,
            source_id: unsafe { convert::nonempty(raw.source_id) }?,
            source_layer: unsafe { convert::nonempty(raw.source_layer) }?,
            min_zoom: raw.min_zoom,
            max_zoom: raw.max_zoom,
            visibility: unsafe { from_native(raw.visibility) }?,
        })
    }
}

native_enum! {
/// Layer visibility values used by the visibility setter and layer info.
///
/// See `mln_style_layer_visibility` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub enum StyleLayerVisibility: u32 {
    Visible = 0,
    None = 1,
} Unknown
}

native_enum! {
/// DEM raster encoding values used by `mln_style_tile_source_options`.
///
/// See `mln_style_raster_dem_encoding` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub enum StyleRasterDemEncoding: u32 {
    Mapbox = 0,
    Terrarium = 1,
} Unknown
}

/// Complete metadata of one style source, borrowed for a completion callback.
///
/// See `mln_style_source_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleSourceInfo {
    /// Source ID.
    pub id: String,
    /// One of `mln_style_source_type`.
    pub r#type: StyleSourceType,
    /// Whether the source is marked volatile.
    pub is_volatile: bool,
    /// Attribution string, when the source sets one. It may be empty.
    pub attribution: Option<String>,
    /// URL that the source loads from, when it has one.
    pub url: Option<String>,
    /// Inline TileJSON metadata, when the source was defined with it.
    pub tilejson: Option<StyleSourceTileInfo>,
    /// Geographic bounds, when inline TileJSON sets them.
    pub bounds: Option<LatLngBounds>,
    /// Tile size in pixels, for a tile source.
    pub tile_size: Option<u32>,
    /// Vector tile encoding, for a vector source.
    pub vector_encoding: Option<StyleVectorTileEncoding>,
    /// DEM raster encoding, when inline TileJSON sets one.
    pub raster_encoding: Option<StyleRasterDemEncoding>,
}
impl FromNative<sys::mln_style_source_info> for StyleSourceInfo {
    unsafe fn from_native(raw: sys::mln_style_source_info) -> Result<Self> {
        Ok(Self {
            id: unsafe { from_native(raw.id) }?,
            r#type: unsafe { from_native(raw.type_) }?,
            is_volatile: raw.is_volatile,
            attribution: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_STYLE_SOURCE_INFO_ATTRIBUTION,
                    raw.attribution,
                )
            }?,
            url: unsafe { convert::present(raw.fields, sys::MLN_STYLE_SOURCE_INFO_URL, raw.url) }?,
            tilejson: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_STYLE_SOURCE_INFO_TILEJSON,
                    raw.tilejson,
                )
            }?,
            bounds: unsafe {
                convert::present(raw.fields, sys::MLN_STYLE_SOURCE_INFO_BOUNDS, raw.bounds)
            }?,
            tile_size: (raw.fields & sys::MLN_STYLE_SOURCE_INFO_TILE_SIZE != 0)
                .then_some(raw.tile_size),
            vector_encoding: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING,
                    raw.vector_encoding,
                )
            }?,
            raster_encoding: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_STYLE_SOURCE_INFO_RASTER_ENCODING,
                    raw.raster_encoding,
                )
            }?,
        })
    }
}

native_flags! {
/// Fields available in `mln_style_source_info`.
///
/// See `mln_style_source_info_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub struct StyleSourceInfoField: u32 {
    /// The source retains a URL.
    const URL = 1;
    /// The tile source was defined with an inline TileJSON description.
    const TILEJSON = 2;
    /// The inline TileJSON description contains geographic bounds.
    const BOUNDS = 4;
    /// The source exposes a tile size.
    const TILE_SIZE = 8;
    /// The source exposes a vector tile encoding.
    const VECTOR_ENCODING = 16;
    /// The source exposes a DEM raster encoding.
    const RASTER_ENCODING = 32;
    /// The source carries an attribution string, which may be empty.
    const ATTRIBUTION = 64;
}
}

/// Inline TileJSON metadata of a tile source.
///
/// See `mln_style_source_tile_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleSourceTileInfo {
    /// Tile URL templates in TileJSON order.
    pub tile_urls: Vec<String>,
    /// Lowest zoom level the TileJSON provides tiles for.
    pub min_zoom: f64,
    /// Highest zoom level the TileJSON provides tiles for.
    pub max_zoom: f64,
    /// One of `mln_style_tile_scheme`.
    pub scheme: StyleTileScheme,
}
impl FromNative<sys::mln_style_source_tile_info> for StyleSourceTileInfo {
    unsafe fn from_native(raw: sys::mln_style_source_tile_info) -> Result<Self> {
        Ok(Self {
            tile_urls: unsafe { convert::copy_array(raw.tile_urls, raw.tile_url_count) }?,
            min_zoom: raw.min_zoom,
            max_zoom: raw.max_zoom,
            scheme: unsafe { from_native(raw.scheme) }?,
        })
    }
}

native_enum! {
/// Style source type values returned by source metadata queries.
///
/// See `mln_style_source_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub enum StyleSourceType: u32 {
    Unknown = 0,
    Vector = 1,
    Raster = 2,
    RasterDem = 3,
    Geojson = 4,
    Image = 5,
    Video = 6,
    Annotations = 7,
    CustomVector = 8,
    CustomMvtVector = 9,
} Unrecognized
}

native_enum! {
/// Tile URL coordinate scheme values used by `mln_style_tile_source_options`.
///
/// See `mln_style_tile_scheme` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub enum StyleTileScheme: u32 {
    Xyz = 0,
    Tms = 1,
} Unknown
}

native_flags! {
/// Field mask values for `mln_style_tile_source_options`.
///
/// See `mln_style_tile_source_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub struct StyleTileSourceOptionField: u32 {
    const MIN_ZOOM = 1;
    const MAX_ZOOM = 2;
    const ATTRIBUTION = 4;
    const SCHEME = 8;
    const BOUNDS = 16;
    const TILE_SIZE = 32;
    const VECTOR_ENCODING = 64;
    const RASTER_ENCODING = 128;
}
}

/// Options for vector and raster tile sources.
///
/// See `mln_style_tile_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct StyleTileSourceOptions {
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
    pub attribution: Option<String>,
    /// One of `mln_style_tile_scheme`. Defaults to `MLN_STYLE_TILE_SCHEME_XYZ`.
    pub scheme: Option<StyleTileScheme>,
    pub bounds: Option<LatLngBounds>,
    /// Raster tile size in pixels. Defaults to 512.
    pub tile_size: Option<u32>,
    /// One of `mln_style_vector_tile_encoding`. Defaults to MVT.
    pub vector_encoding: Option<StyleVectorTileEncoding>,
    /// One of `mln_style_raster_dem_encoding`. Defaults to Mapbox.
    pub raster_encoding: Option<StyleRasterDemEncoding>,
}
impl Default for StyleTileSourceOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_style_tile_source_options_default() })
    }
}
impl ToNative<sys::mln_style_tile_source_options> for StyleTileSourceOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_style_tile_source_options> {
        let mut raw: sys::mln_style_tile_source_options =
            unsafe { sys::mln_style_tile_source_options_default() };
        raw.size = std::mem::size_of::<sys::mln_style_tile_source_options>() as _;
        raw.fields = 0;
        if let Some(item) = &self.min_zoom {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM;
            raw.min_zoom = *item;
        }
        if let Some(item) = &self.max_zoom {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM;
            raw.max_zoom = *item;
        }
        if let Some(item) = &self.attribution {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
            raw.attribution = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.scheme {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_SCHEME;
            raw.scheme = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.bounds {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS;
            raw.bounds = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.tile_size {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE;
            raw.tile_size = *item;
        }
        if let Some(item) = &self.vector_encoding {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
            raw.vector_encoding = to_native(&*item, arena)?;
        }
        if let Some(item) = &self.raster_encoding {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
            raw.raster_encoding = to_native(&*item, arena)?;
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_style_tile_source_options> for StyleTileSourceOptions {
    unsafe fn from_native(raw: sys::mln_style_tile_source_options) -> Result<Self> {
        Ok(Self {
            min_zoom: (raw.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM != 0)
                .then_some(raw.min_zoom),
            max_zoom: (raw.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM != 0)
                .then_some(raw.max_zoom),
            attribution: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION,
                    raw.attribution,
                )
            }?,
            scheme: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_STYLE_TILE_SOURCE_OPTION_SCHEME,
                    raw.scheme,
                )
            }?,
            bounds: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS,
                    raw.bounds,
                )
            }?,
            tile_size: (raw.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE != 0)
                .then_some(raw.tile_size),
            vector_encoding: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING,
                    raw.vector_encoding,
                )
            }?,
            raster_encoding: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING,
                    raw.raster_encoding,
                )
            }?,
        })
    }
}

native_flags! {
/// Field mask values for `mln_style_transition_options`.
///
/// See `mln_style_transition_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub struct StyleTransitionOptionField: u32 {
    const DURATION = 1;
    const DELAY = 2;
    const ENABLE_PLACEMENT_TRANSITIONS = 4;
}
}

/// Global style transition options.
///
/// See `mln_style_transition_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct StyleTransitionOptions {
    /// Transition duration in milliseconds. Must be finite and non-negative.
    /// Values that would overflow MapLibre Native's internal duration are
    /// invalid.
    pub duration_ms: Option<f64>,
    /// Transition delay in milliseconds. Must be finite and non-negative.
    /// Values that would overflow MapLibre Native's internal duration are
    /// invalid.
    pub delay_ms: Option<f64>,
    /// Whether symbol placement changes cross-fade.
    pub enable_placement_transitions: Option<bool>,
}
impl Default for StyleTransitionOptions {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_style_transition_options_default() })
    }
}
impl ToNative<sys::mln_style_transition_options> for StyleTransitionOptions {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_style_transition_options> {
        let mut raw: sys::mln_style_transition_options =
            unsafe { sys::mln_style_transition_options_default() };
        raw.size = std::mem::size_of::<sys::mln_style_transition_options>() as _;
        raw.fields = 0;
        if let Some(item) = &self.duration_ms {
            raw.fields |= sys::MLN_STYLE_TRANSITION_OPTION_DURATION;
            raw.duration_ms = *item;
        }
        if let Some(item) = &self.delay_ms {
            raw.fields |= sys::MLN_STYLE_TRANSITION_OPTION_DELAY;
            raw.delay_ms = *item;
        }
        if let Some(item) = &self.enable_placement_transitions {
            raw.fields |= sys::MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
            raw.enable_placement_transitions = *item;
        }
        Ok(raw)
    }
}
impl FromNative<sys::mln_style_transition_options> for StyleTransitionOptions {
    unsafe fn from_native(raw: sys::mln_style_transition_options) -> Result<Self> {
        Ok(Self {
            duration_ms: (raw.fields & sys::MLN_STYLE_TRANSITION_OPTION_DURATION != 0)
                .then_some(raw.duration_ms),
            delay_ms: (raw.fields & sys::MLN_STYLE_TRANSITION_OPTION_DELAY != 0)
                .then_some(raw.delay_ms),
            enable_placement_transitions: (raw.fields
                & sys::MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS
                != 0)
                .then_some(raw.enable_placement_transitions),
        })
    }
}

native_enum! {
/// Vector tile encoding values used by `mln_style_tile_source_options`.
///
/// See `mln_style_vector_tile_encoding` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub enum StyleVectorTileEncoding: u32 {
    Mvt = 0,
    Mlt = 1,
} Unknown
}

/// CPU image readback metadata for a texture target frame.
///
/// See `mln_texture_image_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct TextureImageInfo {
    /// Physical image width in device pixels.
    pub width: u32,
    /// Physical image height in device pixels.
    pub height: u32,
    /// Bytes per image row.
    pub stride: u32,
    /// Required output buffer byte length.
    pub byte_length: usize,
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
}
impl FromNative<sys::mln_texture_image_info> for TextureImageInfo {
    unsafe fn from_native(raw: sys::mln_texture_image_info) -> Result<Self> {
        Ok(Self {
            width: raw.width,
            height: raw.height,
            stride: raw.stride,
            byte_length: raw.byte_length,
        })
    }
}

/// Texture readback borrowed for a completion callback.
///
/// See `mln_texture_readback_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct TextureReadbackResult {
    /// Borrowed pixel bytes, valid only during the callback.
    pub data: Vec<u8>,
    pub info: TextureImageInfo,
}
impl FromNative<sys::mln_texture_readback_result> for TextureReadbackResult {
    unsafe fn from_native(raw: sys::mln_texture_readback_result) -> Result<Self> {
        Ok(Self {
            data: unsafe { from_native(raw.data) }?,
            info: unsafe { from_native(raw.info) }?,
        })
    }
}

/// Overscaled tile identity reported in tile observer events.
///
/// See `mln_tile_id` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
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
}
impl FromNative<sys::mln_tile_id> for TileId {
    unsafe fn from_native(raw: sys::mln_tile_id) -> Result<Self> {
        Ok(Self {
            overscaled_z: raw.overscaled_z,
            wrap: raw.wrap,
            canonical_z: raw.canonical_z,
            canonical_x: raw.canonical_x,
            canonical_y: raw.canonical_y,
        })
    }
}

native_enum! {
/// Tile LOD algorithms used by `mln_map_tile_options`.
///
/// See `mln_tile_lod_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub enum TileLodMode: u32 {
    Default = 0,
    Distance = 1,
} Unknown
}

native_enum! {
/// Tile operations reported by tile observer events.
///
/// See `mln_tile_operation` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub enum TileOperation: u32 {
    RequestedFromCache = 0,
    RequestedFromNetwork = 1,
    LoadFromNetwork = 2,
    LoadFromCache = 3,
    StartParse = 4,
    EndParse = 5,
    Error = 6,
    Cancelled = 7,
    Null = 8,
} Unknown
}

/// Cubic easing curve for animated camera transitions.
///
/// See `mln_unit_bezier` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
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
}
impl ToNative<sys::mln_unit_bezier> for UnitBezier {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_unit_bezier> {
        let mut raw: sys::mln_unit_bezier = unsafe { std::mem::zeroed() };
        raw.x1 = self.x1;
        raw.y1 = self.y1;
        raw.x2 = self.x2;
        raw.y2 = self.y2;
        Ok(raw)
    }
}
impl FromNative<sys::mln_unit_bezier> for UnitBezier {
    unsafe fn from_native(raw: sys::mln_unit_bezier) -> Result<Self> {
        Ok(Self {
            x1: raw.x1,
            y1: raw.y1,
            x2: raw.x2,
            y2: raw.y2,
        })
    }
}

/// Three-component vector used by free camera options.
///
/// See `mln_vec3` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
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
}
impl ToNative<sys::mln_vec3> for Vec3 {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_vec3> {
        let mut raw: sys::mln_vec3 = unsafe { std::mem::zeroed() };
        raw.x = self.x;
        raw.y = self.y;
        raw.z = self.z;
        Ok(raw)
    }
}
impl FromNative<sys::mln_vec3> for Vec3 {
    unsafe fn from_native(raw: sys::mln_vec3) -> Result<Self> {
        Ok(Self {
            x: raw.x,
            y: raw.y,
            z: raw.z,
        })
    }
}

native_enum! {
/// Viewport orientation modes used by `mln_map_viewport_options`.
///
/// See `mln_viewport_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub enum ViewportMode: u32 {
    Default = 0,
    FlippedY = 1,
} Unknown
}

/// One caller-owned Vulkan image of a borrowed texture ring.
///
/// See `mln_vulkan_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct VulkanBorrowedTexture {
    /// Borrowed VkImage. Required.
    pub image: u64,
    /// Borrowed VkImageView for image. Required. The view must be a 2D color
    /// view that matches image and the descriptor's format.
    pub image_view: u64,
}
impl VulkanBorrowedTexture {
    pub const fn new(image: u64, image_view: u64) -> Self {
        Self { image, image_view }
    }
}
impl ToNative<sys::mln_vulkan_borrowed_texture> for VulkanBorrowedTexture {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_vulkan_borrowed_texture> {
        let mut raw: sys::mln_vulkan_borrowed_texture = unsafe { std::mem::zeroed() };
        raw.image = self.image;
        raw.image_view = self.image_view;
        Ok(raw)
    }
}
impl FromNative<sys::mln_vulkan_borrowed_texture> for VulkanBorrowedTexture {
    unsafe fn from_native(raw: sys::mln_vulkan_borrowed_texture) -> Result<Self> {
        Ok(Self {
            image: raw.image,
            image_view: raw.image_view,
        })
    }
}

/// Vulkan attachment options for a borrowed texture target.
///
/// See `mln_vulkan_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct VulkanBorrowedTextureDescriptor {
    /// Logical texture extent. The map viewport uses width and height and the
    /// renderer uses scale_factor; the physical size is stated separately
    /// below. A scale_factor that differs from the map's is accepted and logged
    /// as a warning.
    pub extent: LogicalExtent,
    /// Physical image width in device pixels. Must be positive. Defaults to
    /// 256.
    pub physical_width: u32,
    /// Physical image height in device pixels. Must be positive. Defaults to
    /// 256.
    pub physical_height: u32,
    /// Borrowed Vulkan context. All handles are required.
    pub context: VulkanContextDescriptor,
    /// The ring's images, one per slot, in slot order. Required.
    pub textures: Vec<VulkanBorrowedTexture>,
    /// Backend-native VkFormat value of every image. VK_FORMAT_UNDEFINED is
    /// invalid.
    pub format: u32,
    /// Backend-native VkImageLayout value expected at render-pass begin.
    pub initial_layout: u32,
    /// Backend-native VkImageLayout value left after rendering succeeds.
    /// Defaults to 5, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
    pub final_layout: u32,
}
impl Default for VulkanBorrowedTextureDescriptor {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_vulkan_borrowed_texture_descriptor_default() })
    }
}
impl ToNative<sys::mln_vulkan_borrowed_texture_descriptor> for VulkanBorrowedTextureDescriptor {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_vulkan_borrowed_texture_descriptor> {
        let mut raw: sys::mln_vulkan_borrowed_texture_descriptor =
            unsafe { sys::mln_vulkan_borrowed_texture_descriptor_default() };
        raw.size = std::mem::size_of::<sys::mln_vulkan_borrowed_texture_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.physical_width = self.physical_width;
        raw.physical_height = self.physical_height;
        raw.context = to_native(&self.context, arena)?;
        raw.textures = convert::array(&self.textures, arena)?;
        raw.texture_count = convert::count(self.textures.len())?;
        raw.format = self.format;
        raw.initial_layout = self.initial_layout;
        raw.final_layout = self.final_layout;
        Ok(raw)
    }
}
impl FromNative<sys::mln_vulkan_borrowed_texture_descriptor> for VulkanBorrowedTextureDescriptor {
    unsafe fn from_native(raw: sys::mln_vulkan_borrowed_texture_descriptor) -> Result<Self> {
        Ok(Self {
            extent: unsafe { from_native(raw.extent) }?,
            physical_width: raw.physical_width,
            physical_height: raw.physical_height,
            context: unsafe { from_native(raw.context) }?,
            textures: unsafe { convert::copy_array(raw.textures, raw.texture_count) }?,
            format: raw.format,
            initial_layout: raw.initial_layout,
            final_layout: raw.final_layout,
        })
    }
}

/// Vulkan backend context fields shared by Vulkan render targets.
///
/// See `mln_vulkan_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct VulkanContextDescriptor {
    /// Borrowed VkInstance. Required.
    pub instance: *mut std::ffi::c_void,
    /// Borrowed VkPhysicalDevice. Required.
    pub physical_device: *mut std::ffi::c_void,
    /// Borrowed VkDevice. Required.
    pub device: *mut std::ffi::c_void,
    /// Borrowed graphics VkQueue. Required. The session's driver submits to it
    /// from its own thread, so a host that uses the same queue passes
    /// `mln_render_session_attach_options.queue_lock` at attach.
    pub graphics_queue: *mut std::ffi::c_void,
    /// Queue family index for graphics_queue. Must support graphics commands.
    pub graphics_queue_family_index: u32,
    /// PFN_vkGetInstanceProcAddr for the loader that created the Vulkan
    /// handles.
    pub get_instance_proc_addr: *mut std::ffi::c_void,
    /// PFN_vkGetDeviceProcAddr for the loader that created the Vulkan device.
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
}
impl ToNative<sys::mln_vulkan_context_descriptor> for VulkanContextDescriptor {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_vulkan_context_descriptor> {
        let mut raw: sys::mln_vulkan_context_descriptor = unsafe { std::mem::zeroed() };
        raw.instance = self.instance;
        raw.physical_device = self.physical_device;
        raw.device = self.device;
        raw.graphics_queue = self.graphics_queue;
        raw.graphics_queue_family_index = self.graphics_queue_family_index;
        raw.get_instance_proc_addr = self.get_instance_proc_addr;
        raw.get_device_proc_addr = self.get_device_proc_addr;
        Ok(raw)
    }
}
impl FromNative<sys::mln_vulkan_context_descriptor> for VulkanContextDescriptor {
    unsafe fn from_native(raw: sys::mln_vulkan_context_descriptor) -> Result<Self> {
        Ok(Self {
            instance: raw.instance,
            physical_device: raw.physical_device,
            device: raw.device,
            graphics_queue: raw.graphics_queue,
            graphics_queue_family_index: raw.graphics_queue_family_index,
            get_instance_proc_addr: raw.get_instance_proc_addr,
            get_device_proc_addr: raw.get_device_proc_addr,
        })
    }
}

/// Vulkan attachment options for an owned texture target.
///
/// See `mln_vulkan_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanOwnedTextureDescriptor {
    /// Logical texture extent. A scale_factor that differs from the map's is
    /// accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Borrowed Vulkan context. All handles are required.
    pub context: VulkanContextDescriptor,
}
impl Default for VulkanOwnedTextureDescriptor {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_vulkan_owned_texture_descriptor_default() })
    }
}
impl VulkanOwnedTextureDescriptor {
    pub const fn new(extent: LogicalExtent, context: VulkanContextDescriptor) -> Self {
        Self { extent, context }
    }
}
impl ToNative<sys::mln_vulkan_owned_texture_descriptor> for VulkanOwnedTextureDescriptor {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_vulkan_owned_texture_descriptor> {
        let mut raw: sys::mln_vulkan_owned_texture_descriptor =
            unsafe { sys::mln_vulkan_owned_texture_descriptor_default() };
        raw.size = std::mem::size_of::<sys::mln_vulkan_owned_texture_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.context = to_native(&self.context, arena)?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_vulkan_owned_texture_descriptor> for VulkanOwnedTextureDescriptor {
    unsafe fn from_native(raw: sys::mln_vulkan_owned_texture_descriptor) -> Result<Self> {
        Ok(Self {
            extent: unsafe { from_native(raw.extent) }?,
            context: unsafe { from_native(raw.context) }?,
        })
    }
}

/// Vulkan attachment options for a native surface.
///
/// See `mln_vulkan_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanSurfaceDescriptor {
    /// Logical surface extent. A scale_factor that differs from the map's is
    /// accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Borrowed Vulkan context. All handles are required. The device must
    /// support VK_KHR_swapchain, and the queue family must support graphics and
    /// presentation to this descriptor's surface.
    pub context: VulkanContextDescriptor,
    /// Borrowed VkSurfaceKHR bit pattern. Required.
    pub surface: u64,
}
impl Default for VulkanSurfaceDescriptor {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_vulkan_surface_descriptor_default() })
    }
}
impl VulkanSurfaceDescriptor {
    pub const fn new(
        extent: LogicalExtent,
        context: VulkanContextDescriptor,
        surface: u64,
    ) -> Self {
        Self {
            extent,
            context,
            surface,
        }
    }
}
impl ToNative<sys::mln_vulkan_surface_descriptor> for VulkanSurfaceDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_vulkan_surface_descriptor> {
        let mut raw: sys::mln_vulkan_surface_descriptor =
            unsafe { sys::mln_vulkan_surface_descriptor_default() };
        raw.size = std::mem::size_of::<sys::mln_vulkan_surface_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.context = to_native(&self.context, arena)?;
        raw.surface = self.surface;
        Ok(raw)
    }
}
impl FromNative<sys::mln_vulkan_surface_descriptor> for VulkanSurfaceDescriptor {
    unsafe fn from_native(raw: sys::mln_vulkan_surface_descriptor) -> Result<Self> {
        Ok(Self {
            extent: unsafe { from_native(raw.extent) }?,
            context: unsafe { from_native(raw.context) }?,
            surface: raw.surface,
        })
    }
}

/// Vulkan frame acquired from a texture ring.
///
/// See `mln_vulkan_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct VulkanTextureFrame {
    /// Session generation that produced this frame.
    pub generation: u64,
    /// Physical Vulkan image width in device pixels.
    pub width: u32,
    /// Physical Vulkan image height in device pixels.
    pub height: u32,
    /// UI-to-device pixel scale used for this frame.
    pub scale_factor: f64,
    /// Opaque frame identity used to reject stale releases.
    pub frame_id: u64,
    /// Ring slot that holds this frame. For a borrowed target, the index of its
    /// image in the descriptor's textures array.
    pub slot: u32,
    /// Borrowed VkImage bit pattern. Valid until frame release.
    pub image: u64,
    /// Borrowed VkImageView bit pattern. Valid until frame release.
    pub image_view: u64,
    /// Borrowed VkDevice. Valid until frame release.
    pub device: *mut std::ffi::c_void,
    /// Backend-native VkFormat value.
    pub format: u32,
    /// Backend-native VkImageLayout value that the image is in:
    /// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for a session-owned ring, and
    /// the descriptor's final_layout for a borrowed one.
    pub layout: u32,
}
impl VulkanTextureFrame {
    pub const fn new(
        generation: u64,
        width: u32,
        height: u32,
        scale_factor: f64,
        frame_id: u64,
        slot: u32,
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
            slot,
            image,
            image_view,
            device,
            format,
            layout,
        }
    }
}
impl FromNative<sys::mln_vulkan_texture_frame> for VulkanTextureFrame {
    unsafe fn from_native(raw: sys::mln_vulkan_texture_frame) -> Result<Self> {
        Ok(Self {
            generation: raw.generation,
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: raw.frame_id,
            slot: raw.slot,
            image: raw.image,
            image_view: raw.image_view,
            device: raw.device,
            format: raw.format,
            layout: raw.layout,
        })
    }
}

/// Receiver wake callback copied by a successful owning call.
///
/// See `mln_wake` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
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
    unsafe extern "C" fn callback_trampoline(user_data: *mut std::ffi::c_void) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_wake_callback", None, (), || {
            callback::require(&state.callback)?();
            Ok(())
        })
    }
}
impl ToNative<sys::mln_wake> for Wake {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_wake> {
        let mut raw: sys::mln_wake = unsafe { std::mem::zeroed() };
        raw.callback = self
            .callback
            .as_ref()
            .map(|_| Self::callback_trampoline as _);
        if !(raw.callback.is_none()) {
            // SAFETY: the release reclaims exactly this state.
            raw.user_data = unsafe { arena.registration(self.clone(), callback::release::<Self>) };
            raw.release_user_data = Some(callback::release::<Self>);
        }
        Ok(raw)
    }
}

/// WebGL context fields shared by OpenGL render targets in the browser.
///
/// See `mln_webgl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct WebglContextDescriptor {
    /// One `mln_webgl_context_kind` value.
    pub kind: WebglContextKind,
    /// Borrowed EMSCRIPTEN_WEBGL_CONTEXT_HANDLE for EXISTING. Must be positive.
    pub context: i32,
    /// Copied UTF-8 Emscripten target selector for TRANSFERRED_CANVAS. The HTML
    /// canvas must still be transferable when attachment starts.
    pub canvas_selector: String,
}
impl ToNative<sys::mln_webgl_context_descriptor> for WebglContextDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_webgl_context_descriptor> {
        let mut raw: sys::mln_webgl_context_descriptor = unsafe { std::mem::zeroed() };
        raw.kind = to_native(&self.kind, arena)?;
        raw.context = self.context;
        raw.canvas_selector = to_native(&self.canvas_selector, arena)?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_webgl_context_descriptor> for WebglContextDescriptor {
    unsafe fn from_native(raw: sys::mln_webgl_context_descriptor) -> Result<Self> {
        Ok(Self {
            kind: unsafe { from_native(raw.kind) }?,
            context: raw.context,
            canvas_selector: unsafe { from_native(raw.canvas_selector) }?,
        })
    }
}

native_enum! {
/// WebGL context placement.
///
/// See `mln_webgl_context_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub enum WebglContextKind: u32 {
    /// Use a host-created context on its current browser agent.
    Existing = 0,
    /// Create a WebGL 2 context on a native worker whose pthread creation
    /// claims canvas_selector through Emscripten's transferred-canvases
    /// attribute.
    TransferredCanvas = 1,
} Unknown
}

/// One caller-owned WebGPU texture of a borrowed texture ring.
///
/// See `mln_webgpu_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct WebgpuBorrowedTexture {
    /// Borrowed WGPUTexture. Required.
    pub texture: *mut std::ffi::c_void,
    /// Borrowed WGPUTextureView for texture. Required. The view must be a 2D
    /// color view compatible with texture and the descriptor's format.
    pub texture_view: *mut std::ffi::c_void,
}
impl WebgpuBorrowedTexture {
    pub const fn new(texture: *mut std::ffi::c_void, texture_view: *mut std::ffi::c_void) -> Self {
        Self {
            texture,
            texture_view,
        }
    }
}
impl ToNative<sys::mln_webgpu_borrowed_texture> for WebgpuBorrowedTexture {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_webgpu_borrowed_texture> {
        let mut raw: sys::mln_webgpu_borrowed_texture = unsafe { std::mem::zeroed() };
        raw.texture = self.texture;
        raw.texture_view = self.texture_view;
        Ok(raw)
    }
}
impl FromNative<sys::mln_webgpu_borrowed_texture> for WebgpuBorrowedTexture {
    unsafe fn from_native(raw: sys::mln_webgpu_borrowed_texture) -> Result<Self> {
        Ok(Self {
            texture: raw.texture,
            texture_view: raw.texture_view,
        })
    }
}

/// WebGPU attachment options for a borrowed texture target.
///
/// See `mln_webgpu_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct WebgpuBorrowedTextureDescriptor {
    /// Logical texture extent. The map viewport uses width and height and the
    /// renderer uses scale_factor; the physical size is stated separately
    /// below. A scale_factor that differs from the map's is accepted and logged
    /// as a warning.
    pub extent: LogicalExtent,
    /// Physical texture width in device pixels. Defaults to 256.
    pub physical_width: u32,
    /// Physical texture height in device pixels. Defaults to 256.
    pub physical_height: u32,
    /// Borrowed WebGPU context. device is required. Rendering is submitted
    /// through context.queue or that device's default queue.
    pub context: WebgpuContextDescriptor,
    /// The ring's textures, one per slot, in slot order. Required.
    pub textures: Vec<WebgpuBorrowedTexture>,
    /// Backend-native WGPUTextureFormat value of every texture. Undefined is
    /// invalid.
    pub format: u32,
}
impl Default for WebgpuBorrowedTextureDescriptor {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_webgpu_borrowed_texture_descriptor_default() })
    }
}
impl ToNative<sys::mln_webgpu_borrowed_texture_descriptor> for WebgpuBorrowedTextureDescriptor {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_webgpu_borrowed_texture_descriptor> {
        let mut raw: sys::mln_webgpu_borrowed_texture_descriptor =
            unsafe { sys::mln_webgpu_borrowed_texture_descriptor_default() };
        raw.size = std::mem::size_of::<sys::mln_webgpu_borrowed_texture_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.physical_width = self.physical_width;
        raw.physical_height = self.physical_height;
        raw.context = to_native(&self.context, arena)?;
        raw.textures = convert::array(&self.textures, arena)?;
        raw.texture_count = convert::count(self.textures.len())?;
        raw.format = self.format;
        Ok(raw)
    }
}
impl FromNative<sys::mln_webgpu_borrowed_texture_descriptor> for WebgpuBorrowedTextureDescriptor {
    unsafe fn from_native(raw: sys::mln_webgpu_borrowed_texture_descriptor) -> Result<Self> {
        Ok(Self {
            extent: unsafe { from_native(raw.extent) }?,
            physical_width: raw.physical_width,
            physical_height: raw.physical_height,
            context: unsafe { from_native(raw.context) }?,
            textures: unsafe { convert::copy_array(raw.textures, raw.texture_count) }?,
            format: raw.format,
        })
    }
}

/// WebGPU backend context fields shared by WebGPU render targets.
///
/// See `mln_webgpu_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct WebgpuContextDescriptor {
    /// Borrowed WGPUInstance. Optional for texture targets.
    pub instance: *mut std::ffi::c_void,
    /// Borrowed WGPUDevice. Required.
    pub device: *mut std::ffi::c_void,
    /// Borrowed WGPUQueue. Optional; null uses the device default queue. A
    /// non-null queue must belong to device.
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
}
impl ToNative<sys::mln_webgpu_context_descriptor> for WebgpuContextDescriptor {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_webgpu_context_descriptor> {
        let mut raw: sys::mln_webgpu_context_descriptor = unsafe { std::mem::zeroed() };
        raw.instance = self.instance;
        raw.device = self.device;
        raw.queue = self.queue;
        Ok(raw)
    }
}
impl FromNative<sys::mln_webgpu_context_descriptor> for WebgpuContextDescriptor {
    unsafe fn from_native(raw: sys::mln_webgpu_context_descriptor) -> Result<Self> {
        Ok(Self {
            instance: raw.instance,
            device: raw.device,
            queue: raw.queue,
        })
    }
}

/// WebGPU attachment options for an owned texture target.
///
/// See `mln_webgpu_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WebgpuOwnedTextureDescriptor {
    /// Logical texture extent. A scale_factor that differs from the map's is
    /// accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Borrowed WebGPU context. device is required.
    pub context: WebgpuContextDescriptor,
}
impl Default for WebgpuOwnedTextureDescriptor {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_webgpu_owned_texture_descriptor_default() })
    }
}
impl WebgpuOwnedTextureDescriptor {
    pub const fn new(extent: LogicalExtent, context: WebgpuContextDescriptor) -> Self {
        Self { extent, context }
    }
}
impl ToNative<sys::mln_webgpu_owned_texture_descriptor> for WebgpuOwnedTextureDescriptor {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_webgpu_owned_texture_descriptor> {
        let mut raw: sys::mln_webgpu_owned_texture_descriptor =
            unsafe { sys::mln_webgpu_owned_texture_descriptor_default() };
        raw.size = std::mem::size_of::<sys::mln_webgpu_owned_texture_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.context = to_native(&self.context, arena)?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_webgpu_owned_texture_descriptor> for WebgpuOwnedTextureDescriptor {
    unsafe fn from_native(raw: sys::mln_webgpu_owned_texture_descriptor) -> Result<Self> {
        Ok(Self {
            extent: unsafe { from_native(raw.extent) }?,
            context: unsafe { from_native(raw.context) }?,
        })
    }
}

/// WebGPU attachment options for a native surface.
///
/// See `mln_webgpu_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WebgpuSurfaceDescriptor {
    /// Logical surface extent. A scale_factor that differs from the map's is
    /// accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Borrowed WebGPU context. device is required.
    pub context: WebgpuContextDescriptor,
    /// Borrowed WGPUSurface. Required, and must stay alive for the session. The
    /// session configures it for this device and extent, and unconfigures it
    /// when the session ends.
    pub surface: *mut std::ffi::c_void,
    /// WGPUTextureFormat to configure the surface with. Required. A browser
    /// host takes it from navigator.gpu.getPreferredCanvasFormat().
    pub format: u32,
}
impl Default for WebgpuSurfaceDescriptor {
    fn default() -> Self {
        convert::native_default(unsafe { sys::mln_webgpu_surface_descriptor_default() })
    }
}
impl WebgpuSurfaceDescriptor {
    pub const fn new(
        extent: LogicalExtent,
        context: WebgpuContextDescriptor,
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
}
impl ToNative<sys::mln_webgpu_surface_descriptor> for WebgpuSurfaceDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_webgpu_surface_descriptor> {
        let mut raw: sys::mln_webgpu_surface_descriptor =
            unsafe { sys::mln_webgpu_surface_descriptor_default() };
        raw.size = std::mem::size_of::<sys::mln_webgpu_surface_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.context = to_native(&self.context, arena)?;
        raw.surface = self.surface;
        raw.format = self.format;
        Ok(raw)
    }
}
impl FromNative<sys::mln_webgpu_surface_descriptor> for WebgpuSurfaceDescriptor {
    unsafe fn from_native(raw: sys::mln_webgpu_surface_descriptor) -> Result<Self> {
        Ok(Self {
            extent: unsafe { from_native(raw.extent) }?,
            context: unsafe { from_native(raw.context) }?,
            surface: raw.surface,
            format: raw.format,
        })
    }
}

/// WebGPU frame acquired from a texture ring.
///
/// See `mln_webgpu_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct WebgpuTextureFrame {
    /// Session generation that produced this frame.
    pub generation: u64,
    /// Physical WebGPU texture width in device pixels.
    pub width: u32,
    /// Physical WebGPU texture height in device pixels.
    pub height: u32,
    /// UI-to-device pixel scale used for this frame.
    pub scale_factor: f64,
    /// Opaque frame identity used to reject stale releases.
    pub frame_id: u64,
    /// Ring slot that holds this frame. For a borrowed target, the index of its
    /// texture in the descriptor's textures array.
    pub slot: u32,
    /// Borrowed WGPUTexture. Valid until frame release.
    pub texture: *mut std::ffi::c_void,
    /// Borrowed WGPUTextureView. Valid until frame release.
    pub texture_view: *mut std::ffi::c_void,
    /// Borrowed WGPUDevice. Valid until frame release.
    pub device: *mut std::ffi::c_void,
    /// Backend-native WGPUTextureFormat value.
    pub format: u32,
}
impl WebgpuTextureFrame {
    pub const fn new(
        generation: u64,
        width: u32,
        height: u32,
        scale_factor: f64,
        frame_id: u64,
        slot: u32,
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
            slot,
            texture,
            texture_view,
            device,
            format,
        }
    }
}
impl FromNative<sys::mln_webgpu_texture_frame> for WebgpuTextureFrame {
    unsafe fn from_native(raw: sys::mln_webgpu_texture_frame) -> Result<Self> {
        Ok(Self {
            generation: raw.generation,
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: raw.frame_id,
            slot: raw.slot,
            texture: raw.texture,
            texture_view: raw.texture_view,
            device: raw.device,
            format: raw.format,
        })
    }
}

/// WGL context fields shared by OpenGL render targets on Windows.
///
/// See `mln_wgl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct WglContextDescriptor {
    /// Borrowed HDC used to create the session context. Required.
    pub device_context: *mut std::ffi::c_void,
    /// Borrowed HGLRC whose share group the session context joins. Required
    /// under shared ownership. A dedicated session joins no share group, so it
    /// must be null there.
    pub share_context: *mut std::ffi::c_void,
    /// Optional wglGetProcAddress-compatible function for the host loader.
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
}
impl ToNative<sys::mln_wgl_context_descriptor> for WglContextDescriptor {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_wgl_context_descriptor> {
        let mut raw: sys::mln_wgl_context_descriptor = unsafe { std::mem::zeroed() };
        raw.device_context = self.device_context;
        raw.share_context = self.share_context;
        raw.get_proc_address = self.get_proc_address;
        Ok(raw)
    }
}
impl FromNative<sys::mln_wgl_context_descriptor> for WglContextDescriptor {
    unsafe fn from_native(raw: sys::mln_wgl_context_descriptor) -> Result<Self> {
        Ok(Self {
            device_context: raw.device_context,
            share_context: raw.share_context,
            get_proc_address: raw.get_proc_address,
        })
    }
}

unsafe fn register_resource_request_cancel(
    handle: sys::mln_resource_request_handle,
    callback: maplibre_core::decision::ContextCallback,
    user_data: *mut std::ffi::c_void,
    release: maplibre_core::decision::ContextCallback,
    out_cancelled: *mut bool,
    out_diagnostic: *mut sys::mln_diagnostic,
) -> sys::mln_status {
    let mut handler: sys::mln_resource_request_cancel_handler = unsafe { std::mem::zeroed() };
    handler.size = std::mem::size_of::<sys::mln_resource_request_cancel_handler>() as _;
    handler.callback = callback;
    handler.user_data = user_data;
    handler.release_user_data = release;
    // SAFETY: the caller passes the decision handle and the outputs that
    // the C function requires; the handler is borrowed for the call.
    unsafe {
        sys::mln_resource_request_set_cancel_callback(
            handle,
            &handler,
            out_cancelled,
            out_diagnostic,
        )
    }
}
pub(crate) const RESOURCE_REQUEST_DECISION: maplibre_core::decision::DecisionHandleFns<
    sys::mln_resource_request_handle,
> = unsafe {
    maplibre_core::decision::DecisionHandleFns::new(
        "ResourceRequestHandle",
        sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE,
        sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH,
        sys::mln_resource_request_release,
        register_resource_request_cancel,
        &[
            "mln_resource_request_complete",
            "mln_resource_request_is_cancelled",
            "mln_resource_request_set_cancel_callback",
            "mln_resource_request_release",
        ],
    )
};
