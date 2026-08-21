use std::ptr;

pub(crate) use maplibre_core::style::{
    GeoJsonSourceOptionsNativeExt, NativeGeoJsonSourceOptions, NativeStyleImageOptions,
    NativeTileSourceOptions, NativeTileUrls, StyleImageOptionsNativeExt,
    TileSourceOptionsNativeExt,
};
pub use maplibre_core::{
    GeoJsonSourceOptions, ImageContent, ImageStretch, LocationIndicatorImageKind,
    RasterDemEncoding, SourceInfo, SourceType, StyleImage, StyleImageInfo, StyleImageOptions,
    StyleImageTextFit, StyleLayerVisibility, StyleTransitionOptions, TileJsonInfo, TileScheme,
    TileSourceOptions, VectorTileEncoding,
};
use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_core::ptr::const_ptr_or_null;
use maplibre_native_ffi_core::query::FeatureStateSelectorNativeExt;
use maplibre_native_ffi_core::values::lat_lngs_to_native;
use maplibre_native_ffi_sys as sys;

use crate::custom_geometry::{CanonicalTileId, CustomGeometrySourceState};
use crate::custom_mvt_vector::CustomMvtVectorSourceState;
use crate::render::PremultipliedRgba8Image;
use crate::values::NativeValue;
use crate::{
    CustomGeometrySourceOptions, CustomMvtVectorSourceOptions, Error, ErrorKind,
    FeatureStateSelector, LatLng, LatLngBounds, Result,
};

impl super::MapHandle {
    /// Loads a style URL through MapLibre Native style APIs.
    ///
    /// Loading is asynchronous: a style that fails to fetch or parse still
    /// returns `Ok` here and reports through a later loading-failed runtime
    /// event. Watch the event stream for the load outcome.
    pub fn set_style_url(&self, url: &str) -> Result<()> {
        let map = self.inner.native()?;
        let url = maplibre_core::string::c_string(url)?;
        // SAFETY: map is live and url is a NUL-terminated UTF-8 string the C
        // API consumes before returning.
        maplibre_core::check(unsafe { sys::mln_map_set_style_url(map, url.as_ptr()) })?;
        Ok(())
    }

    /// Loads inline style JSON through MapLibre Native style APIs.
    ///
    /// A parse failure is reported twice: this call returns the error, and the
    /// same message arrives as a loading-failed runtime event.
    pub fn set_style_json(&self, json: &[u8]) -> Result<()> {
        let map = self.inner.native()?;
        let json = maplibre_core::string::buffer_view(json);
        // SAFETY: map is live and json is valid for the call. Style replacement
        // completes before a successful return, so the C API has already
        // released the callback state of the sources this load dropped.
        maplibre_core::check(unsafe { sys::mln_map_set_style_json(map, json) })
    }

    /// Sets per-feature state on this map.
    pub fn set_feature_state(&self, selector: &FeatureStateSelector, state: &[u8]) -> Result<()> {
        let map = self.inner.native()?;
        let selector = selector.to_native();
        let state = maplibre_core::string::buffer_view(state);
        // SAFETY: map is live and all borrowed storage remains valid for the call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_feature_state(map, selector.as_ptr(), state)
        })
    }

    /// Copies per-feature state from this map.
    pub fn get_feature_state(&self, selector: &FeatureStateSelector) -> Result<Vec<u8>> {
        let map = self.inner.native()?;
        let selector = selector.to_native();
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        // SAFETY: map is live, selector storage remains valid, and out is writable.
        maplibre_core::check(unsafe {
            sys::mln_map_get_feature_state(map, selector.as_ptr(), out.as_mut_ptr())
        })?;
        // SAFETY: Success transfers the owned buffer to this call.
        unsafe { maplibre_core::string::copy_owned_buffer(out.get()) }
    }

    /// Removes per-feature state selected on this map.
    pub fn remove_feature_state(&self, selector: &FeatureStateSelector) -> Result<()> {
        let map = self.inner.native()?;
        let selector = selector.to_native();
        // SAFETY: map is live and selector storage remains valid for the call.
        maplibre_core::check(unsafe { sys::mln_map_remove_feature_state(map, selector.as_ptr()) })
    }

    /// Copies the style document this map's style was last parsed from: the
    /// string given to [`Self::set_style_json`] or the body fetched for
    /// [`Self::set_style_url`], byte for byte. Runtime mutations do not change
    /// it. An empty buffer means no document has been parsed.
    pub fn loaded_style_json(&self) -> Result<Vec<u8>> {
        let map = self.inner.native()?;
        // SAFETY: map is live, and each call writes only through the pointers
        // it is given.
        unsafe {
            copy_bytes(|text, capacity, out_size| {
                sys::mln_map_copy_loaded_style_json(map, text, capacity, out_size)
            })
        }
    }

    /// Copies the URL this map's style was last requested from.
    ///
    /// [`Self::set_style_url`] records the URL when the request is made, before
    /// the response arrives, and [`Self::set_style_json`] clears it, so this can
    /// disagree with [`Self::loaded_style_json`] while a load is in flight. An
    /// empty string means no URL bytes are available.
    pub fn style_url(&self) -> Result<String> {
        let map = self.inner.native()?;
        // SAFETY: map is live, and each call writes only through the pointers
        // it is given.
        unsafe {
            copy_text(|url, capacity, out_size| {
                sys::mln_map_copy_style_url(map, url, capacity, out_size)
            })
        }
    }

    /// Adds a custom geometry source to the current style.
    ///
    /// The callback state is scoped to this map's current style. The C API
    /// frees it once it stops referencing it, whether the source is removed,
    /// dropped by a style load, or retired with the map. Native may invoke
    /// callbacks from worker threads, so queue owner-thread work before calling
    /// map APIs.
    pub fn add_custom_geometry_source(
        &self,
        source_id: &str,
        options: CustomGeometrySourceOptions,
    ) -> Result<()> {
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
            sys::mln_map_add_custom_geometry_source(map, source_id_view.raw(), &descriptor)
        };
        if let Err(error) = maplibre_core::check(status) {
            // SAFETY: A rejected add releases nothing, so this box is still
            // this call's to free.
            drop(unsafe { Box::from_raw(state) });
            return Err(error);
        }
        Ok(())
    }

    /// Sets custom geometry source data for one canonical tile.
    pub fn set_custom_geometry_source_tile_data(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        data: &[u8],
    ) -> Result<()> {
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
            )
        })
    }

    /// Invalidates custom geometry source data for one canonical tile.
    pub fn invalidate_custom_geometry_source_tile(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
    ) -> Result<()> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live, source_id is valid for this call, and tile_id is
        // passed by value.
        maplibre_core::check(unsafe {
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
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live, source_id is valid for this call, and bounds is
        // passed by value.
        maplibre_core::check(unsafe {
            sys::mln_map_invalidate_custom_geometry_source_region(
                map,
                source_id.raw(),
                bounds.to_native(),
            )
        })
    }

    /// Adds a custom MVT vector source to the current style.
    ///
    /// The callback state is scoped to this map's current style. The C API
    /// frees it once it stops referencing it, whether the source is removed,
    /// dropped by a style load, or retired with the map. Native may invoke
    /// callbacks from worker threads, so queue owner-thread work before calling
    /// map APIs.
    pub fn add_custom_mvt_vector_source(
        &self,
        source_id: &str,
        options: CustomMvtVectorSourceOptions,
    ) -> Result<()> {
        let map = self.inner.native()?;
        let source_id_view = maplibre_core::string::string_view(source_id);
        let state = CustomMvtVectorSourceState::new(options);
        let descriptor = state.descriptor();
        // The descriptor's release callback frees this box, so the C API owns
        // the callback state from a successful add onwards.
        let state = Box::into_raw(state);
        // SAFETY: map is live, source_id_view is valid for this call, and
        // descriptor names callback state that lives until the release callback.
        let status = unsafe {
            sys::mln_map_add_custom_mvt_vector_source(map, source_id_view.raw(), &descriptor)
        };
        if let Err(error) = maplibre_core::check(status) {
            // SAFETY: A rejected add releases nothing, so this box is still
            // this call's to free.
            drop(unsafe { Box::from_raw(state) });
            return Err(error);
        }
        Ok(())
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
    ) -> Result<()> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let data = maplibre_core::string::buffer_view(data);
        // SAFETY: map is live, source_id is valid for this call, tile_id is
        // passed by value, and data remains valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_custom_mvt_vector_source_tile_data(
                map,
                source_id.raw(),
                tile_id.to_native(),
                data,
            )
        })
    }

    /// Reports a custom MVT vector source error for one canonical tile.
    pub fn set_custom_mvt_vector_source_tile_error(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        message: &str,
    ) -> Result<()> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let message = maplibre_core::string::string_view(message);
        // SAFETY: map is live, source_id and message are valid for this call,
        // and tile_id is passed by value.
        maplibre_core::check(unsafe {
            sys::mln_map_set_custom_mvt_vector_source_tile_error(
                map,
                source_id.raw(),
                tile_id.to_native(),
                message.raw(),
            )
        })
    }

    /// Invalidates custom MVT vector source data for one canonical tile.
    pub fn invalidate_custom_mvt_vector_source_tile(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
    ) -> Result<()> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live, source_id is valid for this call, and tile_id is
        // passed by value.
        maplibre_core::check(unsafe {
            sys::mln_map_invalidate_custom_mvt_vector_source_tile(
                map,
                source_id.raw(),
                tile_id.to_native(),
            )
        })
    }

    /// Adds one style source from a style-spec source JSON object.
    pub fn add_style_source_json(&self, source_id: &str, source_json: &[u8]) -> Result<()> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let source_json = maplibre_core::string::buffer_view(source_json);
        // SAFETY: map is live, source_id and source_json are explicit-length
        // views valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_add_style_source_json(map, source_id.raw(), source_json)
        })
    }

    /// Adds a vector source with a TileJSON URL.
    pub fn add_vector_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&TileSourceOptions>,
    ) -> Result<()> {
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
            )
        })
    }

    /// Updates an image source to load its image from a URL.
    pub fn set_image_source_url(&self, source_id: &str, url: &str) -> Result<()> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let url = maplibre_core::string::string_view(url);
        // SAFETY: map is live, and source_id and url are explicit-length views
        // valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_image_source_url(map, source_id.raw(), url.raw())
        })
    }

    /// Updates an image source with inline premultiplied RGBA8 pixels.
    pub fn set_image_source_image(
        &self,
        source_id: &str,
        image: &PremultipliedRgba8Image,
    ) -> Result<()> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let image = maplibre_core::values::premultiplied_rgba8_image_to_native(image);
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and image points into the borrowed Rust image for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_image_source_image(map, source_id.raw(), &image)
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
    ) -> Result<()> {
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
            )
        })
    }

    /// Copies image source coordinates into owned Rust values.
    pub fn image_source_coordinates(&self, source_id: &str) -> Result<Option<[LatLng; 4]>> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let mut coordinates = [sys::mln_lat_lng {
            latitude: 0.0,
            longitude: 0.0,
        }; 4];
        let mut coordinate_count = 0;
        let mut found = false;
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, coordinates has capacity for four native coordinates, and
        // output pointers refer to writable storage.
        maplibre_core::check(unsafe {
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
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let mut removed = false;
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and removed points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_remove_style_source(map, source_id.raw(), &mut removed)
        })?;
        Ok(removed)
    }

    /// Reports whether a style source ID exists.
    pub fn style_source_exists(&self, source_id: &str) -> Result<bool> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let mut exists = false;
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and exists points to writable storage.
        maplibre_core::check(unsafe {
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
            sys::mln_map_set_style_image(map, image_id.raw(), &image, options_ptr)
        })
    }

    /// Removes one runtime style image by ID.
    ///
    /// Returns whether an image existed and was removed.
    pub fn remove_style_image(&self, image_id: &str) -> Result<bool> {
        let map = self.inner.native()?;
        let image_id = maplibre_core::string::string_view(image_id);
        let mut removed = false;
        // SAFETY: map is live, image_id is an explicit-length view valid for
        // this call, and removed points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_remove_style_image(map, image_id.raw(), &mut removed)
        })?;
        Ok(removed)
    }

    /// Reports whether a runtime style image ID exists.
    pub fn style_image_exists(&self, image_id: &str) -> Result<bool> {
        let map = self.inner.native()?;
        let image_id = maplibre_core::string::string_view(image_id);
        let mut exists = false;
        // SAFETY: map is live, image_id is an explicit-length view valid for
        // this call, and exists points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_style_image_exists(map, image_id.raw(), &mut exists)
        })?;
        Ok(exists)
    }

    /// Copies fixed metadata for one runtime style image.
    pub fn style_image_info(&self, image_id: &str) -> Result<Option<StyleImageInfo>> {
        let map = self.inner.native()?;
        let image_id = maplibre_core::string::string_view(image_id);
        let mut info = maplibre_core::style::empty_style_image_info();
        let mut found = false;
        // SAFETY: map is live, image_id is an explicit-length view valid for
        // this call, info has its ABI size initialized, and found points to
        // writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_image_info(map, image_id.raw(), &mut info, &mut found)
        })?;
        Ok(found.then(|| maplibre_core::values::style_image_info_from_native(&info)))
    }

    /// Copies one runtime style image into owned tightly packed premultiplied RGBA8 pixels.
    pub fn copy_style_image_premultiplied_rgba8(
        &self,
        image_id: &str,
    ) -> Result<Option<StyleImage>> {
        let map = self.inner.native()?;
        let image_id = maplibre_core::string::string_view(image_id);
        let mut raw_info = maplibre_core::style::empty_style_image_info();
        let mut info_found = false;
        // SAFETY: map is live, image_id is an explicit-length view valid for
        // this call, raw_info has its ABI size initialized, and info_found
        // points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_image_info(map, image_id.raw(), &mut raw_info, &mut info_found)
        })?;
        if !info_found {
            return Ok(None);
        }
        let info = maplibre_core::values::style_image_info_from_native(&raw_info);

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
        maplibre_core::check(unsafe {
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
        maplibre_core::style::style_image_from_copied_premultiplied_rgba8(info, data, copied_size)
            .map(Some)
    }

    /// Gets one style source type.
    pub fn style_source_type(&self, source_id: &str) -> Result<Option<SourceType>> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let mut raw_source_type = sys::MLN_STYLE_SOURCE_TYPE_UNKNOWN;
        let mut found = false;
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and output pointers refer to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_source_type(
                map,
                source_id.raw(),
                &mut raw_source_type,
                &mut found,
            )
        })?;
        Ok(found.then(|| SourceType::from_raw(raw_source_type)))
    }

    /// Copies retained metadata for one style source.
    pub fn style_source_info(&self, source_id: &str) -> Result<Option<SourceInfo>> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let mut info = maplibre_core::style::empty_style_source_info();
        let mut found = false;
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, info has its ABI size initialized, and found points to
        // writable storage.
        maplibre_core::check(unsafe {
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

        let url = if info.fields & sys::MLN_STYLE_SOURCE_INFO_URL != 0 {
            match self.copy_style_source_url(map, source_id.raw(), info.url_size)? {
                Some(url) => Some(url),
                None => return Ok(None),
            }
        } else {
            None
        };

        let tiles = if info.fields & sys::MLN_STYLE_SOURCE_INFO_TILEJSON != 0 {
            match self.copy_style_source_tile_urls(map, source_id.raw())? {
                Some(tiles) => tiles,
                None => return Ok(None),
            }
        } else {
            Vec::new()
        };

        Ok(Some(maplibre_core::style::style_source_info_from_native(
            &info,
            attribution,
            url,
            tiles,
        )))
    }

    fn copy_style_source_attribution(
        &self,
        map: sys::mln_map,
        source_id: sys::mln_buffer_view,
        attribution_size: usize,
    ) -> Result<Option<String>> {
        if attribution_size == 0 {
            let mut copied_size = 0;
            let mut found = false;
            // SAFETY: map is live, source_id remains valid for this call,
            // capacity is zero so the output buffer may be null, and output
            // pointers refer to writable storage.
            maplibre_core::check(unsafe {
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
        maplibre_core::check(unsafe {
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

    fn copy_style_source_url(
        &self,
        map: sys::mln_map,
        source_id: sys::mln_buffer_view,
        url_size: usize,
    ) -> Result<Option<String>> {
        let mut buffer = vec![0u8; url_size];
        let mut copied_size = 0;
        let mut found = false;
        // SAFETY: map and source_id remain live for this call, the buffer is
        // writable for url_size bytes or null-equivalent when empty, and the
        // output pointers refer to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_copy_style_source_url(
                map,
                source_id,
                if buffer.is_empty() {
                    ptr::null_mut()
                } else {
                    buffer.as_mut_ptr().cast()
                },
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
                "native style source URL size exceeded caller buffer",
            ));
        }
        buffer.truncate(copied_size);
        String::from_utf8(buffer).map(Some).map_err(|error| {
            Error::invalid_argument(format!(
                "native style source URL was not valid UTF-8: {error}"
            ))
        })
    }

    fn copy_style_source_tile_urls(
        &self,
        map: sys::mln_map,
        source_id: sys::mln_buffer_view,
    ) -> Result<Option<Vec<String>>> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_style_string_list>::new();
        let mut found = false;
        // SAFETY: map and source_id remain live for this call, out is a
        // null-initialized output handle, and found points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_source_tile_urls(map, source_id, out.as_mut_ptr(), &mut found)
        })?;
        if !found {
            return Ok(None);
        }
        // SAFETY: A found source returns an owned style string list; core
        // copies every borrowed view and releases the list on all paths.
        unsafe {
            maplibre_core::style::copy_style_string_list(out.into_live("mln_style_string_list")?)
                .map(Some)
        }
    }

    /// Adds a GeoJSON source that loads data from a URL.
    /// `options` are fixed at creation; later data or URL updates keep them.
    pub fn add_geojson_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&GeoJsonSourceOptions>,
    ) -> Result<()> {
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
            sys::mln_map_add_geojson_source_url(map, source_id.raw(), url.raw(), options_ptr)
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
    ) -> Result<()> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live, source_id is valid for this call, and data is a
        // live prepared-data handle the call only borrows.
        maplibre_core::check(unsafe {
            sys::mln_map_add_geojson_source_data(map, source_id.raw(), data.native())
        })
    }

    /// Updates one GeoJSON source to load data from a URL.
    ///
    /// The source keeps the options it was added with.
    pub fn set_geojson_source_url(&self, source_id: &str, url: &str) -> Result<()> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let url = maplibre_core::string::string_view(url);
        // SAFETY: map is live and source_id and url are valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_geojson_source_url(map, source_id.raw(), url.raw())
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
    ) -> Result<()> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live, source_id is valid for this call, and data is a
        // live prepared-data handle the call only borrows.
        maplibre_core::check(unsafe {
            sys::mln_map_set_geojson_source_data(map, source_id.raw(), data.native())
        })
    }

    /// Overrides one GeoJSON source's synchronous tiling at runtime.
    ///
    /// While enabled, the source slices requested tiles inline during the
    /// update pass, as if its options had set `synchronous_tiling`; disabling
    /// restores the option the source was added with. The override applies to
    /// update passes after this call returns.
    pub fn set_geojson_source_synchronous_tiling(
        &self,
        source_id: &str,
        enabled: bool,
    ) -> Result<()> {
        let map = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live and source_id is valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_geojson_source_synchronous_tiling(map, source_id.raw(), enabled)
        })
    }

    /// Adds one style layer from a full style-spec layer JSON object.
    pub fn add_style_layer_json(
        &self,
        layer_json: &[u8],
        before_layer_id: Option<&str>,
    ) -> Result<()> {
        let map = self.inner.native()?;
        let layer_json = maplibre_core::string::buffer_view(layer_json);
        let before_layer_id = maplibre_core::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live, and layer_json and before_layer_id are
        // explicit-length views valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_add_style_layer_json(map, layer_json, before_layer_id.raw())
        })
    }

    /// Adds a hillshade layer for a raster DEM source.
    pub fn add_hillshade_layer(
        &self,
        layer_id: &str,
        source_id: &str,
        before_layer_id: Option<&str>,
    ) -> Result<()> {
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
            )
        })
    }

    /// Adds a source-free location indicator layer.
    pub fn add_location_indicator_layer(
        &self,
        layer_id: &str,
        before_layer_id: Option<&str>,
    ) -> Result<()> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let before_layer_id = maplibre_core::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live, and string views are valid for this call.
        maplibre_core::check(unsafe {
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
            )
        })
    }

    /// Sets a location indicator layer bearing in degrees.
    pub fn set_location_indicator_bearing(&self, layer_id: &str, bearing: f64) -> Result<()> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id is valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_location_indicator_bearing(map, layer_id.raw(), bearing)
        })
    }

    /// Sets a location indicator layer accuracy radius in meters.
    pub fn set_location_indicator_accuracy_radius(
        &self,
        layer_id: &str,
        radius: f64,
    ) -> Result<()> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id is valid for this call.
        maplibre_core::check(unsafe {
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
            )
        })
    }

    /// Copies one style layer as a full style-spec JSON object.
    pub fn style_layer_json(&self, layer_id: &str) -> Result<Option<Vec<u8>>> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        let mut found = false;
        // SAFETY: map is live, layer_id is valid for this call, out is a
        // null-initialized out-pointer, and found points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_layer_json(map, layer_id.raw(), out.as_mut_ptr(), &mut found)
        })?;
        if !found {
            return Ok(None);
        }
        // SAFETY: Success transfers the owned buffer to this call.
        unsafe { maplibre_core::string::copy_owned_buffer(out.get()) }.map(Some)
    }

    /// Sets the style light from a style-spec light JSON object.
    pub fn set_style_light_json(&self, light_json: &[u8]) -> Result<()> {
        let map = self.inner.native()?;
        let light_json = maplibre_core::string::buffer_view(light_json);
        // SAFETY: map is live and light_json remains valid for this call.
        maplibre_core::check(unsafe { sys::mln_map_set_style_light_json(map, light_json) })
    }

    /// Sets one style light property.
    pub fn set_style_light_property(&self, property_name: &str, value: &[u8]) -> Result<()> {
        let map = self.inner.native()?;
        let property_name = maplibre_core::string::string_view(property_name);
        let value = maplibre_core::string::buffer_view(value);
        // SAFETY: map is live, and property_name and value remain valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_style_light_property(map, property_name.raw(), value)
        })
    }

    /// Copies one style light property as a style-spec JSON value.
    pub fn style_light_property(&self, property_name: &str) -> Result<Option<Vec<u8>>> {
        let map = self.inner.native()?;
        let property_name = maplibre_core::string::string_view(property_name);
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        // SAFETY: map is live, property_name is valid for this call, and out is
        // a null-initialized out-pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_light_property(map, property_name.raw(), out.as_mut_ptr())
        })?;
        let Some(buffer) = out.into_option() else {
            return Ok(None);
        };
        // SAFETY: Success transfers the owned buffer to this call.
        unsafe { maplibre_core::string::copy_owned_buffer(buffer) }.map(Some)
    }

    /// Sets the style's global transition options. This replaces the whole
    /// configuration rather than merging, and loading a style replaces it
    /// again, so apply an override after the style loads.
    pub fn set_style_transition_options(&self, options: &StyleTransitionOptions) -> Result<()> {
        let map = self.inner.native()?;
        let raw = maplibre_core::style::style_transition_options_to_native(options);
        // SAFETY: map is live and raw is a fully initialized options struct
        // borrowed for this call.
        maplibre_core::check(unsafe { sys::mln_map_set_style_transition_options(map, &raw) })
    }

    /// Reads the style's global transition options.
    pub fn style_transition_options(&self) -> Result<StyleTransitionOptions> {
        let map = self.inner.native()?;
        let mut raw = maplibre_core::style::empty_style_transition_options();
        // SAFETY: map is live and raw has its ABI size initialized.
        maplibre_core::check(unsafe { sys::mln_map_get_style_transition_options(map, &mut raw) })?;
        Ok(maplibre_core::style::style_transition_options_from_native(
            &raw,
        ))
    }

    /// Sets one layer style property.
    pub fn set_layer_property(
        &self,
        layer_id: &str,
        property_name: &str,
        value: &[u8],
    ) -> Result<()> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let property_name = maplibre_core::string::string_view(property_name);
        let value = maplibre_core::string::buffer_view(value);
        // SAFETY: map is live, and all string and buffer views remain valid for
        // this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_property(map, layer_id.raw(), property_name.raw(), value)
        })
    }

    /// Copies one layer style property as a style-spec JSON value.
    pub fn layer_property(&self, layer_id: &str, property_name: &str) -> Result<Option<Vec<u8>>> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let property_name = maplibre_core::string::string_view(property_name);
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        // SAFETY: map is live, string views are valid for this call, and out is
        // a null-initialized out-pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_layer_property(
                map,
                layer_id.raw(),
                property_name.raw(),
                out.as_mut_ptr(),
            )
        })?;
        let Some(buffer) = out.into_option() else {
            return Ok(None);
        };
        // SAFETY: Success transfers the owned buffer to this call.
        unsafe { maplibre_core::string::copy_owned_buffer(buffer) }.map(Some)
    }

    /// Sets or clears one layer filter.
    pub fn set_layer_filter(&self, layer_id: &str, filter: Option<&[u8]>) -> Result<()> {
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
            )
        })
    }

    /// Copies one layer filter as a style-spec JSON value.
    pub fn layer_filter(&self, layer_id: &str) -> Result<Option<Vec<u8>>> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        // SAFETY: map is live, layer_id is valid for this call, and out is a
        // null-initialized out-pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_layer_filter(map, layer_id.raw(), out.as_mut_ptr())
        })?;
        let Some(buffer) = out.into_option() else {
            return Ok(None);
        };
        // SAFETY: Success transfers the owned buffer to this call.
        unsafe { maplibre_core::string::copy_owned_buffer(buffer) }.map(Some)
    }

    /// Copies one runtime style image's stretchable intervals.
    ///
    /// Returns `None` when no image carries `image_id`.
    pub fn style_image_stretches(
        &self,
        image_id: &str,
    ) -> Result<Option<(Vec<ImageStretch>, Vec<ImageStretch>)>> {
        let map = self.inner.native()?;
        let image_id = maplibre_core::string::string_view(image_id);
        let mut x_count = 0;
        let mut y_count = 0;
        let mut found = false;
        // SAFETY: map is live, image_id stays valid for this call, both arrays
        // are null with zero capacity so this is a size probe, and the output
        // pointers refer to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_copy_style_image_stretches(
                map,
                image_id.raw(),
                ptr::null_mut(),
                0,
                &mut x_count,
                ptr::null_mut(),
                0,
                &mut y_count,
                &mut found,
            )
        })?;
        if !found {
            return Ok(None);
        }

        let mut stretch_x = vec![sys::mln_image_stretch { from: 0.0, to: 0.0 }; x_count];
        let mut stretch_y = vec![sys::mln_image_stretch { from: 0.0, to: 0.0 }; y_count];
        // SAFETY: each buffer is writable for its reported count, and the output
        // pointers refer to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_copy_style_image_stretches(
                map,
                image_id.raw(),
                stretch_x.as_mut_ptr(),
                stretch_x.len(),
                &mut x_count,
                stretch_y.as_mut_ptr(),
                stretch_y.len(),
                &mut y_count,
                &mut found,
            )
        })?;
        let to_public = |stretches: &[sys::mln_image_stretch]| -> Vec<ImageStretch> {
            stretches
                .iter()
                .map(|stretch| ImageStretch::new(stretch.from, stretch.to))
                .collect()
        };
        Ok(Some((to_public(&stretch_x), to_public(&stretch_y))))
    }

    /// Sets one layer's source-layer ID.
    ///
    /// Layer types that take no source, such as background, are rejected.
    pub fn set_layer_source_layer(&self, layer_id: &str, source_layer: &str) -> Result<()> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let source_layer = maplibre_core::string::string_view(source_layer);
        // SAFETY: map is live and both string views stay valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_source_layer(map, layer_id.raw(), source_layer.raw())
        })
    }

    /// Copies one layer's source-layer ID, empty when the layer carries none.
    pub fn layer_source_layer(&self, layer_id: &str) -> Result<String> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live, layer_id stays valid for both calls, and each
        // call writes only through the pointers it is given.
        unsafe {
            copy_text(|text, capacity, out_size| {
                sys::mln_map_copy_layer_source_layer(map, layer_id.raw(), text, capacity, out_size)
            })
        }
    }

    /// Sets one layer's source ID.
    ///
    /// Layer types that take no source, such as background, are rejected. The
    /// named source need not exist yet.
    pub fn set_layer_source_id(&self, layer_id: &str, source_id: &str) -> Result<()> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let source_id = maplibre_core::string::string_view(source_id);
        // SAFETY: map is live and both string views stay valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_source_id(map, layer_id.raw(), source_id.raw())
        })
    }

    /// Copies one layer's source ID, empty when the layer carries none.
    pub fn layer_source_id(&self, layer_id: &str) -> Result<String> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live, layer_id stays valid for both calls, and each
        // call writes only through the pointers it is given.
        unsafe {
            copy_text(|text, capacity, out_size| {
                sys::mln_map_copy_layer_source_id(map, layer_id.raw(), text, capacity, out_size)
            })
        }
    }

    /// Sets the lowest zoom at which one layer draws.
    ///
    /// Pass `f64::NEG_INFINITY` for no lower bound.
    pub fn set_layer_min_zoom(&self, layer_id: &str, min_zoom: f64) -> Result<()> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id stays valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_min_zoom(map, layer_id.raw(), min_zoom)
        })
    }

    /// Reads the lowest zoom at which one layer draws.
    ///
    /// A layer with no lower bound reports `f64::NEG_INFINITY`.
    pub fn layer_min_zoom(&self, layer_id: &str) -> Result<f64> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let mut min_zoom = 0.0;
        // SAFETY: map is live, layer_id stays valid for this call, and min_zoom
        // is writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_get_layer_min_zoom(map, layer_id.raw(), &mut min_zoom)
        })?;
        Ok(min_zoom)
    }

    /// Sets the highest zoom at which one layer draws.
    ///
    /// Pass `f64::INFINITY` for no upper bound.
    pub fn set_layer_max_zoom(&self, layer_id: &str, max_zoom: f64) -> Result<()> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id stays valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_max_zoom(map, layer_id.raw(), max_zoom)
        })
    }

    /// Reads the highest zoom at which one layer draws.
    ///
    /// A layer with no upper bound reports `f64::INFINITY`.
    pub fn layer_max_zoom(&self, layer_id: &str) -> Result<f64> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let mut max_zoom = 0.0;
        // SAFETY: map is live, layer_id stays valid for this call, and max_zoom
        // is writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_get_layer_max_zoom(map, layer_id.raw(), &mut max_zoom)
        })?;
        Ok(max_zoom)
    }

    /// Sets whether one layer draws.
    pub fn set_layer_visibility(
        &self,
        layer_id: &str,
        visibility: StyleLayerVisibility,
    ) -> Result<()> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id stays valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_visibility(map, layer_id.raw(), visibility.raw_value())
        })
    }

    /// Reads whether one layer draws.
    pub fn layer_visibility(&self, layer_id: &str) -> Result<StyleLayerVisibility> {
        let map = self.inner.native()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let mut visibility = 0;
        // SAFETY: map is live, layer_id stays valid for this call, and
        // visibility is writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_get_layer_visibility(map, layer_id.raw(), &mut visibility)
        })?;
        Ok(StyleLayerVisibility::from_raw(visibility))
    }

    /// Copies current style source IDs into owned Rust strings.
    pub fn style_source_ids(&self) -> Result<Vec<String>> {
        let map = self.inner.native()?;
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_style_id_list>::new();
        // SAFETY: map is live and out is a null-initialized out-pointer owned by
        // this call. On success the returned handle is wrapped and destroyed by
        // the copying helper below.
        maplibre_core::check(unsafe { sys::mln_map_list_style_source_ids(map, out.as_mut_ptr()) })?;
        // SAFETY: On success, the C API returns an owned style ID list handle;
        // core copies and releases it.
        unsafe { maplibre_core::style::copy_style_id_list(out.into_live("mln_style_id_list")?) }
    }

    /// Copies current style layer IDs into owned Rust strings.
    pub fn style_layer_ids(&self) -> Result<Vec<String>> {
        let map = self.inner.native()?;
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_style_id_list>::new();
        // SAFETY: map is live and out is a null-initialized out-pointer owned by
        // this call. On success the returned handle is wrapped and destroyed by
        // the copying helper below.
        maplibre_core::check(unsafe { sys::mln_map_list_style_layer_ids(map, out.as_mut_ptr()) })?;
        // SAFETY: On success, the C API returns an owned style ID list handle;
        // core copies and releases it.
        unsafe { maplibre_core::style::copy_style_id_list(out.into_live("mln_style_id_list")?) }
    }
}

/// Probes the required byte length, then copies the text into an owned `String`.
///
/// # Safety
///
/// `copy` must forward its arguments to a C entry point that writes at most
/// `capacity` bytes through the text pointer and the required length through the
/// size pointer.
unsafe fn copy_text(
    copy: impl Fn(*mut std::os::raw::c_char, usize, *mut usize) -> sys::mln_status,
) -> Result<String> {
    let mut required = 0;
    maplibre_core::check(copy(ptr::null_mut(), 0, &mut required))?;
    if required == 0 {
        return Ok(String::new());
    }

    let mut buffer = vec![0u8; required];
    let mut copied = 0;
    maplibre_core::check(copy(buffer.as_mut_ptr().cast(), buffer.len(), &mut copied))?;
    if copied > buffer.len() {
        return Err(Error::new(
            ErrorKind::NativeError,
            None,
            "native text size exceeded caller buffer",
        ));
    }
    buffer.truncate(copied);
    String::from_utf8(buffer).map_err(|_| {
        Error::new(
            ErrorKind::NativeError,
            None,
            "native text was not valid UTF-8",
        )
    })
}

/// Probes the required byte length, then copies the bytes into owned storage.
///
/// # Safety
///
/// `copy` must write at most `capacity` bytes and report the required length
/// through the size pointer.
unsafe fn copy_bytes(
    copy: impl Fn(*mut u8, usize, *mut usize) -> sys::mln_status,
) -> Result<Vec<u8>> {
    let mut required = 0;
    maplibre_core::check(copy(ptr::null_mut(), 0, &mut required))?;
    if required == 0 {
        return Ok(Vec::new());
    }

    let mut buffer = vec![0u8; required];
    let mut copied = 0;
    maplibre_core::check(copy(buffer.as_mut_ptr(), buffer.len(), &mut copied))?;
    if copied > buffer.len() {
        return Err(Error::new(
            ErrorKind::NativeError,
            None,
            "native byte size exceeded caller buffer",
        ));
    }
    buffer.truncate(copied);
    Ok(buffer)
}
