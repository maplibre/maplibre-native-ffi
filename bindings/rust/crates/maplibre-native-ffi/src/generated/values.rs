// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

/// How a camera change animates.
///
/// See `mln_animation_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Default)]
pub struct AnimationOptions {
    pub duration_ms: Option<f64>,
    /// The average speed of a fly animation, in screens per second.
    /// `duration_ms` overrides it.
    pub velocity: Option<f64>,
    /// The zoom level at the peak of a fly animation.
    pub min_zoom: Option<f64>,
    pub easing: Option<UnitBezier>,
    /// An ID that `MapHandle::cancel_camera_transitions_with_id()` cancels the
    /// change by. Several changes can share one.
    pub transition_id: Option<u64>,
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

/// Limits on the camera, for `MapHandle::set_bounds()`.
///
/// See `mln_bound_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct BoundOptions {
    /// Removes the area that limits the camera's center.
    pub unbounded: bool,
    /// The area that the camera's center stays within.
    pub bounds: Option<LatLngBounds>,
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
    /// The lowest pitch, in degrees.
    pub min_pitch: Option<f64>,
    /// The highest pitch, in degrees.
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

native_enum! {
/// A maintenance action on a runtime's cache database.
///
/// The cache database is the file that `RuntimeOptions::cache_path` names. It
/// holds the ambient cache, meaning the resources that maps loaded, and the
/// offline regions.
///
/// See `mln_cache_database_operation` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub enum CacheDatabaseOperation: u32 {
    /// Deletes the ambient cache's resources and frees the space they used.
    /// Offline regions keep their resources.
    Clear = 1,
    /// Keeps the ambient cache's resources, but makes maps check each one with
    /// the server before they use it again. Offline regions keep their
    /// resources as they are.
    Invalidate = 2,
    /// Compacts the whole database file to free the disk space that it no
    /// longer uses.
    Pack = 3,
    /// Deletes the whole database, offline regions included, and starts an
    /// empty one.
    Reset = 4,
} Unknown
}

/// A map's whole camera.
///
/// See `mln_camera` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Camera {
    pub center: LatLng,
    /// The altitude of the center, in meters above sea level.
    pub center_altitude: f64,
    /// Insets from the map's edges. The camera centers within the rest.
    pub padding: EdgeInsets,
    pub zoom: f64,
    /// The rotation from north, in degrees.
    pub bearing: f64,
    /// The tilt toward the horizon, in degrees. 0 looks straight down.
    pub pitch: f64,
    /// The roll, in degrees.
    pub roll: f64,
    /// The vertical field of view, in degrees.
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
    /// Converts this `Camera` to `CameraOptions` that set each of its members.
    pub const fn to_options(self) -> CameraOptions {
        CameraOptions {
            center: Some(self.center),
            center_altitude: Some(self.center_altitude),
            padding: Some(self.padding),
            anchor: None,
            zoom: Some(self.zoom),
            bearing: Some(self.bearing),
            pitch: Some(self.pitch),
            roll: Some(self.roll),
            field_of_view: Some(self.field_of_view),
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

/// All the limits on a map's camera.
///
/// Its members mean the same as those of `BoundOptions`.
///
/// See `mln_camera_bounds` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct CameraBounds {
    /// The area that the camera's center stays within. When `unbounded` is
    /// true, it covers the whole world.
    pub bounds: LatLngBounds,
    /// Whether no area limits the camera's center.
    pub unbounded: bool,
    pub min_zoom: f64,
    pub max_zoom: f64,
    /// The lowest pitch, in degrees.
    pub min_pitch: f64,
    /// The highest pitch, in degrees.
    pub max_pitch: f64,
}
impl FromNative<sys::mln_camera_bounds> for CameraBounds {
    unsafe fn from_native(raw: sys::mln_camera_bounds) -> Result<Self> {
        Ok(Self {
            bounds: unsafe { from_native(raw.bounds) }?,
            unbounded: raw.unbounded,
            min_zoom: raw.min_zoom,
            max_zoom: raw.max_zoom,
            min_pitch: raw.min_pitch,
            max_pitch: raw.max_pitch,
        })
    }
}

native_enum! {
/// Whether a camera change jumps or animates.
///
/// See `mln_camera_change_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub enum CameraChangeMode: u32 {
    Immediate = 0,
    Animated = 1,
} Unknown
}
impl Default for CameraChangeMode {
    fn default() -> Self {
        Self::Immediate
    }
}

/// A change to the camera from where it is now, for gestures.
///
/// See `mln_camera_delta` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Default)]
pub struct CameraDelta {
    /// Moves the map's content this far on the screen, as a drag does.
    pub offset: Option<ScreenPoint>,
    /// Multiplies the map's scale. 2 zooms in by one level.
    pub scale: Option<f64>,
    /// Degrees to add to the bearing.
    pub bearing: Option<f64>,
    /// Degrees to add to the pitch.
    pub pitch: Option<f64>,
    /// The screen point that `scale`, `bearing`, and `pitch` keep in place. It
    /// needs one of them, and it combines with `offset` only in a delta without
    /// a duration.
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

/// Options for finding the camera that fits an area.
///
/// See `mln_camera_fit_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct CameraFitOptions {
    /// Space to leave between the area and each edge of the map.
    pub padding: Option<EdgeInsets>,
    /// The rotation from north, in degrees.
    pub bearing: Option<f64>,
    /// The tilt toward the horizon, in degrees.
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

/// A camera change that fits an area, for
/// `MapHandle::fit_camera_to_lat_lng_bounds()` and the other fit commands.
///
/// A jump uses only the end handler of `animation`, and the handler runs as
/// soon as the camera moves.
///
/// See `mln_camera_fit_update` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Default)]
pub struct CameraFitUpdate {
    pub mode: CameraUpdateMode,
    pub fit: CameraFitOptions,
    pub animation: AnimationOptions,
}
impl ToNative<sys::mln_camera_fit_update> for CameraFitUpdate {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_camera_fit_update> {
        let mut raw: sys::mln_camera_fit_update = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_camera_fit_update>() as _;
        raw.mode = to_native(&self.mode, arena)?;
        raw.fit = to_native(&self.fit, arena)?;
        raw.animation = to_native(&self.animation, arena)?;
        Ok(raw)
    }
}

/// A camera, with only the members that `fields` selects.
///
/// See `mln_camera_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct CameraOptions {
    pub center: Option<LatLng>,
    /// The altitude of the center, in meters above sea level.
    pub center_altitude: Option<f64>,
    /// Insets from the map's edges. The camera centers within the rest.
    pub padding: Option<EdgeInsets>,
    /// The screen point that a change to the zoom, bearing, or pitch keeps in
    /// place. A `center` overrides it.
    pub anchor: Option<ScreenPoint>,
    pub zoom: Option<f64>,
    /// The rotation from north, in degrees.
    pub bearing: Option<f64>,
    /// The tilt toward the horizon, in degrees. 0 looks straight down.
    pub pitch: Option<f64>,
    /// The roll, in degrees.
    pub roll: Option<f64>,
    /// The vertical field of view, in degrees.
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

/// A map's camera, with the snapshot generation that it matches.
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

/// The end of a camera change.
///
/// See `mln_camera_transition_end` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct CameraTransitionEnd {
    pub outcome: CameraTransitionOutcome,
    /// The generation of the first map snapshot that shows where the change
    /// left the camera, or 0 when the map closed first.
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

/// A callback for the end of a camera change, for `AnimationOptions`.
///
/// The callback runs once for each camera command, after every part of the
/// change that the command started has ended.
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
/// How a camera change ended.
///
/// A camera delta that pans and also scales, rotates, or pitches animates as
/// two parts, and any other change as one. When the parts of a change end in
/// different ways, the change reports `CameraTransitionOutcome::Cancelled` if
/// any part was cancelled, then `CameraTransitionOutcome::Replaced` if any part
/// was replaced.
///
/// See `mln_camera_transition_outcome` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub enum CameraTransitionOutcome: u32 {
    /// The camera finished the change.
    Completed = 0,
    /// `MapHandle::cancel_camera_transitions()`,
    /// `MapHandle::cancel_camera_transitions_with_id()`, a gesture's
    /// `GesturePhase::Begin` or `GesturePhase::Cancel`,
    /// `MapHandle::set_free_camera_options()`, or a resize that moves the
    /// camera back inside its bounds stopped the change, or the command failed
    /// before it moved the camera.
    Cancelled = 1,
    /// The map closed before the change ended.
    Closed = 2,
    /// A later camera change set every camera field that a part of the change
    /// still animated, so that part stopped.
    ///
    /// A fly animates its center and zoom together, and an anchor animates the
    /// center together with the fields it keeps in place, so a later change of
    /// any of them replaces them all. A part that a later change took only some
    /// fields from keeps animating the others, and ends as they end.
    Replaced = 3,
} Unknown
}
impl Default for CameraTransitionOutcome {
    fn default() -> Self {
        Self::Completed
    }
}

/// A camera change for `MapHandle::update_camera()`.
///
/// A jump uses only the end handler of `animation`, and the handler runs as
/// soon as the camera moves.
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
/// How a camera command moves the camera.
///
/// See `mln_camera_update_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub enum CameraUpdateMode: u32 {
    /// Moves the camera at once.
    Jump = 0,
    /// Animates the camera straight to the target.
    Ease = 1,
    /// Zooms out, travels to the target, and zooms back in.
    Fly = 2,
} Unknown
}
impl Default for CameraUpdateMode {
    fn default() -> Self {
        Self::Jump
    }
}

/// A tile's zoom level `z`, column `x`, and row `y`, with row 0 at the north.
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
/// How the map keeps the camera within the world or within its bounds.
///
/// See `mln_constrain_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub enum ConstrainMode: u32 {
    None = 0,
    /// Keeps the world filling the view from top to bottom.
    HeightOnly = 1,
    /// Keeps the world filling the view in both directions.
    WidthAndHeight = 2,
    /// Keeps the whole view inside the bounds from `MapHandle::set_bounds()`.
    Screen = 3,
} Unknown
}
impl Default for ConstrainMode {
    fn default() -> Self {
        Self::None
    }
}

/// The callbacks and options of a custom geometry source.
///
/// Zoom levels are whole numbers from 0 to 32.
///
/// See `mln_custom_geometry_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Clone)]
pub struct CustomGeometrySourceOptions {
    /// The callback receives (`map`, `source_id`, `tile_id`).
    pub fetch_tile:
        std::sync::Arc<dyn Fn(&MapHandle, &str, CanonicalTileId) + Send + Sync + 'static>,
    /// The callback receives (`map`, `source_id`, `tile_id`).
    pub cancel_tile:
        Option<std::sync::Arc<dyn Fn(&MapHandle, &str, CanonicalTileId) + Send + Sync + 'static>>,
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
    /// How far, in pixels, simplified geometry can stray from the original.
    pub tolerance: Option<f64>,
    /// The width of a tile in pixels, a power of two no larger than 8192.
    pub tile_size: Option<u32>,
    /// The width in pixels of the margin around a tile that holds geometry from
    /// beyond its edges.
    pub buffer: Option<u32>,
    /// Whether to cut geometry at the outer edge of a tile's margin.
    pub clip: Option<bool>,
    /// Whether to wrap geometry that crosses the antimeridian.
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
        F: Fn(&MapHandle, &str, CanonicalTileId) + Send + Sync + 'static,
    {
        self.cancel_tile = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(fetch_tile: F) -> Self
    where
        F: Fn(&MapHandle, &str, CanonicalTileId) + Send + Sync + 'static,
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
        map: sys::mln_map,
        source_id: sys::mln_buffer_view,
        tile_id: sys::mln_canonical_tile_id,
    ) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_custom_source_fetch_tile_callback", (), || {
            (state.fetch_tile)(
                &*MapHandle::borrow(map)?,
                unsafe { crate::string::borrow_string_view(source_id) }?,
                unsafe { from_native(tile_id) }?,
            );
            Ok(())
        })
    }
    unsafe extern "C" fn cancel_tile_trampoline(
        user_data: *mut std::ffi::c_void,
        map: sys::mln_map,
        source_id: sys::mln_buffer_view,
        tile_id: sys::mln_canonical_tile_id,
    ) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_custom_source_cancel_tile_callback", (), || {
            callback::require(&state.cancel_tile)?(
                &*MapHandle::borrow(map)?,
                unsafe { crate::string::borrow_string_view(source_id) }?,
                unsafe { from_native(tile_id) }?,
            );
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

/// The callbacks and options of a custom vector source.
///
/// Zoom levels are whole numbers from 0 to 32.
///
/// See `mln_custom_vector_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Clone)]
pub struct CustomVectorSourceOptions {
    /// The callback receives (`map`, `source_id`, `tile_id`).
    pub fetch_tile:
        std::sync::Arc<dyn Fn(&MapHandle, &str, CanonicalTileId) + Send + Sync + 'static>,
    /// The callback receives (`map`, `source_id`, `tile_id`).
    pub cancel_tile:
        Option<std::sync::Arc<dyn Fn(&MapHandle, &str, CanonicalTileId) + Send + Sync + 'static>>,
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
}
impl std::fmt::Debug for CustomVectorSourceOptions {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("CustomVectorSourceOptions")
            .finish_non_exhaustive()
    }
}
impl CustomVectorSourceOptions {
    pub fn with_cancel_tile<F>(mut self, callback: F) -> Self
    where
        F: Fn(&MapHandle, &str, CanonicalTileId) + Send + Sync + 'static,
    {
        self.cancel_tile = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(fetch_tile: F) -> Self
    where
        F: Fn(&MapHandle, &str, CanonicalTileId) + Send + Sync + 'static,
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
        map: sys::mln_map,
        source_id: sys::mln_buffer_view,
        tile_id: sys::mln_canonical_tile_id,
    ) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_custom_source_fetch_tile_callback", (), || {
            (state.fetch_tile)(
                &*MapHandle::borrow(map)?,
                unsafe { crate::string::borrow_string_view(source_id) }?,
                unsafe { from_native(tile_id) }?,
            );
            Ok(())
        })
    }
    unsafe extern "C" fn cancel_tile_trampoline(
        user_data: *mut std::ffi::c_void,
        map: sys::mln_map,
        source_id: sys::mln_buffer_view,
        tile_id: sys::mln_canonical_tile_id,
    ) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_custom_source_cancel_tile_callback", (), || {
            callback::require(&state.cancel_tile)?(
                &*MapHandle::borrow(map)?,
                unsafe { crate::string::borrow_string_view(source_id) }?,
                unsafe { from_native(tile_id) }?,
            );
            Ok(())
        })
    }
}
impl ToNative<sys::mln_custom_vector_source_options> for CustomVectorSourceOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_custom_vector_source_options> {
        let mut raw: sys::mln_custom_vector_source_options = unsafe { std::mem::zeroed() };
        raw.fields = 0;
        raw.size = std::mem::size_of::<sys::mln_custom_vector_source_options>() as _;
        raw.fetch_tile = Some(Self::fetch_tile_trampoline as _);
        raw.cancel_tile = self
            .cancel_tile
            .as_ref()
            .map(|_| Self::cancel_tile_trampoline as _);
        if let Some(item) = &self.min_zoom {
            raw.fields |= sys::MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM;
            raw.min_zoom = *item;
        }
        if let Some(item) = &self.max_zoom {
            raw.fields |= sys::MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM;
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

/// Distances in logical pixels from each edge of the map.
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

/// The EGL objects that a render session makes its OpenGL context from.
///
/// See `mln_egl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct EglContextDescriptor {
    /// An `EGLDisplay`.
    pub display: *mut std::ffi::c_void,
    /// An `EGLConfig`.
    pub config: *mut std::ffi::c_void,
    /// Your `EGLContext`, for a shared context.
    pub share_context: *mut std::ffi::c_void,
    /// The API of a dedicated context. A shared context uses the API of yours.
    pub client_api: OpenGlClientApi,
    /// Your `eglGetProcAddress`, or null to use the system's.
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

/// The ID of a feature in a source, as the source's data gives it.
///
/// The map compares feature IDs as text, so a string and a number with the same
/// text, such as "7" and 7, name the same feature. A number's text is its
/// shortest decimal form without a trailing ".0", so 7, 7.0, and "7" name the
/// same feature, and so do 1e21 and "1e21".
///
/// See `mln_feature_id` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub enum FeatureId<'a> {
    StringValue(&'a str),
    UintValue(u64),
    IntValue(i64),
    /// A finite number.
    DoubleValue(f64),
}
impl<'a> ToNative<sys::mln_feature_id> for FeatureId<'a> {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_feature_id> {
        let mut raw: sys::mln_feature_id = unsafe { std::mem::zeroed() };
        match self {
            FeatureId::StringValue(item) => {
                raw.type_ = sys::MLN_FEATURE_ID_TYPE_STRING;
                raw.data.string_value = to_native(&(*item), arena)?;
            }
            FeatureId::UintValue(item) => {
                raw.type_ = sys::MLN_FEATURE_ID_TYPE_UINT;
                raw.data.uint_value = *item;
            }
            FeatureId::IntValue(item) => {
                raw.type_ = sys::MLN_FEATURE_ID_TYPE_INT;
                raw.data.int_value = *item;
            }
            FeatureId::DoubleValue(item) => {
                raw.type_ = sys::MLN_FEATURE_ID_TYPE_DOUBLE;
                raw.data.double_value = *item;
            }
        }
        Ok(raw)
    }
}

/// The features whose state to set, read, or remove.
///
/// See `mln_feature_state_selector` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct FeatureStateSelector<'a> {
    pub source_id: &'a str,
    /// The source layer, for a vector source. Empty for other sources.
    pub source_layer_id: &'a str,
    /// The feature's ID. Setting or reading state needs it. Without it,
    /// `MapHandle::remove_feature_state()` removes the state of every feature
    /// in the source layer.
    pub feature_id: Option<FeatureId<'a>>,
    /// The one key that `MapHandle::remove_feature_state()` removes. It needs
    /// `feature_id`. Empty removes every key.
    pub state_key: &'a str,
}
impl<'a> FeatureStateSelector<'a> {
    pub fn new(source_id: &'a str) -> Self {
        Self {
            source_id,
            source_layer_id: Default::default(),
            feature_id: None,
            state_key: Default::default(),
        }
    }
}
impl<'a> ToNative<sys::mln_feature_state_selector> for FeatureStateSelector<'a> {
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

/// A demand for one frame.
///
/// See `mln_frame_demand` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct FrameDemand {
    pub flags: FrameDemandFlag,
    /// A value of your choice that the demand's frame result carries.
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
/// How a frame demand behaves.
///
/// Without flags, a demand draws a frame when the map changed since the last
/// one, and otherwise finishes with `RenderResult::NoUpdate`. A session with a
/// surface target shows each frame that it draws.
///
/// See `mln_frame_demand_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
pub struct FrameDemandFlag: u32 {
    /// Draws a frame even when nothing changed since the last one. It can't be
    /// combined with `FrameDemandFlag::WAIT_FOR_UPDATE`.
    const FORCE = 1;
    /// When the map has nothing new to draw, waits for its next update instead
    /// of finishing with `RenderResult::NoUpdate`.
    const WAIT_FOR_UPDATE = 4;
}
}

/// A map camera's whole position and orientation in three dimensions.
///
/// Its members mean the same as those of `FreeCameraOptions`.
///
/// See `mln_free_camera` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct FreeCamera {
    pub position: Vec3,
    pub orientation: Quaternion,
}
impl FromNative<sys::mln_free_camera> for FreeCamera {
    unsafe fn from_native(raw: sys::mln_free_camera) -> Result<Self> {
        Ok(Self {
            position: unsafe { from_native(raw.position) }?,
            orientation: unsafe { from_native(raw.orientation) }?,
        })
    }
}

/// The camera's position and orientation in three dimensions.
///
/// See `mln_free_camera_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct FreeCameraOptions {
    /// The camera's position in Web Mercator units, where the world is 1 wide,
    /// (0, 0) is its northwest corner, and (1, 1) is its southeast corner. `z`
    /// is the altitude in the same units.
    pub position: Option<Vec3>,
    /// The camera's rotation from looking straight down with north at the top
    /// of the screen.
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

/// Options for a GeoJSON
/// \[source\](https://maplibre.org/maplibre-style-spec/sources/#geojson).
///
/// Zoom levels are whole numbers.
///
/// See `mln_geo_json_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct GeoJsonSourceOptions {
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
    pub tolerance: Option<f64>,
    pub cluster_max_zoom: Option<f64>,
    /// The `clusterProperties` object, or empty for none.
    pub cluster_properties: String,
    pub buffer: Option<u32>,
    pub cluster_radius: Option<u32>,
    /// At most 4294967295 on a 32-bit target.
    pub cluster_min_points: Option<u64>,
    pub line_metrics: Option<bool>,
    pub cluster: Option<bool>,
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
        Ok(raw)
    }
}
impl FromNative<sys::mln_geo_json_source_options> for GeoJsonSourceOptions {
    unsafe fn from_native(raw: sys::mln_geo_json_source_options) -> Result<Self> {
        Ok(Self {
            min_zoom: (raw.fields & sys::MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM != 0)
                .then_some(raw.min_zoom),
            max_zoom: (raw.fields & sys::MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM != 0)
                .then_some(raw.max_zoom),
            tolerance: (raw.fields & sys::MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE != 0)
                .then_some(raw.tolerance),
            cluster_max_zoom: (raw.fields & sys::MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM != 0)
                .then_some(raw.cluster_max_zoom),
            cluster_properties: unsafe { from_native(raw.cluster_properties) }?,
            buffer: (raw.fields & sys::MLN_GEO_JSON_SOURCE_OPTION_BUFFER != 0)
                .then_some(raw.buffer),
            cluster_radius: (raw.fields & sys::MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS != 0)
                .then_some(raw.cluster_radius),
            cluster_min_points: (raw.fields & sys::MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
                != 0)
                .then_some(raw.cluster_min_points),
            line_metrics: (raw.fields & sys::MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS != 0)
                .then_some(raw.line_metrics),
            cluster: (raw.fields & sys::MLN_GEO_JSON_SOURCE_OPTION_CLUSTER != 0)
                .then_some(raw.cluster),
        })
    }
}

native_enum! {
/// Where a camera change falls in a gesture that the user makes.
///
/// `GesturePhase::Begin` and `GesturePhase::Update` mark a gesture in progress,
/// and `GesturePhase::End` and `GesturePhase::Cancel` end it.
///
/// See `mln_gesture_phase` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub enum GesturePhase: u32 {
    /// The change isn't part of a gesture.
    None = 0,
    /// The gesture starts. This cancels every running camera animation before
    /// the change applies.
    Begin = 1,
    /// The gesture continues. To start a gesture that keeps the running camera
    /// animations, start it with this phase instead of `GesturePhase::Begin`.
    Update = 2,
    End = 3,
    /// The platform cancelled the gesture. This also cancels every running
    /// camera animation after the change applies.
    Cancel = 4,
} Unknown
}
impl Default for GesturePhase {
    fn default() -> Self {
        Self::None
    }
}

/// A point in GPU work that the other side waits for before using a texture.
///
/// See `mln_gpu_sync` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct GpuSync {
    pub kind: GpuSyncKind,
    /// The object to wait on, for the kinds that name one.
    pub handle: u64,
    pub value: u64,
}
impl ToNative<sys::mln_gpu_sync> for GpuSync {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_gpu_sync> {
        let mut raw: sys::mln_gpu_sync = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_gpu_sync>() as _;
        raw.kind = to_native(&self.kind, arena)?;
        raw.handle = self.handle;
        raw.value = self.value;
        Ok(raw)
    }
}
impl FromNative<sys::mln_gpu_sync> for GpuSync {
    unsafe fn from_native(raw: sys::mln_gpu_sync) -> Result<Self> {
        Ok(Self {
            kind: GpuSyncKind::from_raw(raw.kind),
            handle: raw.handle,
            value: raw.value,
        })
    }
}

native_enum! {
/// How one side waits for the other's GPU work on a shared texture.
///
/// A sync file descriptor or an OpenGL fence signals once, so when
/// `AcquiredFrameHandle::release()` succeeds, the session takes the one you
/// pass. A Vulkan timeline semaphore, Metal shared event, or D3D12 fence counts
/// up with each frame, so the one you pass stays yours. Keep it valid until a
/// detach or release of the session completes.
///
/// See `mln_gpu_sync_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub enum GpuSyncKind: u32 {
    /// The work has already finished, so there is nothing to wait for.
    CpuComplete = 0,
    /// `handle` is a `GLsync` in the session's share group.
    OpenGlFence = 1,
    /// The work was submitted to the queue that both sides share, so work
    /// submitted to that queue later runs after it.
    QueueOrder = 2,
    /// `handle` is a `VkSemaphore`. Wait until it reaches `value`.
    VulkanTimelineSemaphore = 3,
    /// `handle` is a sync file descriptor.
    SyncFd = 4,
    /// `handle` is an `MTLSharedEvent`. Wait until it reaches `value`.
    MetalSharedEvent = 5,
    /// `handle` is a shared Windows handle to an `ID3D12Fence`. Wait until it
    /// reaches `value`.
    D3D12Fence = 6,
} Unknown
}
impl Default for GpuSyncKind {
    fn default() -> Self {
        Self::CpuComplete
    }
}

native_flags! {
/// A GPU sync kind, as a bit in a set.
///
/// See `mln_gpu_sync_kind_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub struct GpuSyncKindFlag: u32 {
    const CPU_COMPLETE = 1;
    const OPEN_GL_FENCE = 2;
    const QUEUE_ORDER = 4;
    const VULKAN_TIMELINE_SEMAPHORE = 8;
    const SYNC_FD = 16;
    const METAL_SHARED_EVENT = 32;
    const D3D12_FENCE = 64;
}
}

/// An HTTP header.
///
/// `name` follows the rules of `HttpHeaderTransformResponse::set_header()`.
///
/// See `mln_http_header` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct HttpHeader {
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

/// Headers to add to the HTTP requests whose URL matches a pattern.
///
/// See `mln_http_header_rule` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct HttpHeaderRule {
    /// The kinds of resource that the rule covers. Zero covers every kind.
    pub kinds: ResourceKindFlag,
    pub flags: UrlMatchFlag,
    pub url: String,
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

/// Headers to add to HTTP requests, for
/// `RuntimeHandle::set_http_header_transform()`.
///
/// Set either `callback` or `rules`, not both.
///
/// Runs on a library thread.
///
/// See `mln_http_header_transform` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Clone, Default)]
pub struct HttpHeaderTransform {
    /// The callback receives (`kind`, `url`, `out_response`).
    ///
    /// An `Err` of `Error::Status` with a failure status answers native
    /// that status. Any other error, or a panic, reaches the reporter and
    /// answers `Status::NativeError`.
    pub callback: Option<
        std::sync::Arc<
            dyn Fn(ResourceKind, &str, &mut HttpHeaderTransformResponse<'_>) -> Result<()>
                + Send
                + Sync
                + 'static,
        >,
    >,
    /// The first rule that matches a request adds its headers, and no other.
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
                    unsafe { crate::string::borrow_string_view(url) }?,
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
///
/// The headers that an HTTP header transform callback sets.
///
/// See `mln_http_header_transform_response` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
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
    /// Sets a header on the request that an HTTP header transform callback
    /// handles.
    ///
    /// Header names ignore letter case, and setting a header again replaces its
    /// value. `name` can't be a header that the library or the platform
    /// manages: `Accept-Encoding`, `Connection`, `Content-Length`, `Host`,
    /// `If-Modified-Since`, `If-None-Match`, `Keep-Alive`,
    /// `Proxy-Authorization`, `Proxy-Connection`, `Range`, `TE`, `Trailer`,
    /// `Transfer-Encoding`, `Upgrade`, or `User-Agent`.
    ///
    /// See `mln_http_header_transform_response_set_header` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_header(&mut self, name: &str, value: &str) -> Result<()> {
        let arena = &mut InputArena::default();
        let native_name = to_native(&name, arena)?;
        let native_value = to_native(&value, arena)?;
        crate::error::check(|out_diagnostic| unsafe {
            sys::mln_http_header_transform_response_set_header(
                self.raw.as_ptr(),
                native_name,
                native_value,
                out_diagnostic,
            )
        })
    }
}

/// The area of an image, in image pixels, that text fits inside.
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

/// A range of image pixels, from `from` to `to`, that stretches to fit text.
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

/// A geographic coordinate, in degrees.
///
/// See `mln_lat_lng` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
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

/// A rectangle of coordinates, from its southwest to its northeast corner.
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
/// The part of the library that logged a message.
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

/// A log callback, for `set_log_callback()`.
///
/// Runs on the thread that logged the message, or on a background thread for
/// the severities that `set_log_async_severity_mask()` selects. The callback
/// can't call the library.
///
/// See `mln_log_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
#[derive(Clone)]
pub struct LogHandler {
    /// The callback receives (`severity`, `event`, `code`, `message`).
    pub callback: std::sync::Arc<dyn Fn(LogSeverity, LogEvent, i64, &str) + Send + Sync + 'static>,
    /// The severities that also go to the platform's log, as they do without a
    /// callback. 0 sends none.
    ///
    /// A record that the handler's delivery queue can't accept is dropped.
    /// These severities alone decide whether a record also goes to the
    /// platform's log, whether or not the queue accepts it.
    pub platform_severities: LogSeverityFlag,
}
impl std::fmt::Debug for LogHandler {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("LogHandler").finish_non_exhaustive()
    }
}
impl LogHandler {
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(LogSeverity, LogEvent, i64, &str) + Send + Sync + 'static,
    {
        Self {
            callback: std::sync::Arc::new(callback),
            platform_severities: Default::default(),
        }
    }
    unsafe extern "C" fn callback_trampoline(
        user_data: *mut std::ffi::c_void,
        severity: sys::mln_log_severity,
        event: sys::mln_log_event,
        code: i64,
        message: sys::mln_buffer_view,
    ) {
        // SAFETY: native passes the registration that this trampoline's
        // descriptor transferred.
        let state = unsafe { callback::state::<Self>(user_data) };
        callback::invoke("mln_log_callback", (), || {
            (state.callback)(
                LogSeverity::from_raw(severity),
                LogEvent::from_raw(event),
                code,
                unsafe { crate::string::borrow_string_view(message) }?,
            );
            Ok(())
        })
    }
}
impl ToNative<sys::mln_log_handler> for LogHandler {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_log_handler> {
        let mut raw: sys::mln_log_handler = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_log_handler>() as _;
        raw.callback = Some(Self::callback_trampoline as _);
        raw.platform_severities = to_native(&self.platform_severities, arena)?;
        if !(raw.callback.is_none()) {
            // SAFETY: the release reclaims exactly this state.
            raw.user_data = unsafe { arena.registration(self.clone(), callback::release::<Self>) };
            raw.release_user_data = Some(callback::release::<Self>);
        }
        Ok(raw)
    }
}

native_enum! {
/// The severity of a log message.
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
/// A log severity, as a bit in a set.
///
/// See `mln_log_severity_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
pub struct LogSeverityFlag: u32 {
    const INFO = 2;
    const WARNING = 4;
    const ERROR = 8;
    /// The severities logged on a background thread until
    /// `set_log_async_severity_mask()` changes them.
    const DEFAULT = 6;
    const ALL = 14;
}
}

/// A size in logical pixels, and the physical pixels in each logical pixel.
///
/// See `mln_logical_extent` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
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

/// A size in logical pixels, which the map's scale factor turns into pixels.
///
/// See `mln_logical_size` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct LogicalSize {
    pub width: u32,
    pub height: u32,
}
impl LogicalSize {
    pub const fn new(width: u32, height: u32) -> Self {
        Self { width, height }
    }
}
impl ToNative<sys::mln_logical_size> for LogicalSize {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_logical_size> {
        let mut raw: sys::mln_logical_size = unsafe { std::mem::zeroed() };
        raw.width = self.width;
        raw.height = self.height;
        Ok(raw)
    }
}

native_flags! {
/// A debug overlay that the map draws.
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
/// Why a map failed to load a style.
///
/// See `mln_map_load_error` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub enum MapLoadError: u32 {
    Unknown = 0,
    /// The style document isn't a valid style.
    StyleParse = 1,
    /// The style's request failed, for any reason, including a 404 or a missing
    /// file.
    StyleLoad = 2,
} Unrecognized
}
impl Default for MapLoadError {
    fn default() -> Self {
        Self::Unknown
    }
}

native_enum! {
/// When a map renders.
///
/// See `mln_map_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub enum MapMode: u32 {
    /// Renders whenever the map changes.
    Continuous = 0,
    /// Renders only when `MapHandle::request_still_image()` asks for an image.
    Static = 1,
    /// Renders still images of single tiles, as `MapMode::Static` does.
    Tile = 2,
} Unknown
}
impl Default for MapMode {
    fn default() -> Self {
        Self::Continuous
    }
}

/// Options for `RuntimeHandle::create_map()`.
///
/// See `mln_map_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MapOptions {
    /// The map's first size, and its scale factor. The scale factor stays fixed
    /// for the map's life, and every render session of the map renders at it.
    /// To render at another density, create a new map and attach a new session.
    pub initial_extent: LogicalExtent,
    pub map_mode: MapMode,
    /// Whether the map decodes MapLibre Tiles (MLT) that use FastPFOR integer
    /// encoding. Without it, such tiles fail to load.
    pub fast_pfor_enabled: bool,
    /// The events that the map reports. Without it, the map reports them all.
    pub event_mask: Option<RuntimeEventFlag>,
    /// Whether the map keeps tiles in memory after they leave the view, so they
    /// show again without loading. Without it, the map keeps them.
    pub tile_cache_enabled: Option<bool>,
}
impl MapOptions {
    pub fn new(initial_extent: LogicalExtent) -> Self {
        Self {
            initial_extent,
            map_mode: Default::default(),
            fast_pfor_enabled: Default::default(),
            event_mask: None,
            tile_cache_enabled: None,
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
        if let Some(item) = &self.tile_cache_enabled {
            raw.fields |= sys::MLN_MAP_OPTION_TILE_CACHE_ENABLED;
            raw.tile_cache_enabled = *item;
        }
        Ok(raw)
    }
}

/// A projection's camera, as `MapProjectionHandle::get_camera()` reports it.
///
/// See `mln_map_projection_camera_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MapProjectionCameraResult {
    pub camera: Camera,
}
impl FromNative<sys::mln_map_projection_camera_result> for MapProjectionCameraResult {
    unsafe fn from_native(raw: sys::mln_map_projection_camera_result) -> Result<Self> {
        Ok(Self {
            camera: unsafe { from_native(raw.camera) }?,
        })
    }
}

/// A map's state, as `MapHandle::get_snapshot()` reads it.
///
/// See `mln_map_snapshot` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MapSnapshot {
    pub debug_flags: MapDebugFlag,
    /// Grows with each change to the map's state.
    pub generation: u64,
    /// Whether the style and every resource that the view needs have loaded.
    pub fully_loaded: bool,
    pub rendering_stats_view_enabled: bool,
    pub gesture_in_progress: bool,
    pub transition_in_progress: bool,
    pub event_mask: RuntimeEventFlag,
    /// Counts the render updates that the map has made for its render session.
    pub latest_render_update_generation: u64,
    pub tile: MapTileSettings,
    pub bounds: CameraBounds,
    pub free_camera: FreeCamera,
    pub view: MapView,
}
impl FromNative<sys::mln_map_snapshot> for MapSnapshot {
    unsafe fn from_native(raw: sys::mln_map_snapshot) -> Result<Self> {
        Ok(Self {
            debug_flags: MapDebugFlag::from_bits_retain(raw.debug_flags),
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

/// How the map picks the tiles to load.
///
/// The `lod_` members lower the level of detail away from the camera, so the
/// map loads fewer tiles when the pitch is high.
///
/// See `mln_map_tile_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MapTileOptions {
    /// How many zoom levels lower the map also loads tiles for, so it can show
    /// a coarse map while the current tiles load. 0 loads only the current
    /// tiles.
    pub prefetch_zoom_delta: Option<u32>,
    /// The radius around the view point, in tiles, that keeps full detail.
    pub lod_min_radius: Option<f64>,
    /// Scales the distance to the view point. Above 1 lowers the detail.
    pub lod_scale: Option<f64>,
    /// The pitch, in degrees, above which the map lowers the detail.
    pub lod_pitch_threshold: Option<f64>,
    /// Zoom levels to add for picking the detail. Below 0 loads fewer tiles.
    pub lod_zoom_shift: Option<f64>,
    /// `TileLodMode::Distance` ignores `lod_min_radius`.
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

/// All the settings for how a map picks the tiles to load.
///
/// Its members mean the same as those of `MapTileOptions`.
///
/// See `mln_map_tile_settings` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MapTileSettings {
    pub prefetch_zoom_delta: u32,
    pub lod_min_radius: f64,
    pub lod_scale: f64,
    /// The pitch, in degrees, above which the map lowers the detail.
    pub lod_pitch_threshold: f64,
    pub lod_zoom_shift: f64,
    pub lod_mode: TileLodMode,
}
impl FromNative<sys::mln_map_tile_settings> for MapTileSettings {
    unsafe fn from_native(raw: sys::mln_map_tile_settings) -> Result<Self> {
        Ok(Self {
            prefetch_zoom_delta: raw.prefetch_zoom_delta,
            lod_min_radius: raw.lod_min_radius,
            lod_scale: raw.lod_scale,
            lod_pitch_threshold: raw.lod_pitch_threshold,
            lod_zoom_shift: raw.lod_zoom_shift,
            lod_mode: TileLodMode::from_raw(raw.lod_mode),
        })
    }
}

/// One view of a map: its camera, size, and projection.
///
/// See `mln_map_view` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MapView {
    pub camera: Camera,
    pub extent: LogicalExtent,
    pub projection_mode: ProjectionMode,
    pub viewport: MapViewport,
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

/// A map's whole viewport.
///
/// Its members mean the same as those of `MapViewportOptions`.
///
/// See `mln_map_viewport` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MapViewport {
    pub north_orientation: NorthOrientation,
    pub constrain_mode: ConstrainMode,
    pub viewport_mode: ViewportMode,
    pub frustum_offset: EdgeInsets,
}
impl ToNative<sys::mln_map_viewport> for MapViewport {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_map_viewport> {
        let mut raw: sys::mln_map_viewport = unsafe { std::mem::zeroed() };
        raw.north_orientation = to_native(&self.north_orientation, arena)?;
        raw.constrain_mode = to_native(&self.constrain_mode, arena)?;
        raw.viewport_mode = to_native(&self.viewport_mode, arena)?;
        raw.frustum_offset = to_native(&self.frustum_offset, arena)?;
        Ok(raw)
    }
}
impl FromNative<sys::mln_map_viewport> for MapViewport {
    unsafe fn from_native(raw: sys::mln_map_viewport) -> Result<Self> {
        Ok(Self {
            north_orientation: NorthOrientation::from_raw(raw.north_orientation),
            constrain_mode: ConstrainMode::from_raw(raw.constrain_mode),
            viewport_mode: ViewportMode::from_raw(raw.viewport_mode),
            frustum_offset: unsafe { from_native(raw.frustum_offset) }?,
        })
    }
}

/// How the map orients and constrains its view.
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

/// A Metal texture of a borrowed ring.
///
/// See `mln_metal_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalBorrowedTexture {
    /// Your MTLTexture.
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

/// A ring of your Metal textures, for
/// `MapHandle::attach_metal_borrowed_texture()`.
///
/// `logical_size` sets the map's size. `physical_width` and `physical_height`
/// give the textures' size in pixels, which is usually `logical_size` times the
/// map's scale factor. The textures must all have that size, one device, one
/// pixel format, one sample, and MTLTextureUsageRenderTarget.
///
/// The session uses the textures until a replacement or release of the target
/// completes, a detach or release of the session completes, or an abandon of it
/// returns.
///
/// See `mln_metal_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct MetalBorrowedTextureDescriptor {
    pub logical_size: LogicalSize,
    pub physical_width: u32,
    pub physical_height: u32,
    pub textures: Vec<MetalBorrowedTexture>,
    /// Your MTLCommandQueue on the textures' device, or null for a queue of the
    /// session's own. With your queue, frames can use
    /// `GpuSyncKind::QueueOrder`.
    pub command_queue: *mut std::ffi::c_void,
}
impl MetalBorrowedTextureDescriptor {
    pub fn new(
        logical_size: LogicalSize,
        physical_width: u32,
        physical_height: u32,
        textures: Vec<MetalBorrowedTexture>,
    ) -> Self {
        Self {
            logical_size,
            physical_width,
            physical_height,
            textures,
            command_queue: Default::default(),
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
        raw.logical_size = to_native(&self.logical_size, arena)?;
        raw.physical_width = self.physical_width;
        raw.physical_height = self.physical_height;
        raw.textures = convert::array(&self.textures, arena)?;
        raw.texture_count = convert::count(self.textures.len())?;
        raw.command_queue = self.command_queue;
        Ok(raw)
    }
}

/// New Metal textures for a borrowed ring, for
/// `RenderSessionHandle::set_metal_borrowed_texture_target()`.
///
/// The members mean what they mean in `MetalBorrowedTextureDescriptor`, and the
/// textures follow its rules. The ring keeps its depth, device, and command
/// queue, so give it as many textures as it attached with, on the same device.
/// The session uses the textures until a replacement or release of the target
/// completes, a detach or release of the session completes, or an abandon of it
/// returns.
///
/// See `mln_metal_borrowed_texture_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct MetalBorrowedTextureReplacement {
    pub logical_size: LogicalSize,
    pub physical_width: u32,
    pub physical_height: u32,
    pub textures: Vec<MetalBorrowedTexture>,
}
impl MetalBorrowedTextureReplacement {
    pub fn new(
        logical_size: LogicalSize,
        physical_width: u32,
        physical_height: u32,
        textures: Vec<MetalBorrowedTexture>,
    ) -> Self {
        Self {
            logical_size,
            physical_width,
            physical_height,
            textures,
        }
    }
}
impl ToNative<sys::mln_metal_borrowed_texture_replacement> for MetalBorrowedTextureReplacement {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_metal_borrowed_texture_replacement> {
        let mut raw: sys::mln_metal_borrowed_texture_replacement = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_metal_borrowed_texture_replacement>() as _;
        raw.logical_size = to_native(&self.logical_size, arena)?;
        raw.physical_width = self.physical_width;
        raw.physical_height = self.physical_height;
        raw.textures = convert::array(&self.textures, arena)?;
        raw.texture_count = convert::count(self.textures.len())?;
        Ok(raw)
    }
}

/// The Metal device that a render session uses.
///
/// See `mln_metal_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MetalContextDescriptor {
    /// An `MTLDevice`.
    pub device: *mut std::ffi::c_void,
}
impl ToNative<sys::mln_metal_context_descriptor> for MetalContextDescriptor {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_metal_context_descriptor> {
        let mut raw: sys::mln_metal_context_descriptor = unsafe { std::mem::zeroed() };
        raw.device = self.device;
        Ok(raw)
    }
}

/// A Metal layer to render to, for `MapHandle::attach_metal_surface()`.
///
/// The session sets the layer's device, pixel format, and drawable size. It
/// uses the layer until a replacement or release of the target completes, a
/// detach or disposal of the session completes, or an abandon of it returns.
///
/// See `mln_metal_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalSurfaceDescriptor {
    pub logical_size: LogicalSize,
    /// The MTLDevice to render with. A null device picks the system's default.
    pub context: MetalContextDescriptor,
    /// Your CAMetalLayer.
    pub layer: *mut std::ffi::c_void,
}
impl MetalSurfaceDescriptor {
    pub fn new(logical_size: LogicalSize, layer: *mut std::ffi::c_void) -> Self {
        Self {
            logical_size,
            context: Default::default(),
            layer,
        }
    }
}
impl ToNative<sys::mln_metal_surface_descriptor> for MetalSurfaceDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_metal_surface_descriptor> {
        let mut raw: sys::mln_metal_surface_descriptor = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_metal_surface_descriptor>() as _;
        raw.logical_size = to_native(&self.logical_size, arena)?;
        raw.context = to_native(&self.context, arena)?;
        raw.layer = self.layer;
        Ok(raw)
    }
}

/// A new Metal layer for `RenderSessionHandle::set_metal_surface_target()`.
///
/// The session renders to it with the device that it attached with. It uses the
/// layer until a replacement or release of the target completes, a detach or
/// release of the session completes, or an abandon of it returns.
///
/// See `mln_metal_surface_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalSurfaceReplacement {
    pub logical_size: LogicalSize,
    /// Your CAMetalLayer.
    pub layer: *mut std::ffi::c_void,
}
impl MetalSurfaceReplacement {
    pub fn new(logical_size: LogicalSize, layer: *mut std::ffi::c_void) -> Self {
        Self {
            logical_size,
            layer,
        }
    }
}
impl ToNative<sys::mln_metal_surface_replacement> for MetalSurfaceReplacement {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_metal_surface_replacement> {
        let mut raw: sys::mln_metal_surface_replacement = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_metal_surface_replacement>() as _;
        raw.logical_size = to_native(&self.logical_size, arena)?;
        raw.layer = self.layer;
        Ok(raw)
    }
}

/// The Metal texture of an acquired frame.
///
/// See `mln_metal_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct MetalTextureFrame {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    /// The texture's index in the descriptor's `textures`.
    pub slot: u32,
    /// The MTLTexture.
    pub texture: *mut std::ffi::c_void,
    /// The texture's MTLDevice.
    pub device: *mut std::ffi::c_void,
    /// The texture's MTLPixelFormat.
    pub pixel_format: u64,
}
impl FromNative<sys::mln_metal_texture_frame> for MetalTextureFrame {
    unsafe fn from_native(raw: sys::mln_metal_texture_frame) -> Result<Self> {
        Ok(Self {
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            slot: raw.slot,
            texture: raw.texture,
            device: raw.device,
            pixel_format: raw.pixel_format,
        })
    }
}

native_enum! {
/// Whether the library may use the network.
///
/// See `mln_network_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub enum NetworkStatus: u32 {
    Online = 1,
    Offline = 2,
} Unknown
}

native_enum! {
/// The screen direction that north points to when the bearing is 0.
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

/// An offline region that covers a GeoJSON geometry.
///
/// The members other than `geometry` mean the same as those of
/// `OfflineTilePyramidRegionDefinition`.
///
/// See `mln_offline_geometry_region_definition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct OfflineGeometryRegionDefinition {
    pub style_url: String,
    pub geometry: String,
    pub min_zoom: f64,
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

/// The area, zoom range, and style that an offline region covers.
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
/// Whether an offline region is downloading.
///
/// See `mln_offline_region_download_state` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
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

/// An offline region in the runtime's cache file.
///
/// See `mln_offline_region_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct OfflineRegionInfo {
    pub id: i64,
    pub definition: OfflineRegionDefinition,
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

/// The download progress of an offline region.
///
/// See `mln_offline_region_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct OfflineRegionStatus {
    pub download_state: OfflineRegionDownloadState,
    /// The resources downloaded so far, tiles included.
    pub completed_resource_count: u64,
    /// The size of the downloaded resources in bytes, tiles included.
    pub completed_resource_size: u64,
    pub completed_tile_count: u64,
    pub required_tile_count: u64,
    /// The size of the downloaded tiles in bytes.
    pub completed_tile_size: u64,
    /// The resources that the region needs, tiles included. Until
    /// `required_resource_count_is_precise` is true, this is only a lower
    /// bound.
    pub required_resource_count: u64,
    pub required_resource_count_is_precise: bool,
    /// Whether the region has downloaded every resource it needs.
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

/// An offline region that covers a rectangle of the map.
///
/// See `mln_offline_tile_pyramid_region_definition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct OfflineTilePyramidRegionDefinition {
    /// The style whose resources the region holds. Maps use the region when
    /// they load the same style URL.
    pub style_url: String,
    pub bounds: LatLngBounds,
    pub min_zoom: f64,
    /// INFINITY covers every zoom level that the style's sources have.
    pub max_zoom: f64,
    /// The pixel ratio of the images to download, such as 2 for dense screens.
    pub pixel_ratio: f32,
    /// Whether the region holds the glyphs of CJK ideographs.
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

/// The image that `MapHandle::attach_offscreen()` renders. The session renders
/// it at the map's scale factor.
///
/// See `mln_offscreen_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct OffscreenDescriptor {
    pub logical_size: LogicalSize,
}
impl OffscreenDescriptor {
    pub const fn new(logical_size: LogicalSize) -> Self {
        Self { logical_size }
    }
}
impl ToNative<sys::mln_offscreen_descriptor> for OffscreenDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_offscreen_descriptor> {
        let mut raw: sys::mln_offscreen_descriptor = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_offscreen_descriptor>() as _;
        raw.logical_size = to_native(&self.logical_size, arena)?;
        Ok(raw)
    }
}

/// Options for `OffscreenDeviceHandle::create()`.
///
/// See `mln_offscreen_device_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct OffscreenDeviceOptions {
    /// The GPU to use, as an index into the platform's list of GPUs.
    pub adapter_index: Option<u32>,
    /// Whether the device's sessions take turns in one OpenGL context instead
    /// of having one each. Only an EGL device reads it.
    pub single_context: Option<bool>,
    /// The OpenGL API that the device's sessions render with. An EGL device
    /// defaults to `OpenGlClientApi::Gles`, and takes `OpenGlClientApi::Gl` too
    /// on Linux and FreeBSD. A WGL device takes only `OpenGlClientApi::Gl`, its
    /// default, and a WebGL device only `OpenGlClientApi::Gles`, its default.
    /// Other devices take neither.
    pub client_api: Option<OpenGlClientApi>,
}
impl ToNative<sys::mln_offscreen_device_options> for OffscreenDeviceOptions {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_offscreen_device_options> {
        let mut raw: sys::mln_offscreen_device_options = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_offscreen_device_options>() as _;
        raw.fields = 0;
        if let Some(item) = &self.adapter_index {
            raw.fields |= sys::MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX;
            raw.adapter_index = *item;
        }
        if let Some(item) = &self.single_context {
            raw.fields |= sys::MLN_OFFSCREEN_DEVICE_OPTION_SINGLE_CONTEXT;
            raw.single_context = *item;
        }
        if let Some(item) = &self.client_api {
            raw.fields |= sys::MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API;
            raw.client_api = to_native(&*item, arena)?;
        }
        Ok(raw)
    }
}

/// An OpenGL texture of a borrowed ring.
///
/// See `mln_open_gl_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct OpenGlBorrowedTexture {
    /// Your texture name.
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

/// A ring of your OpenGL textures, for
/// `MapHandle::attach_open_gl_borrowed_texture()`.
///
/// `logical_size` sets the map's size. `physical_width` and `physical_height`
/// give the textures' size in pixels, which is usually `logical_size` times the
/// map's scale factor. The context must use `OpenGlContextOwnership::Shared`.
/// Each texture must be in your context's share group, with storage of that
/// size.
///
/// The session uses the textures until a replacement or release of the target
/// completes, a detach or release of the session completes, or an abandon of it
/// returns.
///
/// See `mln_open_gl_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct OpenGlBorrowedTextureDescriptor<'a> {
    pub logical_size: LogicalSize,
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: OpenGlContextDescriptor<'a>,
    pub textures: Vec<OpenGlBorrowedTexture>,
    /// The textures' target. It must be GL_TEXTURE_2D.
    pub target: u32,
}
impl<'a> OpenGlBorrowedTextureDescriptor<'a> {
    pub fn new(
        logical_size: LogicalSize,
        physical_width: u32,
        physical_height: u32,
        context: OpenGlContextDescriptor<'a>,
        textures: Vec<OpenGlBorrowedTexture>,
        target: u32,
    ) -> Self {
        Self {
            logical_size,
            physical_width,
            physical_height,
            context,
            textures,
            target,
        }
    }
}
impl<'a> ToNative<sys::mln_open_gl_borrowed_texture_descriptor>
    for OpenGlBorrowedTextureDescriptor<'a>
{
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_open_gl_borrowed_texture_descriptor> {
        let mut raw: sys::mln_open_gl_borrowed_texture_descriptor = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_open_gl_borrowed_texture_descriptor>() as _;
        raw.logical_size = to_native(&self.logical_size, arena)?;
        raw.physical_width = self.physical_width;
        raw.physical_height = self.physical_height;
        raw.context = to_native(&self.context, arena)?;
        raw.textures = convert::array(&self.textures, arena)?;
        raw.texture_count = convert::count(self.textures.len())?;
        raw.target = self.target;
        Ok(raw)
    }
}

/// New OpenGL textures for a borrowed ring, for
/// `RenderSessionHandle::set_open_gl_borrowed_texture_target()`.
///
/// The members mean what they mean in `OpenGlBorrowedTextureDescriptor`, and
/// the textures follow its rules. The ring keeps its depth, context, and
/// texture target, so give it as many textures as it attached with, in the same
/// share group. The session uses the textures until a replacement or release of
/// the target completes, a detach or release of the session completes, or an
/// abandon of it returns.
///
/// See `mln_open_gl_borrowed_texture_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct OpenGlBorrowedTextureReplacement {
    pub logical_size: LogicalSize,
    pub physical_width: u32,
    pub physical_height: u32,
    pub textures: Vec<OpenGlBorrowedTexture>,
}
impl OpenGlBorrowedTextureReplacement {
    pub fn new(
        logical_size: LogicalSize,
        physical_width: u32,
        physical_height: u32,
        textures: Vec<OpenGlBorrowedTexture>,
    ) -> Self {
        Self {
            logical_size,
            physical_width,
            physical_height,
            textures,
        }
    }
}
impl ToNative<sys::mln_open_gl_borrowed_texture_replacement> for OpenGlBorrowedTextureReplacement {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_open_gl_borrowed_texture_replacement> {
        let mut raw: sys::mln_open_gl_borrowed_texture_replacement = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_open_gl_borrowed_texture_replacement>() as _;
        raw.logical_size = to_native(&self.logical_size, arena)?;
        raw.physical_width = self.physical_width;
        raw.physical_height = self.physical_height;
        raw.textures = convert::array(&self.textures, arena)?;
        raw.texture_count = convert::count(self.textures.len())?;
        Ok(raw)
    }
}

native_enum! {
/// The OpenGL API that a context renders with.
///
/// See `mln_open_gl_client_api` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub enum OpenGlClientApi: u32 {
    Unspecified = 0,
    Gl = 1,
    Gles = 2,
} Unknown
}
impl Default for OpenGlClientApi {
    fn default() -> Self {
        Self::Unspecified
    }
}

/// How a render session gets the OpenGL context that it renders with.
///
/// Keep the objects that it names alive until a detach or disposal of the
/// session completes, or an abandon of it returns.
///
/// See `mln_open_gl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct OpenGlContextDescriptor<'a> {
    pub ownership: OpenGlContextOwnership,
    pub data: OpenGlContextDescriptorData<'a>,
}
impl<'a> OpenGlContextDescriptor<'a> {
    pub fn new(data: OpenGlContextDescriptorData<'a>) -> Self {
        Self {
            ownership: Default::default(),
            data,
        }
    }
}
impl<'a> ToNative<sys::mln_open_gl_context_descriptor> for OpenGlContextDescriptor<'a> {
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

/// The WGL, EGL, or WebGL part of an OpenGL context descriptor.
///
/// See `mln_open_gl_context_descriptor_data` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub enum OpenGlContextDescriptorData<'a> {
    Wgl(WglContextDescriptor),
    Egl(EglContextDescriptor),
    WebGl(WebGlContextDescriptor<'a>),
}

native_enum! {
/// Whether a render session's OpenGL context shares objects with yours.
///
/// See `mln_open_gl_context_ownership` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub enum OpenGlContextOwnership: u32 {
    /// The session's context shares objects with your `share_context`.
    Shared = 0,
    /// The session's context shares objects with no other context.
    Dedicated = 1,
} Unknown
}
impl Default for OpenGlContextOwnership {
    fn default() -> Self {
        Self::Shared
    }
}

native_flags! {
/// An OpenGL context platform, as a bit in a set.
///
/// See `mln_open_gl_context_provider_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub struct OpenGlContextProviderFlag: u32 {
    const WGL = 1;
    const EGL = 2;
    const WEB_GL = 4;
}
}

/// An OpenGL surface to present to, for `MapHandle::attach_open_gl_surface()`.
///
/// The session uses the surface until a replacement or release of the target
/// completes, a detach or release of the session completes, or an abandon of it
/// returns.
///
/// See `mln_open_gl_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct OpenGlSurfaceDescriptor<'a> {
    pub logical_size: LogicalSize,
    pub context: OpenGlContextDescriptor<'a>,
    /// Your EGLSurface with EGL, or your HDC with WGL. WebGL doesn't use it.
    pub surface: *mut std::ffi::c_void,
}
impl<'a> OpenGlSurfaceDescriptor<'a> {
    pub fn new(logical_size: LogicalSize, context: OpenGlContextDescriptor<'a>) -> Self {
        Self {
            logical_size,
            context,
            surface: Default::default(),
        }
    }
}
impl<'a> ToNative<sys::mln_open_gl_surface_descriptor> for OpenGlSurfaceDescriptor<'a> {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_open_gl_surface_descriptor> {
        let mut raw: sys::mln_open_gl_surface_descriptor = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_open_gl_surface_descriptor>() as _;
        raw.logical_size = to_native(&self.logical_size, arena)?;
        raw.context = to_native(&self.context, arena)?;
        raw.surface = self.surface;
        Ok(raw)
    }
}

/// A new OpenGL surface for
/// `RenderSessionHandle::set_open_gl_surface_target()`.
///
/// The session presents to it with the context that it attached with. It uses
/// the surface until a replacement or release of the target completes, a detach
/// or disposal of the session completes, or an abandon of it returns.
///
/// See `mln_open_gl_surface_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct OpenGlSurfaceReplacement {
    pub logical_size: LogicalSize,
    /// Your EGLSurface with EGL, or your HDC with WGL. WebGL doesn't use it, so
    /// leave it null.
    pub surface: *mut std::ffi::c_void,
}
impl OpenGlSurfaceReplacement {
    pub fn new(logical_size: LogicalSize) -> Self {
        Self {
            logical_size,
            surface: Default::default(),
        }
    }
}
impl ToNative<sys::mln_open_gl_surface_replacement> for OpenGlSurfaceReplacement {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_open_gl_surface_replacement> {
        let mut raw: sys::mln_open_gl_surface_replacement = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_open_gl_surface_replacement>() as _;
        raw.logical_size = to_native(&self.logical_size, arena)?;
        raw.surface = self.surface;
        Ok(raw)
    }
}

/// The OpenGL texture of an acquired frame.
///
/// See `mln_open_gl_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct OpenGlTextureFrame {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    /// The texture's index in the descriptor's `textures`.
    pub slot: u32,
    pub texture: u32,
    pub target: u32,
}
impl FromNative<sys::mln_open_gl_texture_frame> for OpenGlTextureFrame {
    unsafe fn from_native(raw: sys::mln_open_gl_texture_frame) -> Result<Self> {
        Ok(Self {
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            slot: raw.slot,
            texture: raw.texture,
            target: raw.target,
        })
    }
}

/// An image of 8-bit RGBA pixels whose colors are multiplied by their alpha.
///
/// See `mln_premultiplied_rgba8_image` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct PremultipliedRgba8Image<'a> {
    pub width: u32,
    pub height: u32,
    /// The number of bytes from the start of one row to the start of the next,
    /// at least `width` \* 4.
    pub stride: u32,
    /// The rows of pixels, `stride` bytes apart, from the top row down.
    pub pixels: &'a [u8],
}
impl<'a> PremultipliedRgba8Image<'a> {
    pub fn new(width: u32, height: u32, stride: u32, pixels: &'a [u8]) -> Self {
        Self {
            width,
            height,
            stride,
            pixels,
        }
    }
}
impl<'a> ToNative<sys::mln_premultiplied_rgba8_image> for PremultipliedRgba8Image<'a> {
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

/// A point in the Web Mercator projection, in meters.
///
/// See `mln_projected_meters` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct ProjectedMeters {
    pub northing: f64,
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

/// A map's whole projection mode.
///
/// Its members mean the same as those of `ProjectionModeOptions`.
///
/// See `mln_projection_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct ProjectionMode {
    pub axonometric: bool,
    pub x_skew: f64,
    pub y_skew: f64,
}
impl ProjectionMode {
    pub const fn new(axonometric: bool, x_skew: f64, y_skew: f64) -> Self {
        Self {
            axonometric,
            x_skew,
            y_skew,
        }
    }
}
impl ToNative<sys::mln_projection_mode> for ProjectionMode {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_projection_mode> {
        let mut raw: sys::mln_projection_mode = unsafe { std::mem::zeroed() };
        raw.axonometric = self.axonometric;
        raw.x_skew = self.x_skew;
        raw.y_skew = self.y_skew;
        Ok(raw)
    }
}
impl FromNative<sys::mln_projection_mode> for ProjectionMode {
    unsafe fn from_native(raw: sys::mln_projection_mode) -> Result<Self> {
        Ok(Self {
            axonometric: raw.axonometric,
            x_skew: raw.x_skew,
            y_skew: raw.y_skew,
        })
    }
}

/// Settings that skew the map or draw it without perspective.
///
/// See `mln_projection_mode_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct ProjectionModeOptions {
    /// Whether the map draws without perspective.
    pub axonometric: Option<bool>,
    pub x_skew: Option<f64>,
    pub y_skew: Option<f64>,
}
impl ToNative<sys::mln_projection_mode_options> for ProjectionModeOptions {
    fn to_native(&self, _arena: &mut InputArena) -> Result<sys::mln_projection_mode_options> {
        let mut raw: sys::mln_projection_mode_options = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_projection_mode_options>() as _;
        raw.fields = 0;
        if let Some(item) = &self.axonometric {
            raw.fields |= sys::MLN_PROJECTION_MODE_OPTION_AXONOMETRIC;
            raw.axonometric = *item;
        }
        if let Some(item) = &self.x_skew {
            raw.fields |= sys::MLN_PROJECTION_MODE_OPTION_X_SKEW;
            raw.x_skew = *item;
        }
        if let Some(item) = &self.y_skew {
            raw.fields |= sys::MLN_PROJECTION_MODE_OPTION_Y_SKEW;
            raw.y_skew = *item;
        }
        Ok(raw)
    }
}

/// A rotation, as a quaternion.
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

/// Your lock on a Vulkan queue that you and a render session both submit to.
///
/// The session holds the lock only while it submits or presents to the queue.
/// Set both `lock` and `unlock`, or neither when you don't submit to the queue
/// yourself.
///
/// It can run on a library thread, and it can't call the library.
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
/// What `RenderSessionHandle::abandon()` did with a session's graphics objects.
///
/// See `mln_render_abandon_disposition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
pub enum RenderAbandonDisposition: u32 {
    Clean = 0,
    /// It kept some objects that it couldn't destroy safely. They stay
    /// allocated until the process exits.
    Quarantined = 1,
} Unknown
}
impl Default for RenderAbandonDisposition {
    fn default() -> Self {
        Self::Clean
    }
}

/// The outcome of `RenderSessionHandle::abandon()`.
///
/// See `mln_render_abandon_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RenderAbandonResult {
    pub disposition: RenderAbandonDisposition,
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
/// A render backend, as a bit in a set.
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
/// The thread that runs a render session's graphics work.
///
/// See `mln_render_driver_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub enum RenderDriverKind: u32 {
    /// A library thread.
    CoreWorker = 1,
    /// Your thread, when it calls `RenderSessionHandle::service_driver_work()`.
    CallerGraphicsThread = 2,
} Unknown
}

/// How one frame demand finished.
///
/// See `mln_render_frame_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RenderFrameResult {
    pub disposition: RenderResult,
    pub token: u64,
    /// The map update that a rendered frame shows. The frame includes a
    /// command's change when this is at least the command's
    /// `CommandCompletion::render_update_generation`.
    pub map_update_generation: u64,
    pub generation: u64,
    pub frame_generation: u64,
    pub needs_repaint: bool,
    /// The camera and extent that a rendered frame shows.
    pub view: MapView,
    pub mode: RenderMode,
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
            mode: RenderMode::from_raw(raw.mode),
        })
    }
}

native_enum! {
/// Whether a frame drew the map complete, or lacked resources that are still
/// loading.
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
/// How a frame demand finished.
///
/// See `mln_render_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
pub enum RenderResult: u32 {
    Rendered = 0,
    /// The map had nothing new to draw.
    NoUpdate = 1,
    /// The target couldn't take a frame, such as while a window is hidden or
    /// after `RenderSessionHandle::release_target()`. Demand a frame again
    /// later.
    TargetNotReady = 2,
    /// A later demand with the same flags replaced this one.
    Superseded = 3,
    /// The session failed, and `RenderSessionHandle::get_failure()` reports
    /// why. Once `RenderSessionHandle::release()` completes, you can attach a
    /// new session.
    Failed = 4,
} Unknown
}
impl Default for RenderResult {
    fn default() -> Self {
        Self::Rendered
    }
}

/// Options for attaching a render session to a map.
///
/// See `mln_render_session_attach_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone)]
pub struct RenderSessionAttachOptions {
    /// The thread that runs the session's graphics work. Whoever owns the
    /// target's graphics context drives it:
    ///
    /// - `RenderDriverKind::CallerGraphicsThread` for an OpenGL or WebGPU
    ///   target that you pass, since its context belongs to your thread: a
    ///   shared or dedicated EGL or WGL context, an existing WebGL context, or
    ///   a browser's WebGPU device.
    /// - `RenderDriverKind::CoreWorker` for a target whose context the library
    ///   owns: a transferred WebGL canvas, or an offscreen session.
    /// - Either one for a Metal or Vulkan target.
    ///
    /// An attach with another driver fails with `Status::Unsupported`.
    pub driver: RenderDriverKind,
    /// The kinds of `GpuSync` that you can wait on before you read an acquired
    /// frame. The session uses one of them that its target supports. Zero lets
    /// the session pick a kind that names no object to wait on.
    pub producer_sync_kinds: GpuSyncKindFlag,
    /// Called when frame results are waiting for
    /// `RenderSessionHandle::drain_frame_results()`. It isn't called again
    /// until a drain takes them.
    pub frame_wake: Wake,
    /// Called when a session driven by `RenderDriverKind::CallerGraphicsThread`
    /// has work for `RenderSessionHandle::service_driver_work()`. It isn't
    /// called again until a service runs.
    pub driver_work_wake: Wake,
    /// Your lock on the Vulkan queue, when you submit to it yourself.
    pub queue_lock: QueueLock,
}
impl RenderSessionAttachOptions {
    pub fn new(driver: RenderDriverKind) -> Self {
        Self {
            driver,
            producer_sync_kinds: Default::default(),
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
        raw.producer_sync_kinds = to_native(&self.producer_sync_kinds, arena)?;
        raw.frame_wake = to_native(&self.frame_wake, arena)?;
        raw.driver_work_wake = to_native(&self.driver_work_wake, arena)?;
        raw.queue_lock = to_native(&self.queue_lock, arena)?;
        Ok(raw)
    }
}

/// What an attached render session supports.
///
/// See `mln_render_session_capabilities` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct RenderSessionCapabilities {
    pub driver: RenderDriverKind,
    /// How many textures the session's texture ring has.
    pub texture_ring_depth: u32,
    pub flags: RenderSessionCapabilityFlag,
    /// The kind of `GpuSync` that `AcquiredFrameHandle::get_producer_sync()`
    /// reports.
    pub producer_sync_kind: GpuSyncKind,
    /// The kinds of `GpuSync` that `AcquiredFrameHandle::release()` accepts.
    ///
    /// They depend on the target and on what the device supports, so read this
    /// field instead of assuming a kind.
    pub consumer_sync_kinds: GpuSyncKindFlag,
}
impl FromNative<sys::mln_render_session_capabilities> for RenderSessionCapabilities {
    unsafe fn from_native(raw: sys::mln_render_session_capabilities) -> Result<Self> {
        Ok(Self {
            driver: RenderDriverKind::from_raw(raw.driver),
            texture_ring_depth: raw.texture_ring_depth,
            flags: RenderSessionCapabilityFlag::from_bits_retain(raw.flags),
            producer_sync_kind: GpuSyncKind::from_raw(raw.producer_sync_kind),
            consumer_sync_kinds: GpuSyncKindFlag::from_bits_retain(raw.consumer_sync_kinds),
        })
    }
}

native_flags! {
/// Something that a render session can do.
///
/// See `mln_render_session_capability_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub struct RenderSessionCapabilityFlag: u32 {
    /// `RenderSessionHandle::acquire_frame()` can take the frames it renders.
    const FRAME_ACQUISITION = 1;
    /// `RenderSessionHandle::read_texture()` can copy its texture to memory.
    const READBACK = 2;
    /// It shows frames on a surface.
    const PRESENTATION = 4;
}
}

/// The state of a render session at one moment.
///
/// See `mln_render_session_snapshot` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct RenderSessionSnapshot {
    pub state: RenderSessionState,
    /// The target's logical size, and the map's scale factor.
    pub extent: LogicalExtent,
    pub generation: u64,
    pub frame_generation: u64,
    pub acquired_frame_count: u32,
    /// `Status::Ok` until the session fails. Then `Status::TargetLost` when it
    /// failed because it lost its target, or `Status::NativeError` when it
    /// failed for another reason.
    pub failure_status: Status,
    /// Whether the session is attached and has a target to render to. It's
    /// false from when `RenderSessionHandle::release_target()` completes until
    /// you give the session a new target.
    pub has_target: bool,
}
impl FromNative<sys::mln_render_session_snapshot> for RenderSessionSnapshot {
    unsafe fn from_native(raw: sys::mln_render_session_snapshot) -> Result<Self> {
        Ok(Self {
            state: RenderSessionState::from_raw(raw.state),
            extent: unsafe { from_native(raw.extent) }?,
            generation: raw.generation,
            frame_generation: raw.frame_generation,
            acquired_frame_count: raw.acquired_frame_count,
            failure_status: Status::from_raw(raw.failure_status),
            has_target: raw.has_target,
        })
    }
}

native_enum! {
/// Where a render session is in its life.
///
/// See `mln_render_session_state` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
pub enum RenderSessionState: u32 {
    Attaching = 1,
    Attached = 2,
    Detaching = 3,
    Detached = 4,
    Failed = 5,
    Abandoned = 6,
} Unknown
}

/// A feature that a rendered feature query found.
///
/// See `mln_rendered_feature` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct RenderedFeature {
    /// The feature, as a GeoJSON feature.
    pub feature: String,
    /// The layer that drew the feature.
    pub layer_id: String,
    pub source_id: String,
    /// The feature's source layer, or empty when its source has none.
    pub source_layer_id: String,
    /// The feature's state, as a JSON object.
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

/// Options for a rendered feature query.
///
/// See `mln_rendered_feature_query_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct RenderedFeatureQueryOptions<'a> {
    /// The layers to query. Without them, the query covers every layer.
    pub layer_ids: Option<&'a [&'a str]>,
    /// A \[filter\](https://maplibre.org/maplibre-style-spec/expressions/) that
    /// each feature must match. Empty matches every feature.
    pub filter: &'a str,
}
impl<'a> ToNative<sys::mln_rendered_feature_query_options> for RenderedFeatureQueryOptions<'a> {
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

/// The screen area that a rendered feature query covers.
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

/// Statistics about the frames that a map renders.
///
/// See `mln_rendering_stats` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RenderingStats {
    /// The time that the frame took to encode, in milliseconds.
    pub encoding_time_ms: f64,
    /// The time that the frame took to render, in milliseconds.
    pub rendering_time_ms: f64,
    /// The frames rendered so far.
    pub frame_count: i64,
    /// The draw calls in the frame.
    pub draw_call_count: i64,
    /// The draw calls in every frame so far.
    pub total_draw_call_count: i64,
}
impl FromNative<sys::mln_rendering_stats> for RenderingStats {
    unsafe fn from_native(raw: sys::mln_rendering_stats) -> Result<Self> {
        Ok(Self {
            encoding_time_ms: raw.encoding_time_ms,
            rendering_time_ms: raw.rendering_time_ms,
            frame_count: raw.frame_count,
            draw_call_count: raw.draw_call_count,
            total_draw_call_count: raw.total_draw_call_count,
        })
    }
}

native_enum! {
/// Why a resource request failed.
///
/// See `mln_resource_error_reason` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub enum ResourceErrorReason: u32 {
    /// A reason that no other value names.
    Other = 0,
    NotFound = 1,
    /// The server reported an error.
    Server = 2,
    /// The server couldn't be reached.
    Connection = 3,
    RateLimit = 4,
} Unknown
}
impl Default for ResourceErrorReason {
    fn default() -> Self {
        Self::Other
    }
}

native_enum! {
/// The kind of a resource that the library loads.
///
/// See `mln_resource_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
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
/// A resource kind, as a bit in a set.
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
/// Where a request may load its resource from.
///
/// See `mln_resource_loading_method` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub enum ResourceLoadingMethod: u32 {
    /// The cache or the network.
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
/// The priority of a resource request.
///
/// See `mln_resource_priority` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
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

/// A resource provider, for `RuntimeHandle::set_resource_provider()`.
///
/// See `mln_resource_provider` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Clone)]
pub struct ResourceProvider {
    /// The callback receives (`request`, `handle`).
    pub callback:
        std::sync::Arc<dyn Fn(ResourceRequestInfo, ResourceRequestHandle) + Send + Sync + 'static>,
    /// The requests that the provider serves. Without routes, it serves every
    /// network request.
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
            let handle = ResourceRequestHandle::adopt(handle)?;
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

/// The bytes from `start` to `end` of a resource, both included.
///
/// See `mln_resource_range` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct ResourceRange {
    pub start: u64,
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

/// A cancel callback, for `ResourceRequestHandle::set_cancel_callback()`.
///
/// See `mln_resource_request_cancel_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Clone)]
pub struct ResourceRequestCancelHandler {
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

/// A request that a resource provider serves.
///
/// When the cache holds an earlier copy of the resource, the `prior_` fields
/// describe it.
///
/// See `mln_resource_request_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct ResourceRequestInfo {
    /// The URL as the style names it.
    pub requested_url: String,
    /// The URL to load, after the library expands any shorthand in
    /// `requested_url` and before any resource transform. Routes match this
    /// URL.
    pub resolved_url: String,
    pub kind: ResourceKind,
    pub loading_method: ResourceLoadingMethod,
    pub priority: ResourcePriority,
    pub usage: ResourceUsage,
    pub storage_policy: ResourceStoragePolicy,
    /// The only bytes of the resource that the request wants.
    pub range: Option<ResourceRange>,
    pub prior_modified_unix_ms: Option<i64>,
    pub prior_expires_unix_ms: Option<i64>,
    /// Empty when the earlier copy has no ETag.
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

/// The answer to a resource request.
///
/// See `mln_resource_response` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct ResourceResponse<'a> {
    pub status: ResourceResponseStatus,
    /// Why the request failed, for `ResourceResponseStatus::Error`.
    pub error_reason: ResourceErrorReason,
    /// The resource, for `ResourceResponseStatus::Ok`.
    pub bytes: &'a [u8],
    /// A description of the error, for `ResourceResponseStatus::Error`.
    pub error_message: &'a str,
    /// Whether the library must request the resource again before it reuses
    /// this copy.
    pub must_revalidate: bool,
    pub modified_unix_ms: Option<i64>,
    pub expires_unix_ms: Option<i64>,
    /// Empty when the resource has no ETag.
    pub etag: &'a str,
    /// When to retry, for `ResourceResponseStatus::Error`.
    pub retry_after_unix_ms: Option<i64>,
}
impl<'a> ToNative<sys::mln_resource_response> for ResourceResponse<'a> {
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
/// The outcome of a resource request.
///
/// See `mln_resource_response_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub enum ResourceResponseStatus: u32 {
    Ok = 0,
    Error = 1,
    /// The resource exists and is empty.
    NoContent = 2,
    /// The earlier copy that the request's `prior_` fields describe is current.
    NotModified = 3,
} Unknown
}
impl Default for ResourceResponseStatus {
    fn default() -> Self {
        Self::Ok
    }
}

/// The requests that a resource provider serves.
///
/// See `mln_resource_route` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct ResourceRoute {
    /// The kinds of resource that the route covers. Zero covers every kind.
    pub kinds: ResourceKindFlag,
    pub flags: UrlMatchFlag,
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
/// Whether the library keeps a loaded resource in its cache.
///
/// See `mln_resource_storage_policy` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
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

/// How to rewrite request URLs, for `RuntimeHandle::set_resource_transform()`.
///
/// Set either `callback` or `rules`, not both.
///
/// See `mln_resource_transform` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Clone, Default)]
pub struct ResourceTransform {
    /// The callback receives (`kind`, `url`, `request`).
    pub callback: Option<
        std::sync::Arc<
            dyn Fn(ResourceKind, &str, ResourceTransformRequestHandle) + Send + Sync + 'static,
        >,
    >,
    /// The first rule that matches a request adds its parameters, and no other.
    /// A request that no rule matches keeps its URL. Rules never fail a
    /// request.
    pub rules: Vec<ResourceTransformRule>,
}
impl std::fmt::Debug for ResourceTransform {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("ResourceTransform").finish_non_exhaustive()
    }
}
impl ResourceTransform {
    pub fn with_callback<F>(mut self, callback: F) -> Self
    where
        F: Fn(ResourceKind, &str, ResourceTransformRequestHandle) + Send + Sync + 'static,
    {
        self.callback = Some(std::sync::Arc::new(callback));
        self
    }
    pub fn new<F>(callback: F) -> Self
    where
        F: Fn(ResourceKind, &str, ResourceTransformRequestHandle) + Send + Sync + 'static,
    {
        Self::default().with_callback(callback)
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
            let request = ResourceTransformRequestHandle::adopt(request)?;
            callback::require(&state.callback)?(
                ResourceKind::from_raw(kind),
                unsafe { crate::string::borrow_string_view(url) }?,
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

/// Why a resource transform fails a request.
///
/// See `mln_resource_transform_error` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct ResourceTransformError<'a> {
    pub reason: ResourceErrorReason,
    pub message: &'a str,
    /// When to retry, for `ResourceErrorReason::RateLimit`.
    pub retry_after_unix_ms: Option<i64>,
}
impl<'a> ToNative<sys::mln_resource_transform_error> for ResourceTransformError<'a> {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_resource_transform_error> {
        let mut raw: sys::mln_resource_transform_error = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_resource_transform_error>() as _;
        raw.fields = 0;
        raw.reason = to_native(&self.reason, arena)?;
        raw.message = to_native(&self.message, arena)?;
        if let Some(item) = &self.retry_after_unix_ms {
            raw.fields |= sys::MLN_RESOURCE_TRANSFORM_ERROR_RETRY_AFTER;
            raw.retry_after_unix_ms = *item;
        }
        Ok(raw)
    }
}

/// Query parameters to add to the URLs of requests that match a pattern.
///
/// The library adds the parameters, in order, at the end of the URL's query and
/// before any fragment. It writes `?` before them when the URL has no `?`, or
/// `&` when the text before them ends with neither `?` nor `&`. It keeps the
/// parameters that the URL already has, even one with the same name.
///
/// See `mln_resource_transform_rule` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct ResourceTransformRule {
    /// The kinds of resource that the rule covers. Zero covers every kind.
    pub kinds: ResourceKindFlag,
    pub flags: UrlMatchFlag,
    pub url: String,
    pub parameters: Vec<UrlQueryParameter>,
}
impl ToNative<sys::mln_resource_transform_rule> for ResourceTransformRule {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_resource_transform_rule> {
        let mut raw: sys::mln_resource_transform_rule = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_resource_transform_rule>() as _;
        raw.kinds = to_native(&self.kinds, arena)?;
        raw.flags = to_native(&self.flags, arena)?;
        raw.url = to_native(&self.url, arena)?;
        raw.parameters = convert::array(&self.parameters, arena)?;
        raw.parameter_count = convert::count(self.parameters.len())?;
        Ok(raw)
    }
}

native_enum! {
/// Whether a request loads a resource for a map or an offline region.
///
/// See `mln_resource_usage` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
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

/// An event that a runtime or one of its maps raised.
///
/// See `mln_runtime_event` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct RuntimeEvent {
    pub r#type: RuntimeEventType,
    pub source_type: RuntimeEventSourceType,
    /// The runtime or map that raised the event, as `source_type` says.
    pub source: u64,
    /// For a map event, a map snapshot with this generation or a later one
    /// includes what the event reports. Zero for a runtime event.
    pub generation: u64,
    /// The error's text for an event that reports a failure, or empty.
    pub message: String,
    pub payload: RuntimeEventPayload,
}
impl FromNative<sys::mln_runtime_event> for RuntimeEvent {
    unsafe fn from_native(raw: sys::mln_runtime_event) -> Result<Self> {
        Ok(Self {
            r#type: RuntimeEventType::from_raw(raw.type_),
            source_type: RuntimeEventSourceType::from_raw(raw.source_type),
            source: raw.source,
            generation: raw.generation,
            message: unsafe { from_native(raw.message) }?,
            payload: match raw.payload_type {
                sys::MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_CHANGE => {
                    RuntimeEventPayload::CameraChange(unsafe {
                        from_native(raw.payload.camera_change)
                    }?)
                }
                sys::MLN_RUNTIME_EVENT_PAYLOAD_LOADING_FAILED => {
                    RuntimeEventPayload::LoadingFailed(unsafe {
                        from_native(raw.payload.loading_failed)
                    }?)
                }
                sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME => {
                    RuntimeEventPayload::RenderFrame(unsafe {
                        from_native(raw.payload.render_frame)
                    }?)
                }
                sys::MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE => {
                    RuntimeEventPayload::StyleImage(unsafe {
                        from_native(raw.payload.style_image)
                    }?)
                }
                sys::MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION => {
                    RuntimeEventPayload::TileAction(unsafe {
                        from_native(raw.payload.tile_action)
                    }?)
                }
                sys::MLN_RUNTIME_EVENT_PAYLOAD_SOURCE_CHANGED => {
                    RuntimeEventPayload::SourceChanged(unsafe {
                        from_native(raw.payload.source_changed)
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
                sys::MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS => {
                    RuntimeEventPayload::Glyphs(unsafe { from_native(raw.payload.glyphs) }?)
                }
                sys::MLN_RUNTIME_EVENT_PAYLOAD_SPRITE => {
                    RuntimeEventPayload::Sprite(unsafe { from_native(raw.payload.sprite) }?)
                }
                sys::MLN_RUNTIME_EVENT_PAYLOAD_NONE => RuntimeEventPayload::Empty,
                tag => RuntimeEventPayload::Unknown(tag as u32),
            },
        })
    }
}

/// The details of `RuntimeEventType::MapCameraWillChange` and
/// `RuntimeEventType::MapCameraDidChange`.
///
/// See `mln_runtime_event_camera_change` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventCameraChange {
    pub mode: CameraChangeMode,
}
impl FromNative<sys::mln_runtime_event_camera_change> for RuntimeEventCameraChange {
    unsafe fn from_native(raw: sys::mln_runtime_event_camera_change) -> Result<Self> {
        Ok(Self {
            mode: CameraChangeMode::from_raw(raw.mode),
        })
    }
}

native_flags! {
/// An event type, as a bit in a set.
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
    const MAP_GLYPHS_REQUESTED = 1048576;
    const MAP_GLYPHS_LOADED = 2097152;
    const MAP_GLYPHS_ERROR = 4194304;
    const MAP_SPRITE_REQUESTED = 8388608;
    const MAP_SPRITE_LOADED = 16777216;
    const MAP_SPRITE_ERROR = 33554432;
    const ALL_MAP_EVENTS = 66191358;
    const ALL_RUNTIME_EVENTS = 917504;
    const ALL = 67108862;
}
}

/// The details of a glyph event.
///
/// The event covers the glyphs from `first` to `last`, both included.
///
/// See `mln_runtime_event_glyphs` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct RuntimeEventGlyphs {
    /// The fonts that the glyphs come from, in order of preference.
    pub font_stack: Vec<String>,
    pub first: u32,
    pub last: u32,
    pub font_face: bool,
}
impl FromNative<sys::mln_runtime_event_glyphs> for RuntimeEventGlyphs {
    unsafe fn from_native(raw: sys::mln_runtime_event_glyphs) -> Result<Self> {
        Ok(Self {
            font_stack: unsafe { convert::copy_array(raw.font_stack, raw.font_count) }?,
            first: raw.first,
            last: raw.last,
            font_face: raw.font_face,
        })
    }
}

/// An event callback, for `RuntimeHandle::set_event_handler()`.
///
/// See `mln_runtime_event_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Clone)]
pub struct RuntimeEventHandler {
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

/// The details of `RuntimeEventType::MapLoadingFailed`.
///
/// See `mln_runtime_event_loading_failed` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventLoadingFailed {
    pub error: MapLoadError,
}
impl FromNative<sys::mln_runtime_event_loading_failed> for RuntimeEventLoadingFailed {
    unsafe fn from_native(raw: sys::mln_runtime_event_loading_failed) -> Result<Self> {
        Ok(Self {
            error: MapLoadError::from_raw(raw.error),
        })
    }
}

/// The details of `RuntimeEventType::OfflineRegionResponseError`.
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

/// The details of `RuntimeEventType::OfflineRegionStatusChanged`.
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

/// The details of `RuntimeEventType::OfflineRegionTileCountLimitExceeded`.
///
/// See `mln_runtime_event_offline_region_tile_count_limit` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventOfflineRegionTileCountLimit {
    pub region_id: i64,
    /// The number of demo server tiles that the cache file's offline regions
    /// may hold together.
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

/// The details of an event, in the member that its `payload_type` names.
///
/// See `mln_runtime_event_payload` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub enum RuntimeEventPayload {
    CameraChange(RuntimeEventCameraChange),
    LoadingFailed(RuntimeEventLoadingFailed),
    RenderFrame(RuntimeEventRenderFrame),
    StyleImage(RuntimeEventStyleImage),
    TileAction(RuntimeEventTileAction),
    SourceChanged(RuntimeEventSourceChanged),
    OfflineRegionStatus(RuntimeEventOfflineRegionStatus),
    OfflineRegionResponseError(RuntimeEventOfflineRegionResponseError),
    OfflineRegionTileCountLimit(RuntimeEventOfflineRegionTileCountLimit),
    Glyphs(RuntimeEventGlyphs),
    Sprite(RuntimeEventSprite),
    Empty,
    Unknown(u32),
}
impl Default for RuntimeEventPayload {
    fn default() -> Self {
        Self::Unknown(0)
    }
}

/// The details of `RuntimeEventType::MapRenderFrameFinished`.
///
/// See `mln_runtime_event_render_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct RuntimeEventRenderFrame {
    pub mode: RenderMode,
    pub needs_repaint: bool,
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

/// The details of `RuntimeEventType::MapSourceChanged`.
///
/// See `mln_runtime_event_source_changed` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct RuntimeEventSourceChanged {
    pub source_id: String,
}
impl FromNative<sys::mln_runtime_event_source_changed> for RuntimeEventSourceChanged {
    unsafe fn from_native(raw: sys::mln_runtime_event_source_changed) -> Result<Self> {
        Ok(Self {
            source_id: unsafe { from_native(raw.source_id) }?,
        })
    }
}

native_enum! {
/// The kind of handle that raised an event.
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

/// The details of a sprite event.
///
/// `id` and `url` are both empty when the style has no sprite.
///
/// See `mln_runtime_event_sprite` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct RuntimeEventSprite {
    pub id: String,
    pub url: String,
}
impl FromNative<sys::mln_runtime_event_sprite> for RuntimeEventSprite {
    unsafe fn from_native(raw: sys::mln_runtime_event_sprite) -> Result<Self> {
        Ok(Self {
            id: unsafe { from_native(raw.id) }?,
            url: unsafe { from_native(raw.url) }?,
        })
    }
}

/// The details of `RuntimeEventType::MapStyleImageMissing`.
///
/// See `mln_runtime_event_style_image` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct RuntimeEventStyleImage {
    pub image_id: String,
}
impl FromNative<sys::mln_runtime_event_style_image> for RuntimeEventStyleImage {
    unsafe fn from_native(raw: sys::mln_runtime_event_style_image) -> Result<Self> {
        Ok(Self {
            image_id: unsafe { from_native(raw.image_id) }?,
        })
    }
}

/// The details of `RuntimeEventType::MapTileAction`.
///
/// See `mln_runtime_event_tile_action` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct RuntimeEventTileAction {
    /// The source that the tile belongs to.
    pub source_id: String,
    pub operation: TileOperation,
    pub tile_id: TileId,
}
impl FromNative<sys::mln_runtime_event_tile_action> for RuntimeEventTileAction {
    unsafe fn from_native(raw: sys::mln_runtime_event_tile_action) -> Result<Self> {
        Ok(Self {
            source_id: unsafe { from_native(raw.source_id) }?,
            operation: TileOperation::from_raw(raw.operation),
            tile_id: unsafe { from_native(raw.tile_id) }?,
        })
    }
}

native_enum! {
/// The kind of an event.
///
/// An event's `payload_type` names the member of its payload that holds the
/// event's details. An event that reports a failure carries the error's text in
/// `message`.
///
/// Only maps in `MapMode::Continuous` raise
/// `RuntimeEventType::MapLoadingFinished`, `RuntimeEventType::MapIdle`, and the
/// render frame and render map events. Offline region events come only for the
/// regions and types that `RuntimeHandle::set_offline_region_observed()`
/// selects.
///
/// See `mln_runtime_event_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub enum RuntimeEventType: u32 {
    /// The camera is about to change. Its details are in `camera_change`.
    MapCameraWillChange = 1,
    MapCameraIsChanging = 2,
    /// The camera changed. Its details are in `camera_change`.
    MapCameraDidChange = 3,
    MapStyleLoaded = 4,
    /// The map started to load a style.
    MapLoadingStarted = 5,
    /// The map loaded its style and everything that the current view needs, and
    /// rendered the result.
    MapLoadingFinished = 6,
    /// The map failed to load a style. Its details are in `loading_failed`, and
    /// the error's text is in `message`.
    MapLoadingFailed = 7,
    /// The map rendered a complete frame and has nothing left to load, animate,
    /// or redraw.
    MapIdle = 8,
    /// The map failed to render. The error's text is in `message`.
    MapRenderError = 9,
    MapRenderFrameStarted = 10,
    /// The map rendered a frame. Its details are in `render_frame`.
    MapRenderFrameFinished = 11,
    MapRenderMapStarted = 12,
    MapRenderMapFinished = 13,
    /// The style uses an image that it doesn't have. Its details are in
    /// `style_image`.
    MapStyleImageMissing = 14,
    /// A tile moved to another step of loading. Its details are in
    /// `tile_action`.
    MapTileAction = 15,
    /// A source changed. Its details are in `source_changed`.
    MapSourceChanged = 16,
    /// The region's status changed. Its details are in `offline_region_status`.
    OfflineRegionStatusChanged = 17,
    /// A resource that the region needs failed to download. Its details are in
    /// `offline_region_response_error`, and the error's text is in `message`.
    OfflineRegionResponseError = 18,
    /// The region's download became inactive because the cache file's offline
    /// regions reached their limit of 6000 tiles from MapLibre's demo tile
    /// server. Its details are in `offline_region_tile_count_limit`.
    ///
    /// Only tiles of sources on the demo server count toward the limit: sources
    /// with a `maplibre://` URL, which resolves to
    /// https://demotiles.maplibre.org. Tiles from other servers never count.
    OfflineRegionTileCountLimitExceeded = 19,
    /// The map requested a range of glyphs. Its details are in `glyphs`.
    MapGlyphsRequested = 20,
    /// A range of glyphs loaded. Its details are in `glyphs`.
    MapGlyphsLoaded = 21,
    /// A range of glyphs failed to load. Its details are in `glyphs`, and the
    /// error's text is in `message`.
    MapGlyphsError = 22,
    /// The map requested a sprite. Its details are in `sprite`.
    MapSpriteRequested = 23,
    /// A sprite loaded. Its details are in `sprite`.
    MapSpriteLoaded = 24,
    /// A sprite failed to load. Its details are in `sprite`, and the error's
    /// text is in `message`.
    MapSpriteError = 25,
} Unknown
}

/// Options for `RuntimeHandle::create()`.
///
/// See `mln_runtime_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct RuntimeOptions<'a> {
    /// The directory that `asset://` URLs load from. Empty uses the working
    /// directory, or the app's assets on Android.
    pub asset_path: &'a str,
    /// The cache database file, which holds the ambient cache and offline
    /// regions. Empty keeps them in memory.
    pub cache_path: &'a str,
}
impl<'a> ToNative<sys::mln_runtime_options> for RuntimeOptions<'a> {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_runtime_options> {
        let mut raw: sys::mln_runtime_options = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_runtime_options>() as _;
        raw.asset_path = to_native(&self.asset_path, arena)?;
        raw.cache_path = to_native(&self.cache_path, arena)?;
        Ok(raw)
    }
}

/// A rectangle of the screen, from its `min` corner to its `max` corner.
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

/// A line through points on the screen.
///
/// See `mln_screen_line_string` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct ScreenLineString {
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

/// A point on the screen, in logical pixels from the map's top-left corner.
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

/// A feature that a source feature query found.
///
/// See `mln_source_feature` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct SourceFeature {
    /// The feature, as a GeoJSON feature.
    pub feature: String,
    /// The feature's source layer, or empty when its source has none.
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

/// Options for a source feature query.
///
/// See `mln_source_feature_query_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct SourceFeatureQueryOptions<'a> {
    /// The source layers to query. A vector source finds features only in the
    /// source layers named here.
    pub source_layer_ids: Option<&'a [&'a str]>,
    /// A \[filter\](https://maplibre.org/maplibre-style-spec/expressions/) that
    /// each feature must match. Empty matches every feature.
    pub filter: &'a str,
}
impl<'a> ToNative<sys::mln_source_feature_query_options> for SourceFeatureQueryOptions<'a> {
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

pub use crate::status::Status;
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

/// What a style expression's result depends on.
///
/// See `mln_style_expression_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct StyleExpressionInfo {
    /// True when the result doesn't depend on the feature or its state.
    pub is_feature_constant: bool,
    /// True when the result doesn't depend on the zoom level.
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

/// The inputs that a style expression reads while it evaluates.
///
/// See `mln_style_expression_inputs` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleExpressionInputs<'a> {
    pub zoom: Option<f64>,
    /// The value of `heatmap-density`, `line-progress`, and `elevation`.
    pub color_ramp_parameter: Option<f64>,
    /// The feature to evaluate against, as a GeoJSON feature, or empty for
    /// none.
    pub feature: &'a str,
    /// The feature's state, as a JSON object, or empty for none.
    pub feature_state: &'a str,
    /// The values that `global-state` reads, as a JSON object, or empty for
    /// none.
    pub global_state: &'a str,
    /// The tile that holds the feature. `within` and `distance` need it to read
    /// the feature's geometry.
    pub canonical_tile_id: Option<CanonicalTileId>,
    /// The IDs of the images that `image` treats as available.
    pub available_images: &'a [&'a str],
}
impl<'a> ToNative<sys::mln_style_expression_inputs> for StyleExpressionInputs<'a> {
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
    /// The type that the result must have.
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

/// The type of a style expression's result.
///
/// See `mln_style_expression_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct StyleExpressionType {
    pub kind: StyleExpressionTypeKind,
    /// The type of each item when `kind` is `StyleExpressionTypeKind::Array`.
    /// It can't be an array itself.
    pub item_kind: StyleExpressionTypeKind,
    /// The length of an array, or 0 for any length.
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
/// The type of a style expression's result, or of the items of an array result.
///
/// See `mln_style_expression_type_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
pub enum StyleExpressionTypeKind: u32 {
    /// Any type.
    Value = 0,
    Number = 1,
    String = 2,
    Boolean = 3,
    Color = 4,
    Object = 5,
    Formatted = 6,
    Image = 7,
    Padding = 8,
    VariableAnchorOffsetCollection = 9,
    Array = 10,
} Unknown
}
impl Default for StyleExpressionTypeKind {
    fn default() -> Self {
        Self::Value
    }
}

/// An image in the map's style.
///
/// See `mln_style_image_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleImageInfo {
    pub width: u32,
    pub height: u32,
    /// The premultiplied RGBA pixels, in rows of `width` \* 4 bytes, from the
    /// top row down.
    pub pixels: Vec<u8>,
    pub stretch_x: Vec<ImageStretch>,
    pub stretch_y: Vec<ImageStretch>,
    pub content: Option<ImageContent>,
    pub text_fit_width: Option<StyleImageTextFit>,
    pub text_fit_height: Option<StyleImageTextFit>,
    pub pixel_ratio: f32,
    pub sdf: bool,
    pub keep_unused: bool,
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
            keep_unused: raw.keep_unused,
        })
    }
}

/// Options for a style image, as the
/// \[sprite\](https://maplibre.org/maplibre-style-spec/sprite/) spec describes
/// them.
///
/// See `mln_style_image_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleImageOptions {
    /// The ranges of columns that stretch, in increasing order without overlap.
    pub stretch_x: Option<Vec<ImageStretch>>,
    /// The ranges of rows that stretch, in increasing order without overlap.
    pub stretch_y: Option<Vec<ImageStretch>>,
    pub content: Option<ImageContent>,
    pub text_fit_width: Option<StyleImageTextFit>,
    pub text_fit_height: Option<StyleImageTextFit>,
    pub pixel_ratio: Option<f32>,
    pub sdf: Option<bool>,
    /// Keeps the image after no layer uses it. Without this, the map can remove
    /// an image that it asked for through
    /// `RuntimeEventType::MapStyleImageMissing`.
    pub keep_unused: Option<bool>,
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
        if let Some(item) = &self.keep_unused {
            raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED;
            raw.keep_unused = *item;
        }
        Ok(raw)
    }
}

native_enum! {
/// How an image stretches to fit text along one axis.
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

/// A layer in the map's style.
///
/// See `mln_style_layer_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleLayerInfo {
    pub id: String,
    /// The layer's
    /// \[type\](https://maplibre.org/maplibre-style-spec/layers/#type), such as
    /// `fill`.
    pub r#type: String,
    pub source_id: String,
    pub source_layer: String,
    /// The zoom level where the layer starts to show, or negative infinity when
    /// the layer doesn't set one.
    pub min_zoom: f64,
    /// The zoom level where the layer stops showing, or infinity when the layer
    /// doesn't set one.
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
/// Whether a layer shows.
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
/// How a raster DEM tile encodes elevation in its colors.
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

/// A source in the map's style.
///
/// See `mln_style_source_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleSourceInfo {
    pub id: String,
    pub r#type: StyleSourceType,
    pub is_volatile: bool,
    pub attribution: String,
    pub url: String,
    /// The tiles that the style lists in the source itself, instead of at a
    /// URL.
    pub tile_json: Option<StyleSourceTileInfo>,
    pub bounds: Option<LatLngBounds>,
    pub tile_size: Option<u32>,
    pub vector_encoding: Option<StyleVectorTileEncoding>,
    pub raster_encoding: Option<StyleRasterDemEncoding>,
    /// A GeoJSON source's options, with every field set. Data prepared with
    /// them can replace the source's data.
    pub geo_json: Option<GeoJsonSourceOptions>,
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
            geo_json: unsafe {
                convert::present(
                    raw.fields,
                    sys::MLN_STYLE_SOURCE_INFO_GEO_JSON,
                    raw.geo_json,
                )
            }?,
        })
    }
}

/// The tiles of a source, as the style lists them.
///
/// See `mln_style_source_tile_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct StyleSourceTileInfo {
    pub tile_urls: Vec<String>,
    pub min_zoom: f64,
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
/// The type of a style source.
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
    CustomGeometry = 8,
    CustomVector = 9,
} Unrecognized
}
impl Default for StyleSourceType {
    fn default() -> Self {
        Self::Unknown
    }
}

native_enum! {
/// Whether a tileset counts tile rows from the north (XYZ) or the south (TMS).
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

/// The default
/// \[transition\](https://maplibre.org/maplibre-style-spec/transition/) for
/// changes to a style's paint properties.
///
/// See `mln_style_transition_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct StyleTransitionOptions {
    pub duration_ms: Option<f64>,
    pub delay_ms: Option<f64>,
    /// Whether symbols fade in and out as the map places them.
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
/// The format of vector tiles.
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

/// The layout of the pixels that `RenderSessionHandle::read_texture()` reads.
///
/// See `mln_texture_image_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct TextureImageInfo {
    pub width: u32,
    pub height: u32,
    /// The bytes from the start of one row to the start of the next.
    pub stride: u32,
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

/// The pixels that `RenderSessionHandle::read_texture()` reads.
///
/// See `mln_texture_readback_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct TextureReadbackResult {
    /// Premultiplied RGBA pixels with 8 bits per channel, in rows from top to
    /// bottom.
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

/// The ID of a tile.
///
/// `canonical_x` and `canonical_y` place the tile in the source's grid at zoom
/// `canonical_z`. `overscaled_z` is the zoom that the map draws the tile at,
/// and is greater when the source has no tiles that deep. `wrap` says which
/// copy of the world holds the tile: zero for the main one, negative to the
/// west, and positive to the east.
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
/// How the map picks the level of detail of its tiles.
///
/// See `mln_tile_lod_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub enum TileLodMode: u32 {
    /// Uses the most detail at the center of the screen.
    Default = 0,
    /// Uses the most detail closest to the camera.
    Distance = 1,
} Unknown
}
impl Default for TileLodMode {
    fn default() -> Self {
        Self::Default
    }
}

native_enum! {
/// A step in loading a tile.
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

/// A timing curve from (0, 0) to (1, 1), with the control points (`x1`, `y1`)
/// and (`x2`, `y2`).
///
/// `x1` and `x2` are between 0 and 1.
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
/// How a URL pattern matches a request's URL.
///
/// A pattern matches the whole URL, query included. Without flags, it matches
/// only the URL that equals it exactly.
///
/// See `mln_url_match_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub struct UrlMatchFlag: u32 {
    /// The URL matches the pattern as a glob. `*` matches any characters except
    /// `/`, `**` matches any characters, and `?` matches one character except
    /// `/`. `\` makes the next character match only itself. For example,
    /// `https://*.example.com/tiles/**` matches every URL under `/tiles/` on
    /// each subdomain of `example.com`.
    const GLOB = 1;
}
}

/// A query parameter that a resource transform rule adds to a URL.
///
/// Pass `name` and `value` unencoded. The library percent-encodes every byte of
/// them except the letters, digits, `-`, `.`, `_`, and `~`, and adds them as
/// `name=value`.
///
/// See `mln_url_query_parameter` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct UrlQueryParameter {
    pub name: String,
    pub value: String,
}
impl UrlQueryParameter {
    pub fn new(name: String) -> Self {
        Self {
            name,
            value: Default::default(),
        }
    }
}
impl ToNative<sys::mln_url_query_parameter> for UrlQueryParameter {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_url_query_parameter> {
        let mut raw: sys::mln_url_query_parameter = unsafe { std::mem::zeroed() };
        raw.name = to_native(&self.name, arena)?;
        raw.value = to_native(&self.value, arena)?;
        Ok(raw)
    }
}

/// A vector in three dimensions.
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
/// The vertical direction of the image that the map renders.
///
/// See `mln_viewport_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub enum ViewportMode: u32 {
    Default = 0,
    /// Renders the image upside down.
    FlippedY = 1,
} Unknown
}
impl Default for ViewportMode {
    fn default() -> Self {
        Self::Default
    }
}

/// A Vulkan image of a borrowed ring.
///
/// See `mln_vulkan_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanBorrowedTexture {
    /// Your VkImage.
    pub image: u64,
    /// A 2D VkImageView of `image` with one mip level and one layer.
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

/// A ring of your Vulkan images, for
/// `MapHandle::attach_vulkan_borrowed_texture()`.
///
/// `logical_size` sets the map's size. `physical_width` and `physical_height`
/// give the images' size in pixels, which is usually `logical_size` times the
/// map's scale factor. Each image must have that size, `format`, and
/// VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, and belong to the context's device.
///
/// The session uses the images until a replacement or release of the target
/// completes, a detach or release of the session completes, or an abandon of it
/// returns.
///
/// See `mln_vulkan_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct VulkanBorrowedTextureDescriptor {
    pub logical_size: LogicalSize,
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: VulkanContextDescriptor,
    pub textures: Vec<VulkanBorrowedTexture>,
    /// The images' VkFormat.
    pub format: u32,
    /// The VkImageLayout that each frame leaves its image in.
    pub final_layout: u32,
    pub owner: VulkanImageOwner,
}
impl VulkanBorrowedTextureDescriptor {
    pub fn new(
        logical_size: LogicalSize,
        physical_width: u32,
        physical_height: u32,
        context: VulkanContextDescriptor,
        textures: Vec<VulkanBorrowedTexture>,
        format: u32,
        final_layout: u32,
    ) -> Self {
        Self {
            logical_size,
            physical_width,
            physical_height,
            context,
            textures,
            format,
            final_layout,
            owner: Default::default(),
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
        raw.logical_size = to_native(&self.logical_size, arena)?;
        raw.physical_width = self.physical_width;
        raw.physical_height = self.physical_height;
        raw.context = to_native(&self.context, arena)?;
        raw.textures = convert::array(&self.textures, arena)?;
        raw.texture_count = convert::count(self.textures.len())?;
        raw.format = self.format;
        raw.final_layout = self.final_layout;
        raw.owner = to_native(&self.owner, arena)?;
        Ok(raw)
    }
}

/// New Vulkan images for a borrowed ring, for
/// `RenderSessionHandle::set_vulkan_borrowed_texture_target()`.
///
/// The members mean what they mean in `VulkanBorrowedTextureDescriptor`, and
/// the images follow its rules. The ring keeps its depth, context, `format`,
/// `final_layout`, and `owner`, so give it as many images as it attached with,
/// in that format, on the same device. The session uses the images until a
/// replacement or release of the target completes, a detach or release of the
/// session completes, or an abandon of it returns.
///
/// See `mln_vulkan_borrowed_texture_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct VulkanBorrowedTextureReplacement {
    pub logical_size: LogicalSize,
    pub physical_width: u32,
    pub physical_height: u32,
    pub textures: Vec<VulkanBorrowedTexture>,
}
impl VulkanBorrowedTextureReplacement {
    pub fn new(
        logical_size: LogicalSize,
        physical_width: u32,
        physical_height: u32,
        textures: Vec<VulkanBorrowedTexture>,
    ) -> Self {
        Self {
            logical_size,
            physical_width,
            physical_height,
            textures,
        }
    }
}
impl ToNative<sys::mln_vulkan_borrowed_texture_replacement> for VulkanBorrowedTextureReplacement {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_vulkan_borrowed_texture_replacement> {
        let mut raw: sys::mln_vulkan_borrowed_texture_replacement = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_vulkan_borrowed_texture_replacement>() as _;
        raw.logical_size = to_native(&self.logical_size, arena)?;
        raw.physical_width = self.physical_width;
        raw.physical_height = self.physical_height;
        raw.textures = convert::array(&self.textures, arena)?;
        raw.texture_count = convert::count(self.textures.len())?;
        Ok(raw)
    }
}

/// The Vulkan device and queue that a render session uses.
///
/// Keep these objects alive until a detach or release of the session completes,
/// or an abandon of it returns.
///
/// See `mln_vulkan_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanContextDescriptor {
    /// A `VkInstance`.
    pub instance: *mut std::ffi::c_void,
    /// A `VkPhysicalDevice`.
    pub physical_device: *mut std::ffi::c_void,
    /// A `VkDevice`.
    pub device: *mut std::ffi::c_void,
    /// A `VkQueue` from `graphics_queue_family_index` that supports graphics.
    /// If you submit to it too, set `RenderSessionAttachOptions::queue_lock`.
    pub graphics_queue: *mut std::ffi::c_void,
    pub graphics_queue_family_index: u32,
    /// Your `vkGetInstanceProcAddr`, or null to use the Vulkan loader's.
    pub get_instance_proc_addr: *mut std::ffi::c_void,
    /// Your `vkGetDeviceProcAddr`, or null to look it up from the instance.
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

native_enum! {
/// The queue family that a borrowed image goes to after each frame.
///
/// With `VulkanImageOwner::External` or `VulkanImageOwner::Foreign`, acquire
/// the image on your side before you read it. You don't need to release it
/// back.
///
/// See `mln_vulkan_image_owner` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
pub enum VulkanImageOwner: u32 {
    /// The image stays with the queue family of `graphics_queue_family_index`.
    GraphicsQueue = 0,
    /// Each frame releases the image to VK_QUEUE_FAMILY_EXTERNAL.
    External = 1,
    /// Each frame releases the image to VK_QUEUE_FAMILY_FOREIGN_EXT.
    Foreign = 2,
} Unknown
}
impl Default for VulkanImageOwner {
    fn default() -> Self {
        Self::GraphicsQueue
    }
}

/// A Vulkan surface to present to, for `MapHandle::attach_vulkan_surface()`.
///
/// The device must have VK_KHR_swapchain enabled, and the queue family of
/// `graphics_queue_family_index` must support presenting to the surface. The
/// session creates the swapchain. It uses the surface until a replacement or
/// release of the target completes, a detach or release of the session
/// completes, or an abandon of it returns.
///
/// See `mln_vulkan_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanSurfaceDescriptor {
    pub logical_size: LogicalSize,
    pub context: VulkanContextDescriptor,
    /// Your VkSurfaceKHR.
    pub surface: u64,
}
impl VulkanSurfaceDescriptor {
    pub fn new(logical_size: LogicalSize, context: VulkanContextDescriptor, surface: u64) -> Self {
        Self {
            logical_size,
            context,
            surface,
        }
    }
}
impl ToNative<sys::mln_vulkan_surface_descriptor> for VulkanSurfaceDescriptor {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_vulkan_surface_descriptor> {
        let mut raw: sys::mln_vulkan_surface_descriptor = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_vulkan_surface_descriptor>() as _;
        raw.logical_size = to_native(&self.logical_size, arena)?;
        raw.context = to_native(&self.context, arena)?;
        raw.surface = self.surface;
        Ok(raw)
    }
}

/// A new Vulkan surface for `RenderSessionHandle::set_vulkan_surface_target()`.
///
/// The session presents to it with the context that it attached with, so the
/// queue family of that context's `graphics_queue_family_index` must support
/// presenting to the surface. It uses the surface until a replacement or
/// release of the target completes, a detach or release of the session
/// completes, or an abandon of it returns.
///
/// See `mln_vulkan_surface_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanSurfaceReplacement {
    pub logical_size: LogicalSize,
    /// Your VkSurfaceKHR.
    pub surface: u64,
}
impl VulkanSurfaceReplacement {
    pub const fn new(logical_size: LogicalSize, surface: u64) -> Self {
        Self {
            logical_size,
            surface,
        }
    }
}
impl ToNative<sys::mln_vulkan_surface_replacement> for VulkanSurfaceReplacement {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_vulkan_surface_replacement> {
        let mut raw: sys::mln_vulkan_surface_replacement = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_vulkan_surface_replacement>() as _;
        raw.logical_size = to_native(&self.logical_size, arena)?;
        raw.surface = self.surface;
        Ok(raw)
    }
}

/// The Vulkan image of an acquired frame.
///
/// See `mln_vulkan_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct VulkanTextureFrame {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    /// The image's index in the descriptor's `textures`.
    pub slot: u32,
    /// The VkImage.
    pub image: u64,
    /// The VkImageView.
    pub image_view: u64,
    /// The image's VkDevice.
    pub device: *mut std::ffi::c_void,
    /// The image's VkFormat.
    pub format: u32,
    /// The VkImageLayout that the frame left the image in.
    pub layout: u32,
}
impl FromNative<sys::mln_vulkan_texture_frame> for VulkanTextureFrame {
    unsafe fn from_native(raw: sys::mln_vulkan_texture_frame) -> Result<Self> {
        Ok(Self {
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            slot: raw.slot,
            image: raw.image,
            image_view: raw.image_view,
            device: raw.device,
            format: raw.format,
            layout: raw.layout,
        })
    }
}

/// A callback that tells the caller when work is ready.
///
/// Can run on any thread, including the caller's own thread during a library
/// call. Return quickly and don't block.
///
/// See `mln_wake` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
#[derive(Clone, Default)]
pub struct Wake {
    /// Null to turn the wake off.
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

/// The WebGL context or canvas that a render session uses.
///
/// See `mln_web_gl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, PartialEq, Default)]
pub struct WebGlContextDescriptor<'a> {
    pub kind: WebGlContextKind,
    /// Your `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`, for an existing context.
    pub context: i32,
    /// The canvas's CSS selector, for a transferred canvas.
    pub canvas_selector: &'a str,
}
impl<'a> ToNative<sys::mln_web_gl_context_descriptor> for WebGlContextDescriptor<'a> {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_web_gl_context_descriptor> {
        let mut raw: sys::mln_web_gl_context_descriptor = unsafe { std::mem::zeroed() };
        raw.kind = to_native(&self.kind, arena)?;
        raw.context = self.context;
        raw.canvas_selector = to_native(&self.canvas_selector, arena)?;
        Ok(raw)
    }
}

native_enum! {
/// How a render session gets its WebGL context.
///
/// See `mln_web_gl_context_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub enum WebGlContextKind: u32 {
    /// The session uses a context that you created with
    /// `emscripten_webgl_create_context()`.
    Existing = 0,
    /// The library transfers the canvas that `canvas_selector` names to a
    /// worker and creates the context there.
    TransferredCanvas = 1,
} Unknown
}
impl Default for WebGlContextKind {
    fn default() -> Self {
        Self::Existing
    }
}

/// A WebGPU texture of a borrowed ring.
///
/// See `mln_web_gpu_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WebGpuBorrowedTexture {
    /// Your WGPUTexture.
    pub texture: *mut std::ffi::c_void,
    /// A WGPUTextureView of `texture`.
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

/// A ring of your WebGPU textures, for
/// `MapHandle::attach_web_gpu_borrowed_texture()`.
///
/// `logical_size` sets the map's size. `physical_width` and `physical_height`
/// give the textures' size in pixels, which is usually `logical_size` times the
/// map's scale factor. Each texture must be 2D with that size, `format`, one
/// layer, one mip level, one sample, and WGPUTextureUsage_RenderAttachment.
///
/// The session uses the textures until a replacement or release of the target
/// completes, a detach or release of the session completes, or an abandon of it
/// returns.
///
/// See `mln_web_gpu_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct WebGpuBorrowedTextureDescriptor {
    pub logical_size: LogicalSize,
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: WebGpuContextDescriptor,
    pub textures: Vec<WebGpuBorrowedTexture>,
    /// The textures' WGPUTextureFormat.
    pub format: u32,
}
impl WebGpuBorrowedTextureDescriptor {
    pub fn new(
        logical_size: LogicalSize,
        physical_width: u32,
        physical_height: u32,
        context: WebGpuContextDescriptor,
        textures: Vec<WebGpuBorrowedTexture>,
        format: u32,
    ) -> Self {
        Self {
            logical_size,
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
        raw.logical_size = to_native(&self.logical_size, arena)?;
        raw.physical_width = self.physical_width;
        raw.physical_height = self.physical_height;
        raw.context = to_native(&self.context, arena)?;
        raw.textures = convert::array(&self.textures, arena)?;
        raw.texture_count = convert::count(self.textures.len())?;
        raw.format = self.format;
        Ok(raw)
    }
}

/// New WebGPU textures for a borrowed ring, for
/// `RenderSessionHandle::set_web_gpu_borrowed_texture_target()`.
///
/// The members mean what they mean in `WebGpuBorrowedTextureDescriptor`, and
/// the textures follow its rules. The ring keeps its depth, context, and
/// `format`, so give it as many textures as it attached with, in that format,
/// on the same device. The session uses the textures until a replacement or
/// release of the target completes, a detach or release of the session
/// completes, or an abandon of it returns.
///
/// See `mln_web_gpu_borrowed_texture_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, PartialEq)]
pub struct WebGpuBorrowedTextureReplacement {
    pub logical_size: LogicalSize,
    pub physical_width: u32,
    pub physical_height: u32,
    pub textures: Vec<WebGpuBorrowedTexture>,
}
impl WebGpuBorrowedTextureReplacement {
    pub fn new(
        logical_size: LogicalSize,
        physical_width: u32,
        physical_height: u32,
        textures: Vec<WebGpuBorrowedTexture>,
    ) -> Self {
        Self {
            logical_size,
            physical_width,
            physical_height,
            textures,
        }
    }
}
impl ToNative<sys::mln_web_gpu_borrowed_texture_replacement> for WebGpuBorrowedTextureReplacement {
    fn to_native(
        &self,
        arena: &mut InputArena,
    ) -> Result<sys::mln_web_gpu_borrowed_texture_replacement> {
        let mut raw: sys::mln_web_gpu_borrowed_texture_replacement = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_web_gpu_borrowed_texture_replacement>() as _;
        raw.logical_size = to_native(&self.logical_size, arena)?;
        raw.physical_width = self.physical_width;
        raw.physical_height = self.physical_height;
        raw.textures = convert::array(&self.textures, arena)?;
        raw.texture_count = convert::count(self.textures.len())?;
        Ok(raw)
    }
}

/// The WebGPU device that a render session uses.
///
/// See `mln_web_gpu_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WebGpuContextDescriptor {
    /// A `WGPUInstance`, or null.
    pub instance: *mut std::ffi::c_void,
    /// A `WGPUDevice`.
    pub device: *mut std::ffi::c_void,
    /// A `WGPUQueue`, or null to use the device's queue.
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

/// A WebGPU surface to present to, for `MapHandle::attach_web_gpu_surface()`.
///
/// The session configures the surface. It uses the surface until a replacement
/// or release of the target completes, a detach or release of the session
/// completes, or an abandon of it returns.
///
/// See `mln_web_gpu_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WebGpuSurfaceDescriptor {
    pub logical_size: LogicalSize,
    pub context: WebGpuContextDescriptor,
    /// Your WGPUSurface.
    pub surface: *mut std::ffi::c_void,
    /// The WGPUTextureFormat to configure the surface with.
    pub format: u32,
}
impl WebGpuSurfaceDescriptor {
    pub fn new(
        logical_size: LogicalSize,
        context: WebGpuContextDescriptor,
        surface: *mut std::ffi::c_void,
        format: u32,
    ) -> Self {
        Self {
            logical_size,
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
        raw.logical_size = to_native(&self.logical_size, arena)?;
        raw.context = to_native(&self.context, arena)?;
        raw.surface = self.surface;
        raw.format = self.format;
        Ok(raw)
    }
}

/// A new WebGPU surface for
/// `RenderSessionHandle::set_web_gpu_surface_target()`.
///
/// The session configures it with the context and format that it attached with.
/// It uses the surface until a replacement or release of the target completes,
/// a detach or release of the session completes, or an abandon of it returns.
///
/// See `mln_web_gpu_surface_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WebGpuSurfaceReplacement {
    pub logical_size: LogicalSize,
    /// Your WGPUSurface.
    pub surface: *mut std::ffi::c_void,
}
impl WebGpuSurfaceReplacement {
    pub fn new(logical_size: LogicalSize, surface: *mut std::ffi::c_void) -> Self {
        Self {
            logical_size,
            surface,
        }
    }
}
impl ToNative<sys::mln_web_gpu_surface_replacement> for WebGpuSurfaceReplacement {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_web_gpu_surface_replacement> {
        let mut raw: sys::mln_web_gpu_surface_replacement = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_web_gpu_surface_replacement>() as _;
        raw.logical_size = to_native(&self.logical_size, arena)?;
        raw.surface = self.surface;
        Ok(raw)
    }
}

/// The WebGPU texture of an acquired frame.
///
/// See `mln_web_gpu_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct WebGpuTextureFrame {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    /// The texture's index in the descriptor's `textures`.
    pub slot: u32,
    /// The WGPUTexture.
    pub texture: *mut std::ffi::c_void,
    /// The WGPUTextureView.
    pub texture_view: *mut std::ffi::c_void,
    /// The texture's WGPUDevice.
    pub device: *mut std::ffi::c_void,
    /// The texture's WGPUTextureFormat.
    pub format: u32,
}
impl FromNative<sys::mln_web_gpu_texture_frame> for WebGpuTextureFrame {
    unsafe fn from_native(raw: sys::mln_web_gpu_texture_frame) -> Result<Self> {
        Ok(Self {
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            slot: raw.slot,
            texture: raw.texture,
            texture_view: raw.texture_view,
            device: raw.device,
            format: raw.format,
        })
    }
}

/// The WGL objects that a render session makes its OpenGL context from.
///
/// See `mln_wgl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WglContextDescriptor {
    /// An `HDC`.
    pub device_context: *mut std::ffi::c_void,
    /// Your `HGLRC`, for a shared context.
    pub share_context: *mut std::ffi::c_void,
    /// Your `wglGetProcAddress`, or null to use the system's.
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
