use std::ptr;

pub(crate) use maplibre_core::style::{
    NativeTileSourceOptions, NativeTileUrls, StyleImageOptionsNativeExt, TileSourceOptionsNativeExt,
};
pub use maplibre_core::{
    LocationIndicatorImageKind, RasterDemEncoding, SourceInfo, SourceType, StyleImage,
    StyleImageInfo, StyleImageOptions, TileScheme, TileSourceOptions, VectorTileEncoding,
};
use maplibre_native_core as maplibre_core;
use maplibre_native_core::ptr::const_ptr_or_null;
use maplibre_native_core::values::lat_lngs_to_native;
use maplibre_native_sys as sys;

use crate::custom_geometry::{CanonicalTileId, CustomGeometrySourceState};
use crate::geojson::GeoJsonNativeExt;
use crate::json::JsonValueNativeExt;
use crate::render::PremultipliedRgba8Image;
use crate::values::NativeValue;
use crate::{
    CustomGeometrySourceOptions, Error, ErrorKind, GeoJson, JsonValue, LatLng, LatLngBounds, Result,
};

impl super::MapHandle {
    /// Loads a style URL through MapLibre Native style APIs.
    ///
    pub fn set_style_url(&self, url: &str) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let url = maplibre_core::string::c_string(url)?;
        // SAFETY: map is live and url is a NUL-terminated UTF-8 string valid
        // for the duration of this command. The C API copies/consumes it before
        // returning.
        maplibre_core::check(unsafe { sys::mln_map_set_style_url(map, url.as_ptr()) })?;
        Ok(())
    }

    /// Loads inline style JSON through MapLibre Native style APIs.
    pub fn set_style_json(&self, json: &str) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let json = maplibre_core::string::c_string(json)?;
        // SAFETY: map is live and json is a NUL-terminated UTF-8 string valid
        // for the duration of this command. The C API copies/consumes it before
        // returning. Inline JSON style replacement completes before a successful
        // return, so old custom geometry callback state can be released after.
        maplibre_core::check(unsafe { sys::mln_map_set_style_json(map, json.as_ptr()) })?;
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
        let source_id_view = maplibre_core::string::string_view(source_id);
        let state = CustomGeometrySourceState::new(options);
        let descriptor = state.descriptor();
        // SAFETY: map is live, source_id_view is valid for this call, and
        // descriptor points to callback state retained by this map on success.
        maplibre_core::check(unsafe {
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
        let source_id = maplibre_core::string::string_view(source_id);
        let data = data.try_to_native()?;
        // SAFETY: map is live, source_id is valid for this call, tile_id is
        // passed by value, and data owns the descriptor graph for this call.
        maplibre_core::check(unsafe {
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
        let map = self.inner.as_ptr()?;
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

    /// Adds one style source from a style-spec source JSON object.
    pub fn add_style_source_json(&self, source_id: &str, source_json: &JsonValue) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let source_json = source_json.try_to_native()?;
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and source_json owns the descriptor graph for this call.
        maplibre_core::check(unsafe {
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
        let source_id_key = source_id.to_owned();
        let source_id = maplibre_core::string::string_view(source_id);
        let mut removed = false;
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and removed points to writable storage.
        maplibre_core::check(unsafe {
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
        let map = self.inner.as_ptr()?;
        let image_id = maplibre_core::string::string_view(image_id);
        let image = maplibre_core::values::premultiplied_rgba8_image_to_native(image);
        let options = options.map(StyleImageOptions::to_native);
        let options_ptr = options.as_ref().map_or(ptr::null(), ptr::from_ref);
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
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

    /// Copies fixed metadata and attribution for one style source.
    pub fn style_source_info(&self, source_id: &str) -> Result<Option<SourceInfo>> {
        let map = self.inner.as_ptr()?;
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

        Ok(Some(maplibre_core::style::style_source_info_from_native(
            &info,
            attribution,
        )))
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

    /// Adds a GeoJSON source with inline data.
    pub fn add_geojson_source_data(&self, source_id: &str, data: &GeoJson) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let data = data.try_to_native()?;
        // SAFETY: map is live, source_id is valid for this call, and data owns
        // the descriptor graph for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_add_geojson_source_data(map, source_id.raw(), data.as_ptr())
        })
    }

    /// Updates one GeoJSON source with inline data.
    pub fn set_geojson_source_data(&self, source_id: &str, data: &GeoJson) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let data = data.try_to_native()?;
        // SAFETY: map is live, source_id is valid for this call, and data owns
        // the descriptor graph for this call.
        maplibre_core::check(unsafe {
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
        let before_layer_id = maplibre_core::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live, layer_json owns the descriptor graph, and
        // before_layer_id is an explicit-length view valid for this call.
        maplibre_core::check(unsafe {
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
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
        let map = self.inner.as_ptr()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        // SAFETY: map is live and layer_id is valid for this call.
        maplibre_core::check(unsafe {
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
        let map = self.inner.as_ptr()?;
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
    pub fn style_layer_json(&self, layer_id: &str) -> Result<Option<JsonValue>> {
        let map = self.inner.as_ptr()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_json_snapshot>::new();
        let mut found = false;
        // SAFETY: map is live, layer_id is valid for this call, out is a
        // null-initialized out-pointer, and found points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_layer_json(map, layer_id.raw(), out.as_mut_ptr(), &mut found)
        })?;
        // SAFETY: On success, the C API returns either null or an owned JSON
        // snapshot handle for this call; core copies and releases it.
        let snapshot = unsafe { maplibre_core::json::copy_json_snapshot(out.into_option()) }?;
        if found { Ok(snapshot) } else { Ok(None) }
    }

    /// Sets the style light from a style-spec light JSON object.
    pub fn set_style_light_json(&self, light_json: &JsonValue) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let light_json = light_json.try_to_native()?;
        // SAFETY: map is live and light_json owns the descriptor graph for this call.
        maplibre_core::check(unsafe { sys::mln_map_set_style_light_json(map, light_json.as_ptr()) })
    }

    /// Sets one style light property.
    pub fn set_style_light_property(&self, property_name: &str, value: &JsonValue) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let property_name = maplibre_core::string::string_view(property_name);
        let value = value.try_to_native()?;
        // SAFETY: map is live, property_name is valid for this call, and value
        // owns the descriptor graph for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_style_light_property(map, property_name.raw(), value.as_ptr())
        })
    }

    /// Copies one style light property as a style-spec JSON value.
    pub fn style_light_property(&self, property_name: &str) -> Result<Option<JsonValue>> {
        let map = self.inner.as_ptr()?;
        let property_name = maplibre_core::string::string_view(property_name);
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_json_snapshot>::new();
        // SAFETY: map is live, property_name is valid for this call, and out is
        // a null-initialized out-pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_light_property(map, property_name.raw(), out.as_mut_ptr())
        })?;
        // SAFETY: On success, the C API returns either null or an owned JSON
        // snapshot handle for this call; core copies and releases it.
        unsafe { maplibre_core::json::copy_json_snapshot(out.into_option()) }
    }

    /// Sets one layer style property.
    pub fn set_layer_property(
        &self,
        layer_id: &str,
        property_name: &str,
        value: &JsonValue,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let property_name = maplibre_core::string::string_view(property_name);
        let value = value.try_to_native()?;
        // SAFETY: map is live, string views are valid for this call, and value
        // owns the descriptor graph for this call.
        maplibre_core::check(unsafe {
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
        let layer_id = maplibre_core::string::string_view(layer_id);
        let property_name = maplibre_core::string::string_view(property_name);
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_json_snapshot>::new();
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
        // SAFETY: On success, the C API returns either null or an owned JSON
        // snapshot handle for this call; core copies and releases it.
        unsafe { maplibre_core::json::copy_json_snapshot(out.into_option()) }
    }

    /// Sets or clears one layer filter.
    pub fn set_layer_filter(&self, layer_id: &str, filter: Option<&JsonValue>) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let layer_id = maplibre_core::string::string_view(layer_id);
        let native_filter = filter.map(JsonValue::try_to_native).transpose()?;
        // SAFETY: map is live, layer_id is valid for this call, and the
        // optional filter descriptor is either null or valid for this call.
        maplibre_core::check(unsafe {
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
        let layer_id = maplibre_core::string::string_view(layer_id);
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_json_snapshot>::new();
        // SAFETY: map is live, layer_id is valid for this call, and out is a
        // null-initialized out-pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_layer_filter(map, layer_id.raw(), out.as_mut_ptr())
        })?;
        // SAFETY: On success, the C API returns either null or an owned JSON
        // snapshot handle for this call; core copies and releases it. Some
        // native backends represent a cleared filter as a JSON null snapshot.
        Ok(
            match unsafe { maplibre_core::json::copy_json_snapshot(out.into_option()) }? {
                Some(JsonValue::Null) => None,
                filter => filter,
            },
        )
    }

    /// Copies current style source IDs into owned Rust strings.
    pub fn style_source_ids(&self) -> Result<Vec<String>> {
        let map = self.inner.as_ptr()?;
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_style_id_list>::new();
        // SAFETY: map is live and out is a null-initialized out-pointer owned by
        // this call. On success the returned handle is wrapped and destroyed by
        // the copying helper below.
        maplibre_core::check(unsafe { sys::mln_map_list_style_source_ids(map, out.as_mut_ptr()) })?;
        // SAFETY: On success, the C API returns an owned style ID list handle;
        // core copies and releases it.
        unsafe { maplibre_core::style::copy_style_id_list(out.into_non_null("mln_style_id_list")?) }
    }

    /// Copies current style layer IDs into owned Rust strings.
    pub fn style_layer_ids(&self) -> Result<Vec<String>> {
        let map = self.inner.as_ptr()?;
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_style_id_list>::new();
        // SAFETY: map is live and out is a null-initialized out-pointer owned by
        // this call. On success the returned handle is wrapped and destroyed by
        // the copying helper below.
        maplibre_core::check(unsafe { sys::mln_map_list_style_layer_ids(map, out.as_mut_ptr()) })?;
        // SAFETY: On success, the C API returns an owned style ID list handle;
        // core copies and releases it.
        unsafe { maplibre_core::style::copy_style_id_list(out.into_non_null("mln_style_id_list")?) }
    }
}
