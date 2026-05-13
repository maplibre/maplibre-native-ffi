pub use maplibre_native_core::options::{MapOptions, MapTileOptions, MapViewportOptions};

use crate::sys;

pub(crate) trait MapOptionsNativeExt {
    fn to_native(&self) -> sys::mln_map_options;
}

impl MapOptionsNativeExt for MapOptions {
    fn to_native(&self) -> sys::mln_map_options {
        maplibre_native_core::options::map_options_to_native(self)
    }
}

pub(crate) trait MapViewportOptionsNativeExt {
    fn to_native(&self) -> sys::mln_map_viewport_options;
    fn from_native(raw: sys::mln_map_viewport_options) -> MapViewportOptions;
}

impl MapViewportOptionsNativeExt for MapViewportOptions {
    fn to_native(&self) -> sys::mln_map_viewport_options {
        maplibre_native_core::options::map_viewport_options_to_native(self)
    }

    fn from_native(raw: sys::mln_map_viewport_options) -> MapViewportOptions {
        maplibre_native_core::options::map_viewport_options_from_native(raw)
    }
}

pub(crate) trait MapTileOptionsNativeExt {
    fn to_native(&self) -> sys::mln_map_tile_options;
    fn from_native(raw: sys::mln_map_tile_options) -> MapTileOptions;
}

impl MapTileOptionsNativeExt for MapTileOptions {
    fn to_native(&self) -> sys::mln_map_tile_options {
        maplibre_native_core::options::map_tile_options_to_native(self)
    }

    fn from_native(raw: sys::mln_map_tile_options) -> MapTileOptions {
        maplibre_native_core::options::map_tile_options_from_native(raw)
    }
}
