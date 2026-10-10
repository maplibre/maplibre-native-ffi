// Generated from the C headers by tools/bindgen. Do not edit.

fn generated_copy_mln_animation_options(
    py: Python<'_>,
    value: &sys::mln_animation_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "duration_ms",
        generated_optional(
            py,
            value.fields & sys::MLN_ANIMATION_OPTION_DURATION != 0,
            || generated_value(py, value.duration_ms),
        )?,
    )?;
    dict.set_item(
        "velocity",
        generated_optional(
            py,
            value.fields & sys::MLN_ANIMATION_OPTION_VELOCITY != 0,
            || generated_value(py, value.velocity),
        )?,
    )?;
    dict.set_item(
        "min_zoom",
        generated_optional(
            py,
            value.fields & sys::MLN_ANIMATION_OPTION_MIN_ZOOM != 0,
            || generated_value(py, value.min_zoom),
        )?,
    )?;
    dict.set_item(
        "easing",
        generated_optional(
            py,
            value.fields & sys::MLN_ANIMATION_OPTION_EASING != 0,
            || generated_copy_mln_unit_bezier(py, &value.easing),
        )?,
    )?;
    dict.set_item(
        "transition_id",
        generated_optional(
            py,
            value.fields & sys::MLN_ANIMATION_OPTION_TRANSITION_ID != 0,
            || generated_value(py, value.transition_id),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_bound_options(
    py: Python<'_>,
    value: &sys::mln_bound_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "bounds",
        generated_optional(py, value.fields & sys::MLN_BOUND_OPTION_BOUNDS != 0, || {
            generated_copy_mln_lat_lng_bounds(py, &value.bounds)
        })?,
    )?;
    dict.set_item(
        "min_zoom",
        generated_optional(
            py,
            value.fields & sys::MLN_BOUND_OPTION_MIN_ZOOM != 0,
            || generated_value(py, value.min_zoom),
        )?,
    )?;
    dict.set_item(
        "max_zoom",
        generated_optional(
            py,
            value.fields & sys::MLN_BOUND_OPTION_MAX_ZOOM != 0,
            || generated_value(py, value.max_zoom),
        )?,
    )?;
    dict.set_item(
        "min_pitch",
        generated_optional(
            py,
            value.fields & sys::MLN_BOUND_OPTION_MIN_PITCH != 0,
            || generated_value(py, value.min_pitch),
        )?,
    )?;
    dict.set_item(
        "max_pitch",
        generated_optional(
            py,
            value.fields & sys::MLN_BOUND_OPTION_MAX_PITCH != 0,
            || generated_value(py, value.max_pitch),
        )?,
    )?;
    dict.set_item(
        "unbounded",
        value.fields & sys::MLN_BOUND_OPTION_UNBOUNDED != 0,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_camera_delta(
    py: Python<'_>,
    value: &sys::mln_camera_delta,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("kind", generated_value(py, value.kind)?)?;
    dict.set_item(
        "offset",
        generated_copy_mln_screen_point(py, &value.offset)?,
    )?;
    dict.set_item("amount", generated_value(py, value.amount)?)?;
    dict.set_item(
        "anchor",
        generated_optional(py, value.has_anchor, || {
            generated_copy_mln_screen_point(py, &value.anchor)
        })?,
    )?;
    dict.set_item(
        "animation",
        generated_copy_mln_animation_options(py, &value.animation)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_camera_fit_options(
    py: Python<'_>,
    value: &sys::mln_camera_fit_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "padding",
        generated_optional(
            py,
            value.fields & sys::MLN_CAMERA_FIT_OPTION_PADDING != 0,
            || generated_copy_mln_edge_insets(py, &value.padding),
        )?,
    )?;
    dict.set_item(
        "bearing",
        generated_optional(
            py,
            value.fields & sys::MLN_CAMERA_FIT_OPTION_BEARING != 0,
            || generated_value(py, value.bearing),
        )?,
    )?;
    dict.set_item(
        "pitch",
        generated_optional(
            py,
            value.fields & sys::MLN_CAMERA_FIT_OPTION_PITCH != 0,
            || generated_value(py, value.pitch),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_camera_options(
    py: Python<'_>,
    value: &sys::mln_camera_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "center",
        generated_optional(
            py,
            value.fields & sys::MLN_CAMERA_OPTION_CENTER != 0,
            || {
                Ok({
                    let inner = PyDict::new(py);
                    inner.set_item("latitude", generated_value(py, value.latitude)?)?;
                    inner.set_item("longitude", generated_value(py, value.longitude)?)?;
                    inner.into_any().unbind()
                })
            },
        )?,
    )?;
    dict.set_item(
        "center_altitude",
        generated_optional(
            py,
            value.fields & sys::MLN_CAMERA_OPTION_CENTER_ALTITUDE != 0,
            || generated_value(py, value.center_altitude),
        )?,
    )?;
    dict.set_item(
        "padding",
        generated_optional(
            py,
            value.fields & sys::MLN_CAMERA_OPTION_PADDING != 0,
            || generated_copy_mln_edge_insets(py, &value.padding),
        )?,
    )?;
    dict.set_item(
        "anchor",
        generated_optional(
            py,
            value.fields & sys::MLN_CAMERA_OPTION_ANCHOR != 0,
            || generated_copy_mln_screen_point(py, &value.anchor),
        )?,
    )?;
    dict.set_item(
        "zoom",
        generated_optional(py, value.fields & sys::MLN_CAMERA_OPTION_ZOOM != 0, || {
            generated_value(py, value.zoom)
        })?,
    )?;
    dict.set_item(
        "bearing",
        generated_optional(
            py,
            value.fields & sys::MLN_CAMERA_OPTION_BEARING != 0,
            || generated_value(py, value.bearing),
        )?,
    )?;
    dict.set_item(
        "pitch",
        generated_optional(py, value.fields & sys::MLN_CAMERA_OPTION_PITCH != 0, || {
            generated_value(py, value.pitch)
        })?,
    )?;
    dict.set_item(
        "roll",
        generated_optional(py, value.fields & sys::MLN_CAMERA_OPTION_ROLL != 0, || {
            generated_value(py, value.roll)
        })?,
    )?;
    dict.set_item(
        "field_of_view",
        generated_optional(py, value.fields & sys::MLN_CAMERA_OPTION_FOV != 0, || {
            generated_value(py, value.field_of_view)
        })?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_camera_query_result(
    py: Python<'_>,
    value: &sys::mln_camera_query_result,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("generation", generated_value(py, value.generation)?)?;
    dict.set_item(
        "camera",
        generated_copy_mln_camera_options(py, &value.camera)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_camera_update(
    py: Python<'_>,
    value: &sys::mln_camera_update,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("mode", generated_value(py, value.mode)?)?;
    dict.set_item(
        "camera",
        generated_copy_mln_camera_options(py, &value.camera)?,
    )?;
    dict.set_item(
        "animation",
        generated_copy_mln_animation_options(py, &value.animation)?,
    )?;
    dict.set_item("gesture_phase", generated_value(py, value.gesture_phase)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_canonical_tile_id(
    py: Python<'_>,
    value: &sys::mln_canonical_tile_id,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("z", generated_value(py, value.z)?)?;
    dict.set_item("x", generated_value(py, value.x)?)?;
    dict.set_item("y", generated_value(py, value.y)?)?;
    Ok(dict.into_any().unbind())
}

unsafe extern "C" fn generated_callback_mln_custom_geometry_source_options_fetch_tile(
    user_data: *mut std::ffi::c_void,
    tile_id: sys::mln_canonical_tile_id,
) {
    generated_invoke(
        || (),
        |py| {
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 0) }) else {
                return Ok(());
            };
            let _result = callback
                .bind(py)
                .call1((generated_copy_mln_canonical_tile_id(py, &tile_id)?,))?;
            Ok(())
        },
    )
}

unsafe extern "C" fn generated_callback_mln_custom_geometry_source_options_cancel_tile(
    user_data: *mut std::ffi::c_void,
    tile_id: sys::mln_canonical_tile_id,
) {
    generated_invoke(
        || (),
        |py| {
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 1) }) else {
                return Ok(());
            };
            let _result = callback
                .bind(py)
                .call1((generated_copy_mln_canonical_tile_id(py, &tile_id)?,))?;
            Ok(())
        },
    )
}

fn generated_copy_mln_custom_geometry_source_options(
    py: Python<'_>,
    value: &sys::mln_custom_geometry_source_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("fetch_tile", py.None())?;
    if value.fetch_tile.is_some() {
        return Err(native_error(
            "native callback cannot be copied into a Python closure",
        ));
    }
    dict.set_item("cancel_tile", py.None())?;
    if value.cancel_tile.is_some() {
        return Err(native_error(
            "native callback cannot be copied into a Python closure",
        ));
    }
    dict.set_item(
        "min_zoom",
        generated_optional(
            py,
            value.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM != 0,
            || generated_value(py, value.min_zoom),
        )?,
    )?;
    dict.set_item(
        "max_zoom",
        generated_optional(
            py,
            value.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM != 0,
            || generated_value(py, value.max_zoom),
        )?,
    )?;
    dict.set_item(
        "tolerance",
        generated_optional(
            py,
            value.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE != 0,
            || generated_value(py, value.tolerance),
        )?,
    )?;
    dict.set_item(
        "tile_size",
        generated_optional(
            py,
            value.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE != 0,
            || generated_value(py, value.tile_size),
        )?,
    )?;
    dict.set_item(
        "buffer",
        generated_optional(
            py,
            value.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER != 0,
            || generated_value(py, value.buffer),
        )?,
    )?;
    dict.set_item(
        "clip",
        generated_optional(
            py,
            value.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP != 0,
            || generated_value(py, value.clip),
        )?,
    )?;
    dict.set_item(
        "wrap",
        generated_optional(
            py,
            value.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP != 0,
            || generated_value(py, value.wrap),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

unsafe extern "C" fn generated_callback_mln_custom_mvt_vector_source_options_fetch_tile(
    user_data: *mut std::ffi::c_void,
    tile_id: sys::mln_canonical_tile_id,
) {
    generated_invoke(
        || (),
        |py| {
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 0) }) else {
                return Ok(());
            };
            let _result = callback
                .bind(py)
                .call1((generated_copy_mln_canonical_tile_id(py, &tile_id)?,))?;
            Ok(())
        },
    )
}

unsafe extern "C" fn generated_callback_mln_custom_mvt_vector_source_options_cancel_tile(
    user_data: *mut std::ffi::c_void,
    tile_id: sys::mln_canonical_tile_id,
) {
    generated_invoke(
        || (),
        |py| {
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 1) }) else {
                return Ok(());
            };
            let _result = callback
                .bind(py)
                .call1((generated_copy_mln_canonical_tile_id(py, &tile_id)?,))?;
            Ok(())
        },
    )
}

fn generated_copy_mln_custom_mvt_vector_source_options(
    py: Python<'_>,
    value: &sys::mln_custom_mvt_vector_source_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("fetch_tile", py.None())?;
    if value.fetch_tile.is_some() {
        return Err(native_error(
            "native callback cannot be copied into a Python closure",
        ));
    }
    dict.set_item("cancel_tile", py.None())?;
    if value.cancel_tile.is_some() {
        return Err(native_error(
            "native callback cannot be copied into a Python closure",
        ));
    }
    dict.set_item(
        "min_zoom",
        generated_optional(
            py,
            value.fields & sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM != 0,
            || generated_value(py, value.min_zoom),
        )?,
    )?;
    dict.set_item(
        "max_zoom",
        generated_optional(
            py,
            value.fields & sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM != 0,
            || generated_value(py, value.max_zoom),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_edge_insets(
    py: Python<'_>,
    value: &sys::mln_edge_insets,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("top", generated_value(py, value.top)?)?;
    dict.set_item("left", generated_value(py, value.left)?)?;
    dict.set_item("bottom", generated_value(py, value.bottom)?)?;
    dict.set_item("right", generated_value(py, value.right)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_egl_context_descriptor(
    py: Python<'_>,
    value: &sys::mln_egl_context_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("display", generated_value(py, value.display as usize)?)?;
    dict.set_item("config", generated_value(py, value.config as usize)?)?;
    dict.set_item(
        "share_context",
        generated_value(py, value.share_context as usize)?,
    )?;
    dict.set_item("client_api", generated_value(py, value.client_api)?)?;
    dict.set_item(
        "get_proc_address",
        generated_value(py, value.get_proc_address as usize)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_frame_demand(
    py: Python<'_>,
    value: &sys::mln_frame_demand,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("flags", generated_value(py, value.flags)?)?;
    dict.set_item("token", generated_value(py, value.token)?)?;
    dict.set_item(
        "coalescing_boundary",
        generated_value(py, value.coalescing_boundary)?,
    )?;
    dict.set_item("timeout_ns", generated_value(py, value.timeout_ns)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_free_camera_options(
    py: Python<'_>,
    value: &sys::mln_free_camera_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "position",
        generated_optional(
            py,
            value.fields & sys::MLN_FREE_CAMERA_OPTION_POSITION != 0,
            || generated_copy_mln_vec3(py, &value.position),
        )?,
    )?;
    dict.set_item(
        "orientation",
        generated_optional(
            py,
            value.fields & sys::MLN_FREE_CAMERA_OPTION_ORIENTATION != 0,
            || generated_copy_mln_quaternion(py, &value.orientation),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_geojson_source_options(
    py: Python<'_>,
    value: &sys::mln_geojson_source_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "min_zoom",
        generated_optional(
            py,
            value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM != 0,
            || generated_value(py, value.min_zoom),
        )?,
    )?;
    dict.set_item(
        "max_zoom",
        generated_optional(
            py,
            value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM != 0,
            || generated_value(py, value.max_zoom),
        )?,
    )?;
    dict.set_item(
        "tolerance",
        generated_optional(
            py,
            value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_TOLERANCE != 0,
            || generated_value(py, value.tolerance),
        )?,
    )?;
    dict.set_item(
        "cluster_max_zoom",
        generated_optional(
            py,
            value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM != 0,
            || generated_value(py, value.cluster_max_zoom),
        )?,
    )?;
    dict.set_item(
        "cluster_properties",
        generated_optional(
            py,
            value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES != 0,
            || unsafe { generated_bytes(py, value.cluster_properties) },
        )?,
    )?;
    dict.set_item(
        "tile_size",
        generated_optional(
            py,
            value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE != 0,
            || generated_value(py, value.tile_size),
        )?,
    )?;
    dict.set_item(
        "buffer",
        generated_optional(
            py,
            value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_BUFFER != 0,
            || generated_value(py, value.buffer),
        )?,
    )?;
    dict.set_item(
        "cluster_radius",
        generated_optional(
            py,
            value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS != 0,
            || generated_value(py, value.cluster_radius),
        )?,
    )?;
    dict.set_item(
        "cluster_min_points",
        generated_optional(
            py,
            value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS != 0,
            || generated_value(py, value.cluster_min_points),
        )?,
    )?;
    dict.set_item(
        "line_metrics",
        generated_optional(
            py,
            value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS != 0,
            || generated_value(py, value.line_metrics),
        )?,
    )?;
    dict.set_item(
        "cluster",
        generated_optional(
            py,
            value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER != 0,
            || generated_value(py, value.cluster),
        )?,
    )?;
    dict.set_item(
        "synchronous_tiling",
        generated_optional(
            py,
            value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING != 0,
            || generated_value(py, value.synchronous_tiling),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_gpu_sync(py: Python<'_>, value: &sys::mln_gpu_sync) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("kind", generated_value(py, value.kind)?)?;
    dict.set_item("object", generated_value(py, value.object)?)?;
    dict.set_item("value", generated_value(py, value.value)?)?;
    Ok(dict.into_any().unbind())
}

unsafe extern "C" fn generated_callback_mln_http_header_transform_callback(
    user_data: *mut std::ffi::c_void,
    kind: sys::mln_resource_kind,
    url: *const std::ffi::c_char,
    out_response: *mut sys::mln_http_header_transform_response,
) -> sys::mln_status {
    let _policy = GeneratedCallbackPolicy::enter(
        &["mln_http_header_transform_response_set"],
        out_response as usize as u64,
    );

    generated_invoke(
        || sys::MLN_STATUS_NATIVE_ERROR,
        |py| {
            let callback_scope = GeneratedCallbackGuard::new();
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 0) }) else {
                return Ok(sys::MLN_STATUS_NATIVE_ERROR);
            };
            let result = callback.bind(py).call1((
                generated_value(py, kind)?,
                unsafe { generated_c_string(py, url, false) }?,
                Py::new(
                    py,
                    HttpHeaderTransformResponseScope {
                        native: out_response as usize,
                        scope: callback_scope.scope(),
                    },
                )?,
            ))?;
            result.extract::<sys::mln_status>()
        },
    )
}

fn generated_copy_mln_image_content(
    py: Python<'_>,
    value: &sys::mln_image_content,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("left", generated_value(py, value.left)?)?;
    dict.set_item("top", generated_value(py, value.top)?)?;
    dict.set_item("right", generated_value(py, value.right)?)?;
    dict.set_item("bottom", generated_value(py, value.bottom)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_image_stretch(
    py: Python<'_>,
    value: &sys::mln_image_stretch,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("from_", generated_value(py, value.from)?)?;
    dict.set_item("to", generated_value(py, value.to)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_lat_lng(py: Python<'_>, value: &sys::mln_lat_lng) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("latitude", generated_value(py, value.latitude)?)?;
    dict.set_item("longitude", generated_value(py, value.longitude)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_lat_lng_bounds(
    py: Python<'_>,
    value: &sys::mln_lat_lng_bounds,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "southwest",
        generated_copy_mln_lat_lng(py, &value.southwest)?,
    )?;
    dict.set_item(
        "northeast",
        generated_copy_mln_lat_lng(py, &value.northeast)?,
    )?;
    Ok(dict.into_any().unbind())
}

unsafe extern "C" fn generated_callback_mln_log_set_callback_registration_callback(
    user_data: *mut std::ffi::c_void,
    severity: sys::mln_log_severity,
    event: sys::mln_log_event,
    code: i64,
    message: *const std::ffi::c_char,
) -> u32 {
    let _reentry = GeneratedCallbackPolicy::enter(&[], 0);

    generated_invoke(
        || 0,
        |py| {
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 0) }) else {
                return Ok(0);
            };
            let result = callback.bind(py).call1((
                generated_value(py, severity)?,
                generated_value(py, event)?,
                generated_value(py, code)?,
                unsafe { generated_c_string(py, message, false) }?,
            ))?;
            result.extract::<u32>()
        },
    )
}

fn generated_copy_mln_logical_extent(
    py: Python<'_>,
    value: &sys::mln_logical_extent,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("width", generated_value(py, value.width)?)?;
    dict.set_item("height", generated_value(py, value.height)?)?;
    dict.set_item("scale_factor", generated_value(py, value.scale_factor)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_map_options(
    py: Python<'_>,
    value: &sys::mln_map_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "initial_extent",
        generated_copy_mln_logical_extent(py, &value.initial_extent)?,
    )?;
    dict.set_item("map_mode", generated_value(py, value.map_mode)?)?;
    dict.set_item(
        "fast_pfor_enabled",
        generated_value(py, value.fast_pfor_enabled)?,
    )?;
    dict.set_item("event_mask", generated_value(py, value.event_mask)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_map_snapshot(
    py: Python<'_>,
    value: &sys::mln_map_snapshot,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("debug_options", generated_value(py, value.debug_options)?)?;
    dict.set_item("generation", generated_value(py, value.generation)?)?;
    dict.set_item(
        "camera",
        generated_copy_mln_camera_options(py, &value.camera)?,
    )?;
    dict.set_item(
        "logical_extent",
        generated_copy_mln_logical_extent(py, &value.logical_extent)?,
    )?;
    dict.set_item(
        "projection_mode",
        generated_copy_mln_projection_mode(py, &value.projection_mode)?,
    )?;
    dict.set_item(
        "viewport",
        generated_copy_mln_map_viewport_options(py, &value.viewport)?,
    )?;
    dict.set_item("fully_loaded", generated_value(py, value.fully_loaded)?)?;
    dict.set_item(
        "rendering_stats_view_enabled",
        generated_value(py, value.rendering_stats_view_enabled)?,
    )?;
    dict.set_item("repaint_demand", generated_value(py, value.repaint_demand)?)?;
    dict.set_item(
        "gesture_in_progress",
        generated_value(py, value.gesture_in_progress)?,
    )?;
    dict.set_item("event_mask", generated_value(py, value.event_mask)?)?;
    dict.set_item(
        "latest_render_update_generation",
        generated_value(py, value.latest_render_update_generation)?,
    )?;
    dict.set_item(
        "tile",
        generated_copy_mln_map_tile_options(py, &value.tile)?,
    )?;
    dict.set_item(
        "bounds",
        generated_copy_mln_bound_options(py, &value.bounds)?,
    )?;
    dict.set_item(
        "free_camera",
        generated_copy_mln_free_camera_options(py, &value.free_camera)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_map_tile_options(
    py: Python<'_>,
    value: &sys::mln_map_tile_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "prefetch_zoom_delta",
        generated_optional(
            py,
            value.fields & sys::MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA != 0,
            || generated_value(py, value.prefetch_zoom_delta),
        )?,
    )?;
    dict.set_item(
        "lod_min_radius",
        generated_optional(
            py,
            value.fields & sys::MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS != 0,
            || generated_value(py, value.lod_min_radius),
        )?,
    )?;
    dict.set_item(
        "lod_scale",
        generated_optional(
            py,
            value.fields & sys::MLN_MAP_TILE_OPTION_LOD_SCALE != 0,
            || generated_value(py, value.lod_scale),
        )?,
    )?;
    dict.set_item(
        "lod_pitch_threshold",
        generated_optional(
            py,
            value.fields & sys::MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD != 0,
            || generated_value(py, value.lod_pitch_threshold),
        )?,
    )?;
    dict.set_item(
        "lod_zoom_shift",
        generated_optional(
            py,
            value.fields & sys::MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT != 0,
            || generated_value(py, value.lod_zoom_shift),
        )?,
    )?;
    dict.set_item(
        "lod_mode",
        generated_optional(
            py,
            value.fields & sys::MLN_MAP_TILE_OPTION_LOD_MODE != 0,
            || generated_value(py, value.lod_mode),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_map_viewport_options(
    py: Python<'_>,
    value: &sys::mln_map_viewport_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "north_orientation",
        generated_optional(
            py,
            value.fields & sys::MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION != 0,
            || generated_value(py, value.north_orientation),
        )?,
    )?;
    dict.set_item(
        "constrain_mode",
        generated_optional(
            py,
            value.fields & sys::MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE != 0,
            || generated_value(py, value.constrain_mode),
        )?,
    )?;
    dict.set_item(
        "viewport_mode",
        generated_optional(
            py,
            value.fields & sys::MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE != 0,
            || generated_value(py, value.viewport_mode),
        )?,
    )?;
    dict.set_item(
        "frustum_offset",
        generated_optional(
            py,
            value.fields & sys::MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET != 0,
            || generated_copy_mln_edge_insets(py, &value.frustum_offset),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_metal_borrowed_texture_descriptor(
    py: Python<'_>,
    value: &sys::mln_metal_borrowed_texture_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "extent",
        generated_copy_mln_render_target_extent(py, &value.extent)?,
    )?;
    dict.set_item("physical_width", generated_value(py, value.physical_width)?)?;
    dict.set_item(
        "physical_height",
        generated_value(py, value.physical_height)?,
    )?;
    dict.set_item("texture", generated_value(py, value.texture as usize)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_metal_context_descriptor(
    py: Python<'_>,
    value: &sys::mln_metal_context_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("device", generated_value(py, value.device as usize)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_metal_owned_texture_descriptor(
    py: Python<'_>,
    value: &sys::mln_metal_owned_texture_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "extent",
        generated_copy_mln_render_target_extent(py, &value.extent)?,
    )?;
    dict.set_item(
        "context",
        generated_copy_mln_metal_context_descriptor(py, &value.context)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_metal_owned_texture_frame(
    py: Python<'_>,
    value: &sys::mln_metal_owned_texture_frame,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("generation", generated_value(py, value.generation)?)?;
    dict.set_item("width", generated_value(py, value.width)?)?;
    dict.set_item("height", generated_value(py, value.height)?)?;
    dict.set_item("scale_factor", generated_value(py, value.scale_factor)?)?;
    dict.set_item("frame_id", generated_value(py, value.frame_id)?)?;
    dict.set_item("texture", generated_value(py, value.texture as usize)?)?;
    dict.set_item("device", generated_value(py, value.device as usize)?)?;
    dict.set_item("pixel_format", generated_value(py, value.pixel_format)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_metal_surface_descriptor(
    py: Python<'_>,
    value: &sys::mln_metal_surface_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "extent",
        generated_copy_mln_render_target_extent(py, &value.extent)?,
    )?;
    dict.set_item(
        "context",
        generated_copy_mln_metal_context_descriptor(py, &value.context)?,
    )?;
    dict.set_item("layer", generated_value(py, value.layer as usize)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_offline_geometry_region_definition(
    py: Python<'_>,
    value: &sys::mln_offline_geometry_region_definition,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("style_url", unsafe {
        generated_c_string(py, value.style_url, false)
    }?)?;
    dict.set_item("geometry", unsafe { generated_bytes(py, value.geometry) }?)?;
    dict.set_item("min_zoom", generated_value(py, value.min_zoom)?)?;
    dict.set_item("max_zoom", generated_value(py, value.max_zoom)?)?;
    dict.set_item("pixel_ratio", generated_value(py, value.pixel_ratio)?)?;
    dict.set_item(
        "include_ideographs",
        generated_value(py, value.include_ideographs)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_offline_region_definition(
    py: Python<'_>,
    value: &sys::mln_offline_region_definition,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "data",
        match value.type_ {
            sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID => generated_variant(
                py,
                "tile_pyramid",
                generated_copy_mln_offline_tile_pyramid_region_definition(
                    py,
                    &(unsafe { value.data.tile_pyramid }),
                )?,
            )?,
            sys::MLN_OFFLINE_REGION_DEFINITION_GEOMETRY => generated_variant(
                py,
                "geometry",
                generated_copy_mln_offline_geometry_region_definition(
                    py,
                    &(unsafe { value.data.geometry }),
                )?,
            )?,
            tag => generated_unknown_variant(py, tag)?,
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_offline_region_info(
    py: Python<'_>,
    value: &sys::mln_offline_region_info,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("id", generated_value(py, value.id)?)?;
    dict.set_item(
        "definition",
        generated_copy_mln_offline_region_definition(py, &value.definition)?,
    )?;
    dict.set_item("metadata", unsafe {
        generated_bytes(py, generated_view(value.metadata, value.metadata_size)?)
    }?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_offline_region_status(
    py: Python<'_>,
    value: &sys::mln_offline_region_status,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("download_state", generated_value(py, value.download_state)?)?;
    dict.set_item(
        "completed_resource_count",
        generated_value(py, value.completed_resource_count)?,
    )?;
    dict.set_item(
        "completed_resource_size",
        generated_value(py, value.completed_resource_size)?,
    )?;
    dict.set_item(
        "completed_tile_count",
        generated_value(py, value.completed_tile_count)?,
    )?;
    dict.set_item(
        "required_tile_count",
        generated_value(py, value.required_tile_count)?,
    )?;
    dict.set_item(
        "completed_tile_size",
        generated_value(py, value.completed_tile_size)?,
    )?;
    dict.set_item(
        "required_resource_count",
        generated_value(py, value.required_resource_count)?,
    )?;
    dict.set_item(
        "required_resource_count_is_precise",
        generated_value(py, value.required_resource_count_is_precise)?,
    )?;
    dict.set_item("complete", generated_value(py, value.complete)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_offline_tile_pyramid_region_definition(
    py: Python<'_>,
    value: &sys::mln_offline_tile_pyramid_region_definition,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("style_url", unsafe {
        generated_c_string(py, value.style_url, false)
    }?)?;
    dict.set_item(
        "bounds",
        generated_copy_mln_lat_lng_bounds(py, &value.bounds)?,
    )?;
    dict.set_item("min_zoom", generated_value(py, value.min_zoom)?)?;
    dict.set_item("max_zoom", generated_value(py, value.max_zoom)?)?;
    dict.set_item("pixel_ratio", generated_value(py, value.pixel_ratio)?)?;
    dict.set_item(
        "include_ideographs",
        generated_value(py, value.include_ideographs)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_opengl_borrowed_texture_descriptor(
    py: Python<'_>,
    value: &sys::mln_opengl_borrowed_texture_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "extent",
        generated_copy_mln_render_target_extent(py, &value.extent)?,
    )?;
    dict.set_item("physical_width", generated_value(py, value.physical_width)?)?;
    dict.set_item(
        "physical_height",
        generated_value(py, value.physical_height)?,
    )?;
    dict.set_item(
        "context",
        generated_copy_mln_opengl_context_descriptor(py, &value.context)?,
    )?;
    dict.set_item("texture", generated_value(py, value.texture)?)?;
    dict.set_item("target", generated_value(py, value.target)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_opengl_context_descriptor(
    py: Python<'_>,
    value: &sys::mln_opengl_context_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("ownership", generated_value(py, value.ownership)?)?;
    dict.set_item(
        "data",
        match value.platform {
            sys::MLN_OPENGL_CONTEXT_PLATFORM_WGL => generated_variant(
                py,
                "wgl",
                generated_copy_mln_wgl_context_descriptor(py, &(unsafe { value.data.wgl }))?,
            )?,
            sys::MLN_OPENGL_CONTEXT_PLATFORM_EGL => generated_variant(
                py,
                "egl",
                generated_copy_mln_egl_context_descriptor(py, &(unsafe { value.data.egl }))?,
            )?,
            sys::MLN_OPENGL_CONTEXT_PLATFORM_WEBGL => generated_variant(
                py,
                "webgl",
                generated_copy_mln_webgl_context_descriptor(py, &(unsafe { value.data.webgl }))?,
            )?,
            tag => generated_unknown_variant(py, tag)?,
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_opengl_owned_texture_descriptor(
    py: Python<'_>,
    value: &sys::mln_opengl_owned_texture_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "extent",
        generated_copy_mln_render_target_extent(py, &value.extent)?,
    )?;
    dict.set_item(
        "context",
        generated_copy_mln_opengl_context_descriptor(py, &value.context)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_opengl_owned_texture_frame(
    py: Python<'_>,
    value: &sys::mln_opengl_owned_texture_frame,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("generation", generated_value(py, value.generation)?)?;
    dict.set_item("width", generated_value(py, value.width)?)?;
    dict.set_item("height", generated_value(py, value.height)?)?;
    dict.set_item("scale_factor", generated_value(py, value.scale_factor)?)?;
    dict.set_item("frame_id", generated_value(py, value.frame_id)?)?;
    dict.set_item("texture", generated_value(py, value.texture)?)?;
    dict.set_item("target", generated_value(py, value.target)?)?;
    dict.set_item(
        "internal_format",
        generated_value(py, value.internal_format)?,
    )?;
    dict.set_item("format", generated_value(py, value.format)?)?;
    dict.set_item("type", generated_value(py, value.type_)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_opengl_surface_descriptor(
    py: Python<'_>,
    value: &sys::mln_opengl_surface_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "extent",
        generated_copy_mln_render_target_extent(py, &value.extent)?,
    )?;
    dict.set_item(
        "context",
        generated_copy_mln_opengl_context_descriptor(py, &value.context)?,
    )?;
    dict.set_item("surface", generated_value(py, value.surface as usize)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_premultiplied_rgba8_image(
    py: Python<'_>,
    value: &sys::mln_premultiplied_rgba8_image,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("width", generated_value(py, value.width)?)?;
    dict.set_item("height", generated_value(py, value.height)?)?;
    dict.set_item("stride", generated_value(py, value.stride)?)?;
    dict.set_item("pixels", unsafe {
        generated_bytes(py, generated_view(value.pixels, value.byte_length)?)
    }?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_projected_meters(
    py: Python<'_>,
    value: &sys::mln_projected_meters,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("northing", generated_value(py, value.northing)?)?;
    dict.set_item("easting", generated_value(py, value.easting)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_projection_mode(
    py: Python<'_>,
    value: &sys::mln_projection_mode,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "axonometric",
        generated_optional(
            py,
            value.fields & sys::MLN_PROJECTION_MODE_AXONOMETRIC != 0,
            || generated_value(py, value.axonometric),
        )?,
    )?;
    dict.set_item(
        "x_skew",
        generated_optional(
            py,
            value.fields & sys::MLN_PROJECTION_MODE_X_SKEW != 0,
            || generated_value(py, value.x_skew),
        )?,
    )?;
    dict.set_item(
        "y_skew",
        generated_optional(
            py,
            value.fields & sys::MLN_PROJECTION_MODE_Y_SKEW != 0,
            || generated_value(py, value.y_skew),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_quaternion(
    py: Python<'_>,
    value: &sys::mln_quaternion,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("x", generated_value(py, value.x)?)?;
    dict.set_item("y", generated_value(py, value.y)?)?;
    dict.set_item("z", generated_value(py, value.z)?)?;
    dict.set_item("w", generated_value(py, value.w)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_queried_feature(
    py: Python<'_>,
    value: &sys::mln_queried_feature,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("feature", unsafe { generated_bytes(py, value.feature) }?)?;
    dict.set_item(
        "source_id",
        generated_optional(
            py,
            value.fields & sys::MLN_QUERIED_FEATURE_SOURCE_ID != 0,
            || generated_text(py, value.source_id),
        )?,
    )?;
    dict.set_item(
        "source_layer_id",
        generated_optional(
            py,
            value.fields & sys::MLN_QUERIED_FEATURE_SOURCE_LAYER_ID != 0,
            || generated_text(py, value.source_layer_id),
        )?,
    )?;
    dict.set_item(
        "state",
        generated_optional(
            py,
            value.fields & sys::MLN_QUERIED_FEATURE_STATE != 0,
            || unsafe { generated_bytes(py, value.state) },
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_render_abandon_result(
    py: Python<'_>,
    value: &sys::mln_render_abandon_result,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("disposition", generated_value(py, value.disposition)?)?;
    dict.set_item(
        "quarantined_resource_count",
        generated_value(py, value.quarantined_resource_count)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_render_frame_result(
    py: Python<'_>,
    value: &sys::mln_render_frame_result,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("disposition", generated_value(py, value.disposition)?)?;
    dict.set_item("token", generated_value(py, value.token)?)?;
    dict.set_item(
        "map_update_generation",
        generated_value(py, value.map_update_generation)?,
    )?;
    dict.set_item(
        "extent_generation",
        generated_value(py, value.extent_generation)?,
    )?;
    dict.set_item(
        "frame_generation",
        generated_value(py, value.frame_generation)?,
    )?;
    dict.set_item("needs_repaint", generated_value(py, value.needs_repaint)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_render_session_attach_options(
    py: Python<'_>,
    value: &sys::mln_render_session_attach_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("driver", generated_value(py, value.driver)?)?;
    dict.set_item(
        "requested_texture_ring_depth",
        generated_value(py, value.requested_texture_ring_depth)?,
    )?;
    dict.set_item(
        "frame_wake",
        generated_copy_mln_wake(py, &value.frame_wake)?,
    )?;
    dict.set_item(
        "driver_work_wake",
        generated_copy_mln_wake(py, &value.driver_work_wake)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_render_session_capabilities(
    py: Python<'_>,
    value: &sys::mln_render_session_capabilities,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("driver", generated_value(py, value.driver)?)?;
    dict.set_item(
        "texture_ring_depth",
        generated_value(py, value.texture_ring_depth)?,
    )?;
    dict.set_item("flags", generated_value(py, value.flags)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_render_session_snapshot(
    py: Python<'_>,
    value: &sys::mln_render_session_snapshot,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("state", generated_value(py, value.state)?)?;
    dict.set_item("driver", generated_value(py, value.driver)?)?;
    dict.set_item("latest_result", generated_value(py, value.latest_result)?)?;
    dict.set_item(
        "extent",
        generated_copy_mln_render_target_extent(py, &value.extent)?,
    )?;
    dict.set_item("generation", generated_value(py, value.generation)?)?;
    dict.set_item(
        "map_update_generation",
        generated_value(py, value.map_update_generation)?,
    )?;
    dict.set_item(
        "rendered_update_generation",
        generated_value(py, value.rendered_update_generation)?,
    )?;
    dict.set_item(
        "extent_generation",
        generated_value(py, value.extent_generation)?,
    )?;
    dict.set_item(
        "frame_generation",
        generated_value(py, value.frame_generation)?,
    )?;
    dict.set_item(
        "latest_demand_token",
        generated_value(py, value.latest_demand_token)?,
    )?;
    dict.set_item(
        "pending_demand_count",
        generated_value(py, value.pending_demand_count)?,
    )?;
    dict.set_item(
        "acquired_frame_count",
        generated_value(py, value.acquired_frame_count)?,
    )?;
    dict.set_item("target_ready", generated_value(py, value.target_ready)?)?;
    dict.set_item(
        "pending_changes",
        generated_value(py, value.pending_changes)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_render_target_extent(
    py: Python<'_>,
    value: &sys::mln_render_target_extent,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("width", generated_value(py, value.width)?)?;
    dict.set_item("height", generated_value(py, value.height)?)?;
    dict.set_item("scale_factor", generated_value(py, value.scale_factor)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_rendered_feature_query_options(
    py: Python<'_>,
    value: &sys::mln_rendered_feature_query_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "layer_ids",
        generated_optional(
            py,
            value.fields & sys::MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS != 0,
            || {
                generated_list(
                    py,
                    unsafe { generated_slice(value.layer_ids, value.layer_id_count)? },
                    |element| generated_text(py, *element),
                )
            },
        )?,
    )?;
    dict.set_item(
        "filter",
        generated_optional(py, !value.filter.is_null(), || {
            Ok({
                let referenced = unsafe { *value.filter };
                unsafe { generated_bytes(py, referenced) }?
            })
        })?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_rendered_query_geometry(
    py: Python<'_>,
    value: &sys::mln_rendered_query_geometry,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "data",
        match value.type_ {
            sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT => generated_variant(
                py,
                "point",
                generated_copy_mln_screen_point(py, &(unsafe { value.data.point }))?,
            )?,
            sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX => generated_variant(
                py,
                "box",
                generated_copy_mln_screen_box(py, &(unsafe { value.data.box_ }))?,
            )?,
            sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING => generated_variant(
                py,
                "line_string",
                generated_copy_mln_screen_line_string(py, &(unsafe { value.data.line_string }))?,
            )?,
            tag => generated_unknown_variant(py, tag)?,
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_rendering_stats(
    py: Python<'_>,
    value: &sys::mln_rendering_stats,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("encoding_time", generated_value(py, value.encoding_time)?)?;
    dict.set_item("rendering_time", generated_value(py, value.rendering_time)?)?;
    dict.set_item("frame_count", generated_value(py, value.frame_count)?)?;
    dict.set_item(
        "draw_call_count",
        generated_value(py, value.draw_call_count)?,
    )?;
    dict.set_item(
        "total_draw_call_count",
        generated_value(py, value.total_draw_call_count)?,
    )?;
    Ok(dict.into_any().unbind())
}

unsafe extern "C" fn generated_callback_mln_resource_provider_callback(
    user_data: *mut std::ffi::c_void,
    request: *const sys::mln_resource_request,
    handle: sys::mln_resource_request_handle,
) -> sys::mln_resource_provider_decision {
    let _policy = GeneratedCallbackPolicy::enter(
        &[
            "mln_resource_request_complete",
            "mln_resource_request_cancelled",
            "mln_resource_request_set_cancel_callback",
            "mln_resource_request_release",
        ],
        maplibre_core::handle::NativeHandle::to_raw(handle),
    );
    let decision_state = match unsafe {
        maplibre_core::decision::DecisionHandleState::new(handle, RESOURCE_REQUEST_DECISION)
    } {
        Ok(state) => state,
        Err(_) => return sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH,
    };
    generated_invoke(
        || decision_state.finish_decision(false),
        |py| {
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 0) }) else {
                return Ok(decision_state.finish_decision(false));
            };
            let result = callback.bind(py).call1((
                {
                    if request.is_null() {
                        return Err(native_error("null record pointer"));
                    }
                    {
                        let referenced = unsafe { *request };
                        generated_copy_mln_resource_request(py, &referenced)?
                    }
                },
                Py::new(
                    py,
                    ResourceRequestHandle {
                        state: ManuallyDrop::new(Arc::clone(&decision_state)),
                        cancel_root: Mutex::new(std::sync::Weak::new()),
                    },
                )?,
            ))?;
            let decision = result.extract::<u32>()?;
            Ok(decision_state
                .finish_decision(decision == sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE))
        },
    )
}

fn generated_copy_mln_resource_request(
    py: Python<'_>,
    value: &sys::mln_resource_request,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("requested_url", unsafe {
        generated_c_string(py, value.requested_url, true)
    }?)?;
    dict.set_item("resolved_url", unsafe {
        generated_c_string(py, value.resolved_url, true)
    }?)?;
    dict.set_item("kind", generated_value(py, value.kind)?)?;
    dict.set_item("loading_method", generated_value(py, value.loading_method)?)?;
    dict.set_item("priority", generated_value(py, value.priority)?)?;
    dict.set_item("usage", generated_value(py, value.usage)?)?;
    dict.set_item("storage_policy", generated_value(py, value.storage_policy)?)?;
    dict.set_item(
        "range",
        generated_optional(py, value.has_range, || {
            Ok({
                let inner = PyDict::new(py);
                inner.set_item("start", generated_value(py, value.range_start)?)?;
                inner.set_item("end", generated_value(py, value.range_end)?)?;
                inner.into_any().unbind()
            })
        })?,
    )?;
    dict.set_item(
        "prior_modified_unix_ms",
        generated_optional(py, value.has_prior_modified, || {
            generated_value(py, value.prior_modified_unix_ms)
        })?,
    )?;
    dict.set_item(
        "prior_expires_unix_ms",
        generated_optional(py, value.has_prior_expires, || {
            generated_value(py, value.prior_expires_unix_ms)
        })?,
    )?;
    dict.set_item("prior_etag", unsafe {
        generated_c_string(py, value.prior_etag, true)
    }?)?;
    dict.set_item("prior_data", unsafe {
        generated_bytes(py, generated_view(value.prior_data, value.prior_data_size)?)
    }?)?;
    Ok(dict.into_any().unbind())
}

unsafe extern "C" fn generated_callback_mln_resource_transform_callback(
    user_data: *mut std::ffi::c_void,
    kind: sys::mln_resource_kind,
    url: *const std::ffi::c_char,
    out_response: *mut sys::mln_resource_transform_response,
) -> sys::mln_status {
    let _policy = GeneratedCallbackPolicy::enter(
        &["mln_resource_transform_response_set_url"],
        out_response as usize as u64,
    );

    generated_invoke(
        || sys::MLN_STATUS_NATIVE_ERROR,
        |py| {
            let callback_scope = GeneratedCallbackGuard::new();
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 0) }) else {
                return Ok(sys::MLN_STATUS_NATIVE_ERROR);
            };
            let result = callback.bind(py).call1((
                generated_value(py, kind)?,
                unsafe { generated_c_string(py, url, false) }?,
                Py::new(
                    py,
                    ResourceTransformResponseScope {
                        native: out_response as usize,
                        scope: callback_scope.scope(),
                    },
                )?,
            ))?;
            result.extract::<sys::mln_status>()
        },
    )
}

fn generated_copy_mln_runtime_event(
    py: Python<'_>,
    value: &sys::mln_runtime_event,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("type", generated_value(py, value.type_)?)?;
    dict.set_item("source_type", generated_value(py, value.source_type)?)?;
    dict.set_item("source", generated_value(py, value.source)?)?;
    dict.set_item("code", generated_value(py, value.code)?)?;
    dict.set_item(
        "payload",
        match value.payload_type {
            sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME => generated_variant(
                py,
                "render_frame",
                generated_copy_mln_runtime_event_render_frame(
                    py,
                    &(unsafe { value.payload.render_frame }),
                )?,
            )?,
            sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP => generated_variant(
                py,
                "render_map",
                generated_copy_mln_runtime_event_render_map(
                    py,
                    &(unsafe { value.payload.render_map }),
                )?,
            )?,
            sys::MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION => generated_variant(
                py,
                "tile_action",
                generated_copy_mln_runtime_event_tile_action(
                    py,
                    &(unsafe { value.payload.tile_action }),
                )?,
            )?,
            sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS => generated_variant(
                py,
                "offline_region_status",
                generated_copy_mln_runtime_event_offline_region_status(
                    py,
                    &(unsafe { value.payload.offline_region_status }),
                )?,
            )?,
            sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR => generated_variant(
                py,
                "offline_region_response_error",
                generated_copy_mln_runtime_event_offline_region_response_error(
                    py,
                    &(unsafe { value.payload.offline_region_response_error }),
                )?,
            )?,
            sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT => generated_variant(
                py,
                "offline_region_tile_count_limit",
                generated_copy_mln_runtime_event_offline_region_tile_count_limit(
                    py,
                    &(unsafe { value.payload.offline_region_tile_count_limit }),
                )?,
            )?,
            sys::MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED => generated_variant(
                py,
                "camera_transition_finished",
                generated_copy_mln_runtime_event_camera_transition_finished(
                    py,
                    &(unsafe { value.payload.camera_transition_finished }),
                )?,
            )?,
            tag => generated_unknown_variant(py, tag)?,
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_runtime_event_batch_view(
    py: Python<'_>,
    value: &sys::mln_runtime_event_batch_view,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "events",
        generated_list(
            py,
            unsafe { generated_strided_values(value.events, value.event_count, value.event_size)? },
            |element| {
                Ok({
                    let item = generated_copy_mln_runtime_event(py, &element)?;
                    item.bind(py)
                        .cast::<PyDict>()?
                        .set_item("message", unsafe {
                            generated_arena_string(
                                value.messages,
                                value.messages_size,
                                element.message_offset,
                                element.message_size,
                            )?
                        })?;
                    item
                })
            },
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_runtime_event_camera_transition_finished(
    py: Python<'_>,
    value: &sys::mln_runtime_event_camera_transition_finished,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("transition_id", generated_value(py, value.transition_id)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_runtime_event_offline_region_response_error(
    py: Python<'_>,
    value: &sys::mln_runtime_event_offline_region_response_error,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("region_id", generated_value(py, value.region_id)?)?;
    dict.set_item("reason", generated_value(py, value.reason)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_runtime_event_offline_region_status(
    py: Python<'_>,
    value: &sys::mln_runtime_event_offline_region_status,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("region_id", generated_value(py, value.region_id)?)?;
    dict.set_item(
        "status",
        generated_copy_mln_offline_region_status(py, &value.status)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_runtime_event_offline_region_tile_count_limit(
    py: Python<'_>,
    value: &sys::mln_runtime_event_offline_region_tile_count_limit,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("region_id", generated_value(py, value.region_id)?)?;
    dict.set_item("limit", generated_value(py, value.limit)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_runtime_event_render_frame(
    py: Python<'_>,
    value: &sys::mln_runtime_event_render_frame,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("mode", generated_value(py, value.mode)?)?;
    dict.set_item("needs_repaint", generated_value(py, value.needs_repaint)?)?;
    dict.set_item(
        "placement_changed",
        generated_value(py, value.placement_changed)?,
    )?;
    dict.set_item(
        "stats",
        generated_copy_mln_rendering_stats(py, &value.stats)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_runtime_event_render_map(
    py: Python<'_>,
    value: &sys::mln_runtime_event_render_map,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("mode", generated_value(py, value.mode)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_runtime_event_tile_action(
    py: Python<'_>,
    value: &sys::mln_runtime_event_tile_action,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("operation", generated_value(py, value.operation)?)?;
    dict.set_item("tile_id", generated_copy_mln_tile_id(py, &value.tile_id)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_runtime_options(
    py: Python<'_>,
    value: &sys::mln_runtime_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("flags", generated_value(py, value.flags)?)?;
    dict.set_item("asset_path", unsafe {
        generated_c_string(py, value.asset_path, true)
    }?)?;
    dict.set_item("cache_path", unsafe {
        generated_c_string(py, value.cache_path, true)
    }?)?;
    dict.set_item("event_mask", generated_value(py, value.event_mask)?)?;
    dict.set_item(
        "event_wake",
        generated_copy_mln_wake(py, &value.event_wake)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_screen_box(
    py: Python<'_>,
    value: &sys::mln_screen_box,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("min", generated_copy_mln_screen_point(py, &value.min)?)?;
    dict.set_item("max", generated_copy_mln_screen_point(py, &value.max)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_screen_line_string(
    py: Python<'_>,
    value: &sys::mln_screen_line_string,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "points",
        generated_list(
            py,
            unsafe { generated_slice(value.points, value.point_count)? },
            |element| generated_copy_mln_screen_point(py, element),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_screen_point(
    py: Python<'_>,
    value: &sys::mln_screen_point,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("x", generated_value(py, value.x)?)?;
    dict.set_item("y", generated_value(py, value.y)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_source_feature_query_options(
    py: Python<'_>,
    value: &sys::mln_source_feature_query_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "source_layer_ids",
        generated_optional(
            py,
            value.fields & sys::MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS != 0,
            || {
                generated_list(
                    py,
                    unsafe {
                        generated_slice(value.source_layer_ids, value.source_layer_id_count)?
                    },
                    |element| generated_text(py, *element),
                )
            },
        )?,
    )?;
    dict.set_item(
        "filter",
        generated_optional(py, !value.filter.is_null(), || {
            Ok({
                let referenced = unsafe { *value.filter };
                unsafe { generated_bytes(py, referenced) }?
            })
        })?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_image_info(
    py: Python<'_>,
    value: &sys::mln_style_image_info,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("width", generated_value(py, value.width)?)?;
    dict.set_item("height", generated_value(py, value.height)?)?;
    dict.set_item("stride", generated_value(py, value.stride)?)?;
    dict.set_item("byte_length", generated_value(py, value.byte_length)?)?;
    dict.set_item(
        "stretch_x_count",
        generated_value(py, value.stretch_x_count)?,
    )?;
    dict.set_item(
        "stretch_y_count",
        generated_value(py, value.stretch_y_count)?,
    )?;
    dict.set_item(
        "content",
        generated_optional(py, value.has_content, || {
            generated_copy_mln_image_content(py, &value.content)
        })?,
    )?;
    dict.set_item(
        "text_fit_width",
        generated_optional(py, value.has_text_fit_width, || {
            generated_value(py, value.text_fit_width)
        })?,
    )?;
    dict.set_item(
        "text_fit_height",
        generated_optional(py, value.has_text_fit_height, || {
            generated_value(py, value.text_fit_height)
        })?,
    )?;
    dict.set_item("pixel_ratio", generated_value(py, value.pixel_ratio)?)?;
    dict.set_item("sdf", generated_value(py, value.sdf)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_image_options(
    py: Python<'_>,
    value: &sys::mln_style_image_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "stretch_x",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_IMAGE_OPTION_STRETCH_X != 0,
            || {
                generated_list(
                    py,
                    unsafe { generated_slice(value.stretch_x, value.stretch_x_count)? },
                    |element| generated_copy_mln_image_stretch(py, element),
                )
            },
        )?,
    )?;
    dict.set_item(
        "stretch_y",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_IMAGE_OPTION_STRETCH_Y != 0,
            || {
                generated_list(
                    py,
                    unsafe { generated_slice(value.stretch_y, value.stretch_y_count)? },
                    |element| generated_copy_mln_image_stretch(py, element),
                )
            },
        )?,
    )?;
    dict.set_item(
        "content",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_IMAGE_OPTION_CONTENT != 0,
            || generated_copy_mln_image_content(py, &value.content),
        )?,
    )?;
    dict.set_item(
        "text_fit_width",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH != 0,
            || generated_value(py, value.text_fit_width),
        )?,
    )?;
    dict.set_item(
        "text_fit_height",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT != 0,
            || generated_value(py, value.text_fit_height),
        )?,
    )?;
    dict.set_item(
        "pixel_ratio",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO != 0,
            || generated_value(py, value.pixel_ratio),
        )?,
    )?;
    dict.set_item(
        "sdf",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_IMAGE_OPTION_SDF != 0,
            || generated_value(py, value.sdf),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_image_result(
    py: Python<'_>,
    value: &sys::mln_style_image_result,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "info",
        generated_copy_mln_style_image_info(py, &value.info)?,
    )?;
    dict.set_item("pixels", unsafe { generated_bytes(py, value.pixels) }?)?;
    dict.set_item(
        "stretch_x",
        generated_list(
            py,
            unsafe { generated_slice(value.stretch_x, value.stretch_x_count)? },
            |element| generated_copy_mln_image_stretch(py, element),
        )?,
    )?;
    dict.set_item(
        "stretch_y",
        generated_list(
            py,
            unsafe { generated_slice(value.stretch_y, value.stretch_y_count)? },
            |element| generated_copy_mln_image_stretch(py, element),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_image_stretches_result(
    py: Python<'_>,
    value: &sys::mln_style_image_stretches_result,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "stretch_x",
        generated_list(
            py,
            unsafe { generated_slice(value.stretch_x, value.stretch_x_count)? },
            |element| generated_copy_mln_image_stretch(py, element),
        )?,
    )?;
    dict.set_item(
        "stretch_y",
        generated_list(
            py,
            unsafe { generated_slice(value.stretch_y, value.stretch_y_count)? },
            |element| generated_copy_mln_image_stretch(py, element),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_layer_entry(
    py: Python<'_>,
    value: &sys::mln_style_layer_entry,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("id", generated_text(py, value.id)?)?;
    dict.set_item("type", generated_text(py, value.type_)?)?;
    dict.set_item(
        "source_id",
        generated_optional(py, value.source_id.size != 0, || {
            generated_text(py, value.source_id)
        })?,
    )?;
    dict.set_item(
        "source_layer",
        generated_optional(py, value.source_layer.size != 0, || {
            generated_text(py, value.source_layer)
        })?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_layer_info(
    py: Python<'_>,
    value: &sys::mln_style_layer_info,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("type", generated_text(py, value.type_)?)?;
    dict.set_item("min_zoom", generated_value(py, value.min_zoom)?)?;
    dict.set_item("max_zoom", generated_value(py, value.max_zoom)?)?;
    dict.set_item("visibility", generated_value(py, value.visibility)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_layer_result(
    py: Python<'_>,
    value: &sys::mln_style_layer_result,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "info",
        generated_copy_mln_style_layer_info(py, &value.info)?,
    )?;
    dict.set_item(
        "source_id",
        generated_optional(py, value.source_id.size != 0, || {
            generated_text(py, value.source_id)
        })?,
    )?;
    dict.set_item(
        "source_layer",
        generated_optional(py, value.source_layer.size != 0, || {
            generated_text(py, value.source_layer)
        })?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_source_info(
    py: Python<'_>,
    value: &sys::mln_style_source_info,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("type", generated_value(py, value.type_)?)?;
    dict.set_item("id_size", generated_value(py, value.id_size)?)?;
    dict.set_item("is_volatile", generated_value(py, value.is_volatile)?)?;
    dict.set_item(
        "attribution_size",
        generated_optional(py, value.has_attribution, || {
            generated_value(py, value.attribution_size)
        })?,
    )?;
    dict.set_item(
        "url_size",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_SOURCE_INFO_URL != 0,
            || generated_value(py, value.url_size),
        )?,
    )?;
    dict.set_item(
        "tilejson",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_SOURCE_INFO_TILEJSON != 0,
            || {
                Ok({
                    let inner = PyDict::new(py);
                    inner.set_item("tile_count", generated_value(py, value.tile_count)?)?;
                    inner.set_item("min_zoom", generated_value(py, value.min_zoom)?)?;
                    inner.set_item("max_zoom", generated_value(py, value.max_zoom)?)?;
                    inner.set_item("scheme", generated_value(py, value.scheme)?)?;
                    inner.into_any().unbind()
                })
            },
        )?,
    )?;
    dict.set_item(
        "bounds",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_SOURCE_INFO_BOUNDS != 0,
            || generated_copy_mln_lat_lng_bounds(py, &value.bounds),
        )?,
    )?;
    dict.set_item(
        "tile_size",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_SOURCE_INFO_TILE_SIZE != 0,
            || generated_value(py, value.tile_size),
        )?,
    )?;
    dict.set_item(
        "vector_encoding",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING != 0,
            || generated_value(py, value.vector_encoding),
        )?,
    )?;
    dict.set_item(
        "raster_encoding",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_SOURCE_INFO_RASTER_ENCODING != 0,
            || generated_value(py, value.raster_encoding),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_source_result(
    py: Python<'_>,
    value: &sys::mln_style_source_result,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "info",
        generated_copy_mln_style_source_info(py, &value.info)?,
    )?;
    dict.set_item(
        "attribution",
        generated_optional(py, value.info.has_attribution, || {
            generated_text(py, value.attribution)
        })?,
    )?;
    dict.set_item(
        "url",
        generated_optional(
            py,
            value.info.fields & sys::MLN_STYLE_SOURCE_INFO_URL != 0,
            || generated_text(py, value.url),
        )?,
    )?;
    dict.set_item(
        "tile_urls",
        generated_optional(
            py,
            value.info.fields & sys::MLN_STYLE_SOURCE_INFO_TILEJSON != 0,
            || {
                generated_list(
                    py,
                    unsafe { generated_slice(value.tile_urls, value.tile_url_count)? },
                    |element| generated_text(py, *element),
                )
            },
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_source_tile_urls_result(
    py: Python<'_>,
    value: &sys::mln_style_source_tile_urls_result,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "tile_urls",
        generated_list(
            py,
            unsafe { generated_slice(value.tile_urls, value.tile_url_count)? },
            |element| generated_text(py, *element),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_tile_source_options(
    py: Python<'_>,
    value: &sys::mln_style_tile_source_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "min_zoom",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM != 0,
            || generated_value(py, value.min_zoom),
        )?,
    )?;
    dict.set_item(
        "max_zoom",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM != 0,
            || generated_value(py, value.max_zoom),
        )?,
    )?;
    dict.set_item(
        "attribution",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION != 0,
            || generated_text(py, value.attribution),
        )?,
    )?;
    dict.set_item(
        "scheme",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_SCHEME != 0,
            || generated_value(py, value.scheme),
        )?,
    )?;
    dict.set_item(
        "bounds",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS != 0,
            || generated_copy_mln_lat_lng_bounds(py, &value.bounds),
        )?,
    )?;
    dict.set_item(
        "tile_size",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE != 0,
            || generated_value(py, value.tile_size),
        )?,
    )?;
    dict.set_item(
        "vector_encoding",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING != 0,
            || generated_value(py, value.vector_encoding),
        )?,
    )?;
    dict.set_item(
        "raster_encoding",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING != 0,
            || generated_value(py, value.raster_encoding),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_transition_options(
    py: Python<'_>,
    value: &sys::mln_style_transition_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "duration_ms",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_TRANSITION_OPTION_DURATION != 0,
            || generated_value(py, value.duration_ms),
        )?,
    )?;
    dict.set_item(
        "delay_ms",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_TRANSITION_OPTION_DELAY != 0,
            || generated_value(py, value.delay_ms),
        )?,
    )?;
    dict.set_item(
        "enable_placement_transitions",
        generated_optional(
            py,
            value.fields & sys::MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS != 0,
            || generated_value(py, value.enable_placement_transitions),
        )?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_texture_image_info(
    py: Python<'_>,
    value: &sys::mln_texture_image_info,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("width", generated_value(py, value.width)?)?;
    dict.set_item("height", generated_value(py, value.height)?)?;
    dict.set_item("stride", generated_value(py, value.stride)?)?;
    dict.set_item("byte_length", generated_value(py, value.byte_length)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_texture_readback_result(
    py: Python<'_>,
    value: &sys::mln_texture_readback_result,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("data", unsafe { generated_bytes(py, value.data) }?)?;
    dict.set_item(
        "info",
        generated_copy_mln_texture_image_info(py, &value.info)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_tile_id(py: Python<'_>, value: &sys::mln_tile_id) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("overscaled_z", generated_value(py, value.overscaled_z)?)?;
    dict.set_item("wrap", generated_value(py, value.wrap)?)?;
    dict.set_item("canonical_z", generated_value(py, value.canonical_z)?)?;
    dict.set_item("canonical_x", generated_value(py, value.canonical_x)?)?;
    dict.set_item("canonical_y", generated_value(py, value.canonical_y)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_unit_bezier(
    py: Python<'_>,
    value: &sys::mln_unit_bezier,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("x1", generated_value(py, value.x1)?)?;
    dict.set_item("y1", generated_value(py, value.y1)?)?;
    dict.set_item("x2", generated_value(py, value.x2)?)?;
    dict.set_item("y2", generated_value(py, value.y2)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_vec3(py: Python<'_>, value: &sys::mln_vec3) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("x", generated_value(py, value.x)?)?;
    dict.set_item("y", generated_value(py, value.y)?)?;
    dict.set_item("z", generated_value(py, value.z)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_vulkan_borrowed_texture_descriptor(
    py: Python<'_>,
    value: &sys::mln_vulkan_borrowed_texture_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "extent",
        generated_copy_mln_render_target_extent(py, &value.extent)?,
    )?;
    dict.set_item("physical_width", generated_value(py, value.physical_width)?)?;
    dict.set_item(
        "physical_height",
        generated_value(py, value.physical_height)?,
    )?;
    dict.set_item(
        "context",
        generated_copy_mln_vulkan_context_descriptor(py, &value.context)?,
    )?;
    dict.set_item("image", generated_value(py, value.image)?)?;
    dict.set_item("image_view", generated_value(py, value.image_view)?)?;
    dict.set_item("format", generated_value(py, value.format)?)?;
    dict.set_item("initial_layout", generated_value(py, value.initial_layout)?)?;
    dict.set_item("final_layout", generated_value(py, value.final_layout)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_vulkan_context_descriptor(
    py: Python<'_>,
    value: &sys::mln_vulkan_context_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("instance", generated_value(py, value.instance as usize)?)?;
    dict.set_item(
        "physical_device",
        generated_value(py, value.physical_device as usize)?,
    )?;
    dict.set_item("device", generated_value(py, value.device as usize)?)?;
    dict.set_item(
        "graphics_queue",
        generated_value(py, value.graphics_queue as usize)?,
    )?;
    dict.set_item(
        "graphics_queue_family_index",
        generated_value(py, value.graphics_queue_family_index)?,
    )?;
    dict.set_item(
        "get_instance_proc_addr",
        generated_value(py, value.get_instance_proc_addr as usize)?,
    )?;
    dict.set_item(
        "get_device_proc_addr",
        generated_value(py, value.get_device_proc_addr as usize)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_vulkan_owned_texture_descriptor(
    py: Python<'_>,
    value: &sys::mln_vulkan_owned_texture_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "extent",
        generated_copy_mln_render_target_extent(py, &value.extent)?,
    )?;
    dict.set_item(
        "context",
        generated_copy_mln_vulkan_context_descriptor(py, &value.context)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_vulkan_owned_texture_frame(
    py: Python<'_>,
    value: &sys::mln_vulkan_owned_texture_frame,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("generation", generated_value(py, value.generation)?)?;
    dict.set_item("width", generated_value(py, value.width)?)?;
    dict.set_item("height", generated_value(py, value.height)?)?;
    dict.set_item("scale_factor", generated_value(py, value.scale_factor)?)?;
    dict.set_item("frame_id", generated_value(py, value.frame_id)?)?;
    dict.set_item("image", generated_value(py, value.image)?)?;
    dict.set_item("image_view", generated_value(py, value.image_view)?)?;
    dict.set_item("device", generated_value(py, value.device as usize)?)?;
    dict.set_item("format", generated_value(py, value.format)?)?;
    dict.set_item("layout", generated_value(py, value.layout)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_vulkan_surface_descriptor(
    py: Python<'_>,
    value: &sys::mln_vulkan_surface_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "extent",
        generated_copy_mln_render_target_extent(py, &value.extent)?,
    )?;
    dict.set_item(
        "context",
        generated_copy_mln_vulkan_context_descriptor(py, &value.context)?,
    )?;
    dict.set_item("surface", generated_value(py, value.surface)?)?;
    Ok(dict.into_any().unbind())
}

unsafe extern "C" fn generated_callback_mln_wake_callback(user_data: *mut std::ffi::c_void) {
    generated_invoke(
        || (),
        |py| {
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 0) }) else {
                return Ok(());
            };
            let _result = callback.bind(py).call1(())?;
            Ok(())
        },
    )
}

fn generated_copy_mln_wake(py: Python<'_>, value: &sys::mln_wake) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("callback", py.None())?;
    if value.callback.is_some() {
        return Err(native_error(
            "native callback cannot be copied into a Python closure",
        ));
    }
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_webgl_context_descriptor(
    py: Python<'_>,
    value: &sys::mln_webgl_context_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("kind", generated_value(py, value.kind)?)?;
    dict.set_item("context", generated_value(py, value.context)?)?;
    dict.set_item(
        "canvas_selector",
        generated_text(py, value.canvas_selector)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_webgpu_borrowed_texture_descriptor(
    py: Python<'_>,
    value: &sys::mln_webgpu_borrowed_texture_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "extent",
        generated_copy_mln_render_target_extent(py, &value.extent)?,
    )?;
    dict.set_item("physical_width", generated_value(py, value.physical_width)?)?;
    dict.set_item(
        "physical_height",
        generated_value(py, value.physical_height)?,
    )?;
    dict.set_item(
        "context",
        generated_copy_mln_webgpu_context_descriptor(py, &value.context)?,
    )?;
    dict.set_item("texture", generated_value(py, value.texture as usize)?)?;
    dict.set_item(
        "texture_view",
        generated_value(py, value.texture_view as usize)?,
    )?;
    dict.set_item("format", generated_value(py, value.format)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_webgpu_context_descriptor(
    py: Python<'_>,
    value: &sys::mln_webgpu_context_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("instance", generated_value(py, value.instance as usize)?)?;
    dict.set_item("device", generated_value(py, value.device as usize)?)?;
    dict.set_item("queue", generated_value(py, value.queue as usize)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_webgpu_owned_texture_descriptor(
    py: Python<'_>,
    value: &sys::mln_webgpu_owned_texture_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "extent",
        generated_copy_mln_render_target_extent(py, &value.extent)?,
    )?;
    dict.set_item(
        "context",
        generated_copy_mln_webgpu_context_descriptor(py, &value.context)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_webgpu_owned_texture_frame(
    py: Python<'_>,
    value: &sys::mln_webgpu_owned_texture_frame,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("generation", generated_value(py, value.generation)?)?;
    dict.set_item("width", generated_value(py, value.width)?)?;
    dict.set_item("height", generated_value(py, value.height)?)?;
    dict.set_item("scale_factor", generated_value(py, value.scale_factor)?)?;
    dict.set_item("frame_id", generated_value(py, value.frame_id)?)?;
    dict.set_item("texture", generated_value(py, value.texture as usize)?)?;
    dict.set_item(
        "texture_view",
        generated_value(py, value.texture_view as usize)?,
    )?;
    dict.set_item("device", generated_value(py, value.device as usize)?)?;
    dict.set_item("format", generated_value(py, value.format)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_webgpu_surface_descriptor(
    py: Python<'_>,
    value: &sys::mln_webgpu_surface_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "extent",
        generated_copy_mln_render_target_extent(py, &value.extent)?,
    )?;
    dict.set_item(
        "context",
        generated_copy_mln_webgpu_context_descriptor(py, &value.context)?,
    )?;
    dict.set_item("surface", generated_value(py, value.surface as usize)?)?;
    dict.set_item("format", generated_value(py, value.format)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_wgl_context_descriptor(
    py: Python<'_>,
    value: &sys::mln_wgl_context_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "device_context",
        generated_value(py, value.device_context as usize)?,
    )?;
    dict.set_item(
        "share_context",
        generated_value(py, value.share_context as usize)?,
    )?;
    dict.set_item(
        "get_proc_address",
        generated_value(py, value.get_proc_address as usize)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_input_mln_animation_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_animation_options> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_animation_options_default() });
    }
    let mut raw: sys::mln_animation_options = unsafe { sys::mln_animation_options_default() };
    raw.size = std::mem::size_of::<sys::mln_animation_options>() as _;
    raw.fields = 0;
    if let Some(field) = generated_present(value, "duration_ms")? {
        raw.duration_ms = field.extract::<f64>()?;
        raw.fields |= sys::MLN_ANIMATION_OPTION_DURATION;
    }
    if let Some(field) = generated_present(value, "velocity")? {
        raw.velocity = field.extract::<f64>()?;
        raw.fields |= sys::MLN_ANIMATION_OPTION_VELOCITY;
    }
    if let Some(field) = generated_present(value, "min_zoom")? {
        raw.min_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_ANIMATION_OPTION_MIN_ZOOM;
    }
    if let Some(field) = generated_present(value, "easing")? {
        raw.easing = generated_input_mln_unit_bezier(&field, storage)?;
        raw.fields |= sys::MLN_ANIMATION_OPTION_EASING;
    }
    if let Some(field) = generated_present(value, "transition_id")? {
        raw.transition_id = field.extract::<u64>()?;
        raw.fields |= sys::MLN_ANIMATION_OPTION_TRANSITION_ID;
    }
    Ok(raw)
}

fn generated_input_mln_bound_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_bound_options> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_bound_options_default() });
    }
    let mut raw: sys::mln_bound_options = unsafe { sys::mln_bound_options_default() };
    raw.size = std::mem::size_of::<sys::mln_bound_options>() as _;
    raw.fields = 0;
    if let Some(field) = generated_present(value, "bounds")? {
        raw.bounds = generated_input_mln_lat_lng_bounds(&field, storage)?;
        raw.fields |= sys::MLN_BOUND_OPTION_BOUNDS;
    }
    if let Some(field) = generated_present(value, "min_zoom")? {
        raw.min_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_BOUND_OPTION_MIN_ZOOM;
    }
    if let Some(field) = generated_present(value, "max_zoom")? {
        raw.max_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_BOUND_OPTION_MAX_ZOOM;
    }
    if let Some(field) = generated_present(value, "min_pitch")? {
        raw.min_pitch = field.extract::<f64>()?;
        raw.fields |= sys::MLN_BOUND_OPTION_MIN_PITCH;
    }
    if let Some(field) = generated_present(value, "max_pitch")? {
        raw.max_pitch = field.extract::<f64>()?;
        raw.fields |= sys::MLN_BOUND_OPTION_MAX_PITCH;
    }
    let field = value.getattr("unbounded")?;
    if field.extract::<bool>()? {
        raw.fields |= sys::MLN_BOUND_OPTION_UNBOUNDED;
    }
    Ok(raw)
}

fn generated_input_mln_camera_delta<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_camera_delta> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_camera_delta_default() });
    }
    let mut raw: sys::mln_camera_delta = unsafe { sys::mln_camera_delta_default() };
    raw.size = std::mem::size_of::<sys::mln_camera_delta>() as _;
    raw.has_anchor = false;
    raw.kind = value
        .getattr("kind")?
        .extract::<sys::mln_camera_delta_kind>()?;
    raw.offset = generated_input_mln_screen_point(&value.getattr("offset")?, storage)?;
    raw.amount = value.getattr("amount")?.extract::<f64>()?;
    if let Some(field) = generated_present(value, "anchor")? {
        raw.anchor = generated_input_mln_screen_point(&field, storage)?;
        raw.has_anchor = true;
    }
    raw.animation = generated_input_mln_animation_options(&value.getattr("animation")?, storage)?;
    Ok(raw)
}

fn generated_input_mln_camera_fit_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_camera_fit_options> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_camera_fit_options_default() });
    }
    let mut raw: sys::mln_camera_fit_options = unsafe { sys::mln_camera_fit_options_default() };
    raw.size = std::mem::size_of::<sys::mln_camera_fit_options>() as _;
    raw.fields = 0;
    if let Some(field) = generated_present(value, "padding")? {
        raw.padding = generated_input_mln_edge_insets(&field, storage)?;
        raw.fields |= sys::MLN_CAMERA_FIT_OPTION_PADDING;
    }
    if let Some(field) = generated_present(value, "bearing")? {
        raw.bearing = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_FIT_OPTION_BEARING;
    }
    if let Some(field) = generated_present(value, "pitch")? {
        raw.pitch = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_FIT_OPTION_PITCH;
    }
    Ok(raw)
}

fn generated_input_mln_camera_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_camera_options> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_camera_options_default() });
    }
    let mut raw: sys::mln_camera_options = unsafe { sys::mln_camera_options_default() };
    raw.size = std::mem::size_of::<sys::mln_camera_options>() as _;
    raw.fields = 0;
    if let Some(field) = generated_present(value, "center")? {
        raw.latitude = field.getattr("latitude")?.extract::<f64>()?;
        raw.longitude = field.getattr("longitude")?.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_OPTION_CENTER;
    }
    if let Some(field) = generated_present(value, "center_altitude")? {
        raw.center_altitude = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_OPTION_CENTER_ALTITUDE;
    }
    if let Some(field) = generated_present(value, "padding")? {
        raw.padding = generated_input_mln_edge_insets(&field, storage)?;
        raw.fields |= sys::MLN_CAMERA_OPTION_PADDING;
    }
    if let Some(field) = generated_present(value, "anchor")? {
        raw.anchor = generated_input_mln_screen_point(&field, storage)?;
        raw.fields |= sys::MLN_CAMERA_OPTION_ANCHOR;
    }
    if let Some(field) = generated_present(value, "zoom")? {
        raw.zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_OPTION_ZOOM;
    }
    if let Some(field) = generated_present(value, "bearing")? {
        raw.bearing = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_OPTION_BEARING;
    }
    if let Some(field) = generated_present(value, "pitch")? {
        raw.pitch = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_OPTION_PITCH;
    }
    if let Some(field) = generated_present(value, "roll")? {
        raw.roll = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_OPTION_ROLL;
    }
    if let Some(field) = generated_present(value, "field_of_view")? {
        raw.field_of_view = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_OPTION_FOV;
    }
    Ok(raw)
}

fn generated_input_mln_camera_update<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_camera_update> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_camera_update_default() });
    }
    let mut raw: sys::mln_camera_update = unsafe { sys::mln_camera_update_default() };
    raw.size = std::mem::size_of::<sys::mln_camera_update>() as _;
    raw.reserved = 0;
    raw.mode = value
        .getattr("mode")?
        .extract::<sys::mln_camera_update_mode>()?;
    raw.camera = generated_input_mln_camera_options(&value.getattr("camera")?, storage)?;
    raw.animation = generated_input_mln_animation_options(&value.getattr("animation")?, storage)?;
    raw.gesture_phase = value
        .getattr("gesture_phase")?
        .extract::<sys::mln_gesture_phase>()?;
    Ok(raw)
}

fn generated_input_mln_canonical_tile_id<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_canonical_tile_id> {
    let _ = storage;
    let mut raw: sys::mln_canonical_tile_id = unsafe { std::mem::zeroed() };
    raw.z = value.getattr("z")?.extract::<u32>()?;
    raw.x = value.getattr("x")?.extract::<u32>()?;
    raw.y = value.getattr("y")?.extract::<u32>()?;
    Ok(raw)
}

fn generated_input_mln_custom_geometry_source_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_custom_geometry_source_options> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_custom_geometry_source_options_default() });
    }
    let mut raw: sys::mln_custom_geometry_source_options =
        unsafe { sys::mln_custom_geometry_source_options_default() };
    raw.size = std::mem::size_of::<sys::mln_custom_geometry_source_options>() as _;
    raw.fields = 0;
    if let Some(field) = generated_present(value, "min_zoom")? {
        raw.min_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
    }
    if let Some(field) = generated_present(value, "max_zoom")? {
        raw.max_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
    }
    if let Some(field) = generated_present(value, "tolerance")? {
        raw.tolerance = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE;
    }
    if let Some(field) = generated_present(value, "tile_size")? {
        raw.tile_size = field.extract::<u32>()?;
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE;
    }
    if let Some(field) = generated_present(value, "buffer")? {
        raw.buffer = field.extract::<u32>()?;
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER;
    }
    if let Some(field) = generated_present(value, "clip")? {
        raw.clip = field.extract::<bool>()?;
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP;
    }
    if let Some(field) = generated_present(value, "wrap")? {
        raw.wrap = field.extract::<bool>()?;
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
    }
    let fetch_tile_enabled = !value.getattr("fetch_tile")?.is_none();
    let cancel_tile_enabled = !value.getattr("cancel_tile")?.is_none();
    if fetch_tile_enabled || cancel_tile_enabled {
        raw.user_data = storage.register_callbacks(vec![
            value.getattr("_invoke_fetch_tile")?.unbind(),
            value.getattr("_invoke_cancel_tile")?.unbind(),
        ]);
        raw.release_user_data = Some(generated_release_callbacks_no_reentry);
        raw.fetch_tile = if fetch_tile_enabled {
            Some(generated_callback_mln_custom_geometry_source_options_fetch_tile)
        } else {
            None
        };
        raw.cancel_tile = if cancel_tile_enabled {
            Some(generated_callback_mln_custom_geometry_source_options_cancel_tile)
        } else {
            None
        };
    }
    Ok(raw)
}

fn generated_input_mln_custom_mvt_vector_source_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_custom_mvt_vector_source_options> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_custom_mvt_vector_source_options_default() });
    }
    let mut raw: sys::mln_custom_mvt_vector_source_options =
        unsafe { sys::mln_custom_mvt_vector_source_options_default() };
    raw.size = std::mem::size_of::<sys::mln_custom_mvt_vector_source_options>() as _;
    raw.fields = 0;
    if let Some(field) = generated_present(value, "min_zoom")? {
        raw.min_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM;
    }
    if let Some(field) = generated_present(value, "max_zoom")? {
        raw.max_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
    }
    let fetch_tile_enabled = !value.getattr("fetch_tile")?.is_none();
    let cancel_tile_enabled = !value.getattr("cancel_tile")?.is_none();
    if fetch_tile_enabled || cancel_tile_enabled {
        raw.user_data = storage.register_callbacks(vec![
            value.getattr("_invoke_fetch_tile")?.unbind(),
            value.getattr("_invoke_cancel_tile")?.unbind(),
        ]);
        raw.release_user_data = Some(generated_release_callbacks_no_reentry);
        raw.fetch_tile = if fetch_tile_enabled {
            Some(generated_callback_mln_custom_mvt_vector_source_options_fetch_tile)
        } else {
            None
        };
        raw.cancel_tile = if cancel_tile_enabled {
            Some(generated_callback_mln_custom_mvt_vector_source_options_cancel_tile)
        } else {
            None
        };
    }
    Ok(raw)
}

fn generated_input_mln_edge_insets<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_edge_insets> {
    let _ = storage;
    let mut raw: sys::mln_edge_insets = unsafe { std::mem::zeroed() };
    raw.top = value.getattr("top")?.extract::<f64>()?;
    raw.left = value.getattr("left")?.extract::<f64>()?;
    raw.bottom = value.getattr("bottom")?.extract::<f64>()?;
    raw.right = value.getattr("right")?.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_egl_context_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_egl_context_descriptor> {
    let _ = storage;
    let mut raw: sys::mln_egl_context_descriptor = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_egl_context_descriptor>() as _;
    raw.display = value.getattr("display")?.extract::<usize>()? as _;
    raw.config = value.getattr("config")?.extract::<usize>()? as _;
    raw.share_context = value.getattr("share_context")?.extract::<usize>()? as _;
    raw.client_api = value
        .getattr("client_api")?
        .extract::<sys::mln_opengl_client_api>()?;
    raw.get_proc_address = value.getattr("get_proc_address")?.extract::<usize>()? as _;
    Ok(raw)
}

fn generated_input_mln_feature_state_selector<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_feature_state_selector> {
    let mut raw: sys::mln_feature_state_selector = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_feature_state_selector>() as _;
    raw.fields = 0;
    raw.source_id = storage.buffer(value.getattr("source_id")?, true)?;
    if let Some(field) = generated_present(value, "source_layer_id")? {
        raw.source_layer_id = storage.buffer(field, true)?;
        raw.fields |= sys::MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID;
    }
    if let Some(field) = generated_present(value, "feature_id")? {
        raw.feature_id = storage.buffer(field, true)?;
        raw.fields |= sys::MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
    }
    if let Some(field) = generated_present(value, "state_key")? {
        raw.state_key = storage.buffer(field, true)?;
        raw.fields |= sys::MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
    }
    Ok(raw)
}

fn generated_input_mln_frame_demand<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_frame_demand> {
    let _ = storage;
    if value.is_none() {
        return Ok(unsafe { sys::mln_frame_demand_default() });
    }
    let mut raw: sys::mln_frame_demand = unsafe { sys::mln_frame_demand_default() };
    raw.size = std::mem::size_of::<sys::mln_frame_demand>() as _;
    raw.flags = value
        .getattr("flags")?
        .extract::<sys::mln_frame_demand_flag>()?;
    raw.token = value.getattr("token")?.extract::<u64>()?;
    raw.coalescing_boundary = value.getattr("coalescing_boundary")?.extract::<u64>()?;
    raw.timeout_ns = value.getattr("timeout_ns")?.extract::<u64>()?;
    Ok(raw)
}

fn generated_input_mln_free_camera_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_free_camera_options> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_free_camera_options_default() });
    }
    let mut raw: sys::mln_free_camera_options = unsafe { sys::mln_free_camera_options_default() };
    raw.size = std::mem::size_of::<sys::mln_free_camera_options>() as _;
    raw.fields = 0;
    if let Some(field) = generated_present(value, "position")? {
        raw.position = generated_input_mln_vec3(&field, storage)?;
        raw.fields |= sys::MLN_FREE_CAMERA_OPTION_POSITION;
    }
    if let Some(field) = generated_present(value, "orientation")? {
        raw.orientation = generated_input_mln_quaternion(&field, storage)?;
        raw.fields |= sys::MLN_FREE_CAMERA_OPTION_ORIENTATION;
    }
    Ok(raw)
}

fn generated_input_mln_geojson_source_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_geojson_source_options> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_geojson_source_options_default() });
    }
    let mut raw: sys::mln_geojson_source_options =
        unsafe { sys::mln_geojson_source_options_default() };
    raw.size = std::mem::size_of::<sys::mln_geojson_source_options>() as _;
    raw.fields = 0;
    if let Some(field) = generated_present(value, "min_zoom")? {
        raw.min_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM;
    }
    if let Some(field) = generated_present(value, "max_zoom")? {
        raw.max_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM;
    }
    if let Some(field) = generated_present(value, "tolerance")? {
        raw.tolerance = field.extract::<f64>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_TOLERANCE;
    }
    if let Some(field) = generated_present(value, "cluster_max_zoom")? {
        raw.cluster_max_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM;
    }
    if let Some(field) = generated_present(value, "cluster_properties")? {
        raw.cluster_properties = storage.buffer(field, false)?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
    }
    if let Some(field) = generated_present(value, "tile_size")? {
        raw.tile_size = field.extract::<u32>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE;
    }
    if let Some(field) = generated_present(value, "buffer")? {
        raw.buffer = field.extract::<u32>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_BUFFER;
    }
    if let Some(field) = generated_present(value, "cluster_radius")? {
        raw.cluster_radius = field.extract::<u32>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS;
    }
    if let Some(field) = generated_present(value, "cluster_min_points")? {
        raw.cluster_min_points = field.extract::<u32>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS;
    }
    if let Some(field) = generated_present(value, "line_metrics")? {
        raw.line_metrics = field.extract::<bool>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS;
    }
    if let Some(field) = generated_present(value, "cluster")? {
        raw.cluster = field.extract::<bool>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
    }
    if let Some(field) = generated_present(value, "synchronous_tiling")? {
        raw.synchronous_tiling = field.extract::<bool>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING;
    }
    Ok(raw)
}

fn generated_input_mln_gpu_sync<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_gpu_sync> {
    let _ = storage;
    if value.is_none() {
        return Ok(unsafe { sys::mln_gpu_sync_default() });
    }
    let mut raw: sys::mln_gpu_sync = unsafe { sys::mln_gpu_sync_default() };
    raw.size = std::mem::size_of::<sys::mln_gpu_sync>() as _;
    raw.kind = value.getattr("kind")?.extract::<sys::mln_gpu_sync_kind>()?;
    raw.object = value.getattr("object")?.extract::<u64>()?;
    raw.value = value.getattr("value")?.extract::<u64>()?;
    Ok(raw)
}

fn generated_input_mln_http_header_transform<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_http_header_transform> {
    let mut raw: sys::mln_http_header_transform = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_http_header_transform>() as _;
    let callback_enabled = !value.getattr("callback")?.is_none();
    if callback_enabled {
        raw.user_data =
            storage.register_callbacks(vec![value.getattr("_invoke_callback")?.unbind()]);
        raw.release_user_data = Some(generated_release_callbacks);
        raw.callback = if callback_enabled {
            Some(generated_callback_mln_http_header_transform_callback)
        } else {
            None
        };
    }
    Ok(raw)
}

fn generated_input_mln_image_content<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_image_content> {
    let _ = storage;
    let mut raw: sys::mln_image_content = unsafe { std::mem::zeroed() };
    raw.left = value.getattr("left")?.extract::<f32>()?;
    raw.top = value.getattr("top")?.extract::<f32>()?;
    raw.right = value.getattr("right")?.extract::<f32>()?;
    raw.bottom = value.getattr("bottom")?.extract::<f32>()?;
    Ok(raw)
}

fn generated_input_mln_image_stretch<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_image_stretch> {
    let _ = storage;
    let mut raw: sys::mln_image_stretch = unsafe { std::mem::zeroed() };
    raw.from = value.getattr("from_")?.extract::<f32>()?;
    raw.to = value.getattr("to")?.extract::<f32>()?;
    Ok(raw)
}

fn generated_input_mln_lat_lng<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_lat_lng> {
    let _ = storage;
    let mut raw: sys::mln_lat_lng = unsafe { std::mem::zeroed() };
    raw.latitude = value.getattr("latitude")?.extract::<f64>()?;
    raw.longitude = value.getattr("longitude")?.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_lat_lng_bounds<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_lat_lng_bounds> {
    let mut raw: sys::mln_lat_lng_bounds = unsafe { std::mem::zeroed() };
    raw.southwest = generated_input_mln_lat_lng(&value.getattr("southwest")?, storage)?;
    raw.northeast = generated_input_mln_lat_lng(&value.getattr("northeast")?, storage)?;
    Ok(raw)
}

fn generated_input_mln_logical_extent<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_logical_extent> {
    let _ = storage;
    let mut raw: sys::mln_logical_extent = unsafe { std::mem::zeroed() };
    raw.width = value.getattr("width")?.extract::<u32>()?;
    raw.height = value.getattr("height")?.extract::<u32>()?;
    raw.scale_factor = value.getattr("scale_factor")?.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_map_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_map_options> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_map_options_default() });
    }
    let mut raw: sys::mln_map_options = unsafe { sys::mln_map_options_default() };
    raw.size = std::mem::size_of::<sys::mln_map_options>() as _;
    raw.initial_extent =
        generated_input_mln_logical_extent(&value.getattr("initial_extent")?, storage)?;
    raw.map_mode = value.getattr("map_mode")?.extract::<sys::mln_map_mode>()?;
    raw.fast_pfor_enabled = value.getattr("fast_pfor_enabled")?.extract::<bool>()?;
    raw.event_mask = value
        .getattr("event_mask")?
        .extract::<sys::mln_runtime_event_mask>()?;
    Ok(raw)
}

fn generated_input_mln_map_tile_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_map_tile_options> {
    let _ = storage;
    if value.is_none() {
        return Ok(unsafe { sys::mln_map_tile_options_default() });
    }
    let mut raw: sys::mln_map_tile_options = unsafe { sys::mln_map_tile_options_default() };
    raw.size = std::mem::size_of::<sys::mln_map_tile_options>() as _;
    raw.fields = 0;
    if let Some(field) = generated_present(value, "prefetch_zoom_delta")? {
        raw.prefetch_zoom_delta = field.extract::<u32>()?;
        raw.fields |= sys::MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
    }
    if let Some(field) = generated_present(value, "lod_min_radius")? {
        raw.lod_min_radius = field.extract::<f64>()?;
        raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS;
    }
    if let Some(field) = generated_present(value, "lod_scale")? {
        raw.lod_scale = field.extract::<f64>()?;
        raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_SCALE;
    }
    if let Some(field) = generated_present(value, "lod_pitch_threshold")? {
        raw.lod_pitch_threshold = field.extract::<f64>()?;
        raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD;
    }
    if let Some(field) = generated_present(value, "lod_zoom_shift")? {
        raw.lod_zoom_shift = field.extract::<f64>()?;
        raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT;
    }
    if let Some(field) = generated_present(value, "lod_mode")? {
        raw.lod_mode = field.extract::<sys::mln_tile_lod_mode>()?;
        raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_MODE;
    }
    Ok(raw)
}

fn generated_input_mln_map_viewport_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_map_viewport_options> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_map_viewport_options_default() });
    }
    let mut raw: sys::mln_map_viewport_options = unsafe { sys::mln_map_viewport_options_default() };
    raw.size = std::mem::size_of::<sys::mln_map_viewport_options>() as _;
    raw.fields = 0;
    if let Some(field) = generated_present(value, "north_orientation")? {
        raw.north_orientation = field.extract::<sys::mln_north_orientation>()?;
        raw.fields |= sys::MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
    }
    if let Some(field) = generated_present(value, "constrain_mode")? {
        raw.constrain_mode = field.extract::<sys::mln_constrain_mode>()?;
        raw.fields |= sys::MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
    }
    if let Some(field) = generated_present(value, "viewport_mode")? {
        raw.viewport_mode = field.extract::<sys::mln_viewport_mode>()?;
        raw.fields |= sys::MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
    }
    if let Some(field) = generated_present(value, "frustum_offset")? {
        raw.frustum_offset = generated_input_mln_edge_insets(&field, storage)?;
        raw.fields |= sys::MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
    }
    Ok(raw)
}

fn generated_input_mln_metal_borrowed_texture_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_metal_borrowed_texture_descriptor> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_metal_borrowed_texture_descriptor_default() });
    }
    let mut raw: sys::mln_metal_borrowed_texture_descriptor =
        unsafe { sys::mln_metal_borrowed_texture_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_metal_borrowed_texture_descriptor>() as _;
    raw.extent = generated_input_mln_render_target_extent(&value.getattr("extent")?, storage)?;
    raw.physical_width = value.getattr("physical_width")?.extract::<u32>()?;
    raw.physical_height = value.getattr("physical_height")?.extract::<u32>()?;
    raw.texture = value.getattr("texture")?.extract::<usize>()? as _;
    Ok(raw)
}

fn generated_input_mln_metal_context_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_metal_context_descriptor> {
    let _ = storage;
    let mut raw: sys::mln_metal_context_descriptor = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_metal_context_descriptor>() as _;
    raw.device = value.getattr("device")?.extract::<usize>()? as _;
    Ok(raw)
}

fn generated_input_mln_metal_owned_texture_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_metal_owned_texture_descriptor> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_metal_owned_texture_descriptor_default() });
    }
    let mut raw: sys::mln_metal_owned_texture_descriptor =
        unsafe { sys::mln_metal_owned_texture_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_metal_owned_texture_descriptor>() as _;
    raw.extent = generated_input_mln_render_target_extent(&value.getattr("extent")?, storage)?;
    raw.context =
        generated_input_mln_metal_context_descriptor(&value.getattr("context")?, storage)?;
    Ok(raw)
}

fn generated_input_mln_metal_surface_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_metal_surface_descriptor> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_metal_surface_descriptor_default() });
    }
    let mut raw: sys::mln_metal_surface_descriptor =
        unsafe { sys::mln_metal_surface_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_metal_surface_descriptor>() as _;
    raw.extent = generated_input_mln_render_target_extent(&value.getattr("extent")?, storage)?;
    raw.context =
        generated_input_mln_metal_context_descriptor(&value.getattr("context")?, storage)?;
    raw.layer = value.getattr("layer")?.extract::<usize>()? as _;
    Ok(raw)
}

fn generated_input_mln_offline_geometry_region_definition<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_offline_geometry_region_definition> {
    let mut raw: sys::mln_offline_geometry_region_definition = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_offline_geometry_region_definition>() as _;
    raw.style_url = storage.c_string(value.getattr("style_url")?)?;
    raw.geometry = storage.buffer(value.getattr("geometry")?, false)?;
    raw.min_zoom = value.getattr("min_zoom")?.extract::<f64>()?;
    raw.max_zoom = value.getattr("max_zoom")?.extract::<f64>()?;
    raw.pixel_ratio = value.getattr("pixel_ratio")?.extract::<f32>()?;
    raw.include_ideographs = value.getattr("include_ideographs")?.extract::<bool>()?;
    Ok(raw)
}

fn generated_input_mln_offline_region_definition<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_offline_region_definition> {
    let mut raw: sys::mln_offline_region_definition = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_offline_region_definition>() as _;
    let field = value.getattr("data")?;
    let tag = field.getattr("_tag")?.extract::<u32>()?;
    match tag {
        sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID => {
            raw.data.tile_pyramid = generated_input_mln_offline_tile_pyramid_region_definition(
                &field.getattr("value")?,
                storage,
            )?;
        }
        sys::MLN_OFFLINE_REGION_DEFINITION_GEOMETRY => {
            raw.data.geometry = generated_input_mln_offline_geometry_region_definition(
                &field.getattr("value")?,
                storage,
            )?;
        }
        _ => {}
    }
    raw.type_ = tag;
    Ok(raw)
}

fn generated_input_mln_offline_tile_pyramid_region_definition<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_offline_tile_pyramid_region_definition> {
    let mut raw: sys::mln_offline_tile_pyramid_region_definition = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_offline_tile_pyramid_region_definition>() as _;
    raw.style_url = storage.c_string(value.getattr("style_url")?)?;
    raw.bounds = generated_input_mln_lat_lng_bounds(&value.getattr("bounds")?, storage)?;
    raw.min_zoom = value.getattr("min_zoom")?.extract::<f64>()?;
    raw.max_zoom = value.getattr("max_zoom")?.extract::<f64>()?;
    raw.pixel_ratio = value.getattr("pixel_ratio")?.extract::<f32>()?;
    raw.include_ideographs = value.getattr("include_ideographs")?.extract::<bool>()?;
    Ok(raw)
}

fn generated_input_mln_opengl_borrowed_texture_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_opengl_borrowed_texture_descriptor> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_opengl_borrowed_texture_descriptor_default() });
    }
    let mut raw: sys::mln_opengl_borrowed_texture_descriptor =
        unsafe { sys::mln_opengl_borrowed_texture_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_opengl_borrowed_texture_descriptor>() as _;
    raw.extent = generated_input_mln_render_target_extent(&value.getattr("extent")?, storage)?;
    raw.physical_width = value.getattr("physical_width")?.extract::<u32>()?;
    raw.physical_height = value.getattr("physical_height")?.extract::<u32>()?;
    raw.context =
        generated_input_mln_opengl_context_descriptor(&value.getattr("context")?, storage)?;
    raw.texture = value.getattr("texture")?.extract::<u32>()?;
    raw.target = value.getattr("target")?.extract::<u32>()?;
    Ok(raw)
}

fn generated_input_mln_opengl_context_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_opengl_context_descriptor> {
    let mut raw: sys::mln_opengl_context_descriptor = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_opengl_context_descriptor>() as _;
    raw.ownership = value
        .getattr("ownership")?
        .extract::<sys::mln_opengl_context_ownership>()?;
    let field = value.getattr("data")?;
    let tag = field.getattr("_tag")?.extract::<u32>()?;
    match tag {
        sys::MLN_OPENGL_CONTEXT_PLATFORM_WGL => {
            raw.data.wgl =
                generated_input_mln_wgl_context_descriptor(&field.getattr("value")?, storage)?;
        }
        sys::MLN_OPENGL_CONTEXT_PLATFORM_EGL => {
            raw.data.egl =
                generated_input_mln_egl_context_descriptor(&field.getattr("value")?, storage)?;
        }
        sys::MLN_OPENGL_CONTEXT_PLATFORM_WEBGL => {
            raw.data.webgl =
                generated_input_mln_webgl_context_descriptor(&field.getattr("value")?, storage)?;
        }
        _ => {}
    }
    raw.platform = tag;
    Ok(raw)
}

fn generated_input_mln_opengl_owned_texture_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_opengl_owned_texture_descriptor> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_opengl_owned_texture_descriptor_default() });
    }
    let mut raw: sys::mln_opengl_owned_texture_descriptor =
        unsafe { sys::mln_opengl_owned_texture_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_opengl_owned_texture_descriptor>() as _;
    raw.extent = generated_input_mln_render_target_extent(&value.getattr("extent")?, storage)?;
    raw.context =
        generated_input_mln_opengl_context_descriptor(&value.getattr("context")?, storage)?;
    Ok(raw)
}

fn generated_input_mln_opengl_surface_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_opengl_surface_descriptor> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_opengl_surface_descriptor_default() });
    }
    let mut raw: sys::mln_opengl_surface_descriptor =
        unsafe { sys::mln_opengl_surface_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_opengl_surface_descriptor>() as _;
    raw.extent = generated_input_mln_render_target_extent(&value.getattr("extent")?, storage)?;
    raw.context =
        generated_input_mln_opengl_context_descriptor(&value.getattr("context")?, storage)?;
    raw.surface = value.getattr("surface")?.extract::<usize>()? as _;
    Ok(raw)
}

fn generated_input_mln_premultiplied_rgba8_image<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_premultiplied_rgba8_image> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_premultiplied_rgba8_image_default() });
    }
    let mut raw: sys::mln_premultiplied_rgba8_image =
        unsafe { sys::mln_premultiplied_rgba8_image_default() };
    raw.size = std::mem::size_of::<sys::mln_premultiplied_rgba8_image>() as _;
    raw.width = value.getattr("width")?.extract::<u32>()?;
    raw.height = value.getattr("height")?.extract::<u32>()?;
    raw.stride = value.getattr("stride")?.extract::<u32>()?;
    raw.pixels = {
        let buffer = storage.buffer(value.getattr("pixels")?, false)?;
        raw.byte_length = generated_length(buffer.size)?;
        buffer.data.cast()
    };
    Ok(raw)
}

fn generated_input_mln_projected_meters<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_projected_meters> {
    let _ = storage;
    let mut raw: sys::mln_projected_meters = unsafe { std::mem::zeroed() };
    raw.northing = value.getattr("northing")?.extract::<f64>()?;
    raw.easting = value.getattr("easting")?.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_projection_mode<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_projection_mode> {
    let _ = storage;
    if value.is_none() {
        return Ok(unsafe { sys::mln_projection_mode_default() });
    }
    let mut raw: sys::mln_projection_mode = unsafe { sys::mln_projection_mode_default() };
    raw.size = std::mem::size_of::<sys::mln_projection_mode>() as _;
    raw.fields = 0;
    if let Some(field) = generated_present(value, "axonometric")? {
        raw.axonometric = field.extract::<bool>()?;
        raw.fields |= sys::MLN_PROJECTION_MODE_AXONOMETRIC;
    }
    if let Some(field) = generated_present(value, "x_skew")? {
        raw.x_skew = field.extract::<f64>()?;
        raw.fields |= sys::MLN_PROJECTION_MODE_X_SKEW;
    }
    if let Some(field) = generated_present(value, "y_skew")? {
        raw.y_skew = field.extract::<f64>()?;
        raw.fields |= sys::MLN_PROJECTION_MODE_Y_SKEW;
    }
    Ok(raw)
}

fn generated_input_mln_quaternion<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_quaternion> {
    let _ = storage;
    let mut raw: sys::mln_quaternion = unsafe { std::mem::zeroed() };
    raw.x = value.getattr("x")?.extract::<f64>()?;
    raw.y = value.getattr("y")?.extract::<f64>()?;
    raw.z = value.getattr("z")?.extract::<f64>()?;
    raw.w = value.getattr("w")?.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_render_session_attach_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_render_session_attach_options> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_render_session_attach_options_default() });
    }
    let mut raw: sys::mln_render_session_attach_options =
        unsafe { sys::mln_render_session_attach_options_default() };
    raw.size = std::mem::size_of::<sys::mln_render_session_attach_options>() as _;
    raw.reserved = 0;
    raw.driver = value
        .getattr("driver")?
        .extract::<sys::mln_render_driver_kind>()?;
    raw.requested_texture_ring_depth = value
        .getattr("requested_texture_ring_depth")?
        .extract::<u32>()?;
    raw.frame_wake = generated_input_mln_wake(&value.getattr("frame_wake")?, storage)?;
    raw.driver_work_wake = generated_input_mln_wake(&value.getattr("driver_work_wake")?, storage)?;
    Ok(raw)
}

fn generated_input_mln_render_target_extent<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_render_target_extent> {
    let _ = storage;
    let mut raw: sys::mln_render_target_extent = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_render_target_extent>() as _;
    raw.width = value.getattr("width")?.extract::<u32>()?;
    raw.height = value.getattr("height")?.extract::<u32>()?;
    raw.scale_factor = value.getattr("scale_factor")?.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_rendered_feature_query_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_rendered_feature_query_options> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_rendered_feature_query_options_default() });
    }
    let mut raw: sys::mln_rendered_feature_query_options =
        unsafe { sys::mln_rendered_feature_query_options_default() };
    raw.size = std::mem::size_of::<sys::mln_rendered_feature_query_options>() as _;
    raw.fields = 0;
    if let Some(field) = generated_present(value, "layer_ids")? {
        raw.layer_ids = {
            let items = generated_items(&field, |item| storage.buffer(item, true))?;
            raw.layer_id_count = generated_length(items.len())?;
            storage.keep_array(items)
        };
        raw.fields |= sys::MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
    }
    let field = value.getattr("filter")?;
    raw.filter = if field.is_none() {
        std::ptr::null()
    } else {
        {
            let value = storage.buffer(field, false)?;
            storage.keep_one(value)
        }
    };
    Ok(raw)
}

fn generated_input_mln_rendered_query_geometry<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_rendered_query_geometry> {
    let mut raw: sys::mln_rendered_query_geometry = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_rendered_query_geometry>() as _;
    let field = value.getattr("data")?;
    let tag = field.getattr("_tag")?.extract::<u32>()?;
    match tag {
        sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT => {
            raw.data.point = generated_input_mln_screen_point(&field.getattr("value")?, storage)?;
        }
        sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX => {
            raw.data.box_ = generated_input_mln_screen_box(&field.getattr("value")?, storage)?;
        }
        sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING => {
            raw.data.line_string =
                generated_input_mln_screen_line_string(&field.getattr("value")?, storage)?;
        }
        _ => {}
    }
    raw.type_ = tag;
    Ok(raw)
}

fn generated_input_mln_resource_provider<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_resource_provider> {
    let mut raw: sys::mln_resource_provider = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_resource_provider>() as _;
    let callback_enabled = !value.getattr("callback")?.is_none();
    if callback_enabled {
        raw.user_data =
            storage.register_callbacks(vec![value.getattr("_invoke_callback")?.unbind()]);
        raw.release_user_data = Some(generated_release_callbacks);
        raw.callback = if callback_enabled {
            Some(generated_callback_mln_resource_provider_callback)
        } else {
            None
        };
    }
    Ok(raw)
}

fn generated_input_mln_resource_response<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_resource_response> {
    let mut raw: sys::mln_resource_response = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_resource_response>() as _;
    raw.has_modified = false;
    raw.has_expires = false;
    raw.has_retry_after = false;
    raw.status = value
        .getattr("status")?
        .extract::<sys::mln_resource_response_status>()?;
    raw.error_reason = value
        .getattr("error_reason")?
        .extract::<sys::mln_resource_error_reason>()?;
    raw.bytes = {
        let buffer = storage.buffer(value.getattr("bytes")?, false)?;
        raw.byte_count = generated_length(buffer.size)?;
        buffer.data.cast()
    };
    let field = value.getattr("error_message")?;
    raw.error_message = if field.is_none() {
        std::ptr::null()
    } else {
        storage.c_string(field)?
    };
    raw.must_revalidate = value.getattr("must_revalidate")?.extract::<bool>()?;
    if let Some(field) = generated_present(value, "modified_unix_ms")? {
        raw.modified_unix_ms = field.extract::<i64>()?;
        raw.has_modified = true;
    }
    if let Some(field) = generated_present(value, "expires_unix_ms")? {
        raw.expires_unix_ms = field.extract::<i64>()?;
        raw.has_expires = true;
    }
    let field = value.getattr("etag")?;
    raw.etag = if field.is_none() {
        std::ptr::null()
    } else {
        storage.c_string(field)?
    };
    if let Some(field) = generated_present(value, "retry_after_unix_ms")? {
        raw.retry_after_unix_ms = field.extract::<i64>()?;
        raw.has_retry_after = true;
    }
    Ok(raw)
}

fn generated_input_mln_resource_transform<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_resource_transform> {
    let mut raw: sys::mln_resource_transform = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_resource_transform>() as _;
    let callback_enabled = !value.getattr("callback")?.is_none();
    if callback_enabled {
        raw.user_data =
            storage.register_callbacks(vec![value.getattr("_invoke_callback")?.unbind()]);
        raw.release_user_data = Some(generated_release_callbacks);
        raw.callback = if callback_enabled {
            Some(generated_callback_mln_resource_transform_callback)
        } else {
            None
        };
    }
    Ok(raw)
}

fn generated_input_mln_runtime_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_runtime_options> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_runtime_options_default() });
    }
    let mut raw: sys::mln_runtime_options = unsafe { sys::mln_runtime_options_default() };
    raw.size = std::mem::size_of::<sys::mln_runtime_options>() as _;
    raw.flags = value.getattr("flags")?.extract::<u32>()?;
    let field = value.getattr("asset_path")?;
    raw.asset_path = if field.is_none() {
        std::ptr::null()
    } else {
        storage.c_string(field)?
    };
    let field = value.getattr("cache_path")?;
    raw.cache_path = if field.is_none() {
        std::ptr::null()
    } else {
        storage.c_string(field)?
    };
    raw.event_mask = value
        .getattr("event_mask")?
        .extract::<sys::mln_runtime_event_mask>()?;
    raw.event_wake = generated_input_mln_wake(&value.getattr("event_wake")?, storage)?;
    Ok(raw)
}

fn generated_input_mln_screen_box<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_screen_box> {
    let mut raw: sys::mln_screen_box = unsafe { std::mem::zeroed() };
    raw.min = generated_input_mln_screen_point(&value.getattr("min")?, storage)?;
    raw.max = generated_input_mln_screen_point(&value.getattr("max")?, storage)?;
    Ok(raw)
}

fn generated_input_mln_screen_line_string<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_screen_line_string> {
    let mut raw: sys::mln_screen_line_string = unsafe { std::mem::zeroed() };
    raw.points = {
        let items = generated_items(&value.getattr("points")?, |item| {
            generated_input_mln_screen_point(&item, storage)
        })?;
        raw.point_count = generated_length(items.len())?;
        storage.keep_array(items)
    };
    Ok(raw)
}

fn generated_input_mln_screen_point<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_screen_point> {
    let _ = storage;
    let mut raw: sys::mln_screen_point = unsafe { std::mem::zeroed() };
    raw.x = value.getattr("x")?.extract::<f64>()?;
    raw.y = value.getattr("y")?.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_source_feature_query_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_source_feature_query_options> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_source_feature_query_options_default() });
    }
    let mut raw: sys::mln_source_feature_query_options =
        unsafe { sys::mln_source_feature_query_options_default() };
    raw.size = std::mem::size_of::<sys::mln_source_feature_query_options>() as _;
    raw.fields = 0;
    if let Some(field) = generated_present(value, "source_layer_ids")? {
        raw.source_layer_ids = {
            let items = generated_items(&field, |item| storage.buffer(item, true))?;
            raw.source_layer_id_count = generated_length(items.len())?;
            storage.keep_array(items)
        };
        raw.fields |= sys::MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
    }
    let field = value.getattr("filter")?;
    raw.filter = if field.is_none() {
        std::ptr::null()
    } else {
        {
            let value = storage.buffer(field, false)?;
            storage.keep_one(value)
        }
    };
    Ok(raw)
}

fn generated_input_mln_style_image_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_style_image_options> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_style_image_options_default() });
    }
    let mut raw: sys::mln_style_image_options = unsafe { sys::mln_style_image_options_default() };
    raw.size = std::mem::size_of::<sys::mln_style_image_options>() as _;
    raw.fields = 0;
    if let Some(field) = generated_present(value, "stretch_x")? {
        raw.stretch_x = {
            let items = generated_items(&field, |item| {
                generated_input_mln_image_stretch(&item, storage)
            })?;
            raw.stretch_x_count = generated_length(items.len())?;
            storage.keep_array(items)
        };
        raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_STRETCH_X;
    }
    if let Some(field) = generated_present(value, "stretch_y")? {
        raw.stretch_y = {
            let items = generated_items(&field, |item| {
                generated_input_mln_image_stretch(&item, storage)
            })?;
            raw.stretch_y_count = generated_length(items.len())?;
            storage.keep_array(items)
        };
        raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
    }
    if let Some(field) = generated_present(value, "content")? {
        raw.content = generated_input_mln_image_content(&field, storage)?;
        raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_CONTENT;
    }
    if let Some(field) = generated_present(value, "text_fit_width")? {
        raw.text_fit_width = field.extract::<sys::mln_style_image_text_fit>()?;
        raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH;
    }
    if let Some(field) = generated_present(value, "text_fit_height")? {
        raw.text_fit_height = field.extract::<sys::mln_style_image_text_fit>()?;
        raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
    }
    if let Some(field) = generated_present(value, "pixel_ratio")? {
        raw.pixel_ratio = field.extract::<f32>()?;
        raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
    }
    if let Some(field) = generated_present(value, "sdf")? {
        raw.sdf = field.extract::<bool>()?;
        raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_SDF;
    }
    Ok(raw)
}

fn generated_input_mln_style_tile_source_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_style_tile_source_options> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_style_tile_source_options_default() });
    }
    let mut raw: sys::mln_style_tile_source_options =
        unsafe { sys::mln_style_tile_source_options_default() };
    raw.size = std::mem::size_of::<sys::mln_style_tile_source_options>() as _;
    raw.fields = 0;
    if let Some(field) = generated_present(value, "min_zoom")? {
        raw.min_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM;
    }
    if let Some(field) = generated_present(value, "max_zoom")? {
        raw.max_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM;
    }
    if let Some(field) = generated_present(value, "attribution")? {
        raw.attribution = storage.buffer(field, true)?;
        raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
    }
    if let Some(field) = generated_present(value, "scheme")? {
        raw.scheme = field.extract::<sys::mln_style_tile_scheme>()?;
        raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_SCHEME;
    }
    if let Some(field) = generated_present(value, "bounds")? {
        raw.bounds = generated_input_mln_lat_lng_bounds(&field, storage)?;
        raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS;
    }
    if let Some(field) = generated_present(value, "tile_size")? {
        raw.tile_size = field.extract::<u32>()?;
        raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE;
    }
    if let Some(field) = generated_present(value, "vector_encoding")? {
        raw.vector_encoding = field.extract::<sys::mln_style_vector_tile_encoding>()?;
        raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
    }
    if let Some(field) = generated_present(value, "raster_encoding")? {
        raw.raster_encoding = field.extract::<sys::mln_style_raster_dem_encoding>()?;
        raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
    }
    Ok(raw)
}

fn generated_input_mln_style_transition_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_style_transition_options> {
    let _ = storage;
    if value.is_none() {
        return Ok(unsafe { sys::mln_style_transition_options_default() });
    }
    let mut raw: sys::mln_style_transition_options =
        unsafe { sys::mln_style_transition_options_default() };
    raw.size = std::mem::size_of::<sys::mln_style_transition_options>() as _;
    raw.fields = 0;
    if let Some(field) = generated_present(value, "duration_ms")? {
        raw.duration_ms = field.extract::<f64>()?;
        raw.fields |= sys::MLN_STYLE_TRANSITION_OPTION_DURATION;
    }
    if let Some(field) = generated_present(value, "delay_ms")? {
        raw.delay_ms = field.extract::<f64>()?;
        raw.fields |= sys::MLN_STYLE_TRANSITION_OPTION_DELAY;
    }
    if let Some(field) = generated_present(value, "enable_placement_transitions")? {
        raw.enable_placement_transitions = field.extract::<bool>()?;
        raw.fields |= sys::MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
    }
    Ok(raw)
}

fn generated_input_mln_unit_bezier<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_unit_bezier> {
    let _ = storage;
    let mut raw: sys::mln_unit_bezier = unsafe { std::mem::zeroed() };
    raw.x1 = value.getattr("x1")?.extract::<f64>()?;
    raw.y1 = value.getattr("y1")?.extract::<f64>()?;
    raw.x2 = value.getattr("x2")?.extract::<f64>()?;
    raw.y2 = value.getattr("y2")?.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_vec3<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_vec3> {
    let _ = storage;
    let mut raw: sys::mln_vec3 = unsafe { std::mem::zeroed() };
    raw.x = value.getattr("x")?.extract::<f64>()?;
    raw.y = value.getattr("y")?.extract::<f64>()?;
    raw.z = value.getattr("z")?.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_vulkan_borrowed_texture_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_vulkan_borrowed_texture_descriptor> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_vulkan_borrowed_texture_descriptor_default() });
    }
    let mut raw: sys::mln_vulkan_borrowed_texture_descriptor =
        unsafe { sys::mln_vulkan_borrowed_texture_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_vulkan_borrowed_texture_descriptor>() as _;
    raw.extent = generated_input_mln_render_target_extent(&value.getattr("extent")?, storage)?;
    raw.physical_width = value.getattr("physical_width")?.extract::<u32>()?;
    raw.physical_height = value.getattr("physical_height")?.extract::<u32>()?;
    raw.context =
        generated_input_mln_vulkan_context_descriptor(&value.getattr("context")?, storage)?;
    raw.image = value.getattr("image")?.extract::<u64>()?;
    raw.image_view = value.getattr("image_view")?.extract::<u64>()?;
    raw.format = value.getattr("format")?.extract::<u32>()?;
    raw.initial_layout = value.getattr("initial_layout")?.extract::<u32>()?;
    raw.final_layout = value.getattr("final_layout")?.extract::<u32>()?;
    Ok(raw)
}

fn generated_input_mln_vulkan_context_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_vulkan_context_descriptor> {
    let _ = storage;
    let mut raw: sys::mln_vulkan_context_descriptor = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_vulkan_context_descriptor>() as _;
    raw.instance = value.getattr("instance")?.extract::<usize>()? as _;
    raw.physical_device = value.getattr("physical_device")?.extract::<usize>()? as _;
    raw.device = value.getattr("device")?.extract::<usize>()? as _;
    raw.graphics_queue = value.getattr("graphics_queue")?.extract::<usize>()? as _;
    raw.graphics_queue_family_index = value
        .getattr("graphics_queue_family_index")?
        .extract::<u32>()?;
    raw.get_instance_proc_addr = value
        .getattr("get_instance_proc_addr")?
        .extract::<usize>()? as _;
    raw.get_device_proc_addr = value.getattr("get_device_proc_addr")?.extract::<usize>()? as _;
    Ok(raw)
}

fn generated_input_mln_vulkan_owned_texture_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_vulkan_owned_texture_descriptor> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_vulkan_owned_texture_descriptor_default() });
    }
    let mut raw: sys::mln_vulkan_owned_texture_descriptor =
        unsafe { sys::mln_vulkan_owned_texture_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_vulkan_owned_texture_descriptor>() as _;
    raw.extent = generated_input_mln_render_target_extent(&value.getattr("extent")?, storage)?;
    raw.context =
        generated_input_mln_vulkan_context_descriptor(&value.getattr("context")?, storage)?;
    Ok(raw)
}

fn generated_input_mln_vulkan_surface_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_vulkan_surface_descriptor> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_vulkan_surface_descriptor_default() });
    }
    let mut raw: sys::mln_vulkan_surface_descriptor =
        unsafe { sys::mln_vulkan_surface_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_vulkan_surface_descriptor>() as _;
    raw.extent = generated_input_mln_render_target_extent(&value.getattr("extent")?, storage)?;
    raw.context =
        generated_input_mln_vulkan_context_descriptor(&value.getattr("context")?, storage)?;
    raw.surface = value.getattr("surface")?.extract::<u64>()?;
    Ok(raw)
}

fn generated_input_mln_wake<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_wake> {
    let mut raw: sys::mln_wake = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_wake>() as _;
    let callback_enabled = !value.getattr("callback")?.is_none();
    if callback_enabled {
        raw.user_data =
            storage.register_callbacks(vec![value.getattr("_invoke_callback")?.unbind()]);
        raw.release_user_data = Some(generated_release_callbacks);
        raw.callback = if callback_enabled {
            Some(generated_callback_mln_wake_callback)
        } else {
            None
        };
    }
    Ok(raw)
}

fn generated_input_mln_webgl_context_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_webgl_context_descriptor> {
    let mut raw: sys::mln_webgl_context_descriptor = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_webgl_context_descriptor>() as _;
    raw.kind = value
        .getattr("kind")?
        .extract::<sys::mln_webgl_context_kind>()?;
    raw.context = value.getattr("context")?.extract::<i32>()?;
    raw.canvas_selector = storage.buffer(value.getattr("canvas_selector")?, true)?;
    Ok(raw)
}

fn generated_input_mln_webgpu_borrowed_texture_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_webgpu_borrowed_texture_descriptor> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_webgpu_borrowed_texture_descriptor_default() });
    }
    let mut raw: sys::mln_webgpu_borrowed_texture_descriptor =
        unsafe { sys::mln_webgpu_borrowed_texture_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_webgpu_borrowed_texture_descriptor>() as _;
    raw.extent = generated_input_mln_render_target_extent(&value.getattr("extent")?, storage)?;
    raw.physical_width = value.getattr("physical_width")?.extract::<u32>()?;
    raw.physical_height = value.getattr("physical_height")?.extract::<u32>()?;
    raw.context =
        generated_input_mln_webgpu_context_descriptor(&value.getattr("context")?, storage)?;
    raw.texture = value.getattr("texture")?.extract::<usize>()? as _;
    raw.texture_view = value.getattr("texture_view")?.extract::<usize>()? as _;
    raw.format = value.getattr("format")?.extract::<u32>()?;
    Ok(raw)
}

fn generated_input_mln_webgpu_context_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_webgpu_context_descriptor> {
    let _ = storage;
    let mut raw: sys::mln_webgpu_context_descriptor = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_webgpu_context_descriptor>() as _;
    raw.instance = value.getattr("instance")?.extract::<usize>()? as _;
    raw.device = value.getattr("device")?.extract::<usize>()? as _;
    raw.queue = value.getattr("queue")?.extract::<usize>()? as _;
    Ok(raw)
}

fn generated_input_mln_webgpu_owned_texture_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_webgpu_owned_texture_descriptor> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_webgpu_owned_texture_descriptor_default() });
    }
    let mut raw: sys::mln_webgpu_owned_texture_descriptor =
        unsafe { sys::mln_webgpu_owned_texture_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_webgpu_owned_texture_descriptor>() as _;
    raw.extent = generated_input_mln_render_target_extent(&value.getattr("extent")?, storage)?;
    raw.context =
        generated_input_mln_webgpu_context_descriptor(&value.getattr("context")?, storage)?;
    Ok(raw)
}

fn generated_input_mln_webgpu_surface_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_webgpu_surface_descriptor> {
    if value.is_none() {
        return Ok(unsafe { sys::mln_webgpu_surface_descriptor_default() });
    }
    let mut raw: sys::mln_webgpu_surface_descriptor =
        unsafe { sys::mln_webgpu_surface_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_webgpu_surface_descriptor>() as _;
    raw.extent = generated_input_mln_render_target_extent(&value.getattr("extent")?, storage)?;
    raw.context =
        generated_input_mln_webgpu_context_descriptor(&value.getattr("context")?, storage)?;
    raw.surface = value.getattr("surface")?.extract::<usize>()? as _;
    raw.format = value.getattr("format")?.extract::<u32>()?;
    Ok(raw)
}

fn generated_input_mln_wgl_context_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_wgl_context_descriptor> {
    let _ = storage;
    let mut raw: sys::mln_wgl_context_descriptor = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_wgl_context_descriptor>() as _;
    raw.device_context = value.getattr("device_context")?.extract::<usize>()? as _;
    raw.share_context = value.getattr("share_context")?.extract::<usize>()? as _;
    raw.get_proc_address = value.getattr("get_proc_address")?.extract::<usize>()? as _;
    Ok(raw)
}
#[pyfunction]
fn _default_animation_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_animation_options_default() };
    generated_copy_mln_animation_options(py, &value)
}

#[pyfunction]
fn _default_bound_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_bound_options_default() };
    generated_copy_mln_bound_options(py, &value)
}

#[pyfunction]
fn _default_camera_delta(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_camera_delta_default() };
    generated_copy_mln_camera_delta(py, &value)
}

#[pyfunction]
fn _default_camera_fit_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_camera_fit_options_default() };
    generated_copy_mln_camera_fit_options(py, &value)
}

#[pyfunction]
fn _default_camera_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_camera_options_default() };
    generated_copy_mln_camera_options(py, &value)
}

#[pyfunction]
fn _default_camera_update(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_camera_update_default() };
    generated_copy_mln_camera_update(py, &value)
}

#[pyfunction]
fn _default_custom_geometry_source_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_custom_geometry_source_options_default() };
    generated_copy_mln_custom_geometry_source_options(py, &value)
}

#[pyfunction]
fn _default_custom_mvt_vector_source_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_custom_mvt_vector_source_options_default() };
    generated_copy_mln_custom_mvt_vector_source_options(py, &value)
}

#[pyfunction]
fn _default_frame_demand(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_frame_demand_default() };
    generated_copy_mln_frame_demand(py, &value)
}

#[pyfunction]
fn _default_free_camera_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_free_camera_options_default() };
    generated_copy_mln_free_camera_options(py, &value)
}

#[pyfunction]
fn _default_geojson_source_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_geojson_source_options_default() };
    generated_copy_mln_geojson_source_options(py, &value)
}

#[pyfunction]
fn _default_gpu_sync(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_gpu_sync_default() };
    generated_copy_mln_gpu_sync(py, &value)
}

#[pyfunction]
fn _default_map_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_map_options_default() };
    generated_copy_mln_map_options(py, &value)
}

#[pyfunction]
fn _default_map_tile_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_map_tile_options_default() };
    generated_copy_mln_map_tile_options(py, &value)
}

#[pyfunction]
fn _default_map_viewport_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_map_viewport_options_default() };
    generated_copy_mln_map_viewport_options(py, &value)
}

#[pyfunction]
fn _default_metal_borrowed_texture_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_metal_borrowed_texture_descriptor_default() };
    generated_copy_mln_metal_borrowed_texture_descriptor(py, &value)
}

#[pyfunction]
fn _default_metal_owned_texture_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_metal_owned_texture_descriptor_default() };
    generated_copy_mln_metal_owned_texture_descriptor(py, &value)
}

#[pyfunction]
fn _default_metal_surface_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_metal_surface_descriptor_default() };
    generated_copy_mln_metal_surface_descriptor(py, &value)
}

#[pyfunction]
fn _default_opengl_borrowed_texture_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_opengl_borrowed_texture_descriptor_default() };
    generated_copy_mln_opengl_borrowed_texture_descriptor(py, &value)
}

#[pyfunction]
fn _default_opengl_owned_texture_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_opengl_owned_texture_descriptor_default() };
    generated_copy_mln_opengl_owned_texture_descriptor(py, &value)
}

#[pyfunction]
fn _default_opengl_surface_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_opengl_surface_descriptor_default() };
    generated_copy_mln_opengl_surface_descriptor(py, &value)
}

#[pyfunction]
fn _default_premultiplied_rgba8_image(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_premultiplied_rgba8_image_default() };
    generated_copy_mln_premultiplied_rgba8_image(py, &value)
}

#[pyfunction]
fn _default_projection_mode(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_projection_mode_default() };
    generated_copy_mln_projection_mode(py, &value)
}

#[pyfunction]
fn _default_render_session_attach_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_render_session_attach_options_default() };
    generated_copy_mln_render_session_attach_options(py, &value)
}

#[pyfunction]
fn _default_rendered_feature_query_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_rendered_feature_query_options_default() };
    generated_copy_mln_rendered_feature_query_options(py, &value)
}

#[pyfunction]
fn _default_runtime_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_runtime_options_default() };
    generated_copy_mln_runtime_options(py, &value)
}

#[pyfunction]
fn _default_source_feature_query_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_source_feature_query_options_default() };
    generated_copy_mln_source_feature_query_options(py, &value)
}

#[pyfunction]
fn _default_style_image_info(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_style_image_info_default() };
    generated_copy_mln_style_image_info(py, &value)
}

#[pyfunction]
fn _default_style_image_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_style_image_options_default() };
    generated_copy_mln_style_image_options(py, &value)
}

#[pyfunction]
fn _default_style_tile_source_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_style_tile_source_options_default() };
    generated_copy_mln_style_tile_source_options(py, &value)
}

#[pyfunction]
fn _default_style_transition_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_style_transition_options_default() };
    generated_copy_mln_style_transition_options(py, &value)
}

#[pyfunction]
fn _default_texture_image_info(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_texture_image_info_default() };
    generated_copy_mln_texture_image_info(py, &value)
}

#[pyfunction]
fn _default_vulkan_borrowed_texture_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_vulkan_borrowed_texture_descriptor_default() };
    generated_copy_mln_vulkan_borrowed_texture_descriptor(py, &value)
}

#[pyfunction]
fn _default_vulkan_owned_texture_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_vulkan_owned_texture_descriptor_default() };
    generated_copy_mln_vulkan_owned_texture_descriptor(py, &value)
}

#[pyfunction]
fn _default_vulkan_surface_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_vulkan_surface_descriptor_default() };
    generated_copy_mln_vulkan_surface_descriptor(py, &value)
}

#[pyfunction]
fn _default_webgpu_borrowed_texture_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_webgpu_borrowed_texture_descriptor_default() };
    generated_copy_mln_webgpu_borrowed_texture_descriptor(py, &value)
}

#[pyfunction]
fn _default_webgpu_owned_texture_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_webgpu_owned_texture_descriptor_default() };
    generated_copy_mln_webgpu_owned_texture_descriptor(py, &value)
}

#[pyfunction]
fn _default_webgpu_surface_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_webgpu_surface_descriptor_default() };
    generated_copy_mln_webgpu_surface_descriptor(py, &value)
}

unsafe extern "C" fn generated_dispose_mln_acquired_frame(
    handle: sys::mln_acquired_frame,
) -> sys::mln_status {
    unsafe { sys::mln_acquired_frame_dispose(handle, std::ptr::null_mut()) }
}

unsafe extern "C" fn generated_dispose_mln_buffer(handle: sys::mln_buffer) -> sys::mln_status {
    unsafe { sys::mln_buffer_destroy(handle) };
    sys::MLN_STATUS_OK
}

unsafe extern "C" fn generated_dispose_mln_event_batch(
    handle: sys::mln_event_batch,
) -> sys::mln_status {
    unsafe { sys::mln_event_batch_release(handle) };
    sys::MLN_STATUS_OK
}

unsafe extern "C" fn generated_dispose_mln_geojson_source_data(
    handle: sys::mln_geojson_source_data,
) -> sys::mln_status {
    unsafe { sys::mln_geojson_source_data_destroy(handle) };
    sys::MLN_STATUS_OK
}

unsafe extern "C" fn generated_dispose_mln_map(handle: sys::mln_map) -> sys::mln_status {
    unsafe { sys::mln_map_dispose(handle, std::ptr::null_mut()) }
}

unsafe extern "C" fn generated_dispose_mln_map_projection(
    handle: sys::mln_map_projection,
) -> sys::mln_status {
    unsafe { sys::mln_map_projection_close(handle, std::ptr::null_mut()) }
}

unsafe extern "C" fn generated_dispose_mln_render_frame_batch(
    handle: sys::mln_render_frame_batch,
) -> sys::mln_status {
    unsafe { sys::mln_render_frame_batch_release(handle) };
    sys::MLN_STATUS_OK
}

unsafe extern "C" fn generated_dispose_mln_render_session(
    handle: sys::mln_render_session,
) -> sys::mln_status {
    unsafe { sys::mln_render_session_dispose(handle, std::ptr::null_mut()) }
}

unsafe extern "C" fn generated_dispose_mln_runtime(handle: sys::mln_runtime) -> sys::mln_status {
    unsafe { sys::mln_runtime_dispose(handle, std::ptr::null_mut()) }
}

#[pymethods]
impl AcquiredFrameHandle {
    #[pyo3(signature = ())]
    fn with_metal_texture(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_acquired_frame_get_metal_texture", self.admission())?;
        let handle = self.live()?;
        let mut out_frame: sys::mln_metal_owned_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_metal_owned_texture_frame>() as _;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_acquired_frame_get_metal_texture(handle, &mut out_frame, diagnostic)
            })
        }?;
        generated_copy_mln_metal_owned_texture_frame(py, &out_frame)
    }
    #[pyo3(signature = ())]
    fn with_opengl_texture(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_acquired_frame_get_opengl_texture",
            self.admission(),
        )?;
        let handle = self.live()?;
        let mut out_frame: sys::mln_opengl_owned_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_opengl_owned_texture_frame>() as _;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_acquired_frame_get_opengl_texture(handle, &mut out_frame, diagnostic)
            })
        }?;
        generated_copy_mln_opengl_owned_texture_frame(py, &out_frame)
    }
    #[pyo3(signature = ())]
    fn with_producer_sync(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_acquired_frame_get_producer_sync", self.admission())?;
        let handle = self.live()?;
        let mut out_sync: sys::mln_gpu_sync = unsafe { sys::mln_gpu_sync_default() };
        out_sync.size = std::mem::size_of::<sys::mln_gpu_sync>() as _;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_acquired_frame_get_producer_sync(handle, &mut out_sync, diagnostic)
            })
        }?;
        generated_copy_mln_gpu_sync(py, &out_sync)
    }
    #[pyo3(signature = ())]
    fn get_result(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_acquired_frame_get_result", self.admission())?;
        let handle = self.live()?;
        let mut out_result: sys::mln_render_frame_result = unsafe { std::mem::zeroed() };
        out_result.size = std::mem::size_of::<sys::mln_render_frame_result>() as _;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_acquired_frame_get_result(handle, &mut out_result, diagnostic)
            })
        }?;
        generated_copy_mln_render_frame_result(py, &out_result)
    }
    #[pyo3(signature = ())]
    fn with_vulkan_texture(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_acquired_frame_get_vulkan_texture",
            self.admission(),
        )?;
        let handle = self.live()?;
        let mut out_frame: sys::mln_vulkan_owned_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_vulkan_owned_texture_frame>() as _;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_acquired_frame_get_vulkan_texture(handle, &mut out_frame, diagnostic)
            })
        }?;
        generated_copy_mln_vulkan_owned_texture_frame(py, &out_frame)
    }
    #[pyo3(signature = ())]
    fn with_webgpu_texture(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_acquired_frame_get_webgpu_texture",
            self.admission(),
        )?;
        let handle = self.live()?;
        let mut out_frame: sys::mln_webgpu_owned_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_webgpu_owned_texture_frame>() as _;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_acquired_frame_get_webgpu_texture(handle, &mut out_frame, diagnostic)
            })
        }?;
        generated_copy_mln_webgpu_owned_texture_frame(py, &out_frame)
    }
    #[pyo3(signature = (consumer_completion=None))]
    fn close(
        &self,
        py: Python<'_>,
        consumer_completion: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_acquired_frame_release", self.admission())?;
        let storage = &mut call.storage;
        let consumer_completion = consumer_completion.unwrap_or_else(|| py.None().into_bound(py));
        let consumer_completion_value =
            generated_input_mln_gpu_sync(&consumer_completion.clone(), storage)?;
        let Some(mut reservation) = self.reserve()? else {
            return Ok(py.None());
        };
        let mut handle = reservation.handle();
        unsafe {
            call.status(|diagnostic| {
                sys::mln_acquired_frame_release(&mut handle, &consumer_completion_value, diagnostic)
            })
        }?;
        reservation.commit();
        Ok(py.None())
    }
}

#[pymethods]
impl BufferHandle {
    #[pyo3(signature = ())]
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_buffer_destroy", self.admission())?;
        let Some(mut reservation) = self.reserve()? else {
            return Ok(py.None());
        };
        let handle = reservation.handle();
        unsafe { call.run(|| sys::mln_buffer_destroy(handle)) };
        reservation.commit();
        Ok(py.None())
    }
    #[pyo3(signature = ())]
    fn get(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_buffer_get", self.admission())?;
        let read = self.read()?;
        let handle = read.handle;
        let mut out_view: sys::mln_buffer_view = unsafe { std::mem::zeroed() };
        unsafe {
            call.status(|diagnostic| sys::mln_buffer_get(handle, &mut out_view, diagnostic))
        }?;
        unsafe { generated_bytes(py, out_view) }
    }
}

#[pymethods]
impl EventBatchHandle {
    #[pyo3(signature = ())]
    fn get(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_event_batch_get", self.admission())?;
        let read = self.read()?;
        let handle = read.handle;
        let mut out_view: sys::mln_runtime_event_batch_view = unsafe { std::mem::zeroed() };
        out_view.size = std::mem::size_of::<sys::mln_runtime_event_batch_view>() as _;
        unsafe {
            call.status(|diagnostic| sys::mln_event_batch_get(handle, &mut out_view, diagnostic))
        }?;
        generated_copy_mln_runtime_event_batch_view(py, &out_view)
    }
    #[pyo3(signature = ())]
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_event_batch_release", self.admission())?;
        let Some(mut reservation) = self.reserve()? else {
            return Ok(py.None());
        };
        let handle = reservation.handle();
        unsafe { call.run(|| sys::mln_event_batch_release(handle)) };
        reservation.commit();
        Ok(py.None())
    }
}

#[pymethods]
impl GeojsonSourceDataHandle {
    #[pyo3(signature = ())]
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_geojson_source_data_destroy", self.admission())?;
        let Some(mut reservation) = self.reserve()? else {
            return Ok(py.None());
        };
        let handle = reservation.handle();
        unsafe { call.run(|| sys::mln_geojson_source_data_destroy(handle)) };
        reservation.commit();
        Ok(py.None())
    }
}

#[pymethods]
impl HttpHeaderTransformResponseScope {
    #[pyo3(signature = (name, value))]
    fn set(
        &self,
        py: Python<'_>,
        name: &Bound<'_, PyAny>,
        value: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_http_header_transform_response_set",
            self.native as u64,
        )?;
        let storage = &mut call.storage;
        let name_view = storage.buffer(name.clone(), true)?;
        let value_view = storage.buffer(value.clone(), true)?;
        let handle =
            self.scope.pointer(self.native)? as *mut sys::mln_http_header_transform_response;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_http_header_transform_response_set(
                    handle,
                    name_view.data.cast(),
                    name_view.size,
                    value_view.data.cast(),
                    value_view.size,
                    diagnostic,
                )
            })
        }?;
        Ok(py.None())
    }
}

#[pymethods]
impl MapHandle {
    #[pyo3(signature = (layer_id, source_id, before_layer_id=None))]
    fn add_color_relief_layer(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        source_id: &Bound<'_, PyAny>,
        before_layer_id: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_add_color_relief_layer", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let before_layer_id = before_layer_id.unwrap_or_else(|| py.None().into_bound(py));
        let before_layer_id_value = storage.buffer(before_layer_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_color_relief_layer(
                    handle,
                    layer_id_value,
                    source_id_value,
                    before_layer_id_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, options=None))]
    fn add_custom_geometry_source(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_add_custom_geometry_source", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value =
            generated_input_mln_custom_geometry_source_options(&options.clone(), storage)?;
        let handle = self.live()?;
        let future = unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_custom_geometry_source(
                    handle,
                    source_id_value,
                    &options_value,
                    completion,
                    diagnostic,
                )
            })
        }?;
        let callback_roots = call.accept_callbacks();
        self.state().retain_callback_roots(callback_roots);
        Ok(future)
    }
    #[pyo3(signature = (source_id, options=None))]
    fn add_custom_mvt_vector_source(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_add_custom_mvt_vector_source", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value =
            generated_input_mln_custom_mvt_vector_source_options(&options.clone(), storage)?;
        let handle = self.live()?;
        let future = unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_custom_mvt_vector_source(
                    handle,
                    source_id_value,
                    &options_value,
                    completion,
                    diagnostic,
                )
            })
        }?;
        let callback_roots = call.accept_callbacks();
        self.state().retain_callback_roots(callback_roots);
        Ok(future)
    }
    #[pyo3(signature = (source_id, data))]
    fn add_geojson_source_data(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        data: &GeojsonSourceDataHandle,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_add_geojson_source_data", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let data_handle = data.input()?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_geojson_source_data(
                    handle,
                    source_id_value,
                    data_handle,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, url, options=None))]
    fn add_geojson_source_url(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        url: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_add_geojson_source_url", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let url_value = storage.buffer(url.clone(), true)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = generated_maybe(&options, |options| {
            generated_input_mln_geojson_source_options(&options, storage)
        })?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_geojson_source_url(
                    handle,
                    source_id_value,
                    url_value,
                    generated_pointer(&options_value),
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (layer_id, source_id, before_layer_id=None))]
    fn add_hillshade_layer(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        source_id: &Bound<'_, PyAny>,
        before_layer_id: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_add_hillshade_layer", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let before_layer_id = before_layer_id.unwrap_or_else(|| py.None().into_bound(py));
        let before_layer_id_value = storage.buffer(before_layer_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_hillshade_layer(
                    handle,
                    layer_id_value,
                    source_id_value,
                    before_layer_id_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, coordinates, image=None))]
    fn add_image_source_image(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        coordinates: &Bound<'_, PyAny>,
        image: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_add_image_source_image", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let coordinates_values = generated_items(coordinates, |item| {
            generated_input_mln_lat_lng(&item, storage)
        })?;
        let image = image.unwrap_or_else(|| py.None().into_bound(py));
        let image_value = generated_input_mln_premultiplied_rgba8_image(&image.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_image_source_image(
                    handle,
                    source_id_value,
                    coordinates_values.as_ptr(),
                    coordinates_values.len(),
                    &image_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, coordinates, url))]
    fn add_image_source_url(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        coordinates: &Bound<'_, PyAny>,
        url: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_add_image_source_url", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let coordinates_values = generated_items(coordinates, |item| {
            generated_input_mln_lat_lng(&item, storage)
        })?;
        let url_value = storage.buffer(url.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_image_source_url(
                    handle,
                    source_id_value,
                    coordinates_values.as_ptr(),
                    coordinates_values.len(),
                    url_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (layer_id, before_layer_id=None))]
    fn add_location_indicator_layer(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        before_layer_id: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_add_location_indicator_layer", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let before_layer_id = before_layer_id.unwrap_or_else(|| py.None().into_bound(py));
        let before_layer_id_value = storage.buffer(before_layer_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_location_indicator_layer(
                    handle,
                    layer_id_value,
                    before_layer_id_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, tiles, options=None))]
    fn add_raster_dem_source_tiles(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        tiles: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_add_raster_dem_source_tiles", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let tiles_values = generated_items(tiles, |item| storage.buffer(item, true))?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = generated_maybe(&options, |options| {
            generated_input_mln_style_tile_source_options(&options, storage)
        })?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_raster_dem_source_tiles(
                    handle,
                    source_id_value,
                    tiles_values.as_ptr(),
                    tiles_values.len(),
                    generated_pointer(&options_value),
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, url, options=None))]
    fn add_raster_dem_source_url(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        url: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_add_raster_dem_source_url", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let url_value = storage.buffer(url.clone(), true)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = generated_maybe(&options, |options| {
            generated_input_mln_style_tile_source_options(&options, storage)
        })?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_raster_dem_source_url(
                    handle,
                    source_id_value,
                    url_value,
                    generated_pointer(&options_value),
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, tiles, options=None))]
    fn add_raster_source_tiles(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        tiles: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_add_raster_source_tiles", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let tiles_values = generated_items(tiles, |item| storage.buffer(item, true))?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = generated_maybe(&options, |options| {
            generated_input_mln_style_tile_source_options(&options, storage)
        })?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_raster_source_tiles(
                    handle,
                    source_id_value,
                    tiles_values.as_ptr(),
                    tiles_values.len(),
                    generated_pointer(&options_value),
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, url, options=None))]
    fn add_raster_source_url(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        url: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_add_raster_source_url", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let url_value = storage.buffer(url.clone(), true)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = generated_maybe(&options, |options| {
            generated_input_mln_style_tile_source_options(&options, storage)
        })?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_raster_source_url(
                    handle,
                    source_id_value,
                    url_value,
                    generated_pointer(&options_value),
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (layer_json, before_layer_id=None))]
    fn add_style_layer_json(
        &self,
        py: Python<'_>,
        layer_json: &Bound<'_, PyAny>,
        before_layer_id: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_add_style_layer_json", self.admission())?;
        let storage = &mut call.storage;
        let layer_json_value = storage.buffer(layer_json.clone(), false)?;
        let before_layer_id = before_layer_id.unwrap_or_else(|| py.None().into_bound(py));
        let before_layer_id_value = storage.buffer(before_layer_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_style_layer_json(
                    handle,
                    layer_json_value,
                    before_layer_id_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, source_json))]
    fn add_style_source_json(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        source_json: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_add_style_source_json", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let source_json_value = storage.buffer(source_json.clone(), false)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_style_source_json(
                    handle,
                    source_id_value,
                    source_json_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, tiles, options=None))]
    fn add_vector_source_tiles(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        tiles: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_add_vector_source_tiles", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let tiles_values = generated_items(tiles, |item| storage.buffer(item, true))?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = generated_maybe(&options, |options| {
            generated_input_mln_style_tile_source_options(&options, storage)
        })?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_vector_source_tiles(
                    handle,
                    source_id_value,
                    tiles_values.as_ptr(),
                    tiles_values.len(),
                    generated_pointer(&options_value),
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, url, options=None))]
    fn add_vector_source_url(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        url: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_add_vector_source_url", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let url_value = storage.buffer(url.clone(), true)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = generated_maybe(&options, |options| {
            generated_input_mln_style_tile_source_options(&options, storage)
        })?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_add_vector_source_url(
                    handle,
                    source_id_value,
                    url_value,
                    generated_pointer(&options_value),
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (delta=None))]
    fn apply_camera_delta(
        &self,
        py: Python<'_>,
        delta: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_apply_camera_delta", self.admission())?;
        let storage = &mut call.storage;
        let delta = delta.unwrap_or_else(|| py.None().into_bound(py));
        let delta_value = generated_input_mln_camera_delta(&delta.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_apply_camera_delta(handle, &delta_value, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (geometry, fit_options=None))]
    fn camera_for_geometry(
        &self,
        py: Python<'_>,
        geometry: &Bound<'_, PyAny>,
        fit_options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_camera_for_geometry", self.admission())?;
        let storage = &mut call.storage;
        let geometry_value = storage.buffer(geometry.clone(), false)?;
        let fit_options = fit_options.unwrap_or_else(|| py.None().into_bound(py));
        let fit_options_value = generated_maybe(&fit_options, |fit_options| {
            generated_input_mln_camera_fit_options(&fit_options, storage)
        })?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_camera_options>(result)?;
            generated_copy_mln_camera_options(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_camera_for_geometry(
                        handle,
                        geometry_value,
                        generated_pointer(&fit_options_value),
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (bounds, fit_options=None))]
    fn camera_for_lat_lng_bounds(
        &self,
        py: Python<'_>,
        bounds: &Bound<'_, PyAny>,
        fit_options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_camera_for_lat_lng_bounds", self.admission())?;
        let storage = &mut call.storage;
        let bounds_value = generated_input_mln_lat_lng_bounds(&bounds.clone(), storage)?;
        let fit_options = fit_options.unwrap_or_else(|| py.None().into_bound(py));
        let fit_options_value = generated_maybe(&fit_options, |fit_options| {
            generated_input_mln_camera_fit_options(&fit_options, storage)
        })?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_camera_options>(result)?;
            generated_copy_mln_camera_options(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_camera_for_lat_lng_bounds(
                        handle,
                        bounds_value,
                        generated_pointer(&fit_options_value),
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (coordinates, fit_options=None))]
    fn camera_for_lat_lngs(
        &self,
        py: Python<'_>,
        coordinates: &Bound<'_, PyAny>,
        fit_options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_camera_for_lat_lngs", self.admission())?;
        let storage = &mut call.storage;
        let coordinates_values = generated_items(coordinates, |item| {
            generated_input_mln_lat_lng(&item, storage)
        })?;
        let fit_options = fit_options.unwrap_or_else(|| py.None().into_bound(py));
        let fit_options_value = generated_maybe(&fit_options, |fit_options| {
            generated_input_mln_camera_fit_options(&fit_options, storage)
        })?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_camera_options>(result)?;
            generated_copy_mln_camera_options(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_camera_for_lat_lngs(
                        handle,
                        coordinates_values.as_ptr(),
                        coordinates_values.len(),
                        generated_pointer(&fit_options_value),
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn camera_query(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_camera_query", self.admission())?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_camera_query_result>(result)?;
            generated_copy_mln_camera_query_result(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| sys::mln_map_camera_query(handle, completion, diagnostic),
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn camera_snapshot_get(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_camera_snapshot_get", self.admission())?;
        let handle = self.live()?;
        let mut out_camera: sys::mln_camera_options = unsafe { sys::mln_camera_options_default() };
        out_camera.size = std::mem::size_of::<sys::mln_camera_options>() as _;
        let mut out_generation: u64 = unsafe { std::mem::zeroed() };
        unsafe {
            call.status(|diagnostic| {
                sys::mln_map_camera_snapshot_get(
                    handle,
                    &mut out_camera,
                    &mut out_generation,
                    diagnostic,
                )
            })
        }?;
        let dict = PyDict::new(py);
        dict.set_item(
            "camera",
            generated_copy_mln_camera_options(py, &out_camera)?,
        )?;
        dict.set_item("generation", generated_value(py, out_generation)?)?;
        Ok(dict.into_any().unbind())
    }
    #[pyo3(signature = ())]
    fn cancel_transitions(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_cancel_transitions", self.admission())?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_cancel_transitions(handle, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (layer_id))]
    fn copy_layer_source_id(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_copy_layer_source_id", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_buffer_view>(result)?;
            generated_optional(py, value.size != 0, || generated_text(py, value))
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_copy_layer_source_id(
                        handle,
                        layer_id_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (layer_id))]
    fn copy_layer_source_layer(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_copy_layer_source_layer", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_buffer_view>(result)?;
            generated_optional(py, value.size != 0, || generated_text(py, value))
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_copy_layer_source_layer(
                        handle,
                        layer_id_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (image_id))]
    fn copy_style_image_premultiplied_rgba8(
        &self,
        py: Python<'_>,
        image_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_copy_style_image_premultiplied_rgba8",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let image_id_value = storage.buffer(image_id.clone(), true)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            if result.value.is_null() {
                return Ok(py.None());
            }
            let value = completion_value::<sys::mln_buffer_view>(result)?;
            generated_optional(py, !value.data.is_null(), || unsafe {
                generated_bytes(py, value)
            })
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_copy_style_image_premultiplied_rgba8(
                        handle,
                        image_id_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (image_id))]
    fn copy_style_image_stretches(
        &self,
        py: Python<'_>,
        image_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_copy_style_image_stretches", self.admission())?;
        let storage = &mut call.storage;
        let image_id_value = storage.buffer(image_id.clone(), true)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            if result.value.is_null() {
                return Ok(py.None());
            }
            let value = completion_value::<sys::mln_style_image_stretches_result>(result)?;
            generated_copy_mln_style_image_stretches_result(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_copy_style_image_stretches(
                        handle,
                        image_id_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (source_id))]
    fn copy_style_source_attribution(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_copy_style_source_attribution",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            if result.value.is_null() {
                return Ok(py.None());
            }
            let value = completion_value::<sys::mln_buffer_view>(result)?;
            generated_optional(py, !value.data.is_null(), || generated_text(py, value))
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_copy_style_source_attribution(
                        handle,
                        source_id_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (source_id))]
    fn copy_style_source_url(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_copy_style_source_url", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            if result.value.is_null() {
                return Ok(py.None());
            }
            let value = completion_value::<sys::mln_buffer_view>(result)?;
            generated_optional(py, !value.data.is_null(), || generated_text(py, value))
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_copy_style_source_url(
                        handle,
                        source_id_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn dump_debug_logs(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_dump_debug_logs", self.admission())?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_dump_debug_logs(handle, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (selector))]
    fn get_feature_state(
        &self,
        py: Python<'_>,
        selector: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_get_feature_state", self.admission())?;
        let storage = &mut call.storage;
        let selector_value =
            generated_input_mln_feature_state_selector(&selector.clone(), storage)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_buffer_view>(result)?;
            unsafe { generated_bytes(py, value) }
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_get_feature_state(handle, &selector_value, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn get_global_state(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_get_global_state", self.admission())?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_buffer_view>(result)?;
            unsafe { generated_bytes(py, value) }
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_get_global_state(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (source_id))]
    fn get_image_source_coordinates(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_get_image_source_coordinates", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            if result.value.is_null() {
                return Ok(py.None());
            }
            let list = PyList::empty(py);
            for value in generated_completion_slice::<sys::mln_lat_lng>(result)? {
                list.append(generated_copy_mln_lat_lng(py, value)?)?;
            }
            Ok(list.into_any().unbind())
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_get_image_source_coordinates(
                        handle,
                        source_id_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (layer_id))]
    fn get_layer_filter(&self, py: Python<'_>, layer_id: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_get_layer_filter", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            if result.value.is_null() {
                return Ok(py.None());
            }
            let value = completion_value::<sys::mln_buffer_view>(result)?;
            generated_optional(py, !value.data.is_null(), || unsafe {
                generated_bytes(py, value)
            })
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_get_layer_filter(handle, layer_id_value, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (layer_id, property_name))]
    fn get_layer_property(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        property_name: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_get_layer_property", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let property_name_value = storage.buffer(property_name.clone(), true)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            if result.value.is_null() {
                return Ok(py.None());
            }
            let value = completion_value::<sys::mln_buffer_view>(result)?;
            generated_optional(py, !value.data.is_null(), || unsafe {
                generated_bytes(py, value)
            })
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_get_layer_property(
                        handle,
                        layer_id_value,
                        property_name_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (image_id))]
    fn get_style_image_info(
        &self,
        py: Python<'_>,
        image_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_get_style_image_info", self.admission())?;
        let storage = &mut call.storage;
        let image_id_value = storage.buffer(image_id.clone(), true)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            if result.value.is_null() {
                return Ok(py.None());
            }
            let value = completion_value::<sys::mln_style_image_result>(result)?;
            generated_copy_mln_style_image_result(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_get_style_image_info(
                        handle,
                        image_id_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (layer_id))]
    fn get_style_layer_info(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_get_style_layer_info", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            if result.value.is_null() {
                return Ok(py.None());
            }
            let value = completion_value::<sys::mln_style_layer_result>(result)?;
            generated_copy_mln_style_layer_result(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_get_style_layer_info(
                        handle,
                        layer_id_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (layer_id))]
    fn get_style_layer_json(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_get_style_layer_json", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            if result.value.is_null() {
                return Ok(py.None());
            }
            let value = completion_value::<sys::mln_buffer_view>(result)?;
            generated_optional(py, !value.data.is_null(), || unsafe {
                generated_bytes(py, value)
            })
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_get_style_layer_json(
                        handle,
                        layer_id_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (property_name))]
    fn get_style_light_property(
        &self,
        py: Python<'_>,
        property_name: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_get_style_light_property", self.admission())?;
        let storage = &mut call.storage;
        let property_name_value = storage.buffer(property_name.clone(), true)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            if result.value.is_null() {
                return Ok(py.None());
            }
            let value = completion_value::<sys::mln_buffer_view>(result)?;
            generated_optional(py, !value.data.is_null(), || unsafe {
                generated_bytes(py, value)
            })
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_get_style_light_property(
                        handle,
                        property_name_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (source_id))]
    fn get_style_source_info(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_get_style_source_info", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            if result.value.is_null() {
                return Ok(py.None());
            }
            let value = completion_value::<sys::mln_style_source_result>(result)?;
            generated_copy_mln_style_source_result(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_get_style_source_info(
                        handle,
                        source_id_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (source_id))]
    fn get_style_source_tile_urls(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_get_style_source_tile_urls", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            if result.value.is_null() {
                return Ok(py.None());
            }
            let value = completion_value::<sys::mln_style_source_tile_urls_result>(result)?;
            generated_copy_mln_style_source_tile_urls_result(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_get_style_source_tile_urls(
                        handle,
                        source_id_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn get_style_transition_options(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_get_style_transition_options", self.admission())?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_style_transition_options>(result)?;
            generated_copy_mln_style_transition_options(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_get_style_transition_options(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (source_id, bounds))]
    fn invalidate_custom_geometry_source_region(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        bounds: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_invalidate_custom_geometry_source_region",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let bounds_value = generated_input_mln_lat_lng_bounds(&bounds.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_invalidate_custom_geometry_source_region(
                    handle,
                    source_id_value,
                    bounds_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, tile_id))]
    fn invalidate_custom_geometry_source_tile(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        tile_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_invalidate_custom_geometry_source_tile",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let tile_id_value = generated_input_mln_canonical_tile_id(&tile_id.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_invalidate_custom_geometry_source_tile(
                    handle,
                    source_id_value,
                    tile_id_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, tile_id))]
    fn invalidate_custom_mvt_vector_source_tile(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        tile_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_invalidate_custom_mvt_vector_source_tile",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let tile_id_value = generated_input_mln_canonical_tile_id(&tile_id.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_invalidate_custom_mvt_vector_source_tile(
                    handle,
                    source_id_value,
                    tile_id_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (camera=None))]
    fn lat_lng_bounds_for_camera(
        &self,
        py: Python<'_>,
        camera: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_lat_lng_bounds_for_camera", self.admission())?;
        let storage = &mut call.storage;
        let camera = camera.unwrap_or_else(|| py.None().into_bound(py));
        let camera_value = generated_input_mln_camera_options(&camera.clone(), storage)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_lat_lng_bounds>(result)?;
            generated_copy_mln_lat_lng_bounds(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_lat_lng_bounds_for_camera(
                        handle,
                        &camera_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (camera=None))]
    fn lat_lng_bounds_for_camera_unwrapped(
        &self,
        py: Python<'_>,
        camera: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_lat_lng_bounds_for_camera_unwrapped",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let camera = camera.unwrap_or_else(|| py.None().into_bound(py));
        let camera_value = generated_input_mln_camera_options(&camera.clone(), storage)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_lat_lng_bounds>(result)?;
            generated_copy_mln_lat_lng_bounds(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_lat_lng_bounds_for_camera_unwrapped(
                        handle,
                        &camera_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (point))]
    fn lat_lng_for_pixel(&self, py: Python<'_>, point: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_lat_lng_for_pixel", self.admission())?;
        let storage = &mut call.storage;
        let point_value = generated_input_mln_screen_point(&point.clone(), storage)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_lat_lng>(result)?;
            generated_copy_mln_lat_lng(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_lat_lng_for_pixel(handle, point_value, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (point))]
    fn lat_lng_for_pixel_unwrapped(
        &self,
        py: Python<'_>,
        point: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_lat_lng_for_pixel_unwrapped", self.admission())?;
        let storage = &mut call.storage;
        let point_value = generated_input_mln_screen_point(&point.clone(), storage)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_lat_lng>(result)?;
            generated_copy_mln_lat_lng(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_lat_lng_for_pixel_unwrapped(
                        handle,
                        point_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (points))]
    fn lat_lngs_for_pixels(
        &self,
        py: Python<'_>,
        points: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_lat_lngs_for_pixels", self.admission())?;
        let storage = &mut call.storage;
        let points_values = generated_items(points, |item| {
            generated_input_mln_screen_point(&item, storage)
        })?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let list = PyList::empty(py);
            for value in generated_completion_slice::<sys::mln_lat_lng>(result)? {
                list.append(generated_copy_mln_lat_lng(py, value)?)?;
            }
            Ok(list.into_any().unbind())
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_lat_lngs_for_pixels(
                        handle,
                        points_values.as_ptr(),
                        points_values.len(),
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (points))]
    fn lat_lngs_for_pixels_unwrapped(
        &self,
        py: Python<'_>,
        points: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_lat_lngs_for_pixels_unwrapped",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let points_values = generated_items(points, |item| {
            generated_input_mln_screen_point(&item, storage)
        })?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let list = PyList::empty(py);
            for value in generated_completion_slice::<sys::mln_lat_lng>(result)? {
                list.append(generated_copy_mln_lat_lng(py, value)?)?;
            }
            Ok(list.into_any().unbind())
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_lat_lngs_for_pixels_unwrapped(
                        handle,
                        points_values.as_ptr(),
                        points_values.len(),
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn list_style_layer_ids(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_list_style_layer_ids", self.admission())?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let list = PyList::empty(py);
            for value in generated_completion_slice::<sys::mln_buffer_view>(result)? {
                list.append(generated_text(py, *value)?)?;
            }
            Ok(list.into_any().unbind())
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_list_style_layer_ids(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn list_style_layers(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_list_style_layers", self.admission())?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let list = PyList::empty(py);
            for value in generated_completion_slice::<sys::mln_style_layer_entry>(result)? {
                list.append(generated_copy_mln_style_layer_entry(py, value)?)?;
            }
            Ok(list.into_any().unbind())
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_list_style_layers(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn list_style_source_ids(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_list_style_source_ids", self.admission())?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let list = PyList::empty(py);
            for value in generated_completion_slice::<sys::mln_buffer_view>(result)? {
                list.append(generated_text(py, *value)?)?;
            }
            Ok(list.into_any().unbind())
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_list_style_source_ids(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn loaded_style_json(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_loaded_style_json", self.admission())?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_buffer_view>(result)?;
            unsafe { generated_bytes(py, value) }
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_loaded_style_json(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (latitude))]
    fn meters_per_pixel_at_latitude(&self, py: Python<'_>, latitude: f64) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_meters_per_pixel_at_latitude", self.admission())?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<f64>(result)?;
            generated_value(py, value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_meters_per_pixel_at_latitude(
                        handle, latitude, completion, diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (layer_id, before_layer_id=None))]
    fn move_style_layer(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        before_layer_id: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_move_style_layer", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let before_layer_id = before_layer_id.unwrap_or_else(|| py.None().into_bound(py));
        let before_layer_id_value = storage.buffer(before_layer_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_move_style_layer(
                    handle,
                    layer_id_value,
                    before_layer_id_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (coordinate))]
    fn pixel_for_lat_lng(
        &self,
        py: Python<'_>,
        coordinate: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_pixel_for_lat_lng", self.admission())?;
        let storage = &mut call.storage;
        let coordinate_value = generated_input_mln_lat_lng(&coordinate.clone(), storage)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_screen_point>(result)?;
            generated_copy_mln_screen_point(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_pixel_for_lat_lng(handle, coordinate_value, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (coordinates))]
    fn pixels_for_lat_lngs(
        &self,
        py: Python<'_>,
        coordinates: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_pixels_for_lat_lngs", self.admission())?;
        let storage = &mut call.storage;
        let coordinates_values = generated_items(coordinates, |item| {
            generated_input_mln_lat_lng(&item, storage)
        })?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let list = PyList::empty(py);
            for value in generated_completion_slice::<sys::mln_screen_point>(result)? {
                list.append(generated_copy_mln_screen_point(py, value)?)?;
            }
            Ok(list.into_any().unbind())
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_pixels_for_lat_lngs(
                        handle,
                        coordinates_values.as_ptr(),
                        coordinates_values.len(),
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn projection_create(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_projection_create", self.admission())?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| unsafe {
            MapProjectionHandle::adopt(
                py,
                completion_value::<sys::mln_map_projection>(result)?,
                Vec::new(),
            )
        };
        let discard: unsafe fn(&sys::mln_completion_result) = |result| {
            if !result.value.is_null() && result.value_count == 1 {
                unsafe {
                    generated_dispose_mln_map_projection(
                        result.value.cast::<sys::mln_map_projection>().read(),
                    );
                }
            }
        };
        unsafe {
            call.complete_owned(
                |completion, diagnostic| {
                    sys::mln_map_projection_create(handle, completion, diagnostic)
                },
                convert,
                discard,
            )
        }
    }
    #[pyo3(signature = ())]
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_release", self.admission())?;
        let Some(mut reservation) = self.reserve()? else {
            return completed_python_future(py);
        };
        let handle = reservation.handle();
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| sys::mln_map_release(handle, completion, diagnostic),
                convert,
            )
        }?;
        reservation.commit();
        Ok(future)
    }
    #[pyo3(signature = (selector))]
    fn remove_feature_state(
        &self,
        py: Python<'_>,
        selector: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_remove_feature_state", self.admission())?;
        let storage = &mut call.storage;
        let selector_value =
            generated_input_mln_feature_state_selector(&selector.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_remove_feature_state(handle, &selector_value, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (image_id))]
    fn remove_style_image(
        &self,
        py: Python<'_>,
        image_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_remove_style_image", self.admission())?;
        let storage = &mut call.storage;
        let image_id_value = storage.buffer(image_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_remove_style_image(handle, image_id_value, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (layer_id))]
    fn remove_style_layer(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_remove_style_layer", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_remove_style_layer(handle, layer_id_value, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (source_id))]
    fn remove_style_source(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_remove_style_source", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_remove_style_source(handle, source_id_value, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = ())]
    fn request_repaint(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_request_repaint", self.admission())?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_request_repaint(handle, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = ())]
    fn request_still_image(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_request_still_image", self.admission())?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_map_request_still_image(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (extent))]
    fn resize(&self, py: Python<'_>, extent: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_resize", self.admission())?;
        let storage = &mut call.storage;
        let extent_value = generated_input_mln_logical_extent(&extent.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_resize(handle, extent_value, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (options=None))]
    fn set_bounds(&self, py: Python<'_>, options: Option<Bound<'_, PyAny>>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_bounds", self.admission())?;
        let storage = &mut call.storage;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = generated_input_mln_bound_options(&options.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_bounds(handle, &options_value, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (source_id, tile_id, data))]
    fn set_custom_geometry_source_tile_data(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        tile_id: &Bound<'_, PyAny>,
        data: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_set_custom_geometry_source_tile_data",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let tile_id_value = generated_input_mln_canonical_tile_id(&tile_id.clone(), storage)?;
        let data_value = storage.buffer(data.clone(), false)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_custom_geometry_source_tile_data(
                    handle,
                    source_id_value,
                    tile_id_value,
                    data_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, tile_id, data))]
    fn set_custom_mvt_vector_source_tile_data(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        tile_id: &Bound<'_, PyAny>,
        data: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_set_custom_mvt_vector_source_tile_data",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let tile_id_value = generated_input_mln_canonical_tile_id(&tile_id.clone(), storage)?;
        let data_value = storage.buffer(data.clone(), false)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_custom_mvt_vector_source_tile_data(
                    handle,
                    source_id_value,
                    tile_id_value,
                    data_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, tile_id, message))]
    fn set_custom_mvt_vector_source_tile_error(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        tile_id: &Bound<'_, PyAny>,
        message: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_set_custom_mvt_vector_source_tile_error",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let tile_id_value = generated_input_mln_canonical_tile_id(&tile_id.clone(), storage)?;
        let message_value = storage.buffer(message.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_custom_mvt_vector_source_tile_error(
                    handle,
                    source_id_value,
                    tile_id_value,
                    message_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (options))]
    fn set_debug_options(
        &self,
        py: Python<'_>,
        options: sys::mln_map_debug_option,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_debug_options", self.admission())?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_debug_options(handle, options, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (mask))]
    fn set_event_mask(
        &self,
        py: Python<'_>,
        mask: sys::mln_runtime_event_mask,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_event_mask", self.admission())?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_event_mask(handle, mask, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (selector, input_state))]
    fn set_feature_state(
        &self,
        py: Python<'_>,
        selector: &Bound<'_, PyAny>,
        input_state: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_feature_state", self.admission())?;
        let storage = &mut call.storage;
        let selector_value =
            generated_input_mln_feature_state_selector(&selector.clone(), storage)?;
        let input_state_value = storage.buffer(input_state.clone(), false)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_feature_state(
                    handle,
                    &selector_value,
                    input_state_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (options=None))]
    fn set_free_camera_options(
        &self,
        py: Python<'_>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_free_camera_options", self.admission())?;
        let storage = &mut call.storage;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = generated_input_mln_free_camera_options(&options.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_free_camera_options(handle, &options_value, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (source_id, data))]
    fn set_geojson_source_data(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        data: &GeojsonSourceDataHandle,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_geojson_source_data", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let data_handle = data.input()?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_geojson_source_data(
                    handle,
                    source_id_value,
                    data_handle,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, enabled))]
    fn set_geojson_source_synchronous_tiling(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        enabled: bool,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_set_geojson_source_synchronous_tiling",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_geojson_source_synchronous_tiling(
                    handle,
                    source_id_value,
                    enabled,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, url))]
    fn set_geojson_source_url(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        url: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_geojson_source_url", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let url_value = storage.buffer(url.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_geojson_source_url(
                    handle,
                    source_id_value,
                    url_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (property_name, value))]
    fn set_global_state_property(
        &self,
        py: Python<'_>,
        property_name: &Bound<'_, PyAny>,
        value: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_set_global_state_property", self.admission())?;
        let storage = &mut call.storage;
        let property_name_value = storage.buffer(property_name.clone(), true)?;
        let value_value = storage.buffer(value.clone(), false)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_global_state_property(
                    handle,
                    property_name_value,
                    value_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, coordinates))]
    fn set_image_source_coordinates(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        coordinates: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_set_image_source_coordinates", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let coordinates_values = generated_items(coordinates, |item| {
            generated_input_mln_lat_lng(&item, storage)
        })?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_image_source_coordinates(
                    handle,
                    source_id_value,
                    coordinates_values.as_ptr(),
                    coordinates_values.len(),
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, image=None))]
    fn set_image_source_image(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        image: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_image_source_image", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let image = image.unwrap_or_else(|| py.None().into_bound(py));
        let image_value = generated_input_mln_premultiplied_rgba8_image(&image.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_image_source_image(
                    handle,
                    source_id_value,
                    &image_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, url))]
    fn set_image_source_url(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        url: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_image_source_url", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let url_value = storage.buffer(url.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_image_source_url(
                    handle,
                    source_id_value,
                    url_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (layer_id, filter=None))]
    fn set_layer_filter(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        filter: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_layer_filter", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let filter = filter.unwrap_or_else(|| py.None().into_bound(py));
        let filter_value = generated_maybe(&filter, |filter| storage.buffer(filter, false))?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_layer_filter(
                    handle,
                    layer_id_value,
                    generated_pointer(&filter_value),
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (layer_id, max_zoom))]
    fn set_layer_max_zoom(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        max_zoom: f64,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_layer_max_zoom", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_layer_max_zoom(
                    handle,
                    layer_id_value,
                    max_zoom,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (layer_id, min_zoom))]
    fn set_layer_min_zoom(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        min_zoom: f64,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_layer_min_zoom", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_layer_min_zoom(
                    handle,
                    layer_id_value,
                    min_zoom,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (layer_id, property_name, value))]
    fn set_layer_property(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        property_name: &Bound<'_, PyAny>,
        value: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_layer_property", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let property_name_value = storage.buffer(property_name.clone(), true)?;
        let value_value = storage.buffer(value.clone(), false)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_layer_property(
                    handle,
                    layer_id_value,
                    property_name_value,
                    value_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (layer_id, source_id))]
    fn set_layer_source_id(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        source_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_layer_source_id", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_layer_source_id(
                    handle,
                    layer_id_value,
                    source_id_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (layer_id, source_layer=None))]
    fn set_layer_source_layer(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        source_layer: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_layer_source_layer", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let source_layer = source_layer.unwrap_or_else(|| py.None().into_bound(py));
        let source_layer_value = storage.buffer(source_layer.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_layer_source_layer(
                    handle,
                    layer_id_value,
                    source_layer_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (layer_id, visibility))]
    fn set_layer_visibility(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        visibility: sys::mln_style_layer_visibility,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_layer_visibility", self.admission())?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_layer_visibility(
                    handle,
                    layer_id_value,
                    visibility,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (layer_id, radius))]
    fn set_location_indicator_accuracy_radius(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        radius: f64,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_set_location_indicator_accuracy_radius",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_location_indicator_accuracy_radius(
                    handle,
                    layer_id_value,
                    radius,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (layer_id, bearing))]
    fn set_location_indicator_bearing(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        bearing: f64,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_set_location_indicator_bearing",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_location_indicator_bearing(
                    handle,
                    layer_id_value,
                    bearing,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (layer_id, image_kind, image_id))]
    fn set_location_indicator_image_name(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        image_kind: sys::mln_location_indicator_image_kind,
        image_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_set_location_indicator_image_name",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let image_id_value = storage.buffer(image_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_location_indicator_image_name(
                    handle,
                    layer_id_value,
                    image_kind,
                    image_id_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (layer_id, coordinate, altitude))]
    fn set_location_indicator_location(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        coordinate: &Bound<'_, PyAny>,
        altitude: f64,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_set_location_indicator_location",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let coordinate_value = generated_input_mln_lat_lng(&coordinate.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_location_indicator_location(
                    handle,
                    layer_id_value,
                    coordinate_value,
                    altitude,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (mode=None))]
    fn set_projection_mode(
        &self,
        py: Python<'_>,
        mode: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_projection_mode", self.admission())?;
        let storage = &mut call.storage;
        let mode = mode.unwrap_or_else(|| py.None().into_bound(py));
        let mode_value = generated_input_mln_projection_mode(&mode.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_projection_mode(handle, &mode_value, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (enabled))]
    fn set_rendering_stats_view_enabled(
        &self,
        py: Python<'_>,
        enabled: bool,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_set_rendering_stats_view_enabled",
            self.admission(),
        )?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_rendering_stats_view_enabled(
                    handle, enabled, completion, diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (image_id, image=None, options=None))]
    fn set_style_image(
        &self,
        py: Python<'_>,
        image_id: &Bound<'_, PyAny>,
        image: Option<Bound<'_, PyAny>>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_style_image", self.admission())?;
        let storage = &mut call.storage;
        let image_id_value = storage.buffer(image_id.clone(), true)?;
        let image = image.unwrap_or_else(|| py.None().into_bound(py));
        let image_value = generated_input_mln_premultiplied_rgba8_image(&image.clone(), storage)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = generated_maybe(&options, |options| {
            generated_input_mln_style_image_options(&options, storage)
        })?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_style_image(
                    handle,
                    image_id_value,
                    &image_value,
                    generated_pointer(&options_value),
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (json))]
    fn set_style_json(&self, py: Python<'_>, json: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_style_json", self.admission())?;
        let storage = &mut call.storage;
        let json_value = storage.buffer(json.clone(), false)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_style_json(handle, json_value, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (light_json))]
    fn set_style_light_json(
        &self,
        py: Python<'_>,
        light_json: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_style_light_json", self.admission())?;
        let storage = &mut call.storage;
        let light_json_value = storage.buffer(light_json.clone(), false)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_style_light_json(handle, light_json_value, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (property_name, value))]
    fn set_style_light_property(
        &self,
        py: Python<'_>,
        property_name: &Bound<'_, PyAny>,
        value: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_set_style_light_property", self.admission())?;
        let storage = &mut call.storage;
        let property_name_value = storage.buffer(property_name.clone(), true)?;
        let value_value = storage.buffer(value.clone(), false)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_style_light_property(
                    handle,
                    property_name_value,
                    value_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (source_id, is_volatile))]
    fn set_style_source_volatile(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        is_volatile: bool,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_set_style_source_volatile", self.admission())?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_style_source_volatile(
                    handle,
                    source_id_value,
                    is_volatile,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (options=None))]
    fn set_style_transition_options(
        &self,
        py: Python<'_>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_set_style_transition_options", self.admission())?;
        let storage = &mut call.storage;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value =
            generated_input_mln_style_transition_options(&options.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_style_transition_options(
                    handle,
                    &options_value,
                    completion,
                    diagnostic,
                )
            })
        }
    }
    #[pyo3(signature = (url))]
    fn set_style_url(&self, py: Python<'_>, url: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_style_url", self.admission())?;
        let storage = &mut call.storage;
        let url_value = storage.c_string(url.clone())?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_style_url(handle, url_value, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (options=None))]
    fn set_tile_options(
        &self,
        py: Python<'_>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_tile_options", self.admission())?;
        let storage = &mut call.storage;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = generated_input_mln_map_tile_options(&options.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_tile_options(handle, &options_value, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (options=None))]
    fn set_viewport_options(
        &self,
        py: Python<'_>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_set_viewport_options", self.admission())?;
        let storage = &mut call.storage;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = generated_input_mln_map_viewport_options(&options.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_set_viewport_options(handle, &options_value, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = ())]
    fn snapshot_get(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_snapshot_get", self.admission())?;
        let handle = self.live()?;
        let mut out_snapshot: sys::mln_map_snapshot = unsafe { std::mem::zeroed() };
        out_snapshot.size = std::mem::size_of::<sys::mln_map_snapshot>() as _;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_map_snapshot_get(handle, &mut out_snapshot, diagnostic)
            })
        }?;
        generated_copy_mln_map_snapshot(py, &out_snapshot)
    }
    #[pyo3(signature = ())]
    fn style_url(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_style_url", self.admission())?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_buffer_view>(result)?;
            generated_text(py, value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| sys::mln_map_style_url(handle, completion, diagnostic),
                convert,
            )
        }
    }
    #[pyo3(signature = (update=None))]
    fn update_camera(
        &self,
        py: Python<'_>,
        update: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_update_camera", self.admission())?;
        let storage = &mut call.storage;
        let update = update.unwrap_or_else(|| py.None().into_bound(py));
        let update_value = generated_input_mln_camera_update(&update.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_map_update_camera(handle, &update_value, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (descriptor=None, options=None))]
    fn metal_borrowed_texture_attach(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_metal_borrowed_texture_attach", self.admission())?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_metal_borrowed_texture_descriptor(&descriptor.clone(), storage)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value =
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?;
        let handle = self.live()?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_metal_borrowed_texture_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }?;
        let callback_roots = call.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = unsafe {
            RenderSessionHandle::adopt(py, out_session_owner.take(), callback_roots.clone())
        }?;
        let result = PyDict::new(py);
        result.set_item("session", out_session_python)?;
        result.set_item("completion", future)?;
        Ok(result.into_any().unbind())
    }
    #[pyo3(signature = (descriptor=None, options=None))]
    fn metal_owned_texture_attach(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_metal_owned_texture_attach", self.admission())?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_metal_owned_texture_descriptor(&descriptor.clone(), storage)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value =
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?;
        let handle = self.live()?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_metal_owned_texture_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }?;
        let callback_roots = call.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = unsafe {
            RenderSessionHandle::adopt(py, out_session_owner.take(), callback_roots.clone())
        }?;
        let result = PyDict::new(py);
        result.set_item("session", out_session_python)?;
        result.set_item("completion", future)?;
        Ok(result.into_any().unbind())
    }
    #[pyo3(signature = (descriptor=None, options=None))]
    fn metal_surface_attach(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_metal_surface_attach", self.admission())?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_metal_surface_descriptor(&descriptor.clone(), storage)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value =
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?;
        let handle = self.live()?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_metal_surface_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }?;
        let callback_roots = call.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = unsafe {
            RenderSessionHandle::adopt(py, out_session_owner.take(), callback_roots.clone())
        }?;
        let result = PyDict::new(py);
        result.set_item("session", out_session_python)?;
        result.set_item("completion", future)?;
        Ok(result.into_any().unbind())
    }
    #[pyo3(signature = (descriptor=None, options=None))]
    fn opengl_borrowed_texture_attach(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_opengl_borrowed_texture_attach", self.admission())?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_opengl_borrowed_texture_descriptor(&descriptor.clone(), storage)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value =
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?;
        let handle = self.live()?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_opengl_borrowed_texture_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }?;
        let callback_roots = call.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = unsafe {
            RenderSessionHandle::adopt(py, out_session_owner.take(), callback_roots.clone())
        }?;
        let result = PyDict::new(py);
        result.set_item("session", out_session_python)?;
        result.set_item("completion", future)?;
        Ok(result.into_any().unbind())
    }
    #[pyo3(signature = (descriptor=None, options=None))]
    fn opengl_owned_texture_attach(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_opengl_owned_texture_attach", self.admission())?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_opengl_owned_texture_descriptor(&descriptor.clone(), storage)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value =
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?;
        let handle = self.live()?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_opengl_owned_texture_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }?;
        let callback_roots = call.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = unsafe {
            RenderSessionHandle::adopt(py, out_session_owner.take(), callback_roots.clone())
        }?;
        let result = PyDict::new(py);
        result.set_item("session", out_session_python)?;
        result.set_item("completion", future)?;
        Ok(result.into_any().unbind())
    }
    #[pyo3(signature = (descriptor=None, options=None))]
    fn opengl_surface_attach(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_opengl_surface_attach", self.admission())?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_opengl_surface_descriptor(&descriptor.clone(), storage)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value =
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?;
        let handle = self.live()?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_opengl_surface_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }?;
        let callback_roots = call.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = unsafe {
            RenderSessionHandle::adopt(py, out_session_owner.take(), callback_roots.clone())
        }?;
        let result = PyDict::new(py);
        result.set_item("session", out_session_python)?;
        result.set_item("completion", future)?;
        Ok(result.into_any().unbind())
    }
    #[pyo3(signature = (descriptor=None, options=None))]
    fn vulkan_borrowed_texture_attach(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_vulkan_borrowed_texture_attach", self.admission())?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_vulkan_borrowed_texture_descriptor(&descriptor.clone(), storage)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value =
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?;
        let handle = self.live()?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_vulkan_borrowed_texture_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }?;
        let callback_roots = call.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = unsafe {
            RenderSessionHandle::adopt(py, out_session_owner.take(), callback_roots.clone())
        }?;
        let result = PyDict::new(py);
        result.set_item("session", out_session_python)?;
        result.set_item("completion", future)?;
        Ok(result.into_any().unbind())
    }
    #[pyo3(signature = (descriptor=None, options=None))]
    fn vulkan_owned_texture_attach(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_vulkan_owned_texture_attach", self.admission())?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_vulkan_owned_texture_descriptor(&descriptor.clone(), storage)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value =
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?;
        let handle = self.live()?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_vulkan_owned_texture_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }?;
        let callback_roots = call.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = unsafe {
            RenderSessionHandle::adopt(py, out_session_owner.take(), callback_roots.clone())
        }?;
        let result = PyDict::new(py);
        result.set_item("session", out_session_python)?;
        result.set_item("completion", future)?;
        Ok(result.into_any().unbind())
    }
    #[pyo3(signature = (descriptor=None, options=None))]
    fn vulkan_surface_attach(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_vulkan_surface_attach", self.admission())?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_vulkan_surface_descriptor(&descriptor.clone(), storage)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value =
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?;
        let handle = self.live()?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_vulkan_surface_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }?;
        let callback_roots = call.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = unsafe {
            RenderSessionHandle::adopt(py, out_session_owner.take(), callback_roots.clone())
        }?;
        let result = PyDict::new(py);
        result.set_item("session", out_session_python)?;
        result.set_item("completion", future)?;
        Ok(result.into_any().unbind())
    }
    #[pyo3(signature = (descriptor=None, options=None))]
    fn webgpu_borrowed_texture_attach(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_webgpu_borrowed_texture_attach", self.admission())?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_webgpu_borrowed_texture_descriptor(&descriptor.clone(), storage)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value =
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?;
        let handle = self.live()?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_webgpu_borrowed_texture_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }?;
        let callback_roots = call.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = unsafe {
            RenderSessionHandle::adopt(py, out_session_owner.take(), callback_roots.clone())
        }?;
        let result = PyDict::new(py);
        result.set_item("session", out_session_python)?;
        result.set_item("completion", future)?;
        Ok(result.into_any().unbind())
    }
    #[pyo3(signature = (descriptor=None, options=None))]
    fn webgpu_owned_texture_attach(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_webgpu_owned_texture_attach", self.admission())?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_webgpu_owned_texture_descriptor(&descriptor.clone(), storage)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value =
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?;
        let handle = self.live()?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_webgpu_owned_texture_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }?;
        let callback_roots = call.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = unsafe {
            RenderSessionHandle::adopt(py, out_session_owner.take(), callback_roots.clone())
        }?;
        let result = PyDict::new(py);
        result.set_item("session", out_session_python)?;
        result.set_item("completion", future)?;
        Ok(result.into_any().unbind())
    }
    #[pyo3(signature = (descriptor=None, options=None))]
    fn webgpu_surface_attach(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_webgpu_surface_attach", self.admission())?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_webgpu_surface_descriptor(&descriptor.clone(), storage)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value =
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?;
        let handle = self.live()?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_webgpu_surface_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }?;
        let callback_roots = call.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = unsafe {
            RenderSessionHandle::adopt(py, out_session_owner.take(), callback_roots.clone())
        }?;
        let result = PyDict::new(py);
        result.set_item("session", out_session_python)?;
        result.set_item("completion", future)?;
        Ok(result.into_any().unbind())
    }
}

#[pymethods]
impl MapProjectionHandle {
    #[pyo3(signature = ())]
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_projection_close", self.admission())?;
        let Some(mut reservation) = self.reserve()? else {
            return Ok(py.None());
        };
        let handle = reservation.handle();
        unsafe { call.status(|diagnostic| sys::mln_map_projection_close(handle, diagnostic)) }?;
        reservation.commit();
        Ok(py.None())
    }
    #[pyo3(signature = ())]
    fn get_camera(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_projection_get_camera", self.admission())?;
        let handle = self.live()?;
        let mut out_camera: sys::mln_camera_options = unsafe { sys::mln_camera_options_default() };
        out_camera.size = std::mem::size_of::<sys::mln_camera_options>() as _;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_map_projection_get_camera(handle, &mut out_camera, diagnostic)
            })
        }?;
        generated_copy_mln_camera_options(py, &out_camera)
    }
    #[pyo3(signature = (point))]
    fn lat_lng_for_pixel(&self, py: Python<'_>, point: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_projection_lat_lng_for_pixel", self.admission())?;
        let storage = &mut call.storage;
        let point_value = generated_input_mln_screen_point(&point.clone(), storage)?;
        let handle = self.live()?;
        let mut out_coordinate: sys::mln_lat_lng = unsafe { std::mem::zeroed() };
        unsafe {
            call.status(|diagnostic| {
                sys::mln_map_projection_lat_lng_for_pixel(
                    handle,
                    point_value,
                    &mut out_coordinate,
                    diagnostic,
                )
            })
        }?;
        generated_copy_mln_lat_lng(py, &out_coordinate)
    }
    #[pyo3(signature = (point))]
    fn lat_lng_for_pixel_unwrapped(
        &self,
        py: Python<'_>,
        point: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_projection_lat_lng_for_pixel_unwrapped",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let point_value = generated_input_mln_screen_point(&point.clone(), storage)?;
        let handle = self.live()?;
        let mut out_coordinate: sys::mln_lat_lng = unsafe { std::mem::zeroed() };
        unsafe {
            call.status(|diagnostic| {
                sys::mln_map_projection_lat_lng_for_pixel_unwrapped(
                    handle,
                    point_value,
                    &mut out_coordinate,
                    diagnostic,
                )
            })
        }?;
        generated_copy_mln_lat_lng(py, &out_coordinate)
    }
    #[pyo3(signature = (latitude))]
    fn meters_per_pixel_at_latitude(&self, py: Python<'_>, latitude: f64) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_projection_meters_per_pixel_at_latitude",
            self.admission(),
        )?;
        let handle = self.live()?;
        let mut out_meters_per_pixel: f64 = unsafe { std::mem::zeroed() };
        unsafe {
            call.status(|diagnostic| {
                sys::mln_map_projection_meters_per_pixel_at_latitude(
                    handle,
                    latitude,
                    &mut out_meters_per_pixel,
                    diagnostic,
                )
            })
        }?;
        generated_value(py, out_meters_per_pixel)
    }
    #[pyo3(signature = (coordinate))]
    fn pixel_for_lat_lng(
        &self,
        py: Python<'_>,
        coordinate: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_map_projection_pixel_for_lat_lng", self.admission())?;
        let storage = &mut call.storage;
        let coordinate_value = generated_input_mln_lat_lng(&coordinate.clone(), storage)?;
        let handle = self.live()?;
        let mut out_point: sys::mln_screen_point = unsafe { std::mem::zeroed() };
        unsafe {
            call.status(|diagnostic| {
                sys::mln_map_projection_pixel_for_lat_lng(
                    handle,
                    coordinate_value,
                    &mut out_point,
                    diagnostic,
                )
            })
        }?;
        generated_copy_mln_screen_point(py, &out_point)
    }
    #[pyo3(signature = (camera=None))]
    fn set_camera(&self, py: Python<'_>, camera: Option<Bound<'_, PyAny>>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_projection_set_camera", self.admission())?;
        let storage = &mut call.storage;
        let camera = camera.unwrap_or_else(|| py.None().into_bound(py));
        let camera_value = generated_input_mln_camera_options(&camera.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_map_projection_set_camera(handle, &camera_value, diagnostic)
            })
        }?;
        Ok(py.None())
    }
    #[pyo3(signature = (coordinates, padding))]
    fn set_visible_coordinates(
        &self,
        py: Python<'_>,
        coordinates: &Bound<'_, PyAny>,
        padding: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_projection_set_visible_coordinates",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let coordinates_values = generated_items(coordinates, |item| {
            generated_input_mln_lat_lng(&item, storage)
        })?;
        let padding_value = generated_input_mln_edge_insets(&padding.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_map_projection_set_visible_coordinates(
                    handle,
                    coordinates_values.as_ptr(),
                    coordinates_values.len(),
                    padding_value,
                    diagnostic,
                )
            })
        }?;
        Ok(py.None())
    }
    #[pyo3(signature = (geometry, padding))]
    fn set_visible_geometry(
        &self,
        py: Python<'_>,
        geometry: &Bound<'_, PyAny>,
        padding: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_map_projection_set_visible_geometry",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let geometry_value = storage.buffer(geometry.clone(), false)?;
        let padding_value = generated_input_mln_edge_insets(&padding.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_map_projection_set_visible_geometry(
                    handle,
                    geometry_value,
                    padding_value,
                    diagnostic,
                )
            })
        }?;
        Ok(py.None())
    }
}

#[pymethods]
impl RenderFrameBatchHandle {
    #[pyo3(signature = ())]
    fn count(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_render_frame_batch_count", self.admission())?;
        let handle = self.live()?;
        let mut out_count: usize = unsafe { std::mem::zeroed() };
        unsafe {
            call.status(|diagnostic| {
                sys::mln_render_frame_batch_count(handle, &mut out_count, diagnostic)
            })
        }?;
        generated_value(py, out_count)
    }
    #[pyo3(signature = (index))]
    fn get(&self, py: Python<'_>, index: usize) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_render_frame_batch_get", self.admission())?;
        let handle = self.live()?;
        let mut out_result: sys::mln_render_frame_result = unsafe { std::mem::zeroed() };
        out_result.size = std::mem::size_of::<sys::mln_render_frame_result>() as _;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_render_frame_batch_get(handle, index, &mut out_result, diagnostic)
            })
        }?;
        generated_copy_mln_render_frame_result(py, &out_result)
    }
    #[pyo3(signature = ())]
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_render_frame_batch_release", self.admission())?;
        let Some(mut reservation) = self.reserve()? else {
            return Ok(py.None());
        };
        let handle = reservation.handle();
        unsafe { call.run(|| sys::mln_render_frame_batch_release(handle)) };
        reservation.commit();
        Ok(py.None())
    }
}

#[pymethods]
impl RenderSessionHandle {
    #[pyo3(signature = (descriptor=None))]
    fn metal_borrowed_texture_set_target(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_metal_borrowed_texture_set_target",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_metal_borrowed_texture_descriptor(&descriptor.clone(), storage)?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_metal_borrowed_texture_set_target(
                        handle,
                        &descriptor_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (descriptor=None))]
    fn metal_surface_set_target(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_metal_surface_set_target", self.admission())?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_metal_surface_descriptor(&descriptor.clone(), storage)?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_metal_surface_set_target(
                        handle,
                        &descriptor_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (descriptor=None))]
    fn opengl_borrowed_texture_set_target(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_opengl_borrowed_texture_set_target",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_opengl_borrowed_texture_descriptor(&descriptor.clone(), storage)?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_opengl_borrowed_texture_set_target(
                        handle,
                        &descriptor_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (descriptor=None))]
    fn opengl_surface_set_target(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_opengl_surface_set_target", self.admission())?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_opengl_surface_descriptor(&descriptor.clone(), storage)?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_opengl_surface_set_target(
                        handle,
                        &descriptor_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn abandon(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_render_session_abandon", self.admission())?;
        let Some(reservation) = self.reserve()? else {
            return Ok(py.None());
        };
        let handle = reservation.handle();
        let mut out_result: sys::mln_render_abandon_result = unsafe { std::mem::zeroed() };
        out_result.size = std::mem::size_of::<sys::mln_render_abandon_result>() as _;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_render_session_abandon(handle, &mut out_result, diagnostic)
            })
        }?;
        self.state().views_valid = false;
        generated_copy_mln_render_abandon_result(py, &out_result)
    }
    #[pyo3(signature = ())]
    fn acquire_frame(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_render_session_acquire_frame", self.admission())?;
        let handle = self.live()?;
        let mut out_frame: sys::mln_acquired_frame = unsafe { std::mem::zeroed() };
        unsafe {
            call.status(|diagnostic| {
                sys::mln_render_session_acquire_frame(handle, &mut out_frame, diagnostic)
            })
        }?;
        unsafe { AcquiredFrameHandle::adopt(py, out_frame, Vec::new()) }
    }
    #[pyo3(signature = ())]
    fn barrier(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_render_session_barrier", self.admission())?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_render_session_barrier(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn clear_data(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_render_session_clear_data", self.admission())?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_render_session_clear_data(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_render_session_destroy", self.admission())?;
        let Some(mut reservation) = self.reserve()? else {
            return Ok(py.None());
        };
        let handle = reservation.handle();
        unsafe { call.status(|diagnostic| sys::mln_render_session_destroy(handle, diagnostic)) }?;
        reservation.commit();
        Ok(py.None())
    }
    #[pyo3(signature = ())]
    fn detach(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_render_session_detach", self.admission())?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_render_session_detach(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn drain_frame_results(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_render_session_drain_frame_results",
            self.admission(),
        )?;
        let handle = self.live()?;
        let mut out_batch: sys::mln_render_frame_batch = unsafe { std::mem::zeroed() };
        unsafe {
            call.status(|diagnostic| {
                sys::mln_render_session_drain_frame_results(handle, &mut out_batch, diagnostic)
            })
        }?;
        unsafe { RenderFrameBatchHandle::adopt(py, out_batch, Vec::new()) }
    }
    #[pyo3(signature = ())]
    fn dump_debug_logs(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_render_session_dump_debug_logs", self.admission())?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_render_session_dump_debug_logs(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn get_capabilities(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_render_session_get_capabilities", self.admission())?;
        let handle = self.live()?;
        let mut out_capabilities: sys::mln_render_session_capabilities =
            unsafe { std::mem::zeroed() };
        out_capabilities.size = std::mem::size_of::<sys::mln_render_session_capabilities>() as _;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_render_session_get_capabilities(handle, &mut out_capabilities, diagnostic)
            })
        }?;
        generated_copy_mln_render_session_capabilities(py, &out_capabilities)
    }
    #[pyo3(signature = ())]
    fn get_snapshot(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_render_session_get_snapshot", self.admission())?;
        let handle = self.live()?;
        let mut out_snapshot: sys::mln_render_session_snapshot = unsafe { std::mem::zeroed() };
        out_snapshot.size = std::mem::size_of::<sys::mln_render_session_snapshot>() as _;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_render_session_get_snapshot(handle, &mut out_snapshot, diagnostic)
            })
        }?;
        generated_copy_mln_render_session_snapshot(py, &out_snapshot)
    }
    #[pyo3(signature = ())]
    fn projection_create(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_render_session_projection_create", self.admission())?;
        let handle = self.live()?;
        let mut out_projection: sys::mln_map_projection = unsafe { std::mem::zeroed() };
        unsafe {
            call.status(|diagnostic| {
                sys::mln_render_session_projection_create(handle, &mut out_projection, diagnostic)
            })
        }?;
        unsafe { MapProjectionHandle::adopt(py, out_projection, Vec::new()) }
    }
    #[pyo3(signature = (source_id, feature, extension, extension_field, arguments=None))]
    fn query_feature_extensions(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        feature: &Bound<'_, PyAny>,
        extension: &Bound<'_, PyAny>,
        extension_field: &Bound<'_, PyAny>,
        arguments: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_render_session_query_feature_extensions",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let feature_value = storage.buffer(feature.clone(), false)?;
        let extension_value = storage.buffer(extension.clone(), true)?;
        let extension_field_value = storage.buffer(extension_field.clone(), true)?;
        let arguments = arguments.unwrap_or_else(|| py.None().into_bound(py));
        let arguments_value =
            generated_maybe(&arguments, |arguments| storage.buffer(arguments, false))?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_buffer_view>(result)?;
            unsafe { generated_bytes(py, value) }
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_render_session_query_feature_extensions(
                        handle,
                        source_id_value,
                        feature_value,
                        extension_value,
                        extension_field_value,
                        generated_pointer(&arguments_value),
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (geometry, options=None))]
    fn query_rendered_features(
        &self,
        py: Python<'_>,
        geometry: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_render_session_query_rendered_features",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let geometry_value =
            generated_input_mln_rendered_query_geometry(&geometry.clone(), storage)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = generated_maybe(&options, |options| {
            generated_input_mln_rendered_feature_query_options(&options, storage)
        })?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let list = PyList::empty(py);
            for value in generated_completion_slice::<sys::mln_queried_feature>(result)? {
                list.append(generated_copy_mln_queried_feature(py, value)?)?;
            }
            Ok(list.into_any().unbind())
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_render_session_query_rendered_features(
                        handle,
                        &geometry_value,
                        generated_pointer(&options_value),
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (source_id, options=None))]
    fn query_source_features(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_render_session_query_source_features",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = generated_maybe(&options, |options| {
            generated_input_mln_source_feature_query_options(&options, storage)
        })?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let list = PyList::empty(py);
            for value in generated_completion_slice::<sys::mln_queried_feature>(result)? {
                list.append(generated_copy_mln_queried_feature(py, value)?)?;
            }
            Ok(list.into_any().unbind())
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_render_session_query_source_features(
                        handle,
                        source_id_value,
                        generated_pointer(&options_value),
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn reduce_memory_use(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_render_session_reduce_memory_use", self.admission())?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_render_session_reduce_memory_use(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (demand=None))]
    fn request_frame(
        &self,
        py: Python<'_>,
        demand: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_render_session_request_frame", self.admission())?;
        let storage = &mut call.storage;
        let demand = demand.unwrap_or_else(|| py.None().into_bound(py));
        let demand_value = generated_input_mln_frame_demand(&demand.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_render_session_request_frame(handle, &demand_value, diagnostic)
            })
        }?;
        Ok(py.None())
    }
    #[pyo3(signature = (extent))]
    fn resize(&self, py: Python<'_>, extent: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_render_session_resize", self.admission())?;
        let storage = &mut call.storage;
        let extent_value = generated_input_mln_render_target_extent(&extent.clone(), storage)?;
        let handle = self.live()?;
        unsafe {
            call.command(|completion, diagnostic| {
                sys::mln_render_session_resize(handle, &extent_value, completion, diagnostic)
            })
        }
    }
    #[pyo3(signature = (max_work))]
    fn service_driver_work(&self, py: Python<'_>, max_work: usize) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_render_session_service_driver_work",
            self.admission(),
        )?;
        let handle = self.live()?;
        let mut out_serviced: usize = unsafe { std::mem::zeroed() };
        unsafe {
            call.status(|diagnostic| {
                sys::mln_render_session_service_driver_work(
                    handle,
                    max_work,
                    &mut out_serviced,
                    diagnostic,
                )
            })
        }?;
        generated_value(py, out_serviced)
    }
    #[pyo3(signature = ())]
    fn texture_read_premultiplied_rgba8(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_texture_read_premultiplied_rgba8", self.admission())?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_texture_readback_result>(result)?;
            generated_copy_mln_texture_readback_result(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_texture_read_premultiplied_rgba8(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (descriptor=None))]
    fn vulkan_borrowed_texture_set_target(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_vulkan_borrowed_texture_set_target",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_vulkan_borrowed_texture_descriptor(&descriptor.clone(), storage)?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_vulkan_borrowed_texture_set_target(
                        handle,
                        &descriptor_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (descriptor=None))]
    fn vulkan_surface_set_target(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_vulkan_surface_set_target", self.admission())?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_vulkan_surface_descriptor(&descriptor.clone(), storage)?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_vulkan_surface_set_target(
                        handle,
                        &descriptor_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (descriptor=None))]
    fn webgpu_borrowed_texture_set_target(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_webgpu_borrowed_texture_set_target",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_webgpu_borrowed_texture_descriptor(&descriptor.clone(), storage)?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_webgpu_borrowed_texture_set_target(
                        handle,
                        &descriptor_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (descriptor=None))]
    fn webgpu_surface_set_target(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_webgpu_surface_set_target", self.admission())?;
        let storage = &mut call.storage;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value =
            generated_input_mln_webgpu_surface_descriptor(&descriptor.clone(), storage)?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_webgpu_surface_set_target(
                        handle,
                        &descriptor_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
}

#[pymethods]
impl ResourceRequestHandle {
    #[pyo3(signature = ())]
    fn cancelled(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_resource_request_cancelled",
            maplibre_core::handle::NativeHandle::to_raw(self.state.issued_handle()),
        )?;
        let handle = self.state.native_for_call().map_err(map_error)?;
        let mut out_cancelled: bool = unsafe { std::mem::zeroed() };
        unsafe {
            call.status(|diagnostic| {
                sys::mln_resource_request_cancelled(handle, &mut out_cancelled, diagnostic)
            })
        }?;
        generated_value(py, out_cancelled)
    }
    #[pyo3(signature = (response))]
    fn complete(&self, py: Python<'_>, response: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_resource_request_complete",
            maplibre_core::handle::NativeHandle::to_raw(self.state.issued_handle()),
        )?;
        let storage = &mut call.storage;
        let response_value = generated_input_mln_resource_response(&response.clone(), storage)?;
        unsafe {
            call.run(|| {
                self.state.complete_with(|handle| {
                    maplibre_core::check(|diagnostic| {
                        sys::mln_resource_request_complete(handle, &response_value, diagnostic)
                    })
                })
            })
        }
        .map_err(map_error)?;
        Ok(py.None())
    }
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_resource_request_release",
            maplibre_core::handle::NativeHandle::to_raw(self.state.issued_handle()),
        )?;
        unsafe { call.run(|| self.state.close()) };
        Ok(py.None())
    }
    fn set_cancel_callback(&self, py: Python<'_>, callback: Py<PyAny>) -> PyResult<bool> {
        let callback_owner =
            maplibre_core::handle::NativeHandle::to_raw(self.state.issued_handle());
        generated_check_operation("mln_resource_request_set_cancel_callback", callback_owner)?;
        if !callback.bind(py).is_callable() {
            return Err(invalid_argument_error("callback must be callable"));
        }
        let root = GeneratedCallbackRootOwner::new(vec![callback]);
        let weak = root.downgrade();
        let cancelled = self
            .state
            .register_cancel(Box::new(move || {
                Python::try_attach(|py| {
                    if let Some(callback) = root.get(py, 0)
                        && let Err(error) = callback.bind(py).call0()
                    {
                        generated_report_unraisable(py, error);
                    }
                });
            }))
            .map_err(map_error)?;
        if !cancelled {
            *self.cancel_root.lock().unwrap_or_else(|p| p.into_inner()) = weak;
        }
        Ok(cancelled)
    }
    #[pyo3(signature = ())]
    fn wait_until_retired(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_resource_request_wait_until_retired",
            maplibre_core::handle::NativeHandle::to_raw(self.state.issued_handle()),
        )?;
        let handle = self.state.issued_handle();
        unsafe {
            call.status(|diagnostic| {
                sys::mln_resource_request_wait_until_retired(handle, diagnostic)
            })
        }?;
        Ok(py.None())
    }
}

#[pymethods]
impl ResourceTransformResponseScope {
    #[pyo3(signature = (url))]
    fn set_url(&self, py: Python<'_>, url: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_resource_transform_response_set_url",
            self.native as u64,
        )?;
        let storage = &mut call.storage;
        let url_view = storage.buffer(url.clone(), true)?;
        let handle = self.scope.pointer(self.native)? as *mut sys::mln_resource_transform_response;
        unsafe {
            call.status(|diagnostic| {
                sys::mln_resource_transform_response_set_url(
                    handle,
                    url_view.data.cast(),
                    url_view.size,
                    diagnostic,
                )
            })
        }?;
        Ok(py.None())
    }
}

#[pymethods]
impl RuntimeHandle {
    #[pyo3(signature = (options=None))]
    fn map_create(&self, py: Python<'_>, options: Option<Bound<'_, PyAny>>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_map_create", self.admission())?;
        let storage = &mut call.storage;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = generated_input_mln_map_options(&options.clone(), storage)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| unsafe {
            MapHandle::adopt(py, completion_value::<sys::mln_map>(result)?, Vec::new())
        };
        let discard: unsafe fn(&sys::mln_completion_result) = |result| {
            if !result.value.is_null() && result.value_count == 1 {
                unsafe {
                    generated_dispose_mln_map(result.value.cast::<sys::mln_map>().read());
                }
            }
        };
        unsafe {
            call.complete_owned(
                |completion, diagnostic| {
                    sys::mln_map_create(handle, &options_value, completion, diagnostic)
                },
                convert,
                discard,
            )
        }
    }
    #[pyo3(signature = ())]
    fn barrier(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_runtime_barrier", self.admission())?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| sys::mln_runtime_barrier(handle, completion, diagnostic),
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn clear_http_header_transform(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_runtime_clear_http_header_transform",
            self.admission(),
        )?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_clear_http_header_transform(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn clear_resource_provider(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_runtime_clear_resource_provider", self.admission())?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_clear_resource_provider(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn clear_resource_transform(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_runtime_clear_resource_transform", self.admission())?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_clear_resource_transform(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn drain_events(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_runtime_drain_events", self.admission())?;
        let handle = self.live()?;
        let mut out_batch: sys::mln_event_batch = unsafe { std::mem::zeroed() };
        unsafe {
            call.status(|diagnostic| {
                sys::mln_runtime_drain_events(handle, &mut out_batch, diagnostic)
            })
        }?;
        unsafe { EventBatchHandle::adopt(py, out_batch, Vec::new()) }
    }
    #[pyo3(signature = ())]
    fn get_event_mask(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_runtime_get_event_mask", self.admission())?;
        let handle = self.live()?;
        let mut out_mask: sys::mln_runtime_event_mask = unsafe { std::mem::zeroed() };
        unsafe {
            call.status(|diagnostic| {
                sys::mln_runtime_get_event_mask(handle, &mut out_mask, diagnostic)
            })
        }?;
        generated_value(py, out_mask)
    }
    #[pyo3(signature = (definition, metadata))]
    fn offline_region_create(
        &self,
        py: Python<'_>,
        definition: &Bound<'_, PyAny>,
        metadata: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_runtime_offline_region_create", self.admission())?;
        let storage = &mut call.storage;
        let definition_value =
            generated_input_mln_offline_region_definition(&definition.clone(), storage)?;
        let metadata_view = storage.buffer(metadata.clone(), false)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_offline_region_info>(result)?;
            generated_copy_mln_offline_region_info(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_offline_region_create(
                        handle,
                        &definition_value,
                        metadata_view.data.cast(),
                        metadata_view.size,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (region_id))]
    fn offline_region_delete(&self, py: Python<'_>, region_id: i64) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_runtime_offline_region_delete", self.admission())?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_offline_region_delete(
                        handle, region_id, completion, diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (region_id))]
    fn offline_region_get(&self, py: Python<'_>, region_id: i64) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_runtime_offline_region_get", self.admission())?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            if result.value.is_null() {
                return Ok(py.None());
            }
            let value = completion_value::<sys::mln_offline_region_info>(result)?;
            generated_copy_mln_offline_region_info(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_offline_region_get(handle, region_id, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (region_id))]
    fn offline_region_get_status(&self, py: Python<'_>, region_id: i64) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_runtime_offline_region_get_status",
            self.admission(),
        )?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_offline_region_status>(result)?;
            generated_copy_mln_offline_region_status(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_offline_region_get_status(
                        handle, region_id, completion, diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (region_id))]
    fn offline_region_invalidate(&self, py: Python<'_>, region_id: i64) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_runtime_offline_region_invalidate",
            self.admission(),
        )?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_offline_region_invalidate(
                        handle, region_id, completion, diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (region_id, input_state))]
    fn offline_region_set_download_state(
        &self,
        py: Python<'_>,
        region_id: i64,
        input_state: sys::mln_offline_region_download_state,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_runtime_offline_region_set_download_state",
            self.admission(),
        )?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_offline_region_set_download_state(
                        handle,
                        region_id,
                        input_state,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (region_id, observed))]
    fn offline_region_set_observed(
        &self,
        py: Python<'_>,
        region_id: i64,
        observed: bool,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_runtime_offline_region_set_observed",
            self.admission(),
        )?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_offline_region_set_observed(
                        handle, region_id, observed, completion, diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (region_id, metadata))]
    fn offline_region_update_metadata(
        &self,
        py: Python<'_>,
        region_id: i64,
        metadata: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_runtime_offline_region_update_metadata",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let metadata_view = storage.buffer(metadata.clone(), false)?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let value = completion_value::<sys::mln_offline_region_info>(result)?;
            generated_copy_mln_offline_region_info(py, &value)
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_offline_region_update_metadata(
                        handle,
                        region_id,
                        metadata_view.data.cast(),
                        metadata_view.size,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn offline_regions_list(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_runtime_offline_regions_list", self.admission())?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let list = PyList::empty(py);
            for value in generated_completion_slice::<sys::mln_offline_region_info>(result)? {
                list.append(generated_copy_mln_offline_region_info(py, value)?)?;
            }
            Ok(list.into_any().unbind())
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_offline_regions_list(handle, completion, diagnostic)
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (side_database_path))]
    fn offline_regions_merge_database(
        &self,
        py: Python<'_>,
        side_database_path: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_runtime_offline_regions_merge_database",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let side_database_path_value = storage.c_string(side_database_path.clone())?;
        let handle = self.live()?;
        let convert = |py: Python<'_>, result: &sys::mln_completion_result| {
            let list = PyList::empty(py);
            for value in generated_completion_slice::<sys::mln_offline_region_info>(result)? {
                list.append(generated_copy_mln_offline_region_info(py, value)?)?;
            }
            Ok(list.into_any().unbind())
        };
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_offline_regions_merge_database(
                        handle,
                        side_database_path_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = ())]
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_runtime_release", self.admission())?;
        let Some(mut reservation) = self.reserve()? else {
            return completed_python_future(py);
        };
        let handle = reservation.handle();
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| sys::mln_runtime_release(handle, completion, diagnostic),
                convert,
            )
        }?;
        reservation.commit();
        Ok(future)
    }
    #[pyo3(signature = (operation))]
    fn run_ambient_cache_operation(
        &self,
        py: Python<'_>,
        operation: sys::mln_ambient_cache_operation,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_runtime_run_ambient_cache_operation",
            self.admission(),
        )?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_run_ambient_cache_operation(
                        handle, operation, completion, diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (mask))]
    fn set_event_mask(
        &self,
        py: Python<'_>,
        mask: sys::mln_runtime_event_mask,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(py, "mln_runtime_set_event_mask", self.admission())?;
        let handle = self.live()?;
        unsafe {
            call.status(|diagnostic| sys::mln_runtime_set_event_mask(handle, mask, diagnostic))
        }?;
        Ok(py.None())
    }
    #[pyo3(signature = (transform))]
    fn set_http_header_transform(
        &self,
        py: Python<'_>,
        transform: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_runtime_set_http_header_transform",
            self.admission(),
        )?;
        let storage = &mut call.storage;
        let transform_value =
            generated_input_mln_http_header_transform(&transform.clone(), storage)?;
        let handle = self.live()?;
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_set_http_header_transform(
                        handle,
                        &transform_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }?;
        let callback_roots = call.accept_callbacks();
        self.state().retain_callback_roots(callback_roots);
        Ok(future)
    }
    #[pyo3(signature = (size))]
    fn set_maximum_ambient_cache_size(&self, py: Python<'_>, size: u64) -> PyResult<Py<PyAny>> {
        let mut call = GeneratedCall::new(
            py,
            "mln_runtime_set_maximum_ambient_cache_size",
            self.admission(),
        )?;
        let handle = self.live()?;
        let convert = py_none;
        unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_set_maximum_ambient_cache_size(
                        handle, size, completion, diagnostic,
                    )
                },
                convert,
            )
        }
    }
    #[pyo3(signature = (provider))]
    fn set_resource_provider(
        &self,
        py: Python<'_>,
        provider: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_runtime_set_resource_provider", self.admission())?;
        let storage = &mut call.storage;
        let provider_value = generated_input_mln_resource_provider(&provider.clone(), storage)?;
        let handle = self.live()?;
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_set_resource_provider(
                        handle,
                        &provider_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }?;
        let callback_roots = call.accept_callbacks();
        self.state().retain_callback_roots(callback_roots);
        Ok(future)
    }
    #[pyo3(signature = (transform))]
    fn set_resource_transform(
        &self,
        py: Python<'_>,
        transform: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let mut call =
            GeneratedCall::new(py, "mln_runtime_set_resource_transform", self.admission())?;
        let storage = &mut call.storage;
        let transform_value = generated_input_mln_resource_transform(&transform.clone(), storage)?;
        let handle = self.live()?;
        let convert = py_none;
        let future = unsafe {
            call.complete(
                |completion, diagnostic| {
                    sys::mln_runtime_set_resource_transform(
                        handle,
                        &transform_value,
                        completion,
                        diagnostic,
                    )
                },
                convert,
            )
        }?;
        let callback_roots = call.accept_callbacks();
        self.state().retain_callback_roots(callback_roots);
        Ok(future)
    }
}
generated_owner!(
    AcquiredFrameHandle,
    "_AcquiredFrameHandle",
    mln_acquired_frame,
    Some(generated_dispose_mln_acquired_frame),
    None,
    |py, owner| GeneratedReadScope::with_native::<sys::mln_acquired_frame, _>(
        py,
        Arc::clone(&owner.state),
        sys::mln_adapter_acquired_frame_view_begin,
        sys::mln_adapter_acquired_frame_view_end
    )
);
generated_owner!(
    BufferHandle,
    "_BufferHandle",
    mln_buffer,
    Some(generated_dispose_mln_buffer),
    None,
    |_, owner| GeneratedReadScope::new::<sys::mln_buffer, _>(Arc::clone(&owner.state))
);
generated_owner!(
    EventBatchHandle,
    "_EventBatchHandle",
    mln_event_batch,
    Some(generated_dispose_mln_event_batch),
    None,
    |_, owner| GeneratedReadScope::new::<sys::mln_event_batch, _>(Arc::clone(&owner.state))
);
generated_owner!(
    GeojsonSourceDataHandle,
    "_GeojsonSourceDataHandle",
    mln_geojson_source_data,
    Some(generated_dispose_mln_geojson_source_data),
    None,
    |_, owner| GeneratedReadScope::new::<sys::mln_geojson_source_data, _>(Arc::clone(&owner.state))
);
generated_owner!(
    MapHandle,
    "_MapHandle",
    mln_map,
    Some(generated_dispose_mln_map),
    None,
    |_, owner| GeneratedReadScope::new::<sys::mln_map, _>(Arc::clone(&owner.state))
);
generated_owner!(
    MapProjectionHandle,
    "_MapProjectionHandle",
    mln_map_projection,
    Some(generated_dispose_mln_map_projection),
    None,
    |_, owner| GeneratedReadScope::new::<sys::mln_map_projection, _>(Arc::clone(&owner.state))
);
generated_owner!(
    RenderFrameBatchHandle,
    "_RenderFrameBatchHandle",
    mln_render_frame_batch,
    Some(generated_dispose_mln_render_frame_batch),
    None,
    |_, owner| GeneratedReadScope::new::<sys::mln_render_frame_batch, _>(Arc::clone(&owner.state))
);
generated_owner!(
    RenderSessionHandle,
    "_RenderSessionHandle",
    mln_render_session,
    Some(generated_dispose_mln_render_session),
    None,
    |_, owner| GeneratedReadScope::new::<sys::mln_render_session, _>(Arc::clone(&owner.state))
);

#[pyclass(name = "_ResourceRequestHandle")]
struct ResourceRequestHandle {
    // The accepted cancel callback, which native owns until it retires.
    cancel_root: Mutex<std::sync::Weak<GeneratedCallbackRoot>>,
    // Dropped by hand, with the GIL released; see the Drop impl below.
    state: ManuallyDrop<
        Arc<maplibre_core::decision::DecisionHandleState<sys::mln_resource_request_handle>>,
    >,
}
impl Drop for ResourceRequestHandle {
    fn drop(&mut self) {
        // SAFETY: drop runs once, and nothing reads the field after this take.
        let state = unsafe { ManuallyDrop::take(&mut self.state) };
        // The last reference releases the native handle, and release waits for
        // a cancel callback running on a MapLibre thread. That callback needs
        // the GIL this thread holds while collecting the Python owner, so the
        // release runs detached.
        generated_finalize(move || drop(state));
    }
}
#[pymethods]
impl ResourceRequestHandle {
    #[getter]
    fn closed(&self) -> bool {
        self.state.native_for_call().is_err()
    }
    #[getter]
    fn id(&self) -> u64 {
        maplibre_core::handle::NativeHandle::to_raw(self.state.issued_handle())
    }
    fn __traverse__(&self, visit: pyo3::gc::PyVisit<'_>) -> Result<(), pyo3::gc::PyTraverseError> {
        let root = self
            .cancel_root
            .lock()
            .unwrap_or_else(|p| p.into_inner())
            .upgrade();
        if let Some(root) = root {
            for callback in root.lock().unwrap_or_else(|p| p.into_inner()).iter() {
                visit.call(callback)?;
            }
        }
        Ok(())
    }
    fn __clear__(&self) {
        let root = self
            .cancel_root
            .lock()
            .unwrap_or_else(|p| p.into_inner())
            .upgrade();
        if let Some(root) = root {
            let callbacks = std::mem::take(&mut *root.lock().unwrap_or_else(|p| p.into_inner()));
            drop(callbacks);
        }
    }
}
pub(crate) const RESOURCE_REQUEST_DECISION: maplibre_core::decision::DecisionHandleFns<
    sys::mln_resource_request_handle,
> = unsafe {
    maplibre_core::decision::DecisionHandleFns::new(
        "ResourceRequestHandle",
        sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE,
        sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH,
        sys::mln_resource_request_release,
        sys::mln_resource_request_set_cancel_callback,
        &[
            "mln_resource_request_complete",
            "mln_resource_request_cancelled",
            "mln_resource_request_set_cancel_callback",
            "mln_resource_request_release",
        ],
    )
};
unsafe extern "C" fn generated_exit_release_mln_runtime(
    handle: sys::mln_runtime,
    completion: *const sys::mln_completion,
) -> sys::mln_status {
    unsafe { sys::mln_runtime_release(handle, completion, std::ptr::null_mut()) }
}
generated_owner!(
    RuntimeHandle,
    "_RuntimeHandle",
    mln_runtime,
    Some(generated_dispose_mln_runtime),
    Some(generated_exit_release_mln_runtime),
    |_, owner| GeneratedReadScope::new::<sys::mln_runtime, _>(Arc::clone(&owner.state))
);
#[pyfunction]
#[pyo3(signature = (jni_env, jni_class, context))]
fn android_init(
    py: Python<'_>,
    jni_env: &Bound<'_, PyAny>,
    jni_class: &Bound<'_, PyAny>,
    context: &Bound<'_, PyAny>,
) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_android_init", 0)?;
    let jni_env_value = jni_env.clone().extract::<usize>()? as _;
    let jni_class_value = jni_class.clone().extract::<usize>()? as _;
    let context_value = context.clone().extract::<usize>()? as _;
    unsafe {
        call.status(|diagnostic| {
            sys::mln_android_init(jni_env_value, jni_class_value, context_value, diagnostic)
        })
    }?;
    Ok(py.None())
}

#[pyfunction]
#[pyo3(signature = ())]
fn c_version(py: Python<'_>) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_c_version", 0)?;
    let result = unsafe { call.run(|| sys::mln_c_version()) };
    generated_value(py, result)
}

#[pyfunction]
#[pyo3(signature = (data, options=None))]
fn geojson_source_data_create(
    py: Python<'_>,
    data: &Bound<'_, PyAny>,
    options: Option<Bound<'_, PyAny>>,
) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_geojson_source_data_create", 0)?;
    let storage = &mut call.storage;
    let data_value = storage.buffer(data.clone(), false)?;
    let options = options.unwrap_or_else(|| py.None().into_bound(py));
    let options_value = generated_maybe(&options, |options| {
        generated_input_mln_geojson_source_options(&options, storage)
    })?;
    let mut out_data: sys::mln_geojson_source_data = unsafe { std::mem::zeroed() };
    unsafe {
        call.status(|diagnostic| {
            sys::mln_geojson_source_data_create(
                data_value,
                generated_pointer(&options_value),
                &mut out_data,
                diagnostic,
            )
        })
    }?;
    unsafe { GeojsonSourceDataHandle::adopt(py, out_data, Vec::new()) }
}

#[pyfunction]
#[pyo3(signature = (meters))]
fn lat_lng_for_projected_meters(py: Python<'_>, meters: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_lat_lng_for_projected_meters", 0)?;
    let storage = &mut call.storage;
    let meters_value = generated_input_mln_projected_meters(&meters.clone(), storage)?;
    let mut out_coordinate: sys::mln_lat_lng = unsafe { std::mem::zeroed() };
    unsafe {
        call.status(|diagnostic| {
            sys::mln_lat_lng_for_projected_meters(meters_value, &mut out_coordinate, diagnostic)
        })
    }?;
    generated_copy_mln_lat_lng(py, &out_coordinate)
}

#[pyfunction]
#[pyo3(signature = ())]
fn log_clear_callback(py: Python<'_>) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_log_clear_callback", 0)?;
    unsafe { call.status(|diagnostic| sys::mln_log_clear_callback(diagnostic)) }?;
    Ok(py.None())
}

#[pyfunction]
#[pyo3(signature = (mask))]
fn log_set_async_severity_mask(
    py: Python<'_>,
    mask: sys::mln_log_severity_mask,
) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_log_set_async_severity_mask", 0)?;
    unsafe { call.status(|diagnostic| sys::mln_log_set_async_severity_mask(mask, diagnostic)) }?;
    Ok(py.None())
}

#[pyfunction]
fn log_set_callback(py: Python<'_>, callback: &Bound<'_, PyAny>) -> PyResult<()> {
    generated_check_reentry()?;
    let storage = &mut GeneratedInputStorage::default();
    let enabled = !callback.getattr("callback")?.is_none();
    let context = if enabled {
        storage.register_callbacks(vec![callback.getattr("_invoke_callback")?.unbind()])
    } else {
        std::ptr::null_mut()
    };
    let native_callback: sys::mln_log_callback = if enabled {
        Some(generated_callback_mln_log_set_callback_registration_callback)
    } else {
        None
    };
    let release = if enabled {
        Some(generated_release_callbacks_no_reentry as unsafe extern "C" fn(*mut c_void))
    } else {
        None
    };
    maplibre_core::check(|diagnostic| unsafe {
        generated_native_call(py, || {
            sys::mln_log_set_callback(native_callback, context, release, diagnostic)
        })
    })
    .map_err(map_error)?;
    storage.accept_callbacks();
    Ok(())
}

#[pyfunction]
#[pyo3(signature = ())]
fn network_status_get(py: Python<'_>) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_network_status_get", 0)?;
    let mut out_status: sys::mln_network_status = unsafe { std::mem::zeroed() };
    unsafe { call.status(|diagnostic| sys::mln_network_status_get(&mut out_status, diagnostic)) }?;
    generated_value(py, out_status)
}

#[pyfunction]
#[pyo3(signature = (input_status))]
fn network_status_set(
    py: Python<'_>,
    input_status: sys::mln_network_status,
) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_network_status_set", 0)?;
    unsafe { call.status(|diagnostic| sys::mln_network_status_set(input_status, diagnostic)) }?;
    Ok(py.None())
}

#[pyfunction]
#[pyo3(signature = ())]
fn opengl_supported_context_provider_mask(py: Python<'_>) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_opengl_supported_context_provider_mask", 0)?;
    let result = unsafe { call.run(|| sys::mln_opengl_supported_context_provider_mask()) };
    generated_value(py, result)
}

#[pyfunction]
#[pyo3(signature = ())]
fn plugin_get_register_function_v1(py: Python<'_>) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_plugin_get_register_function_v1", 0)?;
    let result = unsafe { call.run(|| sys::mln_plugin_get_register_function_v1()) };
    generated_value(py, result.map_or(0, |function| function as usize))
}

#[pyfunction]
#[pyo3(signature = (coordinate))]
fn projected_meters_for_lat_lng(
    py: Python<'_>,
    coordinate: &Bound<'_, PyAny>,
) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_projected_meters_for_lat_lng", 0)?;
    let storage = &mut call.storage;
    let coordinate_value = generated_input_mln_lat_lng(&coordinate.clone(), storage)?;
    let mut out_meters: sys::mln_projected_meters = unsafe { std::mem::zeroed() };
    unsafe {
        call.status(|diagnostic| {
            sys::mln_projected_meters_for_lat_lng(coordinate_value, &mut out_meters, diagnostic)
        })
    }?;
    generated_copy_mln_projected_meters(py, &out_meters)
}

#[pyfunction]
#[pyo3(signature = (extent))]
fn render_target_extent_physical_size(
    py: Python<'_>,
    extent: &Bound<'_, PyAny>,
) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_render_target_extent_physical_size", 0)?;
    let storage = &mut call.storage;
    let extent_value = generated_input_mln_render_target_extent(&extent.clone(), storage)?;
    let mut out_width: u32 = unsafe { std::mem::zeroed() };
    let mut out_height: u32 = unsafe { std::mem::zeroed() };
    unsafe {
        call.status(|diagnostic| {
            sys::mln_render_target_extent_physical_size(
                &extent_value,
                &mut out_width,
                &mut out_height,
                diagnostic,
            )
        })
    }?;
    let dict = PyDict::new(py);
    dict.set_item("width", generated_value(py, out_width)?)?;
    dict.set_item("height", generated_value(py, out_height)?)?;
    Ok(dict.into_any().unbind())
}

#[pyfunction]
#[pyo3(signature = (input_box))]
fn rendered_query_geometry_box(
    py: Python<'_>,
    input_box: &Bound<'_, PyAny>,
) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_rendered_query_geometry_box", 0)?;
    let storage = &mut call.storage;
    let input_box_value = generated_input_mln_screen_box(&input_box.clone(), storage)?;
    let result = unsafe { call.run(|| sys::mln_rendered_query_geometry_box(input_box_value)) };
    generated_copy_mln_rendered_query_geometry(py, &result)
}

#[pyfunction]
#[pyo3(signature = (points))]
fn rendered_query_geometry_line_string(
    py: Python<'_>,
    points: &Bound<'_, PyAny>,
) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_rendered_query_geometry_line_string", 0)?;
    let storage = &mut call.storage;
    let points_values = generated_items(points, |item| {
        generated_input_mln_screen_point(&item, storage)
    })?;
    let result = unsafe {
        call.run(|| {
            sys::mln_rendered_query_geometry_line_string(
                points_values.as_ptr(),
                points_values.len(),
            )
        })
    };
    generated_copy_mln_rendered_query_geometry(py, &result)
}

#[pyfunction]
#[pyo3(signature = (point))]
fn rendered_query_geometry_point(py: Python<'_>, point: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_rendered_query_geometry_point", 0)?;
    let storage = &mut call.storage;
    let point_value = generated_input_mln_screen_point(&point.clone(), storage)?;
    let result = unsafe { call.run(|| sys::mln_rendered_query_geometry_point(point_value)) };
    generated_copy_mln_rendered_query_geometry(py, &result)
}

#[pyfunction]
#[pyo3(signature = (options=None))]
fn runtime_create(py: Python<'_>, options: Option<Bound<'_, PyAny>>) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_runtime_create", 0)?;
    let storage = &mut call.storage;
    let options = options.unwrap_or_else(|| py.None().into_bound(py));
    let options_value = generated_input_mln_runtime_options(&options.clone(), storage)?;
    let mut out_runtime: sys::mln_runtime = unsafe { std::mem::zeroed() };
    unsafe {
        call.status(|diagnostic| {
            sys::mln_runtime_create(&options_value, &mut out_runtime, diagnostic)
        })
    }?;
    let callback_roots = call.accept_callbacks();
    unsafe { RuntimeHandle::adopt(py, out_runtime, callback_roots.clone()) }
}

#[pyfunction]
#[pyo3(signature = ())]
fn supported_render_backend_mask(py: Python<'_>) -> PyResult<Py<PyAny>> {
    let mut call = GeneratedCall::new(py, "mln_supported_render_backend_mask", 0)?;
    let result = unsafe { call.run(|| sys::mln_supported_render_backend_mask()) };
    generated_value(py, result)
}

fn register_generated_functions(module: &Bound<'_, PyModule>) -> PyResult<()> {
    module.add_class::<GeneratedReadScope>()?;
    module.add_class::<AcquiredFrameHandle>()?;
    module.add_class::<BufferHandle>()?;
    module.add_class::<EventBatchHandle>()?;
    module.add_class::<GeojsonSourceDataHandle>()?;
    module.add_class::<MapHandle>()?;
    module.add_class::<MapProjectionHandle>()?;
    module.add_class::<RenderFrameBatchHandle>()?;
    module.add_class::<RenderSessionHandle>()?;
    module.add_class::<ResourceRequestHandle>()?;
    module.add_class::<RuntimeHandle>()?;
    module.add_class::<HttpHeaderTransformResponseScope>()?;
    module.add_class::<ResourceTransformResponseScope>()?;
    module.add_function(wrap_pyfunction!(_default_animation_options, module)?)?;
    module.add_function(wrap_pyfunction!(_default_bound_options, module)?)?;
    module.add_function(wrap_pyfunction!(_default_camera_delta, module)?)?;
    module.add_function(wrap_pyfunction!(_default_camera_fit_options, module)?)?;
    module.add_function(wrap_pyfunction!(_default_camera_options, module)?)?;
    module.add_function(wrap_pyfunction!(_default_camera_update, module)?)?;
    module.add_function(wrap_pyfunction!(
        _default_custom_geometry_source_options,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(
        _default_custom_mvt_vector_source_options,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(_default_frame_demand, module)?)?;
    module.add_function(wrap_pyfunction!(_default_free_camera_options, module)?)?;
    module.add_function(wrap_pyfunction!(_default_geojson_source_options, module)?)?;
    module.add_function(wrap_pyfunction!(_default_gpu_sync, module)?)?;
    module.add_function(wrap_pyfunction!(_default_map_options, module)?)?;
    module.add_function(wrap_pyfunction!(_default_map_tile_options, module)?)?;
    module.add_function(wrap_pyfunction!(_default_map_viewport_options, module)?)?;
    module.add_function(wrap_pyfunction!(
        _default_metal_borrowed_texture_descriptor,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(
        _default_metal_owned_texture_descriptor,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(_default_metal_surface_descriptor, module)?)?;
    module.add_function(wrap_pyfunction!(
        _default_opengl_borrowed_texture_descriptor,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(
        _default_opengl_owned_texture_descriptor,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(
        _default_opengl_surface_descriptor,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(
        _default_premultiplied_rgba8_image,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(_default_projection_mode, module)?)?;
    module.add_function(wrap_pyfunction!(
        _default_render_session_attach_options,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(
        _default_rendered_feature_query_options,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(_default_runtime_options, module)?)?;
    module.add_function(wrap_pyfunction!(
        _default_source_feature_query_options,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(_default_style_image_info, module)?)?;
    module.add_function(wrap_pyfunction!(_default_style_image_options, module)?)?;
    module.add_function(wrap_pyfunction!(
        _default_style_tile_source_options,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(_default_style_transition_options, module)?)?;
    module.add_function(wrap_pyfunction!(_default_texture_image_info, module)?)?;
    module.add_function(wrap_pyfunction!(
        _default_vulkan_borrowed_texture_descriptor,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(
        _default_vulkan_owned_texture_descriptor,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(
        _default_vulkan_surface_descriptor,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(
        _default_webgpu_borrowed_texture_descriptor,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(
        _default_webgpu_owned_texture_descriptor,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(
        _default_webgpu_surface_descriptor,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(android_init, module)?)?;
    module.add_function(wrap_pyfunction!(c_version, module)?)?;
    module.add_function(wrap_pyfunction!(geojson_source_data_create, module)?)?;
    module.add_function(wrap_pyfunction!(lat_lng_for_projected_meters, module)?)?;
    module.add_function(wrap_pyfunction!(log_clear_callback, module)?)?;
    module.add_function(wrap_pyfunction!(log_set_async_severity_mask, module)?)?;
    module.add_function(wrap_pyfunction!(log_set_callback, module)?)?;
    module.add_function(wrap_pyfunction!(network_status_get, module)?)?;
    module.add_function(wrap_pyfunction!(network_status_set, module)?)?;
    module.add_function(wrap_pyfunction!(
        opengl_supported_context_provider_mask,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(plugin_get_register_function_v1, module)?)?;
    module.add_function(wrap_pyfunction!(projected_meters_for_lat_lng, module)?)?;
    module.add_function(wrap_pyfunction!(
        render_target_extent_physical_size,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(rendered_query_geometry_box, module)?)?;
    module.add_function(wrap_pyfunction!(
        rendered_query_geometry_line_string,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(rendered_query_geometry_point, module)?)?;
    module.add_function(wrap_pyfunction!(runtime_create, module)?)?;
    module.add_function(wrap_pyfunction!(supported_render_backend_mask, module)?)?;
    Ok(())
}

#[pyclass(name = "_HttpHeaderTransformResponseScope")]
struct HttpHeaderTransformResponseScope {
    native: usize,
    scope: GeneratedCallbackScope,
}

#[pyclass(name = "_ResourceTransformResponseScope")]
struct ResourceTransformResponseScope {
    native: usize,
    scope: GeneratedCallbackScope,
}
