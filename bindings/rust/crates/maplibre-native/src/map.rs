use std::cell::{Cell, RefCell};
use std::collections::HashMap;
use std::fmt;
use std::mem;
use std::ptr;
use std::rc::Rc;

use maplibre_native_support as support;
use maplibre_native_sys as sys;

use crate::custom_geometry::{CanonicalTileId, CustomGeometrySourceState};
use crate::events::MapId;
use crate::handle::{ThreadAffineNativeHandle, closed_handle_error, out_handle};
use crate::render::{
    MetalBorrowedTextureDescriptor, MetalOwnedTextureDescriptor, MetalSurfaceDescriptor,
    OwnedTextureDescriptor, PremultipliedRgba8Image, RenderSessionHandle, TextureImageInfo,
    VulkanBorrowedTextureDescriptor, VulkanOwnedTextureDescriptor, VulkanSurfaceDescriptor,
};
use crate::runtime::{RuntimeHandle, RuntimeState};
use crate::{
    AnimationOptions, BoundOptions, CameraFitOptions, CameraOptions, CustomGeometrySourceOptions,
    Error, ErrorKind, FreeCameraOptions, GeoJson, Geometry, HandleOperationError, JsonValue,
    LatLng, LatLngBounds, MapDebugOptions, MapOptions, MapProjectionHandle, MapTileOptions,
    MapViewportOptions, ProjectionMode, Result, ScreenPoint,
};

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

    fn to_native(&self) -> NativeTileSourceOptions<'_> {
        let attribution = self
            .attribution
            .as_deref()
            .map(support::string::string_view);
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
        if let Some(value) = attribution.as_ref() {
            raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
            raw.attribution = value.raw();
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
        NativeTileSourceOptions { raw, attribution }
    }
}

struct NativeTileSourceOptions<'a> {
    raw: sys::mln_style_tile_source_options,
    #[allow(dead_code)]
    attribution: Option<support::string::StringView<'a>>,
}

impl NativeTileSourceOptions<'_> {
    fn as_ptr(&self) -> *const sys::mln_style_tile_source_options {
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
    fn from_native(raw: &sys::mln_style_image_info) -> Self {
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

fn premultiplied_rgba8_image_to_native(
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

#[derive(Debug)]
pub(crate) struct MapState {
    handle: ThreadAffineNativeHandle<sys::mln_map>,
    runtime: RefCell<Option<Rc<RuntimeState>>>,
    id: MapId,
    custom_geometry_sources: RefCell<HashMap<String, Box<CustomGeometrySourceState>>>,
    pending_custom_geometry_source_url_cleanup: Cell<bool>,
}

impl MapState {
    fn new(ptr: std::ptr::NonNull<sys::mln_map>, runtime: Rc<RuntimeState>, id: MapId) -> Self {
        // SAFETY: ptr came from successful mln_map_create and is paired with
        // the matching map destroy function.
        let handle =
            unsafe { ThreadAffineNativeHandle::from_raw(ptr, sys::mln_map_destroy, "mln_map") };
        Self {
            handle,
            runtime: RefCell::new(Some(runtime)),
            id,
            custom_geometry_sources: RefCell::new(HashMap::new()),
            pending_custom_geometry_source_url_cleanup: Cell::new(false),
        }
    }

    pub(crate) fn as_ptr(&self) -> Result<*mut sys::mln_map> {
        let ptr = self.handle.as_ptr();
        if ptr.is_null() {
            Err(closed_handle_error("MapHandle"))
        } else {
            Ok(ptr)
        }
    }

    fn is_closed(&self) -> bool {
        self.handle.is_closed()
    }

    fn close(&self) -> Result<()> {
        let ptr = self.handle.as_ptr();
        self.handle.close()?;
        if let Some(runtime) = self.runtime.borrow_mut().take() {
            runtime.unregister_map(ptr);
        }
        self.clear_custom_geometry_sources();
        Ok(())
    }

    pub(crate) fn clear_custom_geometry_sources(&self) {
        self.pending_custom_geometry_source_url_cleanup.set(false);
        self.custom_geometry_sources.borrow_mut().clear();
    }

    pub(crate) fn mark_custom_geometry_sources_pending_url_cleanup(&self) {
        self.pending_custom_geometry_source_url_cleanup.set(true);
    }

    pub(crate) fn finish_custom_geometry_sources_pending_url_cleanup(&self) {
        if self
            .pending_custom_geometry_source_url_cleanup
            .replace(false)
        {
            self.custom_geometry_sources.borrow_mut().clear();
        }
    }

    pub(crate) fn cancel_custom_geometry_sources_pending_url_cleanup(&self) {
        self.pending_custom_geometry_source_url_cleanup.set(false);
    }

    fn has_pending_custom_geometry_source_url_cleanup(&self) -> bool {
        self.pending_custom_geometry_source_url_cleanup.get()
    }
}

impl Drop for MapState {
    fn drop(&mut self) {
        if let Some(runtime) = self.runtime.borrow_mut().take() {
            runtime.unregister_map(self.handle.as_ptr());
        }
    }
}

/// Owner-thread map handle bound to a retained runtime.
pub struct MapHandle {
    pub(crate) inner: Rc<MapState>,
}

impl fmt::Debug for MapHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("MapHandle")
            .field("closed", &self.inner.is_closed())
            .finish()
    }
}

impl MapHandle {
    /// Creates a map with native default map options on the runtime owner thread.
    pub fn new(runtime: &RuntimeHandle) -> Result<Self> {
        Self::with_options(runtime, &MapOptions::default())
    }

    /// Creates a map with explicit map options on the runtime owner thread.
    pub fn with_options(runtime: &RuntimeHandle, options: &MapOptions) -> Result<Self> {
        let runtime_ptr = runtime.inner.as_ptr()?;
        let mut out = support::ptr::OutPtr::<sys::mln_map>::new();
        let raw_options = options.to_native();

        // SAFETY: runtime_ptr is a live runtime handle. raw_options is a
        // materialized map descriptor with size filled by the binding. out is a
        // valid null-initialized out-pointer owned by this call.
        support::check(unsafe {
            sys::mln_map_create(runtime_ptr, &raw_options, out.as_mut_ptr())
        })?;
        let ptr = out_handle(out, "mln_map")?;
        let id = runtime.inner.register_map(ptr.as_ptr());
        let state = Rc::new(MapState::new(ptr, Rc::clone(&runtime.inner), id));
        runtime
            .inner
            .register_map_state(ptr.as_ptr(), Rc::downgrade(&state));

        Ok(Self { inner: state })
    }

    /// Returns this map's runtime-local event source identity.
    pub fn id(&self) -> MapId {
        self.inner.id
    }

    #[cfg(test)]
    fn custom_geometry_source_count_for_testing(&self) -> usize {
        self.inner.custom_geometry_sources.borrow().len()
    }

    /// Explicitly destroys the map.
    ///
    /// Native destruction errors are returned. When destruction fails, the
    /// underlying native handle remains live in the shared state so future child
    /// handles can continue to retain and close the map safely.
    pub fn close(self) -> std::result::Result<(), HandleOperationError<Self>> {
        if self.inner.is_closed() {
            return Ok(());
        }
        if Rc::strong_count(&self.inner) > 1 {
            return Err(HandleOperationError::new(
                Error::new(
                    ErrorKind::InvalidState,
                    None,
                    "MapHandle cannot close while child handles are live",
                ),
                self,
            ));
        }
        self.inner
            .close()
            .map_err(|error| HandleOperationError::new(error, self))
    }

    /// Requests a repaint for a continuous map.
    pub fn request_repaint(&self) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is a live map handle owned by this wrapper.
        support::check(unsafe { sys::mln_map_request_repaint(map) })
    }

    /// Requests one still image for a static or tile map.
    pub fn request_still_image(&self) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is a live map handle owned by this wrapper.
        support::check(unsafe { sys::mln_map_request_still_image(map) })
    }

    /// Loads a style URL through MapLibre Native style APIs.
    ///
    /// Custom geometry source callback state from the previous style is kept
    /// until the replacement style has loaded and that style-loaded event is
    /// polled or discarded, because URL style replacement is asynchronous in
    /// the C API. New custom geometry sources can be added after the pending
    /// URL style load finishes or fails.
    pub fn set_style_url(&self, url: &str) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let url = support::string::c_string(url)?;
        // SAFETY: map is live and url is a NUL-terminated UTF-8 string valid
        // for the duration of this command. The C API copies/consumes it before
        // returning.
        support::check(unsafe { sys::mln_map_set_style_url(map, url.as_ptr()) })?;
        self.inner
            .mark_custom_geometry_sources_pending_url_cleanup();
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
    /// on map close/drop, successful inline JSON style replacement, or after an
    /// asynchronous URL style replacement reports `MapStyleLoaded` through
    /// runtime event polling or draining. Native may invoke callbacks from
    /// worker threads; callbacks should queue owner-thread work before calling
    /// map APIs. While a style URL load is pending, adding new custom geometry
    /// sources returns `ErrorKind::InvalidState`.
    pub fn add_custom_geometry_source(
        &self,
        source_id: &str,
        options: CustomGeometrySourceOptions,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        if self.inner.has_pending_custom_geometry_source_url_cleanup() {
            return Err(Error::new(
                ErrorKind::InvalidState,
                None,
                "custom geometry sources can be added after the pending style URL load finishes",
            ));
        }
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

    /// Applies MapLibre debug overlay mask bits.
    pub fn set_debug_options(&self, options: MapDebugOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live. The C API validates unknown mask bits.
        support::check(unsafe { sys::mln_map_set_debug_options(map, options.bits()) })
    }

    /// Reads MapLibre debug overlay mask bits.
    pub fn debug_options(&self) -> Result<MapDebugOptions> {
        let map = self.inner.as_ptr()?;
        let mut raw = 0;
        // SAFETY: map is live and out_options points to writable u32 storage.
        support::check(unsafe { sys::mln_map_get_debug_options(map, &mut raw) })?;
        Ok(MapDebugOptions::from_bits_retain(raw))
    }

    /// Enables or disables MapLibre's rendering stats overlay view.
    pub fn set_rendering_stats_view_enabled(&self, enabled: bool) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live and enabled is passed by value.
        support::check(unsafe { sys::mln_map_set_rendering_stats_view_enabled(map, enabled) })
    }

    /// Reads whether MapLibre's rendering stats overlay view is enabled.
    pub fn rendering_stats_view_enabled(&self) -> Result<bool> {
        let map = self.inner.as_ptr()?;
        let mut enabled = false;
        // SAFETY: map is live and out_enabled points to writable bool storage.
        support::check(unsafe {
            sys::mln_map_get_rendering_stats_view_enabled(map, &mut enabled)
        })?;
        Ok(enabled)
    }

    /// Reads whether MapLibre currently considers the map fully loaded.
    pub fn is_fully_loaded(&self) -> Result<bool> {
        let map = self.inner.as_ptr()?;
        let mut loaded = false;
        // SAFETY: map is live and out_loaded points to writable bool storage.
        support::check(unsafe { sys::mln_map_is_fully_loaded(map, &mut loaded) })?;
        Ok(loaded)
    }

    /// Dumps map debug logs through MapLibre Native logging.
    pub fn dump_debug_logs(&self) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live.
        support::check(unsafe { sys::mln_map_dump_debug_logs(map) })
    }

    /// Reads live viewport and render-transform controls.
    pub fn viewport_options(&self) -> Result<MapViewportOptions> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_map_viewport_options_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        support::check(unsafe { sys::mln_map_get_viewport_options(map, &mut raw) })?;
        Ok(MapViewportOptions::from_native(raw))
    }

    /// Applies selected live viewport and render-transform controls.
    pub fn set_viewport_options(&self, options: &MapViewportOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = options.to_native();
        // SAFETY: map is live and raw is a materialized descriptor valid for
        // the duration of this call.
        support::check(unsafe { sys::mln_map_set_viewport_options(map, &raw) })
    }

    /// Reads tile prefetch and LOD tuning controls.
    pub fn tile_options(&self) -> Result<MapTileOptions> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_map_tile_options_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        support::check(unsafe { sys::mln_map_get_tile_options(map, &mut raw) })?;
        Ok(MapTileOptions::from_native(raw))
    }

    /// Applies selected tile prefetch and LOD tuning controls.
    pub fn set_tile_options(&self, options: &MapTileOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = options.to_native();
        // SAFETY: map is live and raw is a materialized descriptor valid for
        // the duration of this call.
        support::check(unsafe { sys::mln_map_set_tile_options(map, &raw) })
    }

    /// Reads the current camera snapshot.
    pub fn camera(&self) -> Result<CameraOptions> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_camera_options_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        support::check(unsafe { sys::mln_map_get_camera(map, &mut raw) })?;
        Ok(CameraOptions::from_native(raw))
    }

    /// Applies a camera jump command.
    pub fn jump_to(&self, camera: &CameraOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = camera.to_native();
        // SAFETY: map is live and raw is a materialized descriptor valid for
        // the duration of this call.
        support::check(unsafe { sys::mln_map_jump_to(map, &raw) })
    }

    /// Applies a camera ease transition command.
    pub fn ease_to(
        &self,
        camera: &CameraOptions,
        animation: Option<&AnimationOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw_camera = camera.to_native();
        let raw_animation = animation.map(AnimationOptions::to_native);
        // SAFETY: map is live and descriptors are valid for this call. A null
        // animation pointer requests native defaults.
        support::check(unsafe {
            sys::mln_map_ease_to(map, &raw_camera, option_ptr(raw_animation.as_ref()))
        })
    }

    /// Applies a camera fly transition command.
    pub fn fly_to(
        &self,
        camera: &CameraOptions,
        animation: Option<&AnimationOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw_camera = camera.to_native();
        let raw_animation = animation.map(AnimationOptions::to_native);
        // SAFETY: map is live and descriptors are valid for this call. A null
        // animation pointer requests native defaults.
        support::check(unsafe {
            sys::mln_map_fly_to(map, &raw_camera, option_ptr(raw_animation.as_ref()))
        })
    }

    /// Applies a screen-space pan command.
    pub fn move_by(&self, delta_x: f64, delta_y: f64) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live. The C API validates numeric values.
        support::check(unsafe { sys::mln_map_move_by(map, delta_x, delta_y) })
    }

    /// Applies an animated screen-space pan command.
    pub fn move_by_animated(
        &self,
        delta_x: f64,
        delta_y: f64,
        animation: Option<&AnimationOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw_animation = animation.map(AnimationOptions::to_native);
        // SAFETY: map is live and the optional animation descriptor is valid
        // for this call. The C API validates numeric values.
        support::check(unsafe {
            sys::mln_map_move_by_animated(map, delta_x, delta_y, option_ptr(raw_animation.as_ref()))
        })
    }

    /// Applies a screen-space zoom command.
    pub fn scale_by(&self, scale: f64, anchor: Option<ScreenPoint>) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw_anchor = anchor.map(ScreenPoint::to_native);
        // SAFETY: map is live and the optional anchor pointer is valid for this
        // call. The C API validates numeric values.
        support::check(unsafe {
            sys::mln_map_scale_by(map, scale, option_ptr(raw_anchor.as_ref()))
        })
    }

    /// Applies an animated screen-space zoom command.
    pub fn scale_by_animated(
        &self,
        scale: f64,
        anchor: Option<ScreenPoint>,
        animation: Option<&AnimationOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw_anchor = anchor.map(ScreenPoint::to_native);
        let raw_animation = animation.map(AnimationOptions::to_native);
        // SAFETY: map is live and optional descriptors are valid for this call.
        // The C API validates numeric values.
        support::check(unsafe {
            sys::mln_map_scale_by_animated(
                map,
                scale,
                option_ptr(raw_anchor.as_ref()),
                option_ptr(raw_animation.as_ref()),
            )
        })
    }

    /// Applies a screen-space rotate command.
    pub fn rotate_by(&self, first: ScreenPoint, second: ScreenPoint) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live. Points are passed by value and validated by C.
        support::check(unsafe {
            sys::mln_map_rotate_by(map, first.to_native(), second.to_native())
        })
    }

    /// Applies an animated screen-space rotate command.
    pub fn rotate_by_animated(
        &self,
        first: ScreenPoint,
        second: ScreenPoint,
        animation: Option<&AnimationOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw_animation = animation.map(AnimationOptions::to_native);
        // SAFETY: map is live and optional animation descriptor is valid for
        // this call. Points are passed by value and validated by C.
        support::check(unsafe {
            sys::mln_map_rotate_by_animated(
                map,
                first.to_native(),
                second.to_native(),
                option_ptr(raw_animation.as_ref()),
            )
        })
    }

    /// Applies a pitch delta command.
    pub fn pitch_by(&self, pitch: f64) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live. The C API validates numeric values.
        support::check(unsafe { sys::mln_map_pitch_by(map, pitch) })
    }

    /// Applies an animated pitch delta command.
    pub fn pitch_by_animated(
        &self,
        pitch: f64,
        animation: Option<&AnimationOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw_animation = animation.map(AnimationOptions::to_native);
        // SAFETY: map is live and optional animation descriptor is valid for
        // this call. The C API validates numeric values.
        support::check(unsafe {
            sys::mln_map_pitch_by_animated(map, pitch, option_ptr(raw_animation.as_ref()))
        })
    }

    /// Cancels active camera transitions.
    pub fn cancel_transitions(&self) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live.
        support::check(unsafe { sys::mln_map_cancel_transitions(map) })
    }

    /// Computes a camera that fits geographic bounds in the current viewport.
    pub fn camera_for_lat_lng_bounds(
        &self,
        bounds: LatLngBounds,
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<CameraOptions> {
        let map = self.inner.as_ptr()?;
        let raw_fit = fit_options.map(CameraFitOptions::to_native);
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw_camera = unsafe { sys::mln_camera_options_default() };
        // SAFETY: map is live, bounds is passed by value, optional fit options
        // are valid for this call, and raw_camera is writable.
        support::check(unsafe {
            sys::mln_map_camera_for_lat_lng_bounds(
                map,
                bounds.to_native(),
                option_ptr(raw_fit.as_ref()),
                &mut raw_camera,
            )
        })?;
        Ok(CameraOptions::from_native(raw_camera))
    }

    /// Computes a camera that fits geographic coordinates in the current viewport.
    pub fn camera_for_lat_lngs(
        &self,
        coordinates: &[LatLng],
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<CameraOptions> {
        let map = self.inner.as_ptr()?;
        if coordinates.is_empty() {
            return Err(Error::invalid_argument(
                "camera_for_lat_lngs requires at least one coordinate",
            ));
        }
        let raw_coordinates = lat_lngs_to_native(coordinates);
        let raw_fit = fit_options.map(CameraFitOptions::to_native);
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw_camera = unsafe { sys::mln_camera_options_default() };
        // SAFETY: map is live, arrays are valid for coordinate_count non-empty
        // entries, optional fit options are valid, and raw_camera is writable.
        support::check(unsafe {
            sys::mln_map_camera_for_lat_lngs(
                map,
                const_ptr_or_null(&raw_coordinates),
                raw_coordinates.len(),
                option_ptr(raw_fit.as_ref()),
                &mut raw_camera,
            )
        })?;
        Ok(CameraOptions::from_native(raw_camera))
    }

    /// Computes a camera that fits a geometry in the current viewport.
    pub fn camera_for_geometry(
        &self,
        geometry: &Geometry,
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<CameraOptions> {
        let map = self.inner.as_ptr()?;
        let native_geometry = geometry.try_to_native()?;
        let raw_fit = fit_options.map(CameraFitOptions::to_native);
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw_camera = unsafe { sys::mln_camera_options_default() };
        // SAFETY: map is live, native_geometry owns backing storage for the
        // duration of this call, optional fit options are valid, and raw_camera
        // is writable.
        support::check(unsafe {
            sys::mln_map_camera_for_geometry(
                map,
                native_geometry.as_ptr(),
                option_ptr(raw_fit.as_ref()),
                &mut raw_camera,
            )
        })?;
        Ok(CameraOptions::from_native(raw_camera))
    }

    /// Computes wrapped geographic bounds for a camera in the current viewport.
    pub fn lat_lng_bounds_for_camera(&self, camera: &CameraOptions) -> Result<LatLngBounds> {
        let map = self.inner.as_ptr()?;
        let raw_camera = camera.to_native();
        let mut raw_bounds = empty_bounds();
        // SAFETY: map is live, raw_camera is a valid descriptor for this call,
        // and raw_bounds points to writable storage.
        support::check(unsafe {
            sys::mln_map_lat_lng_bounds_for_camera(map, &raw_camera, &mut raw_bounds)
        })?;
        Ok(LatLngBounds::from_native(raw_bounds))
    }

    /// Computes unwrapped geographic bounds for a camera in the current viewport.
    pub fn lat_lng_bounds_for_camera_unwrapped(
        &self,
        camera: &CameraOptions,
    ) -> Result<LatLngBounds> {
        let map = self.inner.as_ptr()?;
        let raw_camera = camera.to_native();
        let mut raw_bounds = empty_bounds();
        // SAFETY: map is live, raw_camera is a valid descriptor for this call,
        // and raw_bounds points to writable storage.
        support::check(unsafe {
            sys::mln_map_lat_lng_bounds_for_camera_unwrapped(map, &raw_camera, &mut raw_bounds)
        })?;
        Ok(LatLngBounds::from_native(raw_bounds))
    }

    /// Reads map camera constraint options.
    pub fn bounds(&self) -> Result<BoundOptions> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_bound_options_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        support::check(unsafe { sys::mln_map_get_bounds(map, &mut raw) })?;
        Ok(BoundOptions::from_native(raw))
    }

    /// Applies selected map camera constraint options.
    pub fn set_bounds(&self, options: &BoundOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = options.to_native();
        // SAFETY: map is live and raw is a valid descriptor for this call.
        support::check(unsafe { sys::mln_map_set_bounds(map, &raw) })
    }

    /// Reads the current free camera position and orientation.
    pub fn free_camera_options(&self) -> Result<FreeCameraOptions> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_free_camera_options_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        support::check(unsafe { sys::mln_map_get_free_camera_options(map, &mut raw) })?;
        Ok(FreeCameraOptions::from_native(raw))
    }

    /// Applies selected free camera position and orientation fields.
    pub fn set_free_camera_options(&self, options: &FreeCameraOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = options.to_native();
        // SAFETY: map is live and raw is a valid descriptor for this call.
        support::check(unsafe { sys::mln_map_set_free_camera_options(map, &raw) })
    }

    /// Reads current axonometric rendering options.
    pub fn projection_mode(&self) -> Result<ProjectionMode> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_projection_mode_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        support::check(unsafe { sys::mln_map_get_projection_mode(map, &mut raw) })?;
        Ok(ProjectionMode::from_native(raw))
    }

    /// Applies selected axonometric rendering option fields.
    pub fn set_projection_mode(&self, mode: &ProjectionMode) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = mode.to_native();
        // SAFETY: map is live and raw is a valid descriptor for this call.
        support::check(unsafe { sys::mln_map_set_projection_mode(map, &raw) })
    }

    /// Converts a geographic world coordinate to a screen point for the current map.
    pub fn pixel_for_lat_lng(&self, coordinate: LatLng) -> Result<ScreenPoint> {
        let map = self.inner.as_ptr()?;
        let mut raw_point = empty_screen_point();
        // SAFETY: map is live, coordinate is passed by value, and raw_point is
        // writable storage for the output.
        support::check(unsafe {
            sys::mln_map_pixel_for_lat_lng(map, coordinate.to_native(), &mut raw_point)
        })?;
        Ok(ScreenPoint::from_native(raw_point))
    }

    /// Converts a screen point to a geographic world coordinate for the current map.
    pub fn lat_lng_for_pixel(&self, point: ScreenPoint) -> Result<LatLng> {
        let map = self.inner.as_ptr()?;
        let mut raw_coordinate = empty_lat_lng();
        // SAFETY: map is live, point is passed by value, and raw_coordinate is
        // writable storage for the output.
        support::check(unsafe {
            sys::mln_map_lat_lng_for_pixel(map, point.to_native(), &mut raw_coordinate)
        })?;
        Ok(LatLng::from_native(raw_coordinate))
    }

    /// Converts geographic world coordinates to screen points for the current map.
    pub fn pixels_for_lat_lngs(&self, coordinates: &[LatLng]) -> Result<Vec<ScreenPoint>> {
        let map = self.inner.as_ptr()?;
        let raw_coordinates = lat_lngs_to_native(coordinates);
        let mut raw_points = vec![empty_screen_point(); coordinates.len()];
        // SAFETY: map is live. Input and output arrays are valid for len
        // entries, or null when len is 0.
        support::check(unsafe {
            sys::mln_map_pixels_for_lat_lngs(
                map,
                const_ptr_or_null(&raw_coordinates),
                raw_coordinates.len(),
                mut_ptr_or_null(&mut raw_points),
            )
        })?;
        Ok(raw_points
            .into_iter()
            .map(ScreenPoint::from_native)
            .collect())
    }

    /// Converts screen points to geographic world coordinates for the current map.
    pub fn lat_lngs_for_pixels(&self, points: &[ScreenPoint]) -> Result<Vec<LatLng>> {
        let map = self.inner.as_ptr()?;
        let raw_points = screen_points_to_native(points);
        let mut raw_coordinates = vec![empty_lat_lng(); points.len()];
        // SAFETY: map is live. Input and output arrays are valid for len
        // entries, or null when len is 0.
        support::check(unsafe {
            sys::mln_map_lat_lngs_for_pixels(
                map,
                const_ptr_or_null(&raw_points),
                raw_points.len(),
                mut_ptr_or_null(&mut raw_coordinates),
            )
        })?;
        Ok(raw_coordinates
            .into_iter()
            .map(LatLng::from_native)
            .collect())
    }

    /// Creates a standalone projection snapshot from the current map transform.
    pub fn create_projection(&self) -> Result<MapProjectionHandle> {
        MapProjectionHandle::new(self)
    }

    /// Attaches a session-owned offscreen texture render target to this map.
    ///
    /// The session owns the backend texture target. The returned render session
    /// retains the map and remains owner-thread affine.
    pub fn attach_owned_texture(
        &self,
        descriptor: &OwnedTextureDescriptor,
    ) -> Result<RenderSessionHandle> {
        let raw = descriptor.to_native();
        RenderSessionHandle::attach(self, |map, out| {
            // SAFETY: map is live, raw is a materialized descriptor valid for
            // this call, and out is a null-initialized out-pointer.
            unsafe { sys::mln_owned_texture_attach(map, &raw, out) }
        })
    }

    /// Attaches a Metal native surface render target to this map.
    ///
    /// The layer and optional device pointers are backend-native handles. They
    /// must name valid Metal objects for this session and remain usable on the
    /// owner thread until the session is detached or closed.
    pub fn attach_metal_surface(
        &self,
        descriptor: &MetalSurfaceDescriptor,
    ) -> Result<RenderSessionHandle> {
        let raw = descriptor.to_native();
        RenderSessionHandle::attach(self, |map, out| {
            // SAFETY: map is live, raw is a materialized descriptor valid for
            // this call, and out is a null-initialized out-pointer.
            unsafe { sys::mln_metal_surface_attach(map, &raw, out) }
        })
    }

    /// Attaches a Vulkan native surface render target to this map.
    ///
    /// Vulkan handles are borrowed. They must remain valid and externally
    /// synchronized until the session is detached or closed.
    pub fn attach_vulkan_surface(
        &self,
        descriptor: &VulkanSurfaceDescriptor,
    ) -> Result<RenderSessionHandle> {
        let raw = descriptor.to_native();
        RenderSessionHandle::attach(self, |map, out| {
            // SAFETY: map is live, raw is a materialized descriptor valid for
            // this call, and out is a null-initialized out-pointer.
            unsafe { sys::mln_vulkan_surface_attach(map, &raw, out) }
        })
    }

    /// Attaches a Metal session-owned texture render target to this map.
    ///
    /// The device pointer must name a valid Metal device that remains usable on
    /// the owner thread until the session is detached or closed.
    pub fn attach_metal_owned_texture(
        &self,
        descriptor: &MetalOwnedTextureDescriptor,
    ) -> Result<RenderSessionHandle> {
        let raw = descriptor.to_native();
        RenderSessionHandle::attach(self, |map, out| {
            // SAFETY: map is live, raw is a materialized descriptor valid for
            // this call, and out is a null-initialized out-pointer.
            unsafe { sys::mln_metal_owned_texture_attach(map, &raw, out) }
        })
    }

    /// Attaches a Metal caller-owned texture render target to this map.
    ///
    /// The texture pointer is borrowed. The caller owns the texture, keeps it
    /// valid until detach or close, and synchronizes use outside this session.
    pub fn attach_metal_borrowed_texture(
        &self,
        descriptor: &MetalBorrowedTextureDescriptor,
    ) -> Result<RenderSessionHandle> {
        let raw = descriptor.to_native();
        RenderSessionHandle::attach(self, |map, out| {
            // SAFETY: map is live, raw is a materialized descriptor valid for
            // this call, and out is a null-initialized out-pointer.
            unsafe { sys::mln_metal_borrowed_texture_attach(map, &raw, out) }
        })
    }

    /// Attaches a Vulkan session-owned texture render target to this map.
    ///
    /// Vulkan device and queue handles are borrowed. They must remain valid and
    /// externally synchronized until the session is detached or closed.
    pub fn attach_vulkan_owned_texture(
        &self,
        descriptor: &VulkanOwnedTextureDescriptor,
    ) -> Result<RenderSessionHandle> {
        let raw = descriptor.to_native();
        RenderSessionHandle::attach(self, |map, out| {
            // SAFETY: map is live, raw is a materialized descriptor valid for
            // this call, and out is a null-initialized out-pointer.
            unsafe { sys::mln_vulkan_owned_texture_attach(map, &raw, out) }
        })
    }

    /// Attaches a Vulkan caller-owned texture render target to this map.
    ///
    /// Vulkan handles, image, and image view are borrowed. The caller owns the
    /// image resources, keeps them valid until detach or close, and handles
    /// queue-family ownership and synchronization outside this session.
    pub fn attach_vulkan_borrowed_texture(
        &self,
        descriptor: &VulkanBorrowedTextureDescriptor,
    ) -> Result<RenderSessionHandle> {
        let raw = descriptor.to_native();
        RenderSessionHandle::attach(self, |map, out| {
            // SAFETY: map is live, raw is a materialized descriptor valid for
            // this call, and out is a null-initialized out-pointer.
            unsafe { sys::mln_vulkan_borrowed_texture_attach(map, &raw, out) }
        })
    }
}

pub(crate) fn empty_lat_lng() -> sys::mln_lat_lng {
    sys::mln_lat_lng {
        latitude: 0.0,
        longitude: 0.0,
    }
}

pub(crate) fn empty_screen_point() -> sys::mln_screen_point {
    sys::mln_screen_point { x: 0.0, y: 0.0 }
}

pub(crate) fn empty_bounds() -> sys::mln_lat_lng_bounds {
    sys::mln_lat_lng_bounds {
        southwest: empty_lat_lng(),
        northeast: empty_lat_lng(),
    }
}

pub(crate) fn option_ptr<T>(value: Option<&T>) -> *const T {
    value.map_or(ptr::null(), |value| value as *const T)
}

pub(crate) fn const_ptr_or_null<T>(values: &[T]) -> *const T {
    if values.is_empty() {
        ptr::null()
    } else {
        values.as_ptr()
    }
}

pub(crate) fn mut_ptr_or_null<T>(values: &mut [T]) -> *mut T {
    if values.is_empty() {
        ptr::null_mut()
    } else {
        values.as_mut_ptr()
    }
}

pub(crate) fn lat_lngs_to_native(coordinates: &[LatLng]) -> Vec<sys::mln_lat_lng> {
    coordinates.iter().copied().map(LatLng::to_native).collect()
}

pub(crate) fn screen_points_to_native(points: &[ScreenPoint]) -> Vec<sys::mln_screen_point> {
    points.iter().copied().map(ScreenPoint::to_native).collect()
}

pub(crate) fn json_snapshot(
    snapshot: Option<std::ptr::NonNull<sys::mln_json_snapshot>>,
) -> Result<Option<JsonValue>> {
    let Some(snapshot) = snapshot else {
        return Ok(None);
    };
    // SAFETY: snapshot is an owned JSON snapshot returned by the C API and is
    // destroyed by the guard after copying.
    let snapshot = unsafe { support::handle::json_snapshot(snapshot.as_ptr()) }?;
    let mut value = ptr::null();
    // SAFETY: snapshot is live and value points to writable storage. The
    // borrowed JSON value is copied before the guard drops.
    support::check(unsafe { sys::mln_json_snapshot_get(snapshot.as_ptr(), &mut value) })?;
    if value.is_null() {
        return Ok(None);
    }
    // SAFETY: value is borrowed from the live snapshot guard and copied before
    // the guard drops at the end of this function.
    unsafe { JsonValue::from_native(&*value) }.map(Some)
}

fn copy_style_id_list(list: &support::handle::StyleIdListGuard) -> Result<Vec<String>> {
    let mut count = 0;
    // SAFETY: list is a live style ID list guard and count points to writable storage.
    support::check(unsafe { sys::mln_style_id_list_count(list.as_ptr(), &mut count) })?;

    let mut ids = Vec::with_capacity(count);
    for index in 0..count {
        let mut view = sys::mln_string_view {
            data: ptr::null(),
            size: 0,
        };
        // SAFETY: list is live, index is less than count, and view points to
        // writable storage. The borrowed view is copied before the next loop
        // iteration and before the guard drops.
        support::check(unsafe { sys::mln_style_id_list_get(list.as_ptr(), index, &mut view) })?;
        // SAFETY: The C API returns a view into list-owned storage that remains
        // valid until the list guard drops at the end of this function.
        ids.push(unsafe { support::string::copy_string_view(view) }?);
    }
    Ok(ids)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::events::empty_runtime_event;
    use crate::{
        ConstrainMode, CustomGeometrySourceOptions, EdgeInsets, ErrorKind, Feature,
        FeatureIdentifier, JsonMember, MapMode, NorthOrientation, TextureImageInfo, TileLodMode,
        ViewportMode,
    };

    const VALID_STYLE_JSON: &str = r#"{"version":8,"sources":{},"layers":[]}"#;
    const STYLE_WITH_IDS_JSON: &str = r#"{"version":8,"sources":{"geo":{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}},"layers":[{"id":"background","type":"background"},{"id":"geo-fill","type":"fill","source":"geo"}]}"#;

    fn object_member<'a>(value: &'a JsonValue, key: &str) -> Option<&'a JsonValue> {
        let JsonValue::Object(members) = value else {
            return None;
        };
        members
            .iter()
            .find(|member| member.key == key)
            .map(|member| &member.value)
    }

    #[test]
    fn map_create_and_close() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn map_create_with_options_and_close() {
        let runtime = RuntimeHandle::new().unwrap();
        let options = MapOptions::new(320, 240, 2.0).with_mode(MapMode::Static);
        let map = MapHandle::with_options(&runtime, &options).unwrap();

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn map_close_consumes_handle_and_drop_stays_idempotent() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn map_retains_runtime_after_runtime_handle_is_dropped() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();

        drop(runtime);

        map.close().unwrap();
    }

    #[test]
    fn style_setters_accept_valid_input_and_reject_embedded_nul() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();

        map.set_style_json(VALID_STYLE_JSON).unwrap();
        let _ = map.style_source_ids().unwrap();
        let _ = map.style_layer_ids().unwrap();

        map.set_style_json(STYLE_WITH_IDS_JSON).unwrap();
        let source_ids = map.style_source_ids().unwrap();
        let layer_ids = map.style_layer_ids().unwrap();
        assert!(source_ids.iter().any(|id| id == "geo"));
        assert!(layer_ids.iter().any(|id| id == "background"));
        assert!(layer_ids.iter().any(|id| id == "geo-fill"));

        map.set_style_url("https://example.com/style.json").unwrap();

        let error = map
            .set_style_url("https://example.com/\0style.json")
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), None);
        assert!(error.diagnostic().contains("embedded NUL"));

        let error = map.set_style_json("{").unwrap_err();
        assert!(matches!(
            error.kind(),
            ErrorKind::InvalidArgument | ErrorKind::NativeError
        ));
        assert!(error.raw_status().is_some());
        assert!(!error.diagnostic().trim().is_empty());

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn source_type_preserves_raw_values() {
        assert_eq!(SourceType::Unknown.raw_value(), 0);
        assert_eq!(SourceType::from_raw(0), SourceType::Unknown);
        assert_eq!(
            SourceType::GeoJson.raw_value(),
            sys::MLN_STYLE_SOURCE_TYPE_GEOJSON
        );
        assert_eq!(SourceType::from_raw(999_101), SourceType::Other(999_101));
        assert_eq!(SourceType::Other(999_101).raw_value(), 999_101);
    }

    #[test]
    fn style_source_exists_and_remove_call_real_c_api() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        map.set_style_json(VALID_STYLE_JSON).unwrap();

        let source = JsonValue::Object(vec![
            JsonMember::new("type", JsonValue::String("geojson".to_owned())),
            JsonMember::new(
                "data",
                JsonValue::Object(vec![
                    JsonMember::new("type", JsonValue::String("FeatureCollection".to_owned())),
                    JsonMember::new("features", JsonValue::Array(Vec::new())),
                ]),
            ),
        ]);

        assert!(!map.style_source_exists("owned-source").unwrap());
        assert!(!map.remove_style_source("owned-source").unwrap());

        map.add_style_source_json("owned-source", &source).unwrap();
        assert!(map.style_source_exists("owned-source").unwrap());
        assert!(map.remove_style_source("owned-source").unwrap());
        assert!(!map.style_source_exists("owned-source").unwrap());
        assert!(!map.remove_style_source("owned-source").unwrap());

        let error = map.style_source_exists("").unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        let error = map.remove_style_source("").unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        map.close().unwrap();
        runtime.close().unwrap();
    }

    fn test_style_image(data: Vec<u8>) -> PremultipliedRgba8Image {
        PremultipliedRgba8Image {
            info: TextureImageInfo {
                width: 2,
                height: 2,
                stride: 8,
                byte_length: data.len(),
            },
            data,
        }
    }

    #[test]
    fn style_image_add_query_copy_and_remove_call_real_c_api() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        map.set_style_json(VALID_STYLE_JSON).unwrap();

        let plain = test_style_image(vec![
            255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255,
        ]);
        let sdf = test_style_image(vec![1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]);

        assert!(!map.style_image_exists("plain").unwrap());
        assert_eq!(map.style_image_info("plain").unwrap(), None);
        assert_eq!(
            map.copy_style_image_premultiplied_rgba8("plain").unwrap(),
            None
        );
        assert!(!map.remove_style_image("plain").unwrap());

        map.set_style_image("plain", &plain, None).unwrap();
        assert!(map.style_image_exists("plain").unwrap());
        let info = map.style_image_info("plain").unwrap().unwrap();
        assert_eq!(info.width, 2);
        assert_eq!(info.height, 2);
        assert_eq!(info.stride, 8);
        assert_eq!(info.byte_length, 16);
        assert_eq!(info.pixel_ratio, 1.0);
        assert!(!info.sdf);
        let copied = map
            .copy_style_image_premultiplied_rgba8("plain")
            .unwrap()
            .unwrap();
        assert_eq!(copied.image.info.width, info.width);
        assert_eq!(copied.image.info.height, info.height);
        assert_eq!(copied.image.info.stride, info.stride);
        assert_eq!(copied.image.info.byte_length, info.byte_length);
        assert_eq!(copied.pixel_ratio, info.pixel_ratio);
        assert_eq!(copied.sdf, info.sdf);
        assert_eq!(copied.image.data, plain.data);

        map.set_style_image(
            "sdf",
            &sdf,
            Some(
                &StyleImageOptions::new()
                    .with_pixel_ratio(2.0)
                    .with_sdf(true),
            ),
        )
        .unwrap();
        let info = map.style_image_info("sdf").unwrap().unwrap();
        assert_eq!(info.pixel_ratio, 2.0);
        assert!(info.sdf);
        let copied = map
            .copy_style_image_premultiplied_rgba8("sdf")
            .unwrap()
            .unwrap();
        assert_eq!(copied.pixel_ratio, 2.0);
        assert!(copied.sdf);
        assert_eq!(copied.image.data, sdf.data);

        let replacement = test_style_image(vec![16; 16]);
        map.set_style_image(
            "sdf",
            &replacement,
            Some(&StyleImageOptions::new().with_sdf(false)),
        )
        .unwrap();
        let info = map.style_image_info("sdf").unwrap().unwrap();
        assert_eq!(info.pixel_ratio, 1.0);
        assert!(!info.sdf);
        let copied = map
            .copy_style_image_premultiplied_rgba8("sdf")
            .unwrap()
            .unwrap();
        assert_eq!(copied.image.data, replacement.data);

        assert!(map.remove_style_image("plain").unwrap());
        assert!(!map.style_image_exists("plain").unwrap());
        assert!(!map.remove_style_image("plain").unwrap());

        let error = map.style_image_exists("").unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        let error = map.set_style_image("", &plain, None).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        let error = map.remove_style_image("").unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        let error = map.style_image_info("").unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        let error = map.copy_style_image_premultiplied_rgba8("").unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());
    }

    #[test]
    fn style_image_descriptor_materialization_rejects_invalid_images_and_options() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        map.set_style_json(VALID_STYLE_JSON).unwrap();

        let too_short = PremultipliedRgba8Image {
            info: TextureImageInfo {
                width: 2,
                height: 2,
                stride: 8,
                byte_length: 16,
            },
            data: vec![0; 15],
        };
        let error = map.set_style_image("bad", &too_short, None).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        let image = test_style_image(vec![0; 16]);
        let error = map
            .set_style_image(
                "bad-options",
                &image,
                Some(&StyleImageOptions::new().with_pixel_ratio(0.0)),
            )
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());
    }

    fn image_source_coordinates() -> [LatLng; 4] {
        [
            LatLng::new(0.0, 0.0),
            LatLng::new(0.0, 1.0),
            LatLng::new(1.0, 1.0),
            LatLng::new(1.0, 0.0),
        ]
    }

    #[test]
    fn image_source_url_add_get_and_update_coordinates_call_real_c_api() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        map.set_style_json(VALID_STYLE_JSON).unwrap();

        let coordinates = image_source_coordinates();
        assert_eq!(map.image_source_coordinates("missing").unwrap(), None);

        map.add_image_source_url("url-image", &coordinates, "https://example.com/image.png")
            .unwrap();
        assert!(map.style_source_exists("url-image").unwrap());
        assert_eq!(
            map.style_source_type("url-image").unwrap(),
            Some(SourceType::Image)
        );
        assert_eq!(
            map.image_source_coordinates("url-image").unwrap(),
            Some(coordinates)
        );

        let error = map
            .set_image_source_url("missing", "https://example.com/missing.png")
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        map.set_image_source_url("url-image", "https://example.com/replacement.png")
            .unwrap();

        let error = map.set_image_source_url("url-image", "").unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());
        let updated = [
            LatLng::new(2.0, 2.0),
            LatLng::new(2.0, 3.0),
            LatLng::new(3.0, 3.0),
            LatLng::new(3.0, 2.0),
        ];
        map.set_image_source_coordinates("url-image", &updated)
            .unwrap();
        assert_eq!(
            map.image_source_coordinates("url-image").unwrap(),
            Some(updated)
        );

        let error = map
            .add_image_source_url("", &coordinates, "https://example.com/a.png")
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        let error = map
            .add_image_source_url("bad-url", &coordinates, "")
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        let error = map
            .set_image_source_coordinates("missing", &coordinates)
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        let error = map.image_source_coordinates("").unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());
    }

    #[test]
    fn image_source_inline_image_add_and_update_call_real_c_api() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        map.set_style_json(VALID_STYLE_JSON).unwrap();

        let coordinates = image_source_coordinates();
        let image = test_style_image(vec![1; 16]);
        map.add_image_source_image("inline-image", &coordinates, &image)
            .unwrap();
        assert_eq!(
            map.style_source_type("inline-image").unwrap(),
            Some(SourceType::Image)
        );
        assert_eq!(
            map.image_source_coordinates("inline-image").unwrap(),
            Some(coordinates)
        );

        let replacement = test_style_image(vec![2; 16]);
        let error = map
            .set_image_source_image("missing", &replacement)
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        map.set_image_source_image("inline-image", &replacement)
            .unwrap();

        let too_short = PremultipliedRgba8Image {
            info: TextureImageInfo {
                width: 2,
                height: 2,
                stride: 8,
                byte_length: 16,
            },
            data: vec![0; 15],
        };
        let error = map
            .set_image_source_image("inline-image", &too_short)
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());
        map.set_image_source_url("inline-image", "https://example.com/after-inline.png")
            .unwrap();

        let error = map.set_image_source_image("", &replacement).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());
    }

    #[test]
    fn image_source_methods_reject_non_image_sources() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        map.set_style_json(VALID_STYLE_JSON).unwrap();

        let geojson_source = JsonValue::Object(vec![
            JsonMember::new("type", JsonValue::String("geojson".to_owned())),
            JsonMember::new(
                "data",
                JsonValue::Object(vec![
                    JsonMember::new("type", JsonValue::String("FeatureCollection".to_owned())),
                    JsonMember::new("features", JsonValue::Array(Vec::new())),
                ]),
            ),
        ]);
        map.add_style_source_json("geo", &geojson_source).unwrap();

        let error = map.image_source_coordinates("geo").unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        let error = map
            .set_image_source_url("geo", "https://example.com/not-image.png")
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        let image = test_style_image(vec![3; 16]);
        let error = map.set_image_source_image("geo", &image).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        let coordinates = image_source_coordinates();
        let error = map
            .set_image_source_coordinates("geo", &coordinates)
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());
    }

    #[test]
    fn tile_source_helpers_call_real_c_api() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        map.set_style_json(VALID_STYLE_JSON).unwrap();

        let vector_options = TileSourceOptions::new()
            .with_min_zoom(1.0)
            .with_max_zoom(12.0)
            .with_attribution("© vector")
            .with_scheme(TileScheme::Xyz)
            .with_bounds(LatLngBounds::new(
                LatLng::new(-10.0, -20.0),
                LatLng::new(10.0, 20.0),
            ))
            .with_vector_encoding(VectorTileEncoding::Mvt);
        map.add_vector_source_url(
            "vector-url",
            "https://example.com/vector.json",
            Some(&vector_options),
        )
        .unwrap();
        assert_eq!(
            map.style_source_type("vector-url").unwrap(),
            Some(SourceType::Vector)
        );

        map.add_vector_source_tiles(
            "vector-tiles",
            &["https://example.com/vector/{z}/{x}/{y}.pbf"],
            None,
        )
        .unwrap();
        assert_eq!(
            map.style_source_type("vector-tiles").unwrap(),
            Some(SourceType::Vector)
        );

        let raster_options = TileSourceOptions::new()
            .with_tile_size(256)
            .with_scheme(TileScheme::Tms)
            .with_attribution("© raster");
        map.add_raster_source_url(
            "raster-url",
            "https://example.com/raster.json",
            Some(&raster_options),
        )
        .unwrap();
        assert_eq!(
            map.style_source_type("raster-url").unwrap(),
            Some(SourceType::Raster)
        );

        map.add_raster_source_tiles(
            "raster-tiles",
            &["https://example.com/raster/{z}/{x}/{y}.png"],
            None,
        )
        .unwrap();
        assert_eq!(
            map.style_source_type("raster-tiles").unwrap(),
            Some(SourceType::Raster)
        );

        let dem_options = TileSourceOptions::new()
            .with_tile_size(512)
            .with_raster_dem_encoding(RasterDemEncoding::Terrarium);
        map.add_raster_dem_source_url(
            "dem-url",
            "https://example.com/dem.json",
            Some(&dem_options),
        )
        .unwrap();
        assert_eq!(
            map.style_source_type("dem-url").unwrap(),
            Some(SourceType::RasterDem)
        );

        map.add_raster_dem_source_tiles(
            "dem-tiles",
            &["https://example.com/dem/{z}/{x}/{y}.png"],
            Some(&dem_options),
        )
        .unwrap();
        assert_eq!(
            map.style_source_type("dem-tiles").unwrap(),
            Some(SourceType::RasterDem)
        );

        let error = map
            .add_vector_source_url("", "https://example.com/vector.json", None)
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);

        let error = map
            .add_raster_source_tiles("empty-tiles", &[] as &[&str], None)
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);

        let vector_only_options =
            TileSourceOptions::new().with_vector_encoding(VectorTileEncoding::Mvt);
        let error = map
            .add_raster_source_tiles(
                "raster-with-vector-option",
                &["https://example.com/raster/{z}/{x}/{y}.png"],
                Some(&vector_only_options),
            )
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        let dem_only_options =
            TileSourceOptions::new().with_raster_dem_encoding(RasterDemEncoding::Terrarium);
        let error = map
            .add_vector_source_tiles(
                "vector-with-dem-option",
                &["https://example.com/vector/{z}/{x}/{y}.pbf"],
                Some(&dem_only_options),
            )
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        let error = map
            .add_raster_source_tiles(
                "raster-with-dem-option",
                &["https://example.com/raster/{z}/{x}/{y}.png"],
                Some(&dem_only_options),
            )
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());
    }

    #[test]
    fn terrain_and_location_layer_helpers_call_real_c_api() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        map.set_style_json(VALID_STYLE_JSON).unwrap();

        map.add_raster_dem_source_tiles(
            "dem",
            &["https://example.com/dem/{z}/{x}/{y}.png"],
            Some(&TileSourceOptions::new().with_raster_dem_encoding(RasterDemEncoding::Mapbox)),
        )
        .unwrap();
        map.add_hillshade_layer("hillshade", "dem", None).unwrap();
        map.add_color_relief_layer("color-relief", "dem", None)
            .unwrap();

        map.add_location_indicator_layer("location", None).unwrap();
        map.set_location_indicator_location("location", LatLng::new(37.8, -122.4), 12.0)
            .unwrap();
        map.set_location_indicator_bearing("location", 45.0)
            .unwrap();
        map.set_location_indicator_accuracy_radius("location", 24.0)
            .unwrap();
        map.set_location_indicator_image_name(
            "location",
            LocationIndicatorImageKind::Top,
            "location-top",
        )
        .unwrap();

        assert_eq!(
            map.layer_property("location", "location").unwrap(),
            Some(JsonValue::Array(vec![
                JsonValue::Double(-122.4),
                JsonValue::Double(37.8),
                JsonValue::Double(12.0),
            ]))
        );
        assert_eq!(
            map.layer_property("location", "bearing").unwrap(),
            Some(JsonValue::Double(45.0))
        );
        assert_eq!(
            map.layer_property("location", "accuracy-radius").unwrap(),
            Some(JsonValue::Double(24.0))
        );
        assert_eq!(
            map.layer_property("location", "top-image").unwrap(),
            Some(JsonValue::Object(vec![
                JsonMember::new("available", JsonValue::Bool(false)),
                JsonMember::new("name", JsonValue::String("location-top".to_owned())),
            ]))
        );

        let error = map.add_hillshade_layer("", "dem", None).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);

        map.add_raster_source_tiles(
            "raster",
            &["https://example.com/raster/{z}/{x}/{y}.png"],
            None,
        )
        .unwrap();
        let error = map
            .add_hillshade_layer("wrong-source-type", "raster", None)
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);

        let error = map
            .set_location_indicator_image_name("location", LocationIndicatorImageKind::Bearing, "")
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    }

    #[test]
    fn style_source_type_and_info_call_real_c_api() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        map.set_style_json(VALID_STYLE_JSON).unwrap();

        let geojson_source = JsonValue::Object(vec![
            JsonMember::new("type", JsonValue::String("geojson".to_owned())),
            JsonMember::new(
                "data",
                JsonValue::Object(vec![
                    JsonMember::new("type", JsonValue::String("FeatureCollection".to_owned())),
                    JsonMember::new("features", JsonValue::Array(Vec::new())),
                ]),
            ),
        ]);
        let vector_source = JsonValue::Object(vec![
            JsonMember::new("type", JsonValue::String("vector".to_owned())),
            JsonMember::new(
                "tiles",
                JsonValue::Array(vec![JsonValue::String(
                    "https://example.com/{z}/{x}/{y}.pbf".to_owned(),
                )]),
            ),
            JsonMember::new(
                "attribution",
                JsonValue::String("Example attribution".to_owned()),
            ),
        ]);

        assert_eq!(map.style_source_type("missing-source").unwrap(), None);
        assert_eq!(map.style_source_info("missing-source").unwrap(), None);

        map.add_style_source_json("empty", &geojson_source).unwrap();
        assert_eq!(
            map.style_source_type("empty").unwrap(),
            Some(SourceType::GeoJson)
        );
        let info = map.style_source_info("empty").unwrap().unwrap();
        assert_eq!(info.source_type, SourceType::GeoJson);
        assert_eq!(info.raw_source_type, sys::MLN_STYLE_SOURCE_TYPE_GEOJSON);
        assert!(!info.is_volatile);
        assert_eq!(info.attribution, None);

        map.add_style_source_json("vector-meta", &vector_source)
            .unwrap();
        assert_eq!(
            map.style_source_type("vector-meta").unwrap(),
            Some(SourceType::Vector)
        );
        let info = map.style_source_info("vector-meta").unwrap().unwrap();
        assert_eq!(info.source_type, SourceType::Vector);
        assert_eq!(info.raw_source_type, sys::MLN_STYLE_SOURCE_TYPE_VECTOR);
        assert_eq!(info.attribution.as_deref(), Some("Example attribution"));

        let error = map.style_source_type("").unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        let error = map.style_source_info("").unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert!(error.raw_status().is_some());

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn custom_geometry_source_apis_call_real_c_api_and_style_replacement_releases_state() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        map.set_style_json(VALID_STYLE_JSON).unwrap();

        map.add_custom_geometry_source(
            "custom",
            CustomGeometrySourceOptions::new(|_| {})
                .with_cancel_tile(|_| {})
                .with_min_zoom(0.0)
                .with_max_zoom(2.0)
                .with_tolerance(0.375)
                .with_tile_size(512)
                .with_buffer(64)
                .with_clip(true)
                .with_wrap(false),
        )
        .unwrap();
        assert_eq!(map.custom_geometry_source_count_for_testing(), 1);

        let tile_id = CanonicalTileId::new(0, 0, 0);
        map.set_custom_geometry_source_tile_data(
            "custom",
            tile_id,
            &GeoJson::FeatureCollection(Vec::new()),
        )
        .unwrap();
        map.invalidate_custom_geometry_source_tile("custom", tile_id)
            .unwrap();
        map.invalidate_custom_geometry_source_region(
            "custom",
            LatLngBounds::new(LatLng::new(-1.0, -1.0), LatLng::new(1.0, 1.0)),
        )
        .unwrap();

        assert!(map.remove_style_source("custom").unwrap());
        assert_eq!(map.custom_geometry_source_count_for_testing(), 0);
        assert!(!map.style_source_exists("custom").unwrap());

        map.add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
            .unwrap();
        assert_eq!(map.custom_geometry_source_count_for_testing(), 1);

        map.set_style_json(VALID_STYLE_JSON).unwrap();
        assert_eq!(map.custom_geometry_source_count_for_testing(), 0);

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn custom_geometry_source_state_is_released_on_map_close() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        map.set_style_json(VALID_STYLE_JSON).unwrap();
        map.add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
            .unwrap();
        assert_eq!(map.custom_geometry_source_count_for_testing(), 1);

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn custom_geometry_source_state_ignores_stale_style_loaded_events() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        map.set_style_json(VALID_STYLE_JSON).unwrap();
        map.add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
            .unwrap();

        let mut event = empty_runtime_event();
        event.type_ = sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED;
        event.source_type = sys::MLN_RUNTIME_EVENT_SOURCE_MAP;
        event.source = map.inner.handle.as_ptr().cast();
        runtime.inner.apply_event_side_effects_for_testing(&event);

        assert_eq!(map.custom_geometry_source_count_for_testing(), 1);
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn custom_geometry_source_state_releases_on_pending_url_style_loaded_event() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        map.set_style_json(VALID_STYLE_JSON).unwrap();
        map.add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
            .unwrap();
        map.inner.mark_custom_geometry_sources_pending_url_cleanup();

        let mut event = empty_runtime_event();
        event.type_ = sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED;
        event.source_type = sys::MLN_RUNTIME_EVENT_SOURCE_MAP;
        event.source = map.inner.handle.as_ptr().cast();
        runtime.inner.apply_event_side_effects_for_testing(&event);

        assert_eq!(map.custom_geometry_source_count_for_testing(), 0);
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn custom_geometry_source_add_rejects_pending_url_style_replacement() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        map.set_style_json(VALID_STYLE_JSON).unwrap();
        map.inner.mark_custom_geometry_sources_pending_url_cleanup();

        let error = map
            .add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
            .unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn style_json_and_geojson_descriptors_call_real_c_api() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        map.set_style_json(VALID_STYLE_JSON).unwrap();

        let source = JsonValue::Object(vec![
            JsonMember::new("type", JsonValue::String("geojson".to_owned())),
            JsonMember::new(
                "data",
                JsonValue::Object(vec![
                    JsonMember::new("type", JsonValue::String("FeatureCollection".to_owned())),
                    JsonMember::new("features", JsonValue::Array(Vec::new())),
                ]),
            ),
        ]);
        map.add_style_source_json("owned-json-source", &source)
            .unwrap();

        let geojson = GeoJson::Feature(
            Feature::new(
                Geometry::Point(LatLng::new(1.0, 2.0)),
                vec![JsonMember::new("name", JsonValue::String("one".to_owned()))],
            )
            .with_identifier(FeatureIdentifier::String("feature-1".to_owned())),
        );
        map.add_geojson_source_data("owned-geojson-source", &geojson)
            .unwrap();
        map.set_geojson_source_data(
            "owned-geojson-source",
            &GeoJson::FeatureCollection(Vec::new()),
        )
        .unwrap();

        let layer = JsonValue::Object(vec![
            JsonMember::new("id", JsonValue::String("owned-background".to_owned())),
            JsonMember::new("type", JsonValue::String("background".to_owned())),
            JsonMember::new(
                "paint",
                JsonValue::Object(vec![JsonMember::new(
                    "background-opacity",
                    JsonValue::Double(0.5),
                )]),
            ),
        ]);
        map.add_style_layer_json(&layer, None).unwrap();
        let copied_layer = map
            .style_layer_json("owned-background")
            .unwrap()
            .expect("added layer should have a JSON snapshot");
        assert_eq!(
            object_member(&copied_layer, "id"),
            Some(&JsonValue::String("owned-background".to_owned()))
        );
        assert_eq!(
            object_member(&copied_layer, "type"),
            Some(&JsonValue::String("background".to_owned()))
        );
        let paint = object_member(&copied_layer, "paint").expect("layer paint should be copied");
        assert_eq!(
            object_member(paint, "background-opacity"),
            Some(&JsonValue::Double(0.5))
        );
        map.set_layer_property(
            "owned-background",
            "background-opacity",
            &JsonValue::Double(0.75),
        )
        .unwrap();
        assert_eq!(
            map.layer_property("owned-background", "background-opacity")
                .unwrap(),
            Some(JsonValue::Double(0.75))
        );

        let error = map
            .set_layer_filter("owned-background", Some(&JsonValue::Double(f64::NAN)))
            .err()
            .unwrap();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), None);

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn camera_jump_and_coordinate_conversions_round_trip() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = RuntimeHandle::create_map_with_options(
            &runtime,
            &MapOptions::new(512, 512, 1.0).with_mode(MapMode::Continuous),
        )
        .unwrap();
        let center = LatLng::new(45.0, -122.0);

        map.jump_to(&CameraOptions::new().with_center(center).with_zoom(4.0))
            .unwrap();
        let camera = map.camera().unwrap();
        assert_eq!(camera.center, Some(center));
        assert_eq!(camera.zoom, Some(4.0));

        let point = map.pixel_for_lat_lng(center).unwrap();
        let round_tripped = map.lat_lng_for_pixel(point).unwrap();
        assert!((round_tripped.latitude - center.latitude).abs() < 1e-7);
        assert!((round_tripped.longitude - center.longitude).abs() < 1e-7);

        let points = map.pixels_for_lat_lngs(&[center]).unwrap();
        let coordinates = map.lat_lngs_for_pixels(&points).unwrap();
        assert_eq!(points.len(), 1);
        assert!((coordinates[0].latitude - center.latitude).abs() < 1e-7);
        assert!((coordinates[0].longitude - center.longitude).abs() < 1e-7);

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn camera_commands_and_queries_use_real_c_api() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        let camera = CameraOptions::new()
            .with_center(LatLng::new(0.0, 0.0))
            .with_zoom(1.0);
        let animation = AnimationOptions::new().with_duration_ms(0.0);

        map.ease_to(&camera, Some(&animation)).unwrap();
        map.fly_to(&camera, Some(&animation)).unwrap();
        map.move_by(0.0, 0.0).unwrap();
        map.move_by_animated(0.0, 0.0, Some(&animation)).unwrap();
        map.scale_by(1.0, Some(ScreenPoint::new(128.0, 128.0)))
            .unwrap();
        map.scale_by_animated(1.0, None, Some(&animation)).unwrap();
        map.rotate_by(ScreenPoint::new(0.0, 0.0), ScreenPoint::new(0.0, 0.0))
            .unwrap();
        map.rotate_by_animated(
            ScreenPoint::new(0.0, 0.0),
            ScreenPoint::new(0.0, 0.0),
            Some(&animation),
        )
        .unwrap();
        map.pitch_by(0.0).unwrap();
        map.pitch_by_animated(0.0, Some(&animation)).unwrap();
        map.cancel_transitions().unwrap();

        let bounds = LatLngBounds::new(LatLng::new(-1.0, -1.0), LatLng::new(1.0, 1.0));
        let fit = CameraFitOptions::new().with_padding(EdgeInsets::new(1.0, 1.0, 1.0, 1.0));
        map.camera_for_lat_lng_bounds(bounds, Some(&fit)).unwrap();
        map.camera_for_lat_lngs(&[LatLng::new(0.0, 0.0), LatLng::new(1.0, 1.0)], Some(&fit))
            .unwrap();
        let error = map.camera_for_lat_lngs(&[], Some(&fit)).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), None);
        assert!(error.diagnostic().contains("at least one coordinate"));
        map.camera_for_geometry(
            &Geometry::LineString(vec![LatLng::new(0.0, 0.0), LatLng::new(1.0, 1.0)]),
            Some(&fit),
        )
        .unwrap();
        map.lat_lng_bounds_for_camera(&camera).unwrap();
        map.lat_lng_bounds_for_camera_unwrapped(&camera).unwrap();

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn map_state_viewport_tile_debug_and_projection_mode_round_trip() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();

        map.set_debug_options(MapDebugOptions::TILE_BORDERS | MapDebugOptions::PARSE_STATUS)
            .unwrap();
        let debug = map.debug_options().unwrap();
        assert!(debug.contains(MapDebugOptions::TILE_BORDERS));
        assert!(debug.contains(MapDebugOptions::PARSE_STATUS));

        map.set_rendering_stats_view_enabled(true).unwrap();
        assert!(map.rendering_stats_view_enabled().unwrap());
        assert!(!map.is_fully_loaded().unwrap());
        map.dump_debug_logs().unwrap();

        let viewport = MapViewportOptions::new()
            .with_north_orientation(NorthOrientation::Up)
            .with_constrain_mode(ConstrainMode::HeightOnly)
            .with_viewport_mode(ViewportMode::Default)
            .with_frustum_offset(EdgeInsets::new(0.0, 0.0, 0.0, 0.0));
        map.set_viewport_options(&viewport).unwrap();
        let copied_viewport = map.viewport_options().unwrap();
        assert_eq!(
            copied_viewport.north_orientation,
            Some(NorthOrientation::Up)
        );
        assert_eq!(
            copied_viewport.constrain_mode,
            Some(ConstrainMode::HeightOnly)
        );
        assert_eq!(copied_viewport.viewport_mode, Some(ViewportMode::Default));

        let tile = MapTileOptions::new()
            .with_prefetch_zoom_delta(1)
            .with_lod_mode(TileLodMode::Default);
        map.set_tile_options(&tile).unwrap();
        let copied_tile = map.tile_options().unwrap();
        assert_eq!(copied_tile.prefetch_zoom_delta, Some(1));
        assert_eq!(copied_tile.lod_mode, Some(TileLodMode::Default));

        let projection_mode = ProjectionMode::new()
            .with_axonometric(false)
            .with_x_skew(0.0)
            .with_y_skew(0.0);
        map.set_projection_mode(&projection_mode).unwrap();
        let copied_projection_mode = map.projection_mode().unwrap();
        assert_eq!(copied_projection_mode.axonometric, Some(false));

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn bounds_and_free_camera_operations_call_c_api() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();

        let bounds = BoundOptions::new()
            .with_bounds(LatLngBounds::new(
                LatLng::new(-10.0, -20.0),
                LatLng::new(10.0, 20.0),
            ))
            .with_min_zoom(0.0)
            .with_max_zoom(20.0)
            .with_min_pitch(0.0)
            .with_max_pitch(60.0);
        map.set_bounds(&bounds).unwrap();
        let copied_bounds = map.bounds().unwrap();
        assert_eq!(copied_bounds.min_zoom, Some(0.0));
        assert_eq!(copied_bounds.max_zoom, Some(20.0));

        let free = map.free_camera_options().unwrap();
        map.set_free_camera_options(&free).unwrap();

        map.close().unwrap();
        runtime.close().unwrap();
    }
}
