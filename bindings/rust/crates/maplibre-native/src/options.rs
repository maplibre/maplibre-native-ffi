use maplibre_native_sys as sys;

use crate::EdgeInsets;

/// Map rendering mode used when creating a map.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum MapMode {
    Continuous,
    Static,
    Tile,
}

impl MapMode {
    pub(crate) fn from_raw(raw: u32) -> Option<Self> {
        match raw {
            sys::MLN_MAP_MODE_CONTINUOUS => Some(Self::Continuous),
            sys::MLN_MAP_MODE_STATIC => Some(Self::Static),
            sys::MLN_MAP_MODE_TILE => Some(Self::Tile),
            _ => None,
        }
    }

    pub(crate) fn as_raw(self) -> u32 {
        match self {
            Self::Continuous => sys::MLN_MAP_MODE_CONTINUOUS,
            Self::Static => sys::MLN_MAP_MODE_STATIC,
            Self::Tile => sys::MLN_MAP_MODE_TILE,
        }
    }
}

/// Options used when creating a map.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct MapOptions {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub mode: MapMode,
}

impl MapOptions {
    pub fn new(width: u32, height: u32, scale_factor: f64) -> Self {
        Self {
            width,
            height,
            scale_factor,
            ..Self::default()
        }
    }

    pub fn with_mode(mut self, mode: MapMode) -> Self {
        self.mode = mode;
        self
    }

    pub(crate) fn to_native(&self) -> sys::mln_map_options {
        // SAFETY: Default constructor takes no arguments and initializes size
        // and default values for this C ABI version.
        let mut raw = unsafe { sys::mln_map_options_default() };
        raw.width = self.width;
        raw.height = self.height;
        raw.scale_factor = self.scale_factor;
        raw.map_mode = self.mode.as_raw();
        raw
    }
}

impl Default for MapOptions {
    fn default() -> Self {
        // SAFETY: Default constructor takes no arguments and initializes values
        // for this C ABI version.
        let raw = unsafe { sys::mln_map_options_default() };
        Self {
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            mode: MapMode::from_raw(raw.map_mode).unwrap_or(MapMode::Continuous),
        }
    }
}

/// Map north orientation values used by viewport options.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum NorthOrientation {
    Up,
    Right,
    Down,
    Left,
    Unknown(u32),
}

impl NorthOrientation {
    pub(crate) fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_NORTH_ORIENTATION_UP => Self::Up,
            sys::MLN_NORTH_ORIENTATION_RIGHT => Self::Right,
            sys::MLN_NORTH_ORIENTATION_DOWN => Self::Down,
            sys::MLN_NORTH_ORIENTATION_LEFT => Self::Left,
            _ => Self::Unknown(raw),
        }
    }

    pub(crate) fn as_raw(self) -> u32 {
        match self {
            Self::Up => sys::MLN_NORTH_ORIENTATION_UP,
            Self::Right => sys::MLN_NORTH_ORIENTATION_RIGHT,
            Self::Down => sys::MLN_NORTH_ORIENTATION_DOWN,
            Self::Left => sys::MLN_NORTH_ORIENTATION_LEFT,
            Self::Unknown(raw) => raw,
        }
    }
}

/// Map constraint modes used by viewport options.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ConstrainMode {
    None,
    HeightOnly,
    WidthAndHeight,
    Screen,
    Unknown(u32),
}

impl ConstrainMode {
    pub(crate) fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_CONSTRAIN_MODE_NONE => Self::None,
            sys::MLN_CONSTRAIN_MODE_HEIGHT_ONLY => Self::HeightOnly,
            sys::MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT => Self::WidthAndHeight,
            sys::MLN_CONSTRAIN_MODE_SCREEN => Self::Screen,
            _ => Self::Unknown(raw),
        }
    }

    pub(crate) fn as_raw(self) -> u32 {
        match self {
            Self::None => sys::MLN_CONSTRAIN_MODE_NONE,
            Self::HeightOnly => sys::MLN_CONSTRAIN_MODE_HEIGHT_ONLY,
            Self::WidthAndHeight => sys::MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT,
            Self::Screen => sys::MLN_CONSTRAIN_MODE_SCREEN,
            Self::Unknown(raw) => raw,
        }
    }
}

/// Viewport orientation modes used by viewport options.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ViewportMode {
    Default,
    FlippedY,
    Unknown(u32),
}

impl ViewportMode {
    pub(crate) fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_VIEWPORT_MODE_DEFAULT => Self::Default,
            sys::MLN_VIEWPORT_MODE_FLIPPED_Y => Self::FlippedY,
            _ => Self::Unknown(raw),
        }
    }

    pub(crate) fn as_raw(self) -> u32 {
        match self {
            Self::Default => sys::MLN_VIEWPORT_MODE_DEFAULT,
            Self::FlippedY => sys::MLN_VIEWPORT_MODE_FLIPPED_Y,
            Self::Unknown(raw) => raw,
        }
    }
}

/// Tile LOD algorithms used by map tile options.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum TileLodMode {
    Default,
    Distance,
    Unknown(u32),
}

impl TileLodMode {
    pub(crate) fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_TILE_LOD_MODE_DEFAULT => Self::Default,
            sys::MLN_TILE_LOD_MODE_DISTANCE => Self::Distance,
            _ => Self::Unknown(raw),
        }
    }

    pub(crate) fn as_raw(self) -> u32 {
        match self {
            Self::Default => sys::MLN_TILE_LOD_MODE_DEFAULT,
            Self::Distance => sys::MLN_TILE_LOD_MODE_DISTANCE,
            Self::Unknown(raw) => raw,
        }
    }
}

bitflags::bitflags! {
    /// MapLibre debug overlay mask bits.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    pub struct MapDebugOptions: u32 {
        const TILE_BORDERS = sys::MLN_MAP_DEBUG_TILE_BORDERS;
        const PARSE_STATUS = sys::MLN_MAP_DEBUG_PARSE_STATUS;
        const TIMESTAMPS = sys::MLN_MAP_DEBUG_TIMESTAMPS;
        const COLLISION = sys::MLN_MAP_DEBUG_COLLISION;
        const OVERDRAW = sys::MLN_MAP_DEBUG_OVERDRAW;
        const STENCIL_CLIP = sys::MLN_MAP_DEBUG_STENCIL_CLIP;
        const DEPTH_BUFFER = sys::MLN_MAP_DEBUG_DEPTH_BUFFER;
        const _ = !0;
    }
}

/// Live map viewport and render-transform controls.
#[derive(Debug, Clone, PartialEq, Default)]
#[non_exhaustive]
pub struct MapViewportOptions {
    pub north_orientation: Option<NorthOrientation>,
    pub constrain_mode: Option<ConstrainMode>,
    pub viewport_mode: Option<ViewportMode>,
    pub frustum_offset: Option<EdgeInsets>,
}

impl MapViewportOptions {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_north_orientation(mut self, north_orientation: NorthOrientation) -> Self {
        self.north_orientation = Some(north_orientation);
        self
    }

    pub fn with_constrain_mode(mut self, constrain_mode: ConstrainMode) -> Self {
        self.constrain_mode = Some(constrain_mode);
        self
    }

    pub fn with_viewport_mode(mut self, viewport_mode: ViewportMode) -> Self {
        self.viewport_mode = Some(viewport_mode);
        self
    }

    pub fn with_frustum_offset(mut self, frustum_offset: EdgeInsets) -> Self {
        self.frustum_offset = Some(frustum_offset);
        self
    }

    pub(crate) fn to_native(&self) -> sys::mln_map_viewport_options {
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_map_viewport_options_default() };
        if let Some(north_orientation) = self.north_orientation {
            raw.fields |= sys::MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
            raw.north_orientation = north_orientation.as_raw();
        }
        if let Some(constrain_mode) = self.constrain_mode {
            raw.fields |= sys::MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
            raw.constrain_mode = constrain_mode.as_raw();
        }
        if let Some(viewport_mode) = self.viewport_mode {
            raw.fields |= sys::MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
            raw.viewport_mode = viewport_mode.as_raw();
        }
        if let Some(frustum_offset) = self.frustum_offset {
            raw.fields |= sys::MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
            raw.frustum_offset = frustum_offset.to_native();
        }
        raw
    }

    pub(crate) fn from_native(raw: sys::mln_map_viewport_options) -> Self {
        Self {
            north_orientation: maybe_enum(
                raw.fields,
                sys::MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION,
                raw.north_orientation,
                NorthOrientation::from_raw,
            ),
            constrain_mode: maybe_enum(
                raw.fields,
                sys::MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE,
                raw.constrain_mode,
                ConstrainMode::from_raw,
            ),
            viewport_mode: maybe_enum(
                raw.fields,
                sys::MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE,
                raw.viewport_mode,
                ViewportMode::from_raw,
            ),
            frustum_offset: has(raw.fields, sys::MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET)
                .then(|| EdgeInsets::from_native(raw.frustum_offset)),
        }
    }
}

/// Tile prefetch and LOD tuning controls.
#[derive(Debug, Clone, PartialEq, Default)]
#[non_exhaustive]
pub struct MapTileOptions {
    pub prefetch_zoom_delta: Option<u32>,
    pub lod_min_radius: Option<f64>,
    pub lod_scale: Option<f64>,
    pub lod_pitch_threshold: Option<f64>,
    pub lod_zoom_shift: Option<f64>,
    pub lod_mode: Option<TileLodMode>,
}

impl MapTileOptions {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_prefetch_zoom_delta(mut self, prefetch_zoom_delta: u32) -> Self {
        self.prefetch_zoom_delta = Some(prefetch_zoom_delta);
        self
    }

    pub fn with_lod_min_radius(mut self, lod_min_radius: f64) -> Self {
        self.lod_min_radius = Some(lod_min_radius);
        self
    }

    pub fn with_lod_scale(mut self, lod_scale: f64) -> Self {
        self.lod_scale = Some(lod_scale);
        self
    }

    pub fn with_lod_pitch_threshold(mut self, lod_pitch_threshold: f64) -> Self {
        self.lod_pitch_threshold = Some(lod_pitch_threshold);
        self
    }

    pub fn with_lod_zoom_shift(mut self, lod_zoom_shift: f64) -> Self {
        self.lod_zoom_shift = Some(lod_zoom_shift);
        self
    }

    pub fn with_lod_mode(mut self, lod_mode: TileLodMode) -> Self {
        self.lod_mode = Some(lod_mode);
        self
    }

    pub(crate) fn to_native(&self) -> sys::mln_map_tile_options {
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_map_tile_options_default() };
        if let Some(prefetch_zoom_delta) = self.prefetch_zoom_delta {
            raw.fields |= sys::MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
            raw.prefetch_zoom_delta = prefetch_zoom_delta;
        }
        if let Some(lod_min_radius) = self.lod_min_radius {
            raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS;
            raw.lod_min_radius = lod_min_radius;
        }
        if let Some(lod_scale) = self.lod_scale {
            raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_SCALE;
            raw.lod_scale = lod_scale;
        }
        if let Some(lod_pitch_threshold) = self.lod_pitch_threshold {
            raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD;
            raw.lod_pitch_threshold = lod_pitch_threshold;
        }
        if let Some(lod_zoom_shift) = self.lod_zoom_shift {
            raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT;
            raw.lod_zoom_shift = lod_zoom_shift;
        }
        if let Some(lod_mode) = self.lod_mode {
            raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_MODE;
            raw.lod_mode = lod_mode.as_raw();
        }
        raw
    }

    pub(crate) fn from_native(raw: sys::mln_map_tile_options) -> Self {
        Self {
            prefetch_zoom_delta: has(raw.fields, sys::MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA)
                .then_some(raw.prefetch_zoom_delta),
            lod_min_radius: has(raw.fields, sys::MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS)
                .then_some(raw.lod_min_radius),
            lod_scale: has(raw.fields, sys::MLN_MAP_TILE_OPTION_LOD_SCALE).then_some(raw.lod_scale),
            lod_pitch_threshold: has(raw.fields, sys::MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD)
                .then_some(raw.lod_pitch_threshold),
            lod_zoom_shift: has(raw.fields, sys::MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT)
                .then_some(raw.lod_zoom_shift),
            lod_mode: maybe_enum(
                raw.fields,
                sys::MLN_MAP_TILE_OPTION_LOD_MODE,
                raw.lod_mode,
                TileLodMode::from_raw,
            ),
        }
    }
}

fn has(fields: u32, flag: u32) -> bool {
    fields & flag != 0
}

fn maybe_enum<T>(fields: u32, flag: u32, raw: u32, convert: impl FnOnce(u32) -> T) -> Option<T> {
    has(fields, flag).then(|| convert(raw))
}
