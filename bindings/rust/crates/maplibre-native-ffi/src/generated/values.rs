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

/// Optional animation controls for camera transitions.
///
/// See `mln_animation_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Default)]
pub struct AnimationOptions {
    /// Duration in milliseconds. Must be finite, non-negative, and short enough
    /// to represent as a duration.
    pub duration_ms: Option<f64>,
    /// Average fly velocity in screenfuls per second. Must be positive and
    /// finite; absent selects 1.2. A flight whose derived duration is too long
    /// to represent fails its completion with `MLN_STATUS_NATIVE_ERROR`. An
    /// explicit duration overrides velocity.
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
impl ToNative<sys::mln_animation_options> for AnimationOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_animation_options> {
        let mut raw: sys::mln_animation_options = unsafe { std::mem::zeroed() };
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

/// Optional map camera constraint fields.
///
/// See `mln_bound_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
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
impl ToNative<sys::mln_bound_options> for BoundOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_bound_options> {
        let mut raw: sys::mln_bound_options = unsafe { std::mem::zeroed() };
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

/// A complete camera, as native reports it.
///
/// See `mln_camera` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Camera {
    pub center: LatLng,
    pub center_altitude: f64,
    pub padding: EdgeInsets,
    pub zoom: f64,
    pub bearing: f64,
    pub pitch: f64,
    pub roll: f64,
    pub field_of_view: f64,
}
impl Camera {
    pub const fn new(
        center: LatLng,
        center_altitude: f64,
        padding: EdgeInsets,
        zoom: f64,
        bearing: f64,
        pitch: f64,
        roll: f64,
        field_of_view: f64,
    ) -> Self {
        Self {
            center,
            center_altitude,
            padding,
            zoom,
            bearing,
            pitch,
            roll,
            field_of_view,
        }
    }
}
impl ToNative<sys::mln_camera> for Camera {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_camera> {
        let mut raw: sys::mln_camera = unsafe { std::mem::zeroed() };
        raw.center = to_native(&self.center, arena)?;
        raw.center_altitude = self.center_altitude;
        raw.padding = to_native(&self.padding, arena)?;
        raw.zoom = self.zoom;
        raw.bearing = self.bearing;
        raw.pitch = self.pitch;
        raw.roll = self.roll;
        raw.field_of_view = self.field_of_view;
        Ok(raw)
    }
}
impl FromNative<sys::mln_camera> for Camera {
    unsafe fn from_native(raw: sys::mln_camera) -> Result<Self> {
        Ok(Self {
            center: unsafe { from_native(raw.center) }?,
            center_altitude: raw.center_altitude,
            padding: unsafe { from_native(raw.padding) }?,
            zoom: raw.zoom,
            bearing: raw.bearing,
            pitch: raw.pitch,
            roll: raw.roll,
            field_of_view: raw.field_of_view,
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
impl Default for CameraChangeMode {
    fn default() -> Self {
        Self::Immediate
    }
}

/// One atomic relative camera update.
///
/// See `mln_camera_delta` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Default)]
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
impl ToNative<sys::mln_camera_delta> for CameraDelta {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_camera_delta> {
        let mut raw: sys::mln_camera_delta = unsafe { std::mem::zeroed() };
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

/// Optional fitting controls for camera-for-viewport queries.
///
/// See `mln_camera_fit_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct CameraFitOptions {
    pub padding: Option<EdgeInsets>,
    pub bearing: Option<f64>,
    pub pitch: Option<f64>,
}
impl ToNative<sys::mln_camera_fit_options> for CameraFitOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_camera_fit_options> {
        let mut raw: sys::mln_camera_fit_options = unsafe { std::mem::zeroed() };
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

/// Camera fields used by snapshots and camera updates.
///
/// See `mln_camera_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
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
impl ToNative<sys::mln_camera_options> for CameraOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_camera_options> {
        let mut raw: sys::mln_camera_options = unsafe { std::mem::zeroed() };
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
    pub camera: Camera,
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
    pub outcome: CameraTransitionOutcome,
    /// Generation of the published snapshot that shows the camera where the
    /// transitions left it. The map events of the change that ended them carry
    /// this generation and are queued before the callback runs. Zero for
    /// `MLN_CAMERA_TRANSITION_OUTCOME_CLOSED`.
    pub generation: u64,
}
impl FromNative<sys::mln_camera_transition_end> for CameraTransitionEnd {
    unsafe fn from_native(raw: sys::mln_camera_transition_end) -> Result<Self> {
        Ok(Self {
            outcome: CameraTransitionOutcome::from_raw(raw.outcome),
            generation: raw.generation,
        })
    }
}

/// Callback or queue state that one camera command copies to report the end of
/// its transitions.
///
/// See `mln_camera_transition_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Clone, Default)]
pub struct CameraTransitionHandler {
    /// The callback receives (`end`).
    pub callback: Option<std::sync::Arc<dyn Fn(CameraTransitionEnd) + Send + Sync + 'static>>,
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
        F: Fn(CameraTransitionEnd) + Send + Sync + 'static,
    {
        self.callback = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(CameraTransitionEnd) + Send + Sync + 'static,
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
        callback::invoke("mln_camera_transition_end_callback", (), || {
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
impl Default for CameraTransitionOutcome {
    fn default() -> Self {
        Self::Completed
    }
}

/// One atomic absolute camera update.
///
/// See `mln_camera_update` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Default)]
pub struct CameraUpdate {
    pub mode: CameraUpdateMode,
    pub camera: CameraOptions,
    pub animation: AnimationOptions,
    pub gesture_phase: GesturePhase,
}
impl ToNative<sys::mln_camera_update> for CameraUpdate {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_camera_update> {
        let mut raw: sys::mln_camera_update = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_camera_update>() as _;
        raw.mode = to_native(&self.mode, arena)?;
        raw.camera = to_native(&self.camera, arena)?;
        raw.animation = to_native(&self.animation, arena)?;
        raw.gesture_phase = to_native(&self.gesture_phase, arena)?;
        Ok(raw)
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
impl Default for CameraUpdateMode {
    fn default() -> Self {
        Self::Jump
    }
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
impl Default for ConstrainMode {
    fn default() -> Self {
        Self::None
    }
}

/// Options for custom geometry sources.
///
/// See `mln_custom_geometry_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Clone)]
pub struct CustomGeometrySourceOptions {
    /// Tile fetch callback. Required in callback mode; null in queue mode.
    ///
    /// The callback receives (`tile_id`).
    pub fetch_tile: std::sync::Arc<dyn Fn(CanonicalTileId) + Send + Sync + 'static>,
    /// Optional best-effort tile cancel callback. Null in queue mode.
    ///
    /// The callback receives (`tile_id`).
    pub cancel_tile: Option<std::sync::Arc<dyn Fn(CanonicalTileId) + Send + Sync + 'static>>,
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
    pub tolerance: Option<f64>,
    pub tile_size: Option<u32>,
    pub buffer: Option<u32>,
    pub clip: Option<bool>,
    pub wrap: Option<bool>,
}
impl std::fmt::Debug for CustomGeometrySourceOptions {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("CustomGeometrySourceOptions")
            .finish_non_exhaustive()
    }
}
impl CustomGeometrySourceOptions {
    pub fn with_cancel_tile<F>(mut self, callback: F) -> Self
    where
        F: Fn(CanonicalTileId) + Send + Sync + 'static,
    {
        self.cancel_tile = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(fetch_tile: F) -> Self
    where
        F: Fn(CanonicalTileId) + Send + Sync + 'static,
    {
        Self {
            fetch_tile: std::sync::Arc::new(fetch_tile),
            cancel_tile: None,
            min_zoom: Default::default(),
            max_zoom: Default::default(),
            tolerance: Default::default(),
            tile_size: Default::default(),
            buffer: Default::default(),
            clip: Default::default(),
            wrap: Default::default(),
        }
    }
    unsafe extern "C" fn fetch_tile_trampoline(
        user_data: *mut std::ffi::c_void,
        tile_id: sys::mln_canonical_tile_id,
    ) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_custom_source_fetch_tile_callback", (), || {
            (state.fetch_tile)(unsafe { from_native(tile_id) }?);
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
        callback::invoke("mln_custom_source_cancel_tile_callback", (), || {
            callback::require(&state.cancel_tile)?(unsafe { from_native(tile_id) }?);
            Ok(())
        })
    }
}
impl ToNative<sys::mln_custom_geometry_source_options> for CustomGeometrySourceOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_custom_geometry_source_options> {
        let mut raw: sys::mln_custom_geometry_source_options = unsafe { std::mem::zeroed() };
        raw.fields = 0;
        raw.size = std::mem::size_of::<sys::mln_custom_geometry_source_options>() as _;
        raw.fetch_tile = Some(Self::fetch_tile_trampoline as _);
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

/// Options for custom MVT vector sources.
///
/// See `mln_custom_mvt_vector_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Clone)]
pub struct CustomMvtVectorSourceOptions {
    /// Tile fetch callback. Required in callback mode; null in queue mode.
    ///
    /// The callback receives (`tile_id`).
    pub fetch_tile: std::sync::Arc<dyn Fn(CanonicalTileId) + Send + Sync + 'static>,
    /// Optional best-effort tile cancel callback. Null in queue mode.
    ///
    /// The callback receives (`tile_id`).
    pub cancel_tile: Option<std::sync::Arc<dyn Fn(CanonicalTileId) + Send + Sync + 'static>>,
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
}
impl std::fmt::Debug for CustomMvtVectorSourceOptions {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("CustomMvtVectorSourceOptions")
            .finish_non_exhaustive()
    }
}
impl CustomMvtVectorSourceOptions {
    pub fn with_cancel_tile<F>(mut self, callback: F) -> Self
    where
        F: Fn(CanonicalTileId) + Send + Sync + 'static,
    {
        self.cancel_tile = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(fetch_tile: F) -> Self
    where
        F: Fn(CanonicalTileId) + Send + Sync + 'static,
    {
        Self {
            fetch_tile: std::sync::Arc::new(fetch_tile),
            cancel_tile: None,
            min_zoom: Default::default(),
            max_zoom: Default::default(),
        }
    }
    unsafe extern "C" fn fetch_tile_trampoline(
        user_data: *mut std::ffi::c_void,
        tile_id: sys::mln_canonical_tile_id,
    ) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_custom_source_fetch_tile_callback", (), || {
            (state.fetch_tile)(unsafe { from_native(tile_id) }?);
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
        callback::invoke("mln_custom_source_cancel_tile_callback", (), || {
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
        let mut raw: sys::mln_custom_mvt_vector_source_options = unsafe { std::mem::zeroed() };
        raw.fields = 0;
        raw.size = std::mem::size_of::<sys::mln_custom_mvt_vector_source_options>() as _;
        raw.fetch_tile = Some(Self::fetch_tile_trampoline as _);
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
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct EglContextDescriptor {
    /// Borrowed EGLDisplay. Required and kept initialized through teardown.
    /// eglGetDisplay(EGL_DEFAULT_DISPLAY) returns the same display for the
    /// whole process, so a shared display must stay initialized until every
    /// session using it has detached or been destroyed. The host must not call
    /// eglTerminate per session.
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
    pub client_api: OpenGlClientApi,
    /// Optional eglGetProcAddress-compatible function for the host loader.
    pub get_proc_address: *mut std::ffi::c_void,
}
impl EglContextDescriptor {
    pub fn new(display: *mut std::ffi::c_void, config: *mut std::ffi::c_void) -> Self {
        Self {
            display,
            config,
            share_context: Default::default(),
            client_api: Default::default(),
            get_proc_address: Default::default(),
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

/// Feature-state source, feature, and key selector.
///
/// See `mln_feature_state_selector` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct FeatureStateSelector {
    /// Source ID. Required. Borrowed for the duration of the call.
    pub source_id: String,
    /// Source layer ID, or empty for none. Required for vector-source
    /// disambiguation.
    pub source_layer_id: String,
    /// Optional feature ID. Required by set and get, optional for remove. A
    /// numeric feature ID matches its decimal text, such as "7" for the GeoJSON
    /// id 7. Its field bit tells an absent ID from the empty ID "".
    pub feature_id: Option<String>,
    /// State key, or empty to select every key. Used only by remove, and
    /// requires feature_id.
    pub state_key: String,
}
impl FeatureStateSelector {
    pub fn new(source_id: String) -> Self {
        Self {
            source_id,
            source_layer_id: Default::default(),
            feature_id: None,
            state_key: Default::default(),
        }
    }
}
impl ToNative<sys::mln_feature_state_selector> for FeatureStateSelector {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_feature_state_selector> {
        let mut raw: sys::mln_feature_state_selector = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_feature_state_selector>() as _;
        raw.fields = 0;
        raw.source_id = to_native(&self.source_id, arena)?;
        raw.source_layer_id = to_native(&self.source_layer_id, arena)?;
        if let Some(item) = &self.feature_id {
            raw.fields |= sys::MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
            raw.feature_id = to_native(&*item, arena)?;
        }
        raw.state_key = to_native(&self.state_key, arena)?;
        Ok(raw)
    }
}

/// One nonblocking request for a frame.
///
/// See `mln_frame_demand` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct FrameDemand {
    /// A bitwise OR of `mln_frame_demand_flag` values. Zero renders only when a
    /// newer map update exists, without presenting.
    pub flags: FrameDemandFlag,
    /// Host identity returned with the terminal frame result. Any value except
    /// `MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE`, the maximum uint64_t value.
    pub token: u64,
}
impl ToNative<sys::mln_frame_demand> for FrameDemand {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_frame_demand> {
        let mut raw: sys::mln_frame_demand = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_frame_demand>() as _;
        raw.flags = to_native(&self.flags, arena)?;
        raw.token = self.token;
        Ok(raw)
    }
}

native_flags! {
/// Frame-demand policy bits.
///
/// See `mln_frame_demand_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
pub struct FrameDemandFlag: u32 {
    /// Render even when no map update is newer than the last rendered one.
    const FORCE = 1;
    /// Present the rendered frame on a target that supports presentation. A
    /// presenting target whose demand clears this bit still renders and keeps
    /// whatever it presented last. Ignored by targets without presentation.
    const PRESENT = 2;
    /// Without `MLN_FRAME_DEMAND_FORCE`, a demand that would finish with
    /// `MLN_RENDER_RESULT_NO_UPDATE` waits instead, and runs again after the
    /// map's next update, a target replacement, or an applied resize. A waiting
    /// demand holds no ring slot. A later demand with the same flags supersedes
    /// it, a barrier ends its wait with `MLN_RENDER_RESULT_NO_UPDATE`, and
    /// detach, abandon, or the quarantine of the ring's last usable slot end it
    /// with `MLN_RENDER_RESULT_TARGET_NOT_READY`. A waiting demand's result can
    /// follow the results of demands accepted after it. The flag does not pace:
    /// a host that re-arms a waiting demand as each result arrives renders
    /// every update the map publishes, and a command that arrives while a frame
    /// renders appears in a later frame. To show several commands in one frame,
    /// submit them in a command group, or arm demands once per display frame.
    const WAIT_FOR_UPDATE = 4;
}
}

/// Free camera position and orientation in MapLibre Native camera space.
///
/// See `mln_free_camera_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct FreeCameraOptions {
    pub position: Option<Vec3>,
    pub orientation: Option<Quaternion>,
}
impl ToNative<sys::mln_free_camera_options> for FreeCameraOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_free_camera_options> {
        let mut raw: sys::mln_free_camera_options = unsafe { std::mem::zeroed() };
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

/// Options for GeoJSON sources, indexed and rendered at a fixed 512-pixel tile
/// size.
///
/// See `mln_geo_json_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct GeoJsonSourceOptions {
    /// Minimum tiling zoom. Absent selects 0.
    pub min_zoom: Option<f64>,
    /// Maximum tiling zoom, an integer within \[0, 24\]. Absent selects 18.
    pub max_zoom: Option<f64>,
    /// Douglas-Peucker simplification tolerance. Absent selects 0.375.
    pub tolerance: Option<f64>,
    /// Highest zoom that clusters points. Absent selects 17.
    pub cluster_max_zoom: Option<f64>,
    /// Cluster aggregation expressions keyed by property name, as a JSON object
    /// whose members follow the MapLibre Style Spec clusterProperties form, or
    /// empty for none. The UTF-8 bytes are borrowed for the call.
    pub cluster_properties: String,
    /// Tile buffer in pixels within \[0, 512\]. Absent selects 128.
    pub buffer: Option<u32>,
    /// Cluster radius in pixels within \[0, 4095\]. Absent selects 50.
    pub cluster_radius: Option<u32>,
    /// Points required to form a cluster. Absent selects 2.
    pub cluster_min_points: Option<u32>,
    /// Adds line distance metrics to line features. Absent selects false.
    pub line_metrics: Option<bool>,
    /// Clusters point features. Absent selects false.
    pub cluster: Option<bool>,
    /// Tiles the data for the next render update instead of in the background.
    /// Absent selects false.
    pub synchronous_tiling: Option<bool>,
}
impl ToNative<sys::mln_geo_json_source_options> for GeoJsonSourceOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_geo_json_source_options> {
        let mut raw: sys::mln_geo_json_source_options = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_geo_json_source_options>() as _;
        raw.fields = 0;
        if let Some(item) = &self.min_zoom {
            raw.fields |= sys::MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM;
            raw.min_zoom = *item;
        }
        if let Some(item) = &self.max_zoom {
            raw.fields |= sys::MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM;
            raw.max_zoom = *item;
        }
        if let Some(item) = &self.tolerance {
            raw.fields |= sys::MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE;
            raw.tolerance = *item;
        }
        if let Some(item) = &self.cluster_max_zoom {
            raw.fields |= sys::MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM;
            raw.cluster_max_zoom = *item;
        }
        raw.cluster_properties = to_native(&self.cluster_properties, arena)?;
        if let Some(item) = &self.buffer {
            raw.fields |= sys::MLN_GEO_JSON_SOURCE_OPTION_BUFFER;
            raw.buffer = *item;
        }
        if let Some(item) = &self.cluster_radius {
            raw.fields |= sys::MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS;
            raw.cluster_radius = *item;
        }
        if let Some(item) = &self.cluster_min_points {
            raw.fields |= sys::MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS;
            raw.cluster_min_points = *item;
        }
        if let Some(item) = &self.line_metrics {
            raw.fields |= sys::MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS;
            raw.line_metrics = *item;
        }
        if let Some(item) = &self.cluster {
            raw.fields |= sys::MLN_GEO_JSON_SOURCE_OPTION_CLUSTER;
            raw.cluster = *item;
        }
        if let Some(item) = &self.synchronous_tiling {
            raw.fields |= sys::MLN_GEO_JSON_SOURCE_OPTION_SYNCHRONOUS_TILING;
            raw.synchronous_tiling = *item;
        }
        Ok(raw)
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
impl Default for GesturePhase {
    fn default() -> Self {
        Self::None
    }
}

/// Backend synchronization copied by frame access and release calls.
///
/// See `mln_gpu_sync` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct GpuSync {
    pub kind: GpuSyncKind,
    /// Bit pattern of the GLsync pointer.
    pub handle: u64,
}
impl ToNative<sys::mln_gpu_sync> for GpuSync {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_gpu_sync> {
        let mut raw: sys::mln_gpu_sync = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_gpu_sync>() as _;
        raw.kind = to_native(&self.kind, arena)?;
        raw.handle = self.handle;
        Ok(raw)
    }
}
impl FromNative<sys::mln_gpu_sync> for GpuSync {
    unsafe fn from_native(raw: sys::mln_gpu_sync) -> Result<Self> {
        Ok(Self {
            kind: GpuSyncKind::from_raw(raw.kind),
            handle: raw.handle,
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
    /// GLsync in the shared EGL or WGL context's share group, used only by a
    /// caller-graphics-thread driver. handle is nonzero.
    OpenGlFence = 1,
} Unknown
}
impl Default for GpuSyncKind {
    fn default() -> Self {
        Self::CpuComplete
    }
}

/// One HTTP header that a header rule supplies.
///
/// See `mln_http_header` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct HttpHeader {
    /// Header field name. Required.
    pub name: String,
    pub value: String,
}
impl HttpHeader {
    pub fn new(name: String) -> Self {
        Self {
            name,
            value: Default::default(),
        }
    }
}
impl ToNative<sys::mln_http_header> for HttpHeader {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_http_header> {
        let mut raw: sys::mln_http_header = unsafe { std::mem::zeroed() };
        raw.name = to_native(&self.name, arena)?;
        raw.value = to_native(&self.value, arena)?;
        Ok(raw)
    }
}

/// One rule of a native HTTP header transform.
///
/// See `mln_http_header_rule` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct HttpHeaderRule {
    /// The resource kinds to match. Zero selects every kind.
    pub kinds: ResourceKindFlag,
    pub flags: UrlMatchFlag,
    /// The URL or glob pattern to compare.
    pub url: String,
    /// The headers to set. Registration validates each one as
    /// `mln_http_header_transform_response_set_header()` does.
    pub headers: Vec<HttpHeader>,
}
impl ToNative<sys::mln_http_header_rule> for HttpHeaderRule {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_http_header_rule> {
        let mut raw: sys::mln_http_header_rule = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_http_header_rule>() as _;
        raw.kinds = to_native(&self.kinds, arena)?;
        raw.flags = to_native(&self.flags, arena)?;
        raw.url = to_native(&self.url, arena)?;
        raw.headers = convert::array(&self.headers, arena)?;
        raw.header_count = convert::count(self.headers.len())?;
        Ok(raw)
    }
}

/// HTTP header transform state: a callback, or native header rules.
///
/// See `mln_http_header_transform` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Clone, Default)]
pub struct HttpHeaderTransform {
    /// The callback, or null to apply rules.
    ///
    /// The callback receives (`kind`, `url`, `out_response`).
    ///
    /// An `Err` of `Error::Status` with a failure status answers native
    /// that status. Any other error, or a panic, reaches the reporter and
    /// answers `MLN_STATUS_NATIVE_ERROR`.
    pub callback: Option<
        std::sync::Arc<
            dyn Fn(ResourceKind, &str, &mut HttpHeaderTransformResponse<'_>) -> Result<()>
                + Send
                + Sync
                + 'static,
        >,
    >,
    /// Header rules for a transform without a callback; empty with one.
    pub rules: Vec<HttpHeaderRule>,
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
        F: Fn(ResourceKind, &str, &mut HttpHeaderTransformResponse<'_>) -> Result<()>
            + Send
            + Sync
            + 'static,
    {
        self.callback = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(ResourceKind, &str, &mut HttpHeaderTransformResponse<'_>) -> Result<()>
            + Send
            + Sync
            + 'static,
    {
        Self::default().with_callback(callback)
    }
    unsafe extern "C" fn callback_trampoline(
        user_data: *mut std::ffi::c_void,
        kind: sys::mln_resource_kind,
        url: sys::mln_buffer_view,
        out_response: *mut sys::mln_http_header_transform_response,
    ) -> sys::mln_status {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke_status(
            "mln_http_header_transform_callback",
            sys::MLN_STATUS_NATIVE_ERROR,
            || {
                Ok(callback::require(&state.callback)?(
                    ResourceKind::from_raw(kind),
                    unsafe { maplibre_core::string::borrow_string_view(url) }?,
                    &mut HttpHeaderTransformResponse::new(out_response)?,
                ))
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
        raw.rules = convert::array(&self.rules, arena)?;
        raw.rule_count = convert::count(self.rules.len())?;
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
        let arena = &mut InputArena::default();
        let native_name = to_native(&name, arena)?;
        let native_value = to_native(&value, arena)?;
        maplibre_core::check(|out_diagnostic| unsafe {
            sys::mln_http_header_transform_response_set_header(
                self.raw.as_ptr(),
                native_name,
                native_value,
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
    /// Latitude in degrees, finite and within \[-90, 90\] on input.
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
impl Default for LogEvent {
    fn default() -> Self {
        Self::General
    }
}

/// Process-global log callback or queue state.
///
/// See `mln_log_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
#[derive(Clone)]
pub struct LogHandler {
    /// Receives each record.
    ///
    /// The callback receives (`severity`, `event`, `code`, `message`).
    pub callback:
        std::sync::Arc<dyn Fn(LogSeverity, LogEvent, i64, &str) -> bool + Send + Sync + 'static>,
}
impl std::fmt::Debug for LogHandler {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("LogHandler").finish_non_exhaustive()
    }
}
impl LogHandler {
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(LogSeverity, LogEvent, i64, &str) -> bool + Send + Sync + 'static,
    {
        Self {
            callback: std::sync::Arc::new(callback),
        }
    }
    unsafe extern "C" fn callback_trampoline(
        user_data: *mut std::ffi::c_void,
        severity: sys::mln_log_severity,
        event: sys::mln_log_event,
        code: i64,
        message: sys::mln_buffer_view,
    ) -> bool {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_log_callback", false, || {
            let value = (state.callback)(
                LogSeverity::from_raw(severity),
                LogEvent::from_raw(event),
                code,
                unsafe { maplibre_core::string::borrow_string_view(message) }?,
            );
            Ok(value)
        })
    }
}
impl ToNative<sys::mln_log_handler> for LogHandler {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_log_handler> {
        let mut raw: sys::mln_log_handler = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_log_handler>() as _;
        raw.callback = Some(Self::callback_trampoline as _);
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
/// See `mln_log_severity_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
pub struct LogSeverityFlag: u32 {
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
    /// Width in UI pixels.
    pub width: u32,
    /// Height in UI pixels.
    pub height: u32,
    /// Device pixels per UI pixel.
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
/// See `mln_map_debug_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub struct MapDebugFlag: u32 {
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
impl Default for MapMode {
    fn default() -> Self {
        Self::Continuous
    }
}

/// Options used when creating a map.
///
/// See `mln_map_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MapOptions {
    /// Initial logical extent. Required. Width and height must be nonzero, and
    /// scale_factor must be finite and positive. scale_factor is fixed for the
    /// map's lifetime.
    pub initial_extent: LogicalExtent,
    pub map_mode: MapMode,
    /// Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR
    /// encodings.
    pub fast_pfor_enabled: bool,
    /// Map-originated event types this map queues, as a bitwise OR of
    /// `mln_runtime_event_flag` values.
    pub event_mask: Option<RuntimeEventFlag>,
}
impl MapOptions {
    pub fn new(initial_extent: LogicalExtent) -> Self {
        Self {
            initial_extent,
            map_mode: Default::default(),
            fast_pfor_enabled: Default::default(),
            event_mask: None,
        }
    }
}
impl ToNative<sys::mln_map_options> for MapOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_map_options> {
        let mut raw: sys::mln_map_options = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_map_options>() as _;
        raw.fields = 0;
        raw.initial_extent = to_native(&self.initial_extent, arena)?;
        raw.map_mode = to_native(&self.map_mode, arena)?;
        raw.fast_pfor_enabled = self.fast_pfor_enabled;
        if let Some(item) = &self.event_mask {
            raw.fields |= sys::MLN_MAP_OPTION_EVENT_MASK;
            raw.event_mask = to_native(&*item, arena)?;
        }
        Ok(raw)
    }
}

/// Immutable map state copied from the latest published generation.
///
/// See `mln_map_snapshot` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MapSnapshot {
    /// Debug overlay mask of `mln_map_debug_flag` values.
    pub debug_options: MapDebugFlag,
    pub generation: u64,
    /// True once every requested style and tile resource finished loading.
    pub fully_loaded: bool,
    pub rendering_stats_view_enabled: bool,
    /// True while the map is inside a gesture.
    pub gesture_in_progress: bool,
    /// True while a camera command with a duration is still animating.
    pub transition_in_progress: bool,
    pub event_mask: RuntimeEventFlag,
    /// Generation of the latest render update the map published. A rendered
    /// frame at or past it draws map state that includes every command this
    /// snapshot observes, though animations and resource loads finish in later
    /// frames.
    pub latest_render_update_generation: u64,
    pub tile: MapTileOptions,
    pub bounds: BoundOptions,
    pub free_camera: FreeCameraOptions,
    /// The map's camera, extent, projection mode, and viewport options.
    /// `mln_map_projection_create()` converts coordinates for this view.
    pub view: MapView,
}
impl FromNative<sys::mln_map_snapshot> for MapSnapshot {
    unsafe fn from_native(raw: sys::mln_map_snapshot) -> Result<Self> {
        Ok(Self {
            debug_options: MapDebugFlag::from_bits_retain(raw.debug_options),
            generation: raw.generation,
            fully_loaded: raw.fully_loaded,
            rendering_stats_view_enabled: raw.rendering_stats_view_enabled,
            gesture_in_progress: raw.gesture_in_progress,
            transition_in_progress: raw.transition_in_progress,
            event_mask: RuntimeEventFlag::from_bits_retain(raw.event_mask),
            latest_render_update_generation: raw.latest_render_update_generation,
            tile: unsafe { from_native(raw.tile) }?,
            bounds: unsafe { from_native(raw.bounds) }?,
            free_camera: unsafe { from_native(raw.free_camera) }?,
            view: unsafe { from_native(raw.view) }?,
        })
    }
}

/// Tile prefetch and LOD tuning controls.
///
/// See `mln_map_tile_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MapTileOptions {
    /// Prefetch zoom delta, at most 255.
    pub prefetch_zoom_delta: Option<u32>,
    pub lod_min_radius: Option<f64>,
    pub lod_scale: Option<f64>,
    pub lod_pitch_threshold: Option<f64>,
    pub lod_zoom_shift: Option<f64>,
    pub lod_mode: Option<TileLodMode>,
}
impl ToNative<sys::mln_map_tile_options> for MapTileOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_map_tile_options> {
        let mut raw: sys::mln_map_tile_options = unsafe { std::mem::zeroed() };
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
            lod_mode: if raw.fields & sys::MLN_MAP_TILE_OPTION_LOD_MODE != 0 {
                Some(TileLodMode::from_raw(raw.lod_mode))
            } else {
                None
            },
        })
    }
}

/// Everything that a projection depends on: the transform that a published
/// snapshot or a rendered frame describes.
///
/// See `mln_map_view` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MapView {
    /// The camera. Its center is unwrapped: the longitude keeps the visible
    /// world copy and may fall outside -180 to 180.
    pub camera: Camera,
    /// Logical size and device-pixel scale. A snapshot carries the map's scale
    /// factor, and a frame carries the scale factor of the target that drew it.
    pub extent: LogicalExtent,
    /// Axonometric mode and skew, with every field set.
    pub projection_mode: ProjectionMode,
    /// North orientation, constrain mode, viewport mode, and frustum offset,
    /// with every field set.
    pub viewport: MapViewportOptions,
}
impl ToNative<sys::mln_map_view> for MapView {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_map_view> {
        let mut raw: sys::mln_map_view = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_map_view>() as _;
        raw.camera = to_native(&self.camera, arena)?;
        raw.extent = to_native(&self.extent, arena)?;
        raw.projection_mode = to_native(&self.projection_mode, arena)?;
        raw.viewport = to_native(&self.viewport, arena)?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_map_view> for MapView {
    unsafe fn from_native(raw: sys::mln_map_view) -> Result<Self> {
        Ok(Self {
            camera: unsafe { from_native(raw.camera) }?,
            extent: unsafe { from_native(raw.extent) }?,
            projection_mode: unsafe { from_native(raw.projection_mode) }?,
            viewport: unsafe { from_native(raw.viewport) }?,
        })
    }
}

/// Live map viewport and render-transform controls.
///
/// See `mln_map_viewport_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MapViewportOptions {
    pub north_orientation: Option<NorthOrientation>,
    pub constrain_mode: Option<ConstrainMode>,
    pub viewport_mode: Option<ViewportMode>,
    pub frustum_offset: Option<EdgeInsets>,
}
impl ToNative<sys::mln_map_viewport_options> for MapViewportOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_map_viewport_options> {
        let mut raw: sys::mln_map_viewport_options = unsafe { std::mem::zeroed() };
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
            north_orientation: if raw.fields & sys::MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION != 0 {
                Some(NorthOrientation::from_raw(raw.north_orientation))
            } else {
                None
            },
            constrain_mode: if raw.fields & sys::MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE != 0 {
                Some(ConstrainMode::from_raw(raw.constrain_mode))
            } else {
                None
            },
            viewport_mode: if raw.fields & sys::MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE != 0 {
                Some(ViewportMode::from_raw(raw.viewport_mode))
            } else {
                None
            },
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
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalBorrowedTexture {
    /// Borrowed `id<MTLTexture>` / `MTL::Texture*`. Required.
    pub texture: *mut std::ffi::c_void,
}
impl MetalBorrowedTexture {
    pub fn new(texture: *mut std::ffi::c_void) -> Self {
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

/// Metal attachment options for a borrowed texture target.
///
/// See `mln_metal_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct MetalBorrowedTextureDescriptor {
    /// Logical texture extent. Required. physical_width and physical_height
    /// give the texture's size. A scale_factor that differs from the map's is
    /// accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Physical texture width in device pixels. Required.
    pub physical_width: u32,
    /// Physical texture height in device pixels. Required.
    pub physical_height: u32,
    /// The ring's textures, one per slot, in slot order. Required.
    pub textures: Vec<MetalBorrowedTexture>,
}
impl MetalBorrowedTextureDescriptor {
    pub fn new(
        extent: LogicalExtent,
        physical_width: u32,
        physical_height: u32,
        textures: Vec<MetalBorrowedTexture>,
    ) -> Self {
        Self {
            extent,
            physical_width,
            physical_height,
            textures,
        }
    }
}
impl ToNative<sys::mln_metal_borrowed_texture_descriptor> for MetalBorrowedTextureDescriptor {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_metal_borrowed_texture_descriptor> {
        let mut raw: sys::mln_metal_borrowed_texture_descriptor = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_metal_borrowed_texture_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.physical_width = self.physical_width;
        raw.physical_height = self.physical_height;
        raw.textures = convert::array(&self.textures, arena)?;
        raw.texture_count = convert::count(self.textures.len())?;
        Ok(raw)
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
impl ToNative<sys::mln_metal_context_descriptor> for MetalContextDescriptor {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_metal_context_descriptor> {
        let mut raw: sys::mln_metal_context_descriptor = unsafe { std::mem::zeroed() };
        raw.device = self.device;
        Ok(raw)
    }
}

/// Metal attachment options for an owned texture target.
///
/// See `mln_metal_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalOwnedTextureDescriptor {
    /// Logical texture extent. Required. A scale_factor that differs from the
    /// map's is accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Metal backend context. device is required.
    pub context: MetalContextDescriptor,
    /// Backend-native MTLPixelFormat value of the ring's textures: either
    /// MTLPixelFormatRGBA8Unorm or MTLPixelFormatBGRA8Unorm. Required. Readback
    /// returns RGBA8 for either format.
    pub pixel_format: u64,
}
impl MetalOwnedTextureDescriptor {
    pub fn new(extent: LogicalExtent, pixel_format: u64) -> Self {
        Self {
            extent,
            context: Default::default(),
            pixel_format,
        }
    }
}
impl ToNative<sys::mln_metal_owned_texture_descriptor> for MetalOwnedTextureDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_metal_owned_texture_descriptor> {
        let mut raw: sys::mln_metal_owned_texture_descriptor = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_metal_owned_texture_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.context = to_native(&self.context, arena)?;
        raw.pixel_format = self.pixel_format;
        Ok(raw)
    }
}

/// Metal attachment options for a native surface.
///
/// See `mln_metal_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalSurfaceDescriptor {
    /// Logical surface extent. Required. A scale_factor that differs from the
    /// map's is accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Metal backend context. device is optional for Metal surfaces.
    pub context: MetalContextDescriptor,
    /// `CAMetalLayer*` / `CA::MetalLayer*` retained by the session. Required.
    pub layer: *mut std::ffi::c_void,
}
impl MetalSurfaceDescriptor {
    pub fn new(extent: LogicalExtent, layer: *mut std::ffi::c_void) -> Self {
        Self {
            extent,
            context: Default::default(),
            layer,
        }
    }
}
impl ToNative<sys::mln_metal_surface_descriptor> for MetalSurfaceDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_metal_surface_descriptor> {
        let mut raw: sys::mln_metal_surface_descriptor = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_metal_surface_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.context = to_native(&self.context, arena)?;
        raw.layer = self.layer;
        Ok(raw)
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
    /// Opaque identity of this frame.
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
impl Default for NorthOrientation {
    fn default() -> Self {
        Self::Up
    }
}

/// Geometry offline region definition.
///
/// See `mln_offline_geometry_region_definition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct OfflineGeometryRegionDefinition {
    /// Style URL. Copied during region creation.
    pub style_url: String,
    /// UTF-8 GeoJSON Geometry bytes. Required. Borrowed during region creation.
    pub geometry: String,
    pub min_zoom: f64,
    /// Maximum zoom. Positive infinity lets each tile source use its own
    /// maximum zoom.
    pub max_zoom: f64,
    pub pixel_ratio: f32,
    pub include_ideographs: bool,
}
impl OfflineGeometryRegionDefinition {
    pub fn new(geometry: String) -> Self {
        Self {
            style_url: Default::default(),
            geometry,
            min_zoom: Default::default(),
            max_zoom: Default::default(),
            pixel_ratio: Default::default(),
            include_ideographs: Default::default(),
        }
    }
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
#[derive(Debug, Clone, PartialEq)]
pub enum OfflineRegionDefinition {
    TilePyramid(OfflineTilePyramidRegionDefinition),
    Geometry(OfflineGeometryRegionDefinition),
    Unknown(u32),
}
impl Default for OfflineRegionDefinition {
    fn default() -> Self {
        Self::Unknown(0)
    }
}
impl ToNative<sys::mln_offline_region_definition> for OfflineRegionDefinition {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_offline_region_definition> {
        let mut raw: sys::mln_offline_region_definition = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_offline_region_definition>() as _;
        match self {
            OfflineRegionDefinition::TilePyramid(item) => {
                raw.type_ = sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID;
                raw.data.tile_pyramid = to_native(&(*item), arena)?;
            }
            OfflineRegionDefinition::Geometry(item) => {
                raw.type_ = sys::MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
                raw.data.geometry = to_native(&(*item), arena)?;
            }
            OfflineRegionDefinition::Unknown(_) => {
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
        Ok(match raw.type_ {
            sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID => {
                OfflineRegionDefinition::TilePyramid(unsafe { from_native(raw.data.tile_pyramid) }?)
            }
            sys::MLN_OFFLINE_REGION_DEFINITION_GEOMETRY => {
                OfflineRegionDefinition::Geometry(unsafe { from_native(raw.data.geometry) }?)
            }
            tag => OfflineRegionDefinition::Unknown(tag as u32),
        })
    }
}

native_enum! {
pub enum OfflineRegionDownloadState: u32 {
    Inactive = 0,
    Active = 1,
} Unknown
}
impl Default for OfflineRegionDownloadState {
    fn default() -> Self {
        Self::Inactive
    }
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
impl FromNative<sys::mln_offline_region_status> for OfflineRegionStatus {
    unsafe fn from_native(raw: sys::mln_offline_region_status) -> Result<Self> {
        Ok(Self {
            download_state: OfflineRegionDownloadState::from_raw(raw.download_state),
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
    /// Maximum zoom. Positive infinity lets each tile source use its own
    /// maximum zoom.
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
/// See `mln_open_gl_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct OpenGlBorrowedTexture {
    /// Borrowed OpenGL texture object name. Required.
    pub texture: u32,
}
impl OpenGlBorrowedTexture {
    pub const fn new(texture: u32) -> Self {
        Self { texture }
    }
}
impl ToNative<sys::mln_open_gl_borrowed_texture> for OpenGlBorrowedTexture {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_open_gl_borrowed_texture> {
        let mut raw: sys::mln_open_gl_borrowed_texture = unsafe { std::mem::zeroed() };
        raw.texture = self.texture;
        Ok(raw)
    }
}

/// OpenGL attachment options for a borrowed texture target.
///
/// See `mln_open_gl_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct OpenGlBorrowedTextureDescriptor {
    /// Logical texture extent. Required. physical_width and physical_height
    /// give the texture's size. A scale_factor that differs from the map's is
    /// accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Physical texture width in device pixels. Required.
    pub physical_width: u32,
    /// Physical texture height in device pixels. Required.
    pub physical_height: u32,
    /// Borrowed OpenGL context provider data. The textures must belong to this
    /// context or a context in the same share group.
    pub context: OpenGlContextDescriptor,
    /// The ring's textures, one per slot, in slot order. Required.
    pub textures: Vec<OpenGlBorrowedTexture>,
    /// OpenGL texture target of every texture. Required: GL_TEXTURE_2D.
    pub target: u32,
}
impl OpenGlBorrowedTextureDescriptor {
    pub fn new(
        extent: LogicalExtent,
        physical_width: u32,
        physical_height: u32,
        context: OpenGlContextDescriptor,
        textures: Vec<OpenGlBorrowedTexture>,
        target: u32,
    ) -> Self {
        Self {
            extent,
            physical_width,
            physical_height,
            context,
            textures,
            target,
        }
    }
}
impl ToNative<sys::mln_open_gl_borrowed_texture_descriptor> for OpenGlBorrowedTextureDescriptor {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_open_gl_borrowed_texture_descriptor> {
        let mut raw: sys::mln_open_gl_borrowed_texture_descriptor = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_open_gl_borrowed_texture_descriptor>() as _;
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

native_enum! {
/// OpenGL client API a dedicated EGL session creates its context for.
///
/// See `mln_open_gl_client_api` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub enum OpenGlClientApi: u32 {
    /// No client API is named.
    Unspecified = 0,
    /// Desktop OpenGL, as EGL_OPENGL_API names it.
    Gl = 1,
    /// OpenGL ES, as EGL_OPENGL_ES_API names it.
    Gles = 2,
} Unknown
}
impl Default for OpenGlClientApi {
    fn default() -> Self {
        Self::Unspecified
    }
}

/// OpenGL backend context fields shared by OpenGL render targets.
///
/// See `mln_open_gl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct OpenGlContextDescriptor {
    /// Whether the session shares its driver thread and graphics objects with
    /// the host. A private EGL or WGL owned texture and a transferred WebGL
    /// canvas are dedicated to their core worker.
    pub ownership: OpenGlContextOwnership,
    pub data: OpenGlContextDescriptorData,
}
impl OpenGlContextDescriptor {
    pub fn new(data: OpenGlContextDescriptorData) -> Self {
        Self {
            ownership: Default::default(),
            data,
        }
    }
}
impl ToNative<sys::mln_open_gl_context_descriptor> for OpenGlContextDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_open_gl_context_descriptor> {
        let mut raw: sys::mln_open_gl_context_descriptor = unsafe { std::mem::zeroed() };
        raw.ownership = to_native(&self.ownership, arena)?;
        match &self.data {
            OpenGlContextDescriptorData::Wgl(item) => {
                raw.platform = sys::MLN_OPEN_GL_CONTEXT_PLATFORM_WGL;
                raw.data.wgl = to_native(&(*item), arena)?;
            }
            OpenGlContextDescriptorData::Egl(item) => {
                raw.platform = sys::MLN_OPEN_GL_CONTEXT_PLATFORM_EGL;
                raw.data.egl = to_native(&(*item), arena)?;
            }
            OpenGlContextDescriptorData::WebGl(item) => {
                raw.platform = sys::MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL;
                raw.data.web_gl = to_native(&(*item), arena)?;
            }
        }
        Ok(raw)
    }
}

/// Backend-specific OpenGL context data.
///
/// See `mln_open_gl_context_descriptor_data` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub enum OpenGlContextDescriptorData {
    Wgl(WglContextDescriptor),
    Egl(EglContextDescriptor),
    WebGl(WebGlContextDescriptor),
}

native_enum! {
/// How a session's OpenGL context relates to its driver thread and host
/// graphics state.
///
/// See `mln_open_gl_context_ownership` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub enum OpenGlContextOwnership: u32 {
    /// The session shares its thread with host graphics work.
    Shared = 0,
    /// The session keeps its own OpenGL context current on its thread.
    Dedicated = 1,
} Unknown
}
impl Default for OpenGlContextOwnership {
    fn default() -> Self {
        Self::Shared
    }
}

native_flags! {
/// OpenGL context providers supported by this build.
///
/// See `mln_open_gl_context_provider_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub struct OpenGlContextProviderFlag: u32 {
    const WGL = 1;
    const EGL = 2;
    /// Browser WebGL context imported into an Emscripten module.
    const WEB_GL = 4;
}
}

/// OpenGL attachment options for an owned texture target.
///
/// See `mln_open_gl_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct OpenGlOwnedTextureDescriptor {
    /// Logical texture extent. Required. A scale_factor that differs from the
    /// map's is accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Borrowed OpenGL context provider data. Shared ownership creates a
    /// context whose texture frames the host can acquire. Dedicated EGL, WGL,
    /// or transferred WebGL ownership creates a private core-worker context for
    /// CPU readback.
    pub context: OpenGlContextDescriptor,
}
impl OpenGlOwnedTextureDescriptor {
    pub fn new(extent: LogicalExtent, context: OpenGlContextDescriptor) -> Self {
        Self { extent, context }
    }
}
impl ToNative<sys::mln_open_gl_owned_texture_descriptor> for OpenGlOwnedTextureDescriptor {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_open_gl_owned_texture_descriptor> {
        let mut raw: sys::mln_open_gl_owned_texture_descriptor = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_open_gl_owned_texture_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.context = to_native(&self.context, arena)?;
        Ok(raw)
    }
}

/// OpenGL attachment options for a native surface.
///
/// See `mln_open_gl_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct OpenGlSurfaceDescriptor {
    /// Logical surface extent. Required. A scale_factor that differs from the
    /// map's is accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Borrowed OpenGL context provider data.
    pub context: OpenGlContextDescriptor,
    /// Borrowed platform surface handle: an HDC for WGL and an EGLSurface for
    /// EGL, both required. Null for WebGL, whose context carries its canvas
    /// binding.
    pub surface: *mut std::ffi::c_void,
}
impl OpenGlSurfaceDescriptor {
    pub fn new(extent: LogicalExtent, context: OpenGlContextDescriptor) -> Self {
        Self {
            extent,
            context,
            surface: Default::default(),
        }
    }
}
impl ToNative<sys::mln_open_gl_surface_descriptor> for OpenGlSurfaceDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_open_gl_surface_descriptor> {
        let mut raw: sys::mln_open_gl_surface_descriptor = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_open_gl_surface_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.context = to_native(&self.context, arena)?;
        raw.surface = self.surface;
        Ok(raw)
    }
}

/// OpenGL frame acquired from a texture ring.
///
/// See `mln_open_gl_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct OpenGlTextureFrame {
    /// Session generation that produced this frame.
    pub generation: u64,
    /// Physical OpenGL texture width in device pixels.
    pub width: u32,
    /// Physical OpenGL texture height in device pixels.
    pub height: u32,
    /// UI-to-device pixel scale used for this frame.
    pub scale_factor: f64,
    /// Opaque identity of this frame.
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
impl FromNative<sys::mln_open_gl_texture_frame> for OpenGlTextureFrame {
    unsafe fn from_native(raw: sys::mln_open_gl_texture_frame) -> Result<Self> {
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
    /// Image width in pixels. Required.
    pub width: u32,
    /// Image height in pixels. Required.
    pub height: u32,
    /// Bytes per image row. Required, and at least width \* 4.
    pub stride: u32,
    /// Premultiplied RGBA8 pixels. Required.
    pub pixels: Vec<u8>,
}
impl PremultipliedRgba8Image {
    pub fn new(width: u32, height: u32, stride: u32, pixels: Vec<u8>) -> Self {
        Self {
            width,
            height,
            stride,
            pixels,
        }
    }
}
impl ToNative<sys::mln_premultiplied_rgba8_image> for PremultipliedRgba8Image {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_premultiplied_rgba8_image> {
        let mut raw: sys::mln_premultiplied_rgba8_image = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_premultiplied_rgba8_image>() as _;
        raw.width = self.width;
        raw.height = self.height;
        raw.stride = self.stride;
        raw.pixels = self.pixels.as_ptr().cast();
        raw.byte_length = convert::count(self.pixels.len())?;
        Ok(raw)
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
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct ProjectionMode {
    /// Enables a non-perspective axonometric render transform.
    pub axonometric: Option<bool>,
    /// X-skew factor of the axonometric transform.
    pub x_skew: Option<f64>,
    /// Y-skew factor of the axonometric transform.
    pub y_skew: Option<f64>,
}
impl ToNative<sys::mln_projection_mode> for ProjectionMode {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_projection_mode> {
        let mut raw: sys::mln_projection_mode = unsafe { std::mem::zeroed() };
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

/// Host lock on the graphics queue that a session shares with its host, copied
/// by a successful attach.
///
/// See `mln_queue_lock` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Clone, Default)]
pub struct QueueLock {
    pub lock: Option<std::sync::Arc<dyn Fn() + Send + Sync + 'static>>,
    pub unlock: Option<std::sync::Arc<dyn Fn() + Send + Sync + 'static>>,
}
impl std::fmt::Debug for QueueLock {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("QueueLock").finish_non_exhaustive()
    }
}
impl QueueLock {
    pub fn with_lock<F>(mut self, callback: F) -> Self
    where
        F: Fn() + Send + Sync + 'static,
    {
        self.lock = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn() + Send + Sync + 'static,
    {
        Self::default().with_lock(callback)
    }
    pub fn with_unlock<F>(mut self, callback: F) -> Self
    where
        F: Fn() + Send + Sync + 'static,
    {
        self.unlock = Some(std::sync::Arc::new(callback));
        self
    }
    unsafe extern "C" fn lock_trampoline(user_data: *mut std::ffi::c_void) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_queue_lock_callback", (), || {
            callback::require(&state.lock)?();
            Ok(())
        })
    }
    unsafe extern "C" fn unlock_trampoline(user_data: *mut std::ffi::c_void) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_queue_lock_callback", (), || {
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
impl Default for RenderAbandonDisposition {
    fn default() -> Self {
        Self::Clean
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RenderAbandonResult {
    pub disposition: RenderAbandonDisposition,
    /// Backend resource groups intentionally retained until process exit.
    pub quarantined_resource_count: u32,
}
impl FromNative<sys::mln_render_abandon_result> for RenderAbandonResult {
    unsafe fn from_native(raw: sys::mln_render_abandon_result) -> Result<Self> {
        Ok(Self {
            disposition: RenderAbandonDisposition::from_raw(raw.disposition),
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
    const OPEN_GL = 4;
    const WEB_GPU = 8;
}
}

native_enum! {
/// Execution placement for one render session.
///
/// See `mln_render_driver_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub enum RenderDriverKind: u32 {
    /// Native code initializes, services, and tears down transferable graphics
    /// state on a library-owned thread.
    CoreWorker = 1,
    /// The host services driver work on the thread or realm where its graphics
    /// context is current.
    CallerGraphicsThread = 2,
} Unknown
}

/// Terminal result of one frame demand, held by an owned frame-result batch and
/// copied by `mln_acquired_frame_get_result()`.
///
/// See `mln_render_frame_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RenderFrameResult {
    pub disposition: RenderResult,
    pub token: u64,
    /// Generation of the map render update the demand evaluated. When
    /// disposition is `MLN_RENDER_RESULT_RENDERED`, the frame drew that update;
    /// compare it with `mln_completion_result.render_update_generation` to find
    /// the first frame that includes a command.
    pub map_update_generation: u64,
    /// The session's target generation when the demand reached its result. See
    /// `mln_render_session_snapshot.generation`.
    pub generation: u64,
    /// Position of the frame among the frames that the session rendered,
    /// counting from one. Zero unless disposition is
    /// `MLN_RENDER_RESULT_RENDERED`.
    pub frame_generation: u64,
    /// Whether the map asked for another frame while it rendered this one, as
    /// during an ongoing paint transition. Set only when disposition is
    /// `MLN_RENDER_RESULT_RENDERED`, and false for every other outcome. It
    /// matches the needs_repaint field of
    /// `MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED`. A camera transition does
    /// not set it: the map publishes an update for each of the transition's
    /// frames instead, which a render-if-needed demand renders.
    pub needs_repaint: bool,
    /// The view that this frame drew, from the transform of the map update that
    /// it rendered. A host that displays the frame builds its projection with
    /// `mln_map_projection_create()` to place overlays where the frame drew
    /// their coordinates. Zero, with a size of zero, unless disposition is
    /// `MLN_RENDER_RESULT_RENDERED`.
    pub view: MapView,
}
impl FromNative<sys::mln_render_frame_result> for RenderFrameResult {
    unsafe fn from_native(raw: sys::mln_render_frame_result) -> Result<Self> {
        Ok(Self {
            disposition: RenderResult::from_raw(raw.disposition),
            token: raw.token,
            map_update_generation: raw.map_update_generation,
            generation: raw.generation,
            frame_generation: raw.frame_generation,
            needs_repaint: raw.needs_repaint,
            view: unsafe { from_native(raw.view) }?,
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
impl Default for RenderMode {
    fn default() -> Self {
        Self::Partial
    }
}

native_enum! {
/// Terminal disposition of one accepted frame demand.
///
/// See `mln_render_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
pub enum RenderResult: u32 {
    /// A frame was rendered for acquisition, presentation, or ordered readback.
    Rendered = 0,
    /// No newer map update was available, the map had no complete frame to
    /// draw, or the update extent differed from the target. The map publishes
    /// another update when it has one. A demand with
    /// `MLN_FRAME_DEMAND_WAIT_FOR_UPDATE` waits for that update instead, and
    /// finishes with this result only when a barrier ends its wait.
    NoUpdate = 1,
    /// The target could not produce a frame. The attempt consumes nothing, so a
    /// later demand with the same flags renders what this one would have. This
    /// result does not cause a map update, so the host demands again when the
    /// target can be ready, such as after a paced delay.
    TargetNotReady = 2,
    /// A newer demand with the same flags, neither of them forced, replaced
    /// this demand. A still image's demand is never replaced, and never
    /// replaces one.
    Superseded = 3,
} Unknown
}
impl Default for RenderResult {
    fn default() -> Self {
        Self::Rendered
    }
}

/// Common attachment policy copied before an attach call returns.
///
/// See `mln_render_session_attach_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone)]
pub struct RenderSessionAttachOptions {
    /// Where the session's driver runs. Required.
    pub driver: RenderDriverKind,
    /// Requested slot count of a session-owned texture ring, up to
    /// `MLN_TEXTURE_RING_MAX_DEPTH`. Zero selects 2, so the host can retain one
    /// frame while the session renders its replacement. Private targets grant
    /// one slot regardless of this value. A borrowed texture ring's depth is
    /// its texture count, so borrowed and other targets ignore this value.
    pub requested_texture_ring_depth: u32,
    /// Producer synchronization for acquired texture frames.
    /// `MLN_GPU_SYNC_OPEN_GL_FENCE` requires a shared EGL or WGL texture target
    /// and a caller-graphics-thread driver. Other targets support only CPU
    /// completion and fail an unsupported request.
    pub producer_sync_kind: GpuSyncKind,
    /// Wakes the receiver when the frame-result queue becomes nonempty.
    pub frame_wake: Wake,
    /// Wakes the graphics receiver when caller-driver work is available,
    /// including work that a finished driver result releases.
    pub driver_work_wake: Wake,
    /// Host lock on the graphics queue, disabled by default. Only Vulkan
    /// targets accept an enabled lock; other backends fail the attach with
    /// `MLN_STATUS_UNSUPPORTED`.
    pub queue_lock: QueueLock,
}
impl RenderSessionAttachOptions {
    pub fn new(driver: RenderDriverKind) -> Self {
        Self {
            driver,
            requested_texture_ring_depth: Default::default(),
            producer_sync_kind: Default::default(),
            frame_wake: Default::default(),
            driver_work_wake: Default::default(),
            queue_lock: Default::default(),
        }
    }
}
impl ToNative<sys::mln_render_session_attach_options> for RenderSessionAttachOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_render_session_attach_options> {
        let mut raw: sys::mln_render_session_attach_options = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_render_session_attach_options>() as _;
        raw.driver = to_native(&self.driver, arena)?;
        raw.requested_texture_ring_depth = self.requested_texture_ring_depth;
        raw.producer_sync_kind = to_native(&self.producer_sync_kind, arena)?;
        raw.frame_wake = to_native(&self.frame_wake, arena)?;
        raw.driver_work_wake = to_native(&self.driver_work_wake, arena)?;
        raw.queue_lock = to_native(&self.queue_lock, arena)?;
        Ok(raw)
    }
}

/// Driver and target capabilities fixed for one attached render session.
///
/// See `mln_render_session_capabilities` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct RenderSessionCapabilities {
    pub driver: RenderDriverKind,
    /// Granted texture ring depth: the slot count of a session-owned ring, or
    /// the texture count of a borrowed one. Zero for a surface.
    pub texture_ring_depth: u32,
    pub flags: RenderSessionCapabilityFlag,
}
impl FromNative<sys::mln_render_session_capabilities> for RenderSessionCapabilities {
    unsafe fn from_native(raw: sys::mln_render_session_capabilities) -> Result<Self> {
        Ok(Self {
            driver: RenderDriverKind::from_raw(raw.driver),
            texture_ring_depth: raw.texture_ring_depth,
            flags: RenderSessionCapabilityFlag::from_bits_retain(raw.flags),
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
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct RenderSessionSnapshot {
    pub state: RenderSessionState,
    /// Logical extent applied by the driver.
    pub extent: LogicalExtent,
    /// Increases when the session's lifecycle state, target, or applied extent
    /// changes. An acquired texture frame reports the value it rendered under.
    pub generation: u64,
    /// Generation of the latest map render update that the session has seen.
    pub map_update_generation: u64,
    /// Map render update generation that the latest rendered frame drew. Zero
    /// before the first frame after attachment or a target replacement.
    pub rendered_update_generation: u64,
    /// Frame generation of the latest rendered frame, zero before the first.
    pub frame_generation: u64,
    /// Acquired frames that the host has not released or discarded.
    pub acquired_frame_count: u32,
    /// True when a map update or an accepted resize arrived after the latest
    /// rendered frame.
    pub pending_changes: bool,
}
impl FromNative<sys::mln_render_session_snapshot> for RenderSessionSnapshot {
    unsafe fn from_native(raw: sys::mln_render_session_snapshot) -> Result<Self> {
        Ok(Self {
            state: RenderSessionState::from_raw(raw.state),
            extent: unsafe { from_native(raw.extent) }?,
            generation: raw.generation,
            map_update_generation: raw.map_update_generation,
            rendered_update_generation: raw.rendered_update_generation,
            frame_generation: raw.frame_generation,
            acquired_frame_count: raw.acquired_frame_count,
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
    /// A frame or driver task failed and the session can no longer render. The
    /// map reports the failure as `MLN_RUNTIME_EVENT_MAP_RENDER_ERROR`. A
    /// failed attach reports it through its completion instead.
    TargetLost = 5,
    Abandoned = 6,
} Unknown
}

/// One rendered-feature query hit borrowed for a completion callback.
///
/// See `mln_rendered_feature` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct RenderedFeature {
    pub feature: String,
    pub layer_id: String,
    pub source_id: String,
    pub source_layer_id: String,
    pub state: String,
}
impl FromNative<sys::mln_rendered_feature> for RenderedFeature {
    unsafe fn from_native(raw: sys::mln_rendered_feature) -> Result<Self> {
        Ok(Self {
            feature: unsafe { from_native(raw.feature) }?,
            layer_id: unsafe { from_native(raw.layer_id) }?,
            source_id: unsafe { from_native(raw.source_id) }?,
            source_layer_id: unsafe { from_native(raw.source_layer_id) }?,
            state: unsafe { from_native(raw.state) }?,
        })
    }
}

/// Options for rendered feature queries.
///
/// See `mln_rendered_feature_query_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct RenderedFeatureQueryOptions {
    /// Optional style layer IDs. When absent, all rendered layers are queried.
    pub layer_ids: Option<Vec<String>>,
    /// UTF-8 MapLibre style-spec filter JSON, or empty for no filter.
    pub filter: String,
}
impl ToNative<sys::mln_rendered_feature_query_options> for RenderedFeatureQueryOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_rendered_feature_query_options> {
        let mut raw: sys::mln_rendered_feature_query_options = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_rendered_feature_query_options>() as _;
        raw.fields = 0;
        if let Some(item) = &self.layer_ids {
            raw.fields |= sys::MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
            raw.layer_ids = convert::array(&*item, arena)?;
        }
        raw.layer_id_count =
            convert::count(self.layer_ids.as_ref().map_or(0, |items| items.len()))?;
        raw.filter = to_native(&self.filter, arena)?;
        Ok(raw)
    }
}

/// Rendered feature query geometry descriptor.
///
/// See `mln_rendered_query_geometry` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub enum RenderedQueryGeometry {
    Point(ScreenPoint),
    Box(ScreenBox),
    LineString(ScreenLineString),
}
impl ToNative<sys::mln_rendered_query_geometry> for RenderedQueryGeometry {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_rendered_query_geometry> {
        let mut raw: sys::mln_rendered_query_geometry = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_rendered_query_geometry>() as _;
        match self {
            RenderedQueryGeometry::Point(item) => {
                raw.type_ = sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT;
                raw.data.point = to_native(&(*item), arena)?;
            }
            RenderedQueryGeometry::Box(item) => {
                raw.type_ = sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX;
                raw.data.box_ = to_native(&(*item), arena)?;
            }
            RenderedQueryGeometry::LineString(item) => {
                raw.type_ = sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING;
                raw.data.line_string = to_native(&(*item), arena)?;
            }
        }
        Ok(raw)
    }
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
impl Default for ResourceErrorReason {
    fn default() -> Self {
        Self::None
    }
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
impl Default for ResourceKind {
    fn default() -> Self {
        Self::Unknown
    }
}

native_flags! {
/// The resource kinds that a route or rule matches, one bit per
/// `mln_resource_kind` value. Registration rejects unknown bits.
/// `MLN_RESOURCE_KIND_FLAG_ALL`, like zero, selects every resource kind.
///
/// See `mln_resource_kind_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub struct ResourceKindFlag: u32 {
    const UNKNOWN = 1;
    const STYLE = 2;
    const SOURCE = 4;
    const TILE = 8;
    const GLYPHS = 16;
    const SPRITE_IMAGE = 32;
    const SPRITE_JSON = 64;
    const IMAGE = 128;
    const ALL = 255;
}
}

native_enum! {
pub enum ResourceLoadingMethod: u32 {
    All = 0,
    CacheOnly = 1,
    NetworkOnly = 2,
} Unknown
}
impl Default for ResourceLoadingMethod {
    fn default() -> Self {
        Self::All
    }
}

native_enum! {
pub enum ResourcePriority: u32 {
    Regular = 0,
    Low = 1,
} Unknown
}
impl Default for ResourcePriority {
    fn default() -> Self {
        Self::Regular
    }
}

/// Resource provider state.
///
/// See `mln_resource_provider` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Clone)]
pub struct ResourceProvider {
    /// Answers each request that the routes leave to the provider.
    ///
    /// The callback receives (`request`, `handle`).
    pub callback:
        std::sync::Arc<dyn Fn(ResourceRequestInfo, ResourceRequestHandle) + Send + Sync + 'static>,
    /// The requests that reach the provider; empty to claim every request.
    pub routes: Vec<ResourceRoute>,
}
impl std::fmt::Debug for ResourceProvider {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("ResourceProvider").finish_non_exhaustive()
    }
}
impl ResourceProvider {
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(ResourceRequestInfo, ResourceRequestHandle) + Send + Sync + 'static,
    {
        Self {
            callback: std::sync::Arc::new(callback),
            routes: Default::default(),
        }
    }
    unsafe extern "C" fn callback_trampoline(
        user_data: *mut std::ffi::c_void,
        request: *const sys::mln_resource_request_info,
        handle: sys::mln_resource_request,
    ) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_resource_provider_callback", (), || {
            let handle = ResourceRequestHandle::adopt(handle, None)?;
            (state.callback)(unsafe { convert::copy_reference(request) }?, handle);
            Ok(())
        })
    }
}
impl ToNative<sys::mln_resource_provider> for ResourceProvider {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_resource_provider> {
        let mut raw: sys::mln_resource_provider = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_resource_provider>() as _;
        raw.callback = Some(Self::callback_trampoline as _);
        raw.routes = convert::array(&self.routes, arena)?;
        raw.route_count = convert::count(self.routes.len())?;
        if !(raw.callback.is_none()) {
            // SAFETY: the release reclaims exactly this state.
            raw.user_data = unsafe { arena.registration(self.clone(), callback::release::<Self>) };
            raw.release_user_data = Some(callback::release::<Self>);
        }
        Ok(raw)
    }
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
impl FromNative<sys::mln_resource_range> for ResourceRange {
    unsafe fn from_native(raw: sys::mln_resource_range) -> Result<Self> {
        Ok(Self {
            start: raw.start,
            end: raw.end,
        })
    }
}

/// Cancel callback or queue state for one handled resource request.
///
/// See `mln_resource_request_cancel_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Clone)]
pub struct ResourceRequestCancelHandler {
    /// Required in callback mode; null in queue mode.
    pub callback: std::sync::Arc<dyn Fn() + Send + Sync + 'static>,
}
impl std::fmt::Debug for ResourceRequestCancelHandler {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("ResourceRequestCancelHandler")
            .finish_non_exhaustive()
    }
}
impl ResourceRequestCancelHandler {
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn() + Send + Sync + 'static,
    {
        Self {
            callback: std::sync::Arc::new(callback),
        }
    }
    unsafe extern "C" fn callback_trampoline(
        user_data: *mut std::ffi::c_void,
        _handle: sys::mln_resource_request,
    ) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_resource_request_cancel_callback", (), || {
            (state.callback)();
            Ok(())
        })
    }
}
impl ToNative<sys::mln_resource_request_cancel_handler> for ResourceRequestCancelHandler {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_resource_request_cancel_handler> {
        let mut raw: sys::mln_resource_request_cancel_handler = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_resource_request_cancel_handler>() as _;
        raw.callback = Some(Self::callback_trampoline as _);
        if !(raw.callback.is_none()) {
            // SAFETY: the release reclaims exactly this state.
            raw.user_data = unsafe { arena.registration(self.clone(), callback::release::<Self>) };
            raw.release_user_data = Some(callback::release::<Self>);
        }
        Ok(raw)
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct ResourceRequestInfo {
    /// URL entering the network layer, before tile server normalization.
    pub requested_url: String,
    /// URL to fetch, after resource-kind normalization against the runtime's
    /// tile server options and API key.
    pub resolved_url: String,
    pub kind: ResourceKind,
    pub loading_method: ResourceLoadingMethod,
    pub priority: ResourcePriority,
    pub usage: ResourceUsage,
    pub storage_policy: ResourceStoragePolicy,
    pub range: Option<ResourceRange>,
    pub prior_modified_unix_ms: Option<i64>,
    pub prior_expires_unix_ms: Option<i64>,
    /// Entity tag of the cached copy, or empty when there is none.
    pub prior_etag: String,
    pub prior_data: Vec<u8>,
}
impl FromNative<sys::mln_resource_request_info> for ResourceRequestInfo {
    unsafe fn from_native(raw: sys::mln_resource_request_info) -> Result<Self> {
        Ok(Self {
            requested_url: unsafe { from_native(raw.requested_url) }?,
            resolved_url: unsafe { from_native(raw.resolved_url) }?,
            kind: ResourceKind::from_raw(raw.kind),
            loading_method: ResourceLoadingMethod::from_raw(raw.loading_method),
            priority: ResourcePriority::from_raw(raw.priority),
            usage: ResourceUsage::from_raw(raw.usage),
            storage_policy: ResourceStoragePolicy::from_raw(raw.storage_policy),
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
    pub error_message: String,
    pub must_revalidate: bool,
    pub modified_unix_ms: Option<i64>,
    pub expires_unix_ms: Option<i64>,
    pub etag: String,
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
impl Default for ResourceResponseStatus {
    fn default() -> Self {
        Self::Ok
    }
}

/// One request pattern that a resource provider claims.
///
/// See `mln_resource_route` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct ResourceRoute {
    /// The resource kinds to match. Zero selects every kind.
    pub kinds: ResourceKindFlag,
    pub flags: UrlMatchFlag,
    /// The URL or glob pattern to compare.
    pub url: String,
}
impl ToNative<sys::mln_resource_route> for ResourceRoute {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_resource_route> {
        let mut raw: sys::mln_resource_route = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_resource_route>() as _;
        raw.kinds = to_native(&self.kinds, arena)?;
        raw.flags = to_native(&self.flags, arena)?;
        raw.url = to_native(&self.url, arena)?;
        Ok(raw)
    }
}

native_enum! {
pub enum ResourceStoragePolicy: u32 {
    Permanent = 0,
    Volatile = 1,
} Unknown
}
impl Default for ResourceStoragePolicy {
    fn default() -> Self {
        Self::Permanent
    }
}

/// Resource transform state.
///
/// See `mln_resource_transform` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Clone)]
pub struct ResourceTransform {
    /// Required in callback mode; null in queue mode.
    ///
    /// The callback receives (`kind`, `url`, `request`).
    pub callback: std::sync::Arc<
        dyn Fn(ResourceKind, &str, ResourceTransformRequestHandle) + Send + Sync + 'static,
    >,
}
impl std::fmt::Debug for ResourceTransform {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("ResourceTransform").finish_non_exhaustive()
    }
}
impl ResourceTransform {
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(ResourceKind, &str, ResourceTransformRequestHandle) + Send + Sync + 'static,
    {
        Self {
            callback: std::sync::Arc::new(callback),
        }
    }
    unsafe extern "C" fn callback_trampoline(
        user_data: *mut std::ffi::c_void,
        kind: sys::mln_resource_kind,
        url: sys::mln_buffer_view,
        request: sys::mln_resource_transform_request,
    ) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_resource_transform_callback", (), || {
            let request = ResourceTransformRequestHandle::adopt(request, None)?;
            (state.callback)(
                ResourceKind::from_raw(kind),
                unsafe { maplibre_core::string::borrow_string_view(url) }?,
                request,
            );
            Ok(())
        })
    }
}
impl ToNative<sys::mln_resource_transform> for ResourceTransform {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_resource_transform> {
        let mut raw: sys::mln_resource_transform = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_resource_transform>() as _;
        raw.callback = Some(Self::callback_trampoline as _);
        if !(raw.callback.is_none()) {
            // SAFETY: the release reclaims exactly this state.
            raw.user_data = unsafe { arena.registration(self.clone(), callback::release::<Self>) };
            raw.release_user_data = Some(callback::release::<Self>);
        }
        Ok(raw)
    }
}

native_enum! {
pub enum ResourceUsage: u32 {
    Online = 0,
    Offline = 1,
} Unknown
}
impl Default for ResourceUsage {
    fn default() -> Self {
        Self::Online
    }
}

/// One delivered runtime event.
///
/// See `mln_runtime_event` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct RuntimeEvent {
    pub r#type: RuntimeEventType,
    pub source_type: RuntimeEventSourceType,
    /// Source handle selected by source_type: an `mln_runtime` or an `mln_map`.
    pub source: u64,
    /// Map snapshot generation that the event reports, or zero when source_type
    /// is `MLN_RUNTIME_EVENT_SOURCE_RUNTIME`.
    pub generation: u64,
    /// Secondary event detail whose meaning type selects; see
    /// `mln_runtime_event_type`.
    pub code: u32,
    /// Borrowed UTF-8 message bytes, empty when this event carries no message.
    pub message: String,
    /// Typed payload selected by payload_type.
    pub payload: RuntimeEventPayload,
}
impl FromNative<sys::mln_runtime_event> for RuntimeEvent {
    unsafe fn from_native(raw: sys::mln_runtime_event) -> Result<Self> {
        Ok(Self {
            r#type: RuntimeEventType::from_raw(raw.type_),
            source_type: RuntimeEventSourceType::from_raw(raw.source_type),
            source: raw.source,
            generation: raw.generation,
            code: raw.code,
            message: unsafe { from_native(raw.message) }?,
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
/// See `mln_runtime_event_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub struct RuntimeEventFlag: u64 {
    const MAP_CAMERA_WILL_CHANGE = 2;
    const MAP_CAMERA_IS_CHANGING = 4;
    const MAP_CAMERA_DID_CHANGE = 8;
    const MAP_STYLE_LOADED = 16;
    const MAP_LOADING_STARTED = 32;
    const MAP_LOADING_FINISHED = 64;
    const MAP_LOADING_FAILED = 128;
    const MAP_IDLE = 256;
    const MAP_RENDER_ERROR = 512;
    const MAP_RENDER_FRAME_STARTED = 1024;
    const MAP_RENDER_FRAME_FINISHED = 2048;
    const MAP_RENDER_MAP_STARTED = 4096;
    const MAP_RENDER_MAP_FINISHED = 8192;
    const MAP_STYLE_IMAGE_MISSING = 16384;
    const MAP_TILE_ACTION = 32768;
    const MAP_SOURCE_CHANGED = 65536;
    const OFFLINE_REGION_STATUS_CHANGED = 131072;
    const OFFLINE_REGION_RESPONSE_ERROR = 262144;
    const OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 524288;
    /// Selects every map-originated event type this version defines.
    const ALL_MAP_EVENTS = 131070;
    /// Selects every runtime-originated event type this version defines.
    const ALL_RUNTIME_EVENTS = 917504;
    /// Selects every event type this version defines.
    const ALL = 1048574;
}
}

/// Runtime and map event delivery. Exactly one of callback and queue is set.
///
/// See `mln_runtime_event_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Clone)]
pub struct RuntimeEventHandler {
    /// Receives each event batch.
    ///
    /// The callback receives (`events`).
    pub callback: std::sync::Arc<dyn Fn(Vec<RuntimeEvent>) + Send + Sync + 'static>,
}
impl std::fmt::Debug for RuntimeEventHandler {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("RuntimeEventHandler")
            .finish_non_exhaustive()
    }
}
impl RuntimeEventHandler {
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(Vec<RuntimeEvent>) + Send + Sync + 'static,
    {
        Self {
            callback: std::sync::Arc::new(callback),
        }
    }
    unsafe extern "C" fn callback_trampoline(
        user_data: *mut std::ffi::c_void,
        events: *const sys::mln_event_batch_view,
    ) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_runtime_event_callback", (), || {
            (state.callback)({
                let view = unsafe { events.as_ref() }
                    .ok_or_else(|| Error::invalid_argument("null record value"))?;
                unsafe { convert::copy_strided(view.events, view.event_count, view.event_size) }?
            });
            Ok(())
        })
    }
}
impl ToNative<sys::mln_runtime_event_handler> for RuntimeEventHandler {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_runtime_event_handler> {
        let mut raw: sys::mln_runtime_event_handler = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_runtime_event_handler>() as _;
        raw.callback = Some(Self::callback_trampoline as _);
        if !(raw.callback.is_none()) {
            // SAFETY: the release reclaims exactly this state.
            raw.user_data = unsafe { arena.registration(self.clone(), callback::release::<Self>) };
            raw.release_user_data = Some(callback::release::<Self>);
        }
        Ok(raw)
    }
}

/// Payload for `MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR`.
///
/// See `mln_runtime_event_offline_region_response_error` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventOfflineRegionResponseError {
    pub region_id: i64,
    pub reason: ResourceErrorReason,
}
impl FromNative<sys::mln_runtime_event_offline_region_response_error>
    for RuntimeEventOfflineRegionResponseError
{
    unsafe fn from_native(
        raw: sys::mln_runtime_event_offline_region_response_error,
    ) -> Result<Self> {
        Ok(Self {
            region_id: raw.region_id,
            reason: ResourceErrorReason::from_raw(raw.reason),
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

/// Payload for `MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED`.
///
/// See `mln_runtime_event_render_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventRenderFrame {
    pub mode: RenderMode,
    /// Whether MapLibre needs another frame after this one.
    pub needs_repaint: bool,
    /// Whether symbol placement changed during this frame.
    pub placement_changed: bool,
    pub stats: RenderingStats,
}
impl FromNative<sys::mln_runtime_event_render_frame> for RuntimeEventRenderFrame {
    unsafe fn from_native(raw: sys::mln_runtime_event_render_frame) -> Result<Self> {
        Ok(Self {
            mode: RenderMode::from_raw(raw.mode),
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
    pub mode: RenderMode,
}
impl FromNative<sys::mln_runtime_event_render_map> for RuntimeEventRenderMap {
    unsafe fn from_native(raw: sys::mln_runtime_event_render_map) -> Result<Self> {
        Ok(Self {
            mode: RenderMode::from_raw(raw.mode),
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
impl Default for RuntimeEventSourceType {
    fn default() -> Self {
        Self::Runtime
    }
}

/// Payload for `MLN_RUNTIME_EVENT_MAP_TILE_ACTION`.
///
/// See `mln_runtime_event_tile_action` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventTileAction {
    pub operation: TileOperation,
    pub tile_id: TileId,
}
impl FromNative<sys::mln_runtime_event_tile_action> for RuntimeEventTileAction {
    unsafe fn from_native(raw: sys::mln_runtime_event_tile_action) -> Result<Self> {
        Ok(Self {
            operation: TileOperation::from_raw(raw.operation),
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
    MapRenderError = 9,
    MapRenderFrameStarted = 10,
    MapRenderFrameFinished = 11,
    MapRenderMapStarted = 12,
    MapRenderMapFinished = 13,
    MapStyleImageMissing = 14,
    MapTileAction = 15,
    MapSourceChanged = 16,
    OfflineRegionStatusChanged = 17,
    OfflineRegionResponseError = 18,
    OfflineRegionTileCountLimitExceeded = 19,
} Unknown
}

/// Options used when creating a runtime.
///
/// See `mln_runtime_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct RuntimeOptions {
    /// Directory root for asset:// URLs. Copied during runtime creation. Empty
    /// selects `/android_asset` on Android and `.` elsewhere.
    pub asset_path: String,
    /// Cache database path. Copied during runtime creation. Empty selects an
    /// in-memory cache.
    pub cache_path: String,
}
impl ToNative<sys::mln_runtime_options> for RuntimeOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_runtime_options> {
        let mut raw: sys::mln_runtime_options = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_runtime_options>() as _;
        raw.asset_path = to_native(&self.asset_path, arena)?;
        raw.cache_path = to_native(&self.cache_path, arena)?;
        Ok(raw)
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

/// Screen-space point in logical map pixels, from the top-left of the viewport
/// with y growing downward.
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

/// One source-feature query hit borrowed for a completion callback.
///
/// See `mln_source_feature` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct SourceFeature {
    pub feature: String,
    pub source_layer_id: String,
}
impl FromNative<sys::mln_source_feature> for SourceFeature {
    unsafe fn from_native(raw: sys::mln_source_feature) -> Result<Self> {
        Ok(Self {
            feature: unsafe { from_native(raw.feature) }?,
            source_layer_id: unsafe { from_native(raw.source_layer_id) }?,
        })
    }
}

/// Options for source feature queries.
///
/// See `mln_source_feature_query_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct SourceFeatureQueryOptions {
    /// Optional source-layer IDs. Required by vector sources; ignored by
    /// GeoJSON.
    pub source_layer_ids: Option<Vec<String>>,
    /// UTF-8 MapLibre style-spec filter JSON, or empty for no filter.
    pub filter: String,
}
impl ToNative<sys::mln_source_feature_query_options> for SourceFeatureQueryOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_source_feature_query_options> {
        let mut raw: sys::mln_source_feature_query_options = unsafe { std::mem::zeroed() };
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
        raw.filter = to_native(&self.filter, arena)?;
        Ok(raw)
    }
}

pub use maplibre_core::Status;
impl ToNative<i32> for Status {
    fn to_native(&self, _: &mut InputArena) -> Result<i32> {
        Ok(self.as_raw())
    }
}
impl FromNative<i32> for Status {
    unsafe fn from_native(raw: i32) -> Result<Self> {
        Ok(Self::from_raw(raw))
    }
}

/// What a parsed expression depends on.
///
/// See `mln_style_expression_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct StyleExpressionInfo {
    /// Whether the result is the same for every feature and feature state.
    pub is_feature_constant: bool,
    /// Whether the result is the same at every zoom.
    pub is_zoom_constant: bool,
}
impl FromNative<sys::mln_style_expression_info> for StyleExpressionInfo {
    unsafe fn from_native(raw: sys::mln_style_expression_info) -> Result<Self> {
        Ok(Self {
            is_feature_constant: raw.is_feature_constant,
            is_zoom_constant: raw.is_zoom_constant,
        })
    }
}

/// What one evaluation reads.
///
/// See `mln_style_expression_inputs` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleExpressionInputs {
    /// The value of the zoom expression.
    pub zoom: Option<f64>,
    /// The value of the heatmap-density and line-progress expressions, and of
    /// the elevation expression in a color-relief layer.
    pub color_ramp_parameter: Option<f64>,
    /// The value of the elevation expression outside a color-relief layer.
    pub elevation: Option<f64>,
    /// One UTF-8 GeoJSON Feature that feature data expressions read.
    pub feature: String,
    /// A UTF-8 JSON object that the feature-state expression reads.
    pub feature_state: String,
    /// A UTF-8 JSON object that the global-state expression reads.
    pub global_state: String,
    /// The tile that holds the feature, which the within and distance
    /// expressions need to place its geometry.
    pub canonical_tile_id: Option<CanonicalTileId>,
    /// The image IDs that the image expression reports as available. The
    /// expression reports any other ID as unavailable.
    pub available_images: Vec<String>,
}
impl ToNative<sys::mln_style_expression_inputs> for StyleExpressionInputs {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_style_expression_inputs> {
        let mut raw: sys::mln_style_expression_inputs = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_style_expression_inputs>() as _;
        raw.fields = 0;
        if let Some(item) = &self.zoom {
            raw.fields |= sys::MLN_STYLE_EXPRESSION_INPUT_ZOOM;
            raw.zoom = *item;
        }
        if let Some(item) = &self.color_ramp_parameter {
            raw.fields |= sys::MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER;
            raw.color_ramp_parameter = *item;
        }
        if let Some(item) = &self.elevation {
            raw.fields |= sys::MLN_STYLE_EXPRESSION_INPUT_ELEVATION;
            raw.elevation = *item;
        }
        raw.feature = to_native(&self.feature, arena)?;
        raw.feature_state = to_native(&self.feature_state, arena)?;
        raw.global_state = to_native(&self.global_state, arena)?;
        if let Some(item) = &self.canonical_tile_id {
            raw.fields |= sys::MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID;
            raw.canonical_tile_id = to_native(&*item, arena)?;
        }
        raw.available_images = convert::array(&self.available_images, arena)?;
        raw.available_image_count = convert::count(self.available_images.len())?;
        Ok(raw)
    }
}

/// Options for parsing a style expression.
///
/// See `mln_style_expression_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct StyleExpressionOptions {
    /// The type that the expression must produce. A string type coerces the
    /// result to a string, as a string layer property does; every other type
    /// asserts it. Without it, the expression keeps the type it infers.
    pub expected_type: Option<StyleExpressionType>,
}
impl ToNative<sys::mln_style_expression_options> for StyleExpressionOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_style_expression_options> {
        let mut raw: sys::mln_style_expression_options = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_style_expression_options>() as _;
        raw.fields = 0;
        if let Some(item) = &self.expected_type {
            raw.fields |= sys::MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE;
            raw.expected_type = to_native(&*item, arena)?;
        }
        Ok(raw)
    }
}

/// A style expression type.
///
/// See `mln_style_expression_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct StyleExpressionType {
    pub kind: StyleExpressionTypeKind,
    /// For an array, its items' kind; ignored for any other kind.
    pub item_kind: StyleExpressionTypeKind,
    /// For an array, its fixed length, or zero for any length.
    pub item_count: u32,
}
impl ToNative<sys::mln_style_expression_type> for StyleExpressionType {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_style_expression_type> {
        let mut raw: sys::mln_style_expression_type = unsafe { std::mem::zeroed() };
        raw.kind = to_native(&self.kind, arena)?;
        raw.item_kind = to_native(&self.item_kind, arena)?;
        raw.item_count = self.item_count;
        Ok(raw)
    }
}

native_enum! {
/// Style expression types that a parsed expression may be required to have.
///
/// See `mln_style_expression_type_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
pub enum StyleExpressionTypeKind: u32 {
    /// Any value.
    Value = 0,
    Number = 1,
    String = 2,
    Boolean = 3,
    Color = 4,
    Object = 5,
    Formatted = 6,
    /// A resolved image, as the image expression returns.
    Image = 7,
    Padding = 8,
    VariableAnchorOffsetCollection = 9,
    /// An array whose items have item_kind, which is not itself an array.
    Array = 10,
} Unknown
}
impl Default for StyleExpressionTypeKind {
    fn default() -> Self {
        Self::Value
    }
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
            text_fit_width: if raw.fields & sys::MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH != 0 {
                Some(StyleImageTextFit::from_raw(raw.text_fit_width))
            } else {
                None
            },
            text_fit_height: if raw.fields & sys::MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT != 0 {
                Some(StyleImageTextFit::from_raw(raw.text_fit_height))
            } else {
                None
            },
            pixel_ratio: raw.pixel_ratio,
            sdf: raw.sdf,
        })
    }
}

/// Options for runtime style images.
///
/// See `mln_style_image_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleImageOptions {
    /// Horizontally stretchable intervals. Borrowed for the call and copied
    /// before return. May be null only when stretch_x_count is 0.
    pub stretch_x: Option<Vec<ImageStretch>>,
    /// Vertically stretchable intervals. Borrowed for the call and copied
    /// before return. May be null only when stretch_y_count is 0.
    pub stretch_y: Option<Vec<ImageStretch>>,
    /// Content box used when icon-text-fit applies.
    pub content: Option<ImageContent>,
    /// Absent selects STRETCH_OR_SHRINK.
    pub text_fit_width: Option<StyleImageTextFit>,
    /// Absent selects STRETCH_OR_SHRINK.
    pub text_fit_height: Option<StyleImageTextFit>,
    /// Sprite pixel ratio. Absent selects 1.
    pub pixel_ratio: Option<f32>,
    /// Whether the image is a signed distance field icon. Absent selects false.
    pub sdf: Option<bool>,
}
impl ToNative<sys::mln_style_image_options> for StyleImageOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_style_image_options> {
        let mut raw: sys::mln_style_image_options = unsafe { std::mem::zeroed() };
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
impl Default for StyleImageTextFit {
    fn default() -> Self {
        Self::StretchOrShrink
    }
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
    pub source_id: String,
    /// Source-layer ID. Empty when the layer sets none.
    pub source_layer: String,
    /// Lowest zoom at which the layer draws; -INFINITY with no lower bound.
    pub min_zoom: f64,
    /// Highest zoom at which the layer draws; INFINITY with no upper bound.
    pub max_zoom: f64,
    pub visibility: StyleLayerVisibility,
}
impl FromNative<sys::mln_style_layer_info> for StyleLayerInfo {
    unsafe fn from_native(raw: sys::mln_style_layer_info) -> Result<Self> {
        Ok(Self {
            id: unsafe { from_native(raw.id) }?,
            r#type: unsafe { from_native(raw.type_) }?,
            source_id: unsafe { from_native(raw.source_id) }?,
            source_layer: unsafe { from_native(raw.source_layer) }?,
            min_zoom: raw.min_zoom,
            max_zoom: raw.max_zoom,
            visibility: StyleLayerVisibility::from_raw(raw.visibility),
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
impl Default for StyleLayerVisibility {
    fn default() -> Self {
        Self::Visible
    }
}

native_enum! {
/// DEM raster encoding values.
///
/// See `mln_style_raster_dem_encoding` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub enum StyleRasterDemEncoding: u32 {
    Mapbox = 0,
    Terrarium = 1,
} Unknown
}
impl Default for StyleRasterDemEncoding {
    fn default() -> Self {
        Self::Mapbox
    }
}

/// Complete metadata of one style source, borrowed for a completion callback.
///
/// See `mln_style_source_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleSourceInfo {
    /// Source ID.
    pub id: String,
    pub r#type: StyleSourceType,
    /// Whether the source is marked volatile.
    pub is_volatile: bool,
    /// Attribution string, or empty when the source sets none.
    pub attribution: String,
    /// URL that the source loads from, or empty when it has none.
    pub url: String,
    /// Inline TileJSON metadata, when the source was defined with it.
    pub tile_json: Option<StyleSourceTileInfo>,
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
            r#type: StyleSourceType::from_raw(raw.type_),
            is_volatile: raw.is_volatile,
            attribution: unsafe { from_native(raw.attribution) }?,
            url: unsafe { from_native(raw.url) }?,
            tile_json: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_STYLE_SOURCE_INFO_TILE_JSON,
                    raw.tile_json,
                )
            }?,
            bounds: unsafe {
                convert::present(raw.fields, sys::MLN_STYLE_SOURCE_INFO_BOUNDS, raw.bounds)
            }?,
            tile_size: (raw.fields & sys::MLN_STYLE_SOURCE_INFO_TILE_SIZE != 0)
                .then_some(raw.tile_size),
            vector_encoding: if raw.fields & sys::MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING != 0 {
                Some(StyleVectorTileEncoding::from_raw(raw.vector_encoding))
            } else {
                None
            },
            raster_encoding: if raw.fields & sys::MLN_STYLE_SOURCE_INFO_RASTER_ENCODING != 0 {
                Some(StyleRasterDemEncoding::from_raw(raw.raster_encoding))
            } else {
                None
            },
        })
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
    pub scheme: StyleTileScheme,
}
impl FromNative<sys::mln_style_source_tile_info> for StyleSourceTileInfo {
    unsafe fn from_native(raw: sys::mln_style_source_tile_info) -> Result<Self> {
        Ok(Self {
            tile_urls: unsafe { convert::copy_array(raw.tile_urls, raw.tile_url_count) }?,
            min_zoom: raw.min_zoom,
            max_zoom: raw.max_zoom,
            scheme: StyleTileScheme::from_raw(raw.scheme),
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
    GeoJson = 4,
    Image = 5,
    Video = 6,
    Annotations = 7,
    CustomVector = 8,
    CustomMvtVector = 9,
} Unrecognized
}
impl Default for StyleSourceType {
    fn default() -> Self {
        Self::Unknown
    }
}

native_enum! {
/// Tile URL coordinate scheme values.
///
/// See `mln_style_tile_scheme` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub enum StyleTileScheme: u32 {
    Xyz = 0,
    Tms = 1,
} Unknown
}
impl Default for StyleTileScheme {
    fn default() -> Self {
        Self::Xyz
    }
}

/// Global style transition options.
///
/// See `mln_style_transition_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct StyleTransitionOptions {
    /// Transition duration in milliseconds. Must be finite, non-negative, and
    /// short enough to represent as a duration.
    pub duration_ms: Option<f64>,
    /// Transition delay in milliseconds. Must be finite, non-negative, and
    /// short enough to represent as a duration.
    pub delay_ms: Option<f64>,
    /// Whether symbol placement changes cross-fade.
    pub enable_placement_transitions: Option<bool>,
}
impl ToNative<sys::mln_style_transition_options> for StyleTransitionOptions {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_style_transition_options> {
        let mut raw: sys::mln_style_transition_options = unsafe { std::mem::zeroed() };
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
/// Vector tile encoding values.
///
/// See `mln_style_vector_tile_encoding` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub enum StyleVectorTileEncoding: u32 {
    Mvt = 0,
    Mlt = 1,
} Unknown
}
impl Default for StyleVectorTileEncoding {
    fn default() -> Self {
        Self::Mvt
    }
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
impl Default for TileLodMode {
    fn default() -> Self {
        Self::Default
    }
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
impl Default for TileOperation {
    fn default() -> Self {
        Self::RequestedFromCache
    }
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

native_flags! {
/// How a rule or route compares its url against a request URL.
///
/// See `mln_url_match_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub struct UrlMatchFlag: u32 {
    /// Reads url as a glob pattern.
    const GLOB = 1;
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
impl Default for ViewportMode {
    fn default() -> Self {
        Self::Default
    }
}

/// One caller-owned Vulkan image of a borrowed texture ring.
///
/// See `mln_vulkan_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
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

/// Vulkan attachment options for a borrowed texture target.
///
/// See `mln_vulkan_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct VulkanBorrowedTextureDescriptor {
    /// Logical texture extent. Required. physical_width and physical_height
    /// give the texture's size. A scale_factor that differs from the map's is
    /// accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Physical image width in device pixels. Required.
    pub physical_width: u32,
    /// Physical image height in device pixels. Required.
    pub physical_height: u32,
    /// Borrowed Vulkan context. All handles are required.
    pub context: VulkanContextDescriptor,
    /// The ring's images, one per slot, in slot order. Required.
    pub textures: Vec<VulkanBorrowedTexture>,
    /// Backend-native VkFormat value of every image. Required, so
    /// VK_FORMAT_UNDEFINED is invalid.
    pub format: u32,
    /// Backend-native VkImageLayout value expected at render-pass begin.
    pub initial_layout: u32,
    /// Backend-native VkImageLayout value left after rendering succeeds, such
    /// as VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL. Required, so
    /// VK_IMAGE_LAYOUT_UNDEFINED is invalid.
    pub final_layout: u32,
}
impl VulkanBorrowedTextureDescriptor {
    pub fn new(
        extent: LogicalExtent,
        physical_width: u32,
        physical_height: u32,
        context: VulkanContextDescriptor,
        textures: Vec<VulkanBorrowedTexture>,
        format: u32,
        final_layout: u32,
    ) -> Self {
        Self {
            extent,
            physical_width,
            physical_height,
            context,
            textures,
            format,
            initial_layout: Default::default(),
            final_layout,
        }
    }
}
impl ToNative<sys::mln_vulkan_borrowed_texture_descriptor> for VulkanBorrowedTextureDescriptor {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_vulkan_borrowed_texture_descriptor> {
        let mut raw: sys::mln_vulkan_borrowed_texture_descriptor = unsafe { std::mem::zeroed() };
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

/// Vulkan backend context fields shared by Vulkan render targets.
///
/// See `mln_vulkan_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
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
    /// PFN_vkGetInstanceProcAddr for the loader that created the instance.
    pub get_instance_proc_addr: *mut std::ffi::c_void,
    /// PFN_vkGetDeviceProcAddr for the loader that created the Vulkan device.
    pub get_device_proc_addr: *mut std::ffi::c_void,
}
impl VulkanContextDescriptor {
    pub fn new(
        instance: *mut std::ffi::c_void,
        physical_device: *mut std::ffi::c_void,
        device: *mut std::ffi::c_void,
        graphics_queue: *mut std::ffi::c_void,
    ) -> Self {
        Self {
            instance,
            physical_device,
            device,
            graphics_queue,
            graphics_queue_family_index: Default::default(),
            get_instance_proc_addr: Default::default(),
            get_device_proc_addr: Default::default(),
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

/// Vulkan attachment options for an owned texture target.
///
/// See `mln_vulkan_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanOwnedTextureDescriptor {
    /// Logical texture extent. Required. A scale_factor that differs from the
    /// map's is accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Borrowed Vulkan context. All handles are required.
    pub context: VulkanContextDescriptor,
}
impl VulkanOwnedTextureDescriptor {
    pub fn new(extent: LogicalExtent, context: VulkanContextDescriptor) -> Self {
        Self { extent, context }
    }
}
impl ToNative<sys::mln_vulkan_owned_texture_descriptor> for VulkanOwnedTextureDescriptor {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_vulkan_owned_texture_descriptor> {
        let mut raw: sys::mln_vulkan_owned_texture_descriptor = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_vulkan_owned_texture_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.context = to_native(&self.context, arena)?;
        Ok(raw)
    }
}

/// Vulkan attachment options for a native surface.
///
/// See `mln_vulkan_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanSurfaceDescriptor {
    /// Logical surface extent. Required. A scale_factor that differs from the
    /// map's is accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Borrowed Vulkan context. All handles are required. The device must
    /// support VK_KHR_swapchain, and the queue family must support graphics and
    /// presentation to this descriptor's surface.
    pub context: VulkanContextDescriptor,
    /// Borrowed VkSurfaceKHR bit pattern. Required.
    pub surface: u64,
}
impl VulkanSurfaceDescriptor {
    pub fn new(extent: LogicalExtent, context: VulkanContextDescriptor, surface: u64) -> Self {
        Self {
            extent,
            context,
            surface,
        }
    }
}
impl ToNative<sys::mln_vulkan_surface_descriptor> for VulkanSurfaceDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_vulkan_surface_descriptor> {
        let mut raw: sys::mln_vulkan_surface_descriptor = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_vulkan_surface_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.context = to_native(&self.context, arena)?;
        raw.surface = self.surface;
        Ok(raw)
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
    /// Opaque identity of this frame.
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

/// Receiver wake callback copied by a successful owning call. The callback
/// schedules service by the receiver and returns without waiting for another
/// thread.
///
/// See `mln_wake` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
#[derive(Clone, Default)]
pub struct Wake {
    pub callback: Option<std::sync::Arc<dyn Fn() + Send + Sync + 'static>>,
}
impl std::fmt::Debug for Wake {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Wake").finish_non_exhaustive()
    }
}
impl Wake {
    pub fn with_callback<F>(mut self, callback: F) -> Self
    where
        F: Fn() + Send + Sync + 'static,
    {
        self.callback = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn() + Send + Sync + 'static,
    {
        Self::default().with_callback(callback)
    }
    unsafe extern "C" fn callback_trampoline(user_data: *mut std::ffi::c_void) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_wake_callback", (), || {
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
/// See `mln_web_gl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct WebGlContextDescriptor {
    pub kind: WebGlContextKind,
    /// Borrowed EMSCRIPTEN_WEBGL_CONTEXT_HANDLE for EXISTING. Must be positive.
    pub context: i32,
    /// Copied UTF-8 Emscripten target selector for TRANSFERRED_CANVAS. The HTML
    /// canvas must still be transferable when attachment starts.
    pub canvas_selector: String,
}
impl ToNative<sys::mln_web_gl_context_descriptor> for WebGlContextDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_web_gl_context_descriptor> {
        let mut raw: sys::mln_web_gl_context_descriptor = unsafe { std::mem::zeroed() };
        raw.kind = to_native(&self.kind, arena)?;
        raw.context = self.context;
        raw.canvas_selector = to_native(&self.canvas_selector, arena)?;
        Ok(raw)
    }
}

native_enum! {
/// WebGL context placement.
///
/// See `mln_web_gl_context_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub enum WebGlContextKind: u32 {
    /// Use a host-created context on its current browser agent.
    Existing = 0,
    /// Transfer the canvas named by canvas_selector to a native worker and
    /// create a WebGL 2 context there.
    TransferredCanvas = 1,
} Unknown
}
impl Default for WebGlContextKind {
    fn default() -> Self {
        Self::Existing
    }
}

/// One caller-owned WebGPU texture of a borrowed texture ring.
///
/// See `mln_web_gpu_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WebGpuBorrowedTexture {
    /// Borrowed WGPUTexture. Required.
    pub texture: *mut std::ffi::c_void,
    /// Borrowed WGPUTextureView for texture. Required. The view must be a 2D
    /// color view compatible with texture and the descriptor's format.
    pub texture_view: *mut std::ffi::c_void,
}
impl WebGpuBorrowedTexture {
    pub fn new(texture: *mut std::ffi::c_void, texture_view: *mut std::ffi::c_void) -> Self {
        Self {
            texture,
            texture_view,
        }
    }
}
impl ToNative<sys::mln_web_gpu_borrowed_texture> for WebGpuBorrowedTexture {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_web_gpu_borrowed_texture> {
        let mut raw: sys::mln_web_gpu_borrowed_texture = unsafe { std::mem::zeroed() };
        raw.texture = self.texture;
        raw.texture_view = self.texture_view;
        Ok(raw)
    }
}

/// WebGPU attachment options for a borrowed texture target.
///
/// See `mln_web_gpu_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct WebGpuBorrowedTextureDescriptor {
    /// Logical texture extent. Required. physical_width and physical_height
    /// give the texture's size. A scale_factor that differs from the map's is
    /// accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Physical texture width in device pixels. Required.
    pub physical_width: u32,
    /// Physical texture height in device pixels. Required.
    pub physical_height: u32,
    /// Borrowed WebGPU context. device is required. Rendering is submitted
    /// through context.queue or that device's default queue.
    pub context: WebGpuContextDescriptor,
    /// The ring's textures, one per slot, in slot order. Required.
    pub textures: Vec<WebGpuBorrowedTexture>,
    /// Backend-native WGPUTextureFormat value of every texture. Required, so
    /// Undefined is invalid.
    pub format: u32,
}
impl WebGpuBorrowedTextureDescriptor {
    pub fn new(
        extent: LogicalExtent,
        physical_width: u32,
        physical_height: u32,
        context: WebGpuContextDescriptor,
        textures: Vec<WebGpuBorrowedTexture>,
        format: u32,
    ) -> Self {
        Self {
            extent,
            physical_width,
            physical_height,
            context,
            textures,
            format,
        }
    }
}
impl ToNative<sys::mln_web_gpu_borrowed_texture_descriptor> for WebGpuBorrowedTextureDescriptor {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_web_gpu_borrowed_texture_descriptor> {
        let mut raw: sys::mln_web_gpu_borrowed_texture_descriptor = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_web_gpu_borrowed_texture_descriptor>() as _;
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

/// WebGPU backend context fields shared by WebGPU render targets.
///
/// See `mln_web_gpu_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WebGpuContextDescriptor {
    /// Borrowed WGPUInstance. Optional for texture targets.
    pub instance: *mut std::ffi::c_void,
    /// Borrowed WGPUDevice. Required.
    pub device: *mut std::ffi::c_void,
    /// Borrowed WGPUQueue. Optional; null uses the device default queue. A
    /// non-null queue must belong to device.
    pub queue: *mut std::ffi::c_void,
}
impl WebGpuContextDescriptor {
    pub fn new(device: *mut std::ffi::c_void) -> Self {
        Self {
            instance: Default::default(),
            device,
            queue: Default::default(),
        }
    }
}
impl ToNative<sys::mln_web_gpu_context_descriptor> for WebGpuContextDescriptor {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_web_gpu_context_descriptor> {
        let mut raw: sys::mln_web_gpu_context_descriptor = unsafe { std::mem::zeroed() };
        raw.instance = self.instance;
        raw.device = self.device;
        raw.queue = self.queue;
        Ok(raw)
    }
}

/// WebGPU attachment options for an owned texture target.
///
/// See `mln_web_gpu_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WebGpuOwnedTextureDescriptor {
    /// Logical texture extent. Required. A scale_factor that differs from the
    /// map's is accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Borrowed WebGPU context. device is required.
    pub context: WebGpuContextDescriptor,
}
impl WebGpuOwnedTextureDescriptor {
    pub fn new(extent: LogicalExtent, context: WebGpuContextDescriptor) -> Self {
        Self { extent, context }
    }
}
impl ToNative<sys::mln_web_gpu_owned_texture_descriptor> for WebGpuOwnedTextureDescriptor {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_web_gpu_owned_texture_descriptor> {
        let mut raw: sys::mln_web_gpu_owned_texture_descriptor = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_web_gpu_owned_texture_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.context = to_native(&self.context, arena)?;
        Ok(raw)
    }
}

/// WebGPU attachment options for a native surface.
///
/// See `mln_web_gpu_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WebGpuSurfaceDescriptor {
    /// Logical surface extent. Required. A scale_factor that differs from the
    /// map's is accepted and logged as a warning.
    pub extent: LogicalExtent,
    /// Borrowed WebGPU context. device is required.
    pub context: WebGpuContextDescriptor,
    /// Borrowed WGPUSurface. Required, and must stay alive for the session. The
    /// session configures it for this device and extent, and unconfigures it
    /// when the session ends.
    pub surface: *mut std::ffi::c_void,
    /// WGPUTextureFormat to configure the surface with. Required. A browser
    /// host takes it from navigator.gpu.getPreferredCanvasFormat().
    pub format: u32,
}
impl WebGpuSurfaceDescriptor {
    pub fn new(
        extent: LogicalExtent,
        context: WebGpuContextDescriptor,
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
impl ToNative<sys::mln_web_gpu_surface_descriptor> for WebGpuSurfaceDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_web_gpu_surface_descriptor> {
        let mut raw: sys::mln_web_gpu_surface_descriptor = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_web_gpu_surface_descriptor>() as _;
        raw.extent = to_native(&self.extent, arena)?;
        raw.context = to_native(&self.context, arena)?;
        raw.surface = self.surface;
        raw.format = self.format;
        Ok(raw)
    }
}

/// WebGPU frame acquired from a texture ring.
///
/// See `mln_web_gpu_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct WebGpuTextureFrame {
    /// Session generation that produced this frame.
    pub generation: u64,
    /// Physical WebGPU texture width in device pixels.
    pub width: u32,
    /// Physical WebGPU texture height in device pixels.
    pub height: u32,
    /// UI-to-device pixel scale used for this frame.
    pub scale_factor: f64,
    /// Opaque identity of this frame.
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
impl FromNative<sys::mln_web_gpu_texture_frame> for WebGpuTextureFrame {
    unsafe fn from_native(raw: sys::mln_web_gpu_texture_frame) -> Result<Self> {
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
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WglContextDescriptor {
    /// Borrowed HDC used to create the session context. Required. A dedicated
    /// owned texture makes its context current on this HDC from its core
    /// worker.
    pub device_context: *mut std::ffi::c_void,
    /// Borrowed HGLRC whose share group the session context joins. Required
    /// under shared ownership. A dedicated session joins no share group, so it
    /// must be null there.
    pub share_context: *mut std::ffi::c_void,
    /// Optional wglGetProcAddress-compatible function for the host loader.
    pub get_proc_address: *mut std::ffi::c_void,
}
impl WglContextDescriptor {
    pub fn new(device_context: *mut std::ffi::c_void) -> Self {
        Self {
            device_context,
            share_context: Default::default(),
            get_proc_address: Default::default(),
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
