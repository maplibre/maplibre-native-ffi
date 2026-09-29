// Generated from the C headers by tools/bindgen. Do not edit.

fn generated_copy_mln_animation_options(
    py: Python<'_>,
    value: &sys::mln_animation_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "duration_ms",
        if value.fields & sys::MLN_ANIMATION_OPTION_DURATION == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.duration_ms).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "velocity",
        if value.fields & sys::MLN_ANIMATION_OPTION_VELOCITY == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.velocity).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "min_zoom",
        if value.fields & sys::MLN_ANIMATION_OPTION_MIN_ZOOM == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.min_zoom).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "easing",
        if value.fields & sys::MLN_ANIMATION_OPTION_EASING == 0 {
            py.None()
        } else {
            generated_copy_mln_unit_bezier(py, &value.easing)?
        },
    )?;
    dict.set_item(
        "transition_id",
        if value.fields & sys::MLN_ANIMATION_OPTION_TRANSITION_ID == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.transition_id).into_pyobject(py)?).into_any()
        },
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
        if value.fields & sys::MLN_BOUND_OPTION_BOUNDS == 0 {
            py.None()
        } else {
            generated_copy_mln_lat_lng_bounds(py, &value.bounds)?
        },
    )?;
    dict.set_item(
        "min_zoom",
        if value.fields & sys::MLN_BOUND_OPTION_MIN_ZOOM == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.min_zoom).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "max_zoom",
        if value.fields & sys::MLN_BOUND_OPTION_MAX_ZOOM == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.max_zoom).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "min_pitch",
        if value.fields & sys::MLN_BOUND_OPTION_MIN_PITCH == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.min_pitch).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "max_pitch",
        if value.fields & sys::MLN_BOUND_OPTION_MAX_PITCH == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.max_pitch).into_pyobject(py)?).into_any()
        },
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
    dict.set_item(
        "kind",
        pyo3::BoundObject::unbind((value.kind).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "offset",
        generated_copy_mln_screen_point(py, &value.offset)?,
    )?;
    dict.set_item(
        "amount",
        pyo3::BoundObject::unbind((value.amount).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "anchor",
        if !value.has_anchor {
            py.None()
        } else {
            generated_copy_mln_screen_point(py, &value.anchor)?
        },
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
        if value.fields & sys::MLN_CAMERA_FIT_OPTION_PADDING == 0 {
            py.None()
        } else {
            generated_copy_mln_edge_insets(py, &value.padding)?
        },
    )?;
    dict.set_item(
        "bearing",
        if value.fields & sys::MLN_CAMERA_FIT_OPTION_BEARING == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.bearing).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "pitch",
        if value.fields & sys::MLN_CAMERA_FIT_OPTION_PITCH == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.pitch).into_pyobject(py)?).into_any()
        },
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
        if value.fields & sys::MLN_CAMERA_OPTION_CENTER == 0 {
            py.None()
        } else {
            {
                let inner = PyDict::new(py);
                inner.set_item(
                    "latitude",
                    pyo3::BoundObject::unbind((value.latitude).into_pyobject(py)?).into_any(),
                )?;
                inner.set_item(
                    "longitude",
                    pyo3::BoundObject::unbind((value.longitude).into_pyobject(py)?).into_any(),
                )?;
                inner.into_any().unbind()
            }
        },
    )?;
    dict.set_item(
        "center_altitude",
        if value.fields & sys::MLN_CAMERA_OPTION_CENTER_ALTITUDE == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.center_altitude).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "padding",
        if value.fields & sys::MLN_CAMERA_OPTION_PADDING == 0 {
            py.None()
        } else {
            generated_copy_mln_edge_insets(py, &value.padding)?
        },
    )?;
    dict.set_item(
        "anchor",
        if value.fields & sys::MLN_CAMERA_OPTION_ANCHOR == 0 {
            py.None()
        } else {
            generated_copy_mln_screen_point(py, &value.anchor)?
        },
    )?;
    dict.set_item(
        "zoom",
        if value.fields & sys::MLN_CAMERA_OPTION_ZOOM == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.zoom).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "bearing",
        if value.fields & sys::MLN_CAMERA_OPTION_BEARING == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.bearing).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "pitch",
        if value.fields & sys::MLN_CAMERA_OPTION_PITCH == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.pitch).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "roll",
        if value.fields & sys::MLN_CAMERA_OPTION_ROLL == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.roll).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "field_of_view",
        if value.fields & sys::MLN_CAMERA_OPTION_FOV == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.field_of_view).into_pyobject(py)?).into_any()
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_camera_query_result(
    py: Python<'_>,
    value: &sys::mln_camera_query_result,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "generation",
        pyo3::BoundObject::unbind((value.generation).into_pyobject(py)?).into_any(),
    )?;
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
    dict.set_item(
        "mode",
        pyo3::BoundObject::unbind((value.mode).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "camera",
        generated_copy_mln_camera_options(py, &value.camera)?,
    )?;
    dict.set_item(
        "animation",
        generated_copy_mln_animation_options(py, &value.animation)?,
    )?;
    dict.set_item(
        "gesture_phase",
        pyo3::BoundObject::unbind((value.gesture_phase).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_canonical_tile_id(
    py: Python<'_>,
    value: &sys::mln_canonical_tile_id,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "z",
        pyo3::BoundObject::unbind((value.z).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "x",
        pyo3::BoundObject::unbind((value.x).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "y",
        pyo3::BoundObject::unbind((value.y).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

unsafe extern "C" fn generated_callback_mln_custom_geometry_source_options_fetch_tile(
    user_data: *mut std::ffi::c_void,
    tile_id: sys::mln_canonical_tile_id,
) -> () {
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        Python::try_attach(|py| -> PyResult<()> {
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 0) }) else {
                return Ok(());
            };
            let _result = callback
                .bind(py)
                .call1((generated_copy_mln_canonical_tile_id(py, &tile_id)?,))?;
            Ok(())
        })
        .unwrap_or(Ok(()))
    }));
    match result {
        Ok(Ok(result)) => result,
        Ok(Err(error)) => {
            Python::try_attach(|py| error.write_unraisable(py, None));
            ()
        }
        Err(_) => (),
    }
}

unsafe extern "C" fn generated_callback_mln_custom_geometry_source_options_cancel_tile(
    user_data: *mut std::ffi::c_void,
    tile_id: sys::mln_canonical_tile_id,
) -> () {
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        Python::try_attach(|py| -> PyResult<()> {
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 1) }) else {
                return Ok(());
            };
            let _result = callback
                .bind(py)
                .call1((generated_copy_mln_canonical_tile_id(py, &tile_id)?,))?;
            Ok(())
        })
        .unwrap_or(Ok(()))
    }));
    match result {
        Ok(Ok(result)) => result,
        Ok(Err(error)) => {
            Python::try_attach(|py| error.write_unraisable(py, None));
            ()
        }
        Err(_) => (),
    }
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
        if value.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.min_zoom).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "max_zoom",
        if value.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.max_zoom).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "tolerance",
        if value.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.tolerance).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "tile_size",
        if value.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.tile_size).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "buffer",
        if value.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.buffer).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "clip",
        if value.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.clip).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "wrap",
        if value.fields & sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.wrap).into_pyobject(py)?).into_any()
        },
    )?;
    Ok(dict.into_any().unbind())
}

unsafe extern "C" fn generated_callback_mln_custom_mvt_vector_source_options_fetch_tile(
    user_data: *mut std::ffi::c_void,
    tile_id: sys::mln_canonical_tile_id,
) -> () {
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        Python::try_attach(|py| -> PyResult<()> {
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 0) }) else {
                return Ok(());
            };
            let _result = callback
                .bind(py)
                .call1((generated_copy_mln_canonical_tile_id(py, &tile_id)?,))?;
            Ok(())
        })
        .unwrap_or(Ok(()))
    }));
    match result {
        Ok(Ok(result)) => result,
        Ok(Err(error)) => {
            Python::try_attach(|py| error.write_unraisable(py, None));
            ()
        }
        Err(_) => (),
    }
}

unsafe extern "C" fn generated_callback_mln_custom_mvt_vector_source_options_cancel_tile(
    user_data: *mut std::ffi::c_void,
    tile_id: sys::mln_canonical_tile_id,
) -> () {
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        Python::try_attach(|py| -> PyResult<()> {
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 1) }) else {
                return Ok(());
            };
            let _result = callback
                .bind(py)
                .call1((generated_copy_mln_canonical_tile_id(py, &tile_id)?,))?;
            Ok(())
        })
        .unwrap_or(Ok(()))
    }));
    match result {
        Ok(Ok(result)) => result,
        Ok(Err(error)) => {
            Python::try_attach(|py| error.write_unraisable(py, None));
            ()
        }
        Err(_) => (),
    }
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
        if value.fields & sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.min_zoom).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "max_zoom",
        if value.fields & sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.max_zoom).into_pyobject(py)?).into_any()
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_edge_insets(
    py: Python<'_>,
    value: &sys::mln_edge_insets,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "top",
        pyo3::BoundObject::unbind((value.top).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "left",
        pyo3::BoundObject::unbind((value.left).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "bottom",
        pyo3::BoundObject::unbind((value.bottom).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "right",
        pyo3::BoundObject::unbind((value.right).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_egl_context_descriptor(
    py: Python<'_>,
    value: &sys::mln_egl_context_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "display",
        pyo3::BoundObject::unbind((value.display as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "config",
        pyo3::BoundObject::unbind((value.config as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "share_context",
        pyo3::BoundObject::unbind((value.share_context as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "client_api",
        pyo3::BoundObject::unbind((value.client_api).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "get_proc_address",
        pyo3::BoundObject::unbind((value.get_proc_address as usize).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_frame_demand(
    py: Python<'_>,
    value: &sys::mln_frame_demand,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "flags",
        pyo3::BoundObject::unbind((value.flags).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "token",
        pyo3::BoundObject::unbind((value.token).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "coalescing_boundary",
        pyo3::BoundObject::unbind((value.coalescing_boundary).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "timeout_ns",
        pyo3::BoundObject::unbind((value.timeout_ns).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_free_camera_options(
    py: Python<'_>,
    value: &sys::mln_free_camera_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "position",
        if value.fields & sys::MLN_FREE_CAMERA_OPTION_POSITION == 0 {
            py.None()
        } else {
            generated_copy_mln_vec3(py, &value.position)?
        },
    )?;
    dict.set_item(
        "orientation",
        if value.fields & sys::MLN_FREE_CAMERA_OPTION_ORIENTATION == 0 {
            py.None()
        } else {
            generated_copy_mln_quaternion(py, &value.orientation)?
        },
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
        if value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.min_zoom).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "max_zoom",
        if value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.max_zoom).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "tolerance",
        if value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_TOLERANCE == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.tolerance).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "cluster_max_zoom",
        if value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.cluster_max_zoom).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "cluster_properties",
        if value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES == 0 {
            py.None()
        } else {
            PyBytes::new(py, unsafe {
                generated_slice(
                    value.cluster_properties.data.cast::<u8>(),
                    value.cluster_properties.size,
                )?
            })
            .into_any()
            .unbind()
        },
    )?;
    dict.set_item(
        "tile_size",
        if value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.tile_size).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "buffer",
        if value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_BUFFER == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.buffer).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "cluster_radius",
        if value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.cluster_radius).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "cluster_min_points",
        if value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.cluster_min_points).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "line_metrics",
        if value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.line_metrics).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "cluster",
        if value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.cluster).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "synchronous_tiling",
        if value.fields & sys::MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.synchronous_tiling).into_pyobject(py)?).into_any()
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_gpu_sync(py: Python<'_>, value: &sys::mln_gpu_sync) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "kind",
        pyo3::BoundObject::unbind((value.kind).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "object",
        pyo3::BoundObject::unbind((value.object).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "value",
        pyo3::BoundObject::unbind((value.value).into_pyobject(py)?).into_any(),
    )?;
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

    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        Python::try_attach(|py| -> PyResult<sys::mln_status> {
            let callback_scope = GeneratedCallbackGuard::new();
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 0) }) else {
                return Ok(sys::MLN_STATUS_NATIVE_ERROR);
            };
            let result = callback.bind(py).call1((
                pyo3::BoundObject::unbind((kind).into_pyobject(py)?).into_any(),
                {
                    if url.is_null() {
                        return Err(native_error("null native string"));
                    }
                    unsafe { std::ffi::CStr::from_ptr(url) }
                        .to_str()
                        .map_err(|_| native_error("native string is not UTF-8"))?
                        .into_pyobject(py)?
                        .into_any()
                        .unbind()
                },
                Py::new(
                    py,
                    HttpHeaderTransformResponseScope {
                        native: out_response as usize,
                        scope: callback_scope.scope(),
                    },
                )?,
            ))?;
            result.extract::<sys::mln_status>()
        })
        .unwrap_or(Ok(sys::MLN_STATUS_NATIVE_ERROR))
    }));
    match result {
        Ok(Ok(result)) => result,
        Ok(Err(error)) => {
            Python::try_attach(|py| error.write_unraisable(py, None));
            sys::MLN_STATUS_NATIVE_ERROR
        }
        Err(_) => sys::MLN_STATUS_NATIVE_ERROR,
    }
}

fn generated_copy_mln_image_content(
    py: Python<'_>,
    value: &sys::mln_image_content,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "left",
        pyo3::BoundObject::unbind((value.left).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "top",
        pyo3::BoundObject::unbind((value.top).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "right",
        pyo3::BoundObject::unbind((value.right).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "bottom",
        pyo3::BoundObject::unbind((value.bottom).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_image_stretch(
    py: Python<'_>,
    value: &sys::mln_image_stretch,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "from_",
        pyo3::BoundObject::unbind((value.from).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "to",
        pyo3::BoundObject::unbind((value.to).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_lat_lng(py: Python<'_>, value: &sys::mln_lat_lng) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "latitude",
        pyo3::BoundObject::unbind((value.latitude).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "longitude",
        pyo3::BoundObject::unbind((value.longitude).into_pyobject(py)?).into_any(),
    )?;
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

    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        Python::try_attach(|py| -> PyResult<u32> {
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 0) }) else {
                return Ok(0);
            };
            let result = callback.bind(py).call1((
                pyo3::BoundObject::unbind((severity).into_pyobject(py)?).into_any(),
                pyo3::BoundObject::unbind((event).into_pyobject(py)?).into_any(),
                pyo3::BoundObject::unbind((code).into_pyobject(py)?).into_any(),
                {
                    if message.is_null() {
                        return Err(native_error("null native string"));
                    }
                    unsafe { std::ffi::CStr::from_ptr(message) }
                        .to_str()
                        .map_err(|_| native_error("native string is not UTF-8"))?
                        .into_pyobject(py)?
                        .into_any()
                        .unbind()
                },
            ))?;
            result.extract::<u32>()
        })
        .unwrap_or(Ok(0))
    }));
    match result {
        Ok(Ok(result)) => result,
        Ok(Err(error)) => {
            Python::try_attach(|py| error.write_unraisable(py, None));
            0
        }
        Err(_) => 0,
    }
}

fn generated_copy_mln_logical_extent(
    py: Python<'_>,
    value: &sys::mln_logical_extent,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "width",
        pyo3::BoundObject::unbind((value.width).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "height",
        pyo3::BoundObject::unbind((value.height).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "scale_factor",
        pyo3::BoundObject::unbind((value.scale_factor).into_pyobject(py)?).into_any(),
    )?;
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
    dict.set_item(
        "map_mode",
        pyo3::BoundObject::unbind((value.map_mode).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "fast_pfor_enabled",
        pyo3::BoundObject::unbind((value.fast_pfor_enabled).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "event_mask",
        pyo3::BoundObject::unbind((value.event_mask).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_map_snapshot(
    py: Python<'_>,
    value: &sys::mln_map_snapshot,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "debug_options",
        pyo3::BoundObject::unbind((value.debug_options).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "generation",
        pyo3::BoundObject::unbind((value.generation).into_pyobject(py)?).into_any(),
    )?;
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
    dict.set_item(
        "fully_loaded",
        pyo3::BoundObject::unbind((value.fully_loaded).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "rendering_stats_view_enabled",
        pyo3::BoundObject::unbind((value.rendering_stats_view_enabled).into_pyobject(py)?)
            .into_any(),
    )?;
    dict.set_item(
        "repaint_demand",
        pyo3::BoundObject::unbind((value.repaint_demand).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "gesture_in_progress",
        pyo3::BoundObject::unbind((value.gesture_in_progress).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "event_mask",
        pyo3::BoundObject::unbind((value.event_mask).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "latest_render_update_generation",
        pyo3::BoundObject::unbind((value.latest_render_update_generation).into_pyobject(py)?)
            .into_any(),
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
        if value.fields & sys::MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.prefetch_zoom_delta).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "lod_min_radius",
        if value.fields & sys::MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.lod_min_radius).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "lod_scale",
        if value.fields & sys::MLN_MAP_TILE_OPTION_LOD_SCALE == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.lod_scale).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "lod_pitch_threshold",
        if value.fields & sys::MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.lod_pitch_threshold).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "lod_zoom_shift",
        if value.fields & sys::MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.lod_zoom_shift).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "lod_mode",
        if value.fields & sys::MLN_MAP_TILE_OPTION_LOD_MODE == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.lod_mode).into_pyobject(py)?).into_any()
        },
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
        if value.fields & sys::MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.north_orientation).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "constrain_mode",
        if value.fields & sys::MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.constrain_mode).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "viewport_mode",
        if value.fields & sys::MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.viewport_mode).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "frustum_offset",
        if value.fields & sys::MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET == 0 {
            py.None()
        } else {
            generated_copy_mln_edge_insets(py, &value.frustum_offset)?
        },
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
    dict.set_item(
        "physical_width",
        pyo3::BoundObject::unbind((value.physical_width).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "physical_height",
        pyo3::BoundObject::unbind((value.physical_height).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "texture",
        pyo3::BoundObject::unbind((value.texture as usize).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_metal_context_descriptor(
    py: Python<'_>,
    value: &sys::mln_metal_context_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "device",
        pyo3::BoundObject::unbind((value.device as usize).into_pyobject(py)?).into_any(),
    )?;
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
    dict.set_item(
        "generation",
        pyo3::BoundObject::unbind((value.generation).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "width",
        pyo3::BoundObject::unbind((value.width).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "height",
        pyo3::BoundObject::unbind((value.height).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "scale_factor",
        pyo3::BoundObject::unbind((value.scale_factor).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "frame_id",
        pyo3::BoundObject::unbind((value.frame_id).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "texture",
        pyo3::BoundObject::unbind((value.texture as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "device",
        pyo3::BoundObject::unbind((value.device as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "pixel_format",
        pyo3::BoundObject::unbind((value.pixel_format).into_pyobject(py)?).into_any(),
    )?;
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
    dict.set_item(
        "layer",
        pyo3::BoundObject::unbind((value.layer as usize).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_offline_geometry_region_definition(
    py: Python<'_>,
    value: &sys::mln_offline_geometry_region_definition,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("style_url", {
        if value.style_url.is_null() {
            return Err(native_error("null native string"));
        }
        unsafe { std::ffi::CStr::from_ptr(value.style_url) }
            .to_str()
            .map_err(|_| native_error("native string is not UTF-8"))?
            .into_pyobject(py)?
            .into_any()
            .unbind()
    })?;
    dict.set_item(
        "geometry",
        PyBytes::new(py, unsafe {
            generated_slice(value.geometry.data.cast::<u8>(), value.geometry.size)?
        })
        .into_any()
        .unbind(),
    )?;
    dict.set_item(
        "min_zoom",
        pyo3::BoundObject::unbind((value.min_zoom).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "max_zoom",
        pyo3::BoundObject::unbind((value.max_zoom).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "pixel_ratio",
        pyo3::BoundObject::unbind((value.pixel_ratio).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "include_ideographs",
        pyo3::BoundObject::unbind((value.include_ideographs).into_pyobject(py)?).into_any(),
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
            sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID => {
                let variant = PyDict::new(py);
                variant.set_item("kind", "tile_pyramid")?;
                variant.set_item(
                    "value",
                    generated_copy_mln_offline_tile_pyramid_region_definition(
                        py,
                        &(unsafe { value.data.tile_pyramid }),
                    )?,
                )?;
                variant.into_any().unbind()
            }
            sys::MLN_OFFLINE_REGION_DEFINITION_GEOMETRY => {
                let variant = PyDict::new(py);
                variant.set_item("kind", "geometry")?;
                variant.set_item(
                    "value",
                    generated_copy_mln_offline_geometry_region_definition(
                        py,
                        &(unsafe { value.data.geometry }),
                    )?,
                )?;
                variant.into_any().unbind()
            }
            tag => {
                let variant = PyDict::new(py);
                variant.set_item("kind", py.None())?;
                variant.set_item("tag", tag)?;
                variant.into_any().unbind()
            }
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_offline_region_info(
    py: Python<'_>,
    value: &sys::mln_offline_region_info,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "id",
        pyo3::BoundObject::unbind((value.id).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "definition",
        generated_copy_mln_offline_region_definition(py, &value.definition)?,
    )?;
    dict.set_item(
        "metadata",
        PyBytes::new(py, unsafe {
            generated_slice(
                sys::mln_buffer_view {
                    data: value.metadata.cast(),
                    size: value.metadata_size as usize,
                }
                .data
                .cast::<u8>(),
                sys::mln_buffer_view {
                    data: value.metadata.cast(),
                    size: value.metadata_size as usize,
                }
                .size,
            )?
        })
        .into_any()
        .unbind(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_offline_region_status(
    py: Python<'_>,
    value: &sys::mln_offline_region_status,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "download_state",
        pyo3::BoundObject::unbind((value.download_state).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "completed_resource_count",
        pyo3::BoundObject::unbind((value.completed_resource_count).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "completed_resource_size",
        pyo3::BoundObject::unbind((value.completed_resource_size).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "completed_tile_count",
        pyo3::BoundObject::unbind((value.completed_tile_count).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "required_tile_count",
        pyo3::BoundObject::unbind((value.required_tile_count).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "completed_tile_size",
        pyo3::BoundObject::unbind((value.completed_tile_size).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "required_resource_count",
        pyo3::BoundObject::unbind((value.required_resource_count).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "required_resource_count_is_precise",
        pyo3::BoundObject::unbind((value.required_resource_count_is_precise).into_pyobject(py)?)
            .into_any(),
    )?;
    dict.set_item(
        "complete",
        pyo3::BoundObject::unbind((value.complete).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_offline_tile_pyramid_region_definition(
    py: Python<'_>,
    value: &sys::mln_offline_tile_pyramid_region_definition,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("style_url", {
        if value.style_url.is_null() {
            return Err(native_error("null native string"));
        }
        unsafe { std::ffi::CStr::from_ptr(value.style_url) }
            .to_str()
            .map_err(|_| native_error("native string is not UTF-8"))?
            .into_pyobject(py)?
            .into_any()
            .unbind()
    })?;
    dict.set_item(
        "bounds",
        generated_copy_mln_lat_lng_bounds(py, &value.bounds)?,
    )?;
    dict.set_item(
        "min_zoom",
        pyo3::BoundObject::unbind((value.min_zoom).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "max_zoom",
        pyo3::BoundObject::unbind((value.max_zoom).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "pixel_ratio",
        pyo3::BoundObject::unbind((value.pixel_ratio).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "include_ideographs",
        pyo3::BoundObject::unbind((value.include_ideographs).into_pyobject(py)?).into_any(),
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
    dict.set_item(
        "physical_width",
        pyo3::BoundObject::unbind((value.physical_width).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "physical_height",
        pyo3::BoundObject::unbind((value.physical_height).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "context",
        generated_copy_mln_opengl_context_descriptor(py, &value.context)?,
    )?;
    dict.set_item(
        "texture",
        pyo3::BoundObject::unbind((value.texture).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "target",
        pyo3::BoundObject::unbind((value.target).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_opengl_context_descriptor(
    py: Python<'_>,
    value: &sys::mln_opengl_context_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "ownership",
        pyo3::BoundObject::unbind((value.ownership).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "data",
        match value.platform {
            sys::MLN_OPENGL_CONTEXT_PLATFORM_WGL => {
                let variant = PyDict::new(py);
                variant.set_item("kind", "wgl")?;
                variant.set_item(
                    "value",
                    generated_copy_mln_wgl_context_descriptor(py, &(unsafe { value.data.wgl }))?,
                )?;
                variant.into_any().unbind()
            }
            sys::MLN_OPENGL_CONTEXT_PLATFORM_EGL => {
                let variant = PyDict::new(py);
                variant.set_item("kind", "egl")?;
                variant.set_item(
                    "value",
                    generated_copy_mln_egl_context_descriptor(py, &(unsafe { value.data.egl }))?,
                )?;
                variant.into_any().unbind()
            }
            sys::MLN_OPENGL_CONTEXT_PLATFORM_WEBGL => {
                let variant = PyDict::new(py);
                variant.set_item("kind", "webgl")?;
                variant.set_item(
                    "value",
                    generated_copy_mln_webgl_context_descriptor(
                        py,
                        &(unsafe { value.data.webgl }),
                    )?,
                )?;
                variant.into_any().unbind()
            }
            tag => {
                let variant = PyDict::new(py);
                variant.set_item("kind", py.None())?;
                variant.set_item("tag", tag)?;
                variant.into_any().unbind()
            }
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
    dict.set_item(
        "generation",
        pyo3::BoundObject::unbind((value.generation).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "width",
        pyo3::BoundObject::unbind((value.width).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "height",
        pyo3::BoundObject::unbind((value.height).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "scale_factor",
        pyo3::BoundObject::unbind((value.scale_factor).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "frame_id",
        pyo3::BoundObject::unbind((value.frame_id).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "texture",
        pyo3::BoundObject::unbind((value.texture).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "target",
        pyo3::BoundObject::unbind((value.target).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "internal_format",
        pyo3::BoundObject::unbind((value.internal_format).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "format",
        pyo3::BoundObject::unbind((value.format).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "type",
        pyo3::BoundObject::unbind((value.type_).into_pyobject(py)?).into_any(),
    )?;
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
    dict.set_item(
        "surface",
        pyo3::BoundObject::unbind((value.surface as usize).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_premultiplied_rgba8_image(
    py: Python<'_>,
    value: &sys::mln_premultiplied_rgba8_image,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "width",
        pyo3::BoundObject::unbind((value.width).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "height",
        pyo3::BoundObject::unbind((value.height).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "stride",
        pyo3::BoundObject::unbind((value.stride).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "pixels",
        PyBytes::new(py, unsafe {
            generated_slice(
                sys::mln_buffer_view {
                    data: value.pixels.cast(),
                    size: value.byte_length as usize,
                }
                .data
                .cast::<u8>(),
                sys::mln_buffer_view {
                    data: value.pixels.cast(),
                    size: value.byte_length as usize,
                }
                .size,
            )?
        })
        .into_any()
        .unbind(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_projected_meters(
    py: Python<'_>,
    value: &sys::mln_projected_meters,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "northing",
        pyo3::BoundObject::unbind((value.northing).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "easting",
        pyo3::BoundObject::unbind((value.easting).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_projection_mode(
    py: Python<'_>,
    value: &sys::mln_projection_mode,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "axonometric",
        if value.fields & sys::MLN_PROJECTION_MODE_AXONOMETRIC == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.axonometric).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "x_skew",
        if value.fields & sys::MLN_PROJECTION_MODE_X_SKEW == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.x_skew).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "y_skew",
        if value.fields & sys::MLN_PROJECTION_MODE_Y_SKEW == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.y_skew).into_pyobject(py)?).into_any()
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_quaternion(
    py: Python<'_>,
    value: &sys::mln_quaternion,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "x",
        pyo3::BoundObject::unbind((value.x).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "y",
        pyo3::BoundObject::unbind((value.y).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "z",
        pyo3::BoundObject::unbind((value.z).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "w",
        pyo3::BoundObject::unbind((value.w).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_queried_feature(
    py: Python<'_>,
    value: &sys::mln_queried_feature,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "feature",
        PyBytes::new(py, unsafe {
            generated_slice(value.feature.data.cast::<u8>(), value.feature.size)?
        })
        .into_any()
        .unbind(),
    )?;
    dict.set_item(
        "source_id",
        if value.fields & sys::MLN_QUERIED_FEATURE_SOURCE_ID == 0 {
            py.None()
        } else {
            copied_string_view(value.source_id)?
                .into_pyobject(py)?
                .into_any()
                .unbind()
        },
    )?;
    dict.set_item(
        "source_layer_id",
        if value.fields & sys::MLN_QUERIED_FEATURE_SOURCE_LAYER_ID == 0 {
            py.None()
        } else {
            copied_string_view(value.source_layer_id)?
                .into_pyobject(py)?
                .into_any()
                .unbind()
        },
    )?;
    dict.set_item(
        "state",
        if value.fields & sys::MLN_QUERIED_FEATURE_STATE == 0 {
            py.None()
        } else {
            PyBytes::new(py, unsafe {
                generated_slice(value.state.data.cast::<u8>(), value.state.size)?
            })
            .into_any()
            .unbind()
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_render_abandon_result(
    py: Python<'_>,
    value: &sys::mln_render_abandon_result,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "disposition",
        pyo3::BoundObject::unbind((value.disposition).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "quarantined_resource_count",
        pyo3::BoundObject::unbind((value.quarantined_resource_count).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_render_frame_result(
    py: Python<'_>,
    value: &sys::mln_render_frame_result,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "disposition",
        pyo3::BoundObject::unbind((value.disposition).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "token",
        pyo3::BoundObject::unbind((value.token).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "map_update_generation",
        pyo3::BoundObject::unbind((value.map_update_generation).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "extent_generation",
        pyo3::BoundObject::unbind((value.extent_generation).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "frame_generation",
        pyo3::BoundObject::unbind((value.frame_generation).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "needs_repaint",
        pyo3::BoundObject::unbind((value.needs_repaint).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_render_session_attach_options(
    py: Python<'_>,
    value: &sys::mln_render_session_attach_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "driver",
        pyo3::BoundObject::unbind((value.driver).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "requested_texture_ring_depth",
        pyo3::BoundObject::unbind((value.requested_texture_ring_depth).into_pyobject(py)?)
            .into_any(),
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
    dict.set_item(
        "driver",
        pyo3::BoundObject::unbind((value.driver).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "texture_ring_depth",
        pyo3::BoundObject::unbind((value.texture_ring_depth).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "flags",
        pyo3::BoundObject::unbind((value.flags).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_render_session_snapshot(
    py: Python<'_>,
    value: &sys::mln_render_session_snapshot,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "state",
        pyo3::BoundObject::unbind((value.state).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "driver",
        pyo3::BoundObject::unbind((value.driver).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "latest_result",
        pyo3::BoundObject::unbind((value.latest_result).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "extent",
        generated_copy_mln_render_target_extent(py, &value.extent)?,
    )?;
    dict.set_item(
        "generation",
        pyo3::BoundObject::unbind((value.generation).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "map_update_generation",
        pyo3::BoundObject::unbind((value.map_update_generation).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "rendered_update_generation",
        pyo3::BoundObject::unbind((value.rendered_update_generation).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "extent_generation",
        pyo3::BoundObject::unbind((value.extent_generation).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "frame_generation",
        pyo3::BoundObject::unbind((value.frame_generation).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "latest_demand_token",
        pyo3::BoundObject::unbind((value.latest_demand_token).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "pending_demand_count",
        pyo3::BoundObject::unbind((value.pending_demand_count).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "acquired_frame_count",
        pyo3::BoundObject::unbind((value.acquired_frame_count).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "target_ready",
        pyo3::BoundObject::unbind((value.target_ready).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "pending_changes",
        pyo3::BoundObject::unbind((value.pending_changes).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_render_target_extent(
    py: Python<'_>,
    value: &sys::mln_render_target_extent,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "width",
        pyo3::BoundObject::unbind((value.width).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "height",
        pyo3::BoundObject::unbind((value.height).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "scale_factor",
        pyo3::BoundObject::unbind((value.scale_factor).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_rendered_feature_query_options(
    py: Python<'_>,
    value: &sys::mln_rendered_feature_query_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "layer_ids",
        if value.fields & sys::MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS == 0 {
            py.None()
        } else {
            {
                let items = PyList::empty(py);
                for element in
                    unsafe { generated_slice(value.layer_ids, value.layer_id_count as usize)? }
                {
                    let item = copied_string_view(*element)?
                        .into_pyobject(py)?
                        .into_any()
                        .unbind();
                    items.append(item)?;
                }
                items.into_any().unbind()
            }
        },
    )?;
    dict.set_item(
        "filter",
        if value.filter.is_null() {
            py.None()
        } else {
            {
                let referenced = unsafe { *value.filter };
                PyBytes::new(py, unsafe {
                    generated_slice(referenced.data.cast::<u8>(), referenced.size)?
                })
                .into_any()
                .unbind()
            }
        },
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
            sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT => {
                let variant = PyDict::new(py);
                variant.set_item("kind", "point")?;
                variant.set_item(
                    "value",
                    generated_copy_mln_screen_point(py, &(unsafe { value.data.point }))?,
                )?;
                variant.into_any().unbind()
            }
            sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX => {
                let variant = PyDict::new(py);
                variant.set_item("kind", "box")?;
                variant.set_item(
                    "value",
                    generated_copy_mln_screen_box(py, &(unsafe { value.data.box_ }))?,
                )?;
                variant.into_any().unbind()
            }
            sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING => {
                let variant = PyDict::new(py);
                variant.set_item("kind", "line_string")?;
                variant.set_item(
                    "value",
                    generated_copy_mln_screen_line_string(
                        py,
                        &(unsafe { value.data.line_string }),
                    )?,
                )?;
                variant.into_any().unbind()
            }
            tag => {
                let variant = PyDict::new(py);
                variant.set_item("kind", py.None())?;
                variant.set_item("tag", tag)?;
                variant.into_any().unbind()
            }
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_rendering_stats(
    py: Python<'_>,
    value: &sys::mln_rendering_stats,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "encoding_time",
        pyo3::BoundObject::unbind((value.encoding_time).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "rendering_time",
        pyo3::BoundObject::unbind((value.rendering_time).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "frame_count",
        pyo3::BoundObject::unbind((value.frame_count).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "draw_call_count",
        pyo3::BoundObject::unbind((value.draw_call_count).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "total_draw_call_count",
        pyo3::BoundObject::unbind((value.total_draw_call_count).into_pyobject(py)?).into_any(),
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
        maplibre_core::resource::ResourceRequestHandleState::new(
            handle,
            maplibre_core::resource::ResourceRequestHandleFns::new(
                sys::mln_resource_request_complete,
                sys::mln_resource_request_release,
            ),
        )
    } {
        Ok(state) => state,
        Err(_) => return sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH,
    };
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        Python::try_attach(|py| -> PyResult<sys::mln_resource_provider_decision> {
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 0) }) else {
                return Ok(decision_state.finish_provider_decision(
                    maplibre_core::ResourceProviderDecision::PassThrough,
                ));
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
            Ok(decision_state.finish_provider_decision(
                if decision == sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE {
                    maplibre_core::ResourceProviderDecision::Handle
                } else {
                    maplibre_core::ResourceProviderDecision::PassThrough
                },
            ))
        })
        .unwrap_or(Ok(decision_state.finish_provider_decision(
            maplibre_core::ResourceProviderDecision::PassThrough,
        )))
    }));
    match result {
        Ok(Ok(result)) => result,
        Ok(Err(error)) => {
            Python::try_attach(|py| error.write_unraisable(py, None));
            decision_state
                .finish_provider_decision(maplibre_core::ResourceProviderDecision::PassThrough)
        }
        Err(_) => decision_state
            .finish_provider_decision(maplibre_core::ResourceProviderDecision::PassThrough),
    }
}

fn generated_copy_mln_resource_request(
    py: Python<'_>,
    value: &sys::mln_resource_request,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "requested_url",
        if value.requested_url.is_null() {
            py.None()
        } else {
            unsafe { std::ffi::CStr::from_ptr(value.requested_url) }
                .to_str()
                .map_err(|_| native_error("native string is not UTF-8"))?
                .into_pyobject(py)?
                .into_any()
                .unbind()
        },
    )?;
    dict.set_item(
        "resolved_url",
        if value.resolved_url.is_null() {
            py.None()
        } else {
            unsafe { std::ffi::CStr::from_ptr(value.resolved_url) }
                .to_str()
                .map_err(|_| native_error("native string is not UTF-8"))?
                .into_pyobject(py)?
                .into_any()
                .unbind()
        },
    )?;
    dict.set_item(
        "kind",
        pyo3::BoundObject::unbind((value.kind).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "loading_method",
        pyo3::BoundObject::unbind((value.loading_method).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "priority",
        pyo3::BoundObject::unbind((value.priority).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "usage",
        pyo3::BoundObject::unbind((value.usage).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "storage_policy",
        pyo3::BoundObject::unbind((value.storage_policy).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "range",
        if !value.has_range {
            py.None()
        } else {
            {
                let inner = PyDict::new(py);
                inner.set_item(
                    "start",
                    pyo3::BoundObject::unbind((value.range_start).into_pyobject(py)?).into_any(),
                )?;
                inner.set_item(
                    "end",
                    pyo3::BoundObject::unbind((value.range_end).into_pyobject(py)?).into_any(),
                )?;
                inner.into_any().unbind()
            }
        },
    )?;
    dict.set_item(
        "prior_modified_unix_ms",
        if !value.has_prior_modified {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.prior_modified_unix_ms).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "prior_expires_unix_ms",
        if !value.has_prior_expires {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.prior_expires_unix_ms).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "prior_etag",
        if value.prior_etag.is_null() {
            py.None()
        } else {
            unsafe { std::ffi::CStr::from_ptr(value.prior_etag) }
                .to_str()
                .map_err(|_| native_error("native string is not UTF-8"))?
                .into_pyobject(py)?
                .into_any()
                .unbind()
        },
    )?;
    dict.set_item(
        "prior_data",
        PyBytes::new(py, unsafe {
            generated_slice(
                sys::mln_buffer_view {
                    data: value.prior_data.cast(),
                    size: value.prior_data_size as usize,
                }
                .data
                .cast::<u8>(),
                sys::mln_buffer_view {
                    data: value.prior_data.cast(),
                    size: value.prior_data_size as usize,
                }
                .size,
            )?
        })
        .into_any()
        .unbind(),
    )?;
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

    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        Python::try_attach(|py| -> PyResult<sys::mln_status> {
            let callback_scope = GeneratedCallbackGuard::new();
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 0) }) else {
                return Ok(sys::MLN_STATUS_NATIVE_ERROR);
            };
            let result = callback.bind(py).call1((
                pyo3::BoundObject::unbind((kind).into_pyobject(py)?).into_any(),
                {
                    if url.is_null() {
                        return Err(native_error("null native string"));
                    }
                    unsafe { std::ffi::CStr::from_ptr(url) }
                        .to_str()
                        .map_err(|_| native_error("native string is not UTF-8"))?
                        .into_pyobject(py)?
                        .into_any()
                        .unbind()
                },
                Py::new(
                    py,
                    ResourceTransformResponseScope {
                        native: out_response as usize,
                        scope: callback_scope.scope(),
                    },
                )?,
            ))?;
            result.extract::<sys::mln_status>()
        })
        .unwrap_or(Ok(sys::MLN_STATUS_NATIVE_ERROR))
    }));
    match result {
        Ok(Ok(result)) => result,
        Ok(Err(error)) => {
            Python::try_attach(|py| error.write_unraisable(py, None));
            sys::MLN_STATUS_NATIVE_ERROR
        }
        Err(_) => sys::MLN_STATUS_NATIVE_ERROR,
    }
}

fn generated_copy_mln_runtime_event(
    py: Python<'_>,
    value: &sys::mln_runtime_event,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "type",
        pyo3::BoundObject::unbind((value.type_).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "source_type",
        pyo3::BoundObject::unbind((value.source_type).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "source",
        pyo3::BoundObject::unbind((value.source).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "code",
        pyo3::BoundObject::unbind((value.code).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "payload",
        match value.payload_type {
            sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME => {
                let variant = PyDict::new(py);
                variant.set_item("kind", "render_frame")?;
                variant.set_item(
                    "value",
                    generated_copy_mln_runtime_event_render_frame(
                        py,
                        &(unsafe { value.payload.render_frame }),
                    )?,
                )?;
                variant.into_any().unbind()
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP => {
                let variant = PyDict::new(py);
                variant.set_item("kind", "render_map")?;
                variant.set_item(
                    "value",
                    generated_copy_mln_runtime_event_render_map(
                        py,
                        &(unsafe { value.payload.render_map }),
                    )?,
                )?;
                variant.into_any().unbind()
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION => {
                let variant = PyDict::new(py);
                variant.set_item("kind", "tile_action")?;
                variant.set_item(
                    "value",
                    generated_copy_mln_runtime_event_tile_action(
                        py,
                        &(unsafe { value.payload.tile_action }),
                    )?,
                )?;
                variant.into_any().unbind()
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS => {
                let variant = PyDict::new(py);
                variant.set_item("kind", "offline_region_status")?;
                variant.set_item(
                    "value",
                    generated_copy_mln_runtime_event_offline_region_status(
                        py,
                        &(unsafe { value.payload.offline_region_status }),
                    )?,
                )?;
                variant.into_any().unbind()
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR => {
                let variant = PyDict::new(py);
                variant.set_item("kind", "offline_region_response_error")?;
                variant.set_item(
                    "value",
                    generated_copy_mln_runtime_event_offline_region_response_error(
                        py,
                        &(unsafe { value.payload.offline_region_response_error }),
                    )?,
                )?;
                variant.into_any().unbind()
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT => {
                let variant = PyDict::new(py);
                variant.set_item("kind", "offline_region_tile_count_limit")?;
                variant.set_item(
                    "value",
                    generated_copy_mln_runtime_event_offline_region_tile_count_limit(
                        py,
                        &(unsafe { value.payload.offline_region_tile_count_limit }),
                    )?,
                )?;
                variant.into_any().unbind()
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED => {
                let variant = PyDict::new(py);
                variant.set_item("kind", "camera_transition_finished")?;
                variant.set_item(
                    "value",
                    generated_copy_mln_runtime_event_camera_transition_finished(
                        py,
                        &(unsafe { value.payload.camera_transition_finished }),
                    )?,
                )?;
                variant.into_any().unbind()
            }
            tag => {
                let variant = PyDict::new(py);
                variant.set_item("kind", py.None())?;
                variant.set_item("tag", tag)?;
                variant.into_any().unbind()
            }
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_runtime_event_batch_view(
    py: Python<'_>,
    value: &sys::mln_runtime_event_batch_view,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("events", {
        let items = PyList::empty(py);
        for element in unsafe {
            generated_strided_values(
                value.events,
                value.event_count as usize,
                value.event_size as usize,
            )?
        } {
            let item = generated_copy_mln_runtime_event(py, &element)?;
            item.bind(py)
                .cast::<PyDict>()?
                .set_item("message", unsafe {
                    generated_arena_string(
                        value.messages.cast(),
                        value.messages_size as usize,
                        element.message_offset as usize,
                        element.message_size as usize,
                    )?
                })?;
            items.append(item)?;
        }
        items.into_any().unbind()
    })?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_runtime_event_camera_transition_finished(
    py: Python<'_>,
    value: &sys::mln_runtime_event_camera_transition_finished,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "transition_id",
        pyo3::BoundObject::unbind((value.transition_id).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_runtime_event_offline_region_response_error(
    py: Python<'_>,
    value: &sys::mln_runtime_event_offline_region_response_error,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "region_id",
        pyo3::BoundObject::unbind((value.region_id).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "reason",
        pyo3::BoundObject::unbind((value.reason).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_runtime_event_offline_region_status(
    py: Python<'_>,
    value: &sys::mln_runtime_event_offline_region_status,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "region_id",
        pyo3::BoundObject::unbind((value.region_id).into_pyobject(py)?).into_any(),
    )?;
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
    dict.set_item(
        "region_id",
        pyo3::BoundObject::unbind((value.region_id).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "limit",
        pyo3::BoundObject::unbind((value.limit).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_runtime_event_render_frame(
    py: Python<'_>,
    value: &sys::mln_runtime_event_render_frame,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "mode",
        pyo3::BoundObject::unbind((value.mode).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "needs_repaint",
        pyo3::BoundObject::unbind((value.needs_repaint).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "placement_changed",
        pyo3::BoundObject::unbind((value.placement_changed).into_pyobject(py)?).into_any(),
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
    dict.set_item(
        "mode",
        pyo3::BoundObject::unbind((value.mode).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_runtime_event_tile_action(
    py: Python<'_>,
    value: &sys::mln_runtime_event_tile_action,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "operation",
        pyo3::BoundObject::unbind((value.operation).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item("tile_id", generated_copy_mln_tile_id(py, &value.tile_id)?)?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_runtime_options(
    py: Python<'_>,
    value: &sys::mln_runtime_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "flags",
        pyo3::BoundObject::unbind((value.flags).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "asset_path",
        if value.asset_path.is_null() {
            py.None()
        } else {
            unsafe { std::ffi::CStr::from_ptr(value.asset_path) }
                .to_str()
                .map_err(|_| native_error("native string is not UTF-8"))?
                .into_pyobject(py)?
                .into_any()
                .unbind()
        },
    )?;
    dict.set_item(
        "cache_path",
        if value.cache_path.is_null() {
            py.None()
        } else {
            unsafe { std::ffi::CStr::from_ptr(value.cache_path) }
                .to_str()
                .map_err(|_| native_error("native string is not UTF-8"))?
                .into_pyobject(py)?
                .into_any()
                .unbind()
        },
    )?;
    dict.set_item(
        "event_mask",
        pyo3::BoundObject::unbind((value.event_mask).into_pyobject(py)?).into_any(),
    )?;
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
    dict.set_item("points", {
        let items = PyList::empty(py);
        for element in unsafe { generated_slice(value.points, value.point_count as usize)? } {
            let item = generated_copy_mln_screen_point(py, &(*element))?;
            items.append(item)?;
        }
        items.into_any().unbind()
    })?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_screen_point(
    py: Python<'_>,
    value: &sys::mln_screen_point,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "x",
        pyo3::BoundObject::unbind((value.x).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "y",
        pyo3::BoundObject::unbind((value.y).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_source_feature_query_options(
    py: Python<'_>,
    value: &sys::mln_source_feature_query_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "source_layer_ids",
        if value.fields & sys::MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS == 0 {
            py.None()
        } else {
            {
                let items = PyList::empty(py);
                for element in unsafe {
                    generated_slice(value.source_layer_ids, value.source_layer_id_count as usize)?
                } {
                    let item = copied_string_view(*element)?
                        .into_pyobject(py)?
                        .into_any()
                        .unbind();
                    items.append(item)?;
                }
                items.into_any().unbind()
            }
        },
    )?;
    dict.set_item(
        "filter",
        if value.filter.is_null() {
            py.None()
        } else {
            {
                let referenced = unsafe { *value.filter };
                PyBytes::new(py, unsafe {
                    generated_slice(referenced.data.cast::<u8>(), referenced.size)?
                })
                .into_any()
                .unbind()
            }
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_image_info(
    py: Python<'_>,
    value: &sys::mln_style_image_info,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "width",
        pyo3::BoundObject::unbind((value.width).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "height",
        pyo3::BoundObject::unbind((value.height).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "stride",
        pyo3::BoundObject::unbind((value.stride).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "byte_length",
        pyo3::BoundObject::unbind((value.byte_length).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "stretch_x_count",
        pyo3::BoundObject::unbind((value.stretch_x_count).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "stretch_y_count",
        pyo3::BoundObject::unbind((value.stretch_y_count).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "content",
        if !value.has_content {
            py.None()
        } else {
            generated_copy_mln_image_content(py, &value.content)?
        },
    )?;
    dict.set_item(
        "text_fit_width",
        if !value.has_text_fit_width {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.text_fit_width).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "text_fit_height",
        if !value.has_text_fit_height {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.text_fit_height).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "pixel_ratio",
        pyo3::BoundObject::unbind((value.pixel_ratio).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "sdf",
        pyo3::BoundObject::unbind((value.sdf).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_image_options(
    py: Python<'_>,
    value: &sys::mln_style_image_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "stretch_x",
        if value.fields & sys::MLN_STYLE_IMAGE_OPTION_STRETCH_X == 0 {
            py.None()
        } else {
            {
                let items = PyList::empty(py);
                for element in
                    unsafe { generated_slice(value.stretch_x, value.stretch_x_count as usize)? }
                {
                    let item = generated_copy_mln_image_stretch(py, &(*element))?;
                    items.append(item)?;
                }
                items.into_any().unbind()
            }
        },
    )?;
    dict.set_item(
        "stretch_y",
        if value.fields & sys::MLN_STYLE_IMAGE_OPTION_STRETCH_Y == 0 {
            py.None()
        } else {
            {
                let items = PyList::empty(py);
                for element in
                    unsafe { generated_slice(value.stretch_y, value.stretch_y_count as usize)? }
                {
                    let item = generated_copy_mln_image_stretch(py, &(*element))?;
                    items.append(item)?;
                }
                items.into_any().unbind()
            }
        },
    )?;
    dict.set_item(
        "content",
        if value.fields & sys::MLN_STYLE_IMAGE_OPTION_CONTENT == 0 {
            py.None()
        } else {
            generated_copy_mln_image_content(py, &value.content)?
        },
    )?;
    dict.set_item(
        "text_fit_width",
        if value.fields & sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.text_fit_width).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "text_fit_height",
        if value.fields & sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.text_fit_height).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "pixel_ratio",
        if value.fields & sys::MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.pixel_ratio).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "sdf",
        if value.fields & sys::MLN_STYLE_IMAGE_OPTION_SDF == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.sdf).into_pyobject(py)?).into_any()
        },
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
    dict.set_item(
        "pixels",
        PyBytes::new(py, unsafe {
            generated_slice(value.pixels.data.cast::<u8>(), value.pixels.size)?
        })
        .into_any()
        .unbind(),
    )?;
    dict.set_item("stretch_x", {
        let items = PyList::empty(py);
        for element in unsafe { generated_slice(value.stretch_x, value.stretch_x_count as usize)? }
        {
            let item = generated_copy_mln_image_stretch(py, &(*element))?;
            items.append(item)?;
        }
        items.into_any().unbind()
    })?;
    dict.set_item("stretch_y", {
        let items = PyList::empty(py);
        for element in unsafe { generated_slice(value.stretch_y, value.stretch_y_count as usize)? }
        {
            let item = generated_copy_mln_image_stretch(py, &(*element))?;
            items.append(item)?;
        }
        items.into_any().unbind()
    })?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_image_stretches_result(
    py: Python<'_>,
    value: &sys::mln_style_image_stretches_result,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("stretch_x", {
        let items = PyList::empty(py);
        for element in unsafe { generated_slice(value.stretch_x, value.stretch_x_count as usize)? }
        {
            let item = generated_copy_mln_image_stretch(py, &(*element))?;
            items.append(item)?;
        }
        items.into_any().unbind()
    })?;
    dict.set_item("stretch_y", {
        let items = PyList::empty(py);
        for element in unsafe { generated_slice(value.stretch_y, value.stretch_y_count as usize)? }
        {
            let item = generated_copy_mln_image_stretch(py, &(*element))?;
            items.append(item)?;
        }
        items.into_any().unbind()
    })?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_layer_entry(
    py: Python<'_>,
    value: &sys::mln_style_layer_entry,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "id",
        copied_string_view(value.id)?
            .into_pyobject(py)?
            .into_any()
            .unbind(),
    )?;
    dict.set_item(
        "type",
        copied_string_view(value.type_)?
            .into_pyobject(py)?
            .into_any()
            .unbind(),
    )?;
    dict.set_item(
        "source_id",
        if value.source_id.size == 0 {
            py.None()
        } else {
            copied_string_view(value.source_id)?
                .into_pyobject(py)?
                .into_any()
                .unbind()
        },
    )?;
    dict.set_item(
        "source_layer",
        if value.source_layer.size == 0 {
            py.None()
        } else {
            copied_string_view(value.source_layer)?
                .into_pyobject(py)?
                .into_any()
                .unbind()
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_layer_info(
    py: Python<'_>,
    value: &sys::mln_style_layer_info,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "type",
        copied_string_view(value.type_)?
            .into_pyobject(py)?
            .into_any()
            .unbind(),
    )?;
    dict.set_item(
        "min_zoom",
        pyo3::BoundObject::unbind((value.min_zoom).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "max_zoom",
        pyo3::BoundObject::unbind((value.max_zoom).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "visibility",
        pyo3::BoundObject::unbind((value.visibility).into_pyobject(py)?).into_any(),
    )?;
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
        if value.source_id.size == 0 {
            py.None()
        } else {
            copied_string_view(value.source_id)?
                .into_pyobject(py)?
                .into_any()
                .unbind()
        },
    )?;
    dict.set_item(
        "source_layer",
        if value.source_layer.size == 0 {
            py.None()
        } else {
            copied_string_view(value.source_layer)?
                .into_pyobject(py)?
                .into_any()
                .unbind()
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_source_info(
    py: Python<'_>,
    value: &sys::mln_style_source_info,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "type",
        pyo3::BoundObject::unbind((value.type_).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "id_size",
        pyo3::BoundObject::unbind((value.id_size).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "is_volatile",
        pyo3::BoundObject::unbind((value.is_volatile).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "attribution_size",
        if !value.has_attribution {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.attribution_size).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "url_size",
        if value.fields & sys::MLN_STYLE_SOURCE_INFO_URL == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.url_size).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "tilejson",
        if value.fields & sys::MLN_STYLE_SOURCE_INFO_TILEJSON == 0 {
            py.None()
        } else {
            {
                let inner = PyDict::new(py);
                inner.set_item(
                    "tile_count",
                    pyo3::BoundObject::unbind((value.tile_count).into_pyobject(py)?).into_any(),
                )?;
                inner.set_item(
                    "min_zoom",
                    pyo3::BoundObject::unbind((value.min_zoom).into_pyobject(py)?).into_any(),
                )?;
                inner.set_item(
                    "max_zoom",
                    pyo3::BoundObject::unbind((value.max_zoom).into_pyobject(py)?).into_any(),
                )?;
                inner.set_item(
                    "scheme",
                    pyo3::BoundObject::unbind((value.scheme).into_pyobject(py)?).into_any(),
                )?;
                inner.into_any().unbind()
            }
        },
    )?;
    dict.set_item(
        "bounds",
        if value.fields & sys::MLN_STYLE_SOURCE_INFO_BOUNDS == 0 {
            py.None()
        } else {
            generated_copy_mln_lat_lng_bounds(py, &value.bounds)?
        },
    )?;
    dict.set_item(
        "tile_size",
        if value.fields & sys::MLN_STYLE_SOURCE_INFO_TILE_SIZE == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.tile_size).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "vector_encoding",
        if value.fields & sys::MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.vector_encoding).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "raster_encoding",
        if value.fields & sys::MLN_STYLE_SOURCE_INFO_RASTER_ENCODING == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.raster_encoding).into_pyobject(py)?).into_any()
        },
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
        if !value.info.has_attribution {
            py.None()
        } else {
            copied_string_view(value.attribution)?
                .into_pyobject(py)?
                .into_any()
                .unbind()
        },
    )?;
    dict.set_item(
        "url",
        if value.info.fields & sys::MLN_STYLE_SOURCE_INFO_URL == 0 {
            py.None()
        } else {
            copied_string_view(value.url)?
                .into_pyobject(py)?
                .into_any()
                .unbind()
        },
    )?;
    dict.set_item(
        "tile_urls",
        if value.info.fields & sys::MLN_STYLE_SOURCE_INFO_TILEJSON == 0 {
            py.None()
        } else {
            {
                let items = PyList::empty(py);
                for element in
                    unsafe { generated_slice(value.tile_urls, value.tile_url_count as usize)? }
                {
                    let item = copied_string_view(*element)?
                        .into_pyobject(py)?
                        .into_any()
                        .unbind();
                    items.append(item)?;
                }
                items.into_any().unbind()
            }
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_source_tile_urls_result(
    py: Python<'_>,
    value: &sys::mln_style_source_tile_urls_result,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("tile_urls", {
        let items = PyList::empty(py);
        for element in unsafe { generated_slice(value.tile_urls, value.tile_url_count as usize)? } {
            let item = copied_string_view(*element)?
                .into_pyobject(py)?
                .into_any()
                .unbind();
            items.append(item)?;
        }
        items.into_any().unbind()
    })?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_style_tile_source_options(
    py: Python<'_>,
    value: &sys::mln_style_tile_source_options,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "min_zoom",
        if value.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.min_zoom).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "max_zoom",
        if value.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.max_zoom).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "attribution",
        if value.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION == 0 {
            py.None()
        } else {
            copied_string_view(value.attribution)?
                .into_pyobject(py)?
                .into_any()
                .unbind()
        },
    )?;
    dict.set_item(
        "scheme",
        if value.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_SCHEME == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.scheme).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "bounds",
        if value.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS == 0 {
            py.None()
        } else {
            generated_copy_mln_lat_lng_bounds(py, &value.bounds)?
        },
    )?;
    dict.set_item(
        "tile_size",
        if value.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.tile_size).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "vector_encoding",
        if value.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.vector_encoding).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "raster_encoding",
        if value.fields & sys::MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.raster_encoding).into_pyobject(py)?).into_any()
        },
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
        if value.fields & sys::MLN_STYLE_TRANSITION_OPTION_DURATION == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.duration_ms).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "delay_ms",
        if value.fields & sys::MLN_STYLE_TRANSITION_OPTION_DELAY == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.delay_ms).into_pyobject(py)?).into_any()
        },
    )?;
    dict.set_item(
        "enable_placement_transitions",
        if value.fields & sys::MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS == 0 {
            py.None()
        } else {
            pyo3::BoundObject::unbind((value.enable_placement_transitions).into_pyobject(py)?)
                .into_any()
        },
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_texture_image_info(
    py: Python<'_>,
    value: &sys::mln_texture_image_info,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "width",
        pyo3::BoundObject::unbind((value.width).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "height",
        pyo3::BoundObject::unbind((value.height).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "stride",
        pyo3::BoundObject::unbind((value.stride).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "byte_length",
        pyo3::BoundObject::unbind((value.byte_length).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_texture_readback_result(
    py: Python<'_>,
    value: &sys::mln_texture_readback_result,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "data",
        PyBytes::new(py, unsafe {
            generated_slice(value.data.data.cast::<u8>(), value.data.size)?
        })
        .into_any()
        .unbind(),
    )?;
    dict.set_item(
        "info",
        generated_copy_mln_texture_image_info(py, &value.info)?,
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_tile_id(py: Python<'_>, value: &sys::mln_tile_id) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "overscaled_z",
        pyo3::BoundObject::unbind((value.overscaled_z).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "wrap",
        pyo3::BoundObject::unbind((value.wrap).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "canonical_z",
        pyo3::BoundObject::unbind((value.canonical_z).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "canonical_x",
        pyo3::BoundObject::unbind((value.canonical_x).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "canonical_y",
        pyo3::BoundObject::unbind((value.canonical_y).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_unit_bezier(
    py: Python<'_>,
    value: &sys::mln_unit_bezier,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "x1",
        pyo3::BoundObject::unbind((value.x1).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "y1",
        pyo3::BoundObject::unbind((value.y1).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "x2",
        pyo3::BoundObject::unbind((value.x2).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "y2",
        pyo3::BoundObject::unbind((value.y2).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_vec3(py: Python<'_>, value: &sys::mln_vec3) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "x",
        pyo3::BoundObject::unbind((value.x).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "y",
        pyo3::BoundObject::unbind((value.y).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "z",
        pyo3::BoundObject::unbind((value.z).into_pyobject(py)?).into_any(),
    )?;
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
    dict.set_item(
        "physical_width",
        pyo3::BoundObject::unbind((value.physical_width).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "physical_height",
        pyo3::BoundObject::unbind((value.physical_height).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "context",
        generated_copy_mln_vulkan_context_descriptor(py, &value.context)?,
    )?;
    dict.set_item(
        "image",
        pyo3::BoundObject::unbind((value.image).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "image_view",
        pyo3::BoundObject::unbind((value.image_view).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "format",
        pyo3::BoundObject::unbind((value.format).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "initial_layout",
        pyo3::BoundObject::unbind((value.initial_layout).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "final_layout",
        pyo3::BoundObject::unbind((value.final_layout).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_vulkan_context_descriptor(
    py: Python<'_>,
    value: &sys::mln_vulkan_context_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "instance",
        pyo3::BoundObject::unbind((value.instance as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "physical_device",
        pyo3::BoundObject::unbind((value.physical_device as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "device",
        pyo3::BoundObject::unbind((value.device as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "graphics_queue",
        pyo3::BoundObject::unbind((value.graphics_queue as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "graphics_queue_family_index",
        pyo3::BoundObject::unbind((value.graphics_queue_family_index).into_pyobject(py)?)
            .into_any(),
    )?;
    dict.set_item(
        "get_instance_proc_addr",
        pyo3::BoundObject::unbind((value.get_instance_proc_addr as usize).into_pyobject(py)?)
            .into_any(),
    )?;
    dict.set_item(
        "get_device_proc_addr",
        pyo3::BoundObject::unbind((value.get_device_proc_addr as usize).into_pyobject(py)?)
            .into_any(),
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
    dict.set_item(
        "generation",
        pyo3::BoundObject::unbind((value.generation).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "width",
        pyo3::BoundObject::unbind((value.width).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "height",
        pyo3::BoundObject::unbind((value.height).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "scale_factor",
        pyo3::BoundObject::unbind((value.scale_factor).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "frame_id",
        pyo3::BoundObject::unbind((value.frame_id).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "image",
        pyo3::BoundObject::unbind((value.image).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "image_view",
        pyo3::BoundObject::unbind((value.image_view).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "device",
        pyo3::BoundObject::unbind((value.device as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "format",
        pyo3::BoundObject::unbind((value.format).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "layout",
        pyo3::BoundObject::unbind((value.layout).into_pyobject(py)?).into_any(),
    )?;
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
    dict.set_item(
        "surface",
        pyo3::BoundObject::unbind((value.surface).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

unsafe extern "C" fn generated_callback_mln_wake_callback(user_data: *mut std::ffi::c_void) -> () {
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        Python::try_attach(|py| -> PyResult<()> {
            let Some(callback) = (unsafe { generated_get_callback(py, user_data, 0) }) else {
                return Ok(());
            };
            let _result = callback.bind(py).call1(())?;
            Ok(())
        })
        .unwrap_or(Ok(()))
    }));
    match result {
        Ok(Ok(result)) => result,
        Ok(Err(error)) => {
            Python::try_attach(|py| error.write_unraisable(py, None));
            ()
        }
        Err(_) => (),
    }
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
    dict.set_item(
        "kind",
        pyo3::BoundObject::unbind((value.kind).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "context",
        pyo3::BoundObject::unbind((value.context).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "canvas_selector",
        copied_string_view(value.canvas_selector)?
            .into_pyobject(py)?
            .into_any()
            .unbind(),
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
    dict.set_item(
        "physical_width",
        pyo3::BoundObject::unbind((value.physical_width).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "physical_height",
        pyo3::BoundObject::unbind((value.physical_height).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "context",
        generated_copy_mln_webgpu_context_descriptor(py, &value.context)?,
    )?;
    dict.set_item(
        "texture",
        pyo3::BoundObject::unbind((value.texture as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "texture_view",
        pyo3::BoundObject::unbind((value.texture_view as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "format",
        pyo3::BoundObject::unbind((value.format).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_webgpu_context_descriptor(
    py: Python<'_>,
    value: &sys::mln_webgpu_context_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "instance",
        pyo3::BoundObject::unbind((value.instance as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "device",
        pyo3::BoundObject::unbind((value.device as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "queue",
        pyo3::BoundObject::unbind((value.queue as usize).into_pyobject(py)?).into_any(),
    )?;
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
    dict.set_item(
        "generation",
        pyo3::BoundObject::unbind((value.generation).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "width",
        pyo3::BoundObject::unbind((value.width).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "height",
        pyo3::BoundObject::unbind((value.height).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "scale_factor",
        pyo3::BoundObject::unbind((value.scale_factor).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "frame_id",
        pyo3::BoundObject::unbind((value.frame_id).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "texture",
        pyo3::BoundObject::unbind((value.texture as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "texture_view",
        pyo3::BoundObject::unbind((value.texture_view as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "device",
        pyo3::BoundObject::unbind((value.device as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "format",
        pyo3::BoundObject::unbind((value.format).into_pyobject(py)?).into_any(),
    )?;
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
    dict.set_item(
        "surface",
        pyo3::BoundObject::unbind((value.surface as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "format",
        pyo3::BoundObject::unbind((value.format).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_copy_mln_wgl_context_descriptor(
    py: Python<'_>,
    value: &sys::mln_wgl_context_descriptor,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "device_context",
        pyo3::BoundObject::unbind((value.device_context as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "share_context",
        pyo3::BoundObject::unbind((value.share_context as usize).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "get_proc_address",
        pyo3::BoundObject::unbind((value.get_proc_address as usize).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

fn generated_input_mln_animation_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_animation_options> {
    let mut raw: sys::mln_animation_options = unsafe { sys::mln_animation_options_default() };
    raw.size = std::mem::size_of::<sys::mln_animation_options>() as _;
    raw.fields = 0;
    let field = value.getattr("duration_ms")?;
    if !field.is_none() {
        raw.duration_ms = field.extract::<f64>()?;
        raw.fields |= sys::MLN_ANIMATION_OPTION_DURATION;
    }
    let field = value.getattr("velocity")?;
    if !field.is_none() {
        raw.velocity = field.extract::<f64>()?;
        raw.fields |= sys::MLN_ANIMATION_OPTION_VELOCITY;
    }
    let field = value.getattr("min_zoom")?;
    if !field.is_none() {
        raw.min_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_ANIMATION_OPTION_MIN_ZOOM;
    }
    let field = value.getattr("easing")?;
    if !field.is_none() {
        raw.easing = generated_input_mln_unit_bezier(&field, storage)?;
        raw.fields |= sys::MLN_ANIMATION_OPTION_EASING;
    }
    let field = value.getattr("transition_id")?;
    if !field.is_none() {
        raw.transition_id = field.extract::<u64>()?;
        raw.fields |= sys::MLN_ANIMATION_OPTION_TRANSITION_ID;
    }
    Ok(raw)
}

fn generated_input_mln_bound_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_bound_options> {
    let mut raw: sys::mln_bound_options = unsafe { sys::mln_bound_options_default() };
    raw.size = std::mem::size_of::<sys::mln_bound_options>() as _;
    raw.fields = 0;
    let field = value.getattr("bounds")?;
    if !field.is_none() {
        raw.bounds = generated_input_mln_lat_lng_bounds(&field, storage)?;
        raw.fields |= sys::MLN_BOUND_OPTION_BOUNDS;
    }
    let field = value.getattr("min_zoom")?;
    if !field.is_none() {
        raw.min_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_BOUND_OPTION_MIN_ZOOM;
    }
    let field = value.getattr("max_zoom")?;
    if !field.is_none() {
        raw.max_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_BOUND_OPTION_MAX_ZOOM;
    }
    let field = value.getattr("min_pitch")?;
    if !field.is_none() {
        raw.min_pitch = field.extract::<f64>()?;
        raw.fields |= sys::MLN_BOUND_OPTION_MIN_PITCH;
    }
    let field = value.getattr("max_pitch")?;
    if !field.is_none() {
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
    let mut raw: sys::mln_camera_delta = unsafe { sys::mln_camera_delta_default() };
    raw.size = std::mem::size_of::<sys::mln_camera_delta>() as _;
    raw.has_anchor = false;
    let field = value.getattr("kind")?;
    raw.kind = field.extract::<sys::mln_camera_delta_kind>()?;
    let field = value.getattr("offset")?;
    raw.offset = generated_input_mln_screen_point(&field, storage)?;
    let field = value.getattr("amount")?;
    raw.amount = field.extract::<f64>()?;
    let field = value.getattr("anchor")?;
    if !field.is_none() {
        raw.anchor = generated_input_mln_screen_point(&field, storage)?;
        raw.has_anchor = true;
    }
    let field = value.getattr("animation")?;
    raw.animation = if field.is_none() {
        unsafe { sys::mln_animation_options_default() }
    } else {
        generated_input_mln_animation_options(&field, storage)?
    };
    Ok(raw)
}

fn generated_input_mln_camera_fit_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_camera_fit_options> {
    let mut raw: sys::mln_camera_fit_options = unsafe { sys::mln_camera_fit_options_default() };
    raw.size = std::mem::size_of::<sys::mln_camera_fit_options>() as _;
    raw.fields = 0;
    let field = value.getattr("padding")?;
    if !field.is_none() {
        raw.padding = generated_input_mln_edge_insets(&field, storage)?;
        raw.fields |= sys::MLN_CAMERA_FIT_OPTION_PADDING;
    }
    let field = value.getattr("bearing")?;
    if !field.is_none() {
        raw.bearing = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_FIT_OPTION_BEARING;
    }
    let field = value.getattr("pitch")?;
    if !field.is_none() {
        raw.pitch = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_FIT_OPTION_PITCH;
    }
    Ok(raw)
}

fn generated_input_mln_camera_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_camera_options> {
    let mut raw: sys::mln_camera_options = unsafe { sys::mln_camera_options_default() };
    raw.size = std::mem::size_of::<sys::mln_camera_options>() as _;
    raw.fields = 0;
    let field = value.getattr("center")?;
    if !field.is_none() {
        raw.latitude = field.getattr("latitude")?.extract::<f64>()?;
        raw.longitude = field.getattr("longitude")?.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_OPTION_CENTER;
    }
    let field = value.getattr("center_altitude")?;
    if !field.is_none() {
        raw.center_altitude = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_OPTION_CENTER_ALTITUDE;
    }
    let field = value.getattr("padding")?;
    if !field.is_none() {
        raw.padding = generated_input_mln_edge_insets(&field, storage)?;
        raw.fields |= sys::MLN_CAMERA_OPTION_PADDING;
    }
    let field = value.getattr("anchor")?;
    if !field.is_none() {
        raw.anchor = generated_input_mln_screen_point(&field, storage)?;
        raw.fields |= sys::MLN_CAMERA_OPTION_ANCHOR;
    }
    let field = value.getattr("zoom")?;
    if !field.is_none() {
        raw.zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_OPTION_ZOOM;
    }
    let field = value.getattr("bearing")?;
    if !field.is_none() {
        raw.bearing = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_OPTION_BEARING;
    }
    let field = value.getattr("pitch")?;
    if !field.is_none() {
        raw.pitch = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_OPTION_PITCH;
    }
    let field = value.getattr("roll")?;
    if !field.is_none() {
        raw.roll = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_OPTION_ROLL;
    }
    let field = value.getattr("field_of_view")?;
    if !field.is_none() {
        raw.field_of_view = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CAMERA_OPTION_FOV;
    }
    Ok(raw)
}

fn generated_input_mln_camera_update<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_camera_update> {
    let mut raw: sys::mln_camera_update = unsafe { sys::mln_camera_update_default() };
    raw.size = std::mem::size_of::<sys::mln_camera_update>() as _;
    raw.reserved = 0;
    let field = value.getattr("mode")?;
    raw.mode = field.extract::<sys::mln_camera_update_mode>()?;
    let field = value.getattr("camera")?;
    raw.camera = if field.is_none() {
        unsafe { sys::mln_camera_options_default() }
    } else {
        generated_input_mln_camera_options(&field, storage)?
    };
    let field = value.getattr("animation")?;
    raw.animation = if field.is_none() {
        unsafe { sys::mln_animation_options_default() }
    } else {
        generated_input_mln_animation_options(&field, storage)?
    };
    let field = value.getattr("gesture_phase")?;
    raw.gesture_phase = field.extract::<sys::mln_gesture_phase>()?;
    Ok(raw)
}

fn generated_input_mln_canonical_tile_id<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_canonical_tile_id> {
    let _ = storage;
    let mut raw: sys::mln_canonical_tile_id = unsafe { std::mem::zeroed() };
    let field = value.getattr("z")?;
    raw.z = field.extract::<u32>()?;
    let field = value.getattr("x")?;
    raw.x = field.extract::<u32>()?;
    let field = value.getattr("y")?;
    raw.y = field.extract::<u32>()?;
    Ok(raw)
}

fn generated_input_mln_custom_geometry_source_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_custom_geometry_source_options> {
    let mut raw: sys::mln_custom_geometry_source_options =
        unsafe { sys::mln_custom_geometry_source_options_default() };
    raw.size = std::mem::size_of::<sys::mln_custom_geometry_source_options>() as _;
    raw.fields = 0;
    let field = value.getattr("min_zoom")?;
    if !field.is_none() {
        raw.min_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
    }
    let field = value.getattr("max_zoom")?;
    if !field.is_none() {
        raw.max_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
    }
    let field = value.getattr("tolerance")?;
    if !field.is_none() {
        raw.tolerance = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE;
    }
    let field = value.getattr("tile_size")?;
    if !field.is_none() {
        raw.tile_size = field.extract::<u32>()?;
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE;
    }
    let field = value.getattr("buffer")?;
    if !field.is_none() {
        raw.buffer = field.extract::<u32>()?;
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER;
    }
    let field = value.getattr("clip")?;
    if !field.is_none() {
        raw.clip = field.extract::<bool>()?;
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP;
    }
    let field = value.getattr("wrap")?;
    if !field.is_none() {
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
    let mut raw: sys::mln_custom_mvt_vector_source_options =
        unsafe { sys::mln_custom_mvt_vector_source_options_default() };
    raw.size = std::mem::size_of::<sys::mln_custom_mvt_vector_source_options>() as _;
    raw.fields = 0;
    let field = value.getattr("min_zoom")?;
    if !field.is_none() {
        raw.min_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM;
    }
    let field = value.getattr("max_zoom")?;
    if !field.is_none() {
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
    let field = value.getattr("top")?;
    raw.top = field.extract::<f64>()?;
    let field = value.getattr("left")?;
    raw.left = field.extract::<f64>()?;
    let field = value.getattr("bottom")?;
    raw.bottom = field.extract::<f64>()?;
    let field = value.getattr("right")?;
    raw.right = field.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_egl_context_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_egl_context_descriptor> {
    let _ = storage;
    let mut raw: sys::mln_egl_context_descriptor = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_egl_context_descriptor>() as _;
    let field = value.getattr("display")?;
    raw.display = field.extract::<usize>()? as _;
    let field = value.getattr("config")?;
    raw.config = field.extract::<usize>()? as _;
    let field = value.getattr("share_context")?;
    raw.share_context = field.extract::<usize>()? as _;
    let field = value.getattr("client_api")?;
    raw.client_api = field.extract::<sys::mln_opengl_client_api>()?;
    let field = value.getattr("get_proc_address")?;
    raw.get_proc_address = field.extract::<usize>()? as _;
    Ok(raw)
}

fn generated_input_mln_feature_state_selector<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_feature_state_selector> {
    let mut raw: sys::mln_feature_state_selector = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_feature_state_selector>() as _;
    raw.fields = 0;
    let field = value.getattr("source_id")?;
    raw.source_id = storage.buffer(field, true)?;
    let field = value.getattr("source_layer_id")?;
    if !field.is_none() {
        raw.source_layer_id = storage.buffer(field, true)?;
        raw.fields |= sys::MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID;
    }
    let field = value.getattr("feature_id")?;
    if !field.is_none() {
        raw.feature_id = storage.buffer(field, true)?;
        raw.fields |= sys::MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
    }
    let field = value.getattr("state_key")?;
    if !field.is_none() {
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
    let mut raw: sys::mln_frame_demand = unsafe { sys::mln_frame_demand_default() };
    raw.size = std::mem::size_of::<sys::mln_frame_demand>() as _;
    let field = value.getattr("flags")?;
    raw.flags = field.extract::<sys::mln_frame_demand_flag>()?;
    let field = value.getattr("token")?;
    raw.token = field.extract::<u64>()?;
    let field = value.getattr("coalescing_boundary")?;
    raw.coalescing_boundary = field.extract::<u64>()?;
    let field = value.getattr("timeout_ns")?;
    raw.timeout_ns = field.extract::<u64>()?;
    Ok(raw)
}

fn generated_input_mln_free_camera_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_free_camera_options> {
    let mut raw: sys::mln_free_camera_options = unsafe { sys::mln_free_camera_options_default() };
    raw.size = std::mem::size_of::<sys::mln_free_camera_options>() as _;
    raw.fields = 0;
    let field = value.getattr("position")?;
    if !field.is_none() {
        raw.position = generated_input_mln_vec3(&field, storage)?;
        raw.fields |= sys::MLN_FREE_CAMERA_OPTION_POSITION;
    }
    let field = value.getattr("orientation")?;
    if !field.is_none() {
        raw.orientation = generated_input_mln_quaternion(&field, storage)?;
        raw.fields |= sys::MLN_FREE_CAMERA_OPTION_ORIENTATION;
    }
    Ok(raw)
}

fn generated_input_mln_geojson_source_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_geojson_source_options> {
    let mut raw: sys::mln_geojson_source_options =
        unsafe { sys::mln_geojson_source_options_default() };
    raw.size = std::mem::size_of::<sys::mln_geojson_source_options>() as _;
    raw.fields = 0;
    let field = value.getattr("min_zoom")?;
    if !field.is_none() {
        raw.min_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM;
    }
    let field = value.getattr("max_zoom")?;
    if !field.is_none() {
        raw.max_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM;
    }
    let field = value.getattr("tolerance")?;
    if !field.is_none() {
        raw.tolerance = field.extract::<f64>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_TOLERANCE;
    }
    let field = value.getattr("cluster_max_zoom")?;
    if !field.is_none() {
        raw.cluster_max_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM;
    }
    let field = value.getattr("cluster_properties")?;
    if !field.is_none() {
        raw.cluster_properties = storage.buffer(field, false)?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
    }
    let field = value.getattr("tile_size")?;
    if !field.is_none() {
        raw.tile_size = field.extract::<u32>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE;
    }
    let field = value.getattr("buffer")?;
    if !field.is_none() {
        raw.buffer = field.extract::<u32>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_BUFFER;
    }
    let field = value.getattr("cluster_radius")?;
    if !field.is_none() {
        raw.cluster_radius = field.extract::<u32>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS;
    }
    let field = value.getattr("cluster_min_points")?;
    if !field.is_none() {
        raw.cluster_min_points = field.extract::<u32>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS;
    }
    let field = value.getattr("line_metrics")?;
    if !field.is_none() {
        raw.line_metrics = field.extract::<bool>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS;
    }
    let field = value.getattr("cluster")?;
    if !field.is_none() {
        raw.cluster = field.extract::<bool>()?;
        raw.fields |= sys::MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
    }
    let field = value.getattr("synchronous_tiling")?;
    if !field.is_none() {
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
    let mut raw: sys::mln_gpu_sync = unsafe { sys::mln_gpu_sync_default() };
    raw.size = std::mem::size_of::<sys::mln_gpu_sync>() as _;
    let field = value.getattr("kind")?;
    raw.kind = field.extract::<sys::mln_gpu_sync_kind>()?;
    let field = value.getattr("object")?;
    raw.object = field.extract::<u64>()?;
    let field = value.getattr("value")?;
    raw.value = field.extract::<u64>()?;
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
    let field = value.getattr("left")?;
    raw.left = field.extract::<f32>()?;
    let field = value.getattr("top")?;
    raw.top = field.extract::<f32>()?;
    let field = value.getattr("right")?;
    raw.right = field.extract::<f32>()?;
    let field = value.getattr("bottom")?;
    raw.bottom = field.extract::<f32>()?;
    Ok(raw)
}

fn generated_input_mln_image_stretch<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_image_stretch> {
    let _ = storage;
    let mut raw: sys::mln_image_stretch = unsafe { std::mem::zeroed() };
    let field = value.getattr("from_")?;
    raw.from = field.extract::<f32>()?;
    let field = value.getattr("to")?;
    raw.to = field.extract::<f32>()?;
    Ok(raw)
}

fn generated_input_mln_lat_lng<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_lat_lng> {
    let _ = storage;
    let mut raw: sys::mln_lat_lng = unsafe { std::mem::zeroed() };
    let field = value.getattr("latitude")?;
    raw.latitude = field.extract::<f64>()?;
    let field = value.getattr("longitude")?;
    raw.longitude = field.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_lat_lng_bounds<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_lat_lng_bounds> {
    let mut raw: sys::mln_lat_lng_bounds = unsafe { std::mem::zeroed() };
    let field = value.getattr("southwest")?;
    raw.southwest = generated_input_mln_lat_lng(&field, storage)?;
    let field = value.getattr("northeast")?;
    raw.northeast = generated_input_mln_lat_lng(&field, storage)?;
    Ok(raw)
}

fn generated_input_mln_logical_extent<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_logical_extent> {
    let _ = storage;
    let mut raw: sys::mln_logical_extent = unsafe { std::mem::zeroed() };
    let field = value.getattr("width")?;
    raw.width = field.extract::<u32>()?;
    let field = value.getattr("height")?;
    raw.height = field.extract::<u32>()?;
    let field = value.getattr("scale_factor")?;
    raw.scale_factor = field.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_map_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_map_options> {
    let mut raw: sys::mln_map_options = unsafe { sys::mln_map_options_default() };
    raw.size = std::mem::size_of::<sys::mln_map_options>() as _;
    let field = value.getattr("initial_extent")?;
    raw.initial_extent = generated_input_mln_logical_extent(&field, storage)?;
    let field = value.getattr("map_mode")?;
    raw.map_mode = field.extract::<sys::mln_map_mode>()?;
    let field = value.getattr("fast_pfor_enabled")?;
    raw.fast_pfor_enabled = field.extract::<bool>()?;
    let field = value.getattr("event_mask")?;
    raw.event_mask = field.extract::<sys::mln_runtime_event_mask>()?;
    Ok(raw)
}

fn generated_input_mln_map_tile_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_map_tile_options> {
    let _ = storage;
    let mut raw: sys::mln_map_tile_options = unsafe { sys::mln_map_tile_options_default() };
    raw.size = std::mem::size_of::<sys::mln_map_tile_options>() as _;
    raw.fields = 0;
    let field = value.getattr("prefetch_zoom_delta")?;
    if !field.is_none() {
        raw.prefetch_zoom_delta = field.extract::<u32>()?;
        raw.fields |= sys::MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
    }
    let field = value.getattr("lod_min_radius")?;
    if !field.is_none() {
        raw.lod_min_radius = field.extract::<f64>()?;
        raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS;
    }
    let field = value.getattr("lod_scale")?;
    if !field.is_none() {
        raw.lod_scale = field.extract::<f64>()?;
        raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_SCALE;
    }
    let field = value.getattr("lod_pitch_threshold")?;
    if !field.is_none() {
        raw.lod_pitch_threshold = field.extract::<f64>()?;
        raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD;
    }
    let field = value.getattr("lod_zoom_shift")?;
    if !field.is_none() {
        raw.lod_zoom_shift = field.extract::<f64>()?;
        raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT;
    }
    let field = value.getattr("lod_mode")?;
    if !field.is_none() {
        raw.lod_mode = field.extract::<sys::mln_tile_lod_mode>()?;
        raw.fields |= sys::MLN_MAP_TILE_OPTION_LOD_MODE;
    }
    Ok(raw)
}

fn generated_input_mln_map_viewport_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_map_viewport_options> {
    let mut raw: sys::mln_map_viewport_options = unsafe { sys::mln_map_viewport_options_default() };
    raw.size = std::mem::size_of::<sys::mln_map_viewport_options>() as _;
    raw.fields = 0;
    let field = value.getattr("north_orientation")?;
    if !field.is_none() {
        raw.north_orientation = field.extract::<sys::mln_north_orientation>()?;
        raw.fields |= sys::MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
    }
    let field = value.getattr("constrain_mode")?;
    if !field.is_none() {
        raw.constrain_mode = field.extract::<sys::mln_constrain_mode>()?;
        raw.fields |= sys::MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
    }
    let field = value.getattr("viewport_mode")?;
    if !field.is_none() {
        raw.viewport_mode = field.extract::<sys::mln_viewport_mode>()?;
        raw.fields |= sys::MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
    }
    let field = value.getattr("frustum_offset")?;
    if !field.is_none() {
        raw.frustum_offset = generated_input_mln_edge_insets(&field, storage)?;
        raw.fields |= sys::MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
    }
    Ok(raw)
}

fn generated_input_mln_metal_borrowed_texture_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_metal_borrowed_texture_descriptor> {
    let mut raw: sys::mln_metal_borrowed_texture_descriptor =
        unsafe { sys::mln_metal_borrowed_texture_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_metal_borrowed_texture_descriptor>() as _;
    let field = value.getattr("extent")?;
    raw.extent = generated_input_mln_render_target_extent(&field, storage)?;
    let field = value.getattr("physical_width")?;
    raw.physical_width = field.extract::<u32>()?;
    let field = value.getattr("physical_height")?;
    raw.physical_height = field.extract::<u32>()?;
    let field = value.getattr("texture")?;
    raw.texture = field.extract::<usize>()? as _;
    Ok(raw)
}

fn generated_input_mln_metal_context_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_metal_context_descriptor> {
    let _ = storage;
    let mut raw: sys::mln_metal_context_descriptor = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_metal_context_descriptor>() as _;
    let field = value.getattr("device")?;
    raw.device = field.extract::<usize>()? as _;
    Ok(raw)
}

fn generated_input_mln_metal_owned_texture_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_metal_owned_texture_descriptor> {
    let mut raw: sys::mln_metal_owned_texture_descriptor =
        unsafe { sys::mln_metal_owned_texture_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_metal_owned_texture_descriptor>() as _;
    let field = value.getattr("extent")?;
    raw.extent = generated_input_mln_render_target_extent(&field, storage)?;
    let field = value.getattr("context")?;
    raw.context = generated_input_mln_metal_context_descriptor(&field, storage)?;
    Ok(raw)
}

fn generated_input_mln_metal_surface_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_metal_surface_descriptor> {
    let mut raw: sys::mln_metal_surface_descriptor =
        unsafe { sys::mln_metal_surface_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_metal_surface_descriptor>() as _;
    let field = value.getattr("extent")?;
    raw.extent = generated_input_mln_render_target_extent(&field, storage)?;
    let field = value.getattr("context")?;
    raw.context = generated_input_mln_metal_context_descriptor(&field, storage)?;
    let field = value.getattr("layer")?;
    raw.layer = field.extract::<usize>()? as _;
    Ok(raw)
}

fn generated_input_mln_offline_geometry_region_definition<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_offline_geometry_region_definition> {
    let mut raw: sys::mln_offline_geometry_region_definition = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_offline_geometry_region_definition>() as _;
    let field = value.getattr("style_url")?;
    raw.style_url = storage.c_string(field)?;
    let field = value.getattr("geometry")?;
    raw.geometry = storage.buffer(field, false)?;
    let field = value.getattr("min_zoom")?;
    raw.min_zoom = field.extract::<f64>()?;
    let field = value.getattr("max_zoom")?;
    raw.max_zoom = field.extract::<f64>()?;
    let field = value.getattr("pixel_ratio")?;
    raw.pixel_ratio = field.extract::<f32>()?;
    let field = value.getattr("include_ideographs")?;
    raw.include_ideographs = field.extract::<bool>()?;
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
    let field = value.getattr("style_url")?;
    raw.style_url = storage.c_string(field)?;
    let field = value.getattr("bounds")?;
    raw.bounds = generated_input_mln_lat_lng_bounds(&field, storage)?;
    let field = value.getattr("min_zoom")?;
    raw.min_zoom = field.extract::<f64>()?;
    let field = value.getattr("max_zoom")?;
    raw.max_zoom = field.extract::<f64>()?;
    let field = value.getattr("pixel_ratio")?;
    raw.pixel_ratio = field.extract::<f32>()?;
    let field = value.getattr("include_ideographs")?;
    raw.include_ideographs = field.extract::<bool>()?;
    Ok(raw)
}

fn generated_input_mln_opengl_borrowed_texture_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_opengl_borrowed_texture_descriptor> {
    let mut raw: sys::mln_opengl_borrowed_texture_descriptor =
        unsafe { sys::mln_opengl_borrowed_texture_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_opengl_borrowed_texture_descriptor>() as _;
    let field = value.getattr("extent")?;
    raw.extent = generated_input_mln_render_target_extent(&field, storage)?;
    let field = value.getattr("physical_width")?;
    raw.physical_width = field.extract::<u32>()?;
    let field = value.getattr("physical_height")?;
    raw.physical_height = field.extract::<u32>()?;
    let field = value.getattr("context")?;
    raw.context = generated_input_mln_opengl_context_descriptor(&field, storage)?;
    let field = value.getattr("texture")?;
    raw.texture = field.extract::<u32>()?;
    let field = value.getattr("target")?;
    raw.target = field.extract::<u32>()?;
    Ok(raw)
}

fn generated_input_mln_opengl_context_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_opengl_context_descriptor> {
    let mut raw: sys::mln_opengl_context_descriptor = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_opengl_context_descriptor>() as _;
    let field = value.getattr("ownership")?;
    raw.ownership = field.extract::<sys::mln_opengl_context_ownership>()?;
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
    let mut raw: sys::mln_opengl_owned_texture_descriptor =
        unsafe { sys::mln_opengl_owned_texture_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_opengl_owned_texture_descriptor>() as _;
    let field = value.getattr("extent")?;
    raw.extent = generated_input_mln_render_target_extent(&field, storage)?;
    let field = value.getattr("context")?;
    raw.context = generated_input_mln_opengl_context_descriptor(&field, storage)?;
    Ok(raw)
}

fn generated_input_mln_opengl_surface_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_opengl_surface_descriptor> {
    let mut raw: sys::mln_opengl_surface_descriptor =
        unsafe { sys::mln_opengl_surface_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_opengl_surface_descriptor>() as _;
    let field = value.getattr("extent")?;
    raw.extent = generated_input_mln_render_target_extent(&field, storage)?;
    let field = value.getattr("context")?;
    raw.context = generated_input_mln_opengl_context_descriptor(&field, storage)?;
    let field = value.getattr("surface")?;
    raw.surface = field.extract::<usize>()? as _;
    Ok(raw)
}

fn generated_input_mln_premultiplied_rgba8_image<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_premultiplied_rgba8_image> {
    let mut raw: sys::mln_premultiplied_rgba8_image =
        unsafe { sys::mln_premultiplied_rgba8_image_default() };
    raw.size = std::mem::size_of::<sys::mln_premultiplied_rgba8_image>() as _;
    let field = value.getattr("width")?;
    raw.width = field.extract::<u32>()?;
    let field = value.getattr("height")?;
    raw.height = field.extract::<u32>()?;
    let field = value.getattr("stride")?;
    raw.stride = field.extract::<u32>()?;
    let field = value.getattr("pixels")?;
    raw.pixels = {
        let buffer = storage.buffer(field, false)?;
        raw.byte_length = buffer.size.try_into().map_err(|_| {
            pyo3::exceptions::PyOverflowError::new_err("buffer length exceeds native count")
        })?;
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
    let field = value.getattr("northing")?;
    raw.northing = field.extract::<f64>()?;
    let field = value.getattr("easting")?;
    raw.easting = field.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_projection_mode<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_projection_mode> {
    let _ = storage;
    let mut raw: sys::mln_projection_mode = unsafe { sys::mln_projection_mode_default() };
    raw.size = std::mem::size_of::<sys::mln_projection_mode>() as _;
    raw.fields = 0;
    let field = value.getattr("axonometric")?;
    if !field.is_none() {
        raw.axonometric = field.extract::<bool>()?;
        raw.fields |= sys::MLN_PROJECTION_MODE_AXONOMETRIC;
    }
    let field = value.getattr("x_skew")?;
    if !field.is_none() {
        raw.x_skew = field.extract::<f64>()?;
        raw.fields |= sys::MLN_PROJECTION_MODE_X_SKEW;
    }
    let field = value.getattr("y_skew")?;
    if !field.is_none() {
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
    let field = value.getattr("x")?;
    raw.x = field.extract::<f64>()?;
    let field = value.getattr("y")?;
    raw.y = field.extract::<f64>()?;
    let field = value.getattr("z")?;
    raw.z = field.extract::<f64>()?;
    let field = value.getattr("w")?;
    raw.w = field.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_render_session_attach_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_render_session_attach_options> {
    let mut raw: sys::mln_render_session_attach_options =
        unsafe { sys::mln_render_session_attach_options_default() };
    raw.size = std::mem::size_of::<sys::mln_render_session_attach_options>() as _;
    raw.reserved = 0;
    let field = value.getattr("driver")?;
    raw.driver = field.extract::<sys::mln_render_driver_kind>()?;
    let field = value.getattr("requested_texture_ring_depth")?;
    raw.requested_texture_ring_depth = field.extract::<u32>()?;
    let field = value.getattr("frame_wake")?;
    raw.frame_wake = generated_input_mln_wake(&field, storage)?;
    let field = value.getattr("driver_work_wake")?;
    raw.driver_work_wake = generated_input_mln_wake(&field, storage)?;
    Ok(raw)
}

fn generated_input_mln_render_target_extent<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_render_target_extent> {
    let _ = storage;
    let mut raw: sys::mln_render_target_extent = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_render_target_extent>() as _;
    let field = value.getattr("width")?;
    raw.width = field.extract::<u32>()?;
    let field = value.getattr("height")?;
    raw.height = field.extract::<u32>()?;
    let field = value.getattr("scale_factor")?;
    raw.scale_factor = field.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_rendered_feature_query_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_rendered_feature_query_options> {
    let mut raw: sys::mln_rendered_feature_query_options =
        unsafe { sys::mln_rendered_feature_query_options_default() };
    raw.size = std::mem::size_of::<sys::mln_rendered_feature_query_options>() as _;
    raw.fields = 0;
    let field = value.getattr("layer_ids")?;
    if !field.is_none() {
        raw.layer_ids = {
            let mut items = Vec::new();
            for item in field.try_iter()? {
                let item = item?;
                items.push(storage.buffer(item, true)?);
            }
            raw.layer_id_count = items.len().try_into().map_err(|_| {
                pyo3::exceptions::PyOverflowError::new_err("array length exceeds native count")
            })?;
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
    let field = value.getattr("status")?;
    raw.status = field.extract::<sys::mln_resource_response_status>()?;
    let field = value.getattr("error_reason")?;
    raw.error_reason = field.extract::<sys::mln_resource_error_reason>()?;
    let field = value.getattr("bytes")?;
    raw.bytes = {
        let buffer = storage.buffer(field, false)?;
        raw.byte_count = buffer.size.try_into().map_err(|_| {
            pyo3::exceptions::PyOverflowError::new_err("buffer length exceeds native count")
        })?;
        buffer.data.cast()
    };
    let field = value.getattr("error_message")?;
    raw.error_message = if field.is_none() {
        std::ptr::null()
    } else {
        storage.c_string(field)?
    };
    let field = value.getattr("must_revalidate")?;
    raw.must_revalidate = field.extract::<bool>()?;
    let field = value.getattr("modified_unix_ms")?;
    if !field.is_none() {
        raw.modified_unix_ms = field.extract::<i64>()?;
        raw.has_modified = true;
    }
    let field = value.getattr("expires_unix_ms")?;
    if !field.is_none() {
        raw.expires_unix_ms = field.extract::<i64>()?;
        raw.has_expires = true;
    }
    let field = value.getattr("etag")?;
    raw.etag = if field.is_none() {
        std::ptr::null()
    } else {
        storage.c_string(field)?
    };
    let field = value.getattr("retry_after_unix_ms")?;
    if !field.is_none() {
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
    let mut raw: sys::mln_runtime_options = unsafe { sys::mln_runtime_options_default() };
    raw.size = std::mem::size_of::<sys::mln_runtime_options>() as _;
    let field = value.getattr("flags")?;
    raw.flags = field.extract::<u32>()?;
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
    let field = value.getattr("event_mask")?;
    raw.event_mask = field.extract::<sys::mln_runtime_event_mask>()?;
    let field = value.getattr("event_wake")?;
    raw.event_wake = generated_input_mln_wake(&field, storage)?;
    Ok(raw)
}

fn generated_input_mln_screen_box<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_screen_box> {
    let mut raw: sys::mln_screen_box = unsafe { std::mem::zeroed() };
    let field = value.getattr("min")?;
    raw.min = generated_input_mln_screen_point(&field, storage)?;
    let field = value.getattr("max")?;
    raw.max = generated_input_mln_screen_point(&field, storage)?;
    Ok(raw)
}

fn generated_input_mln_screen_line_string<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_screen_line_string> {
    let mut raw: sys::mln_screen_line_string = unsafe { std::mem::zeroed() };
    let field = value.getattr("points")?;
    raw.points = {
        let mut items = Vec::new();
        for item in field.try_iter()? {
            let item = item?;
            items.push(generated_input_mln_screen_point(&item, storage)?);
        }
        raw.point_count = items.len().try_into().map_err(|_| {
            pyo3::exceptions::PyOverflowError::new_err("array length exceeds native count")
        })?;
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
    let field = value.getattr("x")?;
    raw.x = field.extract::<f64>()?;
    let field = value.getattr("y")?;
    raw.y = field.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_source_feature_query_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_source_feature_query_options> {
    let mut raw: sys::mln_source_feature_query_options =
        unsafe { sys::mln_source_feature_query_options_default() };
    raw.size = std::mem::size_of::<sys::mln_source_feature_query_options>() as _;
    raw.fields = 0;
    let field = value.getattr("source_layer_ids")?;
    if !field.is_none() {
        raw.source_layer_ids = {
            let mut items = Vec::new();
            for item in field.try_iter()? {
                let item = item?;
                items.push(storage.buffer(item, true)?);
            }
            raw.source_layer_id_count = items.len().try_into().map_err(|_| {
                pyo3::exceptions::PyOverflowError::new_err("array length exceeds native count")
            })?;
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
    let mut raw: sys::mln_style_image_options = unsafe { sys::mln_style_image_options_default() };
    raw.size = std::mem::size_of::<sys::mln_style_image_options>() as _;
    raw.fields = 0;
    let field = value.getattr("stretch_x")?;
    if !field.is_none() {
        raw.stretch_x = {
            let mut items = Vec::new();
            for item in field.try_iter()? {
                let item = item?;
                items.push(generated_input_mln_image_stretch(&item, storage)?);
            }
            raw.stretch_x_count = items.len().try_into().map_err(|_| {
                pyo3::exceptions::PyOverflowError::new_err("array length exceeds native count")
            })?;
            storage.keep_array(items)
        };
        raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_STRETCH_X;
    }
    let field = value.getattr("stretch_y")?;
    if !field.is_none() {
        raw.stretch_y = {
            let mut items = Vec::new();
            for item in field.try_iter()? {
                let item = item?;
                items.push(generated_input_mln_image_stretch(&item, storage)?);
            }
            raw.stretch_y_count = items.len().try_into().map_err(|_| {
                pyo3::exceptions::PyOverflowError::new_err("array length exceeds native count")
            })?;
            storage.keep_array(items)
        };
        raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
    }
    let field = value.getattr("content")?;
    if !field.is_none() {
        raw.content = generated_input_mln_image_content(&field, storage)?;
        raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_CONTENT;
    }
    let field = value.getattr("text_fit_width")?;
    if !field.is_none() {
        raw.text_fit_width = field.extract::<sys::mln_style_image_text_fit>()?;
        raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH;
    }
    let field = value.getattr("text_fit_height")?;
    if !field.is_none() {
        raw.text_fit_height = field.extract::<sys::mln_style_image_text_fit>()?;
        raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
    }
    let field = value.getattr("pixel_ratio")?;
    if !field.is_none() {
        raw.pixel_ratio = field.extract::<f32>()?;
        raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
    }
    let field = value.getattr("sdf")?;
    if !field.is_none() {
        raw.sdf = field.extract::<bool>()?;
        raw.fields |= sys::MLN_STYLE_IMAGE_OPTION_SDF;
    }
    Ok(raw)
}

fn generated_input_mln_style_tile_source_options<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_style_tile_source_options> {
    let mut raw: sys::mln_style_tile_source_options =
        unsafe { sys::mln_style_tile_source_options_default() };
    raw.size = std::mem::size_of::<sys::mln_style_tile_source_options>() as _;
    raw.fields = 0;
    let field = value.getattr("min_zoom")?;
    if !field.is_none() {
        raw.min_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM;
    }
    let field = value.getattr("max_zoom")?;
    if !field.is_none() {
        raw.max_zoom = field.extract::<f64>()?;
        raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM;
    }
    let field = value.getattr("attribution")?;
    if !field.is_none() {
        raw.attribution = storage.buffer(field, true)?;
        raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
    }
    let field = value.getattr("scheme")?;
    if !field.is_none() {
        raw.scheme = field.extract::<sys::mln_style_tile_scheme>()?;
        raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_SCHEME;
    }
    let field = value.getattr("bounds")?;
    if !field.is_none() {
        raw.bounds = generated_input_mln_lat_lng_bounds(&field, storage)?;
        raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS;
    }
    let field = value.getattr("tile_size")?;
    if !field.is_none() {
        raw.tile_size = field.extract::<u32>()?;
        raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE;
    }
    let field = value.getattr("vector_encoding")?;
    if !field.is_none() {
        raw.vector_encoding = field.extract::<sys::mln_style_vector_tile_encoding>()?;
        raw.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
    }
    let field = value.getattr("raster_encoding")?;
    if !field.is_none() {
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
    let mut raw: sys::mln_style_transition_options =
        unsafe { sys::mln_style_transition_options_default() };
    raw.size = std::mem::size_of::<sys::mln_style_transition_options>() as _;
    raw.fields = 0;
    let field = value.getattr("duration_ms")?;
    if !field.is_none() {
        raw.duration_ms = field.extract::<f64>()?;
        raw.fields |= sys::MLN_STYLE_TRANSITION_OPTION_DURATION;
    }
    let field = value.getattr("delay_ms")?;
    if !field.is_none() {
        raw.delay_ms = field.extract::<f64>()?;
        raw.fields |= sys::MLN_STYLE_TRANSITION_OPTION_DELAY;
    }
    let field = value.getattr("enable_placement_transitions")?;
    if !field.is_none() {
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
    let field = value.getattr("x1")?;
    raw.x1 = field.extract::<f64>()?;
    let field = value.getattr("y1")?;
    raw.y1 = field.extract::<f64>()?;
    let field = value.getattr("x2")?;
    raw.x2 = field.extract::<f64>()?;
    let field = value.getattr("y2")?;
    raw.y2 = field.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_vec3<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_vec3> {
    let _ = storage;
    let mut raw: sys::mln_vec3 = unsafe { std::mem::zeroed() };
    let field = value.getattr("x")?;
    raw.x = field.extract::<f64>()?;
    let field = value.getattr("y")?;
    raw.y = field.extract::<f64>()?;
    let field = value.getattr("z")?;
    raw.z = field.extract::<f64>()?;
    Ok(raw)
}

fn generated_input_mln_vulkan_borrowed_texture_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_vulkan_borrowed_texture_descriptor> {
    let mut raw: sys::mln_vulkan_borrowed_texture_descriptor =
        unsafe { sys::mln_vulkan_borrowed_texture_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_vulkan_borrowed_texture_descriptor>() as _;
    let field = value.getattr("extent")?;
    raw.extent = generated_input_mln_render_target_extent(&field, storage)?;
    let field = value.getattr("physical_width")?;
    raw.physical_width = field.extract::<u32>()?;
    let field = value.getattr("physical_height")?;
    raw.physical_height = field.extract::<u32>()?;
    let field = value.getattr("context")?;
    raw.context = generated_input_mln_vulkan_context_descriptor(&field, storage)?;
    let field = value.getattr("image")?;
    raw.image = field.extract::<u64>()?;
    let field = value.getattr("image_view")?;
    raw.image_view = field.extract::<u64>()?;
    let field = value.getattr("format")?;
    raw.format = field.extract::<u32>()?;
    let field = value.getattr("initial_layout")?;
    raw.initial_layout = field.extract::<u32>()?;
    let field = value.getattr("final_layout")?;
    raw.final_layout = field.extract::<u32>()?;
    Ok(raw)
}

fn generated_input_mln_vulkan_context_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_vulkan_context_descriptor> {
    let _ = storage;
    let mut raw: sys::mln_vulkan_context_descriptor = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_vulkan_context_descriptor>() as _;
    let field = value.getattr("instance")?;
    raw.instance = field.extract::<usize>()? as _;
    let field = value.getattr("physical_device")?;
    raw.physical_device = field.extract::<usize>()? as _;
    let field = value.getattr("device")?;
    raw.device = field.extract::<usize>()? as _;
    let field = value.getattr("graphics_queue")?;
    raw.graphics_queue = field.extract::<usize>()? as _;
    let field = value.getattr("graphics_queue_family_index")?;
    raw.graphics_queue_family_index = field.extract::<u32>()?;
    let field = value.getattr("get_instance_proc_addr")?;
    raw.get_instance_proc_addr = field.extract::<usize>()? as _;
    let field = value.getattr("get_device_proc_addr")?;
    raw.get_device_proc_addr = field.extract::<usize>()? as _;
    Ok(raw)
}

fn generated_input_mln_vulkan_owned_texture_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_vulkan_owned_texture_descriptor> {
    let mut raw: sys::mln_vulkan_owned_texture_descriptor =
        unsafe { sys::mln_vulkan_owned_texture_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_vulkan_owned_texture_descriptor>() as _;
    let field = value.getattr("extent")?;
    raw.extent = generated_input_mln_render_target_extent(&field, storage)?;
    let field = value.getattr("context")?;
    raw.context = generated_input_mln_vulkan_context_descriptor(&field, storage)?;
    Ok(raw)
}

fn generated_input_mln_vulkan_surface_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_vulkan_surface_descriptor> {
    let mut raw: sys::mln_vulkan_surface_descriptor =
        unsafe { sys::mln_vulkan_surface_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_vulkan_surface_descriptor>() as _;
    let field = value.getattr("extent")?;
    raw.extent = generated_input_mln_render_target_extent(&field, storage)?;
    let field = value.getattr("context")?;
    raw.context = generated_input_mln_vulkan_context_descriptor(&field, storage)?;
    let field = value.getattr("surface")?;
    raw.surface = field.extract::<u64>()?;
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
    let field = value.getattr("kind")?;
    raw.kind = field.extract::<sys::mln_webgl_context_kind>()?;
    let field = value.getattr("context")?;
    raw.context = field.extract::<i32>()?;
    let field = value.getattr("canvas_selector")?;
    raw.canvas_selector = storage.buffer(field, true)?;
    Ok(raw)
}

fn generated_input_mln_webgpu_borrowed_texture_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_webgpu_borrowed_texture_descriptor> {
    let mut raw: sys::mln_webgpu_borrowed_texture_descriptor =
        unsafe { sys::mln_webgpu_borrowed_texture_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_webgpu_borrowed_texture_descriptor>() as _;
    let field = value.getattr("extent")?;
    raw.extent = generated_input_mln_render_target_extent(&field, storage)?;
    let field = value.getattr("physical_width")?;
    raw.physical_width = field.extract::<u32>()?;
    let field = value.getattr("physical_height")?;
    raw.physical_height = field.extract::<u32>()?;
    let field = value.getattr("context")?;
    raw.context = generated_input_mln_webgpu_context_descriptor(&field, storage)?;
    let field = value.getattr("texture")?;
    raw.texture = field.extract::<usize>()? as _;
    let field = value.getattr("texture_view")?;
    raw.texture_view = field.extract::<usize>()? as _;
    let field = value.getattr("format")?;
    raw.format = field.extract::<u32>()?;
    Ok(raw)
}

fn generated_input_mln_webgpu_context_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_webgpu_context_descriptor> {
    let _ = storage;
    let mut raw: sys::mln_webgpu_context_descriptor = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_webgpu_context_descriptor>() as _;
    let field = value.getattr("instance")?;
    raw.instance = field.extract::<usize>()? as _;
    let field = value.getattr("device")?;
    raw.device = field.extract::<usize>()? as _;
    let field = value.getattr("queue")?;
    raw.queue = field.extract::<usize>()? as _;
    Ok(raw)
}

fn generated_input_mln_webgpu_owned_texture_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_webgpu_owned_texture_descriptor> {
    let mut raw: sys::mln_webgpu_owned_texture_descriptor =
        unsafe { sys::mln_webgpu_owned_texture_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_webgpu_owned_texture_descriptor>() as _;
    let field = value.getattr("extent")?;
    raw.extent = generated_input_mln_render_target_extent(&field, storage)?;
    let field = value.getattr("context")?;
    raw.context = generated_input_mln_webgpu_context_descriptor(&field, storage)?;
    Ok(raw)
}

fn generated_input_mln_webgpu_surface_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_webgpu_surface_descriptor> {
    let mut raw: sys::mln_webgpu_surface_descriptor =
        unsafe { sys::mln_webgpu_surface_descriptor_default() };
    raw.size = std::mem::size_of::<sys::mln_webgpu_surface_descriptor>() as _;
    let field = value.getattr("extent")?;
    raw.extent = generated_input_mln_render_target_extent(&field, storage)?;
    let field = value.getattr("context")?;
    raw.context = generated_input_mln_webgpu_context_descriptor(&field, storage)?;
    let field = value.getattr("surface")?;
    raw.surface = field.extract::<usize>()? as _;
    let field = value.getattr("format")?;
    raw.format = field.extract::<u32>()?;
    Ok(raw)
}

fn generated_input_mln_wgl_context_descriptor<'py>(
    value: &Bound<'py, PyAny>,
    storage: &mut GeneratedInputStorage<'py>,
) -> PyResult<sys::mln_wgl_context_descriptor> {
    let _ = storage;
    let mut raw: sys::mln_wgl_context_descriptor = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_wgl_context_descriptor>() as _;
    let field = value.getattr("device_context")?;
    raw.device_context = field.extract::<usize>()? as _;
    let field = value.getattr("share_context")?;
    raw.share_context = field.extract::<usize>()? as _;
    let field = value.getattr("get_proc_address")?;
    raw.get_proc_address = field.extract::<usize>()? as _;
    Ok(raw)
}
#[pyfunction]
fn _default_animation_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_animation_options_default() };
    Ok(generated_copy_mln_animation_options(py, &value)?)
}

#[pyfunction]
fn _default_bound_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_bound_options_default() };
    Ok(generated_copy_mln_bound_options(py, &value)?)
}

#[pyfunction]
fn _default_camera_delta(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_camera_delta_default() };
    Ok(generated_copy_mln_camera_delta(py, &value)?)
}

#[pyfunction]
fn _default_camera_fit_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_camera_fit_options_default() };
    Ok(generated_copy_mln_camera_fit_options(py, &value)?)
}

#[pyfunction]
fn _default_camera_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_camera_options_default() };
    Ok(generated_copy_mln_camera_options(py, &value)?)
}

#[pyfunction]
fn _default_camera_update(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_camera_update_default() };
    Ok(generated_copy_mln_camera_update(py, &value)?)
}

#[pyfunction]
fn _default_custom_geometry_source_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_custom_geometry_source_options_default() };
    Ok(generated_copy_mln_custom_geometry_source_options(
        py, &value,
    )?)
}

#[pyfunction]
fn _default_custom_mvt_vector_source_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_custom_mvt_vector_source_options_default() };
    Ok(generated_copy_mln_custom_mvt_vector_source_options(
        py, &value,
    )?)
}

#[pyfunction]
fn _default_frame_demand(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_frame_demand_default() };
    Ok(generated_copy_mln_frame_demand(py, &value)?)
}

#[pyfunction]
fn _default_free_camera_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_free_camera_options_default() };
    Ok(generated_copy_mln_free_camera_options(py, &value)?)
}

#[pyfunction]
fn _default_geojson_source_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_geojson_source_options_default() };
    Ok(generated_copy_mln_geojson_source_options(py, &value)?)
}

#[pyfunction]
fn _default_gpu_sync(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_gpu_sync_default() };
    Ok(generated_copy_mln_gpu_sync(py, &value)?)
}

#[pyfunction]
fn _default_map_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_map_options_default() };
    Ok(generated_copy_mln_map_options(py, &value)?)
}

#[pyfunction]
fn _default_map_tile_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_map_tile_options_default() };
    Ok(generated_copy_mln_map_tile_options(py, &value)?)
}

#[pyfunction]
fn _default_map_viewport_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_map_viewport_options_default() };
    Ok(generated_copy_mln_map_viewport_options(py, &value)?)
}

#[pyfunction]
fn _default_metal_borrowed_texture_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_metal_borrowed_texture_descriptor_default() };
    Ok(generated_copy_mln_metal_borrowed_texture_descriptor(
        py, &value,
    )?)
}

#[pyfunction]
fn _default_metal_owned_texture_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_metal_owned_texture_descriptor_default() };
    Ok(generated_copy_mln_metal_owned_texture_descriptor(
        py, &value,
    )?)
}

#[pyfunction]
fn _default_metal_surface_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_metal_surface_descriptor_default() };
    Ok(generated_copy_mln_metal_surface_descriptor(py, &value)?)
}

#[pyfunction]
fn _default_opengl_borrowed_texture_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_opengl_borrowed_texture_descriptor_default() };
    Ok(generated_copy_mln_opengl_borrowed_texture_descriptor(
        py, &value,
    )?)
}

#[pyfunction]
fn _default_opengl_owned_texture_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_opengl_owned_texture_descriptor_default() };
    Ok(generated_copy_mln_opengl_owned_texture_descriptor(
        py, &value,
    )?)
}

#[pyfunction]
fn _default_opengl_surface_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_opengl_surface_descriptor_default() };
    Ok(generated_copy_mln_opengl_surface_descriptor(py, &value)?)
}

#[pyfunction]
fn _default_premultiplied_rgba8_image(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_premultiplied_rgba8_image_default() };
    Ok(generated_copy_mln_premultiplied_rgba8_image(py, &value)?)
}

#[pyfunction]
fn _default_projection_mode(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_projection_mode_default() };
    Ok(generated_copy_mln_projection_mode(py, &value)?)
}

#[pyfunction]
fn _default_render_session_attach_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_render_session_attach_options_default() };
    Ok(generated_copy_mln_render_session_attach_options(
        py, &value,
    )?)
}

#[pyfunction]
fn _default_rendered_feature_query_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_rendered_feature_query_options_default() };
    Ok(generated_copy_mln_rendered_feature_query_options(
        py, &value,
    )?)
}

#[pyfunction]
fn _default_runtime_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_runtime_options_default() };
    Ok(generated_copy_mln_runtime_options(py, &value)?)
}

#[pyfunction]
fn _default_source_feature_query_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_source_feature_query_options_default() };
    Ok(generated_copy_mln_source_feature_query_options(py, &value)?)
}

#[pyfunction]
fn _default_style_image_info(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_style_image_info_default() };
    Ok(generated_copy_mln_style_image_info(py, &value)?)
}

#[pyfunction]
fn _default_style_image_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_style_image_options_default() };
    Ok(generated_copy_mln_style_image_options(py, &value)?)
}

#[pyfunction]
fn _default_style_tile_source_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_style_tile_source_options_default() };
    Ok(generated_copy_mln_style_tile_source_options(py, &value)?)
}

#[pyfunction]
fn _default_style_transition_options(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_style_transition_options_default() };
    Ok(generated_copy_mln_style_transition_options(py, &value)?)
}

#[pyfunction]
fn _default_texture_image_info(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_texture_image_info_default() };
    Ok(generated_copy_mln_texture_image_info(py, &value)?)
}

#[pyfunction]
fn _default_vulkan_borrowed_texture_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_vulkan_borrowed_texture_descriptor_default() };
    Ok(generated_copy_mln_vulkan_borrowed_texture_descriptor(
        py, &value,
    )?)
}

#[pyfunction]
fn _default_vulkan_owned_texture_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_vulkan_owned_texture_descriptor_default() };
    Ok(generated_copy_mln_vulkan_owned_texture_descriptor(
        py, &value,
    )?)
}

#[pyfunction]
fn _default_vulkan_surface_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_vulkan_surface_descriptor_default() };
    Ok(generated_copy_mln_vulkan_surface_descriptor(py, &value)?)
}

#[pyfunction]
fn _default_webgpu_borrowed_texture_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_webgpu_borrowed_texture_descriptor_default() };
    Ok(generated_copy_mln_webgpu_borrowed_texture_descriptor(
        py, &value,
    )?)
}

#[pyfunction]
fn _default_webgpu_owned_texture_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_webgpu_owned_texture_descriptor_default() };
    Ok(generated_copy_mln_webgpu_owned_texture_descriptor(
        py, &value,
    )?)
}

#[pyfunction]
fn _default_webgpu_surface_descriptor(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_reentry()?;
    let value = unsafe { sys::mln_webgpu_surface_descriptor_default() };
    Ok(generated_copy_mln_webgpu_surface_descriptor(py, &value)?)
}

unsafe extern "C" fn generated_dispose_mln_acquired_frame(
    handle: sys::mln_acquired_frame,
) -> sys::mln_status {
    unsafe { sys::mln_acquired_frame_dispose(handle) }
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
    unsafe { sys::mln_map_dispose(handle) }
}

unsafe extern "C" fn generated_dispose_mln_map_projection(
    handle: sys::mln_map_projection,
) -> sys::mln_status {
    unsafe { sys::mln_map_projection_close(handle) }
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
    unsafe { sys::mln_render_session_dispose(handle) }
}

unsafe extern "C" fn generated_dispose_mln_runtime(handle: sys::mln_runtime) -> sys::mln_status {
    unsafe { sys::mln_runtime_dispose(handle) }
}

#[pymethods]
impl AcquiredFrameHandle {
    #[pyo3(signature = ())]
    fn with_metal_texture(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_acquired_frame_get_metal_texture",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_frame: sys::mln_metal_owned_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_metal_owned_texture_frame>() as _;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_acquired_frame_get_metal_texture(handle, &mut out_frame)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(generated_copy_mln_metal_owned_texture_frame(
            py, &out_frame,
        )?)
    }
    #[pyo3(signature = ())]
    fn with_opengl_texture(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_acquired_frame_get_opengl_texture",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_frame: sys::mln_opengl_owned_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_opengl_owned_texture_frame>() as _;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_acquired_frame_get_opengl_texture(handle, &mut out_frame)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(generated_copy_mln_opengl_owned_texture_frame(
            py, &out_frame,
        )?)
    }
    #[pyo3(signature = ())]
    fn with_producer_sync(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_acquired_frame_get_producer_sync",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_sync: sys::mln_gpu_sync = unsafe { sys::mln_gpu_sync_default() };
        out_sync.size = std::mem::size_of::<sys::mln_gpu_sync>() as _;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_acquired_frame_get_producer_sync(handle, &mut out_sync)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(generated_copy_mln_gpu_sync(py, &out_sync)?)
    }
    #[pyo3(signature = ())]
    fn get_result(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_acquired_frame_get_result",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_result: sys::mln_render_frame_result = unsafe { std::mem::zeroed() };
        out_result.size = std::mem::size_of::<sys::mln_render_frame_result>() as _;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_acquired_frame_get_result(handle, &mut out_result)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(generated_copy_mln_render_frame_result(py, &out_result)?)
    }
    #[pyo3(signature = ())]
    fn with_vulkan_texture(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_acquired_frame_get_vulkan_texture",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_frame: sys::mln_vulkan_owned_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_vulkan_owned_texture_frame>() as _;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_acquired_frame_get_vulkan_texture(handle, &mut out_frame)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(generated_copy_mln_vulkan_owned_texture_frame(
            py, &out_frame,
        )?)
    }
    #[pyo3(signature = ())]
    fn with_webgpu_texture(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_acquired_frame_get_webgpu_texture",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_frame: sys::mln_webgpu_owned_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_webgpu_owned_texture_frame>() as _;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_acquired_frame_get_webgpu_texture(handle, &mut out_frame)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(generated_copy_mln_webgpu_owned_texture_frame(
            py, &out_frame,
        )?)
    }
    #[pyo3(signature = (consumer_completion=None))]
    fn close(
        &self,
        py: Python<'_>,
        consumer_completion: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_acquired_frame_release",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let consumer_completion = consumer_completion.unwrap_or_else(|| py.None().into_bound(py));
        let consumer_completion_value = if consumer_completion.clone().is_none() {
            unsafe { sys::mln_gpu_sync_default() }
        } else {
            generated_input_mln_gpu_sync(&consumer_completion.clone(), storage)?
        };
        let Some(mut reservation) = GeneratedHandleReservation::new(&self.state)? else {
            return Ok(py.None());
        };
        let mut handle = reservation.handle();
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_acquired_frame_release(&mut handle, &consumer_completion_value)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        reservation.commit();
        Ok(py.None())
    }
}

#[pymethods]
impl BufferHandle {
    #[pyo3(signature = ())]
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_buffer_destroy",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let Some(mut reservation) = GeneratedHandleReservation::new(&self.state)? else {
            return Ok(py.None());
        };
        let handle = reservation.handle();
        unsafe { generated_native_call(py, || sys::mln_buffer_destroy(handle)) };
        reservation.commit();
        Ok(py.None())
    }
    #[pyo3(signature = ())]
    fn get(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_buffer_get",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let read = GeneratedReadReservation::new(&self.state)?;
        let handle = read.handle;
        let mut out_view: sys::mln_buffer_view = unsafe { std::mem::zeroed() };
        let result =
            unsafe { generated_native_call(py, || sys::mln_buffer_get(handle, &mut out_view)) };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(PyBytes::new(py, unsafe {
            generated_slice(out_view.data.cast::<u8>(), out_view.size)?
        })
        .into_any()
        .unbind())
    }
}

#[pymethods]
impl EventBatchHandle {
    #[pyo3(signature = ())]
    fn get(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_event_batch_get",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let read = GeneratedReadReservation::new(&self.state)?;
        let handle = read.handle;
        let mut out_view: sys::mln_runtime_event_batch_view = unsafe { std::mem::zeroed() };
        out_view.size = std::mem::size_of::<sys::mln_runtime_event_batch_view>() as _;
        let result = unsafe {
            generated_native_call(py, || sys::mln_event_batch_get(handle, &mut out_view))
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(generated_copy_mln_runtime_event_batch_view(py, &out_view)?)
    }
    #[pyo3(signature = ())]
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_event_batch_release",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let Some(mut reservation) = GeneratedHandleReservation::new(&self.state)? else {
            return Ok(py.None());
        };
        let handle = reservation.handle();
        unsafe { generated_native_call(py, || sys::mln_event_batch_release(handle)) };
        reservation.commit();
        Ok(py.None())
    }
}

#[pymethods]
impl GeojsonSourceDataHandle {
    #[pyo3(signature = ())]
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_geojson_source_data_destroy",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let Some(mut reservation) = GeneratedHandleReservation::new(&self.state)? else {
            return Ok(py.None());
        };
        let handle = reservation.handle();
        unsafe { generated_native_call(py, || sys::mln_geojson_source_data_destroy(handle)) };
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation("mln_http_header_transform_response_set", self.native as u64)?;
        let name_view = storage.buffer(name.clone(), true)?;
        let value_view = storage.buffer(value.clone(), true)?;
        let handle =
            self.scope.pointer(self.native)? as *mut sys::mln_http_header_transform_response;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_http_header_transform_response_set(
                    handle,
                    name_view.data.cast(),
                    name_view.size,
                    value_view.data.cast(),
                    value_view.size,
                )
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_color_relief_layer",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let before_layer_id = before_layer_id.unwrap_or_else(|| py.None().into_bound(py));
        let before_layer_id_value = storage.buffer(before_layer_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_color_relief_layer(
                    handle,
                    layer_id_value,
                    source_id_value,
                    before_layer_id_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, options=None))]
    fn add_custom_geometry_source(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_custom_geometry_source",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_custom_geometry_source_options_default() }
        } else {
            generated_input_mln_custom_geometry_source_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let future = submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_custom_geometry_source(
                    handle,
                    source_id_value,
                    &options_value,
                    completion,
                )
            })
        })?;
        let callback_roots = storage.accept_callbacks();
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_custom_mvt_vector_source",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_custom_mvt_vector_source_options_default() }
        } else {
            generated_input_mln_custom_mvt_vector_source_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let future = submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_custom_mvt_vector_source(
                    handle,
                    source_id_value,
                    &options_value,
                    completion,
                )
            })
        })?;
        let callback_roots = storage.accept_callbacks();
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_geojson_source_data",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let data_handle = data
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("input handle is closed"))?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_geojson_source_data(
                    handle,
                    source_id_value,
                    data_handle,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, url, options=None))]
    fn add_geojson_source_url(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        url: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_geojson_source_url",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let url_value = storage.buffer(url.clone(), true)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.is_none() {
            None
        } else {
            Some(if options.clone().is_none() {
                unsafe { sys::mln_geojson_source_options_default() }
            } else {
                generated_input_mln_geojson_source_options(&options.clone(), storage)?
            })
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_geojson_source_url(
                    handle,
                    source_id_value,
                    url_value,
                    options_value
                        .as_ref()
                        .map_or(std::ptr::null(), |value| value),
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (layer_id, source_id, before_layer_id=None))]
    fn add_hillshade_layer(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        source_id: &Bound<'_, PyAny>,
        before_layer_id: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_hillshade_layer",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let before_layer_id = before_layer_id.unwrap_or_else(|| py.None().into_bound(py));
        let before_layer_id_value = storage.buffer(before_layer_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_hillshade_layer(
                    handle,
                    layer_id_value,
                    source_id_value,
                    before_layer_id_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, coordinates, image=None))]
    fn add_image_source_image(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        coordinates: &Bound<'_, PyAny>,
        image: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_image_source_image",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let mut coordinates_values = Vec::new();
        for item in coordinates.try_iter()? {
            let item = item?;
            coordinates_values.push(generated_input_mln_lat_lng(&item, storage)?);
        }
        let image = image.unwrap_or_else(|| py.None().into_bound(py));
        let image_value = if image.clone().is_none() {
            unsafe { sys::mln_premultiplied_rgba8_image_default() }
        } else {
            generated_input_mln_premultiplied_rgba8_image(&image.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_image_source_image(
                    handle,
                    source_id_value,
                    coordinates_values.as_ptr(),
                    coordinates_values.len(),
                    &image_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, coordinates, url))]
    fn add_image_source_url(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        coordinates: &Bound<'_, PyAny>,
        url: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_image_source_url",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let mut coordinates_values = Vec::new();
        for item in coordinates.try_iter()? {
            let item = item?;
            coordinates_values.push(generated_input_mln_lat_lng(&item, storage)?);
        }
        let url_value = storage.buffer(url.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_image_source_url(
                    handle,
                    source_id_value,
                    coordinates_values.as_ptr(),
                    coordinates_values.len(),
                    url_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (layer_id, before_layer_id=None))]
    fn add_location_indicator_layer(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        before_layer_id: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_location_indicator_layer",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let before_layer_id = before_layer_id.unwrap_or_else(|| py.None().into_bound(py));
        let before_layer_id_value = storage.buffer(before_layer_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_location_indicator_layer(
                    handle,
                    layer_id_value,
                    before_layer_id_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, tiles, options=None))]
    fn add_raster_dem_source_tiles(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        tiles: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_raster_dem_source_tiles",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let mut tiles_values = Vec::new();
        for item in tiles.try_iter()? {
            let item = item?;
            tiles_values.push(storage.buffer(item, true)?);
        }
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.is_none() {
            None
        } else {
            Some(if options.clone().is_none() {
                unsafe { sys::mln_style_tile_source_options_default() }
            } else {
                generated_input_mln_style_tile_source_options(&options.clone(), storage)?
            })
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_raster_dem_source_tiles(
                    handle,
                    source_id_value,
                    tiles_values.as_ptr(),
                    tiles_values.len(),
                    options_value
                        .as_ref()
                        .map_or(std::ptr::null(), |value| value),
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, url, options=None))]
    fn add_raster_dem_source_url(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        url: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_raster_dem_source_url",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let url_value = storage.buffer(url.clone(), true)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.is_none() {
            None
        } else {
            Some(if options.clone().is_none() {
                unsafe { sys::mln_style_tile_source_options_default() }
            } else {
                generated_input_mln_style_tile_source_options(&options.clone(), storage)?
            })
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_raster_dem_source_url(
                    handle,
                    source_id_value,
                    url_value,
                    options_value
                        .as_ref()
                        .map_or(std::ptr::null(), |value| value),
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, tiles, options=None))]
    fn add_raster_source_tiles(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        tiles: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_raster_source_tiles",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let mut tiles_values = Vec::new();
        for item in tiles.try_iter()? {
            let item = item?;
            tiles_values.push(storage.buffer(item, true)?);
        }
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.is_none() {
            None
        } else {
            Some(if options.clone().is_none() {
                unsafe { sys::mln_style_tile_source_options_default() }
            } else {
                generated_input_mln_style_tile_source_options(&options.clone(), storage)?
            })
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_raster_source_tiles(
                    handle,
                    source_id_value,
                    tiles_values.as_ptr(),
                    tiles_values.len(),
                    options_value
                        .as_ref()
                        .map_or(std::ptr::null(), |value| value),
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, url, options=None))]
    fn add_raster_source_url(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        url: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_raster_source_url",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let url_value = storage.buffer(url.clone(), true)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.is_none() {
            None
        } else {
            Some(if options.clone().is_none() {
                unsafe { sys::mln_style_tile_source_options_default() }
            } else {
                generated_input_mln_style_tile_source_options(&options.clone(), storage)?
            })
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_raster_source_url(
                    handle,
                    source_id_value,
                    url_value,
                    options_value
                        .as_ref()
                        .map_or(std::ptr::null(), |value| value),
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (layer_json, before_layer_id=None))]
    fn add_style_layer_json(
        &self,
        py: Python<'_>,
        layer_json: &Bound<'_, PyAny>,
        before_layer_id: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_style_layer_json",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_json_value = storage.buffer(layer_json.clone(), false)?;
        let before_layer_id = before_layer_id.unwrap_or_else(|| py.None().into_bound(py));
        let before_layer_id_value = storage.buffer(before_layer_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_style_layer_json(
                    handle,
                    layer_json_value,
                    before_layer_id_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, source_json))]
    fn add_style_source_json(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        source_json: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_style_source_json",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let source_json_value = storage.buffer(source_json.clone(), false)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_style_source_json(
                    handle,
                    source_id_value,
                    source_json_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, tiles, options=None))]
    fn add_vector_source_tiles(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        tiles: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_vector_source_tiles",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let mut tiles_values = Vec::new();
        for item in tiles.try_iter()? {
            let item = item?;
            tiles_values.push(storage.buffer(item, true)?);
        }
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.is_none() {
            None
        } else {
            Some(if options.clone().is_none() {
                unsafe { sys::mln_style_tile_source_options_default() }
            } else {
                generated_input_mln_style_tile_source_options(&options.clone(), storage)?
            })
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_vector_source_tiles(
                    handle,
                    source_id_value,
                    tiles_values.as_ptr(),
                    tiles_values.len(),
                    options_value
                        .as_ref()
                        .map_or(std::ptr::null(), |value| value),
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, url, options=None))]
    fn add_vector_source_url(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        url: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_add_vector_source_url",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let url_value = storage.buffer(url.clone(), true)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.is_none() {
            None
        } else {
            Some(if options.clone().is_none() {
                unsafe { sys::mln_style_tile_source_options_default() }
            } else {
                generated_input_mln_style_tile_source_options(&options.clone(), storage)?
            })
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_add_vector_source_url(
                    handle,
                    source_id_value,
                    url_value,
                    options_value
                        .as_ref()
                        .map_or(std::ptr::null(), |value| value),
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (delta=None))]
    fn apply_camera_delta(
        &self,
        py: Python<'_>,
        delta: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_apply_camera_delta",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let delta = delta.unwrap_or_else(|| py.None().into_bound(py));
        let delta_value = if delta.clone().is_none() {
            unsafe { sys::mln_camera_delta_default() }
        } else {
            generated_input_mln_camera_delta(&delta.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_apply_camera_delta(handle, &delta_value, completion)
            })
        })
    }
    #[pyo3(signature = (geometry, fit_options=None))]
    fn camera_for_geometry(
        &self,
        py: Python<'_>,
        geometry: &Bound<'_, PyAny>,
        fit_options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_camera_for_geometry",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let geometry_value = storage.buffer(geometry.clone(), false)?;
        let fit_options = fit_options.unwrap_or_else(|| py.None().into_bound(py));
        let fit_options_value = if fit_options.is_none() {
            None
        } else {
            Some(if fit_options.clone().is_none() {
                unsafe { sys::mln_camera_fit_options_default() }
            } else {
                generated_input_mln_camera_fit_options(&fit_options.clone(), storage)?
            })
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_camera_for_geometry(
                        handle,
                        geometry_value,
                        fit_options_value
                            .as_ref()
                            .map_or(std::ptr::null(), |value| value),
                        completion,
                    )
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_camera_options>(result)?;
                Ok(generated_copy_mln_camera_options(py, &value)?)
            },
        )
    }
    #[pyo3(signature = (bounds, fit_options=None))]
    fn camera_for_lat_lng_bounds(
        &self,
        py: Python<'_>,
        bounds: &Bound<'_, PyAny>,
        fit_options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_camera_for_lat_lng_bounds",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let bounds_value = generated_input_mln_lat_lng_bounds(&bounds.clone(), storage)?;
        let fit_options = fit_options.unwrap_or_else(|| py.None().into_bound(py));
        let fit_options_value = if fit_options.is_none() {
            None
        } else {
            Some(if fit_options.clone().is_none() {
                unsafe { sys::mln_camera_fit_options_default() }
            } else {
                generated_input_mln_camera_fit_options(&fit_options.clone(), storage)?
            })
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_camera_for_lat_lng_bounds(
                        handle,
                        bounds_value,
                        fit_options_value
                            .as_ref()
                            .map_or(std::ptr::null(), |value| value),
                        completion,
                    )
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_camera_options>(result)?;
                Ok(generated_copy_mln_camera_options(py, &value)?)
            },
        )
    }
    #[pyo3(signature = (coordinates, fit_options=None))]
    fn camera_for_lat_lngs(
        &self,
        py: Python<'_>,
        coordinates: &Bound<'_, PyAny>,
        fit_options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_camera_for_lat_lngs",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let mut coordinates_values = Vec::new();
        for item in coordinates.try_iter()? {
            let item = item?;
            coordinates_values.push(generated_input_mln_lat_lng(&item, storage)?);
        }
        let fit_options = fit_options.unwrap_or_else(|| py.None().into_bound(py));
        let fit_options_value = if fit_options.is_none() {
            None
        } else {
            Some(if fit_options.clone().is_none() {
                unsafe { sys::mln_camera_fit_options_default() }
            } else {
                generated_input_mln_camera_fit_options(&fit_options.clone(), storage)?
            })
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_camera_for_lat_lngs(
                        handle,
                        coordinates_values.as_ptr(),
                        coordinates_values.len(),
                        fit_options_value
                            .as_ref()
                            .map_or(std::ptr::null(), |value| value),
                        completion,
                    )
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_camera_options>(result)?;
                Ok(generated_copy_mln_camera_options(py, &value)?)
            },
        )
    }
    #[pyo3(signature = ())]
    fn camera_query(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_camera_query",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || sys::mln_map_camera_query(handle, completion))
            },
            |py, result| {
                let value = completion_value::<sys::mln_camera_query_result>(result)?;
                Ok(generated_copy_mln_camera_query_result(py, &value)?)
            },
        )
    }
    #[pyo3(signature = ())]
    fn camera_snapshot_get(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_camera_snapshot_get",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_camera: sys::mln_camera_options = unsafe { sys::mln_camera_options_default() };
        out_camera.size = std::mem::size_of::<sys::mln_camera_options>() as _;
        let mut out_generation: u64 = unsafe { std::mem::zeroed() };
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_map_camera_snapshot_get(handle, &mut out_camera, &mut out_generation)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        let dict = PyDict::new(py);
        dict.set_item(
            "camera",
            generated_copy_mln_camera_options(py, &out_camera)?,
        )?;
        dict.set_item(
            "generation",
            pyo3::BoundObject::unbind((out_generation).into_pyobject(py)?).into_any(),
        )?;
        Ok(dict.into_any().unbind())
    }
    #[pyo3(signature = ())]
    fn cancel_transitions(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_cancel_transitions",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || sys::mln_map_cancel_transitions(handle, completion))
        })
    }
    #[pyo3(signature = (layer_id))]
    fn copy_layer_source_id(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_copy_layer_source_id",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_copy_layer_source_id(handle, layer_id_value, completion)
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_buffer_view>(result)?;
                Ok(if value.size == 0 {
                    py.None()
                } else {
                    copied_string_view(value)?
                        .into_pyobject(py)?
                        .into_any()
                        .unbind()
                })
            },
        )
    }
    #[pyo3(signature = (layer_id))]
    fn copy_layer_source_layer(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_copy_layer_source_layer",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_copy_layer_source_layer(handle, layer_id_value, completion)
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_buffer_view>(result)?;
                Ok(if value.size == 0 {
                    py.None()
                } else {
                    copied_string_view(value)?
                        .into_pyobject(py)?
                        .into_any()
                        .unbind()
                })
            },
        )
    }
    #[pyo3(signature = (image_id))]
    fn copy_style_image_premultiplied_rgba8(
        &self,
        py: Python<'_>,
        image_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_copy_style_image_premultiplied_rgba8",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let image_id_value = storage.buffer(image_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_copy_style_image_premultiplied_rgba8(
                        handle,
                        image_id_value,
                        completion,
                    )
                })
            },
            |py, result| {
                if result.value.is_null() {
                    return Ok(py.None());
                }
                let value = completion_value::<sys::mln_buffer_view>(result)?;
                Ok(if value.data.is_null() {
                    py.None()
                } else {
                    PyBytes::new(py, unsafe {
                        generated_slice(value.data.cast::<u8>(), value.size)?
                    })
                    .into_any()
                    .unbind()
                })
            },
        )
    }
    #[pyo3(signature = (image_id))]
    fn copy_style_image_stretches(
        &self,
        py: Python<'_>,
        image_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_copy_style_image_stretches",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let image_id_value = storage.buffer(image_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_copy_style_image_stretches(handle, image_id_value, completion)
                })
            },
            |py, result| {
                if result.value.is_null() {
                    return Ok(py.None());
                }
                let value = completion_value::<sys::mln_style_image_stretches_result>(result)?;
                Ok(generated_copy_mln_style_image_stretches_result(py, &value)?)
            },
        )
    }
    #[pyo3(signature = (source_id))]
    fn copy_style_source_attribution(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_copy_style_source_attribution",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_copy_style_source_attribution(handle, source_id_value, completion)
                })
            },
            |py, result| {
                if result.value.is_null() {
                    return Ok(py.None());
                }
                let value = completion_value::<sys::mln_buffer_view>(result)?;
                Ok(if value.data.is_null() {
                    py.None()
                } else {
                    copied_string_view(value)?
                        .into_pyobject(py)?
                        .into_any()
                        .unbind()
                })
            },
        )
    }
    #[pyo3(signature = (source_id))]
    fn copy_style_source_url(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_copy_style_source_url",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_copy_style_source_url(handle, source_id_value, completion)
                })
            },
            |py, result| {
                if result.value.is_null() {
                    return Ok(py.None());
                }
                let value = completion_value::<sys::mln_buffer_view>(result)?;
                Ok(if value.data.is_null() {
                    py.None()
                } else {
                    copied_string_view(value)?
                        .into_pyobject(py)?
                        .into_any()
                        .unbind()
                })
            },
        )
    }
    #[pyo3(signature = ())]
    fn dump_debug_logs(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_dump_debug_logs",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || sys::mln_map_dump_debug_logs(handle, completion))
        })
    }
    #[pyo3(signature = (selector))]
    fn get_feature_state(
        &self,
        py: Python<'_>,
        selector: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_get_feature_state",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let selector_value =
            generated_input_mln_feature_state_selector(&selector.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_get_feature_state(handle, &selector_value, completion)
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_buffer_view>(result)?;
                Ok(PyBytes::new(py, unsafe {
                    generated_slice(value.data.cast::<u8>(), value.size)?
                })
                .into_any()
                .unbind())
            },
        )
    }
    #[pyo3(signature = ())]
    fn get_global_state(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_get_global_state",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || sys::mln_map_get_global_state(handle, completion))
            },
            |py, result| {
                let value = completion_value::<sys::mln_buffer_view>(result)?;
                Ok(PyBytes::new(py, unsafe {
                    generated_slice(value.data.cast::<u8>(), value.size)?
                })
                .into_any()
                .unbind())
            },
        )
    }
    #[pyo3(signature = (source_id))]
    fn get_image_source_coordinates(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_get_image_source_coordinates",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_get_image_source_coordinates(handle, source_id_value, completion)
                })
            },
            |py, result| {
                if result.value.is_null() {
                    return Ok(py.None());
                }
                let list = PyList::empty(py);
                for value in generated_completion_slice::<sys::mln_lat_lng>(result)? {
                    list.append(generated_copy_mln_lat_lng(py, &(*value))?)?;
                }
                Ok(list.into_any().unbind())
            },
        )
    }
    #[pyo3(signature = (layer_id))]
    fn get_layer_filter(&self, py: Python<'_>, layer_id: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_get_layer_filter",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_get_layer_filter(handle, layer_id_value, completion)
                })
            },
            |py, result| {
                if result.value.is_null() {
                    return Ok(py.None());
                }
                let value = completion_value::<sys::mln_buffer_view>(result)?;
                Ok(if value.data.is_null() {
                    py.None()
                } else {
                    PyBytes::new(py, unsafe {
                        generated_slice(value.data.cast::<u8>(), value.size)?
                    })
                    .into_any()
                    .unbind()
                })
            },
        )
    }
    #[pyo3(signature = (layer_id, property_name))]
    fn get_layer_property(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        property_name: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_get_layer_property",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let property_name_value = storage.buffer(property_name.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_get_layer_property(
                        handle,
                        layer_id_value,
                        property_name_value,
                        completion,
                    )
                })
            },
            |py, result| {
                if result.value.is_null() {
                    return Ok(py.None());
                }
                let value = completion_value::<sys::mln_buffer_view>(result)?;
                Ok(if value.data.is_null() {
                    py.None()
                } else {
                    PyBytes::new(py, unsafe {
                        generated_slice(value.data.cast::<u8>(), value.size)?
                    })
                    .into_any()
                    .unbind()
                })
            },
        )
    }
    #[pyo3(signature = (image_id))]
    fn get_style_image_info(
        &self,
        py: Python<'_>,
        image_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_get_style_image_info",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let image_id_value = storage.buffer(image_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_get_style_image_info(handle, image_id_value, completion)
                })
            },
            |py, result| {
                if result.value.is_null() {
                    return Ok(py.None());
                }
                let value = completion_value::<sys::mln_style_image_result>(result)?;
                Ok(generated_copy_mln_style_image_result(py, &value)?)
            },
        )
    }
    #[pyo3(signature = (layer_id))]
    fn get_style_layer_info(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_get_style_layer_info",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_get_style_layer_info(handle, layer_id_value, completion)
                })
            },
            |py, result| {
                if result.value.is_null() {
                    return Ok(py.None());
                }
                let value = completion_value::<sys::mln_style_layer_result>(result)?;
                Ok(generated_copy_mln_style_layer_result(py, &value)?)
            },
        )
    }
    #[pyo3(signature = (layer_id))]
    fn get_style_layer_json(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_get_style_layer_json",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_get_style_layer_json(handle, layer_id_value, completion)
                })
            },
            |py, result| {
                if result.value.is_null() {
                    return Ok(py.None());
                }
                let value = completion_value::<sys::mln_buffer_view>(result)?;
                Ok(if value.data.is_null() {
                    py.None()
                } else {
                    PyBytes::new(py, unsafe {
                        generated_slice(value.data.cast::<u8>(), value.size)?
                    })
                    .into_any()
                    .unbind()
                })
            },
        )
    }
    #[pyo3(signature = (property_name))]
    fn get_style_light_property(
        &self,
        py: Python<'_>,
        property_name: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_get_style_light_property",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let property_name_value = storage.buffer(property_name.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_get_style_light_property(handle, property_name_value, completion)
                })
            },
            |py, result| {
                if result.value.is_null() {
                    return Ok(py.None());
                }
                let value = completion_value::<sys::mln_buffer_view>(result)?;
                Ok(if value.data.is_null() {
                    py.None()
                } else {
                    PyBytes::new(py, unsafe {
                        generated_slice(value.data.cast::<u8>(), value.size)?
                    })
                    .into_any()
                    .unbind()
                })
            },
        )
    }
    #[pyo3(signature = (source_id))]
    fn get_style_source_info(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_get_style_source_info",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_get_style_source_info(handle, source_id_value, completion)
                })
            },
            |py, result| {
                if result.value.is_null() {
                    return Ok(py.None());
                }
                let value = completion_value::<sys::mln_style_source_result>(result)?;
                Ok(generated_copy_mln_style_source_result(py, &value)?)
            },
        )
    }
    #[pyo3(signature = (source_id))]
    fn get_style_source_tile_urls(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_get_style_source_tile_urls",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_get_style_source_tile_urls(handle, source_id_value, completion)
                })
            },
            |py, result| {
                if result.value.is_null() {
                    return Ok(py.None());
                }
                let value = completion_value::<sys::mln_style_source_tile_urls_result>(result)?;
                Ok(generated_copy_mln_style_source_tile_urls_result(
                    py, &value,
                )?)
            },
        )
    }
    #[pyo3(signature = ())]
    fn get_style_transition_options(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_get_style_transition_options",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_get_style_transition_options(handle, completion)
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_style_transition_options>(result)?;
                Ok(generated_copy_mln_style_transition_options(py, &value)?)
            },
        )
    }
    #[pyo3(signature = (source_id, bounds))]
    fn invalidate_custom_geometry_source_region(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        bounds: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_invalidate_custom_geometry_source_region",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let bounds_value = generated_input_mln_lat_lng_bounds(&bounds.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_invalidate_custom_geometry_source_region(
                    handle,
                    source_id_value,
                    bounds_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, tile_id))]
    fn invalidate_custom_geometry_source_tile(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        tile_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_invalidate_custom_geometry_source_tile",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let tile_id_value = generated_input_mln_canonical_tile_id(&tile_id.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_invalidate_custom_geometry_source_tile(
                    handle,
                    source_id_value,
                    tile_id_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, tile_id))]
    fn invalidate_custom_mvt_vector_source_tile(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        tile_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_invalidate_custom_mvt_vector_source_tile",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let tile_id_value = generated_input_mln_canonical_tile_id(&tile_id.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_invalidate_custom_mvt_vector_source_tile(
                    handle,
                    source_id_value,
                    tile_id_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (camera=None))]
    fn lat_lng_bounds_for_camera(
        &self,
        py: Python<'_>,
        camera: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_lat_lng_bounds_for_camera",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let camera = camera.unwrap_or_else(|| py.None().into_bound(py));
        let camera_value = if camera.clone().is_none() {
            unsafe { sys::mln_camera_options_default() }
        } else {
            generated_input_mln_camera_options(&camera.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_lat_lng_bounds_for_camera(handle, &camera_value, completion)
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_lat_lng_bounds>(result)?;
                Ok(generated_copy_mln_lat_lng_bounds(py, &value)?)
            },
        )
    }
    #[pyo3(signature = (camera=None))]
    fn lat_lng_bounds_for_camera_unwrapped(
        &self,
        py: Python<'_>,
        camera: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_lat_lng_bounds_for_camera_unwrapped",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let camera = camera.unwrap_or_else(|| py.None().into_bound(py));
        let camera_value = if camera.clone().is_none() {
            unsafe { sys::mln_camera_options_default() }
        } else {
            generated_input_mln_camera_options(&camera.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_lat_lng_bounds_for_camera_unwrapped(
                        handle,
                        &camera_value,
                        completion,
                    )
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_lat_lng_bounds>(result)?;
                Ok(generated_copy_mln_lat_lng_bounds(py, &value)?)
            },
        )
    }
    #[pyo3(signature = (point))]
    fn lat_lng_for_pixel(&self, py: Python<'_>, point: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_lat_lng_for_pixel",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let point_value = generated_input_mln_screen_point(&point.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_lat_lng_for_pixel(handle, point_value, completion)
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_lat_lng>(result)?;
                Ok(generated_copy_mln_lat_lng(py, &value)?)
            },
        )
    }
    #[pyo3(signature = (point))]
    fn lat_lng_for_pixel_unwrapped(
        &self,
        py: Python<'_>,
        point: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_lat_lng_for_pixel_unwrapped",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let point_value = generated_input_mln_screen_point(&point.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_lat_lng_for_pixel_unwrapped(handle, point_value, completion)
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_lat_lng>(result)?;
                Ok(generated_copy_mln_lat_lng(py, &value)?)
            },
        )
    }
    #[pyo3(signature = (points))]
    fn lat_lngs_for_pixels(
        &self,
        py: Python<'_>,
        points: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_lat_lngs_for_pixels",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let mut points_values = Vec::new();
        for item in points.try_iter()? {
            let item = item?;
            points_values.push(generated_input_mln_screen_point(&item, storage)?);
        }
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_lat_lngs_for_pixels(
                        handle,
                        points_values.as_ptr(),
                        points_values.len(),
                        completion,
                    )
                })
            },
            |py, result| {
                let list = PyList::empty(py);
                for value in generated_completion_slice::<sys::mln_lat_lng>(result)? {
                    list.append(generated_copy_mln_lat_lng(py, &(*value))?)?;
                }
                Ok(list.into_any().unbind())
            },
        )
    }
    #[pyo3(signature = (points))]
    fn lat_lngs_for_pixels_unwrapped(
        &self,
        py: Python<'_>,
        points: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_lat_lngs_for_pixels_unwrapped",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let mut points_values = Vec::new();
        for item in points.try_iter()? {
            let item = item?;
            points_values.push(generated_input_mln_screen_point(&item, storage)?);
        }
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_lat_lngs_for_pixels_unwrapped(
                        handle,
                        points_values.as_ptr(),
                        points_values.len(),
                        completion,
                    )
                })
            },
            |py, result| {
                let list = PyList::empty(py);
                for value in generated_completion_slice::<sys::mln_lat_lng>(result)? {
                    list.append(generated_copy_mln_lat_lng(py, &(*value))?)?;
                }
                Ok(list.into_any().unbind())
            },
        )
    }
    #[pyo3(signature = ())]
    fn list_style_layer_ids(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_list_style_layer_ids",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || sys::mln_map_list_style_layer_ids(handle, completion))
            },
            |py, result| {
                let list = PyList::empty(py);
                for value in generated_completion_slice::<sys::mln_buffer_view>(result)? {
                    list.append(
                        copied_string_view(*value)?
                            .into_pyobject(py)?
                            .into_any()
                            .unbind(),
                    )?;
                }
                Ok(list.into_any().unbind())
            },
        )
    }
    #[pyo3(signature = ())]
    fn list_style_layers(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_list_style_layers",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || sys::mln_map_list_style_layers(handle, completion))
            },
            |py, result| {
                let list = PyList::empty(py);
                for value in generated_completion_slice::<sys::mln_style_layer_entry>(result)? {
                    list.append(generated_copy_mln_style_layer_entry(py, &(*value))?)?;
                }
                Ok(list.into_any().unbind())
            },
        )
    }
    #[pyo3(signature = ())]
    fn list_style_source_ids(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_list_style_source_ids",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_list_style_source_ids(handle, completion)
                })
            },
            |py, result| {
                let list = PyList::empty(py);
                for value in generated_completion_slice::<sys::mln_buffer_view>(result)? {
                    list.append(
                        copied_string_view(*value)?
                            .into_pyobject(py)?
                            .into_any()
                            .unbind(),
                    )?;
                }
                Ok(list.into_any().unbind())
            },
        )
    }
    #[pyo3(signature = ())]
    fn loaded_style_json(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_loaded_style_json",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || sys::mln_map_loaded_style_json(handle, completion))
            },
            |py, result| {
                let value = completion_value::<sys::mln_buffer_view>(result)?;
                Ok(PyBytes::new(py, unsafe {
                    generated_slice(value.data.cast::<u8>(), value.size)?
                })
                .into_any()
                .unbind())
            },
        )
    }
    #[pyo3(signature = (latitude))]
    fn meters_per_pixel_at_latitude(&self, py: Python<'_>, latitude: f64) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_meters_per_pixel_at_latitude",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_meters_per_pixel_at_latitude(handle, latitude, completion)
                })
            },
            |py, result| {
                let value = completion_value::<f64>(result)?;
                Ok(pyo3::BoundObject::unbind((value).into_pyobject(py)?).into_any())
            },
        )
    }
    #[pyo3(signature = (layer_id, before_layer_id=None))]
    fn move_style_layer(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        before_layer_id: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_move_style_layer",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let before_layer_id = before_layer_id.unwrap_or_else(|| py.None().into_bound(py));
        let before_layer_id_value = storage.buffer(before_layer_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_move_style_layer(
                    handle,
                    layer_id_value,
                    before_layer_id_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (coordinate))]
    fn pixel_for_lat_lng(
        &self,
        py: Python<'_>,
        coordinate: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_pixel_for_lat_lng",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let coordinate_value = generated_input_mln_lat_lng(&coordinate.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_pixel_for_lat_lng(handle, coordinate_value, completion)
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_screen_point>(result)?;
                Ok(generated_copy_mln_screen_point(py, &value)?)
            },
        )
    }
    #[pyo3(signature = (coordinates))]
    fn pixels_for_lat_lngs(
        &self,
        py: Python<'_>,
        coordinates: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_pixels_for_lat_lngs",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let mut coordinates_values = Vec::new();
        for item in coordinates.try_iter()? {
            let item = item?;
            coordinates_values.push(generated_input_mln_lat_lng(&item, storage)?);
        }
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_pixels_for_lat_lngs(
                        handle,
                        coordinates_values.as_ptr(),
                        coordinates_values.len(),
                        completion,
                    )
                })
            },
            |py, result| {
                let list = PyList::empty(py);
                for value in generated_completion_slice::<sys::mln_screen_point>(result)? {
                    list.append(generated_copy_mln_screen_point(py, &(*value))?)?;
                }
                Ok(list.into_any().unbind())
            },
        )
    }
    #[pyo3(signature = ())]
    fn projection_create(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_projection_create",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_owned_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || sys::mln_map_projection_create(handle, completion))
            },
            |py, result| {
                let raw = completion_value::<sys::mln_map_projection>(result)?;
                let state = unsafe { NativeHandleState::from_handle(raw, "mln_map_projection") }
                    .map_err(map_error)?
                    .with_disposal(generated_dispose_mln_map_projection);
                Py::new(
                    py,
                    MapProjectionHandle {
                        state: Arc::new(Mutex::new(state)),
                    },
                )
                .map(|value| value.into_any())
            },
            |result| {
                if !result.value.is_null() && result.value_count == 1 {
                    unsafe {
                        generated_dispose_mln_map_projection(
                            result.value.cast::<sys::mln_map_projection>().read(),
                        );
                    }
                }
            },
        )
    }
    #[pyo3(signature = ())]
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_release",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let Some(mut reservation) = GeneratedHandleReservation::new(&self.state)? else {
            return completed_python_future(py);
        };
        let handle = reservation.handle();
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || sys::mln_map_release(handle, completion))
            },
            py_none,
        )?;
        reservation.commit();
        Ok(future)
    }
    #[pyo3(signature = (selector))]
    fn remove_feature_state(
        &self,
        py: Python<'_>,
        selector: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_remove_feature_state",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let selector_value =
            generated_input_mln_feature_state_selector(&selector.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_remove_feature_state(handle, &selector_value, completion)
            })
        })
    }
    #[pyo3(signature = (image_id))]
    fn remove_style_image(
        &self,
        py: Python<'_>,
        image_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_remove_style_image",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let image_id_value = storage.buffer(image_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_remove_style_image(handle, image_id_value, completion)
            })
        })
    }
    #[pyo3(signature = (layer_id))]
    fn remove_style_layer(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_remove_style_layer",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_remove_style_layer(handle, layer_id_value, completion)
            })
        })
    }
    #[pyo3(signature = (source_id))]
    fn remove_style_source(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_remove_style_source",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_remove_style_source(handle, source_id_value, completion)
            })
        })
    }
    #[pyo3(signature = ())]
    fn request_repaint(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_request_repaint",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || sys::mln_map_request_repaint(handle, completion))
        })
    }
    #[pyo3(signature = ())]
    fn request_still_image(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_request_still_image",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || sys::mln_map_request_still_image(handle, completion))
            },
            py_none,
        )
    }
    #[pyo3(signature = (extent))]
    fn resize(&self, py: Python<'_>, extent: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_resize",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let extent_value = generated_input_mln_logical_extent(&extent.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || sys::mln_map_resize(handle, extent_value, completion))
        })
    }
    #[pyo3(signature = (options=None))]
    fn set_bounds(&self, py: Python<'_>, options: Option<Bound<'_, PyAny>>) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_bounds",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_bound_options_default() }
        } else {
            generated_input_mln_bound_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_bounds(handle, &options_value, completion)
            })
        })
    }
    #[pyo3(signature = (source_id, tile_id, data))]
    fn set_custom_geometry_source_tile_data(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        tile_id: &Bound<'_, PyAny>,
        data: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_custom_geometry_source_tile_data",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let tile_id_value = generated_input_mln_canonical_tile_id(&tile_id.clone(), storage)?;
        let data_value = storage.buffer(data.clone(), false)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_custom_geometry_source_tile_data(
                    handle,
                    source_id_value,
                    tile_id_value,
                    data_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, tile_id, data))]
    fn set_custom_mvt_vector_source_tile_data(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        tile_id: &Bound<'_, PyAny>,
        data: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_custom_mvt_vector_source_tile_data",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let tile_id_value = generated_input_mln_canonical_tile_id(&tile_id.clone(), storage)?;
        let data_value = storage.buffer(data.clone(), false)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_custom_mvt_vector_source_tile_data(
                    handle,
                    source_id_value,
                    tile_id_value,
                    data_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, tile_id, message))]
    fn set_custom_mvt_vector_source_tile_error(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        tile_id: &Bound<'_, PyAny>,
        message: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_custom_mvt_vector_source_tile_error",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let tile_id_value = generated_input_mln_canonical_tile_id(&tile_id.clone(), storage)?;
        let message_value = storage.buffer(message.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_custom_mvt_vector_source_tile_error(
                    handle,
                    source_id_value,
                    tile_id_value,
                    message_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (options))]
    fn set_debug_options(
        &self,
        py: Python<'_>,
        options: sys::mln_map_debug_option,
    ) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_set_debug_options",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_debug_options(handle, options, completion)
            })
        })
    }
    #[pyo3(signature = (mask))]
    fn set_event_mask(
        &self,
        py: Python<'_>,
        mask: sys::mln_runtime_event_mask,
    ) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_set_event_mask",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || sys::mln_map_set_event_mask(handle, mask, completion))
        })
    }
    #[pyo3(signature = (selector, input_state))]
    fn set_feature_state(
        &self,
        py: Python<'_>,
        selector: &Bound<'_, PyAny>,
        input_state: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_feature_state",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let selector_value =
            generated_input_mln_feature_state_selector(&selector.clone(), storage)?;
        let input_state_value = storage.buffer(input_state.clone(), false)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_feature_state(
                    handle,
                    &selector_value,
                    input_state_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (options=None))]
    fn set_free_camera_options(
        &self,
        py: Python<'_>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_free_camera_options",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_free_camera_options_default() }
        } else {
            generated_input_mln_free_camera_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_free_camera_options(handle, &options_value, completion)
            })
        })
    }
    #[pyo3(signature = (source_id, data))]
    fn set_geojson_source_data(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        data: &GeojsonSourceDataHandle,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_geojson_source_data",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let data_handle = data
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("input handle is closed"))?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_geojson_source_data(
                    handle,
                    source_id_value,
                    data_handle,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, enabled))]
    fn set_geojson_source_synchronous_tiling(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        enabled: bool,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_geojson_source_synchronous_tiling",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_geojson_source_synchronous_tiling(
                    handle,
                    source_id_value,
                    enabled,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, url))]
    fn set_geojson_source_url(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        url: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_geojson_source_url",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let url_value = storage.buffer(url.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_geojson_source_url(handle, source_id_value, url_value, completion)
            })
        })
    }
    #[pyo3(signature = (property_name, value))]
    fn set_global_state_property(
        &self,
        py: Python<'_>,
        property_name: &Bound<'_, PyAny>,
        value: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_global_state_property",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let property_name_value = storage.buffer(property_name.clone(), true)?;
        let value_value = storage.buffer(value.clone(), false)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_global_state_property(
                    handle,
                    property_name_value,
                    value_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, coordinates))]
    fn set_image_source_coordinates(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        coordinates: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_image_source_coordinates",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let mut coordinates_values = Vec::new();
        for item in coordinates.try_iter()? {
            let item = item?;
            coordinates_values.push(generated_input_mln_lat_lng(&item, storage)?);
        }
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_image_source_coordinates(
                    handle,
                    source_id_value,
                    coordinates_values.as_ptr(),
                    coordinates_values.len(),
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, image=None))]
    fn set_image_source_image(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        image: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_image_source_image",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let image = image.unwrap_or_else(|| py.None().into_bound(py));
        let image_value = if image.clone().is_none() {
            unsafe { sys::mln_premultiplied_rgba8_image_default() }
        } else {
            generated_input_mln_premultiplied_rgba8_image(&image.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_image_source_image(
                    handle,
                    source_id_value,
                    &image_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, url))]
    fn set_image_source_url(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        url: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_image_source_url",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let url_value = storage.buffer(url.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_image_source_url(handle, source_id_value, url_value, completion)
            })
        })
    }
    #[pyo3(signature = (layer_id, filter=None))]
    fn set_layer_filter(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        filter: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_layer_filter",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let filter = filter.unwrap_or_else(|| py.None().into_bound(py));
        let filter_value = if filter.is_none() {
            None
        } else {
            Some(storage.buffer(filter.clone(), false)?)
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_layer_filter(
                    handle,
                    layer_id_value,
                    filter_value
                        .as_ref()
                        .map_or(std::ptr::null(), |value| value),
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (layer_id, max_zoom))]
    fn set_layer_max_zoom(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        max_zoom: f64,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_layer_max_zoom",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_layer_max_zoom(handle, layer_id_value, max_zoom, completion)
            })
        })
    }
    #[pyo3(signature = (layer_id, min_zoom))]
    fn set_layer_min_zoom(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        min_zoom: f64,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_layer_min_zoom",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_layer_min_zoom(handle, layer_id_value, min_zoom, completion)
            })
        })
    }
    #[pyo3(signature = (layer_id, property_name, value))]
    fn set_layer_property(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        property_name: &Bound<'_, PyAny>,
        value: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_layer_property",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let property_name_value = storage.buffer(property_name.clone(), true)?;
        let value_value = storage.buffer(value.clone(), false)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_layer_property(
                    handle,
                    layer_id_value,
                    property_name_value,
                    value_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (layer_id, source_id))]
    fn set_layer_source_id(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        source_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_layer_source_id",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_layer_source_id(
                    handle,
                    layer_id_value,
                    source_id_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (layer_id, source_layer=None))]
    fn set_layer_source_layer(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        source_layer: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_layer_source_layer",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let source_layer = source_layer.unwrap_or_else(|| py.None().into_bound(py));
        let source_layer_value = storage.buffer(source_layer.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_layer_source_layer(
                    handle,
                    layer_id_value,
                    source_layer_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (layer_id, visibility))]
    fn set_layer_visibility(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        visibility: sys::mln_style_layer_visibility,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_layer_visibility",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_layer_visibility(handle, layer_id_value, visibility, completion)
            })
        })
    }
    #[pyo3(signature = (layer_id, radius))]
    fn set_location_indicator_accuracy_radius(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        radius: f64,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_location_indicator_accuracy_radius",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_location_indicator_accuracy_radius(
                    handle,
                    layer_id_value,
                    radius,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (layer_id, bearing))]
    fn set_location_indicator_bearing(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        bearing: f64,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_location_indicator_bearing",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_location_indicator_bearing(
                    handle,
                    layer_id_value,
                    bearing,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (layer_id, image_kind, image_id))]
    fn set_location_indicator_image_name(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        image_kind: sys::mln_location_indicator_image_kind,
        image_id: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_location_indicator_image_name",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let image_id_value = storage.buffer(image_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_location_indicator_image_name(
                    handle,
                    layer_id_value,
                    image_kind,
                    image_id_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (layer_id, coordinate, altitude))]
    fn set_location_indicator_location(
        &self,
        py: Python<'_>,
        layer_id: &Bound<'_, PyAny>,
        coordinate: &Bound<'_, PyAny>,
        altitude: f64,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_location_indicator_location",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let layer_id_value = storage.buffer(layer_id.clone(), true)?;
        let coordinate_value = generated_input_mln_lat_lng(&coordinate.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_location_indicator_location(
                    handle,
                    layer_id_value,
                    coordinate_value,
                    altitude,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (mode=None))]
    fn set_projection_mode(
        &self,
        py: Python<'_>,
        mode: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_projection_mode",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let mode = mode.unwrap_or_else(|| py.None().into_bound(py));
        let mode_value = if mode.clone().is_none() {
            unsafe { sys::mln_projection_mode_default() }
        } else {
            generated_input_mln_projection_mode(&mode.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_projection_mode(handle, &mode_value, completion)
            })
        })
    }
    #[pyo3(signature = (enabled))]
    fn set_rendering_stats_view_enabled(
        &self,
        py: Python<'_>,
        enabled: bool,
    ) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_set_rendering_stats_view_enabled",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_rendering_stats_view_enabled(handle, enabled, completion)
            })
        })
    }
    #[pyo3(signature = (image_id, image=None, options=None))]
    fn set_style_image(
        &self,
        py: Python<'_>,
        image_id: &Bound<'_, PyAny>,
        image: Option<Bound<'_, PyAny>>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_style_image",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let image_id_value = storage.buffer(image_id.clone(), true)?;
        let image = image.unwrap_or_else(|| py.None().into_bound(py));
        let image_value = if image.clone().is_none() {
            unsafe { sys::mln_premultiplied_rgba8_image_default() }
        } else {
            generated_input_mln_premultiplied_rgba8_image(&image.clone(), storage)?
        };
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.is_none() {
            None
        } else {
            Some(if options.clone().is_none() {
                unsafe { sys::mln_style_image_options_default() }
            } else {
                generated_input_mln_style_image_options(&options.clone(), storage)?
            })
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_style_image(
                    handle,
                    image_id_value,
                    &image_value,
                    options_value
                        .as_ref()
                        .map_or(std::ptr::null(), |value| value),
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (json))]
    fn set_style_json(&self, py: Python<'_>, json: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_style_json",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let json_value = storage.buffer(json.clone(), false)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_style_json(handle, json_value, completion)
            })
        })
    }
    #[pyo3(signature = (light_json))]
    fn set_style_light_json(
        &self,
        py: Python<'_>,
        light_json: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_style_light_json",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let light_json_value = storage.buffer(light_json.clone(), false)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_style_light_json(handle, light_json_value, completion)
            })
        })
    }
    #[pyo3(signature = (property_name, value))]
    fn set_style_light_property(
        &self,
        py: Python<'_>,
        property_name: &Bound<'_, PyAny>,
        value: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_style_light_property",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let property_name_value = storage.buffer(property_name.clone(), true)?;
        let value_value = storage.buffer(value.clone(), false)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_style_light_property(
                    handle,
                    property_name_value,
                    value_value,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (source_id, is_volatile))]
    fn set_style_source_volatile(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        is_volatile: bool,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_style_source_volatile",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_style_source_volatile(
                    handle,
                    source_id_value,
                    is_volatile,
                    completion,
                )
            })
        })
    }
    #[pyo3(signature = (options=None))]
    fn set_style_transition_options(
        &self,
        py: Python<'_>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_style_transition_options",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_style_transition_options_default() }
        } else {
            generated_input_mln_style_transition_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_style_transition_options(handle, &options_value, completion)
            })
        })
    }
    #[pyo3(signature = (url))]
    fn set_style_url(&self, py: Python<'_>, url: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_style_url",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let url_value = storage.c_string(url.clone())?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_style_url(handle, url_value, completion)
            })
        })
    }
    #[pyo3(signature = (options=None))]
    fn set_tile_options(
        &self,
        py: Python<'_>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_tile_options",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_map_tile_options_default() }
        } else {
            generated_input_mln_map_tile_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_tile_options(handle, &options_value, completion)
            })
        })
    }
    #[pyo3(signature = (options=None))]
    fn set_viewport_options(
        &self,
        py: Python<'_>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_set_viewport_options",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_map_viewport_options_default() }
        } else {
            generated_input_mln_map_viewport_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_set_viewport_options(handle, &options_value, completion)
            })
        })
    }
    #[pyo3(signature = ())]
    fn snapshot_get(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_snapshot_get",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_snapshot: sys::mln_map_snapshot = unsafe { std::mem::zeroed() };
        out_snapshot.size = std::mem::size_of::<sys::mln_map_snapshot>() as _;
        let result = unsafe {
            generated_native_call(py, || sys::mln_map_snapshot_get(handle, &mut out_snapshot))
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(generated_copy_mln_map_snapshot(py, &out_snapshot)?)
    }
    #[pyo3(signature = ())]
    fn style_url(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_style_url",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || sys::mln_map_style_url(handle, completion))
            },
            |py, result| {
                let value = completion_value::<sys::mln_buffer_view>(result)?;
                Ok(copied_string_view(value)?
                    .into_pyobject(py)?
                    .into_any()
                    .unbind())
            },
        )
    }
    #[pyo3(signature = (update=None))]
    fn update_camera(
        &self,
        py: Python<'_>,
        update: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_update_camera",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let update = update.unwrap_or_else(|| py.None().into_bound(py));
        let update_value = if update.clone().is_none() {
            unsafe { sys::mln_camera_update_default() }
        } else {
            generated_input_mln_camera_update(&update.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_map_update_camera(handle, &update_value, completion)
            })
        })
    }
    #[pyo3(signature = (descriptor=None, options=None))]
    fn metal_borrowed_texture_attach(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_metal_borrowed_texture_attach",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_metal_borrowed_texture_descriptor_default() }
        } else {
            generated_input_mln_metal_borrowed_texture_descriptor(&descriptor.clone(), storage)?
        };
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_render_session_attach_options_default() }
        } else {
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_metal_borrowed_texture_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                    )
                })
            },
            py_none,
        )?;
        let callback_roots = storage.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = Py::new(
            py,
            RenderSessionHandle {
                state: Arc::new(Mutex::new(
                    unsafe {
                        NativeHandleState::from_handle(
                            out_session_owner.take(),
                            "mln_render_session",
                        )
                    }
                    .map_err(map_error)?
                    .with_disposal(generated_dispose_mln_render_session)
                    .with_callback_roots(callback_roots.clone()),
                )),
            },
        )?;
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_metal_owned_texture_attach",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_metal_owned_texture_descriptor_default() }
        } else {
            generated_input_mln_metal_owned_texture_descriptor(&descriptor.clone(), storage)?
        };
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_render_session_attach_options_default() }
        } else {
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_metal_owned_texture_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                    )
                })
            },
            py_none,
        )?;
        let callback_roots = storage.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = Py::new(
            py,
            RenderSessionHandle {
                state: Arc::new(Mutex::new(
                    unsafe {
                        NativeHandleState::from_handle(
                            out_session_owner.take(),
                            "mln_render_session",
                        )
                    }
                    .map_err(map_error)?
                    .with_disposal(generated_dispose_mln_render_session)
                    .with_callback_roots(callback_roots.clone()),
                )),
            },
        )?;
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_metal_surface_attach",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_metal_surface_descriptor_default() }
        } else {
            generated_input_mln_metal_surface_descriptor(&descriptor.clone(), storage)?
        };
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_render_session_attach_options_default() }
        } else {
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_metal_surface_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                    )
                })
            },
            py_none,
        )?;
        let callback_roots = storage.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = Py::new(
            py,
            RenderSessionHandle {
                state: Arc::new(Mutex::new(
                    unsafe {
                        NativeHandleState::from_handle(
                            out_session_owner.take(),
                            "mln_render_session",
                        )
                    }
                    .map_err(map_error)?
                    .with_disposal(generated_dispose_mln_render_session)
                    .with_callback_roots(callback_roots.clone()),
                )),
            },
        )?;
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_opengl_borrowed_texture_attach",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_opengl_borrowed_texture_descriptor_default() }
        } else {
            generated_input_mln_opengl_borrowed_texture_descriptor(&descriptor.clone(), storage)?
        };
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_render_session_attach_options_default() }
        } else {
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_opengl_borrowed_texture_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                    )
                })
            },
            py_none,
        )?;
        let callback_roots = storage.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = Py::new(
            py,
            RenderSessionHandle {
                state: Arc::new(Mutex::new(
                    unsafe {
                        NativeHandleState::from_handle(
                            out_session_owner.take(),
                            "mln_render_session",
                        )
                    }
                    .map_err(map_error)?
                    .with_disposal(generated_dispose_mln_render_session)
                    .with_callback_roots(callback_roots.clone()),
                )),
            },
        )?;
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_opengl_owned_texture_attach",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_opengl_owned_texture_descriptor_default() }
        } else {
            generated_input_mln_opengl_owned_texture_descriptor(&descriptor.clone(), storage)?
        };
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_render_session_attach_options_default() }
        } else {
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_opengl_owned_texture_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                    )
                })
            },
            py_none,
        )?;
        let callback_roots = storage.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = Py::new(
            py,
            RenderSessionHandle {
                state: Arc::new(Mutex::new(
                    unsafe {
                        NativeHandleState::from_handle(
                            out_session_owner.take(),
                            "mln_render_session",
                        )
                    }
                    .map_err(map_error)?
                    .with_disposal(generated_dispose_mln_render_session)
                    .with_callback_roots(callback_roots.clone()),
                )),
            },
        )?;
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_opengl_surface_attach",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_opengl_surface_descriptor_default() }
        } else {
            generated_input_mln_opengl_surface_descriptor(&descriptor.clone(), storage)?
        };
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_render_session_attach_options_default() }
        } else {
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_opengl_surface_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                    )
                })
            },
            py_none,
        )?;
        let callback_roots = storage.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = Py::new(
            py,
            RenderSessionHandle {
                state: Arc::new(Mutex::new(
                    unsafe {
                        NativeHandleState::from_handle(
                            out_session_owner.take(),
                            "mln_render_session",
                        )
                    }
                    .map_err(map_error)?
                    .with_disposal(generated_dispose_mln_render_session)
                    .with_callback_roots(callback_roots.clone()),
                )),
            },
        )?;
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_vulkan_borrowed_texture_attach",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_vulkan_borrowed_texture_descriptor_default() }
        } else {
            generated_input_mln_vulkan_borrowed_texture_descriptor(&descriptor.clone(), storage)?
        };
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_render_session_attach_options_default() }
        } else {
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_vulkan_borrowed_texture_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                    )
                })
            },
            py_none,
        )?;
        let callback_roots = storage.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = Py::new(
            py,
            RenderSessionHandle {
                state: Arc::new(Mutex::new(
                    unsafe {
                        NativeHandleState::from_handle(
                            out_session_owner.take(),
                            "mln_render_session",
                        )
                    }
                    .map_err(map_error)?
                    .with_disposal(generated_dispose_mln_render_session)
                    .with_callback_roots(callback_roots.clone()),
                )),
            },
        )?;
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_vulkan_owned_texture_attach",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_vulkan_owned_texture_descriptor_default() }
        } else {
            generated_input_mln_vulkan_owned_texture_descriptor(&descriptor.clone(), storage)?
        };
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_render_session_attach_options_default() }
        } else {
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_vulkan_owned_texture_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                    )
                })
            },
            py_none,
        )?;
        let callback_roots = storage.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = Py::new(
            py,
            RenderSessionHandle {
                state: Arc::new(Mutex::new(
                    unsafe {
                        NativeHandleState::from_handle(
                            out_session_owner.take(),
                            "mln_render_session",
                        )
                    }
                    .map_err(map_error)?
                    .with_disposal(generated_dispose_mln_render_session)
                    .with_callback_roots(callback_roots.clone()),
                )),
            },
        )?;
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_vulkan_surface_attach",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_vulkan_surface_descriptor_default() }
        } else {
            generated_input_mln_vulkan_surface_descriptor(&descriptor.clone(), storage)?
        };
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_render_session_attach_options_default() }
        } else {
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_vulkan_surface_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                    )
                })
            },
            py_none,
        )?;
        let callback_roots = storage.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = Py::new(
            py,
            RenderSessionHandle {
                state: Arc::new(Mutex::new(
                    unsafe {
                        NativeHandleState::from_handle(
                            out_session_owner.take(),
                            "mln_render_session",
                        )
                    }
                    .map_err(map_error)?
                    .with_disposal(generated_dispose_mln_render_session)
                    .with_callback_roots(callback_roots.clone()),
                )),
            },
        )?;
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_webgpu_borrowed_texture_attach",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_webgpu_borrowed_texture_descriptor_default() }
        } else {
            generated_input_mln_webgpu_borrowed_texture_descriptor(&descriptor.clone(), storage)?
        };
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_render_session_attach_options_default() }
        } else {
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_webgpu_borrowed_texture_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                    )
                })
            },
            py_none,
        )?;
        let callback_roots = storage.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = Py::new(
            py,
            RenderSessionHandle {
                state: Arc::new(Mutex::new(
                    unsafe {
                        NativeHandleState::from_handle(
                            out_session_owner.take(),
                            "mln_render_session",
                        )
                    }
                    .map_err(map_error)?
                    .with_disposal(generated_dispose_mln_render_session)
                    .with_callback_roots(callback_roots.clone()),
                )),
            },
        )?;
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_webgpu_owned_texture_attach",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_webgpu_owned_texture_descriptor_default() }
        } else {
            generated_input_mln_webgpu_owned_texture_descriptor(&descriptor.clone(), storage)?
        };
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_render_session_attach_options_default() }
        } else {
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_webgpu_owned_texture_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                    )
                })
            },
            py_none,
        )?;
        let callback_roots = storage.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = Py::new(
            py,
            RenderSessionHandle {
                state: Arc::new(Mutex::new(
                    unsafe {
                        NativeHandleState::from_handle(
                            out_session_owner.take(),
                            "mln_render_session",
                        )
                    }
                    .map_err(map_error)?
                    .with_disposal(generated_dispose_mln_render_session)
                    .with_callback_roots(callback_roots.clone()),
                )),
            },
        )?;
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_webgpu_surface_attach",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_webgpu_surface_descriptor_default() }
        } else {
            generated_input_mln_webgpu_surface_descriptor(&descriptor.clone(), storage)?
        };
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_render_session_attach_options_default() }
        } else {
            generated_input_mln_render_session_attach_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_session: sys::mln_render_session = unsafe { std::mem::zeroed() };
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_webgpu_surface_attach(
                        handle,
                        &descriptor_value,
                        &options_value,
                        &mut out_session,
                        completion,
                    )
                })
            },
            py_none,
        )?;
        let callback_roots = storage.accept_callbacks();
        let mut out_session_owner =
            GeneratedOwnedOutput::new(out_session, generated_dispose_mln_render_session);
        let out_session_python = Py::new(
            py,
            RenderSessionHandle {
                state: Arc::new(Mutex::new(
                    unsafe {
                        NativeHandleState::from_handle(
                            out_session_owner.take(),
                            "mln_render_session",
                        )
                    }
                    .map_err(map_error)?
                    .with_disposal(generated_dispose_mln_render_session)
                    .with_callback_roots(callback_roots.clone()),
                )),
            },
        )?;
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
        generated_check_operation(
            "mln_map_projection_close",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let Some(mut reservation) = GeneratedHandleReservation::new(&self.state)? else {
            return Ok(py.None());
        };
        let handle = reservation.handle();
        let result = unsafe { generated_native_call(py, || sys::mln_map_projection_close(handle)) };
        maplibre_core::check(result).map_err(map_error)?;
        reservation.commit();
        Ok(py.None())
    }
    #[pyo3(signature = ())]
    fn get_camera(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_projection_get_camera",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_camera: sys::mln_camera_options = unsafe { sys::mln_camera_options_default() };
        out_camera.size = std::mem::size_of::<sys::mln_camera_options>() as _;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_map_projection_get_camera(handle, &mut out_camera)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(generated_copy_mln_camera_options(py, &out_camera)?)
    }
    #[pyo3(signature = (point))]
    fn lat_lng_for_pixel(&self, py: Python<'_>, point: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_projection_lat_lng_for_pixel",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let point_value = generated_input_mln_screen_point(&point.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_coordinate: sys::mln_lat_lng = unsafe { std::mem::zeroed() };
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_map_projection_lat_lng_for_pixel(handle, point_value, &mut out_coordinate)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(generated_copy_mln_lat_lng(py, &out_coordinate)?)
    }
    #[pyo3(signature = (point))]
    fn lat_lng_for_pixel_unwrapped(
        &self,
        py: Python<'_>,
        point: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_projection_lat_lng_for_pixel_unwrapped",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let point_value = generated_input_mln_screen_point(&point.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_coordinate: sys::mln_lat_lng = unsafe { std::mem::zeroed() };
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_map_projection_lat_lng_for_pixel_unwrapped(
                    handle,
                    point_value,
                    &mut out_coordinate,
                )
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(generated_copy_mln_lat_lng(py, &out_coordinate)?)
    }
    #[pyo3(signature = (latitude))]
    fn meters_per_pixel_at_latitude(&self, py: Python<'_>, latitude: f64) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_map_projection_meters_per_pixel_at_latitude",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_meters_per_pixel: f64 = unsafe { std::mem::zeroed() };
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_map_projection_meters_per_pixel_at_latitude(
                    handle,
                    latitude,
                    &mut out_meters_per_pixel,
                )
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(pyo3::BoundObject::unbind((out_meters_per_pixel).into_pyobject(py)?).into_any())
    }
    #[pyo3(signature = (coordinate))]
    fn pixel_for_lat_lng(
        &self,
        py: Python<'_>,
        coordinate: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_projection_pixel_for_lat_lng",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let coordinate_value = generated_input_mln_lat_lng(&coordinate.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_point: sys::mln_screen_point = unsafe { std::mem::zeroed() };
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_map_projection_pixel_for_lat_lng(handle, coordinate_value, &mut out_point)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(generated_copy_mln_screen_point(py, &out_point)?)
    }
    #[pyo3(signature = (camera=None))]
    fn set_camera(&self, py: Python<'_>, camera: Option<Bound<'_, PyAny>>) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_projection_set_camera",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let camera = camera.unwrap_or_else(|| py.None().into_bound(py));
        let camera_value = if camera.clone().is_none() {
            unsafe { sys::mln_camera_options_default() }
        } else {
            generated_input_mln_camera_options(&camera.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_map_projection_set_camera(handle, &camera_value)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(py.None())
    }
    #[pyo3(signature = (coordinates, padding))]
    fn set_visible_coordinates(
        &self,
        py: Python<'_>,
        coordinates: &Bound<'_, PyAny>,
        padding: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_projection_set_visible_coordinates",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let mut coordinates_values = Vec::new();
        for item in coordinates.try_iter()? {
            let item = item?;
            coordinates_values.push(generated_input_mln_lat_lng(&item, storage)?);
        }
        let padding_value = generated_input_mln_edge_insets(&padding.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_map_projection_set_visible_coordinates(
                    handle,
                    coordinates_values.as_ptr(),
                    coordinates_values.len(),
                    padding_value,
                )
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(py.None())
    }
    #[pyo3(signature = (geometry, padding))]
    fn set_visible_geometry(
        &self,
        py: Python<'_>,
        geometry: &Bound<'_, PyAny>,
        padding: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_projection_set_visible_geometry",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let geometry_value = storage.buffer(geometry.clone(), false)?;
        let padding_value = generated_input_mln_edge_insets(&padding.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_map_projection_set_visible_geometry(handle, geometry_value, padding_value)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(py.None())
    }
}

#[pymethods]
impl RenderFrameBatchHandle {
    #[pyo3(signature = ())]
    fn count(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_render_frame_batch_count",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_count: usize = unsafe { std::mem::zeroed() };
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_render_frame_batch_count(handle, &mut out_count)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(pyo3::BoundObject::unbind((out_count).into_pyobject(py)?).into_any())
    }
    #[pyo3(signature = (index))]
    fn get(&self, py: Python<'_>, index: usize) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_render_frame_batch_get",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_result: sys::mln_render_frame_result = unsafe { std::mem::zeroed() };
        out_result.size = std::mem::size_of::<sys::mln_render_frame_result>() as _;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_render_frame_batch_get(handle, index, &mut out_result)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(generated_copy_mln_render_frame_result(py, &out_result)?)
    }
    #[pyo3(signature = ())]
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_render_frame_batch_release",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let Some(mut reservation) = GeneratedHandleReservation::new(&self.state)? else {
            return Ok(py.None());
        };
        let handle = reservation.handle();
        unsafe { generated_native_call(py, || sys::mln_render_frame_batch_release(handle)) };
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_metal_borrowed_texture_set_target",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_metal_borrowed_texture_descriptor_default() }
        } else {
            generated_input_mln_metal_borrowed_texture_descriptor(&descriptor.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_metal_borrowed_texture_set_target(
                        handle,
                        &descriptor_value,
                        completion,
                    )
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = (descriptor=None))]
    fn metal_surface_set_target(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_metal_surface_set_target",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_metal_surface_descriptor_default() }
        } else {
            generated_input_mln_metal_surface_descriptor(&descriptor.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_metal_surface_set_target(handle, &descriptor_value, completion)
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = (descriptor=None))]
    fn opengl_borrowed_texture_set_target(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_opengl_borrowed_texture_set_target",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_opengl_borrowed_texture_descriptor_default() }
        } else {
            generated_input_mln_opengl_borrowed_texture_descriptor(&descriptor.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_opengl_borrowed_texture_set_target(
                        handle,
                        &descriptor_value,
                        completion,
                    )
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = (descriptor=None))]
    fn opengl_surface_set_target(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_opengl_surface_set_target",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_opengl_surface_descriptor_default() }
        } else {
            generated_input_mln_opengl_surface_descriptor(&descriptor.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_opengl_surface_set_target(handle, &descriptor_value, completion)
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = ())]
    fn abandon(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_render_session_abandon",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let Some(reservation) = GeneratedHandleReservation::new(&self.state)? else {
            return Ok(py.None());
        };
        let handle = reservation.handle();
        let mut out_result: sys::mln_render_abandon_result = unsafe { std::mem::zeroed() };
        out_result.size = std::mem::size_of::<sys::mln_render_abandon_result>() as _;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_render_session_abandon(handle, &mut out_result)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        self.state().views_valid = false;
        Ok(generated_copy_mln_render_abandon_result(py, &out_result)?)
    }
    #[pyo3(signature = ())]
    fn acquire_frame(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_render_session_acquire_frame",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_frame: sys::mln_acquired_frame = unsafe { std::mem::zeroed() };
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_render_session_acquire_frame(handle, &mut out_frame)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Py::new(
            py,
            AcquiredFrameHandle {
                state: Arc::new(Mutex::new(
                    unsafe { NativeHandleState::from_handle(out_frame, "mln_acquired_frame") }
                        .map_err(map_error)?
                        .with_disposal(generated_dispose_mln_acquired_frame),
                )),
            },
        )
        .map(|value| value.into_any())
    }
    #[pyo3(signature = ())]
    fn barrier(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_render_session_barrier",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || sys::mln_render_session_barrier(handle, completion))
            },
            py_none,
        )
    }
    #[pyo3(signature = ())]
    fn clear_data(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_render_session_clear_data",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_render_session_clear_data(handle, completion)
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = ())]
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_render_session_destroy",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let Some(mut reservation) = GeneratedHandleReservation::new(&self.state)? else {
            return Ok(py.None());
        };
        let handle = reservation.handle();
        let result =
            unsafe { generated_native_call(py, || sys::mln_render_session_destroy(handle)) };
        maplibre_core::check(result).map_err(map_error)?;
        reservation.commit();
        Ok(py.None())
    }
    #[pyo3(signature = ())]
    fn detach(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_render_session_detach",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || sys::mln_render_session_detach(handle, completion))
            },
            py_none,
        )
    }
    #[pyo3(signature = ())]
    fn drain_frame_results(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_render_session_drain_frame_results",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_batch: sys::mln_render_frame_batch = unsafe { std::mem::zeroed() };
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_render_session_drain_frame_results(handle, &mut out_batch)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Py::new(
            py,
            RenderFrameBatchHandle {
                state: Arc::new(Mutex::new(
                    unsafe { NativeHandleState::from_handle(out_batch, "mln_render_frame_batch") }
                        .map_err(map_error)?
                        .with_disposal(generated_dispose_mln_render_frame_batch),
                )),
            },
        )
        .map(|value| value.into_any())
    }
    #[pyo3(signature = ())]
    fn dump_debug_logs(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_render_session_dump_debug_logs",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_render_session_dump_debug_logs(handle, completion)
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = ())]
    fn get_capabilities(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_render_session_get_capabilities",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_capabilities: sys::mln_render_session_capabilities =
            unsafe { std::mem::zeroed() };
        out_capabilities.size = std::mem::size_of::<sys::mln_render_session_capabilities>() as _;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_render_session_get_capabilities(handle, &mut out_capabilities)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(generated_copy_mln_render_session_capabilities(
            py,
            &out_capabilities,
        )?)
    }
    #[pyo3(signature = ())]
    fn get_snapshot(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_render_session_get_snapshot",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_snapshot: sys::mln_render_session_snapshot = unsafe { std::mem::zeroed() };
        out_snapshot.size = std::mem::size_of::<sys::mln_render_session_snapshot>() as _;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_render_session_get_snapshot(handle, &mut out_snapshot)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(generated_copy_mln_render_session_snapshot(
            py,
            &out_snapshot,
        )?)
    }
    #[pyo3(signature = ())]
    fn projection_create(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_render_session_projection_create",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_projection: sys::mln_map_projection = unsafe { std::mem::zeroed() };
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_render_session_projection_create(handle, &mut out_projection)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Py::new(
            py,
            MapProjectionHandle {
                state: Arc::new(Mutex::new(
                    unsafe { NativeHandleState::from_handle(out_projection, "mln_map_projection") }
                        .map_err(map_error)?
                        .with_disposal(generated_dispose_mln_map_projection),
                )),
            },
        )
        .map(|value| value.into_any())
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
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_render_session_query_feature_extensions",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let feature_value = storage.buffer(feature.clone(), false)?;
        let extension_value = storage.buffer(extension.clone(), true)?;
        let extension_field_value = storage.buffer(extension_field.clone(), true)?;
        let arguments = arguments.unwrap_or_else(|| py.None().into_bound(py));
        let arguments_value = if arguments.is_none() {
            None
        } else {
            Some(storage.buffer(arguments.clone(), false)?)
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_render_session_query_feature_extensions(
                        handle,
                        source_id_value,
                        feature_value,
                        extension_value,
                        extension_field_value,
                        arguments_value
                            .as_ref()
                            .map_or(std::ptr::null(), |value| value),
                        completion,
                    )
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_buffer_view>(result)?;
                Ok(PyBytes::new(py, unsafe {
                    generated_slice(value.data.cast::<u8>(), value.size)?
                })
                .into_any()
                .unbind())
            },
        )
    }
    #[pyo3(signature = (geometry, options=None))]
    fn query_rendered_features(
        &self,
        py: Python<'_>,
        geometry: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_render_session_query_rendered_features",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let geometry_value =
            generated_input_mln_rendered_query_geometry(&geometry.clone(), storage)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.is_none() {
            None
        } else {
            Some(if options.clone().is_none() {
                unsafe { sys::mln_rendered_feature_query_options_default() }
            } else {
                generated_input_mln_rendered_feature_query_options(&options.clone(), storage)?
            })
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_render_session_query_rendered_features(
                        handle,
                        &geometry_value,
                        options_value
                            .as_ref()
                            .map_or(std::ptr::null(), |value| value),
                        completion,
                    )
                })
            },
            |py, result| {
                let list = PyList::empty(py);
                for value in generated_completion_slice::<sys::mln_queried_feature>(result)? {
                    list.append(generated_copy_mln_queried_feature(py, &(*value))?)?;
                }
                Ok(list.into_any().unbind())
            },
        )
    }
    #[pyo3(signature = (source_id, options=None))]
    fn query_source_features(
        &self,
        py: Python<'_>,
        source_id: &Bound<'_, PyAny>,
        options: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_render_session_query_source_features",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let source_id_value = storage.buffer(source_id.clone(), true)?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.is_none() {
            None
        } else {
            Some(if options.clone().is_none() {
                unsafe { sys::mln_source_feature_query_options_default() }
            } else {
                generated_input_mln_source_feature_query_options(&options.clone(), storage)?
            })
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_render_session_query_source_features(
                        handle,
                        source_id_value,
                        options_value
                            .as_ref()
                            .map_or(std::ptr::null(), |value| value),
                        completion,
                    )
                })
            },
            |py, result| {
                let list = PyList::empty(py);
                for value in generated_completion_slice::<sys::mln_queried_feature>(result)? {
                    list.append(generated_copy_mln_queried_feature(py, &(*value))?)?;
                }
                Ok(list.into_any().unbind())
            },
        )
    }
    #[pyo3(signature = ())]
    fn reduce_memory_use(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_render_session_reduce_memory_use",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_render_session_reduce_memory_use(handle, completion)
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = (demand=None))]
    fn request_frame(
        &self,
        py: Python<'_>,
        demand: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_render_session_request_frame",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let demand = demand.unwrap_or_else(|| py.None().into_bound(py));
        let demand_value = if demand.clone().is_none() {
            unsafe { sys::mln_frame_demand_default() }
        } else {
            generated_input_mln_frame_demand(&demand.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_render_session_request_frame(handle, &demand_value)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(py.None())
    }
    #[pyo3(signature = (extent))]
    fn resize(&self, py: Python<'_>, extent: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_render_session_resize",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let extent_value = generated_input_mln_render_target_extent(&extent.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_command_future(py, |completion| unsafe {
            generated_native_call(py, || {
                sys::mln_render_session_resize(handle, &extent_value, completion)
            })
        })
    }
    #[pyo3(signature = (max_work))]
    fn service_driver_work(&self, py: Python<'_>, max_work: usize) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_render_session_service_driver_work",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_serviced: usize = unsafe { std::mem::zeroed() };
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_render_session_service_driver_work(handle, max_work, &mut out_serviced)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(pyo3::BoundObject::unbind((out_serviced).into_pyobject(py)?).into_any())
    }
    #[pyo3(signature = ())]
    fn texture_read_premultiplied_rgba8(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_texture_read_premultiplied_rgba8",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_texture_read_premultiplied_rgba8(handle, completion)
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_texture_readback_result>(result)?;
                Ok(generated_copy_mln_texture_readback_result(py, &value)?)
            },
        )
    }
    #[pyo3(signature = (descriptor=None))]
    fn vulkan_borrowed_texture_set_target(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_vulkan_borrowed_texture_set_target",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_vulkan_borrowed_texture_descriptor_default() }
        } else {
            generated_input_mln_vulkan_borrowed_texture_descriptor(&descriptor.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_vulkan_borrowed_texture_set_target(
                        handle,
                        &descriptor_value,
                        completion,
                    )
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = (descriptor=None))]
    fn vulkan_surface_set_target(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_vulkan_surface_set_target",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_vulkan_surface_descriptor_default() }
        } else {
            generated_input_mln_vulkan_surface_descriptor(&descriptor.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_vulkan_surface_set_target(handle, &descriptor_value, completion)
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = (descriptor=None))]
    fn webgpu_borrowed_texture_set_target(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_webgpu_borrowed_texture_set_target",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_webgpu_borrowed_texture_descriptor_default() }
        } else {
            generated_input_mln_webgpu_borrowed_texture_descriptor(&descriptor.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_webgpu_borrowed_texture_set_target(
                        handle,
                        &descriptor_value,
                        completion,
                    )
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = (descriptor=None))]
    fn webgpu_surface_set_target(
        &self,
        py: Python<'_>,
        descriptor: Option<Bound<'_, PyAny>>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_webgpu_surface_set_target",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let descriptor = descriptor.unwrap_or_else(|| py.None().into_bound(py));
        let descriptor_value = if descriptor.clone().is_none() {
            unsafe { sys::mln_webgpu_surface_descriptor_default() }
        } else {
            generated_input_mln_webgpu_surface_descriptor(&descriptor.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_webgpu_surface_set_target(handle, &descriptor_value, completion)
                })
            },
            py_none,
        )
    }
}

#[pymethods]
impl ResourceRequestHandle {
    #[pyo3(signature = ())]
    fn cancelled(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_resource_request_cancelled",
            maplibre_core::handle::NativeHandle::to_raw(self.state.issued_handle()),
        )?;
        let handle = self.state.native_for_call().map_err(map_error)?;
        let mut out_cancelled: bool = unsafe { std::mem::zeroed() };
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_resource_request_cancelled(handle, &mut out_cancelled)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(pyo3::BoundObject::unbind((out_cancelled).into_pyobject(py)?).into_any())
    }
    #[pyo3(signature = (response))]
    fn complete(&self, py: Python<'_>, response: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_resource_request_complete",
            maplibre_core::handle::NativeHandle::to_raw(self.state.issued_handle()),
        )?;
        let response_value = generated_input_mln_resource_response(&response.clone(), storage)?;
        let result = unsafe {
            generated_native_call(py, || {
                self.state.complete_with(|handle| {
                    maplibre_core::check(sys::mln_resource_request_complete(
                        handle,
                        &response_value,
                    ))
                })
            })
        };
        result.map_err(map_error)?;
        Ok(py.None())
    }
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_resource_request_release",
            maplibre_core::handle::NativeHandle::to_raw(self.state.issued_handle()),
        )?;
        unsafe { generated_native_call(py, || self.state.close()) };
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
            .set_cancel_callback(Box::new(move || {
                Python::try_attach(|py| {
                    if let Some(callback) = root.get(py, 0) {
                        if let Err(error) = callback.bind(py).call0() {
                            error.write_unraisable(py, None);
                        }
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
        generated_check_operation(
            "mln_resource_request_wait_until_retired",
            maplibre_core::handle::NativeHandle::to_raw(self.state.issued_handle()),
        )?;
        let handle = self.state.issued_handle();
        let result = unsafe {
            generated_native_call(py, || sys::mln_resource_request_wait_until_retired(handle))
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(py.None())
    }
}

#[pymethods]
impl ResourceTransformResponseScope {
    #[pyo3(signature = (url))]
    fn set_url(&self, py: Python<'_>, url: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_resource_transform_response_set_url",
            self.native as u64,
        )?;
        let url_view = storage.buffer(url.clone(), true)?;
        let handle = self.scope.pointer(self.native)? as *mut sys::mln_resource_transform_response;
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_resource_transform_response_set_url(
                    handle,
                    url_view.data.cast(),
                    url_view.size,
                )
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(py.None())
    }
}

#[pymethods]
impl RuntimeHandle {
    #[pyo3(signature = (options=None))]
    fn map_create(&self, py: Python<'_>, options: Option<Bound<'_, PyAny>>) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_map_create",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let options = options.unwrap_or_else(|| py.None().into_bound(py));
        let options_value = if options.clone().is_none() {
            unsafe { sys::mln_map_options_default() }
        } else {
            generated_input_mln_map_options(&options.clone(), storage)?
        };
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_owned_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_map_create(handle, &options_value, completion)
                })
            },
            |py, result| {
                let raw = completion_value::<sys::mln_map>(result)?;
                let state = unsafe { NativeHandleState::from_handle(raw, "mln_map") }
                    .map_err(map_error)?
                    .with_disposal(generated_dispose_mln_map);
                Py::new(
                    py,
                    MapHandle {
                        state: Arc::new(Mutex::new(state)),
                    },
                )
                .map(|value| value.into_any())
            },
            |result| {
                if !result.value.is_null() && result.value_count == 1 {
                    unsafe {
                        generated_dispose_mln_map(result.value.cast::<sys::mln_map>().read());
                    }
                }
            },
        )
    }
    #[pyo3(signature = ())]
    fn barrier(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_barrier",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || sys::mln_runtime_barrier(handle, completion))
            },
            py_none,
        )
    }
    #[pyo3(signature = ())]
    fn clear_http_header_transform(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_clear_http_header_transform",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_clear_http_header_transform(handle, completion)
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = ())]
    fn clear_resource_provider(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_clear_resource_provider",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_clear_resource_provider(handle, completion)
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = ())]
    fn clear_resource_transform(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_clear_resource_transform",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_clear_resource_transform(handle, completion)
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = ())]
    fn drain_events(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_drain_events",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_batch: sys::mln_event_batch = unsafe { std::mem::zeroed() };
        let result = unsafe {
            generated_native_call(py, || sys::mln_runtime_drain_events(handle, &mut out_batch))
        };
        maplibre_core::check(result).map_err(map_error)?;
        Py::new(
            py,
            EventBatchHandle {
                state: Arc::new(Mutex::new(
                    unsafe { NativeHandleState::from_handle(out_batch, "mln_event_batch") }
                        .map_err(map_error)?
                        .with_disposal(generated_dispose_mln_event_batch),
                )),
            },
        )
        .map(|value| value.into_any())
    }
    #[pyo3(signature = ())]
    fn get_event_mask(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_get_event_mask",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let mut out_mask: sys::mln_runtime_event_mask = unsafe { std::mem::zeroed() };
        let result = unsafe {
            generated_native_call(py, || {
                sys::mln_runtime_get_event_mask(handle, &mut out_mask)
            })
        };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(pyo3::BoundObject::unbind((out_mask).into_pyobject(py)?).into_any())
    }
    #[pyo3(signature = (definition, metadata))]
    fn offline_region_create(
        &self,
        py: Python<'_>,
        definition: &Bound<'_, PyAny>,
        metadata: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_runtime_offline_region_create",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let definition_value =
            generated_input_mln_offline_region_definition(&definition.clone(), storage)?;
        let metadata_view = storage.buffer(metadata.clone(), false)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_offline_region_create(
                        handle,
                        &definition_value,
                        metadata_view.data.cast(),
                        metadata_view.size,
                        completion,
                    )
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_offline_region_info>(result)?;
                Ok(generated_copy_mln_offline_region_info(py, &value)?)
            },
        )
    }
    #[pyo3(signature = (region_id))]
    fn offline_region_delete(&self, py: Python<'_>, region_id: i64) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_offline_region_delete",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_offline_region_delete(handle, region_id, completion)
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = (region_id))]
    fn offline_region_get(&self, py: Python<'_>, region_id: i64) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_offline_region_get",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_offline_region_get(handle, region_id, completion)
                })
            },
            |py, result| {
                if result.value.is_null() {
                    return Ok(py.None());
                }
                let value = completion_value::<sys::mln_offline_region_info>(result)?;
                Ok(generated_copy_mln_offline_region_info(py, &value)?)
            },
        )
    }
    #[pyo3(signature = (region_id))]
    fn offline_region_get_status(&self, py: Python<'_>, region_id: i64) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_offline_region_get_status",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_offline_region_get_status(handle, region_id, completion)
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_offline_region_status>(result)?;
                Ok(generated_copy_mln_offline_region_status(py, &value)?)
            },
        )
    }
    #[pyo3(signature = (region_id))]
    fn offline_region_invalidate(&self, py: Python<'_>, region_id: i64) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_offline_region_invalidate",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_offline_region_invalidate(handle, region_id, completion)
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = (region_id, input_state))]
    fn offline_region_set_download_state(
        &self,
        py: Python<'_>,
        region_id: i64,
        input_state: sys::mln_offline_region_download_state,
    ) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_offline_region_set_download_state",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_offline_region_set_download_state(
                        handle,
                        region_id,
                        input_state,
                        completion,
                    )
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = (region_id, observed))]
    fn offline_region_set_observed(
        &self,
        py: Python<'_>,
        region_id: i64,
        observed: bool,
    ) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_offline_region_set_observed",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_offline_region_set_observed(
                        handle, region_id, observed, completion,
                    )
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = (region_id, metadata))]
    fn offline_region_update_metadata(
        &self,
        py: Python<'_>,
        region_id: i64,
        metadata: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_runtime_offline_region_update_metadata",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let metadata_view = storage.buffer(metadata.clone(), false)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_offline_region_update_metadata(
                        handle,
                        region_id,
                        metadata_view.data.cast(),
                        metadata_view.size,
                        completion,
                    )
                })
            },
            |py, result| {
                let value = completion_value::<sys::mln_offline_region_info>(result)?;
                Ok(generated_copy_mln_offline_region_info(py, &value)?)
            },
        )
    }
    #[pyo3(signature = ())]
    fn offline_regions_list(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_offline_regions_list",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_offline_regions_list(handle, completion)
                })
            },
            |py, result| {
                let list = PyList::empty(py);
                for value in generated_completion_slice::<sys::mln_offline_region_info>(result)? {
                    list.append(generated_copy_mln_offline_region_info(py, &(*value))?)?;
                }
                Ok(list.into_any().unbind())
            },
        )
    }
    #[pyo3(signature = (side_database_path))]
    fn offline_regions_merge_database(
        &self,
        py: Python<'_>,
        side_database_path: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_runtime_offline_regions_merge_database",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let side_database_path_value = storage.c_string(side_database_path.clone())?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_offline_regions_merge_database(
                        handle,
                        side_database_path_value,
                        completion,
                    )
                })
            },
            |py, result| {
                let list = PyList::empty(py);
                for value in generated_completion_slice::<sys::mln_offline_region_info>(result)? {
                    list.append(generated_copy_mln_offline_region_info(py, &(*value))?)?;
                }
                Ok(list.into_any().unbind())
            },
        )
    }
    #[pyo3(signature = ())]
    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_release",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let Some(mut reservation) = GeneratedHandleReservation::new(&self.state)? else {
            return completed_python_future(py);
        };
        let handle = reservation.handle();
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || sys::mln_runtime_release(handle, completion))
            },
            py_none,
        )?;
        reservation.commit();
        Ok(future)
    }
    #[pyo3(signature = (operation))]
    fn run_ambient_cache_operation(
        &self,
        py: Python<'_>,
        operation: sys::mln_ambient_cache_operation,
    ) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_run_ambient_cache_operation",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_run_ambient_cache_operation(handle, operation, completion)
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = (mask))]
    fn set_event_mask(
        &self,
        py: Python<'_>,
        mask: sys::mln_runtime_event_mask,
    ) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_set_event_mask",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let result =
            unsafe { generated_native_call(py, || sys::mln_runtime_set_event_mask(handle, mask)) };
        maplibre_core::check(result).map_err(map_error)?;
        Ok(py.None())
    }
    #[pyo3(signature = (transform))]
    fn set_http_header_transform(
        &self,
        py: Python<'_>,
        transform: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_runtime_set_http_header_transform",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let transform_value =
            generated_input_mln_http_header_transform(&transform.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_set_http_header_transform(handle, &transform_value, completion)
                })
            },
            py_none,
        )?;
        let callback_roots = storage.accept_callbacks();
        self.state().retain_callback_roots(callback_roots);
        Ok(future)
    }
    #[pyo3(signature = (size))]
    fn set_maximum_ambient_cache_size(&self, py: Python<'_>, size: u64) -> PyResult<Py<PyAny>> {
        generated_check_operation(
            "mln_runtime_set_maximum_ambient_cache_size",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_set_maximum_ambient_cache_size(handle, size, completion)
                })
            },
            py_none,
        )
    }
    #[pyo3(signature = (provider))]
    fn set_resource_provider(
        &self,
        py: Python<'_>,
        provider: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_runtime_set_resource_provider",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let provider_value = generated_input_mln_resource_provider(&provider.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_set_resource_provider(handle, &provider_value, completion)
                })
            },
            py_none,
        )?;
        let callback_roots = storage.accept_callbacks();
        self.state().retain_callback_roots(callback_roots);
        Ok(future)
    }
    #[pyo3(signature = (transform))]
    fn set_resource_transform(
        &self,
        py: Python<'_>,
        transform: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let storage = &mut GeneratedInputStorage::default();
        generated_check_operation(
            "mln_runtime_set_resource_transform",
            self.state()
                .live_handle()
                .map(maplibre_core::handle::NativeHandle::to_raw)
                .unwrap_or(0),
        )?;
        let transform_value = generated_input_mln_resource_transform(&transform.clone(), storage)?;
        let handle = self
            .state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        let future = submit_python_future(
            py,
            |completion| unsafe {
                generated_native_call(py, || {
                    sys::mln_runtime_set_resource_transform(handle, &transform_value, completion)
                })
            },
            py_none,
        )?;
        let callback_roots = storage.accept_callbacks();
        self.state().retain_callback_roots(callback_roots);
        Ok(future)
    }
}

#[pyclass(name = "_AcquiredFrameHandle")]
struct AcquiredFrameHandle {
    state: Arc<Mutex<NativeHandleState<sys::mln_acquired_frame>>>,
}
impl AcquiredFrameHandle {
    fn state(&self) -> MutexGuard<'_, NativeHandleState<sys::mln_acquired_frame>> {
        self.state.lock().unwrap_or_else(|p| p.into_inner())
    }
}
#[pymethods]
impl AcquiredFrameHandle {
    #[getter]
    fn closed(&self) -> bool {
        self.state().is_closed()
    }
    #[getter]
    fn id(&self) -> u64 {
        self.state().issued_id()
    }
    fn __traverse__(&self, visit: pyo3::gc::PyVisit<'_>) -> Result<(), pyo3::gc::PyTraverseError> {
        self.state().traverse_callbacks(&visit)
    }
    fn __clear__(&self) {
        let callbacks = self.state().take_callbacks();
        drop(callbacks);
    }
    fn _read_scope(&self, py: Python<'_>) -> PyResult<GeneratedReadScope> {
        let _ = py;
        generated_check_reentry()?;
        GeneratedReadScope::with_native::<sys::mln_acquired_frame, _>(
            py,
            Arc::clone(&self.state),
            sys::mln_adapter_acquired_frame_view_begin,
            sys::mln_adapter_acquired_frame_view_end,
        )
    }
}

#[pyclass(name = "_BufferHandle")]
struct BufferHandle {
    state: Arc<Mutex<NativeHandleState<sys::mln_buffer>>>,
}
impl BufferHandle {
    fn state(&self) -> MutexGuard<'_, NativeHandleState<sys::mln_buffer>> {
        self.state.lock().unwrap_or_else(|p| p.into_inner())
    }
}
#[pymethods]
impl BufferHandle {
    #[getter]
    fn closed(&self) -> bool {
        self.state().is_closed()
    }
    #[getter]
    fn id(&self) -> u64 {
        self.state().issued_id()
    }
    fn __traverse__(&self, visit: pyo3::gc::PyVisit<'_>) -> Result<(), pyo3::gc::PyTraverseError> {
        self.state().traverse_callbacks(&visit)
    }
    fn __clear__(&self) {
        let callbacks = self.state().take_callbacks();
        drop(callbacks);
    }
    fn _read_scope(&self, py: Python<'_>) -> PyResult<GeneratedReadScope> {
        let _ = py;
        generated_check_reentry()?;
        GeneratedReadScope::new::<sys::mln_buffer, _>(Arc::clone(&self.state))
    }
}

#[pyclass(name = "_EventBatchHandle")]
struct EventBatchHandle {
    state: Arc<Mutex<NativeHandleState<sys::mln_event_batch>>>,
}
impl EventBatchHandle {
    fn state(&self) -> MutexGuard<'_, NativeHandleState<sys::mln_event_batch>> {
        self.state.lock().unwrap_or_else(|p| p.into_inner())
    }
}
#[pymethods]
impl EventBatchHandle {
    #[getter]
    fn closed(&self) -> bool {
        self.state().is_closed()
    }
    #[getter]
    fn id(&self) -> u64 {
        self.state().issued_id()
    }
    fn __traverse__(&self, visit: pyo3::gc::PyVisit<'_>) -> Result<(), pyo3::gc::PyTraverseError> {
        self.state().traverse_callbacks(&visit)
    }
    fn __clear__(&self) {
        let callbacks = self.state().take_callbacks();
        drop(callbacks);
    }
    fn _read_scope(&self, py: Python<'_>) -> PyResult<GeneratedReadScope> {
        let _ = py;
        generated_check_reentry()?;
        GeneratedReadScope::new::<sys::mln_event_batch, _>(Arc::clone(&self.state))
    }
}

#[pyclass(name = "_GeojsonSourceDataHandle")]
struct GeojsonSourceDataHandle {
    state: Arc<Mutex<NativeHandleState<sys::mln_geojson_source_data>>>,
}
impl GeojsonSourceDataHandle {
    fn state(&self) -> MutexGuard<'_, NativeHandleState<sys::mln_geojson_source_data>> {
        self.state.lock().unwrap_or_else(|p| p.into_inner())
    }
}
#[pymethods]
impl GeojsonSourceDataHandle {
    #[getter]
    fn closed(&self) -> bool {
        self.state().is_closed()
    }
    #[getter]
    fn id(&self) -> u64 {
        self.state().issued_id()
    }
    fn __traverse__(&self, visit: pyo3::gc::PyVisit<'_>) -> Result<(), pyo3::gc::PyTraverseError> {
        self.state().traverse_callbacks(&visit)
    }
    fn __clear__(&self) {
        let callbacks = self.state().take_callbacks();
        drop(callbacks);
    }
    fn _read_scope(&self, py: Python<'_>) -> PyResult<GeneratedReadScope> {
        let _ = py;
        generated_check_reentry()?;
        GeneratedReadScope::new::<sys::mln_geojson_source_data, _>(Arc::clone(&self.state))
    }
}

#[pyclass(name = "_MapHandle")]
struct MapHandle {
    state: Arc<Mutex<NativeHandleState<sys::mln_map>>>,
}
impl MapHandle {
    fn state(&self) -> MutexGuard<'_, NativeHandleState<sys::mln_map>> {
        self.state.lock().unwrap_or_else(|p| p.into_inner())
    }
}
#[pymethods]
impl MapHandle {
    #[getter]
    fn closed(&self) -> bool {
        self.state().is_closed()
    }
    #[getter]
    fn id(&self) -> u64 {
        self.state().issued_id()
    }
    fn __traverse__(&self, visit: pyo3::gc::PyVisit<'_>) -> Result<(), pyo3::gc::PyTraverseError> {
        self.state().traverse_callbacks(&visit)
    }
    fn __clear__(&self) {
        let callbacks = self.state().take_callbacks();
        drop(callbacks);
    }
    fn _read_scope(&self, py: Python<'_>) -> PyResult<GeneratedReadScope> {
        let _ = py;
        generated_check_reentry()?;
        GeneratedReadScope::new::<sys::mln_map, _>(Arc::clone(&self.state))
    }
}

#[pyclass(name = "_MapProjectionHandle")]
struct MapProjectionHandle {
    state: Arc<Mutex<NativeHandleState<sys::mln_map_projection>>>,
}
impl MapProjectionHandle {
    fn state(&self) -> MutexGuard<'_, NativeHandleState<sys::mln_map_projection>> {
        self.state.lock().unwrap_or_else(|p| p.into_inner())
    }
}
#[pymethods]
impl MapProjectionHandle {
    #[getter]
    fn closed(&self) -> bool {
        self.state().is_closed()
    }
    #[getter]
    fn id(&self) -> u64 {
        self.state().issued_id()
    }
    fn __traverse__(&self, visit: pyo3::gc::PyVisit<'_>) -> Result<(), pyo3::gc::PyTraverseError> {
        self.state().traverse_callbacks(&visit)
    }
    fn __clear__(&self) {
        let callbacks = self.state().take_callbacks();
        drop(callbacks);
    }
    fn _read_scope(&self, py: Python<'_>) -> PyResult<GeneratedReadScope> {
        let _ = py;
        generated_check_reentry()?;
        GeneratedReadScope::new::<sys::mln_map_projection, _>(Arc::clone(&self.state))
    }
}

#[pyclass(name = "_RenderFrameBatchHandle")]
struct RenderFrameBatchHandle {
    state: Arc<Mutex<NativeHandleState<sys::mln_render_frame_batch>>>,
}
impl RenderFrameBatchHandle {
    fn state(&self) -> MutexGuard<'_, NativeHandleState<sys::mln_render_frame_batch>> {
        self.state.lock().unwrap_or_else(|p| p.into_inner())
    }
}
#[pymethods]
impl RenderFrameBatchHandle {
    #[getter]
    fn closed(&self) -> bool {
        self.state().is_closed()
    }
    #[getter]
    fn id(&self) -> u64 {
        self.state().issued_id()
    }
    fn __traverse__(&self, visit: pyo3::gc::PyVisit<'_>) -> Result<(), pyo3::gc::PyTraverseError> {
        self.state().traverse_callbacks(&visit)
    }
    fn __clear__(&self) {
        let callbacks = self.state().take_callbacks();
        drop(callbacks);
    }
    fn _read_scope(&self, py: Python<'_>) -> PyResult<GeneratedReadScope> {
        let _ = py;
        generated_check_reentry()?;
        GeneratedReadScope::new::<sys::mln_render_frame_batch, _>(Arc::clone(&self.state))
    }
}

#[pyclass(name = "_RenderSessionHandle")]
struct RenderSessionHandle {
    state: Arc<Mutex<NativeHandleState<sys::mln_render_session>>>,
}
impl RenderSessionHandle {
    fn state(&self) -> MutexGuard<'_, NativeHandleState<sys::mln_render_session>> {
        self.state.lock().unwrap_or_else(|p| p.into_inner())
    }
}
#[pymethods]
impl RenderSessionHandle {
    #[getter]
    fn closed(&self) -> bool {
        self.state().is_closed()
    }
    #[getter]
    fn id(&self) -> u64 {
        self.state().issued_id()
    }
    fn __traverse__(&self, visit: pyo3::gc::PyVisit<'_>) -> Result<(), pyo3::gc::PyTraverseError> {
        self.state().traverse_callbacks(&visit)
    }
    fn __clear__(&self) {
        let callbacks = self.state().take_callbacks();
        drop(callbacks);
    }
    fn _read_scope(&self, py: Python<'_>) -> PyResult<GeneratedReadScope> {
        let _ = py;
        generated_check_reentry()?;
        GeneratedReadScope::new::<sys::mln_render_session, _>(Arc::clone(&self.state))
    }
}

#[pyclass(name = "_ResourceRequestHandle")]
struct ResourceRequestHandle {
    // The accepted cancel callback, which native owns until it retires.
    cancel_root: Mutex<std::sync::Weak<GeneratedCallbackRoot>>,
    // Dropped by hand, with the GIL released; see the Drop impl below.
    state: ManuallyDrop<Arc<maplibre_core::resource::ResourceRequestHandleState>>,
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

#[pyclass(name = "_RuntimeHandle")]
struct RuntimeHandle {
    state: Arc<Mutex<NativeHandleState<sys::mln_runtime>>>,
}
impl RuntimeHandle {
    fn state(&self) -> MutexGuard<'_, NativeHandleState<sys::mln_runtime>> {
        self.state.lock().unwrap_or_else(|p| p.into_inner())
    }
}
#[pymethods]
impl RuntimeHandle {
    #[getter]
    fn closed(&self) -> bool {
        self.state().is_closed()
    }
    #[getter]
    fn id(&self) -> u64 {
        self.state().issued_id()
    }
    fn __traverse__(&self, visit: pyo3::gc::PyVisit<'_>) -> Result<(), pyo3::gc::PyTraverseError> {
        self.state().traverse_callbacks(&visit)
    }
    fn __clear__(&self) {
        let callbacks = self.state().take_callbacks();
        drop(callbacks);
    }
    fn _read_scope(&self, py: Python<'_>) -> PyResult<GeneratedReadScope> {
        let _ = py;
        generated_check_reentry()?;
        GeneratedReadScope::new::<sys::mln_runtime, _>(Arc::clone(&self.state))
    }
}
#[pyfunction]
#[pyo3(signature = (jni_env, jni_class, context))]
fn android_init(
    py: Python<'_>,
    jni_env: &Bound<'_, PyAny>,
    jni_class: &Bound<'_, PyAny>,
    context: &Bound<'_, PyAny>,
) -> PyResult<Py<PyAny>> {
    generated_check_operation("mln_android_init", 0)?;
    let jni_env_value = jni_env.clone().extract::<usize>()? as _;
    let jni_class_value = jni_class.clone().extract::<usize>()? as _;
    let context_value = context.clone().extract::<usize>()? as _;
    let result = unsafe {
        generated_native_call(py, || {
            sys::mln_android_init(jni_env_value, jni_class_value, context_value)
        })
    };
    maplibre_core::check(result).map_err(map_error)?;
    Ok(py.None())
}

#[pyfunction]
#[pyo3(signature = ())]
fn c_version(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_operation("mln_c_version", 0)?;
    let result = unsafe { generated_native_call(py, || sys::mln_c_version()) };
    Ok(pyo3::BoundObject::unbind((result).into_pyobject(py)?).into_any())
}

#[pyfunction]
#[pyo3(signature = (data, options=None))]
fn geojson_source_data_create(
    py: Python<'_>,
    data: &Bound<'_, PyAny>,
    options: Option<Bound<'_, PyAny>>,
) -> PyResult<Py<PyAny>> {
    let storage = &mut GeneratedInputStorage::default();
    generated_check_operation("mln_geojson_source_data_create", 0)?;
    let data_value = storage.buffer(data.clone(), false)?;
    let options = options.unwrap_or_else(|| py.None().into_bound(py));
    let options_value = if options.is_none() {
        None
    } else {
        Some(if options.clone().is_none() {
            unsafe { sys::mln_geojson_source_options_default() }
        } else {
            generated_input_mln_geojson_source_options(&options.clone(), storage)?
        })
    };
    let mut out_data: sys::mln_geojson_source_data = unsafe { std::mem::zeroed() };
    let result = unsafe {
        generated_native_call(py, || {
            sys::mln_geojson_source_data_create(
                data_value,
                options_value
                    .as_ref()
                    .map_or(std::ptr::null(), |value| value),
                &mut out_data,
            )
        })
    };
    maplibre_core::check(result).map_err(map_error)?;
    Py::new(
        py,
        GeojsonSourceDataHandle {
            state: Arc::new(Mutex::new(
                unsafe { NativeHandleState::from_handle(out_data, "mln_geojson_source_data") }
                    .map_err(map_error)?
                    .with_disposal(generated_dispose_mln_geojson_source_data),
            )),
        },
    )
    .map(|value| value.into_any())
}

#[pyfunction]
#[pyo3(signature = (meters))]
fn lat_lng_for_projected_meters(py: Python<'_>, meters: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
    let storage = &mut GeneratedInputStorage::default();
    generated_check_operation("mln_lat_lng_for_projected_meters", 0)?;
    let meters_value = generated_input_mln_projected_meters(&meters.clone(), storage)?;
    let mut out_coordinate: sys::mln_lat_lng = unsafe { std::mem::zeroed() };
    let result = unsafe {
        generated_native_call(py, || {
            sys::mln_lat_lng_for_projected_meters(meters_value, &mut out_coordinate)
        })
    };
    maplibre_core::check(result).map_err(map_error)?;
    Ok(generated_copy_mln_lat_lng(py, &out_coordinate)?)
}

#[pyfunction]
#[pyo3(signature = ())]
fn log_clear_callback(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_operation("mln_log_clear_callback", 0)?;
    let result = unsafe { generated_native_call(py, || sys::mln_log_clear_callback()) };
    maplibre_core::check(result).map_err(map_error)?;
    Ok(py.None())
}

#[pyfunction]
#[pyo3(signature = (mask))]
fn log_set_async_severity_mask(
    py: Python<'_>,
    mask: sys::mln_log_severity_mask,
) -> PyResult<Py<PyAny>> {
    generated_check_operation("mln_log_set_async_severity_mask", 0)?;
    let result =
        unsafe { generated_native_call(py, || sys::mln_log_set_async_severity_mask(mask)) };
    maplibre_core::check(result).map_err(map_error)?;
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
    let status = unsafe {
        generated_native_call(py, || {
            sys::mln_log_set_callback(native_callback, context, release)
        })
    };
    maplibre_core::check(status).map_err(map_error)?;
    storage.accept_callbacks();
    Ok(())
}

#[pyfunction]
#[pyo3(signature = ())]
fn network_status_get(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_operation("mln_network_status_get", 0)?;
    let mut out_status: sys::mln_network_status = unsafe { std::mem::zeroed() };
    let result =
        unsafe { generated_native_call(py, || sys::mln_network_status_get(&mut out_status)) };
    maplibre_core::check(result).map_err(map_error)?;
    Ok(pyo3::BoundObject::unbind((out_status).into_pyobject(py)?).into_any())
}

#[pyfunction]
#[pyo3(signature = (input_status))]
fn network_status_set(
    py: Python<'_>,
    input_status: sys::mln_network_status,
) -> PyResult<Py<PyAny>> {
    generated_check_operation("mln_network_status_set", 0)?;
    let result = unsafe { generated_native_call(py, || sys::mln_network_status_set(input_status)) };
    maplibre_core::check(result).map_err(map_error)?;
    Ok(py.None())
}

#[pyfunction]
#[pyo3(signature = ())]
fn opengl_supported_context_provider_mask(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_operation("mln_opengl_supported_context_provider_mask", 0)?;
    let result =
        unsafe { generated_native_call(py, || sys::mln_opengl_supported_context_provider_mask()) };
    Ok(pyo3::BoundObject::unbind((result).into_pyobject(py)?).into_any())
}

#[pyfunction]
#[pyo3(signature = ())]
fn plugin_get_register_function_v1(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_operation("mln_plugin_get_register_function_v1", 0)?;
    let result =
        unsafe { generated_native_call(py, || sys::mln_plugin_get_register_function_v1()) };
    Ok(pyo3::BoundObject::unbind(
        (result.map_or(0, |function| function as usize)).into_pyobject(py)?,
    )
    .into_any())
}

#[pyfunction]
#[pyo3(signature = (coordinate))]
fn projected_meters_for_lat_lng(
    py: Python<'_>,
    coordinate: &Bound<'_, PyAny>,
) -> PyResult<Py<PyAny>> {
    let storage = &mut GeneratedInputStorage::default();
    generated_check_operation("mln_projected_meters_for_lat_lng", 0)?;
    let coordinate_value = generated_input_mln_lat_lng(&coordinate.clone(), storage)?;
    let mut out_meters: sys::mln_projected_meters = unsafe { std::mem::zeroed() };
    let result = unsafe {
        generated_native_call(py, || {
            sys::mln_projected_meters_for_lat_lng(coordinate_value, &mut out_meters)
        })
    };
    maplibre_core::check(result).map_err(map_error)?;
    Ok(generated_copy_mln_projected_meters(py, &out_meters)?)
}

#[pyfunction]
#[pyo3(signature = (extent))]
fn render_target_extent_physical_size(
    py: Python<'_>,
    extent: &Bound<'_, PyAny>,
) -> PyResult<Py<PyAny>> {
    let storage = &mut GeneratedInputStorage::default();
    generated_check_operation("mln_render_target_extent_physical_size", 0)?;
    let extent_value = generated_input_mln_render_target_extent(&extent.clone(), storage)?;
    let mut out_width: u32 = unsafe { std::mem::zeroed() };
    let mut out_height: u32 = unsafe { std::mem::zeroed() };
    let result = unsafe {
        generated_native_call(py, || {
            sys::mln_render_target_extent_physical_size(
                &extent_value,
                &mut out_width,
                &mut out_height,
            )
        })
    };
    maplibre_core::check(result).map_err(map_error)?;
    let dict = PyDict::new(py);
    dict.set_item(
        "width",
        pyo3::BoundObject::unbind((out_width).into_pyobject(py)?).into_any(),
    )?;
    dict.set_item(
        "height",
        pyo3::BoundObject::unbind((out_height).into_pyobject(py)?).into_any(),
    )?;
    Ok(dict.into_any().unbind())
}

#[pyfunction]
#[pyo3(signature = (input_box))]
fn rendered_query_geometry_box(
    py: Python<'_>,
    input_box: &Bound<'_, PyAny>,
) -> PyResult<Py<PyAny>> {
    let storage = &mut GeneratedInputStorage::default();
    generated_check_operation("mln_rendered_query_geometry_box", 0)?;
    let input_box_value = generated_input_mln_screen_box(&input_box.clone(), storage)?;
    let result = unsafe {
        generated_native_call(py, || sys::mln_rendered_query_geometry_box(input_box_value))
    };
    Ok(generated_copy_mln_rendered_query_geometry(py, &result)?)
}

#[pyfunction]
#[pyo3(signature = (points))]
fn rendered_query_geometry_line_string(
    py: Python<'_>,
    points: &Bound<'_, PyAny>,
) -> PyResult<Py<PyAny>> {
    let storage = &mut GeneratedInputStorage::default();
    generated_check_operation("mln_rendered_query_geometry_line_string", 0)?;
    let mut points_values = Vec::new();
    for item in points.try_iter()? {
        let item = item?;
        points_values.push(generated_input_mln_screen_point(&item, storage)?);
    }
    let result = unsafe {
        generated_native_call(py, || {
            sys::mln_rendered_query_geometry_line_string(
                points_values.as_ptr(),
                points_values.len(),
            )
        })
    };
    Ok(generated_copy_mln_rendered_query_geometry(py, &result)?)
}

#[pyfunction]
#[pyo3(signature = (point))]
fn rendered_query_geometry_point(py: Python<'_>, point: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
    let storage = &mut GeneratedInputStorage::default();
    generated_check_operation("mln_rendered_query_geometry_point", 0)?;
    let point_value = generated_input_mln_screen_point(&point.clone(), storage)?;
    let result = unsafe {
        generated_native_call(py, || sys::mln_rendered_query_geometry_point(point_value))
    };
    Ok(generated_copy_mln_rendered_query_geometry(py, &result)?)
}

#[pyfunction]
#[pyo3(signature = (options=None))]
fn runtime_create(py: Python<'_>, options: Option<Bound<'_, PyAny>>) -> PyResult<Py<PyAny>> {
    let storage = &mut GeneratedInputStorage::default();
    generated_check_operation("mln_runtime_create", 0)?;
    let options = options.unwrap_or_else(|| py.None().into_bound(py));
    let options_value = if options.clone().is_none() {
        unsafe { sys::mln_runtime_options_default() }
    } else {
        generated_input_mln_runtime_options(&options.clone(), storage)?
    };
    let mut out_runtime: sys::mln_runtime = unsafe { std::mem::zeroed() };
    let result = unsafe {
        generated_native_call(py, || {
            sys::mln_runtime_create(&options_value, &mut out_runtime)
        })
    };
    maplibre_core::check(result).map_err(map_error)?;
    let callback_roots = storage.accept_callbacks();
    Py::new(
        py,
        RuntimeHandle {
            state: Arc::new(Mutex::new(
                unsafe { NativeHandleState::from_handle(out_runtime, "mln_runtime") }
                    .map_err(map_error)?
                    .with_disposal(generated_dispose_mln_runtime)
                    .with_callback_roots(callback_roots.clone()),
            )),
        },
    )
    .map(|value| value.into_any())
}

#[pyfunction]
#[pyo3(signature = ())]
fn supported_render_backend_mask(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_operation("mln_supported_render_backend_mask", 0)?;
    let result = unsafe { generated_native_call(py, || sys::mln_supported_render_backend_mask()) };
    Ok(pyo3::BoundObject::unbind((result).into_pyobject(py)?).into_any())
}

#[pyfunction]
#[pyo3(signature = ())]
fn thread_last_error_message(py: Python<'_>) -> PyResult<Py<PyAny>> {
    generated_check_operation("mln_thread_last_error_message", 0)?;
    let result = unsafe { generated_native_call(py, || sys::mln_thread_last_error_message()) };
    Ok({
        if result.is_null() {
            return Err(native_error("null native string"));
        }
        unsafe { std::ffi::CStr::from_ptr(result) }
            .to_str()
            .map_err(|_| native_error("native string is not UTF-8"))?
            .into_pyobject(py)?
            .into_any()
            .unbind()
    })
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
    module.add_function(wrap_pyfunction!(thread_last_error_message, module)?)?;
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
