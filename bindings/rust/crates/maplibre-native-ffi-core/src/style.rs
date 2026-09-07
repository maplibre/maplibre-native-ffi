use std::ffi::c_void;
use std::ptr;

use maplibre_native_ffi_sys as sys;

use crate::enums::{
    RasterDemEncoding, SourceType, StyleImageTextFit, StyleLayerVisibility, TileScheme,
    VectorTileEncoding,
};
use crate::string::{StringView, buffer_view, string_view};
use crate::values::{
    LatLngBounds, PremultipliedRgba8Image, StyleImageInfo, TextureImageInfo,
    lat_lng_bounds_from_native, lat_lng_bounds_to_native,
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

/// Options for GeoJSON sources. Fixed when the source is created; updating the
/// source's URL or data keeps the options it was added with.
#[derive(Debug, Clone, Default, PartialEq)]
#[non_exhaustive]
pub struct GeoJsonSourceOptions {
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
    pub tolerance: Option<f64>,
    pub cluster_max_zoom: Option<f64>,
    /// Cluster aggregation expressions keyed by property name, as a JSON object
    /// whose members follow the MapLibre Style Spec `clusterProperties` form.
    pub cluster_properties: Option<Vec<u8>>,
    pub tile_size: Option<u32>,
    pub buffer: Option<u32>,
    pub cluster_radius: Option<u32>,
    pub cluster_min_points: Option<u32>,
    pub line_metrics: Option<bool>,
    pub cluster: Option<bool>,
    /// Slices requested tiles out of installed data inline during the update
    /// pass, so data installed through `set_geojson_source_data` reaches the
    /// next rendered frame. The map can override this at runtime through
    /// `set_geojson_source_synchronous_tiling`.
    pub synchronous_tiling: Option<bool>,
}

impl GeoJsonSourceOptions {
    fn try_to_native(&self) -> crate::Result<NativeGeoJsonSourceOptions> {
        NativeGeoJsonSourceOptions::new(self)
    }
}

pub struct NativeGeoJsonSourceOptions {
    raw: sys::mln_geojson_source_options,
    _cluster_properties: Option<Vec<u8>>,
}

impl NativeGeoJsonSourceOptions {
    fn new(options: &GeoJsonSourceOptions) -> crate::Result<Self> {
        // SAFETY: This C helper returns a plain value with no preconditions.
        let mut raw = unsafe { sys::mln_geojson_source_options_default() };
        if let Some(value) = options.min_zoom {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM;
            raw.min_zoom = value;
        }
        if let Some(value) = options.max_zoom {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM;
            raw.max_zoom = value;
        }
        if let Some(value) = options.tolerance {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_TOLERANCE;
            raw.tolerance = value;
        }
        if let Some(value) = options.cluster_max_zoom {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM;
            raw.cluster_max_zoom = value;
        }
        let cluster_properties = options.cluster_properties.clone();
        if let Some(properties) = cluster_properties.as_deref() {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
            raw.cluster_properties = buffer_view(properties);
        }
        if let Some(value) = options.tile_size {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE;
            raw.tile_size = value;
        }
        if let Some(value) = options.buffer {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_BUFFER;
            raw.buffer = value;
        }
        if let Some(value) = options.cluster_radius {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS;
            raw.cluster_radius = value;
        }
        if let Some(value) = options.cluster_min_points {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS;
            raw.cluster_min_points = value;
        }
        if let Some(value) = options.line_metrics {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS;
            raw.line_metrics = value;
        }
        if let Some(value) = options.cluster {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
            raw.cluster = value;
        }
        if let Some(value) = options.synchronous_tiling {
            raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING;
            raw.synchronous_tiling = value;
        }
        Ok(Self {
            raw,
            _cluster_properties: cluster_properties,
        })
    }

    pub fn as_ptr(&self) -> *const sys::mln_geojson_source_options {
        ptr::from_ref(&self.raw)
    }
}

impl AsRef<sys::mln_geojson_source_options> for NativeGeoJsonSourceOptions {
    fn as_ref(&self) -> &sys::mln_geojson_source_options {
        &self.raw
    }
}

pub fn geojson_source_options_to_native(
    options: &GeoJsonSourceOptions,
) -> crate::Result<NativeGeoJsonSourceOptions> {
    options.try_to_native()
}

pub struct NativeTileUrls<'a> {
    raw_tiles: Vec<sys::mln_buffer_view>,
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

    pub fn as_ptr(&self) -> *const sys::mln_buffer_view {
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

pub type CustomGeometryReleaseCallbackFn = unsafe extern "C" fn(*mut c_void);

#[derive(Debug, Clone, Copy)]
pub struct CustomGeometrySourceDescriptorFields {
    pub fetch_tile: Option<CustomGeometryTileCallbackFn>,
    pub cancel_tile: Option<CustomGeometryTileCallbackFn>,
    /// Invoked once when the C API stops referencing `user_data`.
    pub release_user_data: Option<CustomGeometryReleaseCallbackFn>,
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
    raw.release_user_data = fields.release_user_data;
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

pub type CustomMvtVectorTileCallbackFn =
    unsafe extern "C" fn(*mut c_void, sys::mln_canonical_tile_id);

pub type CustomMvtVectorReleaseCallbackFn = unsafe extern "C" fn(*mut c_void);

#[derive(Debug, Clone, Copy)]
pub struct CustomMvtVectorSourceDescriptorFields {
    pub fetch_tile: Option<CustomMvtVectorTileCallbackFn>,
    pub cancel_tile: Option<CustomMvtVectorTileCallbackFn>,
    /// Invoked once when the C API stops referencing `user_data`.
    pub release_user_data: Option<CustomMvtVectorReleaseCallbackFn>,
    pub user_data: *mut c_void,
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
}

pub fn custom_mvt_vector_source_options_to_native(
    fields: CustomMvtVectorSourceDescriptorFields,
) -> sys::mln_custom_mvt_vector_source_options {
    // SAFETY: This C helper returns a plain value with no preconditions.
    let mut raw = unsafe { sys::mln_custom_mvt_vector_source_options_default() };
    raw.fetch_tile = fields.fetch_tile;
    raw.cancel_tile = fields.cancel_tile;
    raw.release_user_data = fields.release_user_data;
    raw.user_data = fields.user_data;
    if let Some(min_zoom) = fields.min_zoom {
        raw.fields |= sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM;
        raw.min_zoom = min_zoom;
    }
    if let Some(max_zoom) = fields.max_zoom {
        raw.fields |= sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
        raw.max_zoom = max_zoom;
    }
    raw
}

/// Copied fields from an inline TileJSON source description.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct TileJsonInfo {
    pub tiles: Vec<String>,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub scheme: TileScheme,
    pub bounds: Option<LatLngBounds>,
}

/// Copied retained metadata for one style source.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct SourceInfo {
    pub source_type: SourceType,
    pub raw_source_type: u32,
    pub is_volatile: bool,
    pub attribution: Option<String>,
    pub url: Option<String>,
    pub tile_json: Option<TileJsonInfo>,
    pub tile_size: Option<u32>,
    pub vector_encoding: Option<VectorTileEncoding>,
    pub raster_dem_encoding: Option<RasterDemEncoding>,
}

pub fn empty_style_source_info() -> sys::mln_style_source_info {
    sys::mln_style_source_info {
        size: std::mem::size_of::<sys::mln_style_source_info>() as u32,
        type_: sys::MLN_STYLE_SOURCE_TYPE_UNKNOWN,
        fields: 0,
        id_size: 0,
        is_volatile: false,
        has_attribution: false,
        attribution_size: 0,
        url_size: 0,
        tile_count: 0,
        min_zoom: 0.0,
        max_zoom: 0.0,
        scheme: sys::MLN_STYLE_TILE_SCHEME_XYZ,
        bounds: sys::mln_lat_lng_bounds {
            southwest: sys::mln_lat_lng {
                latitude: 0.0,
                longitude: 0.0,
            },
            northeast: sys::mln_lat_lng {
                latitude: 0.0,
                longitude: 0.0,
            },
        },
        tile_size: 0,
        vector_encoding: sys::MLN_STYLE_VECTOR_TILE_ENCODING_MVT,
        raster_encoding: sys::MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX,
    }
}

pub fn style_source_info_from_native(
    info: &sys::mln_style_source_info,
    attribution: Option<String>,
    url: Option<String>,
    tiles: Vec<String>,
) -> SourceInfo {
    let has = |field| info.fields & field != 0;
    SourceInfo {
        source_type: SourceType::from_raw(info.type_),
        raw_source_type: info.type_,
        is_volatile: info.is_volatile,
        attribution,
        url,
        tile_json: has(sys::MLN_STYLE_SOURCE_INFO_TILEJSON).then(|| TileJsonInfo {
            tiles,
            min_zoom: info.min_zoom,
            max_zoom: info.max_zoom,
            scheme: TileScheme::from_raw(info.scheme),
            bounds: has(sys::MLN_STYLE_SOURCE_INFO_BOUNDS)
                .then(|| lat_lng_bounds_from_native(info.bounds)),
        }),
        tile_size: has(sys::MLN_STYLE_SOURCE_INFO_TILE_SIZE).then_some(info.tile_size),
        vector_encoding: has(sys::MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING)
            .then(|| VectorTileEncoding::from_raw(info.vector_encoding)),
        raster_dem_encoding: has(sys::MLN_STYLE_SOURCE_INFO_RASTER_ENCODING)
            .then(|| RasterDemEncoding::from_raw(info.raster_encoding)),
    }
}

/// Copied fixed metadata for one style layer.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct StyleLayerInfo {
    /// Style-spec layer type string, such as `fill` or `background`.
    pub layer_type: String,
    /// Lowest zoom at which the layer draws; `f64::NEG_INFINITY` with no lower
    /// bound.
    pub min_zoom: f64,
    /// Highest zoom at which the layer draws; `f64::INFINITY` with no upper
    /// bound.
    pub max_zoom: f64,
    pub visibility: StyleLayerVisibility,
    /// Source ID; `None` for layer types that take no source.
    pub source_id: Option<String>,
    /// Source-layer ID; `None` when the layer carries none.
    pub source_layer: Option<String>,
}

/// Converts a filled native layer-info struct plus separately copied strings.
///
/// # Safety
///
/// `info.type_` must view readable bytes for this call.
pub unsafe fn style_layer_info_from_native(
    info: &sys::mln_style_layer_info,
    source_id: Option<String>,
    source_layer: Option<String>,
) -> crate::Result<StyleLayerInfo> {
    // SAFETY: The caller promises the type view is readable.
    let layer_type = unsafe { crate::string::copy_string_view(info.type_) }?;
    Ok(StyleLayerInfo {
        layer_type,
        min_zoom: info.min_zoom,
        max_zoom: info.max_zoom,
        visibility: StyleLayerVisibility::from_raw(info.visibility),
        source_id,
        source_layer,
    })
}

/// Copied runtime style image pixels with style-specific metadata.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct StyleImage {
    pub image: PremultipliedRgba8Image,
    pub pixel_ratio: f32,
    pub sdf: bool,
    pub stretch_x: Vec<ImageStretch>,
    pub stretch_y: Vec<ImageStretch>,
    pub content: Option<ImageContent>,
    pub text_fit_width: Option<StyleImageTextFit>,
    pub text_fit_height: Option<StyleImageTextFit>,
}

impl StyleImage {
    pub fn new(image: PremultipliedRgba8Image, pixel_ratio: f32, sdf: bool) -> Self {
        Self {
            image,
            pixel_ratio,
            sdf,
            stretch_x: Vec::new(),
            stretch_y: Vec::new(),
            content: None,
            text_fit_width: None,
            text_fit_height: None,
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
    /// Horizontally stretchable intervals, in image pixels.
    pub stretch_x: Option<Vec<ImageStretch>>,
    /// Vertically stretchable intervals, in image pixels.
    pub stretch_y: Option<Vec<ImageStretch>>,
    /// Content box used when `icon-text-fit` applies.
    pub content: Option<ImageContent>,
    pub text_fit_width: Option<StyleImageTextFit>,
    pub text_fit_height: Option<StyleImageTextFit>,
}

/// One stretchable interval along an image axis, in image pixels.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct ImageStretch {
    pub from: f32,
    pub to: f32,
}

impl ImageStretch {
    pub fn new(from: f32, to: f32) -> Self {
        Self { from, to }
    }
}

/// Content-box insets in image pixels, measured from the image's top-left.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct ImageContent {
    pub left: f32,
    pub top: f32,
    pub right: f32,
    pub bottom: f32,
}

/// Native style image options holding the stretch arrays the C API borrows.
pub struct NativeStyleImageOptions {
    raw: sys::mln_style_image_options,
    _stretch_x: Vec<sys::mln_image_stretch>,
    _stretch_y: Vec<sys::mln_image_stretch>,
}

impl NativeStyleImageOptions {
    fn new(options: &StyleImageOptions) -> Self {
        // SAFETY: This C helper returns a plain value with no preconditions.
        let mut raw = unsafe { sys::mln_style_image_options_default() };
        if let Some(value) = options.pixel_ratio {
            raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
            raw.pixel_ratio = value;
        }
        if let Some(value) = options.sdf {
            raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_SDF;
            raw.sdf = value;
        }
        let stretch_x = to_native_stretches(options.stretch_x.as_deref());
        if options.stretch_x.is_some() {
            raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_STRETCH_X;
            raw.stretch_x = stretch_x.as_ptr();
            raw.stretch_x_count = stretch_x.len();
        }
        let stretch_y = to_native_stretches(options.stretch_y.as_deref());
        if options.stretch_y.is_some() {
            raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
            raw.stretch_y = stretch_y.as_ptr();
            raw.stretch_y_count = stretch_y.len();
        }
        if let Some(value) = options.content {
            raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_CONTENT;
            raw.content = sys::mln_image_content {
                left: value.left,
                top: value.top,
                right: value.right,
                bottom: value.bottom,
            };
        }
        if let Some(value) = options.text_fit_width {
            raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH;
            raw.text_fit_width = value.raw_value();
        }
        if let Some(value) = options.text_fit_height {
            raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
            raw.text_fit_height = value.raw_value();
        }
        Self {
            raw,
            _stretch_x: stretch_x,
            _stretch_y: stretch_y,
        }
    }

    pub fn as_ptr(&self) -> *const sys::mln_style_image_options {
        ptr::from_ref(&self.raw)
    }
}

impl AsRef<sys::mln_style_image_options> for NativeStyleImageOptions {
    fn as_ref(&self) -> &sys::mln_style_image_options {
        &self.raw
    }
}

fn to_native_stretches(stretches: Option<&[ImageStretch]>) -> Vec<sys::mln_image_stretch> {
    stretches
        .unwrap_or_default()
        .iter()
        .map(|stretch| sys::mln_image_stretch {
            from: stretch.from,
            to: stretch.to,
        })
        .collect()
}

pub fn style_image_options_to_native(options: &StyleImageOptions) -> NativeStyleImageOptions {
    NativeStyleImageOptions::new(options)
}

/// The style's global transition options, controlling how paint property
/// changes animate and whether symbol placement changes cross-fade.
#[derive(Debug, Clone, Copy, Default, PartialEq)]
#[non_exhaustive]
pub struct StyleTransitionOptions {
    /// Transition duration in milliseconds. `None` falls back to the duration
    /// the style declares for each transitioning property.
    pub duration_ms: Option<f64>,
    /// Transition delay in milliseconds. `None` falls back to the delay the
    /// style declares for each transitioning property.
    pub delay_ms: Option<f64>,
    /// Whether symbol placement changes cross-fade. `None` leaves the
    /// cross-fade on; clearing it applies placement changes to the next
    /// rendered frame. A read always reports a value.
    pub enable_placement_transitions: Option<bool>,
}

pub fn style_transition_options_to_native(
    options: &StyleTransitionOptions,
) -> sys::mln_style_transition_options {
    // SAFETY: This C helper returns a plain value with no preconditions.
    let mut raw = unsafe { sys::mln_style_transition_options_default() };
    if let Some(value) = options.enable_placement_transitions {
        raw.fields |= sys::MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
        raw.enable_placement_transitions = value;
    }
    if let Some(value) = options.duration_ms {
        raw.fields |= sys::MLN_STYLE_TRANSITION_OPTION_DURATION;
        raw.duration_ms = value;
    }
    if let Some(value) = options.delay_ms {
        raw.fields |= sys::MLN_STYLE_TRANSITION_OPTION_DELAY;
        raw.delay_ms = value;
    }
    raw
}

pub fn style_transition_options_from_native(
    raw: &sys::mln_style_transition_options,
) -> StyleTransitionOptions {
    StyleTransitionOptions {
        duration_ms: (raw.fields & sys::MLN_STYLE_TRANSITION_OPTION_DURATION != 0)
            .then_some(raw.duration_ms),
        delay_ms: (raw.fields & sys::MLN_STYLE_TRANSITION_OPTION_DELAY != 0)
            .then_some(raw.delay_ms),
        enable_placement_transitions: (raw.fields
            & sys::MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS
            != 0)
            .then_some(raw.enable_placement_transitions),
    }
}

pub fn empty_style_transition_options() -> sys::mln_style_transition_options {
    // SAFETY: This C helper returns a plain value with no preconditions.
    unsafe { sys::mln_style_transition_options_default() }
}

pub fn empty_style_image_info() -> sys::mln_style_image_info {
    sys::mln_style_image_info {
        size: std::mem::size_of::<sys::mln_style_image_info>() as u32,
        width: 0,
        height: 0,
        stride: 0,
        byte_length: 0,
        stretch_x_count: 0,
        stretch_y_count: 0,
        content: sys::mln_image_content {
            left: 0.0,
            top: 0.0,
            right: 0.0,
            bottom: 0.0,
        },
        text_fit_width: sys::MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK,
        text_fit_height: sys::MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK,
        pixel_ratio: 1.0,
        sdf: false,
        has_content: false,
        has_text_fit_width: false,
        has_text_fit_height: false,
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
pub trait GeoJsonSourceOptionsNativeExt {
    fn try_to_native(&self) -> crate::Result<NativeGeoJsonSourceOptions>;
}

impl GeoJsonSourceOptionsNativeExt for GeoJsonSourceOptions {
    fn try_to_native(&self) -> crate::Result<NativeGeoJsonSourceOptions> {
        geojson_source_options_to_native(self)
    }
}

#[doc(hidden)]
pub trait StyleImageOptionsNativeExt {
    fn to_native(&self) -> NativeStyleImageOptions;
}

impl StyleImageOptionsNativeExt for StyleImageOptions {
    fn to_native(&self) -> NativeStyleImageOptions {
        style_image_options_to_native(self)
    }
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
    fn geojson_source_options_materialize_masks_fields_and_cluster_properties() {
        let default_native =
            geojson_source_options_to_native(&GeoJsonSourceOptions::default()).unwrap();
        let default_raw = default_native.as_ref();
        assert_eq!(
            default_raw.size,
            std::mem::size_of::<sys::mln_geojson_source_options>() as u32
        );
        assert_eq!(default_raw.fields, 0);
        assert_eq!(default_raw.cluster_properties.size, 0);
        assert!(default_raw.cluster_properties.data.is_null());

        let options = GeoJsonSourceOptions {
            min_zoom: Some(0.0),
            max_zoom: Some(14.0),
            tolerance: Some(0.5),
            cluster_max_zoom: Some(12.0),
            cluster_properties: Some(br#"{"sum":"+"}"#.to_vec()),
            tile_size: Some(256),
            buffer: Some(64),
            cluster_radius: Some(60),
            cluster_min_points: Some(3),
            line_metrics: Some(false),
            cluster: Some(true),
            synchronous_tiling: Some(true),
        };

        let native = geojson_source_options_to_native(&options).unwrap();
        let raw = native.as_ref();

        assert_eq!(
            raw.fields,
            sys::MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM
                | sys::MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM
                | sys::MLN_GEOJSON_SOURCE_OPTION_TOLERANCE
                | sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM
                | sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES
                | sys::MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE
                | sys::MLN_GEOJSON_SOURCE_OPTION_BUFFER
                | sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS
                | sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
                | sys::MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS
                | sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER
                | sys::MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING
        );
        // A present zero stays distinguishable from an absent field.
        assert_eq!(raw.min_zoom, 0.0);
        assert_eq!(raw.max_zoom, 14.0);
        assert_eq!(raw.tolerance, 0.5);
        assert_eq!(raw.cluster_max_zoom, 12.0);
        assert_eq!(raw.tile_size, 256);
        assert_eq!(raw.buffer, 64);
        assert_eq!(raw.cluster_radius, 60);
        assert_eq!(raw.cluster_min_points, 3);
        assert!(!raw.line_metrics);
        assert!(raw.cluster);
        assert!(raw.synchronous_tiling);
        // SAFETY: native keeps the cluster-properties buffer alive for this scope.
        let copied = unsafe { crate::string::copy_string_view_bytes(raw.cluster_properties) }
            .expect("cluster properties should copy back");
        assert_eq!(copied, options.cluster_properties.unwrap());
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
        unsafe extern "C" fn release(_user_data: *mut c_void) {}

        let raw = custom_geometry_source_options_to_native(CustomGeometrySourceDescriptorFields {
            fetch_tile: Some(fetch),
            cancel_tile: Some(cancel),
            release_user_data: Some(release),
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
        assert_eq!(
            raw.release_user_data.map(|callback| callback as usize),
            Some(release as *const () as usize)
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
    fn custom_mvt_vector_source_options_materialize_masks_and_callbacks() {
        unsafe extern "C" fn fetch(_user_data: *mut c_void, _tile_id: sys::mln_canonical_tile_id) {}
        unsafe extern "C" fn cancel(_user_data: *mut c_void, _tile_id: sys::mln_canonical_tile_id) {
        }
        unsafe extern "C" fn release(_user_data: *mut c_void) {}

        let raw =
            custom_mvt_vector_source_options_to_native(CustomMvtVectorSourceDescriptorFields {
                fetch_tile: Some(fetch),
                cancel_tile: Some(cancel),
                release_user_data: Some(release),
                user_data: 0x1234usize as *mut c_void,
                min_zoom: Some(1.0),
                max_zoom: Some(14.0),
            });

        assert_eq!(
            raw.size,
            std::mem::size_of::<sys::mln_custom_mvt_vector_source_options>() as u32
        );
        assert_eq!(
            raw.fetch_tile.map(|callback| callback as usize),
            Some(fetch as *const () as usize)
        );
        assert_eq!(
            raw.cancel_tile.map(|callback| callback as usize),
            Some(cancel as *const () as usize)
        );
        assert_eq!(
            raw.release_user_data.map(|callback| callback as usize),
            Some(release as *const () as usize)
        );
        assert_eq!(raw.user_data, 0x1234usize as *mut c_void);
        assert_eq!(
            raw.fields,
            sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM
                | sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM
        );
        assert_eq!(raw.min_zoom, 1.0);
        assert_eq!(raw.max_zoom, 14.0);
    }

    #[test]
    fn style_source_info_copies_raw_fields_and_attribution() {
        let raw = sys::mln_style_source_info {
            type_: sys::MLN_STYLE_SOURCE_TYPE_VECTOR,
            fields: sys::MLN_STYLE_SOURCE_INFO_TILEJSON
                | sys::MLN_STYLE_SOURCE_INFO_BOUNDS
                | sys::MLN_STYLE_SOURCE_INFO_TILE_SIZE
                | sys::MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING,
            is_volatile: true,
            has_attribution: true,
            attribution_size: 11,
            min_zoom: 1.0,
            max_zoom: 10.0,
            scheme: sys::MLN_STYLE_TILE_SCHEME_TMS,
            bounds: lat_lng_bounds_to_native(LatLngBounds::new(
                LatLng::new(-5.0, -10.0),
                LatLng::new(15.0, 20.0),
            )),
            tile_size: 512,
            vector_encoding: sys::MLN_STYLE_VECTOR_TILE_ENCODING_MLT,
            ..empty_style_source_info()
        };

        let copied = style_source_info_from_native(
            &raw,
            Some("© MapLibre".to_string()),
            None,
            vec!["https://example.com/{z}/{x}/{y}.mlt".to_owned()],
        );

        assert_eq!(copied.source_type, SourceType::Vector);
        assert_eq!(copied.raw_source_type, sys::MLN_STYLE_SOURCE_TYPE_VECTOR);
        assert!(copied.is_volatile);
        assert_eq!(copied.attribution.as_deref(), Some("© MapLibre"));
        assert_eq!(copied.tile_size, Some(512));
        assert_eq!(copied.vector_encoding, Some(VectorTileEncoding::Mlt));
        let tile_json = copied.tile_json.unwrap();
        assert_eq!(tile_json.scheme, TileScheme::Tms);
        assert_eq!(tile_json.min_zoom, 1.0);
        assert_eq!(tile_json.tiles.len(), 1);
        assert_eq!(
            tile_json.bounds,
            Some(LatLngBounds::new(
                LatLng::new(-5.0, -10.0),
                LatLng::new(15.0, 20.0)
            ))
        );
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
            stretch_x_count: 0,
            stretch_y_count: 0,
            content: None,
            text_fit_width: None,
            text_fit_height: None,
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

        let default_native = style_image_options_to_native(&StyleImageOptions::default());
        let default_raw = default_native.as_ref();
        assert_eq!(
            default_raw.size,
            std::mem::size_of::<sys::mln_style_image_options>() as u32
        );
        assert_eq!(default_raw.fields, 0);
        assert_eq!(default_raw.pixel_ratio, 1.0);
        assert!(!default_raw.sdf);
        assert!(default_raw.stretch_x.is_null());
        assert_eq!(default_raw.stretch_x_count, 0);

        let native = style_image_options_to_native(&StyleImageOptions {
            pixel_ratio: Some(2.0),
            sdf: Some(true),
            stretch_x: Some(vec![ImageStretch::new(0.0, 1.0)]),
            stretch_y: Some(Vec::new()),
            content: Some(ImageContent {
                left: 0.5,
                top: 0.5,
                right: 1.5,
                bottom: 1.5,
            }),
            text_fit_width: Some(StyleImageTextFit::StretchOnly),
            text_fit_height: Some(StyleImageTextFit::Proportional),
        });
        let raw = native.as_ref();
        assert_eq!(
            raw.fields,
            sys::MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO
                | sys::MLN_STYLE_IMAGE_OPTION_SDF
                | sys::MLN_STYLE_IMAGE_OPTION_STRETCH_X
                | sys::MLN_STYLE_IMAGE_OPTION_STRETCH_Y
                | sys::MLN_STYLE_IMAGE_OPTION_CONTENT
                | sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH
                | sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT
        );
        assert_eq!(raw.pixel_ratio, 2.0);
        assert!(raw.sdf);
        // A present empty array stays distinguishable from an absent one.
        assert_eq!(raw.stretch_x_count, 1);
        assert_eq!(raw.stretch_y_count, 0);
        // SAFETY: native owns the stretch storage raw points at for this scope.
        assert_eq!(unsafe { (*raw.stretch_x).to }, 1.0);
        assert_eq!(raw.content.right, 1.5);
        assert_eq!(
            raw.text_fit_height,
            sys::MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL
        );
    }
}
