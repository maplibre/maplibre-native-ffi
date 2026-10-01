// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_map` native handle.
    pub struct MapHandle(mln_map) dispose |raw| maplibre_core::check(|out_diagnostic| unsafe { sys::mln_map_dispose(raw, out_diagnostic) });
}

impl MapHandle {
    /// Calls `mln_map_add_color_relief_layer`.
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

    /// Calls `mln_map_add_custom_geometry_source`.
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

    /// Calls `mln_map_add_custom_mvt_vector_source`.
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

    /// Calls `mln_map_add_geojson_source_data`.
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

    /// Calls `mln_map_add_geojson_source_url`.
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

    /// Calls `mln_map_add_hillshade_layer`.
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

    /// Calls `mln_map_add_image_source_image`.
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

    /// Calls `mln_map_add_image_source_url`.
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

    /// Calls `mln_map_add_location_indicator_layer`.
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

    /// Calls `mln_map_add_raster_dem_source_tiles`.
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

    /// Calls `mln_map_add_raster_dem_source_url`.
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

    /// Calls `mln_map_add_raster_source_tiles`.
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

    /// Calls `mln_map_add_raster_source_url`.
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

    /// Calls `mln_map_add_style_layer_json`.
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

    /// Calls `mln_map_add_style_source_json`.
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

    /// Calls `mln_map_add_vector_source_tiles`.
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

    /// Calls `mln_map_add_vector_source_url`.
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

    /// Calls `mln_map_apply_camera_delta`.
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

    /// Calls `mln_map_camera_for_geometry`.
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

    /// Calls `mln_map_camera_for_lat_lng_bounds`.
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

    /// Calls `mln_map_camera_for_lat_lngs`.
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

    /// Calls `mln_map_camera_query`.
    pub fn camera_query(&self) -> Result<NativeFuture<CameraQueryResult>> {
        let call = self.inner.call("mln_map_camera_query")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_camera_query(map, completion, out_diagnostic)
            },
            completion::value::<sys::mln_camera_query_result, _>,
        )
    }

    /// Calls `mln_map_camera_snapshot_get`.
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

    /// Calls `mln_map_cancel_transitions`.
    pub fn cancel_transitions(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call("mln_map_cancel_transitions")?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_cancel_transitions(map, completion, out_diagnostic)
        })
    }

    /// Calls `mln_map_copy_layer_source_id`.
    pub fn copy_layer_source_id(&self, layer_id: &str) -> Result<NativeFuture<Option<String>>> {
        let mut call = self.inner.call("mln_map_copy_layer_source_id")?;
        let layer_id = call.input(&layer_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_copy_layer_source_id(map, layer_id, completion, out_diagnostic)
            },
            |result| {
                let value = completion::copy_value::<sys::mln_buffer_view>(result)?;
                Ok(unsafe { convert::nonempty(value) }?)
            },
        )
    }

    /// Calls `mln_map_copy_layer_source_layer`.
    pub fn copy_layer_source_layer(&self, layer_id: &str) -> Result<NativeFuture<Option<String>>> {
        let mut call = self.inner.call("mln_map_copy_layer_source_layer")?;
        let layer_id = call.input(&layer_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_copy_layer_source_layer(map, layer_id, completion, out_diagnostic)
            },
            |result| {
                let value = completion::copy_value::<sys::mln_buffer_view>(result)?;
                Ok(unsafe { convert::nonempty(value) }?)
            },
        )
    }

    /// Calls `mln_map_copy_style_image_premultiplied_rgba8`.
    pub fn copy_style_image_premultiplied_rgba8(
        &self,
        image_id: &str,
    ) -> Result<NativeFuture<Option<Vec<u8>>>> {
        let mut call = self
            .inner
            .call("mln_map_copy_style_image_premultiplied_rgba8")?;
        let image_id = call.input(&image_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_copy_style_image_premultiplied_rgba8(
                    map,
                    image_id,
                    completion,
                    out_diagnostic,
                )
            },
            completion::optional::<sys::mln_buffer_view, _>,
        )
    }

    /// Calls `mln_map_copy_style_image_stretches`.
    pub fn copy_style_image_stretches(
        &self,
        image_id: &str,
    ) -> Result<NativeFuture<Option<StyleImageStretchesResult>>> {
        let mut call = self.inner.call("mln_map_copy_style_image_stretches")?;
        let image_id = call.input(&image_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_copy_style_image_stretches(map, image_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_style_image_stretches_result, _>,
        )
    }

    /// Calls `mln_map_copy_style_source_attribution`.
    pub fn copy_style_source_attribution(
        &self,
        source_id: &str,
    ) -> Result<NativeFuture<Option<String>>> {
        let mut call = self.inner.call("mln_map_copy_style_source_attribution")?;
        let source_id = call.input(&source_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_copy_style_source_attribution(
                    map,
                    source_id,
                    completion,
                    out_diagnostic,
                )
            },
            completion::optional::<sys::mln_buffer_view, _>,
        )
    }

    /// Calls `mln_map_copy_style_source_url`.
    pub fn copy_style_source_url(&self, source_id: &str) -> Result<NativeFuture<Option<String>>> {
        let mut call = self.inner.call("mln_map_copy_style_source_url")?;
        let source_id = call.input(&source_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_copy_style_source_url(map, source_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_buffer_view, _>,
        )
    }

    /// Calls `mln_map_dispose`.
    pub fn dispose(&self) -> Result<()> {
        self.inner.close(|map| {
            let mut call = Call::new(map, None);
            call.status(|map, out_diagnostic| unsafe { sys::mln_map_dispose(map, out_diagnostic) })
        })
    }

    /// Calls `mln_map_dump_debug_logs`.
    pub fn dump_debug_logs(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call("mln_map_dump_debug_logs")?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_dump_debug_logs(map, completion, out_diagnostic)
        })
    }

    /// Calls `mln_map_get_feature_state`.
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

    /// Calls `mln_map_get_global_state`.
    pub fn get_global_state(&self) -> Result<NativeFuture<Vec<u8>>> {
        let call = self.inner.call("mln_map_get_global_state")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_global_state(map, completion, out_diagnostic)
            },
            completion::value::<sys::mln_buffer_view, _>,
        )
    }

    /// Calls `mln_map_get_image_source_coordinates`.
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

    /// Calls `mln_map_get_layer_filter`.
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

    /// Calls `mln_map_get_layer_property`.
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

    /// Calls `mln_map_get_style_image_info`.
    pub fn get_style_image_info(
        &self,
        image_id: &str,
    ) -> Result<NativeFuture<Option<StyleImageResult>>> {
        let mut call = self.inner.call("mln_map_get_style_image_info")?;
        let image_id = call.input(&image_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_image_info(map, image_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_style_image_result, _>,
        )
    }

    /// Calls `mln_map_get_style_layer_info`.
    pub fn get_style_layer_info(
        &self,
        layer_id: &str,
    ) -> Result<NativeFuture<Option<StyleLayerResult>>> {
        let mut call = self.inner.call("mln_map_get_style_layer_info")?;
        let layer_id = call.input(&layer_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_layer_info(map, layer_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_style_layer_result, _>,
        )
    }

    /// Calls `mln_map_get_style_layer_json`.
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

    /// Calls `mln_map_get_style_light_property`.
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

    /// Calls `mln_map_get_style_source_info`.
    pub fn get_style_source_info(
        &self,
        source_id: &str,
    ) -> Result<NativeFuture<Option<StyleSourceResult>>> {
        let mut call = self.inner.call("mln_map_get_style_source_info")?;
        let source_id = call.input(&source_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_source_info(map, source_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_style_source_result, _>,
        )
    }

    /// Calls `mln_map_get_style_source_tile_urls`.
    pub fn get_style_source_tile_urls(
        &self,
        source_id: &str,
    ) -> Result<NativeFuture<Option<StyleSourceTileUrlsResult>>> {
        let mut call = self.inner.call("mln_map_get_style_source_tile_urls")?;
        let source_id = call.input(&source_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_source_tile_urls(map, source_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_style_source_tile_urls_result, _>,
        )
    }

    /// Calls `mln_map_get_style_transition_options`.
    pub fn get_style_transition_options(&self) -> Result<NativeFuture<StyleTransitionOptions>> {
        let call = self.inner.call("mln_map_get_style_transition_options")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_transition_options(map, completion, out_diagnostic)
            },
            completion::value::<sys::mln_style_transition_options, _>,
        )
    }

    /// Calls `mln_map_invalidate_custom_geometry_source_region`.
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

    /// Calls `mln_map_invalidate_custom_geometry_source_tile`.
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

    /// Calls `mln_map_invalidate_custom_mvt_vector_source_tile`.
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

    /// Calls `mln_map_lat_lng_bounds_for_camera`.
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

    /// Calls `mln_map_lat_lng_bounds_for_camera_unwrapped`.
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

    /// Calls `mln_map_lat_lng_for_pixel`.
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

    /// Calls `mln_map_lat_lng_for_pixel_unwrapped`.
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

    /// Calls `mln_map_lat_lngs_for_pixels`.
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

    /// Calls `mln_map_lat_lngs_for_pixels_unwrapped`.
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

    /// Calls `mln_map_list_style_layer_ids`.
    pub fn list_style_layer_ids(&self) -> Result<NativeFuture<Vec<String>>> {
        let call = self.inner.call("mln_map_list_style_layer_ids")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_list_style_layer_ids(map, completion, out_diagnostic)
            },
            completion::list::<sys::mln_buffer_view, _>,
        )
    }

    /// Calls `mln_map_list_style_layers`.
    pub fn list_style_layers(&self) -> Result<NativeFuture<Vec<StyleLayerEntry>>> {
        let call = self.inner.call("mln_map_list_style_layers")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_list_style_layers(map, completion, out_diagnostic)
            },
            completion::list::<sys::mln_style_layer_entry, _>,
        )
    }

    /// Calls `mln_map_list_style_source_ids`.
    pub fn list_style_source_ids(&self) -> Result<NativeFuture<Vec<String>>> {
        let call = self.inner.call("mln_map_list_style_source_ids")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_list_style_source_ids(map, completion, out_diagnostic)
            },
            completion::list::<sys::mln_buffer_view, _>,
        )
    }

    /// Calls `mln_map_loaded_style_json`.
    pub fn loaded_style_json(&self) -> Result<NativeFuture<Vec<u8>>> {
        let call = self.inner.call("mln_map_loaded_style_json")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_loaded_style_json(map, completion, out_diagnostic)
            },
            completion::value::<sys::mln_buffer_view, _>,
        )
    }

    /// Calls `mln_map_meters_per_pixel_at_latitude`.
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

    /// Calls `mln_map_move_style_layer`.
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

    /// Calls `mln_map_pixel_for_lat_lng`.
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

    /// Calls `mln_map_pixels_for_lat_lngs`.
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

    /// Calls `mln_map_projection_create`.
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

    /// Calls `mln_map_release`.
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

    /// Calls `mln_map_remove_feature_state`.
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

    /// Calls `mln_map_remove_style_image`.
    pub fn remove_style_image(&self, image_id: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_remove_style_image")?;
        let image_id = call.input(&image_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_style_image(map, image_id, completion, out_diagnostic)
        })
    }

    /// Calls `mln_map_remove_style_layer`.
    pub fn remove_style_layer(&self, layer_id: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_remove_style_layer")?;
        let layer_id = call.input(&layer_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_style_layer(map, layer_id, completion, out_diagnostic)
        })
    }

    /// Calls `mln_map_remove_style_source`.
    pub fn remove_style_source(&self, source_id: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_remove_style_source")?;
        let source_id = call.input(&source_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_style_source(map, source_id, completion, out_diagnostic)
        })
    }

    /// Calls `mln_map_request_repaint`.
    pub fn request_repaint(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call("mln_map_request_repaint")?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_request_repaint(map, completion, out_diagnostic)
        })
    }

    /// Calls `mln_map_request_still_image`.
    pub fn request_still_image(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call("mln_map_request_still_image")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_request_still_image(map, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Calls `mln_map_resize`.
    pub fn resize(&self, extent: LogicalExtent) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_resize")?;
        let extent = call.input(&extent)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_resize(map, extent, completion, out_diagnostic)
        })
    }

    /// Calls `mln_map_set_bounds`.
    pub fn set_bounds(&self, options: &BoundOptions) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_bounds")?;
        let options = call.reference(&options)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_bounds(map, options, completion, out_diagnostic)
        })
    }

    /// Calls `mln_map_set_custom_geometry_source_tile_data`.
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

    /// Calls `mln_map_set_custom_mvt_vector_source_tile_data`.
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

    /// Calls `mln_map_set_custom_mvt_vector_source_tile_error`.
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

    /// Calls `mln_map_set_debug_options`.
    pub fn set_debug_options(
        &self,
        options: MapDebugOption,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call("mln_map_set_debug_options")?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_debug_options(map, options.to_native(), completion, out_diagnostic)
        })
    }

    /// Calls `mln_map_set_event_mask`.
    pub fn set_event_mask(
        &self,
        mask: RuntimeEventMask,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call("mln_map_set_event_mask")?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_event_mask(map, mask.to_native(), completion, out_diagnostic)
        })
    }

    /// Calls `mln_map_set_feature_state`.
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

    /// Calls `mln_map_set_free_camera_options`.
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

    /// Calls `mln_map_set_geojson_source_data`.
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

    /// Calls `mln_map_set_geojson_source_synchronous_tiling`.
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

    /// Calls `mln_map_set_geojson_source_url`.
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

    /// Calls `mln_map_set_global_state_property`.
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

    /// Calls `mln_map_set_image_source_coordinates`.
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

    /// Calls `mln_map_set_image_source_image`.
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

    /// Calls `mln_map_set_image_source_url`.
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

    /// Calls `mln_map_set_layer_filter`.
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

    /// Calls `mln_map_set_layer_max_zoom`.
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

    /// Calls `mln_map_set_layer_min_zoom`.
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

    /// Calls `mln_map_set_layer_property`.
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

    /// Calls `mln_map_set_layer_source_id`.
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

    /// Calls `mln_map_set_layer_source_layer`.
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

    /// Calls `mln_map_set_layer_visibility`.
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

    /// Calls `mln_map_set_location_indicator_accuracy_radius`.
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

    /// Calls `mln_map_set_location_indicator_bearing`.
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

    /// Calls `mln_map_set_location_indicator_image_name`.
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

    /// Calls `mln_map_set_location_indicator_location`.
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

    /// Calls `mln_map_set_projection_mode`.
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

    /// Calls `mln_map_set_rendering_stats_view_enabled`.
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

    /// Calls `mln_map_set_style_image`.
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

    /// Calls `mln_map_set_style_json`.
    pub fn set_style_json(&self, json: &[u8]) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_style_json")?;
        let json = call.input(&json)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_json(map, json, completion, out_diagnostic)
        })
    }

    /// Calls `mln_map_set_style_light_json`.
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

    /// Calls `mln_map_set_style_light_property`.
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

    /// Calls `mln_map_set_style_source_volatile`.
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

    /// Calls `mln_map_set_style_transition_options`.
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

    /// Calls `mln_map_set_style_url`.
    pub fn set_style_url(&self, url: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_set_style_url")?;
        let url = call.input(url)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_url(map, url, completion, out_diagnostic)
        })
    }

    /// Calls `mln_map_set_tile_options`.
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

    /// Calls `mln_map_set_viewport_options`.
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

    /// Calls `mln_map_snapshot_get`.
    pub fn snapshot_get(&self) -> Result<MapSnapshot> {
        let mut call = self.inner.call("mln_map_snapshot_get")?;
        let mut out_snapshot: sys::mln_map_snapshot = unsafe { std::mem::zeroed() };
        out_snapshot.size = std::mem::size_of::<sys::mln_map_snapshot>() as _;
        call.status(|map, out_diagnostic| unsafe {
            sys::mln_map_snapshot_get(map, &mut out_snapshot, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_snapshot) }?)
    }

    /// Calls `mln_map_style_url`.
    pub fn style_url(&self) -> Result<NativeFuture<String>> {
        let call = self.inner.call("mln_map_style_url")?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_style_url(map, completion, out_diagnostic)
            },
            completion::value::<sys::mln_buffer_view, _>,
        )
    }

    /// Calls `mln_map_update_camera`.
    pub fn update_camera(&self, update: &CameraUpdate) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_map_update_camera")?;
        let update = call.reference(&update)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_update_camera(map, update, completion, out_diagnostic)
        })
    }

    /// Calls `mln_metal_borrowed_texture_attach`.
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

    /// Calls `mln_metal_owned_texture_attach`.
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

    /// Calls `mln_metal_surface_attach`.
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

    /// Calls `mln_opengl_borrowed_texture_attach`.
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

    /// Calls `mln_opengl_owned_texture_attach`.
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

    /// Calls `mln_opengl_surface_attach`.
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

    /// Calls `mln_vulkan_borrowed_texture_attach`.
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

    /// Calls `mln_vulkan_owned_texture_attach`.
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

    /// Calls `mln_vulkan_surface_attach`.
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

    /// Calls `mln_webgpu_borrowed_texture_attach`.
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

    /// Calls `mln_webgpu_owned_texture_attach`.
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

    /// Calls `mln_webgpu_surface_attach`.
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
