use maplibre_native_sys as sys;

use crate::enums::{ConstrainMode, MapMode, NorthOrientation, TileLodMode, ViewportMode};
use crate::error::Result;
use crate::values::{EdgeInsets, edge_insets_from_native, edge_insets_to_native};

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

    fn to_native(&self) -> Result<sys::mln_map_options> {
        // SAFETY: Default constructor takes no arguments and initializes size
        // and default values for this C ABI version.
        let mut raw = unsafe { sys::mln_map_options_default() };
        raw.width = self.width;
        raw.height = self.height;
        raw.scale_factor = self.scale_factor;
        raw.map_mode = self.mode.raw_for_set()?;
        Ok(raw)
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
            mode: MapMode::from_raw(raw.map_mode),
        }
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
    fn to_native(&self) -> sys::mln_map_viewport_options {
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
            raw.frustum_offset = edge_insets_to_native(frustum_offset);
        }
        raw
    }

    fn from_native(raw: sys::mln_map_viewport_options) -> Self {
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
                .then(|| edge_insets_from_native(raw.frustum_offset)),
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
    fn to_native(&self) -> sys::mln_map_tile_options {
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

    fn from_native(raw: sys::mln_map_tile_options) -> Self {
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

pub fn map_options_to_native(options: &MapOptions) -> Result<sys::mln_map_options> {
    options.to_native()
}

pub fn map_viewport_options_to_native(
    options: &MapViewportOptions,
) -> sys::mln_map_viewport_options {
    options.to_native()
}

pub fn map_viewport_options_from_native(raw: sys::mln_map_viewport_options) -> MapViewportOptions {
    MapViewportOptions::from_native(raw)
}

pub fn map_tile_options_to_native(options: &MapTileOptions) -> sys::mln_map_tile_options {
    options.to_native()
}

pub fn map_tile_options_from_native(raw: sys::mln_map_tile_options) -> MapTileOptions {
    MapTileOptions::from_native(raw)
}

#[doc(hidden)]
pub trait MapOptionsNativeExt {
    fn to_native(&self) -> Result<sys::mln_map_options>;
}

impl MapOptionsNativeExt for MapOptions {
    fn to_native(&self) -> Result<sys::mln_map_options> {
        map_options_to_native(self)
    }
}

#[doc(hidden)]
pub trait MapViewportOptionsNativeExt {
    fn to_native(&self) -> sys::mln_map_viewport_options;
    fn from_native(raw: sys::mln_map_viewport_options) -> MapViewportOptions;
}

impl MapViewportOptionsNativeExt for MapViewportOptions {
    fn to_native(&self) -> sys::mln_map_viewport_options {
        map_viewport_options_to_native(self)
    }

    fn from_native(raw: sys::mln_map_viewport_options) -> MapViewportOptions {
        map_viewport_options_from_native(raw)
    }
}

#[doc(hidden)]
pub trait MapTileOptionsNativeExt {
    fn to_native(&self) -> sys::mln_map_tile_options;
    fn from_native(raw: sys::mln_map_tile_options) -> MapTileOptions;
}

impl MapTileOptionsNativeExt for MapTileOptions {
    fn to_native(&self) -> sys::mln_map_tile_options {
        map_tile_options_to_native(self)
    }

    fn from_native(raw: sys::mln_map_tile_options) -> MapTileOptions {
        map_tile_options_from_native(raw)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::values::EdgeInsets;

    #[test]
    fn map_options_materializes_defaults_and_fields() {
        let mut options = MapOptions::new(800, 600, 2.0);
        options.mode = MapMode::Static;
        let raw = map_options_to_native(&options).unwrap();

        assert_eq!(raw.size, std::mem::size_of::<sys::mln_map_options>() as u32);
        assert_eq!(raw.width, 800);
        assert_eq!(raw.height, 600);
        assert_eq!(raw.scale_factor, 2.0);
        assert_eq!(raw.map_mode, sys::MLN_MAP_MODE_STATIC);
    }

    #[test]
    fn map_options_rejects_unknown_mode_for_native() {
        let mut options = MapOptions::new(800, 600, 2.0);
        options.mode = MapMode::Unknown(999_030);
        assert!(map_options_to_native(&options).is_err());
    }

    #[test]
    fn viewport_options_materializes_masks_and_round_trips() {
        let options = MapViewportOptions {
            north_orientation: Some(NorthOrientation::Right),
            constrain_mode: Some(ConstrainMode::HeightOnly),
            viewport_mode: Some(ViewportMode::FlippedY),
            frustum_offset: Some(EdgeInsets::new(1.0, 2.0, 3.0, 4.0)),
        };

        let raw = map_viewport_options_to_native(&options);

        assert_eq!(
            raw.fields,
            sys::MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION
                | sys::MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE
                | sys::MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE
                | sys::MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET
        );
        assert_eq!(map_viewport_options_from_native(raw), options);
    }

    #[test]
    fn tile_options_materializes_masks_and_round_trips() {
        let options = MapTileOptions {
            prefetch_zoom_delta: Some(4),
            lod_min_radius: Some(1.5),
            lod_scale: Some(2.5),
            lod_pitch_threshold: Some(3.5),
            lod_zoom_shift: Some(4.5),
            lod_mode: Some(TileLodMode::Distance),
        };

        let raw = map_tile_options_to_native(&options);

        assert_eq!(
            raw.fields,
            sys::MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA
                | sys::MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS
                | sys::MLN_MAP_TILE_OPTION_LOD_SCALE
                | sys::MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD
                | sys::MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT
                | sys::MLN_MAP_TILE_OPTION_LOD_MODE
        );
        assert_eq!(map_tile_options_from_native(raw), options);
    }
}
