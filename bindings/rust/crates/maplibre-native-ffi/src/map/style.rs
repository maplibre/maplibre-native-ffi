use std::ptr;

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
use maplibre_native_ffi_core::query::FeatureStateSelectorNativeExt;
use maplibre_native_ffi_core::values::lat_lngs_to_native;
use maplibre_native_ffi_sys as sys;

use crate::custom_geometry::{
    CanonicalTileId, CustomGeometrySourceOptions, CustomGeometrySourceState,
};
use crate::custom_mvt_vector::{CustomMvtVectorSourceOptions, CustomMvtVectorSourceState};
use crate::render::PremultipliedRgba8Image;
use crate::values::NativeValue;
use crate::{CommandCompletion, FeatureStateSelector, LatLng, LatLngBounds, NativeFuture, Result};

/// Horizontal and vertical stretch intervals for one style image, in that
/// order.
pub type StyleImageStretches = (Vec<ImageStretch>, Vec<ImageStretch>);

impl super::MapHandle {
    /// Submits a style URL load.
    ///
    /// The command commits once the map worker accepts the URL. Fetching and
    /// parsing follow, so a style that fails either one reports through a
    /// later loading-failed runtime event rather than through this future.
    pub fn set_style_url(&self, url: &str) -> Result<NativeFuture<CommandCompletion>> {
        let url = maplibre_core::string::c_string(url)?;
        // SAFETY: map is live and url is a NUL-terminated UTF-8 string the C
        // API consumes before returning.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_style_url(map, url.as_ptr(), completion)
        })
    }

    /// Submits an inline style JSON load. The bytes are copied before this
    /// returns and parsed on the map worker.
    ///
    /// A parse failure is reported twice: the command's completion carries the
    /// error, and the same message arrives as a loading-failed runtime event.
    pub fn set_style_json(&self, json: &[u8]) -> Result<NativeFuture<CommandCompletion>> {
        let json = maplibre_core::string::buffer_view(json);
        // SAFETY: map is live and the view stays readable for this
        // synchronous submission, which copies the bytes it keeps.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_style_json(map, json, completion)
        })
    }

    /// Sets per-feature state on this map. The state bytes must contain one
    /// UTF-8 JSON object and are copied before the call returns.
    pub fn set_feature_state(
        &self,
        selector: &FeatureStateSelector,
        state: &[u8],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let selector = selector.to_native();
        let state = maplibre_core::string::buffer_view(state);
        // SAFETY: map is live and all borrowed storage remains valid for the
        // synchronous submission; the C API copies selector and state bytes
        // before return.
        self.submit_command(|map, completion| unsafe {
            sys::mln_map_set_feature_state(map, selector.as_ptr(), state, completion)
        })
    }

    /// Copies per-feature state from this map. The read observes every map
    /// command accepted before it, and missing state is an empty JSON object.
    pub fn get_feature_state(
        &self,
        selector: &FeatureStateSelector,
    ) -> Result<NativeFuture<Vec<u8>>> {
        let selector = selector.to_native();
        // SAFETY: map is live and selector storage remains valid for the
        // synchronous submission; the C API copies selector bytes before
        // return.
        self.submit_query(
            |map, completion| unsafe {
                sys::mln_map_get_feature_state(map, selector.as_ptr(), completion)
            },
            crate::completion::buffer,
        )
    }

    /// Removes per-feature state selected on this map. A selector with a state
    /// key removes one key, one with only a feature ID removes that feature's
    /// state, and one with neither removes all state for the source.
    pub fn remove_feature_state(
        &self,
        selector: &FeatureStateSelector,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let selector = selector.to_native();
        // SAFETY: map is live and selector storage remains valid for the
        // synchronous submission; the C API copies selector bytes before
        // return.
        self.submit_command(|map, completion| unsafe {
            sys::mln_map_remove_feature_state(map, selector.as_ptr(), completion)
        })
    }

    /// Copies the style document this map's style was last parsed from: the
    /// string given to [`Self::set_style_json`] or the body fetched for
    /// [`Self::set_style_url`], byte for byte. Runtime mutations do not change
    /// it. An empty buffer means no document has been parsed.
    pub fn loaded_style_json(&self) -> Result<NativeFuture<Vec<u8>>> {
        // SAFETY: map is live for this synchronous submission.
        self.submit_query(
            move |map, completion| unsafe { sys::mln_map_loaded_style_json(map, completion) },
            crate::completion::buffer,
        )
    }

    /// Copies the URL this map's style was last requested from.
    ///
    /// [`Self::set_style_url`] records the URL when the request is made, before
    /// the response arrives, and [`Self::set_style_json`] clears it, so this can
    /// disagree with [`Self::loaded_style_json`] while a load is in flight. An
    /// empty string means no URL bytes are available.
    pub fn style_url(&self) -> Result<NativeFuture<String>> {
        // SAFETY: map is live for this synchronous submission.
        self.submit_query(
            move |map, completion| unsafe { sys::mln_map_style_url(map, completion) },
            crate::completion::string,
        )
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
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id_view = maplibre_core::string::string_view(source_id);
        let state = CustomGeometrySourceState::new(options);
        let descriptor = state.descriptor();
        // The descriptor's release callback frees this box, so the C API owns
        // the callback state from a successful add onwards.
        let state = Box::into_raw(state);
        // SAFETY: map is live, source_id_view is valid for this call, and
        // descriptor names callback state that lives until the release callback.
        let result = self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_custom_geometry_source(
                map,
                source_id_view.raw(),
                &descriptor,
                completion,
            )
        });
        if result.is_err() {
            // SAFETY: rejected submissions retain ownership of callback state.
            drop(unsafe { Box::from_raw(state) });
        }
        result
    }

    /// Sets custom geometry source data for one canonical tile.
    pub fn set_custom_geometry_source_tile_data(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        data: &[u8],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        let data = maplibre_core::string::buffer_view(data);
        // SAFETY: map is live, source_id is valid for this call, tile_id is
        // passed by value, and data remains valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_custom_geometry_source_tile_data(
                map,
                source_id.raw(),
                tile_id.to_native(),
                data,
                completion,
            )
        })
    }

    /// Invalidates custom geometry source data for one canonical tile.
    pub fn invalidate_custom_geometry_source_tile(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live, source_id is valid for this call, and tile_id is
        // passed by value.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_invalidate_custom_geometry_source_tile(
                map,
                source_id.raw(),
                tile_id.to_native(),
                completion,
            )
        })
    }

    /// Invalidates custom geometry source data inside a geographic region.
    pub fn invalidate_custom_geometry_source_region(
        &self,
        source_id: &str,
        bounds: LatLngBounds,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live, source_id is valid for this call, and bounds is
        // passed by value.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_invalidate_custom_geometry_source_region(
                map,
                source_id.raw(),
                bounds.to_native(),
                completion,
            )
        })
    }

    /// Adds a custom MVT vector source to the current style.
    ///
    /// The callback state is scoped to this map's current style. The C API
    /// frees it once it stops referencing it, whether the source is removed,
    /// dropped by a style load, or retired with the map. Native may invoke
    /// callbacks from worker threads, so schedule host-context work before
    /// calling map APIs.
    pub fn add_custom_mvt_vector_source(
        &self,
        source_id: &str,
        options: CustomMvtVectorSourceOptions,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id_view = maplibre_core::string::string_view(source_id);
        let state = CustomMvtVectorSourceState::new(options);
        let descriptor = state.descriptor();
        // The descriptor's release callback frees this box, so the C API owns
        // the callback state from a successful add onwards.
        let state = Box::into_raw(state);
        // SAFETY: map is live, source_id_view is valid for this call, and
        // descriptor names callback state that lives until the release callback.
        let result = self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_custom_mvt_vector_source(
                map,
                source_id_view.raw(),
                &descriptor,
                completion,
            )
        });
        if result.is_err() {
            // SAFETY: rejected submissions retain ownership of callback state.
            drop(unsafe { Box::from_raw(state) });
        }
        result
    }

    /// Sets custom MVT vector source data for one canonical tile.
    ///
    /// Pass an empty slice for an empty tile. Native ignores the bytes when
    /// that tile is not awaiting a response after fetch.
    pub fn set_custom_mvt_vector_source_tile_data(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        data: &[u8],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        let data = maplibre_core::string::buffer_view(data);
        // SAFETY: map is live, source_id is valid for this call, tile_id is
        // passed by value, and data remains valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_custom_mvt_vector_source_tile_data(
                map,
                source_id.raw(),
                tile_id.to_native(),
                data,
                completion,
            )
        })
    }

    /// Reports a custom MVT vector source error for one canonical tile.
    pub fn set_custom_mvt_vector_source_tile_error(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        message: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        let message = maplibre_core::string::string_view(message);
        // SAFETY: map is live, source_id and message are valid for this call,
        // and tile_id is passed by value.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_custom_mvt_vector_source_tile_error(
                map,
                source_id.raw(),
                tile_id.to_native(),
                message.raw(),
                completion,
            )
        })
    }

    /// Invalidates custom MVT vector source data for one canonical tile.
    pub fn invalidate_custom_mvt_vector_source_tile(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live, source_id is valid for this call, and tile_id is
        // passed by value.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_invalidate_custom_mvt_vector_source_tile(
                map,
                source_id.raw(),
                tile_id.to_native(),
                completion,
            )
        })
    }

    /// Adds one style source from a style-spec source JSON object.
    pub fn add_style_source_json(
        &self,
        source_id: &str,
        source_json: &[u8],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        let source_json = maplibre_core::string::buffer_view(source_json);
        // SAFETY: map is live, source_id and source_json are explicit-length
        // views valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_style_source_json(map, source_id.raw(), source_json, completion)
        })
    }

    /// Adds a vector source with a TileJSON URL.
    pub fn add_vector_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&TileSourceOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        let url = maplibre_core::string::string_view(url);
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id and url are valid for this call, and
        // options_ptr is null or points to call-scoped native options.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_vector_source_url(
                map,
                source_id.raw(),
                url.raw(),
                options_ptr,
                completion,
            )
        })
    }

    /// Adds a vector source with inline tile URLs.
    pub fn add_vector_source_tiles<S: AsRef<str>>(
        &self,
        source_id: &str,
        tiles: &[S],
        options: Option<&TileSourceOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        let raw_tiles = NativeTileUrls::new(tiles);
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id is valid for this call, raw_tiles
        // points to call-scoped string views, and options_ptr is null or points
        // to call-scoped native options.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_vector_source_tiles(
                map,
                source_id.raw(),
                raw_tiles.as_ptr(),
                raw_tiles.len(),
                options_ptr,
                completion,
            )
        })
    }

    /// Adds a raster source with a TileJSON URL.
    pub fn add_raster_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&TileSourceOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        let url = maplibre_core::string::string_view(url);
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id and url are valid for this call, and
        // options_ptr is null or points to call-scoped native options.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_raster_source_url(
                map,
                source_id.raw(),
                url.raw(),
                options_ptr,
                completion,
            )
        })
    }

    /// Adds a raster source with inline tile URLs.
    pub fn add_raster_source_tiles<S: AsRef<str>>(
        &self,
        source_id: &str,
        tiles: &[S],
        options: Option<&TileSourceOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        let raw_tiles = NativeTileUrls::new(tiles);
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id is valid for this call, raw_tiles
        // points to call-scoped string views, and options_ptr is null or points
        // to call-scoped native options.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_raster_source_tiles(
                map,
                source_id.raw(),
                raw_tiles.as_ptr(),
                raw_tiles.len(),
                options_ptr,
                completion,
            )
        })
    }

    /// Adds a raster DEM source with a TileJSON URL.
    pub fn add_raster_dem_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&TileSourceOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        let url = maplibre_core::string::string_view(url);
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id and url are valid for this call, and
        // options_ptr is null or points to call-scoped native options.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_raster_dem_source_url(
                map,
                source_id.raw(),
                url.raw(),
                options_ptr,
                completion,
            )
        })
    }

    /// Adds a raster DEM source with inline tile URLs.
    pub fn add_raster_dem_source_tiles<S: AsRef<str>>(
        &self,
        source_id: &str,
        tiles: &[S],
        options: Option<&TileSourceOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        let raw_tiles = NativeTileUrls::new(tiles);
        let options = options.map(TileSourceOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeTileSourceOptions::as_ptr);
        // SAFETY: map is live, source_id is valid for this call, raw_tiles
        // points to call-scoped string views, and options_ptr is null or points
        // to call-scoped native options.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_raster_dem_source_tiles(
                map,
                source_id.raw(),
                raw_tiles.as_ptr(),
                raw_tiles.len(),
                options_ptr,
                completion,
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
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        let coordinates = lat_lngs_to_native(coordinates);
        let url = maplibre_core::string::string_view(url);
        // SAFETY: map is live, source_id and url are explicit-length views
        // valid for this call, and coordinates points to call-scoped native
        // coordinate storage. Native validates coordinate contents.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_image_source_url(
                map,
                source_id.raw(),
                const_ptr_or_null(&coordinates),
                coordinates.len(),
                url.raw(),
                completion,
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
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        let coordinates = lat_lngs_to_native(coordinates);
        let image = maplibre_core::values::premultiplied_rgba8_image_to_native(image);
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, coordinates points to call-scoped native coordinate
        // storage, and image points into the borrowed Rust image for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_image_source_image(
                map,
                source_id.raw(),
                const_ptr_or_null(&coordinates),
                coordinates.len(),
                &image,
                completion,
            )
        })
    }

    /// Updates an image source to load its image from a URL.
    pub fn set_image_source_url(
        &self,
        source_id: &str,
        url: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        let url = maplibre_core::string::string_view(url);
        // SAFETY: map is live, and source_id and url are explicit-length views
        // valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_image_source_url(map, source_id.raw(), url.raw(), completion)
        })
    }

    /// Updates an image source with inline premultiplied RGBA8 pixels.
    pub fn set_image_source_image(
        &self,
        source_id: &str,
        image: &PremultipliedRgba8Image,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        let image = maplibre_core::values::premultiplied_rgba8_image_to_native(image);
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and image points into the borrowed Rust image for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_image_source_image(map, source_id.raw(), &image, completion)
        })
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
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        let coordinates = lat_lngs_to_native(coordinates);
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and coordinates points to call-scoped native coordinate
        // storage. Native validates coordinate contents.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_image_source_coordinates(
                map,
                source_id.raw(),
                const_ptr_or_null(&coordinates),
                coordinates.len(),
                completion,
            )
        })
    }

    /// Copies image source coordinates into owned Rust values.
    pub fn image_source_coordinates(
        &self,
        source_id: &str,
    ) -> Result<NativeFuture<Option<[LatLng; 4]>>> {
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live for this synchronous submission.
        self.submit_query(
            move |map, completion| unsafe {
                sys::mln_map_get_image_source_coordinates(map, source_id.raw(), completion)
            },
            |result| {
                let values = crate::completion::copy_slice::<sys::mln_lat_lng>(result)?;
                if values.is_empty() {
                    return Ok(None);
                }
                let values: [sys::mln_lat_lng; 4] = values.try_into().map_err(|_| {
                    crate::Error::new(
                        crate::ErrorKind::NativeError,
                        None,
                        "image coordinates completion did not contain four coordinates",
                    )
                })?;
                Ok(Some(values.map(LatLng::from_native)))
            },
        )
    }

    /// Removes one style source by ID.
    ///
    /// The command's finished event reports `Failed` with a not-found status
    /// code when no source has the ID, and `Failed` with an invalid-state
    /// status code when a layer still uses the source.
    pub fn remove_style_source(&self, source_id: &str) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live and source_id is valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_remove_style_source(map, source_id.raw(), completion)
        })
    }

    /// Adds or replaces one runtime style image.
    pub fn set_style_image(
        &self,
        image_id: &str,
        image: &PremultipliedRgba8Image,
        options: Option<&StyleImageOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let image_id = maplibre_core::string::string_view(image_id);
        let image = maplibre_core::values::premultiplied_rgba8_image_to_native(image);
        let options = options.map(StyleImageOptions::to_native);
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeStyleImageOptions::as_ptr);
        // SAFETY: map is live, image_id is an explicit-length view valid for
        // this call, image points into the borrowed Rust image for this call,
        // and options_ptr is either null or points to call-scoped options.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_style_image(map, image_id.raw(), &image, options_ptr, completion)
        })
    }

    /// Removes one runtime style image by ID.
    ///
    /// The command's finished event reports `Failed` with a not-found status
    /// code when no runtime image has the ID.
    pub fn remove_style_image(&self, image_id: &str) -> Result<NativeFuture<CommandCompletion>> {
        let image_id = maplibre_core::string::string_view(image_id);
        // SAFETY: map is live and image_id is valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_remove_style_image(map, image_id.raw(), completion)
        })
    }

    /// Copies one runtime style image and all of its metadata.
    pub fn style_image(&self, image_id: &str) -> Result<NativeFuture<Option<StyleImage>>> {
        let image_id = maplibre_core::string::string_view(image_id);
        // SAFETY: map is live for this synchronous submission.
        self.submit_query(
            move |map, completion| unsafe {
                sys::mln_map_get_style_image_info(map, image_id.raw(), completion)
            },
            copy_style_image,
        )
    }

    /// Copies one runtime style image's pixels as tightly packed premultiplied
    /// RGBA8, without its metadata.
    ///
    /// The completion carries no value when no runtime image has the ID.
    pub fn style_image_premultiplied_rgba8(
        &self,
        image_id: &str,
    ) -> Result<NativeFuture<Option<Vec<u8>>>> {
        let image_id = maplibre_core::string::string_view(image_id);
        // SAFETY: map is live and image_id stays valid for this synchronous
        // submission.
        self.submit_query(
            move |map, completion| unsafe {
                sys::mln_map_copy_style_image_premultiplied_rgba8(map, image_id.raw(), completion)
            },
            crate::completion::optional_buffer,
        )
    }

    /// Copies one runtime style image's stretchable intervals, without its
    /// pixels.
    ///
    /// The completion carries no value when no runtime image has the ID.
    pub fn style_image_stretches(
        &self,
        image_id: &str,
    ) -> Result<NativeFuture<Option<StyleImageStretches>>> {
        let image_id = maplibre_core::string::string_view(image_id);
        // SAFETY: map is live and image_id stays valid for this synchronous
        // submission.
        self.submit_query(
            move |map, completion| unsafe {
                sys::mln_map_copy_style_image_stretches(map, image_id.raw(), completion)
            },
            copy_style_image_stretches,
        )
    }

    /// Copies retained metadata for one style source.
    pub fn style_source_info(&self, source_id: &str) -> Result<NativeFuture<Option<SourceInfo>>> {
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live for this synchronous submission.
        self.submit_query(
            move |map, completion| unsafe {
                sys::mln_map_get_style_source_info(map, source_id.raw(), completion)
            },
            copy_source_info,
        )
    }

    /// Copies one style source's attribution.
    ///
    /// The completion carries no value when no source has the ID, and when the
    /// source carries no attribution. [`MapHandle::style_source_info`] reads
    /// the attribution together with the rest of a source's metadata.
    pub fn style_source_attribution(
        &self,
        source_id: &str,
    ) -> Result<NativeFuture<Option<String>>> {
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live and source_id stays valid for this synchronous
        // submission.
        self.submit_query(
            move |map, completion| unsafe {
                sys::mln_map_copy_style_source_attribution(map, source_id.raw(), completion)
            },
            crate::completion::optional_string,
        )
    }

    /// Copies one style source's URL.
    ///
    /// The completion carries no value when no source has the ID, and when the
    /// source carries inline TileJSON instead of a URL.
    /// [`MapHandle::style_source_info`] reads the URL together with the rest of
    /// a source's metadata.
    pub fn style_source_url(&self, source_id: &str) -> Result<NativeFuture<Option<String>>> {
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live and source_id stays valid for this synchronous
        // submission.
        self.submit_query(
            move |map, completion| unsafe {
                sys::mln_map_copy_style_source_url(map, source_id.raw(), completion)
            },
            crate::completion::optional_string,
        )
    }

    /// Copies one style source's inline TileJSON tile URLs.
    ///
    /// The completion carries an empty sequence when no source has the ID, and
    /// when the source loads its TileJSON from a URL.
    /// [`MapHandle::style_source_info`] reads the tile URLs together with the
    /// rest of a source's metadata, and reports whether the source exists.
    pub fn style_source_tile_urls(&self, source_id: &str) -> Result<NativeFuture<Vec<String>>> {
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live and source_id stays valid for this synchronous
        // submission.
        self.submit_query(
            move |map, completion| unsafe {
                sys::mln_map_get_style_source_tile_urls(map, source_id.raw(), completion)
            },
            copy_string_views,
        )
    }

    /// Sets whether a style source stores fetched tiles in persistent storage.
    ///
    /// When `is_volatile` is true, source implementations that fetch tiles do
    /// not store fetched tiles in persistent storage. Other source types retain
    /// the value for inspection without changing their loading behavior.
    ///
    /// The command's finished event reports `Failed` with a not-found status
    /// code when no source has the ID.
    pub fn set_style_source_volatile(
        &self,
        source_id: &str,
        is_volatile: bool,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live and source_id is valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_style_source_volatile(map, source_id.raw(), is_volatile, completion)
        })
    }

    /// Adds a GeoJSON source that loads data from a URL.
    /// `options` are fixed at creation; later data or URL updates keep them.
    pub fn add_geojson_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&GeoJsonSourceOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
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
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_geojson_source_url(
                map,
                source_id.raw(),
                url.raw(),
                options_ptr,
                completion,
            )
        })
    }

    /// Adds a GeoJSON source with prepared inline data.
    ///
    /// The call borrows `data`, and the source adopts the options the data
    /// was prepared with, fixed for the lifetime of the source.
    pub fn add_geojson_source_data(
        &self,
        source_id: &str,
        data: &crate::GeoJsonSourceDataHandle,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live, source_id is valid for this call, and data is a
        // live prepared-data handle the call only borrows.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_geojson_source_data(map, source_id.raw(), data.native(), completion)
        })
    }

    /// Updates one GeoJSON source to load data from a URL.
    ///
    /// The source keeps the options it was added with.
    pub fn set_geojson_source_url(
        &self,
        source_id: &str,
        url: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        let url = maplibre_core::string::string_view(url);
        // SAFETY: map is live and source_id and url are valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_geojson_source_url(map, source_id.raw(), url.raw(), completion)
        })
    }

    /// Updates one GeoJSON source with prepared inline data.
    ///
    /// The call borrows `data`, and the expensive parse and tiling already
    /// happened when the data was prepared, so the install is cheap. The data
    /// must have been prepared with options equal to the options the source
    /// was added with, `cluster_properties` excepted; a mismatch is rejected.
    pub fn set_geojson_source_data(
        &self,
        source_id: &str,
        data: &crate::GeoJsonSourceDataHandle,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live, source_id is valid for this call, and data is a
        // live prepared-data handle the call only borrows.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_geojson_source_data(map, source_id.raw(), data.native(), completion)
        })
    }

    /// Overrides one GeoJSON source's synchronous tiling at runtime.
    ///
    /// While enabled, the source slices requested tiles inline during the
    /// update pass, as if its options had set `synchronous_tiling`; disabling
    /// restores the option the source was added with. The override applies to
    /// update passes after the command commits.
    pub fn set_geojson_source_synchronous_tiling(
        &self,
        source_id: &str,
        enabled: bool,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live and source_id is valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_geojson_source_synchronous_tiling(
                map,
                source_id.raw(),
                enabled,
                completion,
            )
        })
    }

    /// Adds one style layer from a full style-spec layer JSON object.
    pub fn add_style_layer_json(
        &self,
        layer_json: &[u8],
        before_layer_id: Option<&str>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let layer_json = maplibre_core::string::buffer_view(layer_json);
        let before_layer_id = maplibre_core::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live, and layer_json and before_layer_id are
        // explicit-length views valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_style_layer_json(map, layer_json, before_layer_id.raw(), completion)
        })
    }

    /// Adds a hillshade layer for a raster DEM source.
    pub fn add_hillshade_layer(
        &self,
        layer_id: &str,
        source_id: &str,
        before_layer_id: Option<&str>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        let source_id = maplibre_core::string::string_view(source_id);
        let before_layer_id = maplibre_core::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live, and all string views are valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_hillshade_layer(
                map,
                layer_id.raw(),
                source_id.raw(),
                before_layer_id.raw(),
                completion,
            )
        })
    }

    /// Adds a color-relief layer for a raster DEM source.
    pub fn add_color_relief_layer(
        &self,
        layer_id: &str,
        source_id: &str,
        before_layer_id: Option<&str>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        let source_id = maplibre_core::string::string_view(source_id);
        let before_layer_id = maplibre_core::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live, and all string views are valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_color_relief_layer(
                map,
                layer_id.raw(),
                source_id.raw(),
                before_layer_id.raw(),
                completion,
            )
        })
    }

    /// Adds a source-free location indicator layer.
    pub fn add_location_indicator_layer(
        &self,
        layer_id: &str,
        before_layer_id: Option<&str>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        let before_layer_id = maplibre_core::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live, and string views are valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_add_location_indicator_layer(
                map,
                layer_id.raw(),
                before_layer_id.raw(),
                completion,
            )
        })
    }

    /// Sets a location indicator layer location.
    pub fn set_location_indicator_location(
        &self,
        layer_id: &str,
        coordinate: LatLng,
        altitude: f64,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live, layer_id is valid for this call, and coordinate
        // is passed by value.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_location_indicator_location(
                map,
                layer_id.raw(),
                coordinate.to_native(),
                altitude,
                completion,
            )
        })
    }

    /// Sets a location indicator layer bearing in degrees.
    pub fn set_location_indicator_bearing(
        &self,
        layer_id: &str,
        bearing: f64,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id is valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_location_indicator_bearing(map, layer_id.raw(), bearing, completion)
        })
    }

    /// Sets a location indicator layer accuracy radius in meters.
    pub fn set_location_indicator_accuracy_radius(
        &self,
        layer_id: &str,
        radius: f64,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id is valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_location_indicator_accuracy_radius(
                map,
                layer_id.raw(),
                radius,
                completion,
            )
        })
    }

    /// Sets one location indicator image-name property.
    pub fn set_location_indicator_image_name(
        &self,
        layer_id: &str,
        image_kind: LocationIndicatorImageKind,
        image_id: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        let image_id = maplibre_core::string::string_view(image_id);
        // SAFETY: map is live, string views are valid for this call, and
        // image_kind is a valid C enum value.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_location_indicator_image_name(
                map,
                layer_id.raw(),
                image_kind.raw_value(),
                image_id.raw(),
                completion,
            )
        })
    }

    /// Copies one style layer as a full style-spec JSON object.
    pub fn style_layer_json(&self, layer_id: &str) -> Result<NativeFuture<Option<Vec<u8>>>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live for this synchronous submission.
        self.submit_query(
            move |map, completion| unsafe {
                sys::mln_map_get_style_layer_json(map, layer_id.raw(), completion)
            },
            crate::completion::optional_buffer,
        )
    }

    /// Sets the style light from a style-spec light JSON object.
    pub fn set_style_light_json(
        &self,
        light_json: &[u8],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let light_json = maplibre_core::string::buffer_view(light_json);
        // SAFETY: map is live and light_json remains valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_style_light_json(map, light_json, completion)
        })
    }

    /// Sets one style light property.
    pub fn set_style_light_property(
        &self,
        property_name: &str,
        value: &[u8],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let property_name = maplibre_core::string::string_view(property_name);
        let value = maplibre_core::string::buffer_view(value);
        // SAFETY: map is live, and property_name and value remain valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_style_light_property(map, property_name.raw(), value, completion)
        })
    }

    /// Copies one style light property as a style-spec JSON value.
    pub fn style_light_property(
        &self,
        property_name: &str,
    ) -> Result<NativeFuture<Option<Vec<u8>>>> {
        let property_name = maplibre_core::string::string_view(property_name);
        // SAFETY: map is live for this synchronous submission.
        self.submit_query(
            move |map, completion| unsafe {
                sys::mln_map_get_style_light_property(map, property_name.raw(), completion)
            },
            crate::completion::optional_buffer,
        )
    }

    /// Sets the style's global transition options. This replaces the whole
    /// configuration rather than merging, and loading a style replaces it
    /// again, so apply an override after the style loads.
    pub fn set_style_transition_options(
        &self,
        options: &StyleTransitionOptions,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let raw = maplibre_core::style::style_transition_options_to_native(options);
        // SAFETY: map is live and raw is a fully initialized options struct
        // borrowed for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_style_transition_options(map, &raw, completion)
        })
    }

    /// Reads the style's global transition options.
    pub fn style_transition_options(&self) -> Result<NativeFuture<StyleTransitionOptions>> {
        // SAFETY: map is live for this synchronous submission.
        self.submit_query(
            move |map, completion| unsafe {
                sys::mln_map_get_style_transition_options(map, completion)
            },
            |result| {
                let raw = crate::completion::copy_value(result)?;
                Ok(maplibre_core::style::style_transition_options_from_native(
                    &raw,
                ))
            },
        )
    }

    /// Sets one layer style property.
    pub fn set_layer_property(
        &self,
        layer_id: &str,
        property_name: &str,
        value: &[u8],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        let property_name = maplibre_core::string::string_view(property_name);
        let value = maplibre_core::string::buffer_view(value);
        // SAFETY: map is live, and all string and buffer views remain valid for
        // this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_layer_property(
                map,
                layer_id.raw(),
                property_name.raw(),
                value,
                completion,
            )
        })
    }

    /// Copies one layer style property as a style-spec JSON value.
    pub fn layer_property(
        &self,
        layer_id: &str,
        property_name: &str,
    ) -> Result<NativeFuture<Option<Vec<u8>>>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        let property_name = maplibre_core::string::string_view(property_name);
        // SAFETY: map is live for this synchronous submission.
        self.submit_query(
            move |map, completion| unsafe {
                sys::mln_map_get_layer_property(
                    map,
                    layer_id.raw(),
                    property_name.raw(),
                    completion,
                )
            },
            crate::completion::optional_buffer,
        )
    }

    /// Sets or clears one layer filter.
    pub fn set_layer_filter(
        &self,
        layer_id: &str,
        filter: Option<&[u8]>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        let native_filter = filter.map(maplibre_core::string::buffer_view);
        // SAFETY: map is live, layer_id is valid for this call, and the
        // optional filter descriptor is either null or valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_layer_filter(
                map,
                layer_id.raw(),
                native_filter.as_ref().map_or(ptr::null(), ptr::from_ref),
                completion,
            )
        })
    }

    /// Copies one layer filter as a style-spec JSON value.
    pub fn layer_filter(&self, layer_id: &str) -> Result<NativeFuture<Option<Vec<u8>>>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live for this synchronous submission.
        self.submit_query(
            move |map, completion| unsafe {
                sys::mln_map_get_layer_filter(map, layer_id.raw(), completion)
            },
            crate::completion::optional_buffer,
        )
    }

    /// Sets one layer's source-layer ID.
    ///
    /// Layer types that take no source, such as background, are rejected.
    pub fn set_layer_source_layer(
        &self,
        layer_id: &str,
        source_layer: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        let source_layer = maplibre_core::string::string_view(source_layer);
        // SAFETY: map is live and both string views stay valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_layer_source_layer(map, layer_id.raw(), source_layer.raw(), completion)
        })
    }

    /// Copies one layer's source-layer ID, empty when the layer carries none.
    pub fn layer_source_layer(&self, layer_id: &str) -> Result<NativeFuture<String>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live for this synchronous submission.
        self.submit_query(
            move |map, completion| unsafe {
                sys::mln_map_copy_layer_source_layer(map, layer_id.raw(), completion)
            },
            crate::completion::string,
        )
    }

    /// Sets one layer's source ID.
    ///
    /// Layer types that take no source, such as background, are rejected. The
    /// named source need not exist yet.
    pub fn set_layer_source_id(
        &self,
        layer_id: &str,
        source_id: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live and both string views stay valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_layer_source_id(map, layer_id.raw(), source_id.raw(), completion)
        })
    }

    /// Copies one layer's source ID, empty when the layer carries none.
    pub fn layer_source_id(&self, layer_id: &str) -> Result<NativeFuture<String>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live for this synchronous submission.
        self.submit_query(
            move |map, completion| unsafe {
                sys::mln_map_copy_layer_source_id(map, layer_id.raw(), completion)
            },
            crate::completion::string,
        )
    }

    /// Sets the lowest zoom at which one layer draws.
    ///
    /// Pass `f64::NEG_INFINITY` for no lower bound.
    pub fn set_layer_min_zoom(
        &self,
        layer_id: &str,
        min_zoom: f64,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id stays valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_layer_min_zoom(map, layer_id.raw(), min_zoom, completion)
        })
    }

    /// Sets the highest zoom at which one layer draws.
    ///
    /// Pass `f64::INFINITY` for no upper bound.
    pub fn set_layer_max_zoom(
        &self,
        layer_id: &str,
        max_zoom: f64,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id stays valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_layer_max_zoom(map, layer_id.raw(), max_zoom, completion)
        })
    }

    /// Sets whether one layer draws.
    pub fn set_layer_visibility(
        &self,
        layer_id: &str,
        visibility: StyleLayerVisibility,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id stays valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_set_layer_visibility(
                map,
                layer_id.raw(),
                visibility.raw_value(),
                completion,
            )
        })
    }

    /// Copies current style source IDs into owned Rust strings.
    pub fn style_source_ids(&self) -> Result<NativeFuture<Vec<String>>> {
        // SAFETY: map is live for this synchronous submission.
        self.submit_query(
            move |map, completion| unsafe { sys::mln_map_list_style_source_ids(map, completion) },
            copy_string_views,
        )
    }

    /// Copies current style layer IDs into owned Rust strings.
    pub fn style_layer_ids(&self) -> Result<NativeFuture<Vec<String>>> {
        // SAFETY: map is live for this synchronous submission.
        self.submit_query(
            move |map, completion| unsafe { sys::mln_map_list_style_layer_ids(map, completion) },
            copy_string_views,
        )
    }
    /// Removes one style layer by ID.
    ///
    /// The command's finished event reports `Failed` with a not-found status
    /// code when no layer has the ID.
    pub fn remove_style_layer(&self, layer_id: &str) -> Result<NativeFuture<CommandCompletion>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id is valid for this call.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_remove_style_layer(map, layer_id.raw(), completion)
        })
    }

    /// Copies fixed metadata for one style layer.
    ///
    /// The operation resolves the layer's type, zoom range, visibility, and
    /// source IDs together; its take returns `None` when no layer carries
    /// `layer_id`.
    pub fn style_layer_info(&self, layer_id: &str) -> Result<NativeFuture<Option<StyleLayerInfo>>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live for this synchronous submission.
        self.submit_query(
            move |map, completion| unsafe {
                sys::mln_map_get_style_layer_info(map, layer_id.raw(), completion)
            },
            copy_layer_info,
        )
    }

    /// Moves one style layer so it draws immediately below `before_layer_id`,
    /// or to the top of the layer stack when that is `None`.
    pub fn move_style_layer(
        &self,
        layer_id: &str,
        before_layer_id: Option<&str>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let layer_id = maplibre_core::string::string_view(layer_id);
        let before_layer_id = maplibre_core::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live for this synchronous submission.
        self.submit_command(move |map, completion| unsafe {
            sys::mln_map_move_style_layer(map, layer_id.raw(), before_layer_id.raw(), completion)
        })
    }
}

fn copy_string_views(result: &sys::mln_completion_result) -> Result<Vec<String>> {
    crate::completion::copy_slice::<sys::mln_buffer_view>(result)?
        .into_iter()
        .map(|view| {
            // SAFETY: each view is borrowed for this completion callback.
            unsafe { maplibre_core::string::copy_string_view(view) }
        })
        .collect()
}

fn copy_source_info(result: &sys::mln_completion_result) -> Result<Option<SourceInfo>> {
    let Some(raw) = crate::completion::optional_value::<sys::mln_style_source_result>(result)?
    else {
        return Ok(None);
    };
    let attribution = raw
        .info
        .has_attribution
        .then(|| {
            // SAFETY: the view is borrowed for this completion callback.
            unsafe { maplibre_core::string::copy_string_view(raw.attribution) }
        })
        .transpose()?;
    let url = (raw.info.fields & sys::MLN_STYLE_SOURCE_INFO_URL != 0)
        .then(|| {
            // SAFETY: the view is borrowed for this completion callback.
            unsafe { maplibre_core::string::copy_string_view(raw.url) }
        })
        .transpose()?;
    let tiles = if raw.tile_url_count == 0 {
        Vec::new()
    } else {
        if raw.tile_urls.is_null() {
            return Err(crate::Error::new(
                crate::ErrorKind::NativeError,
                None,
                "source completion returned a null tile URL array",
            ));
        }
        // SAFETY: the array is borrowed for this completion callback.
        unsafe { std::slice::from_raw_parts(raw.tile_urls, raw.tile_url_count) }
            .iter()
            .map(|view| {
                // SAFETY: each view is borrowed for this completion callback.
                unsafe { maplibre_core::string::copy_string_view(*view) }
            })
            .collect::<Result<Vec<_>>>()?
    };
    Ok(Some(maplibre_core::style::style_source_info_from_native(
        &raw.info,
        attribution,
        url,
        tiles,
    )))
}

/// Copies a borrowed view that carries its own length, treating an empty view
/// as an absent value.
///
/// # Safety
///
/// `view` must view readable bytes for the duration of this call.
unsafe fn copy_optional_view(view: sys::mln_buffer_view) -> Result<Option<String>> {
    if view.size == 0 {
        return Ok(None);
    }
    // SAFETY: the caller promises the view is readable for this call.
    unsafe { maplibre_core::string::copy_string_view(view) }.map(Some)
}

fn copy_layer_info(result: &sys::mln_completion_result) -> Result<Option<StyleLayerInfo>> {
    let Some(raw) = crate::completion::optional_value::<sys::mln_style_layer_result>(result)?
    else {
        return Ok(None);
    };
    // SAFETY: both views are borrowed for this completion callback. An absent
    // value arrives as an empty view.
    let source_id = unsafe { copy_optional_view(raw.source_id) }?;
    // SAFETY: as above.
    let source_layer = unsafe { copy_optional_view(raw.source_layer) }?;
    // SAFETY: the result's type view is borrowed for this callback.
    unsafe {
        maplibre_core::style::style_layer_info_from_native(&raw.info, source_id, source_layer)
    }
    .map(Some)
}

/// Copies a borrowed stretch array out of a completion value.
///
/// # Safety
///
/// `values` must view `count` readable stretches for the duration of this call.
unsafe fn copy_stretches(
    values: *const sys::mln_image_stretch,
    count: usize,
) -> Result<Vec<ImageStretch>> {
    if count == 0 {
        return Ok(Vec::new());
    }
    if values.is_null() {
        return Err(crate::Error::new(
            crate::ErrorKind::NativeError,
            None,
            "native style image returned a null stretch array",
        ));
    }
    // SAFETY: the caller promises the array is readable for this call.
    Ok(unsafe { std::slice::from_raw_parts(values, count) }
        .iter()
        .map(|value| ImageStretch::new(value.from, value.to))
        .collect())
}

fn copy_style_image_stretches(
    result: &sys::mln_completion_result,
) -> Result<Option<StyleImageStretches>> {
    let Some(raw) =
        crate::completion::optional_value::<sys::mln_style_image_stretches_result>(result)?
    else {
        return Ok(None);
    };
    // SAFETY: both arrays are borrowed for this completion callback.
    let stretch_x = unsafe { copy_stretches(raw.stretch_x, raw.stretch_x_count) }?;
    // SAFETY: as above.
    let stretch_y = unsafe { copy_stretches(raw.stretch_y, raw.stretch_y_count) }?;
    Ok(Some((stretch_x, stretch_y)))
}

fn copy_style_image(result: &sys::mln_completion_result) -> Result<Option<StyleImage>> {
    let Some(raw) = crate::completion::optional_value::<sys::mln_style_image_result>(result)?
    else {
        return Ok(None);
    };
    // SAFETY: pixels are borrowed for this completion callback.
    let pixels = unsafe { maplibre_core::string::copy_string_view_bytes(raw.pixels) }?;
    let info = maplibre_core::values::style_image_info_from_native(&raw.info);
    let mut image = maplibre_core::style::style_image_from_copied_premultiplied_rgba8(
        info,
        pixels,
        raw.pixels.size,
    )?;
    // SAFETY: both arrays are borrowed for this completion callback.
    image.stretch_x = unsafe { copy_stretches(raw.stretch_x, raw.stretch_x_count) }?;
    // SAFETY: as above.
    image.stretch_y = unsafe { copy_stretches(raw.stretch_y, raw.stretch_y_count) }?;
    image.content = info.content;
    image.text_fit_width = info.text_fit_width;
    image.text_fit_height = info.text_fit_height;
    Ok(Some(image))
}
