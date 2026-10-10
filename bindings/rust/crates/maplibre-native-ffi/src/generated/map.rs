// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_map` native handle.
    ///
    /// A map, which holds map state independent of any render target.
    ///
    /// See `mln_map` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct MapHandle(mln_map) dispose |raw| maplibre_core::check(|out_diagnostic| unsafe { sys::mln_map_dispose(raw, out_diagnostic) });
}

impl MapHandle {
    /// Adds a color-relief layer for a raster DEM source.
    ///
    /// See `mln_map_add_color_relief_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_color_relief_layer(
        &self,
        layer_id: &str,
        source_id: &str,
        before_layer_id: Option<&str>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_color_relief_layer")?;
        let layer_id = call.input(&layer_id)?;
        let source_id = call.input(&source_id)?;
        let before_layer_id = call.input(&before_layer_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_color_relief_layer(
                map,
                layer_id,
                source_id,
                before_layer_id,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds a custom geometry source.
    ///
    /// See `mln_map_add_custom_geometry_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_custom_geometry_source(
        &self,
        source_id: &str,
        options: CustomGeometrySourceOptions,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_custom_geometry_source")?;
        let source_id = call.input(&source_id)?;
        let options = call.reference(&options)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_custom_geometry_source(
                map,
                source_id,
                options,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds a custom MVT vector source.
    ///
    /// See `mln_map_add_custom_mvt_vector_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_custom_mvt_vector_source(
        &self,
        source_id: &str,
        options: CustomMvtVectorSourceOptions,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_custom_mvt_vector_source")?;
        let source_id = call.input(&source_id)?;
        let options = call.reference(&options)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_custom_mvt_vector_source(
                map,
                source_id,
                options,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds a GeoJSON source with prepared inline data.
    ///
    /// See `mln_map_add_geojson_source_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_geojson_source_data(
        &self,
        source_id: &str,
        data: &GeojsonSourceDataHandle,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_geojson_source_data")?;
        let data = data.inner.native()?;
        let source_id = call.input(&source_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_geojson_source_data(map, source_id, data, completion, out_diagnostic)
        })
    }

    /// Adds a GeoJSON source with URL data.
    ///
    /// See `mln_map_add_geojson_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_geojson_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&GeojsonSourceOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_geojson_source_url")?;
        let source_id = call.input(&source_id)?;
        let url = call.input(&url)?;
        let options = call.optional_reference(options.as_ref())?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_geojson_source_url(
                map,
                source_id,
                url,
                options,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds a hillshade layer for a raster DEM source.
    ///
    /// See `mln_map_add_hillshade_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_hillshade_layer(
        &self,
        layer_id: &str,
        source_id: &str,
        before_layer_id: Option<&str>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_hillshade_layer")?;
        let layer_id = call.input(&layer_id)?;
        let source_id = call.input(&source_id)?;
        let before_layer_id = call.input(&before_layer_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_hillshade_layer(
                map,
                layer_id,
                source_id,
                before_layer_id,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds an image source with inline image pixels.
    ///
    /// See `mln_map_add_image_source_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_image_source_image(
        &self,
        source_id: &str,
        coordinates: &[LatLng],
        image: &PremultipliedRgba8Image,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_image_source_image")?;
        let coordinate_count = convert::count(coordinates.len())?;
        let source_id = call.input(&source_id)?;
        let coordinates = call.array(coordinates)?;
        let image = call.reference(&image)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_image_source_image(
                map,
                source_id,
                coordinates,
                coordinate_count,
                image,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds an image source that loads its image from a URL.
    ///
    /// See `mln_map_add_image_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_image_source_url(
        &self,
        source_id: &str,
        coordinates: &[LatLng],
        url: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_image_source_url")?;
        let coordinate_count = convert::count(coordinates.len())?;
        let source_id = call.input(&source_id)?;
        let coordinates = call.array(coordinates)?;
        let url = call.input(&url)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_image_source_url(
                map,
                source_id,
                coordinates,
                coordinate_count,
                url,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds a source-free location indicator layer.
    ///
    /// See `mln_map_add_location_indicator_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_location_indicator_layer(
        &self,
        layer_id: &str,
        before_layer_id: Option<&str>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_location_indicator_layer")?;
        let layer_id = call.input(&layer_id)?;
        let before_layer_id = call.input(&before_layer_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_location_indicator_layer(
                map,
                layer_id,
                before_layer_id,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds a raster DEM source with inline tile URLs.
    ///
    /// See `mln_map_add_raster_dem_source_tiles` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_raster_dem_source_tiles(
        &self,
        source_id: &str,
        tiles: &[&str],
        options: Option<&StyleTileSourceOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_raster_dem_source_tiles")?;
        let tile_count = convert::count(tiles.len())?;
        let source_id = call.input(&source_id)?;
        let tiles = call.array(tiles)?;
        let options = call.optional_reference(options.as_ref())?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_raster_dem_source_tiles(
                map,
                source_id,
                tiles,
                tile_count,
                options,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds a raster DEM source with a TileJSON URL.
    ///
    /// See `mln_map_add_raster_dem_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_raster_dem_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&StyleTileSourceOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_raster_dem_source_url")?;
        let source_id = call.input(&source_id)?;
        let url = call.input(&url)?;
        let options = call.optional_reference(options.as_ref())?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_raster_dem_source_url(
                map,
                source_id,
                url,
                options,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds a raster source with inline tile URLs.
    ///
    /// See `mln_map_add_raster_source_tiles` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_raster_source_tiles(
        &self,
        source_id: &str,
        tiles: &[&str],
        options: Option<&StyleTileSourceOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_raster_source_tiles")?;
        let tile_count = convert::count(tiles.len())?;
        let source_id = call.input(&source_id)?;
        let tiles = call.array(tiles)?;
        let options = call.optional_reference(options.as_ref())?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_raster_source_tiles(
                map,
                source_id,
                tiles,
                tile_count,
                options,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds a raster source with a TileJSON URL.
    ///
    /// See `mln_map_add_raster_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_raster_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&StyleTileSourceOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_raster_source_url")?;
        let source_id = call.input(&source_id)?;
        let url = call.input(&url)?;
        let options = call.optional_reference(options.as_ref())?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_raster_source_url(
                map,
                source_id,
                url,
                options,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds one style layer from a full style-spec layer JSON object.
    ///
    /// See `mln_map_add_style_layer_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_style_layer_json(
        &self,
        layer_json: &[u8],
        before_layer_id: Option<&str>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_style_layer_json")?;
        let layer_json = call.input(&layer_json)?;
        let before_layer_id = call.input(&before_layer_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_style_layer_json(
                map,
                layer_json,
                before_layer_id,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds one style source from a style-spec source JSON object.
    ///
    /// See `mln_map_add_style_source_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_style_source_json(
        &self,
        source_id: &str,
        source_json: &[u8],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_style_source_json")?;
        let source_id = call.input(&source_id)?;
        let source_json = call.input(&source_json)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_style_source_json(
                map,
                source_id,
                source_json,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds a vector source with inline tile URLs.
    ///
    /// See `mln_map_add_vector_source_tiles` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_vector_source_tiles(
        &self,
        source_id: &str,
        tiles: &[&str],
        options: Option<&StyleTileSourceOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_vector_source_tiles")?;
        let tile_count = convert::count(tiles.len())?;
        let source_id = call.input(&source_id)?;
        let tiles = call.array(tiles)?;
        let options = call.optional_reference(options.as_ref())?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_vector_source_tiles(
                map,
                source_id,
                tiles,
                tile_count,
                options,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds a vector source with a TileJSON URL.
    ///
    /// See `mln_map_add_vector_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_vector_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&StyleTileSourceOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_add_vector_source_url")?;
        let source_id = call.input(&source_id)?;
        let url = call.input(&url)?;
        let options = call.optional_reference(options.as_ref())?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_vector_source_url(
                map,
                source_id,
                url,
                options,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Submits one copied relative camera update.
    ///
    /// See `mln_map_apply_camera_delta` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn apply_camera_delta(
        &self,
        delta: &CameraDelta,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_apply_camera_delta")?;
        let delta = call.reference(&delta)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_apply_camera_delta(map, delta, completion, out_diagnostic)
        })
    }

    /// Begins a command group, which holds this map's render updates until the
    /// group ends.
    ///
    /// See `mln_map_begin_command_group` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn begin_command_group(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call("mln_map_begin_command_group")?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_begin_command_group(map, completion, out_diagnostic)
        })
    }

    /// Starts an ordered query for a camera that fits a GeoJSON geometry.
    ///
    /// See `mln_map_camera_for_geometry` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn camera_for_geometry(
        &self,
        geometry: &[u8],
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<NativeFuture<CameraOptions>> {
        let mut call = self.inner.call("mln_map_camera_for_geometry")?;
        let geometry = call.input(&geometry)?;
        let fit_options = call.optional_reference(fit_options.as_ref())?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_camera_for_geometry(
                    map,
                    geometry,
                    fit_options,
                    completion,
                    out_diagnostic,
                )
            },
            completion::value::<sys::mln_camera_options, _>,
        )
    }

    /// Starts an ordered query for a camera that fits geographic bounds.
    ///
    /// See `mln_map_camera_for_lat_lng_bounds` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn camera_for_lat_lng_bounds(
        &self,
        bounds: LatLngBounds,
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<NativeFuture<CameraOptions>> {
        let mut call = self.inner.call("mln_map_camera_for_lat_lng_bounds")?;
        let bounds = call.input(&bounds)?;
        let fit_options = call.optional_reference(fit_options.as_ref())?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_camera_for_lat_lng_bounds(
                    map,
                    bounds,
                    fit_options,
                    completion,
                    out_diagnostic,
                )
            },
            completion::value::<sys::mln_camera_options, _>,
        )
    }

    /// Starts an ordered query for a camera that fits geographic coordinates.
    ///
    /// See `mln_map_camera_for_lat_lngs` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn camera_for_lat_lngs(
        &self,
        coordinates: &[LatLng],
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<NativeFuture<CameraOptions>> {
        let mut call = self.inner.call("mln_map_camera_for_lat_lngs")?;
        let coordinate_count = convert::count(coordinates.len())?;
        let coordinates = call.array(coordinates)?;
        let fit_options = call.optional_reference(fit_options.as_ref())?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_camera_for_lat_lngs(
                    map,
                    coordinates,
                    coordinate_count,
                    fit_options,
                    completion,
                    out_diagnostic,
                )
            },
            completion::value::<sys::mln_camera_options, _>,
        )
    }

    /// Starts an ordered camera read.
    ///
    /// See `mln_map_camera_query` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn camera_query(&self) -> Result<NativeFuture<CameraQueryResult>> {
        let call = self.inner.call("mln_map_camera_query")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_camera_query(map, completion, out_diagnostic)
            },
            completion::value::<sys::mln_camera_query_result, _>,
        )
    }

    /// Copies the camera from the latest immutable map snapshot.
    ///
    /// See `mln_map_camera_snapshot_get` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn camera_snapshot_get(&self) -> Result<(CameraOptions, u64)> {
        let mut call = self.inner.call("mln_map_camera_snapshot_get")?;
        let mut out_camera: sys::mln_camera_options = unsafe { sys::mln_camera_options_default() };
        out_camera.size = std::mem::size_of::<sys::mln_camera_options>() as _;
        let mut out_generation: u64 = Default::default();
        call.status(|map, out_diagnostic| unsafe {
            sys::mln_map_camera_snapshot_get(
                map,
                &mut out_camera,
                &mut out_generation,
                out_diagnostic,
            )
        })?;
        Ok((unsafe { from_native(out_camera) }?, out_generation))
    }

    /// Cancels the camera transitions running when this command commits.
    ///
    /// See `mln_map_cancel_transitions` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn cancel_transitions(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call("mln_map_cancel_transitions")?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_cancel_transitions(map, completion, out_diagnostic)
        })
    }

    /// Consumes a map handle without observing its asynchronous retirement.
    ///
    /// See `mln_map_dispose` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn dispose(&self) -> Result<()> {
        self.inner.close(|map| {
            let mut call = Call::new(map, None);
            call.status(|map, out_diagnostic| unsafe { sys::mln_map_dispose(map, out_diagnostic) })
        })
    }

    /// Submits an ordered debug-log command.
    ///
    /// See `mln_map_dump_debug_logs` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn dump_debug_logs(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call("mln_map_dump_debug_logs")?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_dump_debug_logs(map, completion, out_diagnostic)
        })
    }

    /// Ends the innermost command group that `mln_map_begin_command_group()`
    /// began.
    ///
    /// See `mln_map_end_command_group` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn end_command_group(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call("mln_map_end_command_group")?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_end_command_group(map, completion, out_diagnostic)
        })
    }

    /// Starts an ordered read of per-feature state from this map.
    ///
    /// See `mln_map_get_feature_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn get_feature_state(
        &self,
        selector: &FeatureStateSelector,
    ) -> Result<NativeFuture<Vec<u8>>> {
        let mut call = self.inner.call("mln_map_get_feature_state")?;
        let selector = call.reference(&selector)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_feature_state(map, selector, completion, out_diagnostic)
            },
            completion::value::<sys::mln_buffer_view, _>,
        )
    }

    /// Queries the global-state JSON object, including style defaults.
    /// Completion borrows one `mln_buffer_view` for the duration of the
    /// callback.
    ///
    /// See `mln_map_get_global_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_global_state(&self) -> Result<NativeFuture<Vec<u8>>> {
        let call = self.inner.call("mln_map_get_global_state")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_global_state(map, completion, out_diagnostic)
            },
            completion::value::<sys::mln_buffer_view, _>,
        )
    }

    /// Copies image source coordinates.
    ///
    /// See `mln_map_get_image_source_coordinates` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_image_source_coordinates(
        &self,
        source_id: &str,
    ) -> Result<NativeFuture<Option<Vec<LatLng>>>> {
        let mut call = self.inner.call("mln_map_get_image_source_coordinates")?;
        let source_id = call.input(&source_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_image_source_coordinates(
                    map,
                    source_id,
                    completion,
                    out_diagnostic,
                )
            },
            completion::optional_list::<sys::mln_lat_lng, _>,
        )
    }

    /// Serializes one layer filter as a style-spec JSON value.
    ///
    /// See `mln_map_get_layer_filter` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_layer_filter(&self, layer_id: &str) -> Result<NativeFuture<Option<Vec<u8>>>> {
        let mut call = self.inner.call("mln_map_get_layer_filter")?;
        let layer_id = call.input(&layer_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_layer_filter(map, layer_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_buffer_view, _>,
        )
    }

    /// Serializes one layer property as a style-spec JSON value.
    ///
    /// See `mln_map_get_layer_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_layer_property(
        &self,
        layer_id: &str,
        property_name: &str,
    ) -> Result<NativeFuture<Option<Vec<u8>>>> {
        let mut call = self.inner.call("mln_map_get_layer_property")?;
        let layer_id = call.input(&layer_id)?;
        let property_name = call.input(&property_name)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_layer_property(
                    map,
                    layer_id,
                    property_name,
                    completion,
                    out_diagnostic,
                )
            },
            completion::optional::<sys::mln_buffer_view, _>,
        )
    }

    /// Copies one complete runtime style image.
    ///
    /// See `mln_map_get_style_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_image(&self, image_id: &str) -> Result<NativeFuture<Option<StyleImageInfo>>> {
        let mut call = self.inner.call("mln_map_get_style_image")?;
        let image_id = call.input(&image_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_image(map, image_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_style_image_info, _>,
        )
    }

    /// Copies the complete metadata of one style layer.
    ///
    /// See `mln_map_get_style_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_layer(&self, layer_id: &str) -> Result<NativeFuture<Option<StyleLayerInfo>>> {
        let mut call = self.inner.call("mln_map_get_style_layer")?;
        let layer_id = call.input(&layer_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_layer(map, layer_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_style_layer_info, _>,
        )
    }

    /// Serializes one style layer as a full style-spec layer JSON object.
    ///
    /// See `mln_map_get_style_layer_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_layer_json(&self, layer_id: &str) -> Result<NativeFuture<Option<Vec<u8>>>> {
        let mut call = self.inner.call("mln_map_get_style_layer_json")?;
        let layer_id = call.input(&layer_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_layer_json(map, layer_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_buffer_view, _>,
        )
    }

    /// Serializes one style light property as a style-spec JSON value.
    ///
    /// See `mln_map_get_style_light_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_light_property(
        &self,
        property_name: &str,
    ) -> Result<NativeFuture<Option<Vec<u8>>>> {
        let mut call = self.inner.call("mln_map_get_style_light_property")?;
        let property_name = call.input(&property_name)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_light_property(
                    map,
                    property_name,
                    completion,
                    out_diagnostic,
                )
            },
            completion::optional::<sys::mln_buffer_view, _>,
        )
    }

    /// Copies the complete metadata of one style source.
    ///
    /// See `mln_map_get_style_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_source(
        &self,
        source_id: &str,
    ) -> Result<NativeFuture<Option<StyleSourceInfo>>> {
        let mut call = self.inner.call("mln_map_get_style_source")?;
        let source_id = call.input(&source_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_source(map, source_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_style_source_info, _>,
        )
    }

    /// Reads the style's global transition options.
    ///
    /// See `mln_map_get_style_transition_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_transition_options(&self) -> Result<NativeFuture<StyleTransitionOptions>> {
        let call = self.inner.call("mln_map_get_style_transition_options")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_transition_options(map, completion, out_diagnostic)
            },
            completion::value::<sys::mln_style_transition_options, _>,
        )
    }

    /// Invalidates custom geometry source data inside one geographic region.
    ///
    /// See `mln_map_invalidate_custom_geometry_source_region` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn invalidate_custom_geometry_source_region(
        &self,
        source_id: &str,
        bounds: LatLngBounds,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self
            .inner
            .call("mln_map_invalidate_custom_geometry_source_region")?;
        let source_id = call.input(&source_id)?;
        let bounds = call.input(&bounds)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_invalidate_custom_geometry_source_region(
                map,
                source_id,
                bounds,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Invalidates custom geometry source data for one canonical tile.
    ///
    /// See `mln_map_invalidate_custom_geometry_source_tile` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn invalidate_custom_geometry_source_tile(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self
            .inner
            .call("mln_map_invalidate_custom_geometry_source_tile")?;
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_invalidate_custom_geometry_source_tile(
                map,
                source_id,
                tile_id,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Invalidates custom MVT vector source data for one canonical tile.
    ///
    /// See `mln_map_invalidate_custom_mvt_vector_source_tile` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn invalidate_custom_mvt_vector_source_tile(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self
            .inner
            .call("mln_map_invalidate_custom_mvt_vector_source_tile")?;
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_invalidate_custom_mvt_vector_source_tile(
                map,
                source_id,
                tile_id,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Starts an ordered wrapped-bounds query for a copied camera.
    ///
    /// See `mln_map_lat_lng_bounds_for_camera` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn lat_lng_bounds_for_camera(
        &self,
        camera: &CameraOptions,
    ) -> Result<NativeFuture<LatLngBounds>> {
        let mut call = self.inner.call("mln_map_lat_lng_bounds_for_camera")?;
        let camera = call.reference(&camera)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_lat_lng_bounds_for_camera(map, camera, completion, out_diagnostic)
            },
            completion::value::<sys::mln_lat_lng_bounds, _>,
        )
    }

    /// Starts an ordered unwrapped-bounds query for a copied camera.
    ///
    /// See `mln_map_lat_lng_bounds_for_camera_unwrapped` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn lat_lng_bounds_for_camera_unwrapped(
        &self,
        camera: &CameraOptions,
    ) -> Result<NativeFuture<LatLngBounds>> {
        let mut call = self
            .inner
            .call("mln_map_lat_lng_bounds_for_camera_unwrapped")?;
        let camera = call.reference(&camera)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_lat_lng_bounds_for_camera_unwrapped(
                    map,
                    camera,
                    completion,
                    out_diagnostic,
                )
            },
            completion::value::<sys::mln_lat_lng_bounds, _>,
        )
    }

    /// Starts an ordered conversion from a screen point to a geographic
    /// coordinate.
    ///
    /// See `mln_map_lat_lng_for_pixel` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn lat_lng_for_pixel(&self, point: ScreenPoint) -> Result<NativeFuture<LatLng>> {
        let mut call = self.inner.call("mln_map_lat_lng_for_pixel")?;
        let point = call.input(&point)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_lat_lng_for_pixel(map, point, completion, out_diagnostic)
            },
            completion::value::<sys::mln_lat_lng, _>,
        )
    }

    /// Starts an ordered conversion from a screen point to an unwrapped
    /// geographic coordinate.
    ///
    /// See `mln_map_lat_lng_for_pixel_unwrapped` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn lat_lng_for_pixel_unwrapped(&self, point: ScreenPoint) -> Result<NativeFuture<LatLng>> {
        let mut call = self.inner.call("mln_map_lat_lng_for_pixel_unwrapped")?;
        let point = call.input(&point)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_lat_lng_for_pixel_unwrapped(map, point, completion, out_diagnostic)
            },
            completion::value::<sys::mln_lat_lng, _>,
        )
    }

    /// Starts an ordered conversion of copied screen points to coordinates.
    ///
    /// See `mln_map_lat_lngs_for_pixels` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn lat_lngs_for_pixels(&self, points: &[ScreenPoint]) -> Result<NativeFuture<Vec<LatLng>>> {
        let mut call = self.inner.call("mln_map_lat_lngs_for_pixels")?;
        let point_count = convert::count(points.len())?;
        let points = call.array(points)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_lat_lngs_for_pixels(
                    map,
                    points,
                    point_count,
                    completion,
                    out_diagnostic,
                )
            },
            completion::list::<sys::mln_lat_lng, _>,
        )
    }

    /// Starts an ordered conversion of copied screen points to unwrapped
    /// coordinates.
    ///
    /// See `mln_map_lat_lngs_for_pixels_unwrapped` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn lat_lngs_for_pixels_unwrapped(
        &self,
        points: &[ScreenPoint],
    ) -> Result<NativeFuture<Vec<LatLng>>> {
        let mut call = self.inner.call("mln_map_lat_lngs_for_pixels_unwrapped")?;
        let point_count = convert::count(points.len())?;
        let points = call.array(points)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_lat_lngs_for_pixels_unwrapped(
                    map,
                    points,
                    point_count,
                    completion,
                    out_diagnostic,
                )
            },
            completion::list::<sys::mln_lat_lng, _>,
        )
    }

    /// Starts an ordered query of every style layer in style order.
    ///
    /// See `mln_map_list_style_layers` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn list_style_layers(&self) -> Result<NativeFuture<Vec<StyleLayerInfo>>> {
        let call = self.inner.call("mln_map_list_style_layers")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_list_style_layers(map, completion, out_diagnostic)
            },
            completion::list::<sys::mln_style_layer_info, _>,
        )
    }

    /// Lists every style source in style order.
    ///
    /// See `mln_map_list_style_sources` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn list_style_sources(&self) -> Result<NativeFuture<Vec<StyleSourceInfo>>> {
        let call = self.inner.call("mln_map_list_style_sources")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_list_style_sources(map, completion, out_diagnostic)
            },
            completion::list::<sys::mln_style_source_info, _>,
        )
    }

    /// Starts an ordered copy of the last successfully parsed style document.
    ///
    /// See `mln_map_loaded_style_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn loaded_style_json(&self) -> Result<NativeFuture<Vec<u8>>> {
        let call = self.inner.call("mln_map_loaded_style_json")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_loaded_style_json(map, completion, out_diagnostic)
            },
            completion::value::<sys::mln_buffer_view, _>,
        )
    }

    /// Starts an ordered query of meters per logical pixel at a latitude and
    /// the current map zoom. The completion borrows one double.
    ///
    /// See `mln_map_meters_per_pixel_at_latitude` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn meters_per_pixel_at_latitude(&self, latitude: f64) -> Result<NativeFuture<f64>> {
        let call = self.inner.call("mln_map_meters_per_pixel_at_latitude")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_meters_per_pixel_at_latitude(map, latitude, completion, out_diagnostic)
            },
            |result| {
                let value = completion::copy_value::<f64>(result)?;
                Ok(value)
            },
        )
    }

    /// Moves one style layer before another layer or to the top.
    ///
    /// See `mln_map_move_style_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn move_style_layer(
        &self,
        layer_id: &str,
        before_layer_id: Option<&str>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_move_style_layer")?;
        let layer_id = call.input(&layer_id)?;
        let before_layer_id = call.input(&before_layer_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_move_style_layer(
                map,
                layer_id,
                before_layer_id,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Starts an ordered conversion from a geographic coordinate to a screen
    /// point.
    ///
    /// See `mln_map_pixel_for_lat_lng` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn pixel_for_lat_lng(&self, coordinate: LatLng) -> Result<NativeFuture<ScreenPoint>> {
        let mut call = self.inner.call("mln_map_pixel_for_lat_lng")?;
        let coordinate = call.input(&coordinate)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_pixel_for_lat_lng(map, coordinate, completion, out_diagnostic)
            },
            completion::value::<sys::mln_screen_point, _>,
        )
    }

    /// Starts an ordered conversion of copied coordinates to screen points.
    ///
    /// See `mln_map_pixels_for_lat_lngs` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn pixels_for_lat_lngs(
        &self,
        coordinates: &[LatLng],
    ) -> Result<NativeFuture<Vec<ScreenPoint>>> {
        let mut call = self.inner.call("mln_map_pixels_for_lat_lngs")?;
        let coordinate_count = convert::count(coordinates.len())?;
        let coordinates = call.array(coordinates)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_pixels_for_lat_lngs(
                    map,
                    coordinates,
                    coordinate_count,
                    completion,
                    out_diagnostic,
                )
            },
            completion::list::<sys::mln_screen_point, _>,
        )
    }

    /// Starts creation of a standalone projection from the map's ordered
    /// transform state.
    ///
    /// See `mln_map_projection_create` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    pub fn projection_create(&self) -> Result<NativeFuture<MapProjectionHandle>> {
        let call = self.inner.call("mln_map_projection_create")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_projection_create(map, completion, out_diagnostic)
            },
            move |result| {
                MapProjectionHandle::adopt(
                    completion::copy_value::<sys::mln_map_projection>(result)?,
                    None,
                )
            },
        )
    }

    /// Releases a map after synchronous state preflight.
    ///
    /// See `mln_map_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn release(&self) -> Result<NativeFuture<()>> {
        self.inner.release(|map| {
            let call = Call::new(map, None);
            call.complete(
                |map, completion, out_diagnostic| unsafe {
                    sys::mln_map_release(map, completion, out_diagnostic)
                },
                completion::unit,
            )
        })
    }

    /// Removes per-feature state from this map.
    ///
    /// See `mln_map_remove_feature_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn remove_feature_state(
        &self,
        selector: &FeatureStateSelector,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_remove_feature_state")?;
        let selector = call.reference(&selector)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_feature_state(map, selector, completion, out_diagnostic)
        })
    }

    /// Removes one runtime style image by ID.
    ///
    /// See `mln_map_remove_style_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn remove_style_image(&self, image_id: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_remove_style_image")?;
        let image_id = call.input(&image_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_style_image(map, image_id, completion, out_diagnostic)
        })
    }

    /// Removes one style layer by ID.
    ///
    /// See `mln_map_remove_style_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn remove_style_layer(&self, layer_id: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_remove_style_layer")?;
        let layer_id = call.input(&layer_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_style_layer(map, layer_id, completion, out_diagnostic)
        })
    }

    /// Removes one style source by ID.
    ///
    /// See `mln_map_remove_style_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn remove_style_source(&self, source_id: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_remove_style_source")?;
        let source_id = call.input(&source_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_style_source(map, source_id, completion, out_diagnostic)
        })
    }

    /// Requests a repaint for a continuous map.
    ///
    /// See `mln_map_request_repaint` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn request_repaint(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call("mln_map_request_repaint")?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_request_repaint(map, completion, out_diagnostic)
        })
    }

    /// Requests one still image for a static or tile map.
    ///
    /// See `mln_map_request_still_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn request_still_image(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call("mln_map_request_still_image")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_request_still_image(map, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Submits the sole post-creation logical extent update.
    ///
    /// See `mln_map_resize` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn resize(&self, extent: LogicalExtent) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_resize")?;
        let extent = call.input(&extent)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_resize(map, extent, completion, out_diagnostic)
        })
    }

    /// Submits a copied camera-constraint command.
    ///
    /// See `mln_map_set_bounds` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_bounds(&self, options: &BoundOptions) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_bounds")?;
        let options = call.reference(&options)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_bounds(map, options, completion, out_diagnostic)
        })
    }

    /// Sets custom geometry source data for one canonical tile.
    ///
    /// See `mln_map_set_custom_geometry_source_tile_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_custom_geometry_source_tile_data(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        data: &[u8],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self
            .inner
            .call("mln_map_set_custom_geometry_source_tile_data")?;
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        let data = call.input(&data)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_custom_geometry_source_tile_data(
                map,
                source_id,
                tile_id,
                data,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets custom MVT vector source data for one canonical tile.
    ///
    /// See `mln_map_set_custom_mvt_vector_source_tile_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_custom_mvt_vector_source_tile_data(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        data: &[u8],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self
            .inner
            .call("mln_map_set_custom_mvt_vector_source_tile_data")?;
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        let data = call.input(&data)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_custom_mvt_vector_source_tile_data(
                map,
                source_id,
                tile_id,
                data,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Reports a custom MVT vector source error for one canonical tile.
    ///
    /// See `mln_map_set_custom_mvt_vector_source_tile_error` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_custom_mvt_vector_source_tile_error(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        message: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self
            .inner
            .call("mln_map_set_custom_mvt_vector_source_tile_error")?;
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        let message = call.input(&message)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_custom_mvt_vector_source_tile_error(
                map,
                source_id,
                tile_id,
                message,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Submits a debug-overlay command.
    ///
    /// See `mln_map_set_debug_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_debug_options(
        &self,
        options: MapDebugOption,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call("mln_map_set_debug_options")?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_debug_options(map, options.to_native(), completion, out_diagnostic)
        })
    }

    /// Selects which map-originated event types this map queues.
    ///
    /// See `mln_map_set_event_mask` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_event_mask(
        &self,
        mask: RuntimeEventMask,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call("mln_map_set_event_mask")?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_event_mask(map, mask.to_native(), completion, out_diagnostic)
        })
    }

    /// Submits a copied per-feature-state command.
    ///
    /// See `mln_map_set_feature_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_feature_state(
        &self,
        selector: &FeatureStateSelector,
        state: &[u8],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_feature_state")?;
        let selector = call.reference(&selector)?;
        let state = call.input(&state)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_feature_state(map, selector, state, completion, out_diagnostic)
        })
    }

    /// Submits a copied free-camera command.
    ///
    /// See `mln_map_set_free_camera_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_free_camera_options(
        &self,
        options: &FreeCameraOptions,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_free_camera_options")?;
        let options = call.reference(&options)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_free_camera_options(map, options, completion, out_diagnostic)
        })
    }

    /// Updates one GeoJSON source with prepared inline data.
    ///
    /// See `mln_map_set_geojson_source_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_geojson_source_data(
        &self,
        source_id: &str,
        data: &GeojsonSourceDataHandle,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_geojson_source_data")?;
        let data = data.inner.native()?;
        let source_id = call.input(&source_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_geojson_source_data(map, source_id, data, completion, out_diagnostic)
        })
    }

    /// Overrides one GeoJSON source's synchronous tiling at runtime.
    ///
    /// See `mln_map_set_geojson_source_synchronous_tiling` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_geojson_source_synchronous_tiling(
        &self,
        source_id: &str,
        enabled: bool,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self
            .inner
            .call("mln_map_set_geojson_source_synchronous_tiling")?;
        let source_id = call.input(&source_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_geojson_source_synchronous_tiling(
                map,
                source_id,
                enabled,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Updates one GeoJSON source to load data from a URL.
    ///
    /// See `mln_map_set_geojson_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_geojson_source_url(
        &self,
        source_id: &str,
        url: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_geojson_source_url")?;
        let source_id = call.input(&source_id)?;
        let url = call.input(&url)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_geojson_source_url(map, source_id, url, completion, out_diagnostic)
        })
    }

    /// Submits a global-state JSON value. JSON null restores the style default.
    /// Input is copied before return.
    ///
    /// See `mln_map_set_global_state_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_global_state_property(
        &self,
        property_name: &str,
        value_: &[u8],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_global_state_property")?;
        let property_name = call.input(&property_name)?;
        let value_ = call.input(&value_)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_global_state_property(
                map,
                property_name,
                value_,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Updates image source coordinates.
    ///
    /// See `mln_map_set_image_source_coordinates` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_image_source_coordinates(
        &self,
        source_id: &str,
        coordinates: &[LatLng],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_image_source_coordinates")?;
        let coordinate_count = convert::count(coordinates.len())?;
        let source_id = call.input(&source_id)?;
        let coordinates = call.array(coordinates)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_image_source_coordinates(
                map,
                source_id,
                coordinates,
                coordinate_count,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Updates an image source with inline image pixels.
    ///
    /// See `mln_map_set_image_source_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_image_source_image(
        &self,
        source_id: &str,
        image: &PremultipliedRgba8Image,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_image_source_image")?;
        let source_id = call.input(&source_id)?;
        let image = call.reference(&image)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_image_source_image(map, source_id, image, completion, out_diagnostic)
        })
    }

    /// Updates an image source to load its image from a URL.
    ///
    /// See `mln_map_set_image_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_image_source_url(
        &self,
        source_id: &str,
        url: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_image_source_url")?;
        let source_id = call.input(&source_id)?;
        let url = call.input(&url)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_image_source_url(map, source_id, url, completion, out_diagnostic)
        })
    }

    /// Sets or clears one layer filter.
    ///
    /// See `mln_map_set_layer_filter` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_layer_filter(
        &self,
        layer_id: &str,
        filter: Option<&[u8]>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_layer_filter")?;
        let layer_id = call.input(&layer_id)?;
        let filter = call.optional_reference(filter)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_layer_filter(map, layer_id, filter, completion, out_diagnostic)
        })
    }

    /// Sets the highest zoom at which one layer draws.
    ///
    /// See `mln_map_set_layer_max_zoom` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_layer_max_zoom(
        &self,
        layer_id: &str,
        max_zoom: f64,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_layer_max_zoom")?;
        let layer_id = call.input(&layer_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_layer_max_zoom(map, layer_id, max_zoom, completion, out_diagnostic)
        })
    }

    /// Sets the lowest zoom at which one layer draws.
    ///
    /// See `mln_map_set_layer_min_zoom` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_layer_min_zoom(
        &self,
        layer_id: &str,
        min_zoom: f64,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_layer_min_zoom")?;
        let layer_id = call.input(&layer_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_layer_min_zoom(map, layer_id, min_zoom, completion, out_diagnostic)
        })
    }

    /// Sets one layer property using its MapLibre style-spec property name.
    ///
    /// See `mln_map_set_layer_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_layer_property(
        &self,
        layer_id: &str,
        property_name: &str,
        value_: &[u8],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_layer_property")?;
        let layer_id = call.input(&layer_id)?;
        let property_name = call.input(&property_name)?;
        let value_ = call.input(&value_)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_layer_property(
                map,
                layer_id,
                property_name,
                value_,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets one layer's source ID.
    ///
    /// See `mln_map_set_layer_source_id` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_layer_source_id(
        &self,
        layer_id: &str,
        source_id: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_layer_source_id")?;
        let layer_id = call.input(&layer_id)?;
        let source_id = call.input(&source_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_layer_source_id(map, layer_id, source_id, completion, out_diagnostic)
        })
    }

    /// Sets one layer's source-layer ID.
    ///
    /// See `mln_map_set_layer_source_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_layer_source_layer(
        &self,
        layer_id: &str,
        source_layer: Option<&str>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_layer_source_layer")?;
        let layer_id = call.input(&layer_id)?;
        let source_layer = call.input(&source_layer)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_layer_source_layer(
                map,
                layer_id,
                source_layer,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets whether one layer draws.
    ///
    /// See `mln_map_set_layer_visibility` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_layer_visibility(
        &self,
        layer_id: &str,
        visibility: StyleLayerVisibility,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_layer_visibility")?;
        let layer_id = call.input(&layer_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_layer_visibility(
                map,
                layer_id,
                visibility.to_native(),
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets a location indicator layer accuracy radius in meters.
    ///
    /// See `mln_map_set_location_indicator_accuracy_radius` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_location_indicator_accuracy_radius(
        &self,
        layer_id: &str,
        radius: f64,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self
            .inner
            .call("mln_map_set_location_indicator_accuracy_radius")?;
        let layer_id = call.input(&layer_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_location_indicator_accuracy_radius(
                map,
                layer_id,
                radius,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets a location indicator layer bearing in degrees.
    ///
    /// See `mln_map_set_location_indicator_bearing` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_location_indicator_bearing(
        &self,
        layer_id: &str,
        bearing: f64,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_location_indicator_bearing")?;
        let layer_id = call.input(&layer_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_location_indicator_bearing(
                map,
                layer_id,
                bearing,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets one location indicator image-name property.
    ///
    /// See `mln_map_set_location_indicator_image_name` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_location_indicator_image_name(
        &self,
        layer_id: &str,
        image_kind: LocationIndicatorImageKind,
        image_id: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self
            .inner
            .call("mln_map_set_location_indicator_image_name")?;
        let layer_id = call.input(&layer_id)?;
        let image_id = call.input(&image_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_location_indicator_image_name(
                map,
                layer_id,
                image_kind.to_native(),
                image_id,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets a location indicator layer location.
    ///
    /// See `mln_map_set_location_indicator_location` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_location_indicator_location(
        &self,
        layer_id: &str,
        coordinate: LatLng,
        altitude: f64,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_location_indicator_location")?;
        let layer_id = call.input(&layer_id)?;
        let coordinate = call.input(&coordinate)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_location_indicator_location(
                map,
                layer_id,
                coordinate,
                altitude,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Submits copied axonometric rendering option fields.
    ///
    /// See `mln_map_set_projection_mode` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_projection_mode(
        &self,
        mode: &ProjectionMode,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_projection_mode")?;
        let mode = call.reference(&mode)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_projection_mode(map, mode, completion, out_diagnostic)
        })
    }

    /// Submits a rendering-stats visibility command.
    ///
    /// See `mln_map_set_rendering_stats_view_enabled` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_rendering_stats_view_enabled(
        &self,
        enabled: bool,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let call = self
            .inner
            .call("mln_map_set_rendering_stats_view_enabled")?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_rendering_stats_view_enabled(map, enabled, completion, out_diagnostic)
        })
    }

    /// Sets one runtime style image.
    ///
    /// See `mln_map_set_style_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_image(
        &self,
        image_id: &str,
        image: &PremultipliedRgba8Image,
        options: Option<&StyleImageOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_style_image")?;
        let image_id = call.input(&image_id)?;
        let image = call.reference(&image)?;
        let options = call.optional_reference(options.as_ref())?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_image(map, image_id, image, options, completion, out_diagnostic)
        })
    }

    /// Queues an inline style JSON command.
    ///
    /// See `mln_map_set_style_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_style_json(&self, json: &[u8]) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_style_json")?;
        let json = call.input(&json)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_json(map, json, completion, out_diagnostic)
        })
    }

    /// Sets the style light from a style-spec light JSON object.
    ///
    /// See `mln_map_set_style_light_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_light_json(
        &self,
        light_json: &[u8],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_style_light_json")?;
        let light_json = call.input(&light_json)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_light_json(map, light_json, completion, out_diagnostic)
        })
    }

    /// Sets one style light property using its MapLibre style-spec property
    /// name.
    ///
    /// See `mln_map_set_style_light_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_light_property(
        &self,
        property_name: &str,
        value_: &[u8],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_style_light_property")?;
        let property_name = call.input(&property_name)?;
        let value_ = call.input(&value_)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_light_property(
                map,
                property_name,
                value_,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets whether one style source stores fetched tiles in the persistent
    /// cache.
    ///
    /// See `mln_map_set_style_source_volatile` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_source_volatile(
        &self,
        source_id: &str,
        is_volatile: bool,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_style_source_volatile")?;
        let source_id = call.input(&source_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_source_volatile(
                map,
                source_id,
                is_volatile,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets the style's global transition options.
    ///
    /// See `mln_map_set_style_transition_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_transition_options(
        &self,
        options: &StyleTransitionOptions,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_style_transition_options")?;
        let options = call.reference(&options)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_transition_options(map, options, completion, out_diagnostic)
        })
    }

    /// Queues a style URL command.
    ///
    /// See `mln_map_set_style_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_style_url(&self, url: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_style_url")?;
        let url = call.input(url)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_url(map, url, completion, out_diagnostic)
        })
    }

    /// Submits a copied tile-options command.
    ///
    /// See `mln_map_set_tile_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_tile_options(
        &self,
        options: &MapTileOptions,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_tile_options")?;
        let options = call.reference(&options)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_tile_options(map, options, completion, out_diagnostic)
        })
    }

    /// Submits a copied viewport-options command.
    ///
    /// See `mln_map_set_viewport_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_viewport_options(
        &self,
        options: &MapViewportOptions,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_viewport_options")?;
        let options = call.reference(&options)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_viewport_options(map, options, completion, out_diagnostic)
        })
    }

    /// Copies the latest immutable state published by the map worker.
    ///
    /// See `mln_map_snapshot_get` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn snapshot_get(&self) -> Result<MapSnapshot> {
        let mut call = self.inner.call("mln_map_snapshot_get")?;
        let mut out_snapshot: sys::mln_map_snapshot = unsafe { std::mem::zeroed() };
        out_snapshot.size = std::mem::size_of::<sys::mln_map_snapshot>() as _;
        call.status(|map, out_diagnostic| unsafe {
            sys::mln_map_snapshot_get(map, &mut out_snapshot, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_snapshot) }?)
    }

    /// Starts an ordered copy of the last requested style URL.
    ///
    /// See `mln_map_style_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn style_url(&self) -> Result<NativeFuture<String>> {
        let call = self.inner.call("mln_map_style_url")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_style_url(map, completion, out_diagnostic)
            },
            completion::value::<sys::mln_buffer_view, _>,
        )
    }

    /// Submits one atomic camera update.
    ///
    /// See `mln_map_update_camera` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn update_camera(&self, update: &CameraUpdate) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_update_camera")?;
        let update = call.reference(&update)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_update_camera(map, update, completion, out_diagnostic)
        })
    }

    /// Starts attachment of a caller-owned Metal texture target.
    ///
    /// See `mln_metal_borrowed_texture_attach` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn metal_borrowed_texture_attach(
        &self,
        descriptor: &MetalBorrowedTextureDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call("mln_metal_borrowed_texture_attach")?;
        let parent = self.inner.parent();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_metal_borrowed_texture_attach(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session, parent)?, future))
    }

    /// Starts attachment of a session-owned Metal texture ring.
    ///
    /// See `mln_metal_owned_texture_attach` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn metal_owned_texture_attach(
        &self,
        descriptor: &MetalOwnedTextureDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call("mln_metal_owned_texture_attach")?;
        let parent = self.inner.parent();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_metal_owned_texture_attach(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session, parent)?, future))
    }

    /// Starts attachment of a Metal surface target.
    ///
    /// See `mln_metal_surface_attach` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn metal_surface_attach(
        &self,
        descriptor: &MetalSurfaceDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call("mln_metal_surface_attach")?;
        let parent = self.inner.parent();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_metal_surface_attach(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session, parent)?, future))
    }

    /// Starts attachment of a caller-owned OpenGL texture target.
    ///
    /// See `mln_opengl_borrowed_texture_attach` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn opengl_borrowed_texture_attach(
        &self,
        descriptor: &OpenglBorrowedTextureDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call("mln_opengl_borrowed_texture_attach")?;
        let parent = self.inner.parent();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_opengl_borrowed_texture_attach(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session, parent)?, future))
    }

    /// Starts attachment of a session-owned OpenGL texture ring.
    ///
    /// See `mln_opengl_owned_texture_attach` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn opengl_owned_texture_attach(
        &self,
        descriptor: &OpenglOwnedTextureDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call("mln_opengl_owned_texture_attach")?;
        let parent = self.inner.parent();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_opengl_owned_texture_attach(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session, parent)?, future))
    }

    /// Starts attachment of an OpenGL surface target.
    ///
    /// See `mln_opengl_surface_attach` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn opengl_surface_attach(
        &self,
        descriptor: &OpenglSurfaceDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call("mln_opengl_surface_attach")?;
        let parent = self.inner.parent();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_opengl_surface_attach(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session, parent)?, future))
    }

    /// Starts attachment of a caller-owned Vulkan texture target.
    ///
    /// See `mln_vulkan_borrowed_texture_attach` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn vulkan_borrowed_texture_attach(
        &self,
        descriptor: &VulkanBorrowedTextureDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call("mln_vulkan_borrowed_texture_attach")?;
        let parent = self.inner.parent();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_vulkan_borrowed_texture_attach(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session, parent)?, future))
    }

    /// Starts attachment of a session-owned Vulkan texture ring.
    ///
    /// See `mln_vulkan_owned_texture_attach` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn vulkan_owned_texture_attach(
        &self,
        descriptor: &VulkanOwnedTextureDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call("mln_vulkan_owned_texture_attach")?;
        let parent = self.inner.parent();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_vulkan_owned_texture_attach(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session, parent)?, future))
    }

    /// Starts attachment of a Vulkan surface target.
    ///
    /// See `mln_vulkan_surface_attach` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn vulkan_surface_attach(
        &self,
        descriptor: &VulkanSurfaceDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call("mln_vulkan_surface_attach")?;
        let parent = self.inner.parent();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_vulkan_surface_attach(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session, parent)?, future))
    }

    /// Starts attachment of a caller-owned WebGPU texture target.
    ///
    /// See `mln_webgpu_borrowed_texture_attach` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn webgpu_borrowed_texture_attach(
        &self,
        descriptor: &WebgpuBorrowedTextureDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call("mln_webgpu_borrowed_texture_attach")?;
        let parent = self.inner.parent();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_webgpu_borrowed_texture_attach(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session, parent)?, future))
    }

    /// Starts attachment of a session-owned WebGPU texture ring.
    ///
    /// See `mln_webgpu_owned_texture_attach` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn webgpu_owned_texture_attach(
        &self,
        descriptor: &WebgpuOwnedTextureDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call("mln_webgpu_owned_texture_attach")?;
        let parent = self.inner.parent();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_webgpu_owned_texture_attach(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session, parent)?, future))
    }

    /// Starts attachment of a WebGPU surface target.
    ///
    /// See `mln_webgpu_surface_attach` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn webgpu_surface_attach(
        &self,
        descriptor: &WebgpuSurfaceDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call("mln_webgpu_surface_attach")?;
        let parent = self.inner.parent();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_webgpu_surface_attach(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session, parent)?, future))
    }
}
