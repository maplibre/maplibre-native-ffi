use std::ptr;
use std::time::{Duration, Instant};

pub(crate) use maplibre_core::style::{
    GeoJsonSourceOptionsNativeExt, NativeGeoJsonSourceOptions, NativeStyleImageOptions,
    NativeTileSourceOptions, NativeTileUrls, StyleImageOptionsNativeExt,
    TileSourceOptionsNativeExt,
};
pub use maplibre_core::{
    GeoJsonSourceOptions, ImageContent, ImageStretch, LocationIndicatorImageKind, SourceInfo,
    SourceType, StyleImage, StyleImageInfo, StyleImageOptions, StyleImageTextFit, StyleLayerInfo,
    StyleLayerVisibility, StyleTransitionOptions, TileJsonInfo, TileScheme, TileSourceOptions,
    VectorTileEncoding,
};
use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_core::ptr::const_ptr_or_null;
use maplibre_native_ffi_core::values::lat_lngs_to_native;
use maplibre_native_ffi_sys as sys;

use crate::custom_geometry::{
    CanonicalTileId, CustomGeometrySourceOptions, CustomGeometrySourceState,
};
use crate::render::PremultipliedRgba8Image;
use crate::runtime::{OperationHandle, OperationKind};
use crate::values::NativeValue;
use crate::{Error, ErrorKind, LatLng, LatLngBounds, Result};

/// Horizontal and vertical stretch intervals for one style image.
pub type StyleImageStretches = (Vec<ImageStretch>, Vec<ImageStretch>);

impl super::MapHandle {
    /// Loads a style URL through MapLibre Native style APIs.
    ///
    /// Loading is asynchronous: a style that fails to fetch or parse still
    /// returns `Ok` here and reports through a later loading-failed runtime
    /// event. Watch the event stream for the load outcome.
    pub fn set_style_url(&self, url: &str) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let url = maplibre_core::string::c_string(url)?;
        // SAFETY: map is live and url is a NUL-terminated UTF-8 string the C
        // API consumes before returning.
        maplibre_core::check(unsafe {
            sys::mln_map_set_style_url(map, url.as_ptr(), &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Loads inline style JSON through MapLibre Native style APIs.
    ///
    /// A parse failure is reported twice: this call returns the error, and the
    /// same message arrives as a loading-failed runtime event.
    pub fn set_style_json(&self, json: &[u8]) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let json = maplibre_core::string::buffer_view(json);
        // SAFETY: map is live and json is valid for the call. Style replacement
        // completes before a successful return, so the C API has already
        // released the callback state of the sources this load dropped.
        maplibre_core::check(unsafe { sys::mln_map_set_style_json(map, json, &mut command_id) })?;
        Ok(command_id)
    }

    /// Copies the style document this map's style was last parsed from: the
    /// string given to [`Self::set_style_json`] or the body fetched for
    /// [`Self::set_style_url`], byte for byte. Runtime mutations do not change
    /// it. An empty buffer means no document has been parsed.
    pub fn loaded_style_json(&self) -> Result<OperationHandle<Vec<u8>>> {
        let map = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe { sys::mln_map_loaded_style_json_start(map, &mut operation) })?;
        self.start_operation(operation, OperationKind::LoadedStyleJson)
    }

    /// Copies the URL this map's style was last requested from.
    ///
    /// [`Self::set_style_url`] records the URL when the request is made, before
    /// the response arrives, and [`Self::set_style_json`] clears it, so this can
    /// disagree with [`Self::loaded_style_json`] while a load is in flight. An
    /// empty string means no URL bytes are available.
    pub fn style_url(&self) -> Result<OperationHandle<String>> {
        let map = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe { sys::mln_map_style_url_start(map, &mut operation) })?;
        self.start_operation(operation, OperationKind::StyleUrl)
    }

    /// Adds a custom geometry source to the current style.
    ///
    /// The callback state is scoped to this map's current style. The C API
    /// frees it once it stops referencing it, whether the source is removed,
    /// dropped by a style load, or retired with the map. Native may invoke
    /// callbacks from worker threads, so schedule host-context work before
    /// calling map APIs.
    pub fn add_custom_geometry_source(
        &self,
        source_id: &str,
        options: CustomGeometrySourceOptions,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id_view = maplibre_core::string::string_view(source_id);
        let state = CustomGeometrySourceState::new(options);
        let descriptor = state.descriptor();
        // The descriptor's release callback frees this box, so the C API owns
        // the callback state from a successful add onwards.
        let state = Box::into_raw(state);
        // SAFETY: map is live, source_id_view is valid for this call, and
        // descriptor names callback state that lives until the release callback.
        let status = unsafe {
            sys::mln_map_add_custom_geometry_source(
                map,
                source_id_view.raw(),
                &descriptor,
                &mut command_id,
            )
        };
        if let Err(error) = maplibre_core::check(status) {
            // SAFETY: A rejected add releases nothing, so this box is still
            // this call's to free.
            drop(unsafe { Box::from_raw(state) });
            return Err(error);
        }
        Ok(command_id)
    }

    /// Sets custom geometry source data for one canonical tile.
    pub fn set_custom_geometry_source_tile_data(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        data: &[u8],
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let data = maplibre_core::string::buffer_view(data);
        // SAFETY: map is live, source_id is valid for this call, tile_id is
        // passed by value, and data remains valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_custom_geometry_source_tile_data(
                map,
                source_id.raw(),
                tile_id.to_native(),
                data,
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Invalidates custom geometry source data for one canonical tile.
    pub fn invalidate_custom_geometry_source_tile(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live, source_id is valid for this call, and tile_id is
        // passed by value.
        maplibre_core::check(unsafe {
            sys::mln_map_invalidate_custom_geometry_source_tile(
                map,
                source_id.raw(),
                tile_id.to_native(),
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Invalidates custom geometry source data inside a geographic region.
    pub fn invalidate_custom_geometry_source_region(
        &self,
        source_id: &str,
        bounds: LatLngBounds,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live, source_id is valid for this call, and bounds is
        // passed by value.
        maplibre_core::check(unsafe {
            sys::mln_map_invalidate_custom_geometry_source_region(
                map,
                source_id.raw(),
                bounds.to_native(),
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Adds one style source from a style-spec source JSON object.
    pub fn add_style_source_json(&self, source_id: &str, source_json: &[u8]) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let source_json = maplibre_core::string::buffer_view(source_json);
        // SAFETY: map is live, source_id and source_json are explicit-length
        // views valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_add_style_source_json(map, source_id.raw(), source_json, &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Adds a vector source with a TileJSON URL.
    pub fn add_vector_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&TileSourceOptions>,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let url = maplibre_core::string::string_view(url);
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id and url are valid for this call, and
        // options_ptr is null or points to call-scoped native options.
        maplibre_core::check(unsafe {
            sys::mln_map_add_vector_source_url(
                map,
                source_id.raw(),
                url.raw(),
                options_ptr,
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Adds a vector source with inline tile URLs.
    pub fn add_vector_source_tiles<S: AsRef<str>>(
        &self,
        source_id: &str,
        tiles: &[S],
        options: Option<&TileSourceOptions>,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let raw_tiles = NativeTileUrls::new(tiles);
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id is valid for this call, raw_tiles
        // points to call-scoped string views, and options_ptr is null or points
        // to call-scoped native options.
        maplibre_core::check(unsafe {
            sys::mln_map_add_vector_source_tiles(
                map,
                source_id.raw(),
                raw_tiles.as_ptr(),
                raw_tiles.len(),
                options_ptr,
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Adds a raster source with a TileJSON URL.
    pub fn add_raster_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&TileSourceOptions>,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let url = maplibre_core::string::string_view(url);
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id and url are valid for this call, and
        // options_ptr is null or points to call-scoped native options.
        maplibre_core::check(unsafe {
            sys::mln_map_add_raster_source_url(
                map,
                source_id.raw(),
                url.raw(),
                options_ptr,
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Adds a raster source with inline tile URLs.
    pub fn add_raster_source_tiles<S: AsRef<str>>(
        &self,
        source_id: &str,
        tiles: &[S],
        options: Option<&TileSourceOptions>,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let raw_tiles = NativeTileUrls::new(tiles);
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id is valid for this call, raw_tiles
        // points to call-scoped string views, and options_ptr is null or points
        // to call-scoped native options.
        maplibre_core::check(unsafe {
            sys::mln_map_add_raster_source_tiles(
                map,
                source_id.raw(),
                raw_tiles.as_ptr(),
                raw_tiles.len(),
                options_ptr,
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Adds a raster DEM source with a TileJSON URL.
    pub fn add_raster_dem_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&TileSourceOptions>,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let url = maplibre_core::string::string_view(url);
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id and url are valid for this call, and
        // options_ptr is null or points to call-scoped native options.
        maplibre_core::check(unsafe {
            sys::mln_map_add_raster_dem_source_url(
                map,
                source_id.raw(),
                url.raw(),
                options_ptr,
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Adds a raster DEM source with inline tile URLs.
    pub fn add_raster_dem_source_tiles<S: AsRef<str>>(
        &self,
        source_id: &str,
        tiles: &[S],
        options: Option<&TileSourceOptions>,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let raw_tiles = NativeTileUrls::new(tiles);
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id is valid for this call, raw_tiles
        // points to call-scoped string views, and options_ptr is null or points
        // to call-scoped native options.
        maplibre_core::check(unsafe {
            sys::mln_map_add_raster_dem_source_tiles(
                map,
                source_id.raw(),
                raw_tiles.as_ptr(),
                raw_tiles.len(),
                options_ptr,
                &mut command_id,
            )
        })?;
        Ok(command_id)
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
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let coordinates = lat_lngs_to_native(coordinates);
        let url = maplibre_core::string::string_view(url);
        // SAFETY: map is live, source_id and url are explicit-length views
        // valid for this call, and coordinates points to call-scoped native
        // coordinate storage. Native validates coordinate contents.
        maplibre_core::check(unsafe {
            sys::mln_map_add_image_source_url(
                map,
                source_id.raw(),
                const_ptr_or_null(&coordinates),
                coordinates.len(),
                url.raw(),
                &mut command_id,
            )
        })?;
        Ok(command_id)
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
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let coordinates = lat_lngs_to_native(coordinates);
        let image = maplibre_core::values::premultiplied_rgba8_image_to_native(image);
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, coordinates points to call-scoped native coordinate
        // storage, and image points into the borrowed Rust image for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_add_image_source_image(
                map,
                source_id.raw(),
                const_ptr_or_null(&coordinates),
                coordinates.len(),
                &image,
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Updates an image source to load its image from a URL.
    pub fn set_image_source_url(&self, source_id: &str, url: &str) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let url = maplibre_core::string::string_view(url);
        // SAFETY: map is live, and source_id and url are explicit-length views
        // valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_image_source_url(map, source_id.raw(), url.raw(), &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Updates an image source with inline premultiplied RGBA8 pixels.
    pub fn set_image_source_image(
        &self,
        source_id: &str,
        image: &PremultipliedRgba8Image,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let image = maplibre_core::values::premultiplied_rgba8_image_to_native(image);
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and image points into the borrowed Rust image for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_image_source_image(map, source_id.raw(), &image, &mut command_id)
        })?;
        Ok(command_id)
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
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let coordinates = lat_lngs_to_native(coordinates);
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and coordinates points to call-scoped native coordinate
        // storage. Native validates coordinate contents.
        maplibre_core::check(unsafe {
            sys::mln_map_set_image_source_coordinates(
                map,
                source_id.raw(),
                const_ptr_or_null(&coordinates),
                coordinates.len(),
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Copies image source coordinates into owned Rust values.
    pub fn image_source_coordinates(
        &self,
        source_id: &str,
    ) -> Result<OperationHandle<Option<[LatLng; 4]>>> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_get_image_source_coordinates_start(map, source_id.raw(), &mut operation)
        })?;
        self.start_operation(operation, OperationKind::ImageSourceCoordinates)
    }

    /// Removes one style source by ID and returns its command ID.
    ///
    /// The command's finished event reports `Failed` with a not-found status
    /// code when no source has the ID, and `Failed` with an invalid-state
    /// status code when a layer still uses the source.
    pub fn remove_style_source(&self, source_id: &str) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live and source_id is valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_remove_style_source(map, source_id.raw(), &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Adds or replaces one runtime style image.
    pub fn set_style_image(
        &self,
        image_id: &str,
        image: &PremultipliedRgba8Image,
        options: Option<&StyleImageOptions>,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let image_id = maplibre_core::string::string_view(image_id);
        let image = maplibre_core::values::premultiplied_rgba8_image_to_native(image);
        let options = options.map(StyleImageOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeStyleImageOptions::as_ptr);
        // SAFETY: map is live, image_id is an explicit-length view valid for
        // this call, image points into the borrowed Rust image for this call,
        // and options_ptr is either null or points to call-scoped options.
        maplibre_core::check(unsafe {
            sys::mln_map_set_style_image(map, image_id.raw(), &image, options_ptr, &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Removes one runtime style image by ID and returns its command ID.
    ///
    /// The command's finished event reports `Failed` with a not-found status
    /// code when no runtime image has the ID.
    pub fn remove_style_image(&self, image_id: &str) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let image_id = maplibre_core::string::string_view(image_id);
        // SAFETY: map is live and image_id is valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_remove_style_image(map, image_id.raw(), &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Copies fixed metadata for one runtime style image.
    pub fn style_image_info(
        &self,
        image_id: &str,
    ) -> Result<OperationHandle<Option<StyleImageInfo>>> {
        let map = self.inner.native()?;
        let image_id = maplibre_core::string::string_view(image_id);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_image_info_start(map, image_id.raw(), &mut operation)
        })?;
        self.start_operation(operation, OperationKind::StyleImageInfo)
    }

    /// Copies one runtime style image into owned tightly packed premultiplied RGBA8 pixels.
    pub fn copy_style_image_premultiplied_rgba8(
        &self,
        image_id: &str,
    ) -> Result<StyleImageOperation> {
        let info = self.style_image_info(image_id)?;
        let map = self.inner.native()?;
        let image_id = maplibre_core::string::string_view(image_id);
        let mut pixels = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_copy_style_image_premultiplied_rgba8_start(
                map,
                image_id.raw(),
                &mut pixels,
            )
        })?;
        let pixels = self.start_operation(pixels, OperationKind::StyleImagePixels)?;
        Ok(StyleImageOperation { info, pixels })
    }

    /// Copies retained metadata for one style source.
    pub fn style_source_info(&self, source_id: &str) -> Result<StyleSourceInfoOperation> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let mut info = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_source_info_start(map, source_id.raw(), &mut info)
        })?;
        let info = self.start_operation(info, OperationKind::StyleSourceInfo)?;
        let mut attribution = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_copy_style_source_attribution_start(map, source_id.raw(), &mut attribution)
        })?;
        let attribution =
            self.start_operation(attribution, OperationKind::StyleSourceAttribution)?;
        let mut url = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_copy_style_source_url_start(map, source_id.raw(), &mut url)
        })?;
        let url = self.start_operation(url, OperationKind::StyleSourceUrl)?;
        let mut tile_urls = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_source_tile_urls_start(map, source_id.raw(), &mut tile_urls)
        })?;
        let tile_urls = self.start_operation(tile_urls, OperationKind::StyleSourceTileUrls)?;
        Ok(StyleSourceInfoOperation {
            info,
            attribution,
            url,
            tile_urls,
        })
    }

    /// Adds a GeoJSON source that loads data from a URL.
    /// `options` are fixed at creation; later data or URL updates keep them.
    pub fn add_geojson_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&GeoJsonSourceOptions>,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let url = maplibre_core::string::string_view(url);
        let options = options
            .map(GeoJsonSourceOptions::try_to_native)
            .transpose()?;
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeGeoJsonSourceOptions::as_ptr);
        // SAFETY: map is live, source_id and url are valid for this call, and
        // options_ptr is null or points to call-scoped native options that keep
        // the cluster-properties buffer alive.
        maplibre_core::check(unsafe {
            sys::mln_map_add_geojson_source_url(
                map,
                source_id.raw(),
                url.raw(),
                options_ptr,
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Adds a GeoJSON source with inline data.
    /// `options` are fixed at creation; later data or URL updates keep them.
    pub fn add_geojson_source_data(
        &self,
        source_id: &str,
        data: &[u8],
        options: Option<&GeoJsonSourceOptions>,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let data = maplibre_core::string::buffer_view(data);
        let options = options
            .map(GeoJsonSourceOptions::try_to_native)
            .transpose()?;
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeGeoJsonSourceOptions::as_ptr);
        // SAFETY: map is live, source_id and data remain valid for this call,
        // and options_ptr is null or points to call-scoped native options that
        // keep the cluster-properties buffer alive.
        maplibre_core::check(unsafe {
            sys::mln_map_add_geojson_source_data(
                map,
                source_id.raw(),
                data,
                options_ptr,
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Updates one GeoJSON source to load data from a URL.
    ///
    /// The source keeps the options it was added with.
    pub fn set_geojson_source_url(&self, source_id: &str, url: &str) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let url = maplibre_core::string::string_view(url);
        // SAFETY: map is live and source_id and url are valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_geojson_source_url(map, source_id.raw(), url.raw(), &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Updates one GeoJSON source with inline data.
    ///
    /// The source keeps the options it was added with.
    pub fn set_geojson_source_data(&self, source_id: &str, data: &[u8]) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let data = maplibre_core::string::buffer_view(data);
        // SAFETY: map is live, and source_id and data remain valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_geojson_source_data(map, source_id.raw(), data, &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Adds one style layer from a full style-spec layer JSON object.
    pub fn add_style_layer_json(
        &self,
        layer_json: &[u8],
        before_layer_id: Option<&str>,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let layer_json = maplibre_core::string::buffer_view(layer_json);
        let before_layer_id = maplibre_core::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live, and layer_json and before_layer_id are
        // explicit-length views valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_add_style_layer_json(
                map,
                layer_json,
                before_layer_id.raw(),
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Adds a hillshade layer for a raster DEM source.
    pub fn add_hillshade_layer(
        &self,
        layer_id: &str,
        source_id: &str,
        before_layer_id: Option<&str>,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let source_id = maplibre_core::string::string_view(source_id);
        let before_layer_id = maplibre_core::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live, and all string views are valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_add_hillshade_layer(
                map,
                layer_id.raw(),
                source_id.raw(),
                before_layer_id.raw(),
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Adds a color-relief layer for a raster DEM source.
    pub fn add_color_relief_layer(
        &self,
        layer_id: &str,
        source_id: &str,
        before_layer_id: Option<&str>,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let source_id = maplibre_core::string::string_view(source_id);
        let before_layer_id = maplibre_core::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live, and all string views are valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_add_color_relief_layer(
                map,
                layer_id.raw(),
                source_id.raw(),
                before_layer_id.raw(),
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Adds a source-free location indicator layer.
    pub fn add_location_indicator_layer(
        &self,
        layer_id: &str,
        before_layer_id: Option<&str>,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let before_layer_id = maplibre_core::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live, and string views are valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_add_location_indicator_layer(
                map,
                layer_id.raw(),
                before_layer_id.raw(),
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Sets a location indicator layer location.
    pub fn set_location_indicator_location(
        &self,
        layer_id: &str,
        coordinate: LatLng,
        altitude: f64,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live, layer_id is valid for this call, and coordinate
        // is passed by value.
        maplibre_core::check(unsafe {
            sys::mln_map_set_location_indicator_location(
                map,
                layer_id.raw(),
                coordinate.to_native(),
                altitude,
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Sets a location indicator layer bearing in degrees.
    pub fn set_location_indicator_bearing(&self, layer_id: &str, bearing: f64) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id is valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_location_indicator_bearing(
                map,
                layer_id.raw(),
                bearing,
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Sets a location indicator layer accuracy radius in logical pixels.
    pub fn set_location_indicator_accuracy_radius(
        &self,
        layer_id: &str,
        radius: f64,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id is valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_location_indicator_accuracy_radius(
                map,
                layer_id.raw(),
                radius,
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Sets one location indicator image-name property.
    pub fn set_location_indicator_image_name(
        &self,
        layer_id: &str,
        image_kind: LocationIndicatorImageKind,
        image_id: &str,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let image_id = maplibre_core::string::string_view(image_id);
        // SAFETY: map is live, string views are valid for this call, and
        // image_kind is a valid C enum value.
        maplibre_core::check(unsafe {
            sys::mln_map_set_location_indicator_image_name(
                map,
                layer_id.raw(),
                image_kind.raw_value(),
                image_id.raw(),
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Copies one style layer as a full style-spec JSON object.
    pub fn style_layer_json(&self, layer_id: &str) -> Result<OperationHandle<Option<Vec<u8>>>> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_layer_json_start(map, layer_id.raw(), &mut operation)
        })?;
        self.start_operation(operation, OperationKind::StyleLayerJson)
    }

    /// Sets the style light from a style-spec light JSON object.
    pub fn set_style_light_json(&self, light_json: &[u8]) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let light_json = maplibre_core::string::buffer_view(light_json);
        // SAFETY: map is live and light_json remains valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_style_light_json(map, light_json, &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Sets one style light property.
    pub fn set_style_light_property(&self, property_name: &str, value: &[u8]) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let property_name = maplibre_core::string::string_view(property_name);
        let value = maplibre_core::string::buffer_view(value);
        // SAFETY: map is live, and property_name and value remain valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_style_light_property(map, property_name.raw(), value, &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Copies one style light property as a style-spec JSON value.
    pub fn style_light_property(
        &self,
        property_name: &str,
    ) -> Result<OperationHandle<Option<Vec<u8>>>> {
        let map = self.inner.native()?;
        let property_name = maplibre_core::string::string_view(property_name);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_light_property_start(map, property_name.raw(), &mut operation)
        })?;
        self.start_operation(operation, OperationKind::StyleLightProperty)
    }

    /// Sets the style's global transition options. This replaces the whole
    /// configuration rather than merging, and loading a style replaces it
    /// again, so apply an override after the style loads.
    pub fn set_style_transition_options(&self, options: &StyleTransitionOptions) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let raw = maplibre_core::style::style_transition_options_to_native(options);
        // SAFETY: map is live and raw is a fully initialized options struct
        // borrowed for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_style_transition_options(map, &raw, &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Reads the style's global transition options.
    pub fn style_transition_options(&self) -> Result<OperationHandle<StyleTransitionOptions>> {
        let map = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_transition_options_start(map, &mut operation)
        })?;
        self.start_operation(operation, OperationKind::StyleTransitionOptions)
    }

    /// Sets one layer style property.
    pub fn set_layer_property(
        &self,
        layer_id: &str,
        property_name: &str,
        value: &[u8],
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let property_name = maplibre_core::string::string_view(property_name);
        let value = maplibre_core::string::buffer_view(value);
        // SAFETY: map is live, and all string and buffer views remain valid for
        // this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_property(
                map,
                layer_id.raw(),
                property_name.raw(),
                value,
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Copies one layer style property as a style-spec JSON value.
    pub fn layer_property(
        &self,
        layer_id: &str,
        property_name: &str,
    ) -> Result<OperationHandle<Option<Vec<u8>>>> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let property_name = maplibre_core::string::string_view(property_name);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_get_layer_property_start(
                map,
                layer_id.raw(),
                property_name.raw(),
                &mut operation,
            )
        })?;
        self.start_operation(operation, OperationKind::LayerProperty)
    }

    /// Sets or clears one layer filter.
    pub fn set_layer_filter(&self, layer_id: &str, filter: Option<&[u8]>) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let native_filter = filter.map(maplibre_core::string::buffer_view);
        // SAFETY: map is live, layer_id is valid for this call, and the
        // optional filter descriptor is either null or valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_filter(
                map,
                layer_id.raw(),
                native_filter.as_ref().map_or(ptr::null(), ptr::from_ref),
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Copies one layer filter as a style-spec JSON value.
    pub fn layer_filter(&self, layer_id: &str) -> Result<OperationHandle<Option<Vec<u8>>>> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_get_layer_filter_start(map, layer_id.raw(), &mut operation)
        })?;
        self.start_operation(operation, OperationKind::LayerFilter)
    }

    /// Copies one runtime style image's stretchable intervals.
    ///
    /// Returns `None` when no image carries `image_id`.
    pub fn style_image_stretches(
        &self,
        image_id: &str,
    ) -> Result<OperationHandle<Option<StyleImageStretches>>> {
        let map = self.inner.native()?;
        let image_id = maplibre_core::string::string_view(image_id);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_copy_style_image_stretches_start(map, image_id.raw(), &mut operation)
        })?;
        self.start_operation(operation, OperationKind::StyleImageStretches)
    }

    /// Sets one layer's source-layer ID.
    ///
    /// Layer types that take no source, such as background, are rejected.
    pub fn set_layer_source_layer(&self, layer_id: &str, source_layer: &str) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let source_layer = maplibre_core::string::string_view(source_layer);
        // SAFETY: map is live and both string views stay valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_source_layer(
                map,
                layer_id.raw(),
                source_layer.raw(),
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Copies one layer's source-layer ID, empty when the layer carries none.
    pub fn layer_source_layer(&self, layer_id: &str) -> Result<OperationHandle<String>> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_copy_layer_source_layer_start(map, layer_id.raw(), &mut operation)
        })?;
        self.start_operation(operation, OperationKind::LayerSourceLayer)
    }

    /// Sets one layer's source ID.
    ///
    /// Layer types that take no source, such as background, are rejected. The
    /// named source need not exist yet.
    pub fn set_layer_source_id(&self, layer_id: &str, source_id: &str) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live and both string views stay valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_source_id(map, layer_id.raw(), source_id.raw(), &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Copies one layer's source ID, empty when the layer carries none.
    pub fn layer_source_id(&self, layer_id: &str) -> Result<OperationHandle<String>> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_copy_layer_source_id_start(map, layer_id.raw(), &mut operation)
        })?;
        self.start_operation(operation, OperationKind::LayerSourceId)
    }

    /// Sets the lowest zoom at which one layer draws.
    ///
    /// Pass `f64::NEG_INFINITY` for no lower bound.
    pub fn set_layer_min_zoom(&self, layer_id: &str, min_zoom: f64) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id stays valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_min_zoom(map, layer_id.raw(), min_zoom, &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Sets the highest zoom at which one layer draws.
    ///
    /// Pass `f64::INFINITY` for no upper bound.
    pub fn set_layer_max_zoom(&self, layer_id: &str, max_zoom: f64) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id stays valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_max_zoom(map, layer_id.raw(), max_zoom, &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Sets whether one layer draws.
    pub fn set_layer_visibility(
        &self,
        layer_id: &str,
        visibility: StyleLayerVisibility,
    ) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id stays valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_visibility(
                map,
                layer_id.raw(),
                visibility.raw_value(),
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    /// Copies current style source IDs into owned Rust strings.
    pub fn style_source_ids(&self) -> Result<OperationHandle<Vec<String>>> {
        let map = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_list_style_source_ids_start(map, &mut operation)
        })?;
        self.start_operation(operation, OperationKind::StyleSourceIds)
    }

    /// Copies current style layer IDs into owned Rust strings.
    pub fn style_layer_ids(&self) -> Result<OperationHandle<Vec<String>>> {
        let map = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_list_style_layer_ids_start(map, &mut operation)
        })?;
        self.start_operation(operation, OperationKind::StyleLayerIds)
    }
    /// Removes one style layer by ID and returns its command ID.
    ///
    /// The command's finished event reports `Failed` with a not-found status
    /// code when no layer has the ID.
    pub fn remove_style_layer(&self, layer_id: &str) -> Result<u64> {
        let mut command_id = 0;
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id is valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_remove_style_layer(map, layer_id.raw(), &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Copies fixed metadata for one style layer.
    ///
    /// The operation resolves the layer's type, zoom range, visibility, and
    /// source IDs together; its take returns `None` when no layer carries
    /// `layer_id`.
    pub fn style_layer_info(&self, layer_id: &str) -> Result<StyleLayerInfoOperation> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let mut info = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_layer_info_start(map, layer_id.raw(), &mut info)
        })?;
        let info = self.start_operation(info, OperationKind::StyleLayerInfo)?;
        let mut source_id = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_copy_layer_source_id_start(map, layer_id.raw(), &mut source_id)
        })?;
        let source_id = self.start_operation(source_id, OperationKind::LayerSourceId)?;
        let mut source_layer = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_copy_layer_source_layer_start(map, layer_id.raw(), &mut source_layer)
        })?;
        let source_layer = self.start_operation(source_layer, OperationKind::LayerSourceLayer)?;
        Ok(StyleLayerInfoOperation {
            info,
            source_id,
            source_layer,
        })
    }

    pub fn move_style_layer(&self, layer_id: &str, before_layer_id: Option<&str>) -> Result<u64> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let before_layer_id = maplibre_core::string::string_view(before_layer_id.unwrap_or(""));
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_move_style_layer(
                map,
                layer_id.raw(),
                before_layer_id.raw(),
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }
}

/// Ordered read of retained metadata for one style source.
pub struct StyleSourceInfoOperation {
    info: OperationHandle<Option<SourceInfo>>,
    attribution: OperationHandle<Option<String>>,
    url: OperationHandle<Option<String>>,
    tile_urls: OperationHandle<Option<Vec<String>>>,
}

impl StyleSourceInfoOperation {
    pub fn wait(&self, timeout: Duration) -> Result<bool> {
        let deadline = Instant::now() + timeout;
        for completed in [
            self.info.wait(timeout)?,
            self.attribution
                .wait(deadline.saturating_duration_since(Instant::now()))?,
            self.url
                .wait(deadline.saturating_duration_since(Instant::now()))?,
            self.tile_urls
                .wait(deadline.saturating_duration_since(Instant::now()))?,
        ] {
            if !completed {
                return Ok(false);
            }
        }
        Ok(true)
    }

    pub fn is_completed(&self) -> Result<bool> {
        Ok(self.info.is_completed()?
            && self.attribution.is_completed()?
            && self.url.is_completed()?
            && self.tile_urls.is_completed()?)
    }

    pub fn cancel(&self) -> Result<()> {
        self.info.cancel()?;
        self.attribution.cancel()?;
        self.url.cancel()?;
        self.tile_urls.cancel()
    }

    pub fn take(&self) -> Result<Option<SourceInfo>> {
        if !self.is_completed()? {
            return Err(Error::new(
                ErrorKind::InvalidState,
                None,
                "style source info operation has not completed",
            ));
        }
        let Some(mut info) = self.info.take()? else {
            self.attribution.discard()?;
            self.url.discard()?;
            self.tile_urls.discard()?;
            return Ok(None);
        };
        info.attribution = self.attribution.take()?;
        info.url = self.url.take()?;
        let tiles = self.tile_urls.take()?.unwrap_or_default();
        if let Some(tile_json) = info.tile_json.as_mut() {
            tile_json.tiles = tiles;
        }
        Ok(Some(info))
    }
}

/// Ordered read of fixed metadata for one style layer.
pub struct StyleLayerInfoOperation {
    info: OperationHandle<Option<StyleLayerInfo>>,
    source_id: OperationHandle<String>,
    source_layer: OperationHandle<String>,
}

impl StyleLayerInfoOperation {
    pub fn wait(&self, timeout: Duration) -> Result<bool> {
        let deadline = Instant::now() + timeout;
        for completed in [
            self.info.wait(timeout)?,
            self.source_id
                .wait(deadline.saturating_duration_since(Instant::now()))?,
            self.source_layer
                .wait(deadline.saturating_duration_since(Instant::now()))?,
        ] {
            if !completed {
                return Ok(false);
            }
        }
        Ok(true)
    }

    pub fn is_completed(&self) -> Result<bool> {
        Ok(self.info.is_completed()?
            && self.source_id.is_completed()?
            && self.source_layer.is_completed()?)
    }

    pub fn cancel(&self) -> Result<()> {
        self.info.cancel()?;
        self.source_id.cancel()?;
        self.source_layer.cancel()
    }

    pub fn take(&self) -> Result<Option<StyleLayerInfo>> {
        if !self.is_completed()? {
            return Err(Error::new(
                ErrorKind::InvalidState,
                None,
                "style layer info operation has not completed",
            ));
        }
        let mut raw = maplibre_core::style::empty_style_layer_info();
        let mut found = false;
        self.info.with_result_operation(|operation| {
            maplibre_core::check(unsafe {
                sys::mln_map_get_style_layer_info_take_result(operation, &mut raw, &mut found)
            })
        })?;
        if !found {
            // The source-ID copies failed on the same missing layer, so they
            // hold no results; dropping this wrapper releases them.
            return Ok(None);
        }
        let source_id = if raw.fields & sys::MLN_STYLE_LAYER_INFO_SOURCE_ID != 0 {
            Some(self.source_id.take()?)
        } else {
            self.source_id.discard()?;
            None
        };
        let source_layer = if raw.fields & sys::MLN_STYLE_LAYER_INFO_SOURCE_LAYER != 0 {
            Some(self.source_layer.take()?)
        } else {
            self.source_layer.discard()?;
            None
        };
        // SAFETY: the take filled raw, whose type field views a style-spec
        // string that stays valid for the process's life.
        unsafe { maplibre_core::style::style_layer_info_from_native(&raw, source_id, source_layer) }
            .map(Some)
    }
}

/// Ordered read of one style image and its metadata.
pub struct StyleImageOperation {
    info: OperationHandle<Option<StyleImageInfo>>,
    pixels: OperationHandle<Option<Vec<u8>>>,
}

impl StyleImageOperation {
    pub fn wait(&self, timeout: Duration) -> Result<bool> {
        let deadline = Instant::now() + timeout;
        if !self.info.wait(timeout)? {
            return Ok(false);
        }
        self.pixels
            .wait(deadline.saturating_duration_since(Instant::now()))
    }

    pub fn is_completed(&self) -> Result<bool> {
        Ok(self.info.is_completed()? && self.pixels.is_completed()?)
    }

    pub fn cancel(&self) -> Result<()> {
        self.info.cancel()?;
        self.pixels.cancel()
    }

    pub fn take(&self) -> Result<Option<StyleImage>> {
        if !self.is_completed()? {
            return Err(Error::new(
                ErrorKind::InvalidState,
                None,
                "style image operation has not completed",
            ));
        }
        let Some(info) = self.info.take()? else {
            self.pixels.discard()?;
            return Ok(None);
        };
        let Some(pixels) = self.pixels.take()? else {
            return Ok(None);
        };
        maplibre_core::style::style_image_from_copied_premultiplied_rgba8(
            info,
            pixels,
            info.byte_length,
        )
        .map(Some)
    }
}

impl OperationHandle<Vec<u8>> {
    pub fn take(&self) -> Result<Vec<u8>> {
        take_buffer(self, |operation, out| unsafe {
            match self.operation_kind {
                OperationKind::LoadedStyleJson => {
                    sys::mln_map_loaded_style_json_take_result(operation, out)
                }
                OperationKind::RenderQuery => sys::mln_render_query_take_result(operation, out),
                OperationKind::RenderFeatureState => {
                    sys::mln_render_session_get_feature_state_take_result(operation, out)
                }
                _ => sys::MLN_STATUS_INVALID_STATE,
            }
        })
    }
}

impl OperationHandle<String> {
    pub fn take(&self) -> Result<String> {
        let bytes = take_buffer(self, |operation, out| unsafe {
            match self.operation_kind {
                OperationKind::StyleUrl => sys::mln_map_style_url_take_result(operation, out),
                OperationKind::LayerSourceLayer => {
                    sys::mln_map_copy_layer_source_layer_take_result(operation, out)
                }
                OperationKind::LayerSourceId => {
                    sys::mln_map_copy_layer_source_id_take_result(operation, out)
                }
                _ => sys::MLN_STATUS_INVALID_STATE,
            }
        })?;
        String::from_utf8(bytes).map_err(|error| {
            Error::invalid_argument(format!("native style text was not valid UTF-8: {error}"))
        })
    }
}

impl OperationHandle<Option<String>> {
    pub fn take(&self) -> Result<Option<String>> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        let mut found = false;
        self.with_result_operation(|operation| {
            let status = unsafe {
                match self.operation_kind {
                    OperationKind::StyleSourceAttribution => {
                        sys::mln_map_copy_style_source_attribution_take_result(
                            operation,
                            out.as_mut_ptr(),
                            &mut found,
                        )
                    }
                    OperationKind::StyleSourceUrl => {
                        sys::mln_map_copy_style_source_url_take_result(
                            operation,
                            out.as_mut_ptr(),
                            &mut found,
                        )
                    }
                    _ => sys::MLN_STATUS_INVALID_STATE,
                }
            };
            maplibre_core::check(status)
        })?;
        if !found {
            return Ok(None);
        }
        let bytes = unsafe { maplibre_core::string::copy_owned_buffer(out.get()) }?;
        String::from_utf8(bytes).map(Some).map_err(|error| {
            Error::invalid_argument(format!("native style text was not valid UTF-8: {error}"))
        })
    }
}

impl OperationHandle<Option<Vec<u8>>> {
    pub fn take(&self) -> Result<Option<Vec<u8>>> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        let mut found = true;
        self.with_result_operation(|operation| {
            let status = unsafe {
                match self.operation_kind {
                    OperationKind::StyleImagePixels => {
                        sys::mln_map_copy_style_image_premultiplied_rgba8_take_result(
                            operation,
                            out.as_mut_ptr(),
                            &mut found,
                        )
                    }
                    OperationKind::StyleLayerJson => sys::mln_map_get_style_layer_json_take_result(
                        operation,
                        out.as_mut_ptr(),
                        &mut found,
                    ),
                    OperationKind::StyleLightProperty => {
                        sys::mln_map_get_style_light_property_take_result(
                            operation,
                            out.as_mut_ptr(),
                        )
                    }
                    OperationKind::LayerProperty => {
                        sys::mln_map_get_layer_property_take_result(operation, out.as_mut_ptr())
                    }
                    OperationKind::LayerFilter => {
                        sys::mln_map_get_layer_filter_take_result(operation, out.as_mut_ptr())
                    }
                    _ => sys::MLN_STATUS_INVALID_STATE,
                }
            };
            maplibre_core::check(status)
        })?;
        if !found || out.get().0 == 0 {
            return Ok(None);
        }
        unsafe { maplibre_core::string::copy_owned_buffer(out.get()) }.map(Some)
    }
}

impl OperationHandle<Option<SourceInfo>> {
    pub fn take(&self) -> Result<Option<SourceInfo>> {
        let mut raw = maplibre_core::style::empty_style_source_info();
        let mut found = false;
        self.with_result_operation(|operation| {
            maplibre_core::check(unsafe {
                sys::mln_map_get_style_source_info_take_result(operation, &mut raw, &mut found)
            })
        })?;
        Ok(found.then(|| {
            maplibre_core::style::style_source_info_from_native(&raw, None, None, Vec::new())
        }))
    }
}

impl OperationHandle<Option<Vec<String>>> {
    pub fn take(&self) -> Result<Option<Vec<String>>> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_style_string_list>::new();
        let mut found = false;
        self.with_result_operation(|operation| {
            maplibre_core::check(unsafe {
                sys::mln_map_get_style_source_tile_urls_take_result(
                    operation,
                    out.as_mut_ptr(),
                    &mut found,
                )
            })
        })?;
        if !found {
            return Ok(None);
        }
        unsafe {
            maplibre_core::style::copy_style_string_list(out.into_live("mln_style_string_list")?)
        }
        .map(Some)
    }
}

impl OperationHandle<Vec<String>> {
    pub fn take(&self) -> Result<Vec<String>> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_style_id_list>::new();
        self.with_result_operation(|operation| {
            let status = unsafe {
                match self.operation_kind {
                    OperationKind::StyleSourceIds => {
                        sys::mln_map_list_style_source_ids_take_result(operation, out.as_mut_ptr())
                    }
                    OperationKind::StyleLayerIds => {
                        sys::mln_map_list_style_layer_ids_take_result(operation, out.as_mut_ptr())
                    }
                    _ => sys::MLN_STATUS_INVALID_STATE,
                }
            };
            maplibre_core::check(status)
        })?;
        unsafe { maplibre_core::style::copy_style_id_list(out.into_live("mln_style_id_list")?) }
    }
}

impl OperationHandle<Option<StyleImageInfo>> {
    pub fn take(&self) -> Result<Option<StyleImageInfo>> {
        let mut raw = maplibre_core::style::empty_style_image_info();
        let mut found = false;
        self.with_result_operation(|operation| {
            maplibre_core::check(unsafe {
                sys::mln_map_get_style_image_info_take_result(operation, &mut raw, &mut found)
            })
        })?;
        Ok(found.then(|| maplibre_core::values::style_image_info_from_native(&raw)))
    }
}

impl OperationHandle<Option<[LatLng; 4]>> {
    pub fn take(&self) -> Result<Option<[LatLng; 4]>> {
        let mut raw = [sys::mln_lat_lng {
            latitude: 0.0,
            longitude: 0.0,
        }; 4];
        let mut count = 0;
        let mut found = false;
        self.with_result_operation(|operation| {
            maplibre_core::check(unsafe {
                sys::mln_map_get_image_source_coordinates_take_result(
                    operation,
                    raw.as_mut_ptr(),
                    raw.len(),
                    &mut count,
                    &mut found,
                )
            })
        })?;
        if !found {
            return Ok(None);
        }
        if count != raw.len() {
            return Err(Error::new(
                ErrorKind::NativeError,
                None,
                "native image source coordinate count did not match Rust image source invariant",
            ));
        }
        Ok(Some(raw.map(LatLng::from_native)))
    }
}

impl OperationHandle<Option<(Vec<ImageStretch>, Vec<ImageStretch>)>> {
    pub fn take(&self) -> Result<Option<(Vec<ImageStretch>, Vec<ImageStretch>)>> {
        let mut x_count = 0;
        let mut y_count = 0;
        let mut found = false;
        let mut copied = None;
        self.with_result_operation(|operation| {
            maplibre_core::check(unsafe {
                sys::mln_map_copy_style_image_stretches_take_result(
                    operation,
                    ptr::null_mut(),
                    0,
                    &mut x_count,
                    ptr::null_mut(),
                    0,
                    &mut y_count,
                    &mut found,
                )
            })?;
            let mut x = vec![sys::mln_image_stretch { from: 0.0, to: 0.0 }; x_count];
            let mut y = vec![sys::mln_image_stretch { from: 0.0, to: 0.0 }; y_count];
            maplibre_core::check(unsafe {
                sys::mln_map_copy_style_image_stretches_take_result(
                    operation,
                    x.as_mut_ptr(),
                    x.len(),
                    &mut x_count,
                    y.as_mut_ptr(),
                    y.len(),
                    &mut y_count,
                    &mut found,
                )
            })?;
            copied = Some((x, y));
            Ok(())
        })?;
        if !found {
            return Ok(None);
        }
        let (x, y) = copied.ok_or_else(|| {
            Error::new(
                ErrorKind::NativeError,
                None,
                "native stretch result was unavailable",
            )
        })?;
        let convert = |values: Vec<sys::mln_image_stretch>| {
            values
                .into_iter()
                .map(|value| ImageStretch::new(value.from, value.to))
                .collect()
        };
        Ok(Some((convert(x), convert(y))))
    }
}

impl OperationHandle<StyleTransitionOptions> {
    pub fn take(&self) -> Result<StyleTransitionOptions> {
        let mut raw = maplibre_core::style::empty_style_transition_options();
        self.with_result_operation(|operation| {
            maplibre_core::check(unsafe {
                sys::mln_map_get_style_transition_options_take_result(operation, &mut raw)
            })
        })?;
        Ok(maplibre_core::style::style_transition_options_from_native(
            &raw,
        ))
    }
}

fn take_buffer<T>(
    operation: &OperationHandle<T>,
    take: impl FnOnce(sys::mln_operation, *mut sys::mln_buffer) -> sys::mln_status,
) -> Result<Vec<u8>> {
    let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
    operation.with_result_operation(|operation| {
        maplibre_core::check(take(operation, out.as_mut_ptr()))
    })?;
    unsafe { maplibre_core::string::copy_owned_buffer(out.get()) }
}
