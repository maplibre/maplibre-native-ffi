use std::{marker::PhantomData, mem, ptr};

use maplibre_native_support as support;
use maplibre_native_sys as sys;

use crate::custom_geometry::{CanonicalTileId, CustomGeometrySourceState};
use crate::handle::out_handle;
use crate::render::{PremultipliedRgba8Image, TextureImageInfo};
use crate::{
    CustomGeometrySourceOptions, Error, ErrorKind, GeoJson, JsonValue, LatLng, LatLngBounds, Result,
};

use super::{const_ptr_or_null, copy_style_id_list, json_snapshot, lat_lngs_to_native};

/// Style source type values returned by native style source metadata.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum SourceType {
    Unknown,
    Vector,
    Raster,
    RasterDem,
    GeoJson,
    Image,
    Video,
    Annotations,
    CustomVector,
    Other(u32),
}

impl SourceType {
    /// Converts a raw C ABI source type value into a Rust value, preserving
    /// future values.
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_STYLE_SOURCE_TYPE_UNKNOWN => Self::Unknown,
            sys::MLN_STYLE_SOURCE_TYPE_VECTOR => Self::Vector,
            sys::MLN_STYLE_SOURCE_TYPE_RASTER => Self::Raster,
            sys::MLN_STYLE_SOURCE_TYPE_RASTER_DEM => Self::RasterDem,
            sys::MLN_STYLE_SOURCE_TYPE_GEOJSON => Self::GeoJson,
            sys::MLN_STYLE_SOURCE_TYPE_IMAGE => Self::Image,
            sys::MLN_STYLE_SOURCE_TYPE_VIDEO => Self::Video,
            sys::MLN_STYLE_SOURCE_TYPE_ANNOTATIONS => Self::Annotations,
            sys::MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR => Self::CustomVector,
            _ => Self::Other(raw),
        }
    }

    /// Returns the raw C ABI source type value.
    pub fn raw_value(self) -> u32 {
        match self {
            Self::Unknown => sys::MLN_STYLE_SOURCE_TYPE_UNKNOWN,
            Self::Vector => sys::MLN_STYLE_SOURCE_TYPE_VECTOR,
            Self::Raster => sys::MLN_STYLE_SOURCE_TYPE_RASTER,
            Self::RasterDem => sys::MLN_STYLE_SOURCE_TYPE_RASTER_DEM,
            Self::GeoJson => sys::MLN_STYLE_SOURCE_TYPE_GEOJSON,
            Self::Image => sys::MLN_STYLE_SOURCE_TYPE_IMAGE,
            Self::Video => sys::MLN_STYLE_SOURCE_TYPE_VIDEO,
            Self::Annotations => sys::MLN_STYLE_SOURCE_TYPE_ANNOTATIONS,
            Self::CustomVector => sys::MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR,
            Self::Other(raw) => raw,
        }
    }
}

/// Copied fixed metadata for one style source.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub struct SourceInfo {
    pub source_type: SourceType,
    pub raw_source_type: u32,
    pub is_volatile: bool,
    pub attribution: Option<String>,
}

/// Tile URL coordinate scheme for vector, raster, and raster DEM sources.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum TileScheme {
    Xyz,
    Tms,
}

impl TileScheme {
    pub fn raw_value(self) -> u32 {
        match self {
            Self::Xyz => sys::MLN_STYLE_TILE_SCHEME_XYZ,
            Self::Tms => sys::MLN_STYLE_TILE_SCHEME_TMS,
        }
    }
}

/// Vector tile encoding for vector style sources.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum VectorTileEncoding {
    Mvt,
    Mlt,
}

impl VectorTileEncoding {
    pub fn raw_value(self) -> u32 {
        match self {
            Self::Mvt => sys::MLN_STYLE_VECTOR_TILE_ENCODING_MVT,
            Self::Mlt => sys::MLN_STYLE_VECTOR_TILE_ENCODING_MLT,
        }
    }
}

/// DEM raster encoding for raster DEM style sources.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum RasterDemEncoding {
    Mapbox,
    Terrarium,
}

impl RasterDemEncoding {
    pub fn raw_value(self) -> u32 {
        match self {
            Self::Mapbox => sys::MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX,
            Self::Terrarium => sys::MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM,
        }
    }
}

/// Image-name property slots for location indicator layers.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum LocationIndicatorImageKind {
    Top,
    Bearing,
    Shadow,
}

impl LocationIndicatorImageKind {
    pub fn raw_value(self) -> u32 {
        match self {
            Self::Top => sys::MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP,
            Self::Bearing => sys::MLN_LOCATION_INDICATOR_IMAGE_KIND_BEARING,
            Self::Shadow => sys::MLN_LOCATION_INDICATOR_IMAGE_KIND_SHADOW,
        }
    }
}

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

    pub(crate) fn to_native(&self) -> NativeTileSourceOptions<'_> {
        // SAFETY: This C helper returns a plain value with no preconditions.
        let mut raw = unsafe { sys::mln_style_tile_source_options_default() };
        if let Some(value) = self.min_zoom {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM;
            raw.min_zoom = value;
        }
        if let Some(value) = self.max_zoom {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM;
            raw.max_zoom = value;
        }
        if let Some(value) = self.attribution.as_deref() {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
            raw.attribution = support::string::string_view(value).raw();
        }
        if let Some(value) = self.scheme {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_SCHEME;
            raw.scheme = value.raw_value();
        }
        if let Some(value) = self.bounds {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS;
            raw.bounds = value.to_native();
        }
        if let Some(value) = self.tile_size {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE;
            raw.tile_size = value;
        }
        if let Some(value) = self.vector_encoding {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
            raw.vector_encoding = value.raw_value();
        }
        if let Some(value) = self.raster_dem_encoding {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
            raw.raster_encoding = value.raw_value();
        }
        NativeTileSourceOptions {
            raw,
            lifetime: PhantomData,
        }
    }
}

pub(crate) struct NativeTileSourceOptions<'a> {
    raw: sys::mln_style_tile_source_options,
    lifetime: PhantomData<&'a str>,
}

impl NativeTileSourceOptions<'_> {
    pub(crate) fn as_ptr(&self) -> *const sys::mln_style_tile_source_options {
        ptr::from_ref(&self.raw)
    }
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

    pub(crate) fn to_native(&self) -> sys::mln_style_image_options {
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
            size: mem::size_of::<sys::mln_style_image_options>() as u32,
            fields,
            pixel_ratio,
            sdf,
        }
    }
}

/// Copied fixed metadata for one runtime style image.
#[derive(Debug, Clone, Copy, PartialEq)]
#[non_exhaustive]
pub struct StyleImageInfo {
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub byte_length: usize,
    pub pixel_ratio: f32,
    pub sdf: bool,
}

impl StyleImageInfo {
    pub(crate) fn from_native(raw: &sys::mln_style_image_info) -> Self {
        Self {
            width: raw.width,
            height: raw.height,
            stride: raw.stride,
            byte_length: raw.byte_length,
            pixel_ratio: raw.pixel_ratio,
            sdf: raw.sdf,
        }
    }
}

/// Copied runtime style image pixels with style-specific metadata.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct StyleImage {
    pub image: PremultipliedRgba8Image,
    pub pixel_ratio: f32,
    pub sdf: bool,
}

pub(crate) fn premultiplied_rgba8_image_to_native(
    image: &PremultipliedRgba8Image,
) -> sys::mln_premultiplied_rgba8_image {
    sys::mln_premultiplied_rgba8_image {
        size: mem::size_of::<sys::mln_premultiplied_rgba8_image>() as u32,
        width: image.info.width,
        height: image.info.height,
        stride: image.info.stride,
        pixels: image.data.as_ptr(),
        byte_length: image.data.len(),
    }
}

impl super::MapHandle {
    /// Loads a style URL through MapLibre Native style APIs.
    ///
    pub fn set_style_url(&self, url: &str) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let url = support::string::c_string(url)?;
        // SAFETY: map is live and url is a NUL-terminated UTF-8 string valid
        // for the duration of this command. The C API copies/consumes it before
        // returning.
        support::check(unsafe { sys::mln_map_set_style_url(map, url.as_ptr()) })?;
        Ok(())
    }

    /// Loads inline style JSON through MapLibre Native style APIs.
    pub fn set_style_json(&self, json: &str) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let json = support::string::c_string(json)?;
        // SAFETY: map is live and json is a NUL-terminated UTF-8 string valid
        // for the duration of this command. The C API copies/consumes it before
        // returning. Inline JSON style replacement completes before a successful
        // return, so old custom geometry callback state can be released after.
        support::check(unsafe { sys::mln_map_set_style_json(map, json.as_ptr()) })?;
        self.inner.clear_custom_geometry_sources();
        Ok(())
    }

    /// Adds a custom geometry source to the current style.
    ///
    /// The callback state is scoped to this map's current style. It is released
    /// on source removal, map close/drop, successful inline JSON style
    /// replacement, or after runtime event polling observes that the loaded
    /// style no longer contains the source. Native may invoke callbacks from
    /// worker threads; callbacks should queue owner-thread work before calling
    /// map APIs.
    pub fn add_custom_geometry_source(
        &self,
        source_id: &str,
        options: CustomGeometrySourceOptions,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id_view = support::string::string_view(source_id);
        let state = CustomGeometrySourceState::new(options);
        let descriptor = state.descriptor();
        // SAFETY: map is live, source_id_view is valid for this call, and
        // descriptor points to callback state retained by this map on success.
        support::check(unsafe {
            sys::mln_map_add_custom_geometry_source(map, source_id_view.raw(), &descriptor)
        })?;
        self.inner
            .custom_geometry_sources
            .borrow_mut()
            .insert(source_id.to_owned(), state);
        Ok(())
    }

    /// Sets custom geometry source data for one canonical tile.
    pub fn set_custom_geometry_source_tile_data(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        data: &GeoJson,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let data = data.try_to_native()?;
        // SAFETY: map is live, source_id is valid for this call, tile_id is
        // passed by value, and data owns the descriptor graph for this call.
        support::check(unsafe {
            sys::mln_map_set_custom_geometry_source_tile_data(
                map,
                source_id.raw(),
                tile_id.to_native(),
                data.as_ptr(),
            )
        })
    }

    /// Invalidates custom geometry source data for one canonical tile.
    pub fn invalidate_custom_geometry_source_tile(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        // SAFETY: map is live, source_id is valid for this call, and tile_id is
        // passed by value.
        support::check(unsafe {
            sys::mln_map_invalidate_custom_geometry_source_tile(
                map,
                source_id.raw(),
                tile_id.to_native(),
            )
        })
    }

    /// Invalidates custom geometry source data inside a geographic region.
    pub fn invalidate_custom_geometry_source_region(
        &self,
        source_id: &str,
        bounds: LatLngBounds,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        // SAFETY: map is live, source_id is valid for this call, and bounds is
        // passed by value.
        support::check(unsafe {
            sys::mln_map_invalidate_custom_geometry_source_region(
                map,
                source_id.raw(),
                bounds.to_native(),
            )
        })
    }

    /// Adds one style source from a style-spec source JSON object.
    pub fn add_style_source_json(&self, source_id: &str, source_json: &JsonValue) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let source_json = source_json.try_to_native()?;
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and source_json owns the descriptor graph for this call.
        support::check(unsafe {
            sys::mln_map_add_style_source_json(map, source_id.raw(), source_json.as_ptr())
        })
    }

    /// Adds a vector source with a TileJSON URL.
    pub fn add_vector_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&TileSourceOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let url = support::string::string_view(url);
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id and url are valid for this call, and
        // options_ptr is null or points to call-scoped native options.
        support::check(unsafe {
            sys::mln_map_add_vector_source_url(map, source_id.raw(), url.raw(), options_ptr)
        })
    }

    /// Adds a vector source with inline tile URLs.
    pub fn add_vector_source_tiles<S: AsRef<str>>(
        &self,
        source_id: &str,
        tiles: &[S],
        options: Option<&TileSourceOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let tile_views: Vec<_> = tiles
            .iter()
            .map(|tile| support::string::string_view(tile.as_ref()))
            .collect();
        let raw_tiles: Vec<_> = tile_views
            .iter()
            .map(support::string::StringView::raw)
            .collect();
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id is valid for this call, raw_tiles
        // points to call-scoped string views, and options_ptr is null or points
        // to call-scoped native options.
        support::check(unsafe {
            sys::mln_map_add_vector_source_tiles(
                map,
                source_id.raw(),
                const_ptr_or_null(&raw_tiles),
                raw_tiles.len(),
                options_ptr,
            )
        })
    }

    /// Adds a raster source with a TileJSON URL.
    pub fn add_raster_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&TileSourceOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let url = support::string::string_view(url);
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id and url are valid for this call, and
        // options_ptr is null or points to call-scoped native options.
        support::check(unsafe {
            sys::mln_map_add_raster_source_url(map, source_id.raw(), url.raw(), options_ptr)
        })
    }

    /// Adds a raster source with inline tile URLs.
    pub fn add_raster_source_tiles<S: AsRef<str>>(
        &self,
        source_id: &str,
        tiles: &[S],
        options: Option<&TileSourceOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let tile_views: Vec<_> = tiles
            .iter()
            .map(|tile| support::string::string_view(tile.as_ref()))
            .collect();
        let raw_tiles: Vec<_> = tile_views
            .iter()
            .map(support::string::StringView::raw)
            .collect();
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id is valid for this call, raw_tiles
        // points to call-scoped string views, and options_ptr is null or points
        // to call-scoped native options.
        support::check(unsafe {
            sys::mln_map_add_raster_source_tiles(
                map,
                source_id.raw(),
                const_ptr_or_null(&raw_tiles),
                raw_tiles.len(),
                options_ptr,
            )
        })
    }

    /// Adds a raster DEM source with a TileJSON URL.
    pub fn add_raster_dem_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&TileSourceOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let url = support::string::string_view(url);
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id and url are valid for this call, and
        // options_ptr is null or points to call-scoped native options.
        support::check(unsafe {
            sys::mln_map_add_raster_dem_source_url(map, source_id.raw(), url.raw(), options_ptr)
        })
    }

    /// Adds a raster DEM source with inline tile URLs.
    pub fn add_raster_dem_source_tiles<S: AsRef<str>>(
        &self,
        source_id: &str,
        tiles: &[S],
        options: Option<&TileSourceOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let tile_views: Vec<_> = tiles
            .iter()
            .map(|tile| support::string::string_view(tile.as_ref()))
            .collect();
        let raw_tiles: Vec<_> = tile_views
            .iter()
            .map(support::string::StringView::raw)
            .collect();
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id is valid for this call, raw_tiles
        // points to call-scoped string views, and options_ptr is null or points
        // to call-scoped native options.
        support::check(unsafe {
            sys::mln_map_add_raster_dem_source_tiles(
                map,
                source_id.raw(),
                const_ptr_or_null(&raw_tiles),
                raw_tiles.len(),
                options_ptr,
            )
        })
    }

    /// Adds an image source that loads its image from a URL.
    ///
    /// Coordinates are borrowed for the call and copied by native on success.
    /// The array entries are in top-left, top-right, bottom-right, bottom-left
    /// order.
    pub fn add_image_source_url(
        &self,
        source_id: &str,
        coordinates: &[LatLng; 4],
        url: &str,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let coordinates = lat_lngs_to_native(coordinates);
        let url = support::string::string_view(url);
        // SAFETY: map is live, source_id and url are explicit-length views
        // valid for this call, and coordinates points to call-scoped native
        // coordinate storage. Native validates coordinate contents.
        support::check(unsafe {
            sys::mln_map_add_image_source_url(
                map,
                source_id.raw(),
                const_ptr_or_null(&coordinates),
                coordinates.len(),
                url.raw(),
            )
        })
    }

    /// Adds an image source with inline premultiplied RGBA8 pixels.
    ///
    /// Coordinates and image pixels are borrowed for the call and copied by
    /// native on success. Coordinate entries are in top-left, top-right,
    /// bottom-right, bottom-left order.
    pub fn add_image_source_image(
        &self,
        source_id: &str,
        coordinates: &[LatLng; 4],
        image: &PremultipliedRgba8Image,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let coordinates = lat_lngs_to_native(coordinates);
        let image = premultiplied_rgba8_image_to_native(image);
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, coordinates points to call-scoped native coordinate
        // storage, and image points into the borrowed Rust image for this call.
        support::check(unsafe {
            sys::mln_map_add_image_source_image(
                map,
                source_id.raw(),
                const_ptr_or_null(&coordinates),
                coordinates.len(),
                &image,
            )
        })
    }

    /// Updates an image source to load its image from a URL.
    pub fn set_image_source_url(&self, source_id: &str, url: &str) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let url = support::string::string_view(url);
        // SAFETY: map is live, and source_id and url are explicit-length views
        // valid for this call.
        support::check(unsafe {
            sys::mln_map_set_image_source_url(map, source_id.raw(), url.raw())
        })
    }

    /// Updates an image source with inline premultiplied RGBA8 pixels.
    pub fn set_image_source_image(
        &self,
        source_id: &str,
        image: &PremultipliedRgba8Image,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let image = premultiplied_rgba8_image_to_native(image);
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and image points into the borrowed Rust image for this call.
        support::check(unsafe { sys::mln_map_set_image_source_image(map, source_id.raw(), &image) })
    }

    /// Updates image source coordinates.
    ///
    /// Coordinates are borrowed for the call and copied by native on success.
    /// The array entries are in top-left, top-right, bottom-right, bottom-left
    /// order.
    pub fn set_image_source_coordinates(
        &self,
        source_id: &str,
        coordinates: &[LatLng; 4],
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let coordinates = lat_lngs_to_native(coordinates);
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and coordinates points to call-scoped native coordinate
        // storage. Native validates coordinate contents.
        support::check(unsafe {
            sys::mln_map_set_image_source_coordinates(
                map,
                source_id.raw(),
                const_ptr_or_null(&coordinates),
                coordinates.len(),
            )
        })
    }

    /// Copies image source coordinates into owned Rust values.
    pub fn image_source_coordinates(&self, source_id: &str) -> Result<Option<[LatLng; 4]>> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let mut coordinates = [sys::mln_lat_lng {
            latitude: 0.0,
            longitude: 0.0,
        }; 4];
        let mut coordinate_count = 0;
        let mut found = false;
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, coordinates has capacity for four native coordinates, and
        // output pointers refer to writable storage.
        support::check(unsafe {
            sys::mln_map_get_image_source_coordinates(
                map,
                source_id.raw(),
                coordinates.as_mut_ptr(),
                coordinates.len(),
                &mut coordinate_count,
                &mut found,
            )
        })?;
        if !found {
            return Ok(None);
        }
        if coordinate_count != coordinates.len() {
            return Err(Error::new(
                ErrorKind::NativeError,
                None,
                "native image source coordinate count did not match Rust image source invariant",
            ));
        }
        Ok(Some(coordinates.map(LatLng::from_native)))
    }

    /// Removes one style source by ID.
    ///
    /// Returns whether a source existed and was removed. Native returns an
    /// error when a layer still uses the source.
    pub fn remove_style_source(&self, source_id: &str) -> Result<bool> {
        let map = self.inner.as_ptr()?;
        let source_id_key = source_id.to_owned();
        let source_id = support::string::string_view(source_id);
        let mut removed = false;
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and removed points to writable storage.
        support::check(unsafe {
            sys::mln_map_remove_style_source(map, source_id.raw(), &mut removed)
        })?;
        if removed {
            self.inner
                .custom_geometry_sources
                .borrow_mut()
                .remove(&source_id_key);
        }
        Ok(removed)
    }

    /// Reports whether a style source ID exists.
    pub fn style_source_exists(&self, source_id: &str) -> Result<bool> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let mut exists = false;
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and exists points to writable storage.
        support::check(unsafe {
            sys::mln_map_style_source_exists(map, source_id.raw(), &mut exists)
        })?;
        Ok(exists)
    }

    /// Adds or replaces one runtime style image.
    pub fn set_style_image(
        &self,
        image_id: &str,
        image: &PremultipliedRgba8Image,
        options: Option<&StyleImageOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let image_id = support::string::string_view(image_id);
        let image = premultiplied_rgba8_image_to_native(image);
        let options = options.map(StyleImageOptions::to_native);
        let options_ptr = options.as_ref().map_or(ptr::null(), ptr::from_ref);
        // SAFETY: map is live, image_id is an explicit-length view valid for
        // this call, image points into the borrowed Rust image for this call,
        // and options_ptr is either null or points to call-scoped options.
        support::check(unsafe {
            sys::mln_map_set_style_image(map, image_id.raw(), &image, options_ptr)
        })
    }

    /// Removes one runtime style image by ID.
    ///
    /// Returns whether an image existed and was removed.
    pub fn remove_style_image(&self, image_id: &str) -> Result<bool> {
        let map = self.inner.as_ptr()?;
        let image_id = support::string::string_view(image_id);
        let mut removed = false;
        // SAFETY: map is live, image_id is an explicit-length view valid for
        // this call, and removed points to writable storage.
        support::check(unsafe {
            sys::mln_map_remove_style_image(map, image_id.raw(), &mut removed)
        })?;
        Ok(removed)
    }

    /// Reports whether a runtime style image ID exists.
    pub fn style_image_exists(&self, image_id: &str) -> Result<bool> {
        let map = self.inner.as_ptr()?;
        let image_id = support::string::string_view(image_id);
        let mut exists = false;
        // SAFETY: map is live, image_id is an explicit-length view valid for
        // this call, and exists points to writable storage.
        support::check(unsafe {
            sys::mln_map_style_image_exists(map, image_id.raw(), &mut exists)
        })?;
        Ok(exists)
    }

    /// Copies fixed metadata for one runtime style image.
    pub fn style_image_info(&self, image_id: &str) -> Result<Option<StyleImageInfo>> {
        let map = self.inner.as_ptr()?;
        let image_id = support::string::string_view(image_id);
        let mut info = sys::mln_style_image_info {
            size: mem::size_of::<sys::mln_style_image_info>() as u32,
            width: 0,
            height: 0,
            stride: 0,
            byte_length: 0,
            pixel_ratio: 1.0,
            sdf: false,
        };
        let mut found = false;
        // SAFETY: map is live, image_id is an explicit-length view valid for
        // this call, info has its ABI size initialized, and found points to
        // writable storage.
        support::check(unsafe {
            sys::mln_map_get_style_image_info(map, image_id.raw(), &mut info, &mut found)
        })?;
        Ok(found.then(|| StyleImageInfo::from_native(&info)))
    }

    /// Copies one runtime style image into owned tightly packed premultiplied RGBA8 pixels.
    pub fn copy_style_image_premultiplied_rgba8(
        &self,
        image_id: &str,
    ) -> Result<Option<StyleImage>> {
        let map = self.inner.as_ptr()?;
        let image_id = support::string::string_view(image_id);
        let mut raw_info = sys::mln_style_image_info {
            size: mem::size_of::<sys::mln_style_image_info>() as u32,
            width: 0,
            height: 0,
            stride: 0,
            byte_length: 0,
            pixel_ratio: 1.0,
            sdf: false,
        };
        let mut info_found = false;
        // SAFETY: map is live, image_id is an explicit-length view valid for
        // this call, raw_info has its ABI size initialized, and info_found
        // points to writable storage.
        support::check(unsafe {
            sys::mln_map_get_style_image_info(map, image_id.raw(), &mut raw_info, &mut info_found)
        })?;
        if !info_found {
            return Ok(None);
        }
        let info = StyleImageInfo::from_native(&raw_info);

        let mut data = vec![0u8; info.byte_length];
        let mut copied_size = 0;
        let mut found = false;
        let pixels = if data.is_empty() {
            ptr::null_mut()
        } else {
            data.as_mut_ptr()
        };
        // SAFETY: map is live, image_id remains valid for this call, data is
        // writable for info.byte_length bytes (or null with zero capacity), and
        // output pointers refer to writable storage.
        support::check(unsafe {
            sys::mln_map_copy_style_image_premultiplied_rgba8(
                map,
                image_id.raw(),
                pixels,
                data.len(),
                &mut copied_size,
                &mut found,
            )
        })?;
        if !found {
            return Ok(None);
        }
        if copied_size > data.len() {
            return Err(Error::new(
                ErrorKind::NativeError,
                None,
                "native style image byte length exceeded caller buffer",
            ));
        }
        data.truncate(copied_size);
        Ok(Some(StyleImage {
            image: PremultipliedRgba8Image {
                info: TextureImageInfo {
                    width: info.width,
                    height: info.height,
                    stride: info.stride,
                    byte_length: copied_size,
                },
                data,
            },
            pixel_ratio: info.pixel_ratio,
            sdf: info.sdf,
        }))
    }

    /// Gets one style source type.
    pub fn style_source_type(&self, source_id: &str) -> Result<Option<SourceType>> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let mut raw_source_type = sys::MLN_STYLE_SOURCE_TYPE_UNKNOWN;
        let mut found = false;
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and output pointers refer to writable storage.
        support::check(unsafe {
            sys::mln_map_get_style_source_type(
                map,
                source_id.raw(),
                &mut raw_source_type,
                &mut found,
            )
        })?;
        Ok(found.then(|| SourceType::from_raw(raw_source_type)))
    }

    /// Copies fixed metadata and attribution for one style source.
    pub fn style_source_info(&self, source_id: &str) -> Result<Option<SourceInfo>> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let mut info = sys::mln_style_source_info {
            size: mem::size_of::<sys::mln_style_source_info>() as u32,
            type_: sys::MLN_STYLE_SOURCE_TYPE_UNKNOWN,
            id_size: 0,
            is_volatile: false,
            has_attribution: false,
            attribution_size: 0,
        };
        let mut found = false;
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, info has its ABI size initialized, and found points to
        // writable storage.
        support::check(unsafe {
            sys::mln_map_get_style_source_info(map, source_id.raw(), &mut info, &mut found)
        })?;
        if !found {
            return Ok(None);
        }

        let attribution = if info.has_attribution {
            match self.copy_style_source_attribution(map, source_id.raw(), info.attribution_size)? {
                Some(attribution) => Some(attribution),
                None => return Ok(None),
            }
        } else {
            None
        };

        Ok(Some(SourceInfo {
            source_type: SourceType::from_raw(info.type_),
            raw_source_type: info.type_,
            is_volatile: info.is_volatile,
            attribution,
        }))
    }

    fn copy_style_source_attribution(
        &self,
        map: *mut sys::mln_map,
        source_id: sys::mln_string_view,
        attribution_size: usize,
    ) -> Result<Option<String>> {
        if attribution_size == 0 {
            let mut copied_size = 0;
            let mut found = false;
            // SAFETY: map is live, source_id remains valid for this call,
            // capacity is zero so the output buffer may be null, and output
            // pointers refer to writable storage.
            support::check(unsafe {
                sys::mln_map_copy_style_source_attribution(
                    map,
                    source_id,
                    ptr::null_mut(),
                    0,
                    &mut copied_size,
                    &mut found,
                )
            })?;
            return Ok(found.then(String::new));
        }

        let mut buffer = vec![0u8; attribution_size];
        let mut copied_size = 0;
        let mut found = false;
        // SAFETY: map is live, source_id remains valid for this call, buffer is
        // writable for attribution_size bytes, and output pointers refer to
        // writable storage.
        support::check(unsafe {
            sys::mln_map_copy_style_source_attribution(
                map,
                source_id,
                buffer.as_mut_ptr().cast(),
                buffer.len(),
                &mut copied_size,
                &mut found,
            )
        })?;
        if !found {
            return Ok(None);
        }
        if copied_size > buffer.len() {
            return Err(Error::new(
                ErrorKind::NativeError,
                None,
                "native style source attribution size exceeded caller buffer",
            ));
        }
        buffer.truncate(copied_size);
        String::from_utf8(buffer).map(Some).map_err(|error| {
            Error::invalid_argument(format!(
                "native style source attribution was not valid UTF-8: {error}"
            ))
        })
    }

    /// Adds a GeoJSON source with inline data.
    pub fn add_geojson_source_data(&self, source_id: &str, data: &GeoJson) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let data = data.try_to_native()?;
        // SAFETY: map is live, source_id is valid for this call, and data owns
        // the descriptor graph for this call.
        support::check(unsafe {
            sys::mln_map_add_geojson_source_data(map, source_id.raw(), data.as_ptr())
        })
    }

    /// Updates one GeoJSON source with inline data.
    pub fn set_geojson_source_data(&self, source_id: &str, data: &GeoJson) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let data = data.try_to_native()?;
        // SAFETY: map is live, source_id is valid for this call, and data owns
        // the descriptor graph for this call.
        support::check(unsafe {
            sys::mln_map_set_geojson_source_data(map, source_id.raw(), data.as_ptr())
        })
    }

    /// Adds one style layer from a full style-spec layer JSON object.
    pub fn add_style_layer_json(
        &self,
        layer_json: &JsonValue,
        before_layer_id: Option<&str>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let layer_json = layer_json.try_to_native()?;
        let before_layer_id = support::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live, layer_json owns the descriptor graph, and
        // before_layer_id is an explicit-length view valid for this call.
        support::check(unsafe {
            sys::mln_map_add_style_layer_json(map, layer_json.as_ptr(), before_layer_id.raw())
        })
    }

    /// Adds a hillshade layer for a raster DEM source.
    pub fn add_hillshade_layer(
        &self,
        layer_id: &str,
        source_id: &str,
        before_layer_id: Option<&str>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        let source_id = support::string::string_view(source_id);
        let before_layer_id = support::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live, and all string views are valid for this call.
        support::check(unsafe {
            sys::mln_map_add_hillshade_layer(
                map,
                layer_id.raw(),
                source_id.raw(),
                before_layer_id.raw(),
            )
        })
    }

    /// Adds a color-relief layer for a raster DEM source.
    pub fn add_color_relief_layer(
        &self,
        layer_id: &str,
        source_id: &str,
        before_layer_id: Option<&str>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        let source_id = support::string::string_view(source_id);
        let before_layer_id = support::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live, and all string views are valid for this call.
        support::check(unsafe {
            sys::mln_map_add_color_relief_layer(
                map,
                layer_id.raw(),
                source_id.raw(),
                before_layer_id.raw(),
            )
        })
    }

    /// Adds a source-free location indicator layer.
    pub fn add_location_indicator_layer(
        &self,
        layer_id: &str,
        before_layer_id: Option<&str>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        let before_layer_id = support::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live, and string views are valid for this call.
        support::check(unsafe {
            sys::mln_map_add_location_indicator_layer(map, layer_id.raw(), before_layer_id.raw())
        })
    }

    /// Sets a location indicator layer location.
    pub fn set_location_indicator_location(
        &self,
        layer_id: &str,
        coordinate: LatLng,
        altitude: f64,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        // SAFETY: map is live, layer_id is valid for this call, and coordinate
        // is passed by value.
        support::check(unsafe {
            sys::mln_map_set_location_indicator_location(
                map,
                layer_id.raw(),
                coordinate.to_native(),
                altitude,
            )
        })
    }

    /// Sets a location indicator layer bearing in degrees.
    pub fn set_location_indicator_bearing(&self, layer_id: &str, bearing: f64) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        // SAFETY: map is live and layer_id is valid for this call.
        support::check(unsafe {
            sys::mln_map_set_location_indicator_bearing(map, layer_id.raw(), bearing)
        })
    }

    /// Sets a location indicator layer accuracy radius in logical pixels.
    pub fn set_location_indicator_accuracy_radius(
        &self,
        layer_id: &str,
        radius: f64,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        // SAFETY: map is live and layer_id is valid for this call.
        support::check(unsafe {
            sys::mln_map_set_location_indicator_accuracy_radius(map, layer_id.raw(), radius)
        })
    }

    /// Sets one location indicator image-name property.
    pub fn set_location_indicator_image_name(
        &self,
        layer_id: &str,
        image_kind: LocationIndicatorImageKind,
        image_id: &str,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        let image_id = support::string::string_view(image_id);
        // SAFETY: map is live, string views are valid for this call, and
        // image_kind is a valid C enum value.
        support::check(unsafe {
            sys::mln_map_set_location_indicator_image_name(
                map,
                layer_id.raw(),
                image_kind.raw_value(),
                image_id.raw(),
            )
        })
    }

    /// Copies one style layer as a full style-spec JSON object.
    pub fn style_layer_json(&self, layer_id: &str) -> Result<Option<JsonValue>> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        let mut out = support::ptr::OutPtr::<sys::mln_json_snapshot>::new();
        let mut found = false;
        // SAFETY: map is live, layer_id is valid for this call, out is a
        // null-initialized out-pointer, and found points to writable storage.
        support::check(unsafe {
            sys::mln_map_get_style_layer_json(map, layer_id.raw(), out.as_mut_ptr(), &mut found)
        })?;
        let snapshot = json_snapshot(out.into_option())?;
        if found { Ok(snapshot) } else { Ok(None) }
    }

    /// Sets the style light from a style-spec light JSON object.
    pub fn set_style_light_json(&self, light_json: &JsonValue) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let light_json = light_json.try_to_native()?;
        // SAFETY: map is live and light_json owns the descriptor graph for this call.
        support::check(unsafe { sys::mln_map_set_style_light_json(map, light_json.as_ptr()) })
    }

    /// Sets one style light property.
    pub fn set_style_light_property(&self, property_name: &str, value: &JsonValue) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let property_name = support::string::string_view(property_name);
        let value = value.try_to_native()?;
        // SAFETY: map is live, property_name is valid for this call, and value
        // owns the descriptor graph for this call.
        support::check(unsafe {
            sys::mln_map_set_style_light_property(map, property_name.raw(), value.as_ptr())
        })
    }

    /// Copies one style light property as a style-spec JSON value.
    pub fn style_light_property(&self, property_name: &str) -> Result<Option<JsonValue>> {
        let map = self.inner.as_ptr()?;
        let property_name = support::string::string_view(property_name);
        let mut out = support::ptr::OutPtr::<sys::mln_json_snapshot>::new();
        // SAFETY: map is live, property_name is valid for this call, and out is
        // a null-initialized out-pointer.
        support::check(unsafe {
            sys::mln_map_get_style_light_property(map, property_name.raw(), out.as_mut_ptr())
        })?;
        json_snapshot(out.into_option())
    }

    /// Sets one layer style property.
    pub fn set_layer_property(
        &self,
        layer_id: &str,
        property_name: &str,
        value: &JsonValue,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        let property_name = support::string::string_view(property_name);
        let value = value.try_to_native()?;
        // SAFETY: map is live, string views are valid for this call, and value
        // owns the descriptor graph for this call.
        support::check(unsafe {
            sys::mln_map_set_layer_property(
                map,
                layer_id.raw(),
                property_name.raw(),
                value.as_ptr(),
            )
        })
    }

    /// Copies one layer style property as a style-spec JSON value.
    pub fn layer_property(&self, layer_id: &str, property_name: &str) -> Result<Option<JsonValue>> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        let property_name = support::string::string_view(property_name);
        let mut out = support::ptr::OutPtr::<sys::mln_json_snapshot>::new();
        // SAFETY: map is live, string views are valid for this call, and out is
        // a null-initialized out-pointer.
        support::check(unsafe {
            sys::mln_map_get_layer_property(
                map,
                layer_id.raw(),
                property_name.raw(),
                out.as_mut_ptr(),
            )
        })?;
        json_snapshot(out.into_option())
    }

    /// Sets or clears one layer filter.
    pub fn set_layer_filter(&self, layer_id: &str, filter: Option<&JsonValue>) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        let native_filter = filter.map(JsonValue::try_to_native).transpose()?;
        // SAFETY: map is live, layer_id is valid for this call, and the
        // optional filter descriptor is either null or valid for this call.
        support::check(unsafe {
            sys::mln_map_set_layer_filter(
                map,
                layer_id.raw(),
                native_filter
                    .as_ref()
                    .map_or(ptr::null(), |filter| filter.as_ptr()),
            )
        })
    }

    /// Copies one layer filter as a style-spec JSON value.
    pub fn layer_filter(&self, layer_id: &str) -> Result<Option<JsonValue>> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        let mut out = support::ptr::OutPtr::<sys::mln_json_snapshot>::new();
        // SAFETY: map is live, layer_id is valid for this call, and out is a
        // null-initialized out-pointer.
        support::check(unsafe {
            sys::mln_map_get_layer_filter(map, layer_id.raw(), out.as_mut_ptr())
        })?;
        json_snapshot(out.into_option())
    }

    /// Copies current style source IDs into owned Rust strings.
    pub fn style_source_ids(&self) -> Result<Vec<String>> {
        let map = self.inner.as_ptr()?;
        let mut out = support::ptr::OutPtr::<sys::mln_style_id_list>::new();
        // SAFETY: map is live and out is a null-initialized out-pointer owned by
        // this call. On success the returned handle is wrapped and destroyed by
        // the copying helper below.
        support::check(unsafe { sys::mln_map_list_style_source_ids(map, out.as_mut_ptr()) })?;
        let list = out_handle(out, "mln_style_id_list")?;
        // SAFETY: list came from mln_map_list_style_source_ids and is owned by
        // this function until the guard drops.
        let list = unsafe { support::handle::style_id_list(list.as_ptr()) }?;
        copy_style_id_list(&list)
    }

    /// Copies current style layer IDs into owned Rust strings.
    pub fn style_layer_ids(&self) -> Result<Vec<String>> {
        let map = self.inner.as_ptr()?;
        let mut out = support::ptr::OutPtr::<sys::mln_style_id_list>::new();
        // SAFETY: map is live and out is a null-initialized out-pointer owned by
        // this call. On success the returned handle is wrapped and destroyed by
        // the copying helper below.
        support::check(unsafe { sys::mln_map_list_style_layer_ids(map, out.as_mut_ptr()) })?;
        let list = out_handle(out, "mln_style_id_list")?;
        // SAFETY: list came from mln_map_list_style_layer_ids and is owned by
        // this function until the guard drops.
        let list = unsafe { support::handle::style_id_list(list.as_ptr()) }?;
        copy_style_id_list(&list)
    }
}
