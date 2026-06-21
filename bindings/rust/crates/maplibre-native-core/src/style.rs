use std::ffi::c_void;
use std::ptr;
use std::ptr::NonNull;

use maplibre_native_sys as sys;

use crate::enums::{RasterDemEncoding, SourceType, TileScheme, VectorTileEncoding};
use crate::string::{StringView, string_view};
use crate::values::{
    LatLngBounds, PremultipliedRgba8Image, StyleImageInfo, TextureImageInfo,
    lat_lng_bounds_to_native,
};

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

pub struct NativeTileUrls<'a> {
    raw_tiles: Vec<sys::mln_string_view>,
    _tile_views: Vec<StringView<'a>>,
}

impl<'a> NativeTileUrls<'a> {
    pub fn new<S: AsRef<str> + 'a>(tiles: &'a [S]) -> Self {
        let tile_views: Vec<_> = tiles
            .iter()
            .map(|tile| string_view(tile.as_ref()))
            .collect();
        let raw_tiles: Vec<_> = tile_views.iter().map(StringView::raw).collect();
        Self {
            raw_tiles,
            _tile_views: tile_views,
        }
    }

    pub fn as_ptr(&self) -> *const sys::mln_string_view {
        crate::ptr::const_ptr_or_null(&self.raw_tiles)
    }

    pub fn len(&self) -> usize {
        self.raw_tiles.len()
    }

    pub fn is_empty(&self) -> bool {
        self.raw_tiles.is_empty()
    }
}

pub type CustomGeometryTileCallbackFn =
    unsafe extern "C" fn(*mut c_void, sys::mln_canonical_tile_id);

#[derive(Debug, Clone, Copy)]
pub struct CustomGeometrySourceDescriptorFields {
    pub fetch_tile: Option<CustomGeometryTileCallbackFn>,
    pub cancel_tile: Option<CustomGeometryTileCallbackFn>,
    pub user_data: *mut c_void,
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
    pub tolerance: Option<f64>,
    pub tile_size: Option<u32>,
    pub buffer: Option<u32>,
    pub clip: Option<bool>,
    pub wrap: Option<bool>,
}

pub fn custom_geometry_source_options_to_native(
    fields: CustomGeometrySourceDescriptorFields,
) -> sys::mln_custom_geometry_source_options {
    // SAFETY: This C helper returns a plain value with no preconditions.
    let mut raw = unsafe { sys::mln_custom_geometry_source_options_default() };
    raw.fetch_tile = fields.fetch_tile;
    raw.cancel_tile = fields.cancel_tile;
    raw.user_data = fields.user_data;
    if let Some(min_zoom) = fields.min_zoom {
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
        raw.min_zoom = min_zoom;
    }
    if let Some(max_zoom) = fields.max_zoom {
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
        raw.max_zoom = max_zoom;
    }
    if let Some(tolerance) = fields.tolerance {
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE;
        raw.tolerance = tolerance;
    }
    if let Some(tile_size) = fields.tile_size {
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE;
        raw.tile_size = tile_size;
    }
    if let Some(buffer) = fields.buffer {
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER;
        raw.buffer = buffer;
    }
    if let Some(clip) = fields.clip {
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP;
        raw.clip = clip;
    }
    if let Some(wrap) = fields.wrap {
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
        raw.wrap = wrap;
    }
    raw
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

pub fn empty_style_source_info() -> sys::mln_style_source_info {
    sys::mln_style_source_info {
        size: std::mem::size_of::<sys::mln_style_source_info>() as u32,
        type_: sys::MLN_STYLE_SOURCE_TYPE_UNKNOWN,
        id_size: 0,
        is_volatile: false,
        has_attribution: false,
        attribution_size: 0,
    }
}

pub fn style_source_info_from_native(
    info: &sys::mln_style_source_info,
    attribution: Option<String>,
) -> SourceInfo {
    SourceInfo {
        source_type: SourceType::from_raw(info.type_),
        raw_source_type: info.type_,
        is_volatile: info.is_volatile,
        attribution,
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

impl StyleImage {
    pub fn new(image: PremultipliedRgba8Image, pixel_ratio: f32, sdf: bool) -> Self {
        Self {
            image,
            pixel_ratio,
            sdf,
        }
    }
}

pub fn style_image_from_copied_premultiplied_rgba8(
    info: StyleImageInfo,
    mut data: Vec<u8>,
    copied_size: usize,
) -> crate::Result<StyleImage> {
    if copied_size > data.len() {
        return Err(crate::Error::new(
            crate::ErrorKind::NativeError,
            None,
            "native style image byte length exceeded caller buffer",
        ));
    }
    data.truncate(copied_size);
    Ok(StyleImage::new(
        PremultipliedRgba8Image::new(
            TextureImageInfo::new(info.width, info.height, info.stride, copied_size),
            data,
        ),
        info.pixel_ratio,
        info.sdf,
    ))
}

/// Options for adding or replacing a runtime style image.
#[derive(Debug, Clone, Default, PartialEq)]
#[non_exhaustive]
pub struct StyleImageOptions {
    pub pixel_ratio: Option<f32>,
    pub sdf: Option<bool>,
}

impl StyleImageOptions {
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

pub fn empty_style_image_info() -> sys::mln_style_image_info {
    sys::mln_style_image_info {
        size: std::mem::size_of::<sys::mln_style_image_info>() as u32,
        width: 0,
        height: 0,
        stride: 0,
        byte_length: 0,
        pixel_ratio: 1.0,
        sdf: false,
    }
}

#[doc(hidden)]
pub trait TileSourceOptionsNativeExt {
    fn to_native(&self) -> NativeTileSourceOptions<'_>;
}

impl TileSourceOptionsNativeExt for TileSourceOptions {
    fn to_native(&self) -> NativeTileSourceOptions<'_> {
        tile_source_options_to_native(self)
    }
}

#[doc(hidden)]
pub trait StyleImageOptionsNativeExt {
    fn to_native(&self) -> sys::mln_style_image_options;
}

impl StyleImageOptionsNativeExt for StyleImageOptions {
    fn to_native(&self) -> sys::mln_style_image_options {
        style_image_options_to_native(self)
    }
}

/// Copies an owned native style ID list into owned Rust strings.
///
/// # Safety
///
/// `ptr` must point to a live `mln_style_id_list` handle owned by the caller
/// and returned by the matching C API. This function takes ownership of that
/// handle and releases it before returning, including on copy errors.
pub unsafe fn copy_style_id_list(
    ptr: NonNull<sys::mln_style_id_list>,
) -> crate::Result<Vec<String>> {
    // SAFETY: ptr is an owned style ID list returned by the C API and released by the guard.
    let list = unsafe { crate::handle::style_id_list(ptr.as_ptr()) }?;
    let mut count = 0;
    // SAFETY: list is live and count points to writable storage.
    crate::check(unsafe { sys::mln_style_id_list_count(list.as_ptr(), &mut count) })?;

    let mut ids = Vec::with_capacity(count);
    for index in 0..count {
        let mut view = sys::mln_string_view {
            data: ptr::null(),
            size: 0,
        };
        // SAFETY: list is live, index is less than count, and view points to writable storage.
        crate::check(unsafe { sys::mln_style_id_list_get(list.as_ptr(), index, &mut view) })?;
        // SAFETY: The C API returns a view into list-owned storage that remains valid here.
        ids.push(unsafe { crate::string::copy_string_view(view) }?);
    }
    Ok(ids)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{LatLng, LatLngBounds};

    #[test]
    fn tile_source_options_materialize_masks_fields_and_views() {
        let options = TileSourceOptions {
            min_zoom: Some(1.0),
            max_zoom: Some(22.0),
            attribution: Some("© MapLibre".into()),
            scheme: Some(TileScheme::Tms),
            bounds: Some(LatLngBounds::new(
                LatLng::new(1.0, 2.0),
                LatLng::new(3.0, 4.0),
            )),
            tile_size: Some(512),
            vector_encoding: Some(VectorTileEncoding::Mvt),
            raster_dem_encoding: Some(RasterDemEncoding::Mapbox),
        };

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
    fn native_tile_urls_materialize_string_view_array() {
        let urls = vec!["a://tile".to_string(), "b://tile".to_string()];
        let native = NativeTileUrls::new(&urls);

        assert_eq!(native.len(), 2);
        assert!(!native.as_ptr().is_null());
        // SAFETY: native keeps string views and raw array storage live for this scope.
        assert_eq!(
            unsafe { crate::string::copy_string_view(*native.as_ptr()) }.unwrap(),
            "a://tile"
        );

        let empty: Vec<String> = Vec::new();
        let native = NativeTileUrls::new(&empty);
        assert!(native.is_empty());
        assert!(native.as_ptr().is_null());
    }

    #[test]
    fn custom_geometry_source_options_materialize_masks_and_callbacks() {
        unsafe extern "C" fn fetch(_user_data: *mut c_void, _tile_id: sys::mln_canonical_tile_id) {}
        unsafe extern "C" fn cancel(_user_data: *mut c_void, _tile_id: sys::mln_canonical_tile_id) {
        }

        let raw = custom_geometry_source_options_to_native(CustomGeometrySourceDescriptorFields {
            fetch_tile: Some(fetch),
            cancel_tile: Some(cancel),
            user_data: 0x1234usize as *mut c_void,
            min_zoom: Some(1.0),
            max_zoom: Some(22.0),
            tolerance: Some(0.5),
            tile_size: Some(512),
            buffer: Some(8),
            clip: Some(true),
            wrap: Some(false),
        });

        assert_eq!(
            raw.size,
            std::mem::size_of::<sys::mln_custom_geometry_source_options>() as u32
        );
        assert_eq!(
            raw.fetch_tile.map(|callback| callback as usize),
            Some(fetch as *const () as usize)
        );
        assert_eq!(
            raw.cancel_tile.map(|callback| callback as usize),
            Some(cancel as *const () as usize)
        );
        assert_eq!(raw.user_data, 0x1234usize as *mut c_void);
        assert_eq!(
            raw.fields,
            sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM
                | sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM
                | sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE
                | sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE
                | sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER
                | sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP
                | sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP
        );
        assert_eq!(raw.min_zoom, 1.0);
        assert_eq!(raw.max_zoom, 22.0);
        assert_eq!(raw.tolerance, 0.5);
        assert_eq!(raw.tile_size, 512);
        assert_eq!(raw.buffer, 8);
        assert!(raw.clip);
        assert!(!raw.wrap);
    }

    #[test]
    fn style_source_info_copies_raw_fields_and_attribution() {
        let raw = sys::mln_style_source_info {
            type_: sys::MLN_STYLE_SOURCE_TYPE_VECTOR,
            is_volatile: true,
            has_attribution: true,
            attribution_size: 11,
            ..empty_style_source_info()
        };

        let copied = style_source_info_from_native(&raw, Some("© MapLibre".to_string()));

        assert_eq!(copied.source_type, SourceType::Vector);
        assert_eq!(copied.raw_source_type, sys::MLN_STYLE_SOURCE_TYPE_VECTOR);
        assert!(copied.is_volatile);
        assert_eq!(copied.attribution.as_deref(), Some("© MapLibre"));
    }

    #[test]
    fn style_image_copy_builds_owned_image_and_rejects_oversized_copy() {
        let info = StyleImageInfo {
            width: 2,
            height: 2,
            stride: 8,
            byte_length: 8,
            pixel_ratio: 2.0,
            sdf: true,
        };
        let image = style_image_from_copied_premultiplied_rgba8(info, vec![1, 2, 3, 4], 3).unwrap();

        assert_eq!(image.image.info.width, 2);
        assert_eq!(image.image.info.byte_length, 3);
        assert_eq!(image.image.data, vec![1, 2, 3]);
        assert_eq!(image.pixel_ratio, 2.0);
        assert!(image.sdf);

        let error = style_image_from_copied_premultiplied_rgba8(info, vec![1, 2], 3).unwrap_err();
        assert!(error.to_string().contains("byte length exceeded"));
    }

    #[test]
    fn style_image_options_materialize_masks_and_defaults() {
        let empty_info = empty_style_image_info();
        assert_eq!(
            empty_info.size,
            std::mem::size_of::<sys::mln_style_image_info>() as u32
        );
        assert_eq!(empty_info.pixel_ratio, 1.0);
        assert!(!empty_info.sdf);

        let default_raw = style_image_options_to_native(&StyleImageOptions::default());
        assert_eq!(
            default_raw.size,
            std::mem::size_of::<sys::mln_style_image_options>() as u32
        );
        assert_eq!(default_raw.fields, 0);
        assert_eq!(default_raw.pixel_ratio, 1.0);
        assert!(!default_raw.sdf);

        let raw = style_image_options_to_native(&StyleImageOptions {
            pixel_ratio: Some(2.0),
            sdf: Some(true),
        });
        assert_eq!(
            raw.fields,
            sys::MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO | sys::MLN_STYLE_IMAGE_OPTION_SDF
        );
        assert_eq!(raw.pixel_ratio, 2.0);
        assert!(raw.sdf);
    }
}
