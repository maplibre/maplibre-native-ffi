use std::ptr;

use maplibre_native_sys as sys;

use crate::enums::{RasterDemEncoding, TileScheme, VectorTileEncoding};
use crate::string::{StringView, string_view};
use crate::values::{LatLngBounds, lat_lng_bounds_to_native};

/// Options for vector, raster, and raster DEM tile sources.
#[derive(Debug, Clone, Default, PartialEq)]
#[non_exhaustive]
pub struct TileSourceOptions {
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
    pub attribution: Option<String>,
    pub scheme: Option<TileScheme>,
    pub bounds: Option<LatLngBounds>,
    pub tile_size: Option<u32>,
    pub vector_encoding: Option<VectorTileEncoding>,
    pub raster_dem_encoding: Option<RasterDemEncoding>,
}

impl TileSourceOptions {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_min_zoom(mut self, min_zoom: f64) -> Self {
        self.min_zoom = Some(min_zoom);
        self
    }

    pub fn with_max_zoom(mut self, max_zoom: f64) -> Self {
        self.max_zoom = Some(max_zoom);
        self
    }

    pub fn with_attribution(mut self, attribution: impl Into<String>) -> Self {
        self.attribution = Some(attribution.into());
        self
    }

    pub fn with_scheme(mut self, scheme: TileScheme) -> Self {
        self.scheme = Some(scheme);
        self
    }

    pub fn with_bounds(mut self, bounds: LatLngBounds) -> Self {
        self.bounds = Some(bounds);
        self
    }

    pub fn with_tile_size(mut self, tile_size: u32) -> Self {
        self.tile_size = Some(tile_size);
        self
    }

    pub fn with_vector_encoding(mut self, vector_encoding: VectorTileEncoding) -> Self {
        self.vector_encoding = Some(vector_encoding);
        self
    }

    pub fn with_raster_dem_encoding(mut self, raster_dem_encoding: RasterDemEncoding) -> Self {
        self.raster_dem_encoding = Some(raster_dem_encoding);
        self
    }

    fn to_native(&self) -> NativeTileSourceOptions<'_> {
        NativeTileSourceOptions::new(self)
    }
}

pub struct NativeTileSourceOptions<'a> {
    raw: sys::mln_style_tile_source_options,
    _attribution: Option<StringView<'a>>,
}

impl<'a> NativeTileSourceOptions<'a> {
    fn new(options: &'a TileSourceOptions) -> Self {
        // SAFETY: This C helper returns a plain value with no preconditions.
        let mut raw = unsafe { sys::mln_style_tile_source_options_default() };
        if let Some(value) = options.min_zoom {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM;
            raw.min_zoom = value;
        }
        if let Some(value) = options.max_zoom {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM;
            raw.max_zoom = value;
        }
        let attribution = options.attribution.as_deref().map(string_view);
        if let Some(value) = attribution {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
            raw.attribution = value.raw();
        }
        if let Some(value) = options.scheme {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_SCHEME;
            raw.scheme = value.raw_value();
        }
        if let Some(value) = options.bounds {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS;
            raw.bounds = lat_lng_bounds_to_native(value);
        }
        if let Some(value) = options.tile_size {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE;
            raw.tile_size = value;
        }
        if let Some(value) = options.vector_encoding {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
            raw.vector_encoding = value.raw_value();
        }
        if let Some(value) = options.raster_dem_encoding {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
            raw.raster_encoding = value.raw_value();
        }
        Self {
            raw,
            _attribution: attribution,
        }
    }

    pub fn as_ptr(&self) -> *const sys::mln_style_tile_source_options {
        ptr::from_ref(&self.raw)
    }
}

impl AsRef<sys::mln_style_tile_source_options> for NativeTileSourceOptions<'_> {
    fn as_ref(&self) -> &sys::mln_style_tile_source_options {
        &self.raw
    }
}

pub fn tile_source_options_to_native(options: &TileSourceOptions) -> NativeTileSourceOptions<'_> {
    options.to_native()
}

/// Options for adding or replacing a runtime style image.
#[derive(Debug, Clone, Default, PartialEq)]
#[non_exhaustive]
pub struct StyleImageOptions {
    pub pixel_ratio: Option<f32>,
    pub sdf: Option<bool>,
}

impl StyleImageOptions {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_pixel_ratio(mut self, pixel_ratio: f32) -> Self {
        self.pixel_ratio = Some(pixel_ratio);
        self
    }

    pub fn with_sdf(mut self, sdf: bool) -> Self {
        self.sdf = Some(sdf);
        self
    }

    fn to_native(&self) -> sys::mln_style_image_options {
        let mut fields = 0;
        let mut pixel_ratio = 1.0;
        let mut sdf = false;
        if let Some(value) = self.pixel_ratio {
            fields |= sys::MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
            pixel_ratio = value;
        }
        if let Some(value) = self.sdf {
            fields |= sys::MLN_STYLE_IMAGE_OPTION_SDF;
            sdf = value;
        }
        sys::mln_style_image_options {
            size: std::mem::size_of::<sys::mln_style_image_options>() as u32,
            fields,
            pixel_ratio,
            sdf,
        }
    }
}

pub fn style_image_options_to_native(options: &StyleImageOptions) -> sys::mln_style_image_options {
    options.to_native()
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{LatLng, LatLngBounds};

    #[test]
    fn tile_source_options_materialize_masks_fields_and_views() {
        let options = TileSourceOptions::new()
            .with_min_zoom(1.0)
            .with_max_zoom(22.0)
            .with_attribution("© MapLibre")
            .with_scheme(TileScheme::Tms)
            .with_bounds(LatLngBounds::new(
                LatLng::new(1.0, 2.0),
                LatLng::new(3.0, 4.0),
            ))
            .with_tile_size(512)
            .with_vector_encoding(VectorTileEncoding::Mvt)
            .with_raster_dem_encoding(RasterDemEncoding::Mapbox);

        let native = tile_source_options_to_native(&options);
        let raw = native.as_ref();

        assert_eq!(
            raw.size,
            std::mem::size_of::<sys::mln_style_tile_source_options>() as u32
        );
        assert_eq!(
            raw.fields,
            sys::MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM
                | sys::MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM
                | sys::MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION
                | sys::MLN_STYLE_TILE_SOURCE_OPTION_SCHEME
                | sys::MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS
                | sys::MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE
                | sys::MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING
                | sys::MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
        );
        assert_eq!(raw.min_zoom, 1.0);
        assert_eq!(raw.max_zoom, 22.0);
        assert_eq!(raw.scheme, sys::MLN_STYLE_TILE_SCHEME_TMS);
        assert_eq!(raw.bounds.southwest.latitude, 1.0);
        assert_eq!(raw.bounds.northeast.longitude, 4.0);
        assert_eq!(raw.tile_size, 512);
        assert_eq!(raw.vector_encoding, sys::MLN_STYLE_VECTOR_TILE_ENCODING_MVT);
        assert_eq!(
            raw.raster_encoding,
            sys::MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX
        );
        // SAFETY: native keeps the attribution string view valid for this scope.
        assert_eq!(
            unsafe { crate::string::copy_string_view(raw.attribution) }.unwrap(),
            "© MapLibre"
        );
    }

    #[test]
    fn style_image_options_materialize_masks_and_defaults() {
        let default_raw = style_image_options_to_native(&StyleImageOptions::new());
        assert_eq!(
            default_raw.size,
            std::mem::size_of::<sys::mln_style_image_options>() as u32
        );
        assert_eq!(default_raw.fields, 0);
        assert_eq!(default_raw.pixel_ratio, 1.0);
        assert!(!default_raw.sdf);

        let raw = style_image_options_to_native(
            &StyleImageOptions::new()
                .with_pixel_ratio(2.0)
                .with_sdf(true),
        );
        assert_eq!(
            raw.fields,
            sys::MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO | sys::MLN_STYLE_IMAGE_OPTION_SDF
        );
        assert_eq!(raw.pixel_ratio, 2.0);
        assert!(raw.sdf);
    }
}
