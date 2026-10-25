// Generated from the C headers by tools/bindgen. Do not edit.
#include <stdlib.h>
#include <string.h>

#include "generated_addon.h"

napi_value mln_ts_decode_buffer_view(
  napi_env env, const mln_completion_result* result
) {
  if (result->value == NULL || result->value_count == 0)
    return mln_ts_missing_value(env);
  if (result->value_size < sizeof(mln_buffer_view)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  const char* value = (const char*)result->value;
  size_t stride = result->value_size;
  (void)stride;
  return mln_ts_text(
    env, (*(const mln_buffer_view*)value).data,
    (*(const mln_buffer_view*)value).size
  );
}

napi_value mln_ts_decode_buffer_view_optional(
  napi_env env, const mln_completion_result* result
) {
  if (result->value == NULL || result->value_count == 0)
    return mln_ts_undefined(env);
  if (result->value_size < sizeof(mln_buffer_view)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  const char* value = (const char*)result->value;
  size_t stride = result->value_size;
  (void)stride;
  return (
    (*(const mln_buffer_view*)value).data == NULL
      ? mln_ts_undefined(env)
      : mln_ts_text(
          env, (*(const mln_buffer_view*)value).data,
          (*(const mln_buffer_view*)value).size
        )
  );
}

napi_value mln_ts_decode_camera_options(
  napi_env env, const mln_completion_result* result
) {
  if (result->value == NULL || result->value_count == 0)
    return mln_ts_missing_value(env);
  if (result->value_size < sizeof(mln_camera_options)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  const char* value = (const char*)result->value;
  size_t stride = result->value_size;
  (void)stride;
  return mln_ts_read_camera_options(
    env, &(*(const mln_camera_options*)value), stride
  );
}

napi_value mln_ts_decode_camera_query_result(
  napi_env env, const mln_completion_result* result
) {
  if (result->value == NULL || result->value_count == 0)
    return mln_ts_missing_value(env);
  if (result->value_size < sizeof(mln_camera_query_result)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  const char* value = (const char*)result->value;
  size_t stride = result->value_size;
  (void)stride;
  return mln_ts_read_camera_query_result(
    env, &(*(const mln_camera_query_result*)value), stride
  );
}

napi_value mln_ts_decode_lat_lng_bounds(
  napi_env env, const mln_completion_result* result
) {
  if (result->value == NULL || result->value_count == 0)
    return mln_ts_missing_value(env);
  if (result->value_size < sizeof(mln_lat_lng_bounds)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  const char* value = (const char*)result->value;
  size_t stride = result->value_size;
  (void)stride;
  return mln_ts_read_lat_lng_bounds(
    env, &(*(const mln_lat_lng_bounds*)value), stride
  );
}

napi_value mln_ts_decode_lat_lng_list(
  napi_env env, const mln_completion_result* result
) {
  size_t stride = result->value_size;
  if (result->value_count > 0 && stride < sizeof(mln_lat_lng)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  napi_value array = mln_ts_array(env, result->value_count);
  if (array == NULL) return NULL;
  for (size_t index = 0; index < result->value_count; index++) {
    const char* value = (const char*)result->value + index * stride;
    if (!mln_ts_set_index(
          env, array, (uint32_t)index,
          mln_ts_read_lat_lng(env, &(*(const mln_lat_lng*)value), stride)
        ))
      return NULL;
  }
  return array;
}

napi_value mln_ts_decode_offline_region_info(
  napi_env env, const mln_completion_result* result
) {
  if (result->value == NULL || result->value_count == 0)
    return mln_ts_missing_value(env);
  if (result->value_size < sizeof(mln_offline_region_info)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  const char* value = (const char*)result->value;
  size_t stride = result->value_size;
  (void)stride;
  return mln_ts_read_offline_region_info(
    env, &(*(const mln_offline_region_info*)value), stride
  );
}

napi_value mln_ts_decode_offline_region_info_list(
  napi_env env, const mln_completion_result* result
) {
  size_t stride = result->value_size;
  if (result->value_count > 0 && stride < sizeof(mln_offline_region_info)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  napi_value array = mln_ts_array(env, result->value_count);
  if (array == NULL) return NULL;
  for (size_t index = 0; index < result->value_count; index++) {
    const char* value = (const char*)result->value + index * stride;
    if (!mln_ts_set_index(
          env, array, (uint32_t)index,
          mln_ts_read_offline_region_info(
            env, &(*(const mln_offline_region_info*)value), stride
          )
        ))
      return NULL;
  }
  return array;
}

napi_value mln_ts_decode_offline_region_info_optional(
  napi_env env, const mln_completion_result* result
) {
  if (result->value == NULL || result->value_count == 0)
    return mln_ts_undefined(env);
  if (result->value_size < sizeof(mln_offline_region_info)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  const char* value = (const char*)result->value;
  size_t stride = result->value_size;
  (void)stride;
  return mln_ts_read_offline_region_info(
    env, &(*(const mln_offline_region_info*)value), stride
  );
}

napi_value mln_ts_decode_offline_region_status(
  napi_env env, const mln_completion_result* result
) {
  if (result->value == NULL || result->value_count == 0)
    return mln_ts_missing_value(env);
  if (result->value_size < sizeof(mln_offline_region_status)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  const char* value = (const char*)result->value;
  size_t stride = result->value_size;
  (void)stride;
  return mln_ts_read_offline_region_status(
    env, &(*(const mln_offline_region_status*)value), stride
  );
}

napi_value mln_ts_decode_rendered_feature_list(
  napi_env env, const mln_completion_result* result
) {
  size_t stride = result->value_size;
  if (result->value_count > 0 && stride < sizeof(mln_rendered_feature)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  napi_value array = mln_ts_array(env, result->value_count);
  if (array == NULL) return NULL;
  for (size_t index = 0; index < result->value_count; index++) {
    const char* value = (const char*)result->value + index * stride;
    if (!mln_ts_set_index(
          env, array, (uint32_t)index,
          mln_ts_read_rendered_feature(
            env, &(*(const mln_rendered_feature*)value), stride
          )
        ))
      return NULL;
  }
  return array;
}

napi_value mln_ts_decode_source_feature_list(
  napi_env env, const mln_completion_result* result
) {
  size_t stride = result->value_size;
  if (result->value_count > 0 && stride < sizeof(mln_source_feature)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  napi_value array = mln_ts_array(env, result->value_count);
  if (array == NULL) return NULL;
  for (size_t index = 0; index < result->value_count; index++) {
    const char* value = (const char*)result->value + index * stride;
    if (!mln_ts_set_index(
          env, array, (uint32_t)index,
          mln_ts_read_source_feature(
            env, &(*(const mln_source_feature*)value), stride
          )
        ))
      return NULL;
  }
  return array;
}

napi_value mln_ts_decode_style_image_info_optional(
  napi_env env, const mln_completion_result* result
) {
  if (result->value == NULL || result->value_count == 0)
    return mln_ts_undefined(env);
  if (result->value_size < sizeof(mln_style_image_info)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  const char* value = (const char*)result->value;
  size_t stride = result->value_size;
  (void)stride;
  return mln_ts_read_style_image_info(
    env, &(*(const mln_style_image_info*)value), stride
  );
}

napi_value mln_ts_decode_style_layer_info_list(
  napi_env env, const mln_completion_result* result
) {
  size_t stride = result->value_size;
  if (result->value_count > 0 && stride < sizeof(mln_style_layer_info)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  napi_value array = mln_ts_array(env, result->value_count);
  if (array == NULL) return NULL;
  for (size_t index = 0; index < result->value_count; index++) {
    const char* value = (const char*)result->value + index * stride;
    if (!mln_ts_set_index(
          env, array, (uint32_t)index,
          mln_ts_read_style_layer_info(
            env, &(*(const mln_style_layer_info*)value), stride
          )
        ))
      return NULL;
  }
  return array;
}

napi_value mln_ts_decode_style_layer_info_optional(
  napi_env env, const mln_completion_result* result
) {
  if (result->value == NULL || result->value_count == 0)
    return mln_ts_undefined(env);
  if (result->value_size < sizeof(mln_style_layer_info)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  const char* value = (const char*)result->value;
  size_t stride = result->value_size;
  (void)stride;
  return mln_ts_read_style_layer_info(
    env, &(*(const mln_style_layer_info*)value), stride
  );
}

napi_value mln_ts_decode_style_source_info_list(
  napi_env env, const mln_completion_result* result
) {
  size_t stride = result->value_size;
  if (result->value_count > 0 && stride < sizeof(mln_style_source_info)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  napi_value array = mln_ts_array(env, result->value_count);
  if (array == NULL) return NULL;
  for (size_t index = 0; index < result->value_count; index++) {
    const char* value = (const char*)result->value + index * stride;
    if (!mln_ts_set_index(
          env, array, (uint32_t)index,
          mln_ts_read_style_source_info(
            env, &(*(const mln_style_source_info*)value), stride
          )
        ))
      return NULL;
  }
  return array;
}

napi_value mln_ts_decode_style_source_info_optional(
  napi_env env, const mln_completion_result* result
) {
  if (result->value == NULL || result->value_count == 0)
    return mln_ts_undefined(env);
  if (result->value_size < sizeof(mln_style_source_info)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  const char* value = (const char*)result->value;
  size_t stride = result->value_size;
  (void)stride;
  return mln_ts_read_style_source_info(
    env, &(*(const mln_style_source_info*)value), stride
  );
}

napi_value mln_ts_decode_style_transition_options(
  napi_env env, const mln_completion_result* result
) {
  if (result->value == NULL || result->value_count == 0)
    return mln_ts_missing_value(env);
  if (result->value_size < sizeof(mln_style_transition_options)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  const char* value = (const char*)result->value;
  size_t stride = result->value_size;
  (void)stride;
  return mln_ts_read_style_transition_options(
    env, &(*(const mln_style_transition_options*)value), stride
  );
}

napi_value mln_ts_decode_texture_readback_result(
  napi_env env, const mln_completion_result* result
) {
  if (result->value == NULL || result->value_count == 0)
    return mln_ts_missing_value(env);
  if (result->value_size < sizeof(mln_texture_readback_result)) {
    napi_throw_range_error(env, NULL, "native completion stride is too small");
    return NULL;
  }
  const char* value = (const char*)result->value;
  size_t stride = result->value_size;
  (void)stride;
  return mln_ts_read_texture_readback_result(
    env, &(*(const mln_texture_readback_result*)value), stride
  );
}

napi_value mln_ts_delivery_payload(napi_env env, const mln_delivery* delivery) {
  switch (delivery->type) {
    case MLN_DELIVERY_TYPE_CAMERA_TRANSITION_END:
      return mln_ts_read_camera_transition_end_delivery(
        env, &delivery->payload.camera_transition_end,
        sizeof delivery->payload.camera_transition_end
      );
    case MLN_DELIVERY_TYPE_CUSTOM_SOURCE_CANCEL_TILE:
      return mln_ts_read_custom_source_tile_delivery(
        env, &delivery->payload.custom_source_cancel_tile,
        sizeof delivery->payload.custom_source_cancel_tile
      );
    case MLN_DELIVERY_TYPE_CUSTOM_SOURCE_FETCH_TILE:
      return mln_ts_read_custom_source_tile_delivery(
        env, &delivery->payload.custom_source_fetch_tile,
        sizeof delivery->payload.custom_source_fetch_tile
      );
    case MLN_DELIVERY_TYPE_LOG_RECORD:
      return mln_ts_read_log_delivery(
        env, &delivery->payload.log_record, sizeof delivery->payload.log_record
      );
    case MLN_DELIVERY_TYPE_RESOURCE_REQUEST:
      return mln_ts_read_resource_request_delivery(
        env, &delivery->payload.resource_request,
        sizeof delivery->payload.resource_request
      );
    case MLN_DELIVERY_TYPE_RESOURCE_REQUEST_CANCEL:
      return mln_ts_read_resource_request_cancel_delivery(
        env, &delivery->payload.resource_request_cancel,
        sizeof delivery->payload.resource_request_cancel
      );
    case MLN_DELIVERY_TYPE_RESOURCE_TRANSFORM:
      return mln_ts_read_resource_transform_delivery(
        env, &delivery->payload.resource_transform,
        sizeof delivery->payload.resource_transform
      );
    case MLN_DELIVERY_TYPE_RUNTIME_EVENTS:
      return mln_ts_read_runtime_event_delivery(
        env, &delivery->payload.runtime_events,
        sizeof delivery->payload.runtime_events
      );
    default:
      return mln_ts_undefined(env);
  }
}

napi_value mln_ts_read_animation_options(
  napi_env env, const mln_animation_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_ANIMATION_OPTION_DURATION) != 0) {
    if (!mln_ts_set(
          env, result, "durationMs",
          mln_ts_number(env, (double)(source->duration_ms))
        ))
      return NULL;
  }
  if ((source->fields & MLN_ANIMATION_OPTION_VELOCITY) != 0) {
    if (!mln_ts_set(
          env, result, "velocity",
          mln_ts_number(env, (double)(source->velocity))
        ))
      return NULL;
  }
  if ((source->fields & MLN_ANIMATION_OPTION_MIN_ZOOM) != 0) {
    if (!mln_ts_set(
          env, result, "minZoom", mln_ts_number(env, (double)(source->min_zoom))
        ))
      return NULL;
  }
  if ((source->fields & MLN_ANIMATION_OPTION_EASING) != 0) {
    if (!mln_ts_set(
          env, result, "easing",
          mln_ts_read_unit_bezier(
            env, &(source->easing), sizeof(source->easing)
          )
        ))
      return NULL;
  }
  if ((source->fields & MLN_ANIMATION_OPTION_TRANSITION_ID) != 0) {
    if (!mln_ts_set(
          env, result, "transitionId",
          mln_ts_u64(env, (uint64_t)(source->transition_id))
        ))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_bound_options(
  napi_env env, const mln_bound_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_BOUND_OPTION_BOUNDS) != 0) {
    if (!mln_ts_set(
          env, result, "bounds",
          mln_ts_read_lat_lng_bounds(
            env, &(source->bounds), sizeof(source->bounds)
          )
        ))
      return NULL;
  }
  if ((source->fields & MLN_BOUND_OPTION_MIN_ZOOM) != 0) {
    if (!mln_ts_set(
          env, result, "minZoom", mln_ts_number(env, (double)(source->min_zoom))
        ))
      return NULL;
  }
  if ((source->fields & MLN_BOUND_OPTION_MAX_ZOOM) != 0) {
    if (!mln_ts_set(
          env, result, "maxZoom", mln_ts_number(env, (double)(source->max_zoom))
        ))
      return NULL;
  }
  if ((source->fields & MLN_BOUND_OPTION_MIN_PITCH) != 0) {
    if (!mln_ts_set(
          env, result, "minPitch",
          mln_ts_number(env, (double)(source->min_pitch))
        ))
      return NULL;
  }
  if ((source->fields & MLN_BOUND_OPTION_MAX_PITCH) != 0) {
    if (!mln_ts_set(
          env, result, "maxPitch",
          mln_ts_number(env, (double)(source->max_pitch))
        ))
      return NULL;
  }
  if (!mln_ts_set(
        env, result, "unbounded",
        mln_ts_bool(env, (source->fields & MLN_BOUND_OPTION_UNBOUNDED) != 0)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_camera_delta(
  napi_env env, const mln_camera_delta* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_CAMERA_DELTA_OFFSET) != 0) {
    if (!mln_ts_set(
          env, result, "offset",
          mln_ts_read_screen_point(
            env, &(source->offset), sizeof(source->offset)
          )
        ))
      return NULL;
  }
  if ((source->fields & MLN_CAMERA_DELTA_SCALE) != 0) {
    if (!mln_ts_set(
          env, result, "scale", mln_ts_number(env, (double)(source->scale))
        ))
      return NULL;
  }
  if ((source->fields & MLN_CAMERA_DELTA_BEARING) != 0) {
    if (!mln_ts_set(
          env, result, "bearing", mln_ts_number(env, (double)(source->bearing))
        ))
      return NULL;
  }
  if ((source->fields & MLN_CAMERA_DELTA_PITCH) != 0) {
    if (!mln_ts_set(
          env, result, "pitch", mln_ts_number(env, (double)(source->pitch))
        ))
      return NULL;
  }
  if ((source->fields & MLN_CAMERA_DELTA_ANCHOR) != 0) {
    if (!mln_ts_set(
          env, result, "anchor",
          mln_ts_read_screen_point(
            env, &(source->anchor), sizeof(source->anchor)
          )
        ))
      return NULL;
  }
  if (!mln_ts_set(
        env, result, "animation",
        mln_ts_read_animation_options(
          env, &(source->animation), sizeof(source->animation)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "gesturePhase",
        mln_ts_number(env, (double)(source->gesture_phase))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_camera_fit_options(
  napi_env env, const mln_camera_fit_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_CAMERA_FIT_OPTION_PADDING) != 0) {
    if (!mln_ts_set(
          env, result, "padding",
          mln_ts_read_edge_insets(
            env, &(source->padding), sizeof(source->padding)
          )
        ))
      return NULL;
  }
  if ((source->fields & MLN_CAMERA_FIT_OPTION_BEARING) != 0) {
    if (!mln_ts_set(
          env, result, "bearing", mln_ts_number(env, (double)(source->bearing))
        ))
      return NULL;
  }
  if ((source->fields & MLN_CAMERA_FIT_OPTION_PITCH) != 0) {
    if (!mln_ts_set(
          env, result, "pitch", mln_ts_number(env, (double)(source->pitch))
        ))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_camera_options(
  napi_env env, const mln_camera_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_CAMERA_OPTION_CENTER) != 0) {
    if (!mln_ts_set(
          env, result, "center",
          mln_ts_read_lat_lng(env, &(source->center), sizeof(source->center))
        ))
      return NULL;
  }
  if ((source->fields & MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0) {
    if (!mln_ts_set(
          env, result, "centerAltitude",
          mln_ts_number(env, (double)(source->center_altitude))
        ))
      return NULL;
  }
  if ((source->fields & MLN_CAMERA_OPTION_PADDING) != 0) {
    if (!mln_ts_set(
          env, result, "padding",
          mln_ts_read_edge_insets(
            env, &(source->padding), sizeof(source->padding)
          )
        ))
      return NULL;
  }
  if ((source->fields & MLN_CAMERA_OPTION_ANCHOR) != 0) {
    if (!mln_ts_set(
          env, result, "anchor",
          mln_ts_read_screen_point(
            env, &(source->anchor), sizeof(source->anchor)
          )
        ))
      return NULL;
  }
  if ((source->fields & MLN_CAMERA_OPTION_ZOOM) != 0) {
    if (!mln_ts_set(
          env, result, "zoom", mln_ts_number(env, (double)(source->zoom))
        ))
      return NULL;
  }
  if ((source->fields & MLN_CAMERA_OPTION_BEARING) != 0) {
    if (!mln_ts_set(
          env, result, "bearing", mln_ts_number(env, (double)(source->bearing))
        ))
      return NULL;
  }
  if ((source->fields & MLN_CAMERA_OPTION_PITCH) != 0) {
    if (!mln_ts_set(
          env, result, "pitch", mln_ts_number(env, (double)(source->pitch))
        ))
      return NULL;
  }
  if ((source->fields & MLN_CAMERA_OPTION_ROLL) != 0) {
    if (!mln_ts_set(
          env, result, "roll", mln_ts_number(env, (double)(source->roll))
        ))
      return NULL;
  }
  if ((source->fields & MLN_CAMERA_OPTION_FOV) != 0) {
    if (!mln_ts_set(
          env, result, "fieldOfView",
          mln_ts_number(env, (double)(source->field_of_view))
        ))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_camera_query_result(
  napi_env env, const mln_camera_query_result* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "generation",
        mln_ts_u64(env, (uint64_t)(source->generation))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "camera",
        mln_ts_read_camera_options(
          env, &(source->camera), sizeof(source->camera)
        )
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_camera_transition_end(
  napi_env env, const mln_camera_transition_end* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "outcome", mln_ts_number(env, (double)(source->outcome))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "generation",
        mln_ts_u64(env, (uint64_t)(source->generation))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_camera_transition_end_delivery(
  napi_env env, const mln_camera_transition_end_delivery* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "end",
        ((source->end) == NULL ? mln_ts_undefined(env)
                               : mln_ts_read_camera_transition_end(
                                   env, source->end, sizeof *(source->end)
                                 ))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_camera_update(
  napi_env env, const mln_camera_update* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "mode", mln_ts_number(env, (double)(source->mode))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "camera",
        mln_ts_read_camera_options(
          env, &(source->camera), sizeof(source->camera)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "animation",
        mln_ts_read_animation_options(
          env, &(source->animation), sizeof(source->animation)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "gesturePhase",
        mln_ts_number(env, (double)(source->gesture_phase))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_canonical_tile_id(
  napi_env env, const mln_canonical_tile_id* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(env, result, "z", mln_ts_number(env, (double)(source->z))))
    return NULL;
  if (!mln_ts_set(env, result, "x", mln_ts_number(env, (double)(source->x))))
    return NULL;
  if (!mln_ts_set(env, result, "y", mln_ts_number(env, (double)(source->y))))
    return NULL;
  return result;
}

napi_value mln_ts_read_custom_geometry_source_options(
  napi_env env, const mln_custom_geometry_source_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM) != 0) {
    if (!mln_ts_set(
          env, result, "minZoom", mln_ts_number(env, (double)(source->min_zoom))
        ))
      return NULL;
  }
  if ((source->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM) != 0) {
    if (!mln_ts_set(
          env, result, "maxZoom", mln_ts_number(env, (double)(source->max_zoom))
        ))
      return NULL;
  }
  if ((source->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE) != 0) {
    if (!mln_ts_set(
          env, result, "tolerance",
          mln_ts_number(env, (double)(source->tolerance))
        ))
      return NULL;
  }
  if ((source->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE) != 0) {
    if (!mln_ts_set(
          env, result, "tileSize",
          mln_ts_number(env, (double)(source->tile_size))
        ))
      return NULL;
  }
  if ((source->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER) != 0) {
    if (!mln_ts_set(
          env, result, "buffer", mln_ts_number(env, (double)(source->buffer))
        ))
      return NULL;
  }
  if ((source->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP) != 0) {
    if (!mln_ts_set(env, result, "clip", mln_ts_bool(env, source->clip)))
      return NULL;
  }
  if ((source->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP) != 0) {
    if (!mln_ts_set(env, result, "wrap", mln_ts_bool(env, source->wrap)))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_custom_mvt_vector_source_options(
  napi_env env, const mln_custom_mvt_vector_source_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM) != 0) {
    if (!mln_ts_set(
          env, result, "minZoom", mln_ts_number(env, (double)(source->min_zoom))
        ))
      return NULL;
  }
  if ((source->fields & MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM) != 0) {
    if (!mln_ts_set(
          env, result, "maxZoom", mln_ts_number(env, (double)(source->max_zoom))
        ))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_custom_source_tile_delivery(
  napi_env env, const mln_custom_source_tile_delivery* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "tileId",
        mln_ts_read_canonical_tile_id(
          env, &(source->tile_id), sizeof(source->tile_id)
        )
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_edge_insets(
  napi_env env, const mln_edge_insets* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "top", mln_ts_number(env, (double)(source->top))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "left", mln_ts_number(env, (double)(source->left))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "bottom", mln_ts_number(env, (double)(source->bottom))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "right", mln_ts_number(env, (double)(source->right))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_egl_context_descriptor(
  napi_env env, const mln_egl_context_descriptor* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "display",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->display))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "config",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->config))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "shareContext",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->share_context))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "clientApi",
        mln_ts_number(env, (double)(source->client_api))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "getProcAddress",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->get_proc_address))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_event_batch_view(
  napi_env env, const mln_event_batch_view* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "events", mln_ts_read_event_batch_view_events(env, source)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_event_batch_view_events(
  napi_env env, const mln_event_batch_view* source
) {
  size_t count = (size_t)source->event_count;
  size_t stride = (size_t)source->event_size;
  if (count > 0 && stride < sizeof(mln_runtime_event)) {
    napi_throw_range_error(env, NULL, "native record stride is too small");
    return NULL;
  }
  napi_value result = mln_ts_array(env, count);
  if (result == NULL) return NULL;
  for (size_t index = 0; index < count; index++) {
    const mln_runtime_event* item =
      (const mln_runtime_event*)((const char*)source->events + index * stride);
    napi_value element = mln_ts_read_runtime_event(env, item, stride);
    if (element == NULL) return NULL;
    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;
  }
  return result;
}

napi_value mln_ts_read_frame_demand(
  napi_env env, const mln_frame_demand* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "flags", mln_ts_number(env, (double)(source->flags))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "token", mln_ts_u64(env, (uint64_t)(source->token))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_free_camera_options(
  napi_env env, const mln_free_camera_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_FREE_CAMERA_OPTION_POSITION) != 0) {
    if (!mln_ts_set(
          env, result, "position",
          mln_ts_read_vec3(env, &(source->position), sizeof(source->position))
        ))
      return NULL;
  }
  if ((source->fields & MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0) {
    if (!mln_ts_set(
          env, result, "orientation",
          mln_ts_read_quaternion(
            env, &(source->orientation), sizeof(source->orientation)
          )
        ))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_geojson_source_options(
  napi_env env, const mln_geojson_source_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM) != 0) {
    if (!mln_ts_set(
          env, result, "minZoom", mln_ts_number(env, (double)(source->min_zoom))
        ))
      return NULL;
  }
  if ((source->fields & MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM) != 0) {
    if (!mln_ts_set(
          env, result, "maxZoom", mln_ts_number(env, (double)(source->max_zoom))
        ))
      return NULL;
  }
  if ((source->fields & MLN_GEOJSON_SOURCE_OPTION_TOLERANCE) != 0) {
    if (!mln_ts_set(
          env, result, "tolerance",
          mln_ts_number(env, (double)(source->tolerance))
        ))
      return NULL;
  }
  if ((source->fields & MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM) != 0) {
    if (!mln_ts_set(
          env, result, "clusterMaxZoom",
          mln_ts_number(env, (double)(source->cluster_max_zoom))
        ))
      return NULL;
  }
  if ((source->fields & MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES) != 0) {
    if (!mln_ts_set(
          env, result, "clusterProperties",
          mln_ts_text(
            env, (source->cluster_properties).data,
            (source->cluster_properties).size
          )
        ))
      return NULL;
  }
  if ((source->fields & MLN_GEOJSON_SOURCE_OPTION_BUFFER) != 0) {
    if (!mln_ts_set(
          env, result, "buffer", mln_ts_number(env, (double)(source->buffer))
        ))
      return NULL;
  }
  if ((source->fields & MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS) != 0) {
    if (!mln_ts_set(
          env, result, "clusterRadius",
          mln_ts_number(env, (double)(source->cluster_radius))
        ))
      return NULL;
  }
  if ((source->fields & MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS) != 0) {
    if (!mln_ts_set(
          env, result, "clusterMinPoints",
          mln_ts_number(env, (double)(source->cluster_min_points))
        ))
      return NULL;
  }
  if ((source->fields & MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS) != 0) {
    if (!mln_ts_set(
          env, result, "lineMetrics", mln_ts_bool(env, source->line_metrics)
        ))
      return NULL;
  }
  if ((source->fields & MLN_GEOJSON_SOURCE_OPTION_CLUSTER) != 0) {
    if (!mln_ts_set(env, result, "cluster", mln_ts_bool(env, source->cluster)))
      return NULL;
  }
  if ((source->fields & MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING) != 0) {
    if (!mln_ts_set(
          env, result, "synchronousTiling",
          mln_ts_bool(env, source->synchronous_tiling)
        ))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_gpu_sync(
  napi_env env, const mln_gpu_sync* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "kind", mln_ts_number(env, (double)(source->kind))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "object", mln_ts_u64(env, (uint64_t)(source->object))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_http_header(
  napi_env env, const mln_http_header* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "name",
        mln_ts_text(env, (source->name).data, (source->name).size)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "value",
        mln_ts_text(env, (source->value).data, (source->value).size)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_http_header_rule(
  napi_env env, const mln_http_header_rule* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "kinds", mln_ts_number(env, (double)(source->kinds))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "flags", mln_ts_number(env, (double)(source->flags))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "url",
        mln_ts_text(env, (source->url).data, (source->url).size)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "headers",
        mln_ts_read_http_header_rule_headers(env, source)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_http_header_rule_headers(
  napi_env env, const mln_http_header_rule* source
) {
  size_t count = (size_t)source->header_count;
  size_t stride = sizeof(mln_http_header);
  if (count > 0 && stride < sizeof(mln_http_header)) {
    napi_throw_range_error(env, NULL, "native record stride is too small");
    return NULL;
  }
  napi_value result = mln_ts_array(env, count);
  if (result == NULL) return NULL;
  for (size_t index = 0; index < count; index++) {
    const mln_http_header* item =
      (const mln_http_header*)((const char*)source->headers + index * stride);
    napi_value element = mln_ts_read_http_header(env, item, stride);
    if (element == NULL) return NULL;
    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;
  }
  return result;
}

napi_value mln_ts_read_image_content(
  napi_env env, const mln_image_content* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "left", mln_ts_number(env, (double)(source->left))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "top", mln_ts_number(env, (double)(source->top))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "right", mln_ts_number(env, (double)(source->right))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "bottom", mln_ts_number(env, (double)(source->bottom))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_image_stretch(
  napi_env env, const mln_image_stretch* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "from", mln_ts_number(env, (double)(source->from))
      ))
    return NULL;
  if (!mln_ts_set(env, result, "to", mln_ts_number(env, (double)(source->to))))
    return NULL;
  return result;
}

napi_value mln_ts_read_items_lat_lng(
  napi_env env, const mln_lat_lng* items, size_t count
) {
  napi_value result = mln_ts_array(env, count);
  if (result == NULL) return NULL;
  for (size_t index = 0; index < count; index++) {
    napi_value element =
      mln_ts_read_lat_lng(env, &(items[index]), sizeof(items[index]));
    if (element == NULL) return NULL;
    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;
  }
  return result;
}

napi_value mln_ts_read_items_screen_point(
  napi_env env, const mln_screen_point* items, size_t count
) {
  napi_value result = mln_ts_array(env, count);
  if (result == NULL) return NULL;
  for (size_t index = 0; index < count; index++) {
    napi_value element =
      mln_ts_read_screen_point(env, &(items[index]), sizeof(items[index]));
    if (element == NULL) return NULL;
    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;
  }
  return result;
}

napi_value mln_ts_read_lat_lng(
  napi_env env, const mln_lat_lng* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "latitude", mln_ts_number(env, (double)(source->latitude))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "longitude",
        mln_ts_number(env, (double)(source->longitude))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_lat_lng_bounds(
  napi_env env, const mln_lat_lng_bounds* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "southwest",
        mln_ts_read_lat_lng(
          env, &(source->southwest), sizeof(source->southwest)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "northeast",
        mln_ts_read_lat_lng(
          env, &(source->northeast), sizeof(source->northeast)
        )
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_log_delivery(
  napi_env env, const mln_log_delivery* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "severity", mln_ts_number(env, (double)(source->severity))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "event", mln_ts_number(env, (double)(source->event))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "code", mln_ts_i64(env, (int64_t)(source->code))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "message",
        ((source->message) == NULL
           ? mln_ts_undefined(env)
           : mln_ts_text(env, source->message, strlen(source->message)))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_logical_extent(
  napi_env env, const mln_logical_extent* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "width", mln_ts_number(env, (double)(source->width))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "height", mln_ts_number(env, (double)(source->height))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "scaleFactor",
        mln_ts_number(env, (double)(source->scale_factor))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_map_options(
  napi_env env, const mln_map_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "initialExtent",
        mln_ts_read_logical_extent(
          env, &(source->initial_extent), sizeof(source->initial_extent)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "mapMode", mln_ts_number(env, (double)(source->map_mode))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "fastPforEnabled",
        mln_ts_bool(env, source->fast_pfor_enabled)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "eventMask",
        mln_ts_u64(env, (uint64_t)(source->event_mask))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_map_snapshot(
  napi_env env, const mln_map_snapshot* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "debugOptions",
        mln_ts_number(env, (double)(source->debug_options))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "generation",
        mln_ts_u64(env, (uint64_t)(source->generation))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "fullyLoaded", mln_ts_bool(env, source->fully_loaded)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "renderingStatsViewEnabled",
        mln_ts_bool(env, source->rendering_stats_view_enabled)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "gestureInProgress",
        mln_ts_bool(env, source->gesture_in_progress)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "transitionInProgress",
        mln_ts_bool(env, source->transition_in_progress)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "eventMask",
        mln_ts_u64(env, (uint64_t)(source->event_mask))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "latestRenderUpdateGeneration",
        mln_ts_u64(env, (uint64_t)(source->latest_render_update_generation))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "tile",
        mln_ts_read_map_tile_options(env, &(source->tile), sizeof(source->tile))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "bounds",
        mln_ts_read_bound_options(
          env, &(source->bounds), sizeof(source->bounds)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "freeCamera",
        mln_ts_read_free_camera_options(
          env, &(source->free_camera), sizeof(source->free_camera)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "view",
        mln_ts_read_map_view(env, &(source->view), sizeof(source->view))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_map_tile_options(
  napi_env env, const mln_map_tile_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0) {
    if (!mln_ts_set(
          env, result, "prefetchZoomDelta",
          mln_ts_number(env, (double)(source->prefetch_zoom_delta))
        ))
      return NULL;
  }
  if ((source->fields & MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0) {
    if (!mln_ts_set(
          env, result, "lodMinRadius",
          mln_ts_number(env, (double)(source->lod_min_radius))
        ))
      return NULL;
  }
  if ((source->fields & MLN_MAP_TILE_OPTION_LOD_SCALE) != 0) {
    if (!mln_ts_set(
          env, result, "lodScale",
          mln_ts_number(env, (double)(source->lod_scale))
        ))
      return NULL;
  }
  if ((source->fields & MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0) {
    if (!mln_ts_set(
          env, result, "lodPitchThreshold",
          mln_ts_number(env, (double)(source->lod_pitch_threshold))
        ))
      return NULL;
  }
  if ((source->fields & MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0) {
    if (!mln_ts_set(
          env, result, "lodZoomShift",
          mln_ts_number(env, (double)(source->lod_zoom_shift))
        ))
      return NULL;
  }
  if ((source->fields & MLN_MAP_TILE_OPTION_LOD_MODE) != 0) {
    if (!mln_ts_set(
          env, result, "lodMode", mln_ts_number(env, (double)(source->lod_mode))
        ))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_map_view(
  napi_env env, const mln_map_view* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "camera",
        mln_ts_read_camera_options(
          env, &(source->camera), sizeof(source->camera)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "extent",
        mln_ts_read_logical_extent(
          env, &(source->extent), sizeof(source->extent)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "projectionMode",
        mln_ts_read_projection_mode(
          env, &(source->projection_mode), sizeof(source->projection_mode)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "viewport",
        mln_ts_read_map_viewport_options(
          env, &(source->viewport), sizeof(source->viewport)
        )
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_map_viewport_options(
  napi_env env, const mln_map_viewport_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0) {
    if (!mln_ts_set(
          env, result, "northOrientation",
          mln_ts_number(env, (double)(source->north_orientation))
        ))
      return NULL;
  }
  if ((source->fields & MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0) {
    if (!mln_ts_set(
          env, result, "constrainMode",
          mln_ts_number(env, (double)(source->constrain_mode))
        ))
      return NULL;
  }
  if ((source->fields & MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0) {
    if (!mln_ts_set(
          env, result, "viewportMode",
          mln_ts_number(env, (double)(source->viewport_mode))
        ))
      return NULL;
  }
  if ((source->fields & MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0) {
    if (!mln_ts_set(
          env, result, "frustumOffset",
          mln_ts_read_edge_insets(
            env, &(source->frustum_offset), sizeof(source->frustum_offset)
          )
        ))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_metal_borrowed_texture(
  napi_env env, const mln_metal_borrowed_texture* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "texture",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->texture))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_metal_borrowed_texture_descriptor(
  napi_env env, const mln_metal_borrowed_texture_descriptor* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "extent",
        mln_ts_read_logical_extent(
          env, &(source->extent), sizeof(source->extent)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "physicalWidth",
        mln_ts_number(env, (double)(source->physical_width))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "physicalHeight",
        mln_ts_number(env, (double)(source->physical_height))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "textures",
        mln_ts_read_metal_borrowed_texture_descriptor_textures(env, source)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_metal_borrowed_texture_descriptor_textures(
  napi_env env, const mln_metal_borrowed_texture_descriptor* source
) {
  size_t count = (size_t)source->texture_count;
  size_t stride = sizeof(mln_metal_borrowed_texture);
  if (count > 0 && stride < sizeof(mln_metal_borrowed_texture)) {
    napi_throw_range_error(env, NULL, "native record stride is too small");
    return NULL;
  }
  napi_value result = mln_ts_array(env, count);
  if (result == NULL) return NULL;
  for (size_t index = 0; index < count; index++) {
    const mln_metal_borrowed_texture* item =
      (const mln_metal_borrowed_texture*)((const char*)source->textures +
                                          index * stride);
    napi_value element = mln_ts_read_metal_borrowed_texture(env, item, stride);
    if (element == NULL) return NULL;
    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;
  }
  return result;
}

napi_value mln_ts_read_metal_context_descriptor(
  napi_env env, const mln_metal_context_descriptor* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "device",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->device))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_metal_owned_texture_descriptor(
  napi_env env, const mln_metal_owned_texture_descriptor* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "extent",
        mln_ts_read_logical_extent(
          env, &(source->extent), sizeof(source->extent)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "context",
        mln_ts_read_metal_context_descriptor(
          env, &(source->context), sizeof(source->context)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "pixelFormat",
        mln_ts_u64(env, (uint64_t)(source->pixel_format))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_metal_surface_descriptor(
  napi_env env, const mln_metal_surface_descriptor* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "extent",
        mln_ts_read_logical_extent(
          env, &(source->extent), sizeof(source->extent)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "context",
        mln_ts_read_metal_context_descriptor(
          env, &(source->context), sizeof(source->context)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "layer",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->layer))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_metal_texture_frame(
  napi_env env, const mln_metal_texture_frame* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "generation",
        mln_ts_u64(env, (uint64_t)(source->generation))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "width", mln_ts_number(env, (double)(source->width))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "height", mln_ts_number(env, (double)(source->height))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "scaleFactor",
        mln_ts_number(env, (double)(source->scale_factor))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "frameId", mln_ts_u64(env, (uint64_t)(source->frame_id))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "slot", mln_ts_number(env, (double)(source->slot))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "texture",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->texture))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "device",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->device))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "pixelFormat",
        mln_ts_u64(env, (uint64_t)(source->pixel_format))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_offline_geometry_region_definition(
  napi_env env, const mln_offline_geometry_region_definition* source,
  size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "styleUrl",
        mln_ts_text(env, (source->style_url).data, (source->style_url).size)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "geometry",
        mln_ts_text(env, (source->geometry).data, (source->geometry).size)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "minZoom", mln_ts_number(env, (double)(source->min_zoom))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "maxZoom", mln_ts_number(env, (double)(source->max_zoom))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "pixelRatio",
        mln_ts_number(env, (double)(source->pixel_ratio))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "includeIdeographs",
        mln_ts_bool(env, source->include_ideographs)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_offline_region_definition(
  napi_env env, const mln_offline_region_definition* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "data",
        mln_ts_read_offline_region_definition_data(env, source, size)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_offline_region_definition_data(
  napi_env env, const mln_offline_region_definition* source, size_t size
) {
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  switch (source->type) {
    case MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID:
      if (!mln_ts_set(env, result, "kind", mln_ts_text(env, "tilePyramid", 11)))
        return NULL;
      if (!mln_ts_set(
            env, result, "value",
            mln_ts_read_offline_tile_pyramid_region_definition(
              env, &(source->data.tile_pyramid),
              sizeof(source->data.tile_pyramid)
            )
          ))
        return NULL;
      return result;
    case MLN_OFFLINE_REGION_DEFINITION_GEOMETRY:
      if (!mln_ts_set(env, result, "kind", mln_ts_text(env, "geometry", 8)))
        return NULL;
      if (!mln_ts_set(
            env, result, "value",
            mln_ts_read_offline_geometry_region_definition(
              env, &(source->data.geometry), sizeof(source->data.geometry)
            )
          ))
        return NULL;
      return result;
    default:
      if (!mln_ts_set(env, result, "kind", mln_ts_text(env, "unknown", 7)))
        return NULL;
      if (!mln_ts_set(
            env, result, "tag", mln_ts_number(env, (double)source->type)
          ))
        return NULL;
      // The bytes of the whole record, as far as its stride reaches.
      if (!mln_ts_set(env, result, "bytes", mln_ts_bytes(env, source, size)))
        return NULL;
      return result;
  }
}

napi_value mln_ts_read_offline_region_info(
  napi_env env, const mln_offline_region_info* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(env, result, "id", mln_ts_i64(env, (int64_t)(source->id))))
    return NULL;
  if (!mln_ts_set(
        env, result, "definition",
        mln_ts_read_offline_region_definition(
          env, &(source->definition), sizeof(source->definition)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "metadata",
        mln_ts_bytes(env, source->metadata, (size_t)source->metadata_size)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_offline_region_status(
  napi_env env, const mln_offline_region_status* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "downloadState",
        mln_ts_number(env, (double)(source->download_state))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "completedResourceCount",
        mln_ts_u64(env, (uint64_t)(source->completed_resource_count))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "completedResourceSize",
        mln_ts_u64(env, (uint64_t)(source->completed_resource_size))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "completedTileCount",
        mln_ts_u64(env, (uint64_t)(source->completed_tile_count))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "requiredTileCount",
        mln_ts_u64(env, (uint64_t)(source->required_tile_count))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "completedTileSize",
        mln_ts_u64(env, (uint64_t)(source->completed_tile_size))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "requiredResourceCount",
        mln_ts_u64(env, (uint64_t)(source->required_resource_count))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "requiredResourceCountIsPrecise",
        mln_ts_bool(env, source->required_resource_count_is_precise)
      ))
    return NULL;
  if (!mln_ts_set(env, result, "complete", mln_ts_bool(env, source->complete)))
    return NULL;
  return result;
}

napi_value mln_ts_read_offline_tile_pyramid_region_definition(
  napi_env env, const mln_offline_tile_pyramid_region_definition* source,
  size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "styleUrl",
        mln_ts_text(env, (source->style_url).data, (source->style_url).size)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "bounds",
        mln_ts_read_lat_lng_bounds(
          env, &(source->bounds), sizeof(source->bounds)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "minZoom", mln_ts_number(env, (double)(source->min_zoom))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "maxZoom", mln_ts_number(env, (double)(source->max_zoom))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "pixelRatio",
        mln_ts_number(env, (double)(source->pixel_ratio))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "includeIdeographs",
        mln_ts_bool(env, source->include_ideographs)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_opengl_borrowed_texture(
  napi_env env, const mln_opengl_borrowed_texture* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "texture", mln_ts_number(env, (double)(source->texture))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_opengl_borrowed_texture_descriptor(
  napi_env env, const mln_opengl_borrowed_texture_descriptor* source,
  size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "extent",
        mln_ts_read_logical_extent(
          env, &(source->extent), sizeof(source->extent)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "physicalWidth",
        mln_ts_number(env, (double)(source->physical_width))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "physicalHeight",
        mln_ts_number(env, (double)(source->physical_height))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "context",
        mln_ts_read_opengl_context_descriptor(
          env, &(source->context), sizeof(source->context)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "textures",
        mln_ts_read_opengl_borrowed_texture_descriptor_textures(env, source)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "target", mln_ts_number(env, (double)(source->target))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_opengl_borrowed_texture_descriptor_textures(
  napi_env env, const mln_opengl_borrowed_texture_descriptor* source
) {
  size_t count = (size_t)source->texture_count;
  size_t stride = sizeof(mln_opengl_borrowed_texture);
  if (count > 0 && stride < sizeof(mln_opengl_borrowed_texture)) {
    napi_throw_range_error(env, NULL, "native record stride is too small");
    return NULL;
  }
  napi_value result = mln_ts_array(env, count);
  if (result == NULL) return NULL;
  for (size_t index = 0; index < count; index++) {
    const mln_opengl_borrowed_texture* item =
      (const mln_opengl_borrowed_texture*)((const char*)source->textures +
                                           index * stride);
    napi_value element = mln_ts_read_opengl_borrowed_texture(env, item, stride);
    if (element == NULL) return NULL;
    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;
  }
  return result;
}

napi_value mln_ts_read_opengl_context_descriptor(
  napi_env env, const mln_opengl_context_descriptor* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "ownership",
        mln_ts_number(env, (double)(source->ownership))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "data",
        mln_ts_read_opengl_context_descriptor_data(env, source, size)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_opengl_context_descriptor_data(
  napi_env env, const mln_opengl_context_descriptor* source, size_t size
) {
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  switch (source->platform) {
    case MLN_OPENGL_CONTEXT_PLATFORM_WGL:
      if (!mln_ts_set(env, result, "kind", mln_ts_text(env, "wgl", 3)))
        return NULL;
      if (!mln_ts_set(
            env, result, "value",
            mln_ts_read_wgl_context_descriptor(
              env, &(source->data.wgl), sizeof(source->data.wgl)
            )
          ))
        return NULL;
      return result;
    case MLN_OPENGL_CONTEXT_PLATFORM_EGL:
      if (!mln_ts_set(env, result, "kind", mln_ts_text(env, "egl", 3)))
        return NULL;
      if (!mln_ts_set(
            env, result, "value",
            mln_ts_read_egl_context_descriptor(
              env, &(source->data.egl), sizeof(source->data.egl)
            )
          ))
        return NULL;
      return result;
    case MLN_OPENGL_CONTEXT_PLATFORM_WEBGL:
      if (!mln_ts_set(env, result, "kind", mln_ts_text(env, "webgl", 5)))
        return NULL;
      if (!mln_ts_set(
            env, result, "value",
            mln_ts_read_webgl_context_descriptor(
              env, &(source->data.webgl), sizeof(source->data.webgl)
            )
          ))
        return NULL;
      return result;
    default:
      if (!mln_ts_set(env, result, "kind", mln_ts_text(env, "unknown", 7)))
        return NULL;
      if (!mln_ts_set(
            env, result, "tag", mln_ts_number(env, (double)source->platform)
          ))
        return NULL;
      // The bytes of the whole record, as far as its stride reaches.
      if (!mln_ts_set(env, result, "bytes", mln_ts_bytes(env, source, size)))
        return NULL;
      return result;
  }
}

napi_value mln_ts_read_opengl_owned_texture_descriptor(
  napi_env env, const mln_opengl_owned_texture_descriptor* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "extent",
        mln_ts_read_logical_extent(
          env, &(source->extent), sizeof(source->extent)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "context",
        mln_ts_read_opengl_context_descriptor(
          env, &(source->context), sizeof(source->context)
        )
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_opengl_surface_descriptor(
  napi_env env, const mln_opengl_surface_descriptor* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "extent",
        mln_ts_read_logical_extent(
          env, &(source->extent), sizeof(source->extent)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "context",
        mln_ts_read_opengl_context_descriptor(
          env, &(source->context), sizeof(source->context)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "surface",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->surface))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_opengl_texture_frame(
  napi_env env, const mln_opengl_texture_frame* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "generation",
        mln_ts_u64(env, (uint64_t)(source->generation))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "width", mln_ts_number(env, (double)(source->width))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "height", mln_ts_number(env, (double)(source->height))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "scaleFactor",
        mln_ts_number(env, (double)(source->scale_factor))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "frameId", mln_ts_u64(env, (uint64_t)(source->frame_id))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "slot", mln_ts_number(env, (double)(source->slot))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "texture", mln_ts_number(env, (double)(source->texture))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "target", mln_ts_number(env, (double)(source->target))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "internalFormat",
        mln_ts_number(env, (double)(source->internal_format))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "format", mln_ts_number(env, (double)(source->format))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "type", mln_ts_number(env, (double)(source->type))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_premultiplied_rgba8_image(
  napi_env env, const mln_premultiplied_rgba8_image* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "width", mln_ts_number(env, (double)(source->width))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "height", mln_ts_number(env, (double)(source->height))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "stride", mln_ts_number(env, (double)(source->stride))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "pixels",
        mln_ts_bytes(env, source->pixels, (size_t)source->byte_length)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_projected_meters(
  napi_env env, const mln_projected_meters* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "northing", mln_ts_number(env, (double)(source->northing))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "easting", mln_ts_number(env, (double)(source->easting))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_projection_mode(
  napi_env env, const mln_projection_mode* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_PROJECTION_MODE_AXONOMETRIC) != 0) {
    if (!mln_ts_set(
          env, result, "axonometric", mln_ts_bool(env, source->axonometric)
        ))
      return NULL;
  }
  if ((source->fields & MLN_PROJECTION_MODE_X_SKEW) != 0) {
    if (!mln_ts_set(
          env, result, "xSkew", mln_ts_number(env, (double)(source->x_skew))
        ))
      return NULL;
  }
  if ((source->fields & MLN_PROJECTION_MODE_Y_SKEW) != 0) {
    if (!mln_ts_set(
          env, result, "ySkew", mln_ts_number(env, (double)(source->y_skew))
        ))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_quaternion(
  napi_env env, const mln_quaternion* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(env, result, "x", mln_ts_number(env, (double)(source->x))))
    return NULL;
  if (!mln_ts_set(env, result, "y", mln_ts_number(env, (double)(source->y))))
    return NULL;
  if (!mln_ts_set(env, result, "z", mln_ts_number(env, (double)(source->z))))
    return NULL;
  if (!mln_ts_set(env, result, "w", mln_ts_number(env, (double)(source->w))))
    return NULL;
  return result;
}

napi_value mln_ts_read_render_abandon_result(
  napi_env env, const mln_render_abandon_result* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "disposition",
        mln_ts_number(env, (double)(source->disposition))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "quarantinedResourceCount",
        mln_ts_number(env, (double)(source->quarantined_resource_count))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_render_frame_batch_view(
  napi_env env, const mln_render_frame_batch_view* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "results",
        mln_ts_read_render_frame_batch_view_results(env, source)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_render_frame_batch_view_results(
  napi_env env, const mln_render_frame_batch_view* source
) {
  size_t count = (size_t)source->result_count;
  size_t stride = (size_t)source->result_size;
  if (count > 0 && stride < sizeof(mln_render_frame_result)) {
    napi_throw_range_error(env, NULL, "native record stride is too small");
    return NULL;
  }
  napi_value result = mln_ts_array(env, count);
  if (result == NULL) return NULL;
  for (size_t index = 0; index < count; index++) {
    const mln_render_frame_result* item =
      (const mln_render_frame_result*)((const char*)source->results +
                                       index * stride);
    napi_value element = mln_ts_read_render_frame_result(env, item, stride);
    if (element == NULL) return NULL;
    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;
  }
  return result;
}

napi_value mln_ts_read_render_frame_result(
  napi_env env, const mln_render_frame_result* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "disposition",
        mln_ts_number(env, (double)(source->disposition))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "token", mln_ts_u64(env, (uint64_t)(source->token))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "mapUpdateGeneration",
        mln_ts_u64(env, (uint64_t)(source->map_update_generation))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "generation",
        mln_ts_u64(env, (uint64_t)(source->generation))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "frameGeneration",
        mln_ts_u64(env, (uint64_t)(source->frame_generation))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "needsRepaint", mln_ts_bool(env, source->needs_repaint)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "view",
        mln_ts_read_map_view(env, &(source->view), sizeof(source->view))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_render_session_attach_options(
  napi_env env, const mln_render_session_attach_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "driver", mln_ts_number(env, (double)(source->driver))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "requestedTextureRingDepth",
        mln_ts_number(env, (double)(source->requested_texture_ring_depth))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "producerSyncKind",
        mln_ts_number(env, (double)(source->producer_sync_kind))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_render_session_capabilities(
  napi_env env, const mln_render_session_capabilities* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "driver", mln_ts_number(env, (double)(source->driver))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "textureRingDepth",
        mln_ts_number(env, (double)(source->texture_ring_depth))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "flags", mln_ts_number(env, (double)(source->flags))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_render_session_snapshot(
  napi_env env, const mln_render_session_snapshot* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "state", mln_ts_number(env, (double)(source->state))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "extent",
        mln_ts_read_logical_extent(
          env, &(source->extent), sizeof(source->extent)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "generation",
        mln_ts_u64(env, (uint64_t)(source->generation))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "mapUpdateGeneration",
        mln_ts_u64(env, (uint64_t)(source->map_update_generation))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "renderedUpdateGeneration",
        mln_ts_u64(env, (uint64_t)(source->rendered_update_generation))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "frameGeneration",
        mln_ts_u64(env, (uint64_t)(source->frame_generation))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "acquiredFrameCount",
        mln_ts_number(env, (double)(source->acquired_frame_count))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "pendingChanges", mln_ts_bool(env, source->pending_changes)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_rendered_feature(
  napi_env env, const mln_rendered_feature* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "feature",
        mln_ts_text(env, (source->feature).data, (source->feature).size)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "layerId",
        mln_ts_text(env, (source->layer_id).data, (source->layer_id).size)
      ))
    return NULL;
  if ((source->fields & MLN_RENDERED_FEATURE_SOURCE_ID) != 0) {
    if (!mln_ts_set(
          env, result, "sourceId",
          mln_ts_text(env, (source->source_id).data, (source->source_id).size)
        ))
      return NULL;
  }
  if ((source->fields & MLN_RENDERED_FEATURE_SOURCE_LAYER_ID) != 0) {
    if (!mln_ts_set(
          env, result, "sourceLayerId",
          mln_ts_text(
            env, (source->source_layer_id).data, (source->source_layer_id).size
          )
        ))
      return NULL;
  }
  if ((source->fields & MLN_RENDERED_FEATURE_STATE) != 0) {
    if (!mln_ts_set(
          env, result, "state",
          mln_ts_text(env, (source->state).data, (source->state).size)
        ))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_rendered_feature_query_options(
  napi_env env, const mln_rendered_feature_query_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS) != 0) {
    if (!mln_ts_set(
          env, result, "layerIds",
          mln_ts_read_rendered_feature_query_options_layer_ids(env, source)
        ))
      return NULL;
  }
  if ((source->fields & MLN_RENDERED_FEATURE_QUERY_OPTION_FILTER) != 0) {
    if (!mln_ts_set(
          env, result, "filter",
          mln_ts_text(env, (source->filter).data, (source->filter).size)
        ))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_rendered_feature_query_options_layer_ids(
  napi_env env, const mln_rendered_feature_query_options* source
) {
  size_t count = (size_t)source->layer_id_count;
  size_t stride = sizeof(mln_buffer_view);
  if (count > 0 && stride < sizeof(mln_buffer_view)) {
    napi_throw_range_error(env, NULL, "native record stride is too small");
    return NULL;
  }
  napi_value result = mln_ts_array(env, count);
  if (result == NULL) return NULL;
  for (size_t index = 0; index < count; index++) {
    const mln_buffer_view* item =
      (const mln_buffer_view*)((const char*)source->layer_ids + index * stride);
    napi_value element = mln_ts_text(env, ((*item)).data, ((*item)).size);
    if (element == NULL) return NULL;
    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;
  }
  return result;
}

napi_value mln_ts_read_rendering_stats(
  napi_env env, const mln_rendering_stats* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "encodingTime",
        mln_ts_number(env, (double)(source->encoding_time))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "renderingTime",
        mln_ts_number(env, (double)(source->rendering_time))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "frameCount",
        mln_ts_i64(env, (int64_t)(source->frame_count))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "drawCallCount",
        mln_ts_i64(env, (int64_t)(source->draw_call_count))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "totalDrawCallCount",
        mln_ts_i64(env, (int64_t)(source->total_draw_call_count))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_resource_range(
  napi_env env, const mln_resource_range* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "start", mln_ts_u64(env, (uint64_t)(source->start))
      ))
    return NULL;
  if (!mln_ts_set(env, result, "end", mln_ts_u64(env, (uint64_t)(source->end))))
    return NULL;
  return result;
}

napi_value mln_ts_read_resource_request_cancel_delivery(
  napi_env env, const mln_resource_request_cancel_delivery* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "handle", mln_ts_u64(env, (uint64_t)(source->handle))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_resource_request_delivery(
  napi_env env, const mln_resource_request_delivery* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "request",
        ((source->request) == NULL
           ? mln_ts_undefined(env)
           : mln_ts_read_resource_request_info(
               env, source->request, sizeof *(source->request)
             ))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "handle", mln_ts_u64(env, (uint64_t)(source->handle))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_resource_request_info(
  napi_env env, const mln_resource_request_info* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "requestedUrl",
        ((source->requested_url) == NULL
           ? mln_ts_undefined(env)
           : mln_ts_text(
               env, source->requested_url, strlen(source->requested_url)
             ))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "resolvedUrl",
        ((source->resolved_url) == NULL
           ? mln_ts_undefined(env)
           : mln_ts_text(
               env, source->resolved_url, strlen(source->resolved_url)
             ))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "kind", mln_ts_number(env, (double)(source->kind))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "loadingMethod",
        mln_ts_number(env, (double)(source->loading_method))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "priority", mln_ts_number(env, (double)(source->priority))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "usage", mln_ts_number(env, (double)(source->usage))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "storagePolicy",
        mln_ts_number(env, (double)(source->storage_policy))
      ))
    return NULL;
  if ((source->fields & MLN_RESOURCE_REQUEST_RANGE) != 0) {
    if (!mln_ts_set(
          env, result, "range",
          mln_ts_read_resource_range(
            env, &(source->range), sizeof(source->range)
          )
        ))
      return NULL;
  }
  if ((source->fields & MLN_RESOURCE_REQUEST_PRIOR_MODIFIED) != 0) {
    if (!mln_ts_set(
          env, result, "priorModifiedUnixMs",
          mln_ts_i64(env, (int64_t)(source->prior_modified_unix_ms))
        ))
      return NULL;
  }
  if ((source->fields & MLN_RESOURCE_REQUEST_PRIOR_EXPIRES) != 0) {
    if (!mln_ts_set(
          env, result, "priorExpiresUnixMs",
          mln_ts_i64(env, (int64_t)(source->prior_expires_unix_ms))
        ))
      return NULL;
  }
  if (!mln_ts_set(
        env, result, "priorEtag",
        ((source->prior_etag) == NULL
           ? mln_ts_undefined(env)
           : mln_ts_text(env, source->prior_etag, strlen(source->prior_etag)))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "priorData",
        mln_ts_bytes(env, source->prior_data, (size_t)source->prior_data_size)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_resource_route(
  napi_env env, const mln_resource_route* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "kinds", mln_ts_number(env, (double)(source->kinds))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "flags", mln_ts_number(env, (double)(source->flags))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "url",
        mln_ts_text(env, (source->url).data, (source->url).size)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_resource_transform_delivery(
  napi_env env, const mln_resource_transform_delivery* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "kind", mln_ts_number(env, (double)(source->kind))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "url",
        ((source->url) == NULL
           ? mln_ts_undefined(env)
           : mln_ts_text(env, source->url, strlen(source->url)))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "request", mln_ts_u64(env, (uint64_t)(source->request))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_runtime_event(
  napi_env env, const mln_runtime_event* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "type", mln_ts_number(env, (double)(source->type))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "sourceType",
        mln_ts_number(env, (double)(source->source_type))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "source", mln_ts_u64(env, (uint64_t)(source->source))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "generation",
        mln_ts_u64(env, (uint64_t)(source->generation))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "code", mln_ts_number(env, (double)(source->code))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "message",
        mln_ts_text(env, (source->message).data, (source->message).size)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "payload",
        mln_ts_read_runtime_event_payload(env, source, size)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_runtime_event_delivery(
  napi_env env, const mln_runtime_event_delivery* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "events",
        ((source->events) == NULL
           ? mln_ts_undefined(env)
           : mln_ts_read_event_batch_view(
               env, source->events, sizeof *(source->events)
             ))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_runtime_event_offline_region_response_error(
  napi_env env, const mln_runtime_event_offline_region_response_error* source,
  size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "regionId", mln_ts_i64(env, (int64_t)(source->region_id))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "reason", mln_ts_number(env, (double)(source->reason))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_runtime_event_offline_region_status(
  napi_env env, const mln_runtime_event_offline_region_status* source,
  size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "regionId", mln_ts_i64(env, (int64_t)(source->region_id))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "status",
        mln_ts_read_offline_region_status(
          env, &(source->status), sizeof(source->status)
        )
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_runtime_event_offline_region_tile_count_limit(
  napi_env env, const mln_runtime_event_offline_region_tile_count_limit* source,
  size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "regionId", mln_ts_i64(env, (int64_t)(source->region_id))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "limit", mln_ts_u64(env, (uint64_t)(source->limit))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_runtime_event_payload(
  napi_env env, const mln_runtime_event* source, size_t size
) {
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  switch (source->payload_type) {
    case MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME:
      if (!mln_ts_set(env, result, "kind", mln_ts_text(env, "renderFrame", 11)))
        return NULL;
      if (!mln_ts_set(
            env, result, "value",
            mln_ts_read_runtime_event_render_frame(
              env, &(source->payload.render_frame),
              sizeof(source->payload.render_frame)
            )
          ))
        return NULL;
      return result;
    case MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP:
      if (!mln_ts_set(env, result, "kind", mln_ts_text(env, "renderMap", 9)))
        return NULL;
      if (!mln_ts_set(
            env, result, "value",
            mln_ts_read_runtime_event_render_map(
              env, &(source->payload.render_map),
              sizeof(source->payload.render_map)
            )
          ))
        return NULL;
      return result;
    case MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION:
      if (!mln_ts_set(env, result, "kind", mln_ts_text(env, "tileAction", 10)))
        return NULL;
      if (!mln_ts_set(
            env, result, "value",
            mln_ts_read_runtime_event_tile_action(
              env, &(source->payload.tile_action),
              sizeof(source->payload.tile_action)
            )
          ))
        return NULL;
      return result;
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS:
      if (!mln_ts_set(
            env, result, "kind", mln_ts_text(env, "offlineRegionStatus", 19)
          ))
        return NULL;
      if (!mln_ts_set(
            env, result, "value",
            mln_ts_read_runtime_event_offline_region_status(
              env, &(source->payload.offline_region_status),
              sizeof(source->payload.offline_region_status)
            )
          ))
        return NULL;
      return result;
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR:
      if (!mln_ts_set(
            env, result, "kind",
            mln_ts_text(env, "offlineRegionResponseError", 26)
          ))
        return NULL;
      if (!mln_ts_set(
            env, result, "value",
            mln_ts_read_runtime_event_offline_region_response_error(
              env, &(source->payload.offline_region_response_error),
              sizeof(source->payload.offline_region_response_error)
            )
          ))
        return NULL;
      return result;
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT:
      if (!mln_ts_set(
            env, result, "kind",
            mln_ts_text(env, "offlineRegionTileCountLimit", 27)
          ))
        return NULL;
      if (!mln_ts_set(
            env, result, "value",
            mln_ts_read_runtime_event_offline_region_tile_count_limit(
              env, &(source->payload.offline_region_tile_count_limit),
              sizeof(source->payload.offline_region_tile_count_limit)
            )
          ))
        return NULL;
      return result;
    case MLN_RUNTIME_EVENT_PAYLOAD_NONE:
      if (!mln_ts_set(env, result, "kind", mln_ts_text(env, "none", 4)))
        return NULL;
      return result;
    default:
      if (!mln_ts_set(env, result, "kind", mln_ts_text(env, "unknown", 7)))
        return NULL;
      if (!mln_ts_set(
            env, result, "tag", mln_ts_number(env, (double)source->payload_type)
          ))
        return NULL;
      // The bytes of the whole record, as far as its stride reaches.
      if (!mln_ts_set(env, result, "bytes", mln_ts_bytes(env, source, size)))
        return NULL;
      return result;
  }
}

napi_value mln_ts_read_runtime_event_render_frame(
  napi_env env, const mln_runtime_event_render_frame* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "mode", mln_ts_number(env, (double)(source->mode))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "needsRepaint", mln_ts_bool(env, source->needs_repaint)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "placementChanged",
        mln_ts_bool(env, source->placement_changed)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "stats",
        mln_ts_read_rendering_stats(
          env, &(source->stats), sizeof(source->stats)
        )
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_runtime_event_render_map(
  napi_env env, const mln_runtime_event_render_map* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "mode", mln_ts_number(env, (double)(source->mode))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_runtime_event_tile_action(
  napi_env env, const mln_runtime_event_tile_action* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "operation",
        mln_ts_number(env, (double)(source->operation))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "tileId",
        mln_ts_read_tile_id(env, &(source->tile_id), sizeof(source->tile_id))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_runtime_options(
  napi_env env, const mln_runtime_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "assetPath",
        ((source->asset_path).size == 0
           ? mln_ts_undefined(env)
           : mln_ts_text(
               env, (source->asset_path).data, (source->asset_path).size
             ))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "cachePath",
        ((source->cache_path).size == 0
           ? mln_ts_undefined(env)
           : mln_ts_text(
               env, (source->cache_path).data, (source->cache_path).size
             ))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_screen_point(
  napi_env env, const mln_screen_point* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(env, result, "x", mln_ts_number(env, (double)(source->x))))
    return NULL;
  if (!mln_ts_set(env, result, "y", mln_ts_number(env, (double)(source->y))))
    return NULL;
  return result;
}

napi_value mln_ts_read_source_feature(
  napi_env env, const mln_source_feature* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "feature",
        mln_ts_text(env, (source->feature).data, (source->feature).size)
      ))
    return NULL;
  if ((source->fields & MLN_SOURCE_FEATURE_SOURCE_LAYER_ID) != 0) {
    if (!mln_ts_set(
          env, result, "sourceLayerId",
          mln_ts_text(
            env, (source->source_layer_id).data, (source->source_layer_id).size
          )
        ))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_source_feature_query_options(
  napi_env env, const mln_source_feature_query_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (
    (source->fields & MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS) != 0
  ) {
    if (!mln_ts_set(
          env, result, "sourceLayerIds",
          mln_ts_read_source_feature_query_options_source_layer_ids(env, source)
        ))
      return NULL;
  }
  if ((source->fields & MLN_SOURCE_FEATURE_QUERY_OPTION_FILTER) != 0) {
    if (!mln_ts_set(
          env, result, "filter",
          mln_ts_text(env, (source->filter).data, (source->filter).size)
        ))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_source_feature_query_options_source_layer_ids(
  napi_env env, const mln_source_feature_query_options* source
) {
  size_t count = (size_t)source->source_layer_id_count;
  size_t stride = sizeof(mln_buffer_view);
  if (count > 0 && stride < sizeof(mln_buffer_view)) {
    napi_throw_range_error(env, NULL, "native record stride is too small");
    return NULL;
  }
  napi_value result = mln_ts_array(env, count);
  if (result == NULL) return NULL;
  for (size_t index = 0; index < count; index++) {
    const mln_buffer_view* item =
      (const mln_buffer_view*)((const char*)source->source_layer_ids +
                               index * stride);
    napi_value element = mln_ts_text(env, ((*item)).data, ((*item)).size);
    if (element == NULL) return NULL;
    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;
  }
  return result;
}

napi_value mln_ts_read_style_expression_info(
  napi_env env, const mln_style_expression_info* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "isFeatureConstant",
        mln_ts_bool(env, source->is_feature_constant)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "isZoomConstant",
        mln_ts_bool(env, source->is_zoom_constant)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_style_expression_inputs(
  napi_env env, const mln_style_expression_inputs* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_STYLE_EXPRESSION_INPUT_ZOOM) != 0) {
    if (!mln_ts_set(
          env, result, "zoom", mln_ts_number(env, (double)(source->zoom))
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER) != 0) {
    if (!mln_ts_set(
          env, result, "colorRampParameter",
          mln_ts_number(env, (double)(source->color_ramp_parameter))
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_EXPRESSION_INPUT_ELEVATION) != 0) {
    if (!mln_ts_set(
          env, result, "elevation",
          mln_ts_number(env, (double)(source->elevation))
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_EXPRESSION_INPUT_FEATURE) != 0) {
    if (!mln_ts_set(
          env, result, "feature",
          mln_ts_text(env, (source->feature).data, (source->feature).size)
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_EXPRESSION_INPUT_FEATURE_STATE) != 0) {
    if (!mln_ts_set(
          env, result, "featureState",
          mln_ts_text(
            env, (source->feature_state).data, (source->feature_state).size
          )
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_EXPRESSION_INPUT_GLOBAL_STATE) != 0) {
    if (!mln_ts_set(
          env, result, "globalState",
          mln_ts_text(
            env, (source->global_state).data, (source->global_state).size
          )
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID) != 0) {
    if (!mln_ts_set(
          env, result, "canonicalTileId",
          mln_ts_read_canonical_tile_id(
            env, &(source->canonical_tile_id), sizeof(source->canonical_tile_id)
          )
        ))
      return NULL;
  }
  if (!mln_ts_set(
        env, result, "availableImages",
        mln_ts_read_style_expression_inputs_available_images(env, source)
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_style_expression_inputs_available_images(
  napi_env env, const mln_style_expression_inputs* source
) {
  size_t count = (size_t)source->available_image_count;
  size_t stride = sizeof(mln_buffer_view);
  if (count > 0 && stride < sizeof(mln_buffer_view)) {
    napi_throw_range_error(env, NULL, "native record stride is too small");
    return NULL;
  }
  napi_value result = mln_ts_array(env, count);
  if (result == NULL) return NULL;
  for (size_t index = 0; index < count; index++) {
    const mln_buffer_view* item =
      (const mln_buffer_view*)((const char*)source->available_images +
                               index * stride);
    napi_value element = mln_ts_text(env, ((*item)).data, ((*item)).size);
    if (element == NULL) return NULL;
    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;
  }
  return result;
}

napi_value mln_ts_read_style_expression_options(
  napi_env env, const mln_style_expression_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE) != 0) {
    if (!mln_ts_set(
          env, result, "expectedType",
          mln_ts_read_style_expression_type(
            env, &(source->expected_type), sizeof(source->expected_type)
          )
        ))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_style_expression_type(
  napi_env env, const mln_style_expression_type* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "kind", mln_ts_number(env, (double)(source->kind))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "itemKind", mln_ts_number(env, (double)(source->item_kind))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "itemCount",
        mln_ts_number(env, (double)(source->item_count))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_style_image_info(
  napi_env env, const mln_style_image_info* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "width", mln_ts_number(env, (double)(source->width))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "height", mln_ts_number(env, (double)(source->height))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "pixels",
        mln_ts_bytes(env, (source->pixels).data, (source->pixels).size)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "stretchX",
        mln_ts_read_style_image_info_stretch_x(env, source)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "stretchY",
        mln_ts_read_style_image_info_stretch_y(env, source)
      ))
    return NULL;
  if ((source->fields & MLN_STYLE_IMAGE_INFO_CONTENT) != 0) {
    if (!mln_ts_set(
          env, result, "content",
          mln_ts_read_image_content(
            env, &(source->content), sizeof(source->content)
          )
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH) != 0) {
    if (!mln_ts_set(
          env, result, "textFitWidth",
          mln_ts_number(env, (double)(source->text_fit_width))
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT) != 0) {
    if (!mln_ts_set(
          env, result, "textFitHeight",
          mln_ts_number(env, (double)(source->text_fit_height))
        ))
      return NULL;
  }
  if (!mln_ts_set(
        env, result, "pixelRatio",
        mln_ts_number(env, (double)(source->pixel_ratio))
      ))
    return NULL;
  if (!mln_ts_set(env, result, "sdf", mln_ts_bool(env, source->sdf)))
    return NULL;
  return result;
}

napi_value mln_ts_read_style_image_info_stretch_x(
  napi_env env, const mln_style_image_info* source
) {
  size_t count = (size_t)source->stretch_x_count;
  size_t stride = sizeof(mln_image_stretch);
  if (count > 0 && stride < sizeof(mln_image_stretch)) {
    napi_throw_range_error(env, NULL, "native record stride is too small");
    return NULL;
  }
  napi_value result = mln_ts_array(env, count);
  if (result == NULL) return NULL;
  for (size_t index = 0; index < count; index++) {
    const mln_image_stretch* item =
      (const mln_image_stretch*)((const char*)source->stretch_x +
                                 index * stride);
    napi_value element = mln_ts_read_image_stretch(env, item, stride);
    if (element == NULL) return NULL;
    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;
  }
  return result;
}

napi_value mln_ts_read_style_image_info_stretch_y(
  napi_env env, const mln_style_image_info* source
) {
  size_t count = (size_t)source->stretch_y_count;
  size_t stride = sizeof(mln_image_stretch);
  if (count > 0 && stride < sizeof(mln_image_stretch)) {
    napi_throw_range_error(env, NULL, "native record stride is too small");
    return NULL;
  }
  napi_value result = mln_ts_array(env, count);
  if (result == NULL) return NULL;
  for (size_t index = 0; index < count; index++) {
    const mln_image_stretch* item =
      (const mln_image_stretch*)((const char*)source->stretch_y +
                                 index * stride);
    napi_value element = mln_ts_read_image_stretch(env, item, stride);
    if (element == NULL) return NULL;
    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;
  }
  return result;
}

napi_value mln_ts_read_style_image_options(
  napi_env env, const mln_style_image_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_STYLE_IMAGE_OPTION_STRETCH_X) != 0) {
    if (!mln_ts_set(
          env, result, "stretchX",
          mln_ts_read_style_image_options_stretch_x(env, source)
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_IMAGE_OPTION_STRETCH_Y) != 0) {
    if (!mln_ts_set(
          env, result, "stretchY",
          mln_ts_read_style_image_options_stretch_y(env, source)
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_IMAGE_OPTION_CONTENT) != 0) {
    if (!mln_ts_set(
          env, result, "content",
          mln_ts_read_image_content(
            env, &(source->content), sizeof(source->content)
          )
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH) != 0) {
    if (!mln_ts_set(
          env, result, "textFitWidth",
          mln_ts_number(env, (double)(source->text_fit_width))
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT) != 0) {
    if (!mln_ts_set(
          env, result, "textFitHeight",
          mln_ts_number(env, (double)(source->text_fit_height))
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO) != 0) {
    if (!mln_ts_set(
          env, result, "pixelRatio",
          mln_ts_number(env, (double)(source->pixel_ratio))
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_IMAGE_OPTION_SDF) != 0) {
    if (!mln_ts_set(env, result, "sdf", mln_ts_bool(env, source->sdf)))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_style_image_options_stretch_x(
  napi_env env, const mln_style_image_options* source
) {
  size_t count = (size_t)source->stretch_x_count;
  size_t stride = sizeof(mln_image_stretch);
  if (count > 0 && stride < sizeof(mln_image_stretch)) {
    napi_throw_range_error(env, NULL, "native record stride is too small");
    return NULL;
  }
  napi_value result = mln_ts_array(env, count);
  if (result == NULL) return NULL;
  for (size_t index = 0; index < count; index++) {
    const mln_image_stretch* item =
      (const mln_image_stretch*)((const char*)source->stretch_x +
                                 index * stride);
    napi_value element = mln_ts_read_image_stretch(env, item, stride);
    if (element == NULL) return NULL;
    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;
  }
  return result;
}

napi_value mln_ts_read_style_image_options_stretch_y(
  napi_env env, const mln_style_image_options* source
) {
  size_t count = (size_t)source->stretch_y_count;
  size_t stride = sizeof(mln_image_stretch);
  if (count > 0 && stride < sizeof(mln_image_stretch)) {
    napi_throw_range_error(env, NULL, "native record stride is too small");
    return NULL;
  }
  napi_value result = mln_ts_array(env, count);
  if (result == NULL) return NULL;
  for (size_t index = 0; index < count; index++) {
    const mln_image_stretch* item =
      (const mln_image_stretch*)((const char*)source->stretch_y +
                                 index * stride);
    napi_value element = mln_ts_read_image_stretch(env, item, stride);
    if (element == NULL) return NULL;
    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;
  }
  return result;
}

napi_value mln_ts_read_style_layer_info(
  napi_env env, const mln_style_layer_info* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "id",
        mln_ts_text(env, (source->id).data, (source->id).size)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "type",
        mln_ts_text(env, (source->type).data, (source->type).size)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "sourceId",
        ((source->source_id).size == 0
           ? mln_ts_undefined(env)
           : mln_ts_text(
               env, (source->source_id).data, (source->source_id).size
             ))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "sourceLayer",
        ((source->source_layer).size == 0
           ? mln_ts_undefined(env)
           : mln_ts_text(
               env, (source->source_layer).data, (source->source_layer).size
             ))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "minZoom", mln_ts_number(env, (double)(source->min_zoom))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "maxZoom", mln_ts_number(env, (double)(source->max_zoom))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "visibility",
        mln_ts_number(env, (double)(source->visibility))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_style_source_info(
  napi_env env, const mln_style_source_info* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "id",
        mln_ts_text(env, (source->id).data, (source->id).size)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "type", mln_ts_number(env, (double)(source->type))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "isVolatile", mln_ts_bool(env, source->is_volatile)
      ))
    return NULL;
  if ((source->fields & MLN_STYLE_SOURCE_INFO_ATTRIBUTION) != 0) {
    if (!mln_ts_set(
          env, result, "attribution",
          mln_ts_text(
            env, (source->attribution).data, (source->attribution).size
          )
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_SOURCE_INFO_URL) != 0) {
    if (!mln_ts_set(
          env, result, "url",
          mln_ts_text(env, (source->url).data, (source->url).size)
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_SOURCE_INFO_TILEJSON) != 0) {
    if (!mln_ts_set(
          env, result, "tilejson",
          mln_ts_read_style_source_tile_info(
            env, &(source->tilejson), sizeof(source->tilejson)
          )
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_SOURCE_INFO_BOUNDS) != 0) {
    if (!mln_ts_set(
          env, result, "bounds",
          mln_ts_read_lat_lng_bounds(
            env, &(source->bounds), sizeof(source->bounds)
          )
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_SOURCE_INFO_TILE_SIZE) != 0) {
    if (!mln_ts_set(
          env, result, "tileSize",
          mln_ts_number(env, (double)(source->tile_size))
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING) != 0) {
    if (!mln_ts_set(
          env, result, "vectorEncoding",
          mln_ts_number(env, (double)(source->vector_encoding))
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_SOURCE_INFO_RASTER_ENCODING) != 0) {
    if (!mln_ts_set(
          env, result, "rasterEncoding",
          mln_ts_number(env, (double)(source->raster_encoding))
        ))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_style_source_tile_info(
  napi_env env, const mln_style_source_tile_info* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "tileUrls",
        mln_ts_read_style_source_tile_info_tile_urls(env, source)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "minZoom", mln_ts_number(env, (double)(source->min_zoom))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "maxZoom", mln_ts_number(env, (double)(source->max_zoom))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "scheme", mln_ts_number(env, (double)(source->scheme))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_style_source_tile_info_tile_urls(
  napi_env env, const mln_style_source_tile_info* source
) {
  size_t count = (size_t)source->tile_url_count;
  size_t stride = sizeof(mln_buffer_view);
  if (count > 0 && stride < sizeof(mln_buffer_view)) {
    napi_throw_range_error(env, NULL, "native record stride is too small");
    return NULL;
  }
  napi_value result = mln_ts_array(env, count);
  if (result == NULL) return NULL;
  for (size_t index = 0; index < count; index++) {
    const mln_buffer_view* item =
      (const mln_buffer_view*)((const char*)source->tile_urls + index * stride);
    napi_value element = mln_ts_text(env, ((*item)).data, ((*item)).size);
    if (element == NULL) return NULL;
    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;
  }
  return result;
}

napi_value mln_ts_read_style_transition_options(
  napi_env env, const mln_style_transition_options* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if ((source->fields & MLN_STYLE_TRANSITION_OPTION_DURATION) != 0) {
    if (!mln_ts_set(
          env, result, "durationMs",
          mln_ts_number(env, (double)(source->duration_ms))
        ))
      return NULL;
  }
  if ((source->fields & MLN_STYLE_TRANSITION_OPTION_DELAY) != 0) {
    if (!mln_ts_set(
          env, result, "delayMs", mln_ts_number(env, (double)(source->delay_ms))
        ))
      return NULL;
  }
  if (
    (source->fields &
     MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS) != 0
  ) {
    if (!mln_ts_set(
          env, result, "enablePlacementTransitions",
          mln_ts_bool(env, source->enable_placement_transitions)
        ))
      return NULL;
  }
  return result;
}

napi_value mln_ts_read_texture_image_info(
  napi_env env, const mln_texture_image_info* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "width", mln_ts_number(env, (double)(source->width))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "height", mln_ts_number(env, (double)(source->height))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "stride", mln_ts_number(env, (double)(source->stride))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "byteLength",
        mln_ts_u64(env, (uint64_t)(source->byte_length))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_texture_readback_result(
  napi_env env, const mln_texture_readback_result* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "data",
        mln_ts_bytes(env, (source->data).data, (source->data).size)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "info",
        mln_ts_read_texture_image_info(
          env, &(source->info), sizeof(source->info)
        )
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_tile_id(
  napi_env env, const mln_tile_id* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "overscaledZ",
        mln_ts_number(env, (double)(source->overscaled_z))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "wrap", mln_ts_number(env, (double)(source->wrap))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "canonicalZ",
        mln_ts_number(env, (double)(source->canonical_z))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "canonicalX",
        mln_ts_number(env, (double)(source->canonical_x))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "canonicalY",
        mln_ts_number(env, (double)(source->canonical_y))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_unit_bezier(
  napi_env env, const mln_unit_bezier* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(env, result, "x1", mln_ts_number(env, (double)(source->x1))))
    return NULL;
  if (!mln_ts_set(env, result, "y1", mln_ts_number(env, (double)(source->y1))))
    return NULL;
  if (!mln_ts_set(env, result, "x2", mln_ts_number(env, (double)(source->x2))))
    return NULL;
  if (!mln_ts_set(env, result, "y2", mln_ts_number(env, (double)(source->y2))))
    return NULL;
  return result;
}

napi_value mln_ts_read_vec3(napi_env env, const mln_vec3* source, size_t size) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(env, result, "x", mln_ts_number(env, (double)(source->x))))
    return NULL;
  if (!mln_ts_set(env, result, "y", mln_ts_number(env, (double)(source->y))))
    return NULL;
  if (!mln_ts_set(env, result, "z", mln_ts_number(env, (double)(source->z))))
    return NULL;
  return result;
}

napi_value mln_ts_read_vulkan_borrowed_texture(
  napi_env env, const mln_vulkan_borrowed_texture* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "image", mln_ts_u64(env, (uint64_t)(source->image))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "imageView",
        mln_ts_u64(env, (uint64_t)(source->image_view))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_vulkan_borrowed_texture_descriptor(
  napi_env env, const mln_vulkan_borrowed_texture_descriptor* source,
  size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "extent",
        mln_ts_read_logical_extent(
          env, &(source->extent), sizeof(source->extent)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "physicalWidth",
        mln_ts_number(env, (double)(source->physical_width))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "physicalHeight",
        mln_ts_number(env, (double)(source->physical_height))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "context",
        mln_ts_read_vulkan_context_descriptor(
          env, &(source->context), sizeof(source->context)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "textures",
        mln_ts_read_vulkan_borrowed_texture_descriptor_textures(env, source)
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "format", mln_ts_number(env, (double)(source->format))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "initialLayout",
        mln_ts_number(env, (double)(source->initial_layout))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "finalLayout",
        mln_ts_number(env, (double)(source->final_layout))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_vulkan_borrowed_texture_descriptor_textures(
  napi_env env, const mln_vulkan_borrowed_texture_descriptor* source
) {
  size_t count = (size_t)source->texture_count;
  size_t stride = sizeof(mln_vulkan_borrowed_texture);
  if (count > 0 && stride < sizeof(mln_vulkan_borrowed_texture)) {
    napi_throw_range_error(env, NULL, "native record stride is too small");
    return NULL;
  }
  napi_value result = mln_ts_array(env, count);
  if (result == NULL) return NULL;
  for (size_t index = 0; index < count; index++) {
    const mln_vulkan_borrowed_texture* item =
      (const mln_vulkan_borrowed_texture*)((const char*)source->textures +
                                           index * stride);
    napi_value element = mln_ts_read_vulkan_borrowed_texture(env, item, stride);
    if (element == NULL) return NULL;
    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;
  }
  return result;
}

napi_value mln_ts_read_vulkan_context_descriptor(
  napi_env env, const mln_vulkan_context_descriptor* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "instance",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->instance))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "physicalDevice",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->physical_device))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "device",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->device))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "graphicsQueue",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->graphics_queue))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "graphicsQueueFamilyIndex",
        mln_ts_number(env, (double)(source->graphics_queue_family_index))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "getInstanceProcAddr",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->get_instance_proc_addr))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "getDeviceProcAddr",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->get_device_proc_addr))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_vulkan_owned_texture_descriptor(
  napi_env env, const mln_vulkan_owned_texture_descriptor* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "extent",
        mln_ts_read_logical_extent(
          env, &(source->extent), sizeof(source->extent)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "context",
        mln_ts_read_vulkan_context_descriptor(
          env, &(source->context), sizeof(source->context)
        )
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_vulkan_surface_descriptor(
  napi_env env, const mln_vulkan_surface_descriptor* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "extent",
        mln_ts_read_logical_extent(
          env, &(source->extent), sizeof(source->extent)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "context",
        mln_ts_read_vulkan_context_descriptor(
          env, &(source->context), sizeof(source->context)
        )
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "surface", mln_ts_u64(env, (uint64_t)(source->surface))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_vulkan_texture_frame(
  napi_env env, const mln_vulkan_texture_frame* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "generation",
        mln_ts_u64(env, (uint64_t)(source->generation))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "width", mln_ts_number(env, (double)(source->width))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "height", mln_ts_number(env, (double)(source->height))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "scaleFactor",
        mln_ts_number(env, (double)(source->scale_factor))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "frameId", mln_ts_u64(env, (uint64_t)(source->frame_id))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "slot", mln_ts_number(env, (double)(source->slot))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "image", mln_ts_u64(env, (uint64_t)(source->image))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "imageView",
        mln_ts_u64(env, (uint64_t)(source->image_view))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "device",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->device))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "format", mln_ts_number(env, (double)(source->format))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "layout", mln_ts_number(env, (double)(source->layout))
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_webgl_context_descriptor(
  napi_env env, const mln_webgl_context_descriptor* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "kind", mln_ts_number(env, (double)(source->kind))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "context", mln_ts_number(env, (double)(source->context))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "canvasSelector",
        mln_ts_text(
          env, (source->canvas_selector).data, (source->canvas_selector).size
        )
      ))
    return NULL;
  return result;
}

napi_value mln_ts_read_wgl_context_descriptor(
  napi_env env, const mln_wgl_context_descriptor* source, size_t size
) {
  (void)size;
  napi_value result = mln_ts_object(env);
  if (result == NULL) return NULL;
  if (!mln_ts_set(
        env, result, "deviceContext",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->device_context))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "shareContext",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->share_context))
      ))
    return NULL;
  if (!mln_ts_set(
        env, result, "getProcAddress",
        mln_ts_u64(env, (uint64_t)(uintptr_t)(source->get_proc_address))
      ))
    return NULL;
  return result;
}

bool mln_ts_write_animation_options(
  mln_ts_call* call, napi_value value, mln_animation_options* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "durationMs", &member)) return false;
  mln_ts_path(child, sizeof child, path, "durationMs");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->duration_ms = (double)number;
    }
    out->fields |= MLN_ANIMATION_OPTION_DURATION;
  }
  if (!mln_ts_member(call, value, "velocity", &member)) return false;
  mln_ts_path(child, sizeof child, path, "velocity");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->velocity = (double)number;
    }
    out->fields |= MLN_ANIMATION_OPTION_VELOCITY;
  }
  if (!mln_ts_member(call, value, "minZoom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "minZoom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->min_zoom = (double)number;
    }
    out->fields |= MLN_ANIMATION_OPTION_MIN_ZOOM;
  }
  if (!mln_ts_member(call, value, "easing", &member)) return false;
  mln_ts_path(child, sizeof child, path, "easing");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_unit_bezier(call, member, &out->easing, child))
      return false;
    out->fields |= MLN_ANIMATION_OPTION_EASING;
  }
  if (!mln_ts_member(call, value, "transitionId", &member)) return false;
  mln_ts_path(child, sizeof child, path, "transitionId");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->transition_id = (uint64_t)number;
    }
    out->fields |= MLN_ANIMATION_OPTION_TRANSITION_ID;
  }
  if (!mln_ts_member(call, value, "endHandler", &member)) return false;
  mln_ts_path(child, sizeof child, path, "endHandler");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_function(call, member, child)) return false;
    if (!mln_ts_read_registration(
          call, member, &out->end_handler.user_data, &out->end_handler.queue,
          child
        ))
      return false;
  }
  return true;
}

bool mln_ts_write_bound_options(
  mln_ts_call* call, napi_value value, mln_bound_options* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "bounds", &member)) return false;
  mln_ts_path(child, sizeof child, path, "bounds");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_lat_lng_bounds(call, member, &out->bounds, child))
      return false;
    out->fields |= MLN_BOUND_OPTION_BOUNDS;
  }
  if (!mln_ts_member(call, value, "minZoom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "minZoom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->min_zoom = (double)number;
    }
    out->fields |= MLN_BOUND_OPTION_MIN_ZOOM;
  }
  if (!mln_ts_member(call, value, "maxZoom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "maxZoom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->max_zoom = (double)number;
    }
    out->fields |= MLN_BOUND_OPTION_MAX_ZOOM;
  }
  if (!mln_ts_member(call, value, "minPitch", &member)) return false;
  mln_ts_path(child, sizeof child, path, "minPitch");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->min_pitch = (double)number;
    }
    out->fields |= MLN_BOUND_OPTION_MIN_PITCH;
  }
  if (!mln_ts_member(call, value, "maxPitch", &member)) return false;
  mln_ts_path(child, sizeof child, path, "maxPitch");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->max_pitch = (double)number;
    }
    out->fields |= MLN_BOUND_OPTION_MAX_PITCH;
  }
  if (!mln_ts_member(call, value, "unbounded", &member)) return false;
  mln_ts_path(child, sizeof child, path, "unbounded");
  if (!mln_ts_is_absent(call, member)) {
    bool flag = false;
    if (!mln_ts_read_bool(call, member, &flag, child)) return false;
    if (flag) out->fields |= MLN_BOUND_OPTION_UNBOUNDED;
  }
  return true;
}

bool mln_ts_write_camera_delta(
  mln_ts_call* call, napi_value value, mln_camera_delta* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "offset", &member)) return false;
  mln_ts_path(child, sizeof child, path, "offset");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_screen_point(call, member, &out->offset, child))
      return false;
    out->fields |= MLN_CAMERA_DELTA_OFFSET;
  }
  if (!mln_ts_member(call, value, "scale", &member)) return false;
  mln_ts_path(child, sizeof child, path, "scale");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->scale = (double)number;
    }
    out->fields |= MLN_CAMERA_DELTA_SCALE;
  }
  if (!mln_ts_member(call, value, "bearing", &member)) return false;
  mln_ts_path(child, sizeof child, path, "bearing");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->bearing = (double)number;
    }
    out->fields |= MLN_CAMERA_DELTA_BEARING;
  }
  if (!mln_ts_member(call, value, "pitch", &member)) return false;
  mln_ts_path(child, sizeof child, path, "pitch");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->pitch = (double)number;
    }
    out->fields |= MLN_CAMERA_DELTA_PITCH;
  }
  if (!mln_ts_member(call, value, "anchor", &member)) return false;
  mln_ts_path(child, sizeof child, path, "anchor");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_screen_point(call, member, &out->anchor, child))
      return false;
    out->fields |= MLN_CAMERA_DELTA_ANCHOR;
  }
  if (!mln_ts_member(call, value, "animation", &member)) return false;
  mln_ts_path(child, sizeof child, path, "animation");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_animation_options(call, member, &out->animation, child))
      return false;
  }
  if (!mln_ts_member(call, value, "gesturePhase", &member)) return false;
  mln_ts_path(child, sizeof child, path, "gesturePhase");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->gesture_phase = (uint32_t)number;
    }
  }
  return true;
}

bool mln_ts_write_camera_fit_options(
  mln_ts_call* call, napi_value value, mln_camera_fit_options* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "padding", &member)) return false;
  mln_ts_path(child, sizeof child, path, "padding");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_edge_insets(call, member, &out->padding, child))
      return false;
    out->fields |= MLN_CAMERA_FIT_OPTION_PADDING;
  }
  if (!mln_ts_member(call, value, "bearing", &member)) return false;
  mln_ts_path(child, sizeof child, path, "bearing");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->bearing = (double)number;
    }
    out->fields |= MLN_CAMERA_FIT_OPTION_BEARING;
  }
  if (!mln_ts_member(call, value, "pitch", &member)) return false;
  mln_ts_path(child, sizeof child, path, "pitch");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->pitch = (double)number;
    }
    out->fields |= MLN_CAMERA_FIT_OPTION_PITCH;
  }
  return true;
}

bool mln_ts_write_camera_options(
  mln_ts_call* call, napi_value value, mln_camera_options* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "center", &member)) return false;
  mln_ts_path(child, sizeof child, path, "center");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_lat_lng(call, member, &out->center, child)) return false;
    out->fields |= MLN_CAMERA_OPTION_CENTER;
  }
  if (!mln_ts_member(call, value, "centerAltitude", &member)) return false;
  mln_ts_path(child, sizeof child, path, "centerAltitude");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->center_altitude = (double)number;
    }
    out->fields |= MLN_CAMERA_OPTION_CENTER_ALTITUDE;
  }
  if (!mln_ts_member(call, value, "padding", &member)) return false;
  mln_ts_path(child, sizeof child, path, "padding");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_edge_insets(call, member, &out->padding, child))
      return false;
    out->fields |= MLN_CAMERA_OPTION_PADDING;
  }
  if (!mln_ts_member(call, value, "anchor", &member)) return false;
  mln_ts_path(child, sizeof child, path, "anchor");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_screen_point(call, member, &out->anchor, child))
      return false;
    out->fields |= MLN_CAMERA_OPTION_ANCHOR;
  }
  if (!mln_ts_member(call, value, "zoom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "zoom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->zoom = (double)number;
    }
    out->fields |= MLN_CAMERA_OPTION_ZOOM;
  }
  if (!mln_ts_member(call, value, "bearing", &member)) return false;
  mln_ts_path(child, sizeof child, path, "bearing");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->bearing = (double)number;
    }
    out->fields |= MLN_CAMERA_OPTION_BEARING;
  }
  if (!mln_ts_member(call, value, "pitch", &member)) return false;
  mln_ts_path(child, sizeof child, path, "pitch");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->pitch = (double)number;
    }
    out->fields |= MLN_CAMERA_OPTION_PITCH;
  }
  if (!mln_ts_member(call, value, "roll", &member)) return false;
  mln_ts_path(child, sizeof child, path, "roll");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->roll = (double)number;
    }
    out->fields |= MLN_CAMERA_OPTION_ROLL;
  }
  if (!mln_ts_member(call, value, "fieldOfView", &member)) return false;
  mln_ts_path(child, sizeof child, path, "fieldOfView");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->field_of_view = (double)number;
    }
    out->fields |= MLN_CAMERA_OPTION_FOV;
  }
  return true;
}

bool mln_ts_write_camera_update(
  mln_ts_call* call, napi_value value, mln_camera_update* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "mode", &member)) return false;
  mln_ts_path(child, sizeof child, path, "mode");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->mode = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "camera", &member)) return false;
  mln_ts_path(child, sizeof child, path, "camera");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_camera_options(call, member, &out->camera, child))
      return false;
  }
  if (!mln_ts_member(call, value, "animation", &member)) return false;
  mln_ts_path(child, sizeof child, path, "animation");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_animation_options(call, member, &out->animation, child))
      return false;
  }
  if (!mln_ts_member(call, value, "gesturePhase", &member)) return false;
  mln_ts_path(child, sizeof child, path, "gesturePhase");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->gesture_phase = (uint32_t)number;
    }
  }
  return true;
}

bool mln_ts_write_canonical_tile_id(
  mln_ts_call* call, napi_value value, mln_canonical_tile_id* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "z", &member)) return false;
  mln_ts_path(child, sizeof child, path, "z");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->z = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "x", &member)) return false;
  mln_ts_path(child, sizeof child, path, "x");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->x = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "y", &member)) return false;
  mln_ts_path(child, sizeof child, path, "y");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->y = (uint32_t)number;
    }
  }
  return true;
}

bool mln_ts_write_custom_geometry_source_options(
  mln_ts_call* call, napi_value value, mln_custom_geometry_source_options* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  bool present = true;
  if (!mln_ts_member(call, value, "fetchTile", &member)) return false;
  mln_ts_path(child, sizeof child, path, "fetchTile");
  if (mln_ts_is_absent(call, member)) {
    present = false;
  } else {
    if (!mln_ts_read_function(call, member, child)) return false;
  }
  if (!mln_ts_member(call, value, "cancelTile", &member)) return false;
  mln_ts_path(child, sizeof child, path, "cancelTile");
  if (mln_ts_is_absent(call, member)) {
  } else {
    if (!mln_ts_read_function(call, member, child)) return false;
  }
  if (present) {
    if (!mln_ts_read_registration(
          call, value, &(*out).user_data, &(*out).queue, path
        ))
      return false;
  }
  if (!mln_ts_member(call, value, "minZoom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "minZoom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->min_zoom = (double)number;
    }
    out->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
  }
  if (!mln_ts_member(call, value, "maxZoom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "maxZoom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->max_zoom = (double)number;
    }
    out->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
  }
  if (!mln_ts_member(call, value, "tolerance", &member)) return false;
  mln_ts_path(child, sizeof child, path, "tolerance");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->tolerance = (double)number;
    }
    out->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE;
  }
  if (!mln_ts_member(call, value, "tileSize", &member)) return false;
  mln_ts_path(child, sizeof child, path, "tileSize");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->tile_size = (uint32_t)number;
    }
    out->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE;
  }
  if (!mln_ts_member(call, value, "buffer", &member)) return false;
  mln_ts_path(child, sizeof child, path, "buffer");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->buffer = (uint32_t)number;
    }
    out->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER;
  }
  if (!mln_ts_member(call, value, "clip", &member)) return false;
  mln_ts_path(child, sizeof child, path, "clip");
  if (!mln_ts_is_absent(call, member)) {
    {
      bool flag = false;
      if (!mln_ts_read_bool(call, member, &flag, child)) return false;
      out->clip = flag;
    }
    out->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP;
  }
  if (!mln_ts_member(call, value, "wrap", &member)) return false;
  mln_ts_path(child, sizeof child, path, "wrap");
  if (!mln_ts_is_absent(call, member)) {
    {
      bool flag = false;
      if (!mln_ts_read_bool(call, member, &flag, child)) return false;
      out->wrap = flag;
    }
    out->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
  }
  return true;
}

bool mln_ts_write_custom_mvt_vector_source_options(
  mln_ts_call* call, napi_value value,
  mln_custom_mvt_vector_source_options* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  bool present = true;
  if (!mln_ts_member(call, value, "fetchTile", &member)) return false;
  mln_ts_path(child, sizeof child, path, "fetchTile");
  if (mln_ts_is_absent(call, member)) {
    present = false;
  } else {
    if (!mln_ts_read_function(call, member, child)) return false;
  }
  if (!mln_ts_member(call, value, "cancelTile", &member)) return false;
  mln_ts_path(child, sizeof child, path, "cancelTile");
  if (mln_ts_is_absent(call, member)) {
  } else {
    if (!mln_ts_read_function(call, member, child)) return false;
  }
  if (present) {
    if (!mln_ts_read_registration(
          call, value, &(*out).user_data, &(*out).queue, path
        ))
      return false;
  }
  if (!mln_ts_member(call, value, "minZoom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "minZoom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->min_zoom = (double)number;
    }
    out->fields |= MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM;
  }
  if (!mln_ts_member(call, value, "maxZoom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "maxZoom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->max_zoom = (double)number;
    }
    out->fields |= MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
  }
  return true;
}

bool mln_ts_write_edge_insets(
  mln_ts_call* call, napi_value value, mln_edge_insets* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "top", &member)) return false;
  mln_ts_path(child, sizeof child, path, "top");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->top = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "left", &member)) return false;
  mln_ts_path(child, sizeof child, path, "left");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->left = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "bottom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "bottom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->bottom = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "right", &member)) return false;
  mln_ts_path(child, sizeof child, path, "right");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->right = (double)number;
    }
  }
  return true;
}

bool mln_ts_write_egl_context_descriptor(
  mln_ts_call* call, napi_value value, mln_egl_context_descriptor* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "display", &member)) return false;
  mln_ts_path(child, sizeof child, path, "display");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->display = (void*)(uintptr_t)number;
    }
  }
  if (!mln_ts_member(call, value, "config", &member)) return false;
  mln_ts_path(child, sizeof child, path, "config");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->config = (void*)(uintptr_t)number;
    }
  }
  if (!mln_ts_member(call, value, "shareContext", &member)) return false;
  mln_ts_path(child, sizeof child, path, "shareContext");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->share_context = (void*)(uintptr_t)number;
    }
  }
  if (!mln_ts_member(call, value, "clientApi", &member)) return false;
  mln_ts_path(child, sizeof child, path, "clientApi");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->client_api = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "getProcAddress", &member)) return false;
  mln_ts_path(child, sizeof child, path, "getProcAddress");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->get_proc_address = (void*)(uintptr_t)number;
    }
  }
  return true;
}

bool mln_ts_write_feature_state_selector(
  mln_ts_call* call, napi_value value, mln_feature_state_selector* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "sourceId", &member)) return false;
  mln_ts_path(child, sizeof child, path, "sourceId");
  if (!mln_ts_read_text_view(call, member, &out->source_id, child))
    return false;
  if (!mln_ts_member(call, value, "sourceLayerId", &member)) return false;
  mln_ts_path(child, sizeof child, path, "sourceLayerId");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_text_view(call, member, &out->source_layer_id, child))
      return false;
    out->fields |= MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID;
  }
  if (!mln_ts_member(call, value, "featureId", &member)) return false;
  mln_ts_path(child, sizeof child, path, "featureId");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_text_view(call, member, &out->feature_id, child))
      return false;
    out->fields |= MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
  }
  if (!mln_ts_member(call, value, "stateKey", &member)) return false;
  mln_ts_path(child, sizeof child, path, "stateKey");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_text_view(call, member, &out->state_key, child))
      return false;
    out->fields |= MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
  }
  return true;
}

bool mln_ts_write_frame_demand(
  mln_ts_call* call, napi_value value, mln_frame_demand* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "flags", &member)) return false;
  mln_ts_path(child, sizeof child, path, "flags");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->flags = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "token", &member)) return false;
  mln_ts_path(child, sizeof child, path, "token");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->token = (uint64_t)number;
    }
  }
  return true;
}

bool mln_ts_write_free_camera_options(
  mln_ts_call* call, napi_value value, mln_free_camera_options* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "position", &member)) return false;
  mln_ts_path(child, sizeof child, path, "position");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_vec3(call, member, &out->position, child)) return false;
    out->fields |= MLN_FREE_CAMERA_OPTION_POSITION;
  }
  if (!mln_ts_member(call, value, "orientation", &member)) return false;
  mln_ts_path(child, sizeof child, path, "orientation");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_quaternion(call, member, &out->orientation, child))
      return false;
    out->fields |= MLN_FREE_CAMERA_OPTION_ORIENTATION;
  }
  return true;
}

bool mln_ts_write_geojson_source_options(
  mln_ts_call* call, napi_value value, mln_geojson_source_options* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "minZoom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "minZoom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->min_zoom = (double)number;
    }
    out->fields |= MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM;
  }
  if (!mln_ts_member(call, value, "maxZoom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "maxZoom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->max_zoom = (double)number;
    }
    out->fields |= MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM;
  }
  if (!mln_ts_member(call, value, "tolerance", &member)) return false;
  mln_ts_path(child, sizeof child, path, "tolerance");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->tolerance = (double)number;
    }
    out->fields |= MLN_GEOJSON_SOURCE_OPTION_TOLERANCE;
  }
  if (!mln_ts_member(call, value, "clusterMaxZoom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "clusterMaxZoom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->cluster_max_zoom = (double)number;
    }
    out->fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM;
  }
  if (!mln_ts_member(call, value, "clusterProperties", &member)) return false;
  mln_ts_path(child, sizeof child, path, "clusterProperties");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_text_view(call, member, &out->cluster_properties, child))
      return false;
    out->fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
  }
  if (!mln_ts_member(call, value, "buffer", &member)) return false;
  mln_ts_path(child, sizeof child, path, "buffer");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->buffer = (uint32_t)number;
    }
    out->fields |= MLN_GEOJSON_SOURCE_OPTION_BUFFER;
  }
  if (!mln_ts_member(call, value, "clusterRadius", &member)) return false;
  mln_ts_path(child, sizeof child, path, "clusterRadius");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->cluster_radius = (uint32_t)number;
    }
    out->fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS;
  }
  if (!mln_ts_member(call, value, "clusterMinPoints", &member)) return false;
  mln_ts_path(child, sizeof child, path, "clusterMinPoints");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->cluster_min_points = (uint32_t)number;
    }
    out->fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS;
  }
  if (!mln_ts_member(call, value, "lineMetrics", &member)) return false;
  mln_ts_path(child, sizeof child, path, "lineMetrics");
  if (!mln_ts_is_absent(call, member)) {
    {
      bool flag = false;
      if (!mln_ts_read_bool(call, member, &flag, child)) return false;
      out->line_metrics = flag;
    }
    out->fields |= MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS;
  }
  if (!mln_ts_member(call, value, "cluster", &member)) return false;
  mln_ts_path(child, sizeof child, path, "cluster");
  if (!mln_ts_is_absent(call, member)) {
    {
      bool flag = false;
      if (!mln_ts_read_bool(call, member, &flag, child)) return false;
      out->cluster = flag;
    }
    out->fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  }
  if (!mln_ts_member(call, value, "synchronousTiling", &member)) return false;
  mln_ts_path(child, sizeof child, path, "synchronousTiling");
  if (!mln_ts_is_absent(call, member)) {
    {
      bool flag = false;
      if (!mln_ts_read_bool(call, member, &flag, child)) return false;
      out->synchronous_tiling = flag;
    }
    out->fields |= MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING;
  }
  return true;
}

bool mln_ts_write_gpu_sync(
  mln_ts_call* call, napi_value value, mln_gpu_sync* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "kind", &member)) return false;
  mln_ts_path(child, sizeof child, path, "kind");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->kind = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "object", &member)) return false;
  mln_ts_path(child, sizeof child, path, "object");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->object = (uint64_t)number;
    }
  }
  return true;
}

bool mln_ts_write_http_header(
  mln_ts_call* call, napi_value value, mln_http_header* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "name", &member)) return false;
  mln_ts_path(child, sizeof child, path, "name");
  if (!mln_ts_read_text_view(call, member, &out->name, child)) return false;
  if (!mln_ts_member(call, value, "value", &member)) return false;
  mln_ts_path(child, sizeof child, path, "value");
  if (!mln_ts_read_text_view(call, member, &out->value, child)) return false;
  return true;
}

bool mln_ts_write_http_header_rule(
  mln_ts_call* call, napi_value value, mln_http_header_rule* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "kinds", &member)) return false;
  mln_ts_path(child, sizeof child, path, "kinds");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->kinds = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "flags", &member)) return false;
  mln_ts_path(child, sizeof child, path, "flags");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->flags = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "url", &member)) return false;
  mln_ts_path(child, sizeof child, path, "url");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_text_view(call, member, &out->url, child)) return false;
  }
  if (!mln_ts_member(call, value, "headers", &member)) return false;
  mln_ts_path(child, sizeof child, path, "headers");
  if (!mln_ts_is_absent(call, member)) {
    {
      mln_http_header* items = NULL;
      size_t count = 0;
      if (!mln_ts_write_list_http_header(
            call, member, UINT32_MAX, &items, &count, child
          ))
        return false;
      out->headers = items;
      out->header_count = (size_t)count;
    }
  }
  return true;
}

bool mln_ts_write_http_header_transform(
  mln_ts_call* call, napi_value value, mln_http_header_transform* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "rules", &member)) return false;
  mln_ts_path(child, sizeof child, path, "rules");
  {
    mln_http_header_rule* items = NULL;
    size_t count = 0;
    if (!mln_ts_write_list_http_header_rule(
          call, member, UINT32_MAX, &items, &count, child
        ))
      return false;
    out->rules = items;
    out->rule_count = (size_t)count;
  }
  return true;
}

bool mln_ts_write_image_content(
  mln_ts_call* call, napi_value value, mln_image_content* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "left", &member)) return false;
  mln_ts_path(child, sizeof child, path, "left");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->left = (float)number;
    }
  }
  if (!mln_ts_member(call, value, "top", &member)) return false;
  mln_ts_path(child, sizeof child, path, "top");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->top = (float)number;
    }
  }
  if (!mln_ts_member(call, value, "right", &member)) return false;
  mln_ts_path(child, sizeof child, path, "right");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->right = (float)number;
    }
  }
  if (!mln_ts_member(call, value, "bottom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "bottom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->bottom = (float)number;
    }
  }
  return true;
}

bool mln_ts_write_image_stretch(
  mln_ts_call* call, napi_value value, mln_image_stretch* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "from", &member)) return false;
  mln_ts_path(child, sizeof child, path, "from");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->from = (float)number;
    }
  }
  if (!mln_ts_member(call, value, "to", &member)) return false;
  mln_ts_path(child, sizeof child, path, "to");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->to = (float)number;
    }
  }
  return true;
}

bool mln_ts_write_lat_lng(
  mln_ts_call* call, napi_value value, mln_lat_lng* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "latitude", &member)) return false;
  mln_ts_path(child, sizeof child, path, "latitude");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->latitude = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "longitude", &member)) return false;
  mln_ts_path(child, sizeof child, path, "longitude");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->longitude = (double)number;
    }
  }
  return true;
}

bool mln_ts_write_lat_lng_bounds(
  mln_ts_call* call, napi_value value, mln_lat_lng_bounds* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "southwest", &member)) return false;
  mln_ts_path(child, sizeof child, path, "southwest");
  if (!mln_ts_is_absent(call, member)) {
    memset(&out->southwest, 0, sizeof out->southwest);
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_lat_lng(call, member, &out->southwest, child))
      return false;
  }
  if (!mln_ts_member(call, value, "northeast", &member)) return false;
  mln_ts_path(child, sizeof child, path, "northeast");
  if (!mln_ts_is_absent(call, member)) {
    memset(&out->northeast, 0, sizeof out->northeast);
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_lat_lng(call, member, &out->northeast, child))
      return false;
  }
  return true;
}

bool mln_ts_write_list_buffer_view(
  mln_ts_call* call, napi_value value, size_t maximum, mln_buffer_view** out,
  size_t* out_count, const char* path
) {
  mln_buffer_view* items = NULL;
  if (!mln_ts_read_list(
        call, value, sizeof *items, maximum, (void**)&items, out_count, path
      ))
    return false;
  for (size_t index = 0; index < *out_count; index++) {
    napi_value element;
    char item_path[256];
    if (!mln_ts_element(call, value, index, &element)) return false;
    mln_ts_index_path(item_path, sizeof item_path, path, index);
    memset(&items[index], 0, sizeof items[index]);
    if (!mln_ts_read_text_view(call, element, &items[index], item_path))
      return false;
  }
  *out = items;
  return true;
}

bool mln_ts_write_list_http_header(
  mln_ts_call* call, napi_value value, size_t maximum, mln_http_header** out,
  size_t* out_count, const char* path
) {
  mln_http_header* items = NULL;
  if (!mln_ts_read_list(
        call, value, sizeof *items, maximum, (void**)&items, out_count, path
      ))
    return false;
  for (size_t index = 0; index < *out_count; index++) {
    napi_value element;
    char item_path[256];
    if (!mln_ts_element(call, value, index, &element)) return false;
    mln_ts_index_path(item_path, sizeof item_path, path, index);
    memset(&items[index], 0, sizeof items[index]);
    if (!mln_ts_read_record(call, element, item_path)) return false;
    if (!mln_ts_write_http_header(call, element, &items[index], item_path))
      return false;
  }
  *out = items;
  return true;
}

bool mln_ts_write_list_http_header_rule(
  mln_ts_call* call, napi_value value, size_t maximum,
  mln_http_header_rule** out, size_t* out_count, const char* path
) {
  mln_http_header_rule* items = NULL;
  if (!mln_ts_read_list(
        call, value, sizeof *items, maximum, (void**)&items, out_count, path
      ))
    return false;
  for (size_t index = 0; index < *out_count; index++) {
    napi_value element;
    char item_path[256];
    if (!mln_ts_element(call, value, index, &element)) return false;
    mln_ts_index_path(item_path, sizeof item_path, path, index);
    items[index] = mln_http_header_rule_default();
    if (!mln_ts_read_record(call, element, item_path)) return false;
    if (!mln_ts_write_http_header_rule(call, element, &items[index], item_path))
      return false;
  }
  *out = items;
  return true;
}

bool mln_ts_write_list_image_stretch(
  mln_ts_call* call, napi_value value, size_t maximum, mln_image_stretch** out,
  size_t* out_count, const char* path
) {
  mln_image_stretch* items = NULL;
  if (!mln_ts_read_list(
        call, value, sizeof *items, maximum, (void**)&items, out_count, path
      ))
    return false;
  for (size_t index = 0; index < *out_count; index++) {
    napi_value element;
    char item_path[256];
    if (!mln_ts_element(call, value, index, &element)) return false;
    mln_ts_index_path(item_path, sizeof item_path, path, index);
    memset(&items[index], 0, sizeof items[index]);
    if (!mln_ts_read_record(call, element, item_path)) return false;
    if (!mln_ts_write_image_stretch(call, element, &items[index], item_path))
      return false;
  }
  *out = items;
  return true;
}

bool mln_ts_write_list_lat_lng(
  mln_ts_call* call, napi_value value, size_t maximum, mln_lat_lng** out,
  size_t* out_count, const char* path
) {
  mln_lat_lng* items = NULL;
  if (!mln_ts_read_list(
        call, value, sizeof *items, maximum, (void**)&items, out_count, path
      ))
    return false;
  for (size_t index = 0; index < *out_count; index++) {
    napi_value element;
    char item_path[256];
    if (!mln_ts_element(call, value, index, &element)) return false;
    mln_ts_index_path(item_path, sizeof item_path, path, index);
    memset(&items[index], 0, sizeof items[index]);
    if (!mln_ts_read_record(call, element, item_path)) return false;
    if (!mln_ts_write_lat_lng(call, element, &items[index], item_path))
      return false;
  }
  *out = items;
  return true;
}

bool mln_ts_write_list_metal_borrowed_texture(
  mln_ts_call* call, napi_value value, size_t maximum,
  mln_metal_borrowed_texture** out, size_t* out_count, const char* path
) {
  mln_metal_borrowed_texture* items = NULL;
  if (!mln_ts_read_list(
        call, value, sizeof *items, maximum, (void**)&items, out_count, path
      ))
    return false;
  for (size_t index = 0; index < *out_count; index++) {
    napi_value element;
    char item_path[256];
    if (!mln_ts_element(call, value, index, &element)) return false;
    mln_ts_index_path(item_path, sizeof item_path, path, index);
    memset(&items[index], 0, sizeof items[index]);
    if (!mln_ts_read_record(call, element, item_path)) return false;
    if (!mln_ts_write_metal_borrowed_texture(
          call, element, &items[index], item_path
        ))
      return false;
  }
  *out = items;
  return true;
}

bool mln_ts_write_list_opengl_borrowed_texture(
  mln_ts_call* call, napi_value value, size_t maximum,
  mln_opengl_borrowed_texture** out, size_t* out_count, const char* path
) {
  mln_opengl_borrowed_texture* items = NULL;
  if (!mln_ts_read_list(
        call, value, sizeof *items, maximum, (void**)&items, out_count, path
      ))
    return false;
  for (size_t index = 0; index < *out_count; index++) {
    napi_value element;
    char item_path[256];
    if (!mln_ts_element(call, value, index, &element)) return false;
    mln_ts_index_path(item_path, sizeof item_path, path, index);
    memset(&items[index], 0, sizeof items[index]);
    if (!mln_ts_read_record(call, element, item_path)) return false;
    if (!mln_ts_write_opengl_borrowed_texture(
          call, element, &items[index], item_path
        ))
      return false;
  }
  *out = items;
  return true;
}

bool mln_ts_write_list_resource_route(
  mln_ts_call* call, napi_value value, size_t maximum, mln_resource_route** out,
  size_t* out_count, const char* path
) {
  mln_resource_route* items = NULL;
  if (!mln_ts_read_list(
        call, value, sizeof *items, maximum, (void**)&items, out_count, path
      ))
    return false;
  for (size_t index = 0; index < *out_count; index++) {
    napi_value element;
    char item_path[256];
    if (!mln_ts_element(call, value, index, &element)) return false;
    mln_ts_index_path(item_path, sizeof item_path, path, index);
    items[index] = mln_resource_route_default();
    if (!mln_ts_read_record(call, element, item_path)) return false;
    if (!mln_ts_write_resource_route(call, element, &items[index], item_path))
      return false;
  }
  *out = items;
  return true;
}

bool mln_ts_write_list_screen_point(
  mln_ts_call* call, napi_value value, size_t maximum, mln_screen_point** out,
  size_t* out_count, const char* path
) {
  mln_screen_point* items = NULL;
  if (!mln_ts_read_list(
        call, value, sizeof *items, maximum, (void**)&items, out_count, path
      ))
    return false;
  for (size_t index = 0; index < *out_count; index++) {
    napi_value element;
    char item_path[256];
    if (!mln_ts_element(call, value, index, &element)) return false;
    mln_ts_index_path(item_path, sizeof item_path, path, index);
    memset(&items[index], 0, sizeof items[index]);
    if (!mln_ts_read_record(call, element, item_path)) return false;
    if (!mln_ts_write_screen_point(call, element, &items[index], item_path))
      return false;
  }
  *out = items;
  return true;
}

bool mln_ts_write_list_vulkan_borrowed_texture(
  mln_ts_call* call, napi_value value, size_t maximum,
  mln_vulkan_borrowed_texture** out, size_t* out_count, const char* path
) {
  mln_vulkan_borrowed_texture* items = NULL;
  if (!mln_ts_read_list(
        call, value, sizeof *items, maximum, (void**)&items, out_count, path
      ))
    return false;
  for (size_t index = 0; index < *out_count; index++) {
    napi_value element;
    char item_path[256];
    if (!mln_ts_element(call, value, index, &element)) return false;
    mln_ts_index_path(item_path, sizeof item_path, path, index);
    memset(&items[index], 0, sizeof items[index]);
    if (!mln_ts_read_record(call, element, item_path)) return false;
    if (!mln_ts_write_vulkan_borrowed_texture(
          call, element, &items[index], item_path
        ))
      return false;
  }
  *out = items;
  return true;
}

bool mln_ts_write_logical_extent(
  mln_ts_call* call, napi_value value, mln_logical_extent* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "width", &member)) return false;
  mln_ts_path(child, sizeof child, path, "width");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->width = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "height", &member)) return false;
  mln_ts_path(child, sizeof child, path, "height");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->height = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "scaleFactor", &member)) return false;
  mln_ts_path(child, sizeof child, path, "scaleFactor");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->scale_factor = (double)number;
    }
  }
  return true;
}

bool mln_ts_write_map_options(
  mln_ts_call* call, napi_value value, mln_map_options* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "initialExtent", &member)) return false;
  mln_ts_path(child, sizeof child, path, "initialExtent");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_logical_extent(call, member, &out->initial_extent, child))
      return false;
  }
  if (!mln_ts_member(call, value, "mapMode", &member)) return false;
  mln_ts_path(child, sizeof child, path, "mapMode");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->map_mode = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "fastPforEnabled", &member)) return false;
  mln_ts_path(child, sizeof child, path, "fastPforEnabled");
  if (!mln_ts_is_absent(call, member)) {
    {
      bool flag = false;
      if (!mln_ts_read_bool(call, member, &flag, child)) return false;
      out->fast_pfor_enabled = flag;
    }
  }
  if (!mln_ts_member(call, value, "eventMask", &member)) return false;
  mln_ts_path(child, sizeof child, path, "eventMask");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->event_mask = (uint64_t)number;
    }
  }
  return true;
}

bool mln_ts_write_map_tile_options(
  mln_ts_call* call, napi_value value, mln_map_tile_options* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "prefetchZoomDelta", &member)) return false;
  mln_ts_path(child, sizeof child, path, "prefetchZoomDelta");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->prefetch_zoom_delta = (uint32_t)number;
    }
    out->fields |= MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
  }
  if (!mln_ts_member(call, value, "lodMinRadius", &member)) return false;
  mln_ts_path(child, sizeof child, path, "lodMinRadius");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->lod_min_radius = (double)number;
    }
    out->fields |= MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS;
  }
  if (!mln_ts_member(call, value, "lodScale", &member)) return false;
  mln_ts_path(child, sizeof child, path, "lodScale");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->lod_scale = (double)number;
    }
    out->fields |= MLN_MAP_TILE_OPTION_LOD_SCALE;
  }
  if (!mln_ts_member(call, value, "lodPitchThreshold", &member)) return false;
  mln_ts_path(child, sizeof child, path, "lodPitchThreshold");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->lod_pitch_threshold = (double)number;
    }
    out->fields |= MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD;
  }
  if (!mln_ts_member(call, value, "lodZoomShift", &member)) return false;
  mln_ts_path(child, sizeof child, path, "lodZoomShift");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->lod_zoom_shift = (double)number;
    }
    out->fields |= MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT;
  }
  if (!mln_ts_member(call, value, "lodMode", &member)) return false;
  mln_ts_path(child, sizeof child, path, "lodMode");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->lod_mode = (uint32_t)number;
    }
    out->fields |= MLN_MAP_TILE_OPTION_LOD_MODE;
  }
  return true;
}

bool mln_ts_write_map_view(
  mln_ts_call* call, napi_value value, mln_map_view* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "camera", &member)) return false;
  mln_ts_path(child, sizeof child, path, "camera");
  if (!mln_ts_is_absent(call, member)) {
    out->camera = mln_camera_options_default();
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_camera_options(call, member, &out->camera, child))
      return false;
  }
  if (!mln_ts_member(call, value, "extent", &member)) return false;
  mln_ts_path(child, sizeof child, path, "extent");
  if (!mln_ts_is_absent(call, member)) {
    out->extent = mln_logical_extent_default();
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_logical_extent(call, member, &out->extent, child))
      return false;
  }
  if (!mln_ts_member(call, value, "projectionMode", &member)) return false;
  mln_ts_path(child, sizeof child, path, "projectionMode");
  if (!mln_ts_is_absent(call, member)) {
    out->projection_mode = mln_projection_mode_default();
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_projection_mode(
          call, member, &out->projection_mode, child
        ))
      return false;
  }
  if (!mln_ts_member(call, value, "viewport", &member)) return false;
  mln_ts_path(child, sizeof child, path, "viewport");
  if (!mln_ts_is_absent(call, member)) {
    out->viewport = mln_map_viewport_options_default();
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_map_viewport_options(call, member, &out->viewport, child))
      return false;
  }
  return true;
}

bool mln_ts_write_map_viewport_options(
  mln_ts_call* call, napi_value value, mln_map_viewport_options* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "northOrientation", &member)) return false;
  mln_ts_path(child, sizeof child, path, "northOrientation");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->north_orientation = (uint32_t)number;
    }
    out->fields |= MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
  }
  if (!mln_ts_member(call, value, "constrainMode", &member)) return false;
  mln_ts_path(child, sizeof child, path, "constrainMode");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->constrain_mode = (uint32_t)number;
    }
    out->fields |= MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
  }
  if (!mln_ts_member(call, value, "viewportMode", &member)) return false;
  mln_ts_path(child, sizeof child, path, "viewportMode");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->viewport_mode = (uint32_t)number;
    }
    out->fields |= MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
  }
  if (!mln_ts_member(call, value, "frustumOffset", &member)) return false;
  mln_ts_path(child, sizeof child, path, "frustumOffset");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_edge_insets(call, member, &out->frustum_offset, child))
      return false;
    out->fields |= MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
  }
  return true;
}

bool mln_ts_write_metal_borrowed_texture(
  mln_ts_call* call, napi_value value, mln_metal_borrowed_texture* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "texture", &member)) return false;
  mln_ts_path(child, sizeof child, path, "texture");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->texture = (void*)(uintptr_t)number;
    }
  }
  return true;
}

bool mln_ts_write_metal_borrowed_texture_descriptor(
  mln_ts_call* call, napi_value value,
  mln_metal_borrowed_texture_descriptor* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "extent", &member)) return false;
  mln_ts_path(child, sizeof child, path, "extent");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_logical_extent(call, member, &out->extent, child))
      return false;
  }
  if (!mln_ts_member(call, value, "physicalWidth", &member)) return false;
  mln_ts_path(child, sizeof child, path, "physicalWidth");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->physical_width = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "physicalHeight", &member)) return false;
  mln_ts_path(child, sizeof child, path, "physicalHeight");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->physical_height = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "textures", &member)) return false;
  mln_ts_path(child, sizeof child, path, "textures");
  if (!mln_ts_is_absent(call, member)) {
    {
      mln_metal_borrowed_texture* items = NULL;
      size_t count = 0;
      if (!mln_ts_write_list_metal_borrowed_texture(
            call, member, UINT32_MAX, &items, &count, child
          ))
        return false;
      out->textures = items;
      out->texture_count = (size_t)count;
    }
  }
  return true;
}

bool mln_ts_write_metal_context_descriptor(
  mln_ts_call* call, napi_value value, mln_metal_context_descriptor* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "device", &member)) return false;
  mln_ts_path(child, sizeof child, path, "device");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->device = (void*)(uintptr_t)number;
    }
  }
  return true;
}

bool mln_ts_write_metal_owned_texture_descriptor(
  mln_ts_call* call, napi_value value, mln_metal_owned_texture_descriptor* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "extent", &member)) return false;
  mln_ts_path(child, sizeof child, path, "extent");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_logical_extent(call, member, &out->extent, child))
      return false;
  }
  if (!mln_ts_member(call, value, "context", &member)) return false;
  mln_ts_path(child, sizeof child, path, "context");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_metal_context_descriptor(
          call, member, &out->context, child
        ))
      return false;
  }
  if (!mln_ts_member(call, value, "pixelFormat", &member)) return false;
  mln_ts_path(child, sizeof child, path, "pixelFormat");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->pixel_format = (uint64_t)number;
    }
  }
  return true;
}

bool mln_ts_write_metal_surface_descriptor(
  mln_ts_call* call, napi_value value, mln_metal_surface_descriptor* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "extent", &member)) return false;
  mln_ts_path(child, sizeof child, path, "extent");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_logical_extent(call, member, &out->extent, child))
      return false;
  }
  if (!mln_ts_member(call, value, "context", &member)) return false;
  mln_ts_path(child, sizeof child, path, "context");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_metal_context_descriptor(
          call, member, &out->context, child
        ))
      return false;
  }
  if (!mln_ts_member(call, value, "layer", &member)) return false;
  mln_ts_path(child, sizeof child, path, "layer");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->layer = (void*)(uintptr_t)number;
    }
  }
  return true;
}

bool mln_ts_write_offline_geometry_region_definition(
  mln_ts_call* call, napi_value value,
  mln_offline_geometry_region_definition* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "styleUrl", &member)) return false;
  mln_ts_path(child, sizeof child, path, "styleUrl");
  if (!mln_ts_read_text_view(call, member, &out->style_url, child))
    return false;
  if (!mln_ts_member(call, value, "geometry", &member)) return false;
  mln_ts_path(child, sizeof child, path, "geometry");
  if (!mln_ts_read_text_view(call, member, &out->geometry, child)) return false;
  if (!mln_ts_member(call, value, "minZoom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "minZoom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->min_zoom = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "maxZoom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "maxZoom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->max_zoom = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "pixelRatio", &member)) return false;
  mln_ts_path(child, sizeof child, path, "pixelRatio");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->pixel_ratio = (float)number;
    }
  }
  if (!mln_ts_member(call, value, "includeIdeographs", &member)) return false;
  mln_ts_path(child, sizeof child, path, "includeIdeographs");
  if (!mln_ts_is_absent(call, member)) {
    {
      bool flag = false;
      if (!mln_ts_read_bool(call, member, &flag, child)) return false;
      out->include_ideographs = flag;
    }
  }
  return true;
}

bool mln_ts_write_offline_region_definition(
  mln_ts_call* call, napi_value value, mln_offline_region_definition* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "data", &member)) return false;
  mln_ts_path(child, sizeof child, path, "data");
  if (!mln_ts_read_record(call, member, child)) return false;
  {
    static const char* const kinds[] = {"tilePyramid", "geometry"};
    napi_value arm;
    char arm_path[256];
    (void)arm;
    (void)arm_path;
    switch (mln_ts_read_kind(call, member, kinds, 2, child)) {
      case 0:
        out->type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID;
        if (!mln_ts_member(call, member, "value", &arm)) return false;
        mln_ts_path(arm_path, sizeof arm_path, child, "value");
        memset(&out->data.tile_pyramid, 0, sizeof out->data.tile_pyramid);
        if (!mln_ts_read_record(call, arm, arm_path)) return false;
        if (!mln_ts_write_offline_tile_pyramid_region_definition(
              call, arm, &out->data.tile_pyramid, arm_path
            ))
          return false;
        break;
      case 1:
        out->type = MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
        if (!mln_ts_member(call, member, "value", &arm)) return false;
        mln_ts_path(arm_path, sizeof arm_path, child, "value");
        memset(&out->data.geometry, 0, sizeof out->data.geometry);
        if (!mln_ts_read_record(call, arm, arm_path)) return false;
        if (!mln_ts_write_offline_geometry_region_definition(
              call, arm, &out->data.geometry, arm_path
            ))
          return false;
        break;
      default:
        return false;
    }
  }
  return true;
}

bool mln_ts_write_offline_tile_pyramid_region_definition(
  mln_ts_call* call, napi_value value,
  mln_offline_tile_pyramid_region_definition* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "styleUrl", &member)) return false;
  mln_ts_path(child, sizeof child, path, "styleUrl");
  if (!mln_ts_read_text_view(call, member, &out->style_url, child))
    return false;
  if (!mln_ts_member(call, value, "bounds", &member)) return false;
  mln_ts_path(child, sizeof child, path, "bounds");
  if (!mln_ts_is_absent(call, member)) {
    memset(&out->bounds, 0, sizeof out->bounds);
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_lat_lng_bounds(call, member, &out->bounds, child))
      return false;
  }
  if (!mln_ts_member(call, value, "minZoom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "minZoom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->min_zoom = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "maxZoom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "maxZoom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->max_zoom = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "pixelRatio", &member)) return false;
  mln_ts_path(child, sizeof child, path, "pixelRatio");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->pixel_ratio = (float)number;
    }
  }
  if (!mln_ts_member(call, value, "includeIdeographs", &member)) return false;
  mln_ts_path(child, sizeof child, path, "includeIdeographs");
  if (!mln_ts_is_absent(call, member)) {
    {
      bool flag = false;
      if (!mln_ts_read_bool(call, member, &flag, child)) return false;
      out->include_ideographs = flag;
    }
  }
  return true;
}

bool mln_ts_write_opengl_borrowed_texture(
  mln_ts_call* call, napi_value value, mln_opengl_borrowed_texture* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "texture", &member)) return false;
  mln_ts_path(child, sizeof child, path, "texture");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->texture = (uint32_t)number;
    }
  }
  return true;
}

bool mln_ts_write_opengl_borrowed_texture_descriptor(
  mln_ts_call* call, napi_value value,
  mln_opengl_borrowed_texture_descriptor* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "extent", &member)) return false;
  mln_ts_path(child, sizeof child, path, "extent");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_logical_extent(call, member, &out->extent, child))
      return false;
  }
  if (!mln_ts_member(call, value, "physicalWidth", &member)) return false;
  mln_ts_path(child, sizeof child, path, "physicalWidth");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->physical_width = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "physicalHeight", &member)) return false;
  mln_ts_path(child, sizeof child, path, "physicalHeight");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->physical_height = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "context", &member)) return false;
  mln_ts_path(child, sizeof child, path, "context");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_opengl_context_descriptor(
          call, member, &out->context, child
        ))
      return false;
  }
  if (!mln_ts_member(call, value, "textures", &member)) return false;
  mln_ts_path(child, sizeof child, path, "textures");
  if (!mln_ts_is_absent(call, member)) {
    {
      mln_opengl_borrowed_texture* items = NULL;
      size_t count = 0;
      if (!mln_ts_write_list_opengl_borrowed_texture(
            call, member, UINT32_MAX, &items, &count, child
          ))
        return false;
      out->textures = items;
      out->texture_count = (size_t)count;
    }
  }
  if (!mln_ts_member(call, value, "target", &member)) return false;
  mln_ts_path(child, sizeof child, path, "target");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->target = (uint32_t)number;
    }
  }
  return true;
}

bool mln_ts_write_opengl_context_descriptor(
  mln_ts_call* call, napi_value value, mln_opengl_context_descriptor* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "ownership", &member)) return false;
  mln_ts_path(child, sizeof child, path, "ownership");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->ownership = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "data", &member)) return false;
  mln_ts_path(child, sizeof child, path, "data");
  if (!mln_ts_read_record(call, member, child)) return false;
  {
    static const char* const kinds[] = {"wgl", "egl", "webgl"};
    napi_value arm;
    char arm_path[256];
    (void)arm;
    (void)arm_path;
    switch (mln_ts_read_kind(call, member, kinds, 3, child)) {
      case 0:
        out->platform = MLN_OPENGL_CONTEXT_PLATFORM_WGL;
        if (!mln_ts_member(call, member, "value", &arm)) return false;
        mln_ts_path(arm_path, sizeof arm_path, child, "value");
        memset(&out->data.wgl, 0, sizeof out->data.wgl);
        if (!mln_ts_read_record(call, arm, arm_path)) return false;
        if (!mln_ts_write_wgl_context_descriptor(
              call, arm, &out->data.wgl, arm_path
            ))
          return false;
        break;
      case 1:
        out->platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL;
        if (!mln_ts_member(call, member, "value", &arm)) return false;
        mln_ts_path(arm_path, sizeof arm_path, child, "value");
        memset(&out->data.egl, 0, sizeof out->data.egl);
        if (!mln_ts_read_record(call, arm, arm_path)) return false;
        if (!mln_ts_write_egl_context_descriptor(
              call, arm, &out->data.egl, arm_path
            ))
          return false;
        break;
      case 2:
        out->platform = MLN_OPENGL_CONTEXT_PLATFORM_WEBGL;
        if (!mln_ts_member(call, member, "value", &arm)) return false;
        mln_ts_path(arm_path, sizeof arm_path, child, "value");
        memset(&out->data.webgl, 0, sizeof out->data.webgl);
        if (!mln_ts_read_record(call, arm, arm_path)) return false;
        if (!mln_ts_write_webgl_context_descriptor(
              call, arm, &out->data.webgl, arm_path
            ))
          return false;
        break;
      default:
        return false;
    }
  }
  return true;
}

bool mln_ts_write_opengl_owned_texture_descriptor(
  mln_ts_call* call, napi_value value, mln_opengl_owned_texture_descriptor* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "extent", &member)) return false;
  mln_ts_path(child, sizeof child, path, "extent");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_logical_extent(call, member, &out->extent, child))
      return false;
  }
  if (!mln_ts_member(call, value, "context", &member)) return false;
  mln_ts_path(child, sizeof child, path, "context");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_opengl_context_descriptor(
          call, member, &out->context, child
        ))
      return false;
  }
  return true;
}

bool mln_ts_write_opengl_surface_descriptor(
  mln_ts_call* call, napi_value value, mln_opengl_surface_descriptor* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "extent", &member)) return false;
  mln_ts_path(child, sizeof child, path, "extent");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_logical_extent(call, member, &out->extent, child))
      return false;
  }
  if (!mln_ts_member(call, value, "context", &member)) return false;
  mln_ts_path(child, sizeof child, path, "context");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_opengl_context_descriptor(
          call, member, &out->context, child
        ))
      return false;
  }
  if (!mln_ts_member(call, value, "surface", &member)) return false;
  mln_ts_path(child, sizeof child, path, "surface");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->surface = (void*)(uintptr_t)number;
    }
  }
  return true;
}

bool mln_ts_write_premultiplied_rgba8_image(
  mln_ts_call* call, napi_value value, mln_premultiplied_rgba8_image* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "width", &member)) return false;
  mln_ts_path(child, sizeof child, path, "width");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->width = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "height", &member)) return false;
  mln_ts_path(child, sizeof child, path, "height");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->height = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "stride", &member)) return false;
  mln_ts_path(child, sizeof child, path, "stride");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->stride = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "pixels", &member)) return false;
  mln_ts_path(child, sizeof child, path, "pixels");
  if (!mln_ts_is_absent(call, member)) {
    {
      const void* data = NULL;
      size_t size = 0;
      if (!mln_ts_read_sized(
            call, member, true, UINT32_MAX, &data, &size, child
          ))
        return false;
      out->pixels = data;
      out->byte_length = (size_t)size;
    }
  }
  return true;
}

bool mln_ts_write_projected_meters(
  mln_ts_call* call, napi_value value, mln_projected_meters* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "northing", &member)) return false;
  mln_ts_path(child, sizeof child, path, "northing");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->northing = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "easting", &member)) return false;
  mln_ts_path(child, sizeof child, path, "easting");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->easting = (double)number;
    }
  }
  return true;
}

bool mln_ts_write_projection_mode(
  mln_ts_call* call, napi_value value, mln_projection_mode* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "axonometric", &member)) return false;
  mln_ts_path(child, sizeof child, path, "axonometric");
  if (!mln_ts_is_absent(call, member)) {
    {
      bool flag = false;
      if (!mln_ts_read_bool(call, member, &flag, child)) return false;
      out->axonometric = flag;
    }
    out->fields |= MLN_PROJECTION_MODE_AXONOMETRIC;
  }
  if (!mln_ts_member(call, value, "xSkew", &member)) return false;
  mln_ts_path(child, sizeof child, path, "xSkew");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->x_skew = (double)number;
    }
    out->fields |= MLN_PROJECTION_MODE_X_SKEW;
  }
  if (!mln_ts_member(call, value, "ySkew", &member)) return false;
  mln_ts_path(child, sizeof child, path, "ySkew");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->y_skew = (double)number;
    }
    out->fields |= MLN_PROJECTION_MODE_Y_SKEW;
  }
  return true;
}

bool mln_ts_write_quaternion(
  mln_ts_call* call, napi_value value, mln_quaternion* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "x", &member)) return false;
  mln_ts_path(child, sizeof child, path, "x");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->x = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "y", &member)) return false;
  mln_ts_path(child, sizeof child, path, "y");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->y = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "z", &member)) return false;
  mln_ts_path(child, sizeof child, path, "z");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->z = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "w", &member)) return false;
  mln_ts_path(child, sizeof child, path, "w");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->w = (double)number;
    }
  }
  return true;
}

bool mln_ts_write_render_session_attach_options(
  mln_ts_call* call, napi_value value, mln_render_session_attach_options* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "driver", &member)) return false;
  mln_ts_path(child, sizeof child, path, "driver");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->driver = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "requestedTextureRingDepth", &member))
    return false;
  mln_ts_path(child, sizeof child, path, "requestedTextureRingDepth");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->requested_texture_ring_depth = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "producerSyncKind", &member)) return false;
  mln_ts_path(child, sizeof child, path, "producerSyncKind");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->producer_sync_kind = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "frameWake", &member)) return false;
  mln_ts_path(child, sizeof child, path, "frameWake");
  if (!mln_ts_is_absent(call, member)) {
    if (mln_ts_is_object(call, member)) {
      {
        uint64_t fields[3];
        if (!mln_ts_read_native_registration(
              call, member, "Wake", 1, fields, child
            ))
          return false;
        out->frame_wake.callback = (mln_wake_callback)(uintptr_t)fields[0];
        out->frame_wake.user_data = (void*)(uintptr_t)fields[1];
        out->frame_wake.release_user_data =
          (mln_user_data_release)(uintptr_t)fields[2];
      }
    } else {
      if (!mln_ts_read_wake(
            call, member, (void**)&out->frame_wake.user_data, child
          ))
        return false;
      if (out->frame_wake.user_data != NULL) {
        out->frame_wake.callback = mln_ts_wake;
        out->frame_wake.release_user_data = mln_ts_wake_release;
      }
    }
  }
  if (!mln_ts_member(call, value, "driverWorkWake", &member)) return false;
  mln_ts_path(child, sizeof child, path, "driverWorkWake");
  if (!mln_ts_is_absent(call, member)) {
    if (mln_ts_is_object(call, member)) {
      {
        uint64_t fields[3];
        if (!mln_ts_read_native_registration(
              call, member, "Wake", 1, fields, child
            ))
          return false;
        out->driver_work_wake.callback =
          (mln_wake_callback)(uintptr_t)fields[0];
        out->driver_work_wake.user_data = (void*)(uintptr_t)fields[1];
        out->driver_work_wake.release_user_data =
          (mln_user_data_release)(uintptr_t)fields[2];
      }
    } else {
      if (!mln_ts_read_wake(
            call, member, (void**)&out->driver_work_wake.user_data, child
          ))
        return false;
      if (out->driver_work_wake.user_data != NULL) {
        out->driver_work_wake.callback = mln_ts_wake;
        out->driver_work_wake.release_user_data = mln_ts_wake_release;
      }
    }
  }
  if (!mln_ts_member(call, value, "queueLock", &member)) return false;
  mln_ts_path(child, sizeof child, path, "queueLock");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t fields[4];
      if (!mln_ts_read_native_registration(
            call, member, "QueueLock", 2, fields, child
          ))
        return false;
      out->queue_lock.lock = (mln_queue_lock_callback)(uintptr_t)fields[0];
      out->queue_lock.unlock = (mln_queue_lock_callback)(uintptr_t)fields[1];
      out->queue_lock.user_data = (void*)(uintptr_t)fields[2];
      out->queue_lock.release_user_data =
        (mln_user_data_release)(uintptr_t)fields[3];
    }
  }
  return true;
}

bool mln_ts_write_rendered_feature_query_options(
  mln_ts_call* call, napi_value value, mln_rendered_feature_query_options* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "layerIds", &member)) return false;
  mln_ts_path(child, sizeof child, path, "layerIds");
  if (!mln_ts_is_absent(call, member)) {
    {
      mln_buffer_view* items = NULL;
      size_t count = 0;
      if (!mln_ts_write_list_buffer_view(
            call, member, UINT32_MAX, &items, &count, child
          ))
        return false;
      out->layer_ids = items;
      out->layer_id_count = (size_t)count;
    }
    out->fields |= MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
  }
  if (!mln_ts_member(call, value, "filter", &member)) return false;
  mln_ts_path(child, sizeof child, path, "filter");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_text_view(call, member, &out->filter, child)) return false;
    out->fields |= MLN_RENDERED_FEATURE_QUERY_OPTION_FILTER;
  }
  return true;
}

bool mln_ts_write_rendered_query_geometry(
  mln_ts_call* call, napi_value value, mln_rendered_query_geometry* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "data", &member)) return false;
  mln_ts_path(child, sizeof child, path, "data");
  if (!mln_ts_read_record(call, member, child)) return false;
  {
    static const char* const kinds[] = {"point", "box", "lineString"};
    napi_value arm;
    char arm_path[256];
    (void)arm;
    (void)arm_path;
    switch (mln_ts_read_kind(call, member, kinds, 3, child)) {
      case 0:
        out->type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT;
        if (!mln_ts_member(call, member, "value", &arm)) return false;
        mln_ts_path(arm_path, sizeof arm_path, child, "value");
        memset(&out->data.point, 0, sizeof out->data.point);
        if (!mln_ts_read_record(call, arm, arm_path)) return false;
        if (!mln_ts_write_screen_point(call, arm, &out->data.point, arm_path))
          return false;
        break;
      case 1:
        out->type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX;
        if (!mln_ts_member(call, member, "value", &arm)) return false;
        mln_ts_path(arm_path, sizeof arm_path, child, "value");
        memset(&out->data.box, 0, sizeof out->data.box);
        if (!mln_ts_read_record(call, arm, arm_path)) return false;
        if (!mln_ts_write_screen_box(call, arm, &out->data.box, arm_path))
          return false;
        break;
      case 2:
        out->type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING;
        if (!mln_ts_member(call, member, "value", &arm)) return false;
        mln_ts_path(arm_path, sizeof arm_path, child, "value");
        memset(&out->data.line_string, 0, sizeof out->data.line_string);
        if (!mln_ts_read_record(call, arm, arm_path)) return false;
        if (!mln_ts_write_screen_line_string(
              call, arm, &out->data.line_string, arm_path
            ))
          return false;
        break;
      default:
        return false;
    }
  }
  return true;
}

bool mln_ts_write_resource_provider(
  mln_ts_call* call, napi_value value, mln_resource_provider* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "callback", &member)) return false;
  mln_ts_path(child, sizeof child, path, "callback");
  if (!mln_ts_read_function(call, member, child)) return false;
  if (!mln_ts_read_registration(
        call, value, &(*out).user_data, &(*out).queue, path
      ))
    return false;
  if (!mln_ts_member(call, value, "routes", &member)) return false;
  mln_ts_path(child, sizeof child, path, "routes");
  {
    mln_resource_route* items = NULL;
    size_t count = 0;
    if (!mln_ts_write_list_resource_route(
          call, member, UINT32_MAX, &items, &count, child
        ))
      return false;
    out->routes = items;
    out->route_count = (size_t)count;
  }
  return true;
}

bool mln_ts_write_resource_response(
  mln_ts_call* call, napi_value value, mln_resource_response* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "status", &member)) return false;
  mln_ts_path(child, sizeof child, path, "status");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->status = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "errorReason", &member)) return false;
  mln_ts_path(child, sizeof child, path, "errorReason");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->error_reason = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "bytes", &member)) return false;
  mln_ts_path(child, sizeof child, path, "bytes");
  if (!mln_ts_is_absent(call, member)) {
    {
      const void* data = NULL;
      size_t size = 0;
      if (!mln_ts_read_sized(
            call, member, true, UINT32_MAX, &data, &size, child
          ))
        return false;
      out->bytes = data;
      out->byte_count = (size_t)size;
    }
  }
  if (!mln_ts_member(call, value, "errorMessage", &member)) return false;
  mln_ts_path(child, sizeof child, path, "errorMessage");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_text_view(call, member, &out->error_message, child))
      return false;
  }
  if (!mln_ts_member(call, value, "mustRevalidate", &member)) return false;
  mln_ts_path(child, sizeof child, path, "mustRevalidate");
  if (!mln_ts_is_absent(call, member)) {
    {
      bool flag = false;
      if (!mln_ts_read_bool(call, member, &flag, child)) return false;
      out->must_revalidate = flag;
    }
  }
  if (!mln_ts_member(call, value, "modifiedUnixMs", &member)) return false;
  mln_ts_path(child, sizeof child, path, "modifiedUnixMs");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_i64(call, member, &number, child)) return false;
      out->modified_unix_ms = (int64_t)number;
    }
    out->fields |= MLN_RESOURCE_RESPONSE_MODIFIED;
  }
  if (!mln_ts_member(call, value, "expiresUnixMs", &member)) return false;
  mln_ts_path(child, sizeof child, path, "expiresUnixMs");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_i64(call, member, &number, child)) return false;
      out->expires_unix_ms = (int64_t)number;
    }
    out->fields |= MLN_RESOURCE_RESPONSE_EXPIRES;
  }
  if (!mln_ts_member(call, value, "etag", &member)) return false;
  mln_ts_path(child, sizeof child, path, "etag");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_text_view(call, member, &out->etag, child)) return false;
  }
  if (!mln_ts_member(call, value, "retryAfterUnixMs", &member)) return false;
  mln_ts_path(child, sizeof child, path, "retryAfterUnixMs");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_i64(call, member, &number, child)) return false;
      out->retry_after_unix_ms = (int64_t)number;
    }
    out->fields |= MLN_RESOURCE_RESPONSE_RETRY_AFTER;
  }
  return true;
}

bool mln_ts_write_resource_route(
  mln_ts_call* call, napi_value value, mln_resource_route* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "kinds", &member)) return false;
  mln_ts_path(child, sizeof child, path, "kinds");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->kinds = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "flags", &member)) return false;
  mln_ts_path(child, sizeof child, path, "flags");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->flags = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "url", &member)) return false;
  mln_ts_path(child, sizeof child, path, "url");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_text_view(call, member, &out->url, child)) return false;
  }
  return true;
}

bool mln_ts_write_runtime_options(
  mln_ts_call* call, napi_value value, mln_runtime_options* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "assetPath", &member)) return false;
  mln_ts_path(child, sizeof child, path, "assetPath");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_text_view(call, member, &out->asset_path, child))
      return false;
  }
  if (!mln_ts_member(call, value, "cachePath", &member)) return false;
  mln_ts_path(child, sizeof child, path, "cachePath");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_text_view(call, member, &out->cache_path, child))
      return false;
  }
  return true;
}

bool mln_ts_write_screen_box(
  mln_ts_call* call, napi_value value, mln_screen_box* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "min", &member)) return false;
  mln_ts_path(child, sizeof child, path, "min");
  if (!mln_ts_is_absent(call, member)) {
    memset(&out->min, 0, sizeof out->min);
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_screen_point(call, member, &out->min, child))
      return false;
  }
  if (!mln_ts_member(call, value, "max", &member)) return false;
  mln_ts_path(child, sizeof child, path, "max");
  if (!mln_ts_is_absent(call, member)) {
    memset(&out->max, 0, sizeof out->max);
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_screen_point(call, member, &out->max, child))
      return false;
  }
  return true;
}

bool mln_ts_write_screen_line_string(
  mln_ts_call* call, napi_value value, mln_screen_line_string* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "points", &member)) return false;
  mln_ts_path(child, sizeof child, path, "points");
  {
    mln_screen_point* items = NULL;
    size_t count = 0;
    if (!mln_ts_write_list_screen_point(
          call, member, UINT32_MAX, &items, &count, child
        ))
      return false;
    out->points = items;
    out->point_count = (size_t)count;
  }
  return true;
}

bool mln_ts_write_screen_point(
  mln_ts_call* call, napi_value value, mln_screen_point* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "x", &member)) return false;
  mln_ts_path(child, sizeof child, path, "x");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->x = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "y", &member)) return false;
  mln_ts_path(child, sizeof child, path, "y");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->y = (double)number;
    }
  }
  return true;
}

bool mln_ts_write_source_feature_query_options(
  mln_ts_call* call, napi_value value, mln_source_feature_query_options* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "sourceLayerIds", &member)) return false;
  mln_ts_path(child, sizeof child, path, "sourceLayerIds");
  if (!mln_ts_is_absent(call, member)) {
    {
      mln_buffer_view* items = NULL;
      size_t count = 0;
      if (!mln_ts_write_list_buffer_view(
            call, member, UINT32_MAX, &items, &count, child
          ))
        return false;
      out->source_layer_ids = items;
      out->source_layer_id_count = (size_t)count;
    }
    out->fields |= MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
  }
  if (!mln_ts_member(call, value, "filter", &member)) return false;
  mln_ts_path(child, sizeof child, path, "filter");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_text_view(call, member, &out->filter, child)) return false;
    out->fields |= MLN_SOURCE_FEATURE_QUERY_OPTION_FILTER;
  }
  return true;
}

bool mln_ts_write_style_expression_inputs(
  mln_ts_call* call, napi_value value, mln_style_expression_inputs* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "zoom", &member)) return false;
  mln_ts_path(child, sizeof child, path, "zoom");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->zoom = (double)number;
    }
    out->fields |= MLN_STYLE_EXPRESSION_INPUT_ZOOM;
  }
  if (!mln_ts_member(call, value, "colorRampParameter", &member)) return false;
  mln_ts_path(child, sizeof child, path, "colorRampParameter");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->color_ramp_parameter = (double)number;
    }
    out->fields |= MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER;
  }
  if (!mln_ts_member(call, value, "elevation", &member)) return false;
  mln_ts_path(child, sizeof child, path, "elevation");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->elevation = (double)number;
    }
    out->fields |= MLN_STYLE_EXPRESSION_INPUT_ELEVATION;
  }
  if (!mln_ts_member(call, value, "feature", &member)) return false;
  mln_ts_path(child, sizeof child, path, "feature");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_text_view(call, member, &out->feature, child))
      return false;
    out->fields |= MLN_STYLE_EXPRESSION_INPUT_FEATURE;
  }
  if (!mln_ts_member(call, value, "featureState", &member)) return false;
  mln_ts_path(child, sizeof child, path, "featureState");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_text_view(call, member, &out->feature_state, child))
      return false;
    out->fields |= MLN_STYLE_EXPRESSION_INPUT_FEATURE_STATE;
  }
  if (!mln_ts_member(call, value, "globalState", &member)) return false;
  mln_ts_path(child, sizeof child, path, "globalState");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_text_view(call, member, &out->global_state, child))
      return false;
    out->fields |= MLN_STYLE_EXPRESSION_INPUT_GLOBAL_STATE;
  }
  if (!mln_ts_member(call, value, "canonicalTileId", &member)) return false;
  mln_ts_path(child, sizeof child, path, "canonicalTileId");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_canonical_tile_id(
          call, member, &out->canonical_tile_id, child
        ))
      return false;
    out->fields |= MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID;
  }
  if (!mln_ts_member(call, value, "availableImages", &member)) return false;
  mln_ts_path(child, sizeof child, path, "availableImages");
  if (!mln_ts_is_absent(call, member)) {
    {
      mln_buffer_view* items = NULL;
      size_t count = 0;
      if (!mln_ts_write_list_buffer_view(
            call, member, UINT32_MAX, &items, &count, child
          ))
        return false;
      out->available_images = items;
      out->available_image_count = (size_t)count;
    }
  }
  return true;
}

bool mln_ts_write_style_expression_options(
  mln_ts_call* call, napi_value value, mln_style_expression_options* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "expectedType", &member)) return false;
  mln_ts_path(child, sizeof child, path, "expectedType");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_style_expression_type(
          call, member, &out->expected_type, child
        ))
      return false;
    out->fields |= MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE;
  }
  return true;
}

bool mln_ts_write_style_expression_type(
  mln_ts_call* call, napi_value value, mln_style_expression_type* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "kind", &member)) return false;
  mln_ts_path(child, sizeof child, path, "kind");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->kind = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "itemKind", &member)) return false;
  mln_ts_path(child, sizeof child, path, "itemKind");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->item_kind = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "itemCount", &member)) return false;
  mln_ts_path(child, sizeof child, path, "itemCount");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->item_count = (uint32_t)number;
    }
  }
  return true;
}

bool mln_ts_write_style_image_options(
  mln_ts_call* call, napi_value value, mln_style_image_options* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "stretchX", &member)) return false;
  mln_ts_path(child, sizeof child, path, "stretchX");
  if (!mln_ts_is_absent(call, member)) {
    {
      mln_image_stretch* items = NULL;
      size_t count = 0;
      if (!mln_ts_write_list_image_stretch(
            call, member, UINT32_MAX, &items, &count, child
          ))
        return false;
      out->stretch_x = items;
      out->stretch_x_count = (size_t)count;
    }
    out->fields |= MLN_STYLE_IMAGE_OPTION_STRETCH_X;
  }
  if (!mln_ts_member(call, value, "stretchY", &member)) return false;
  mln_ts_path(child, sizeof child, path, "stretchY");
  if (!mln_ts_is_absent(call, member)) {
    {
      mln_image_stretch* items = NULL;
      size_t count = 0;
      if (!mln_ts_write_list_image_stretch(
            call, member, UINT32_MAX, &items, &count, child
          ))
        return false;
      out->stretch_y = items;
      out->stretch_y_count = (size_t)count;
    }
    out->fields |= MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
  }
  if (!mln_ts_member(call, value, "content", &member)) return false;
  mln_ts_path(child, sizeof child, path, "content");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_image_content(call, member, &out->content, child))
      return false;
    out->fields |= MLN_STYLE_IMAGE_OPTION_CONTENT;
  }
  if (!mln_ts_member(call, value, "textFitWidth", &member)) return false;
  mln_ts_path(child, sizeof child, path, "textFitWidth");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->text_fit_width = (uint32_t)number;
    }
    out->fields |= MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH;
  }
  if (!mln_ts_member(call, value, "textFitHeight", &member)) return false;
  mln_ts_path(child, sizeof child, path, "textFitHeight");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->text_fit_height = (uint32_t)number;
    }
    out->fields |= MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
  }
  if (!mln_ts_member(call, value, "pixelRatio", &member)) return false;
  mln_ts_path(child, sizeof child, path, "pixelRatio");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->pixel_ratio = (float)number;
    }
    out->fields |= MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
  }
  if (!mln_ts_member(call, value, "sdf", &member)) return false;
  mln_ts_path(child, sizeof child, path, "sdf");
  if (!mln_ts_is_absent(call, member)) {
    {
      bool flag = false;
      if (!mln_ts_read_bool(call, member, &flag, child)) return false;
      out->sdf = flag;
    }
    out->fields |= MLN_STYLE_IMAGE_OPTION_SDF;
  }
  return true;
}

bool mln_ts_write_style_transition_options(
  mln_ts_call* call, napi_value value, mln_style_transition_options* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "durationMs", &member)) return false;
  mln_ts_path(child, sizeof child, path, "durationMs");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->duration_ms = (double)number;
    }
    out->fields |= MLN_STYLE_TRANSITION_OPTION_DURATION;
  }
  if (!mln_ts_member(call, value, "delayMs", &member)) return false;
  mln_ts_path(child, sizeof child, path, "delayMs");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->delay_ms = (double)number;
    }
    out->fields |= MLN_STYLE_TRANSITION_OPTION_DELAY;
  }
  if (!mln_ts_member(call, value, "enablePlacementTransitions", &member))
    return false;
  mln_ts_path(child, sizeof child, path, "enablePlacementTransitions");
  if (!mln_ts_is_absent(call, member)) {
    {
      bool flag = false;
      if (!mln_ts_read_bool(call, member, &flag, child)) return false;
      out->enable_placement_transitions = flag;
    }
    out->fields |= MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
  }
  return true;
}

bool mln_ts_write_unit_bezier(
  mln_ts_call* call, napi_value value, mln_unit_bezier* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "x1", &member)) return false;
  mln_ts_path(child, sizeof child, path, "x1");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->x1 = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "y1", &member)) return false;
  mln_ts_path(child, sizeof child, path, "y1");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->y1 = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "x2", &member)) return false;
  mln_ts_path(child, sizeof child, path, "x2");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->x2 = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "y2", &member)) return false;
  mln_ts_path(child, sizeof child, path, "y2");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->y2 = (double)number;
    }
  }
  return true;
}

bool mln_ts_write_vec3(
  mln_ts_call* call, napi_value value, mln_vec3* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "x", &member)) return false;
  mln_ts_path(child, sizeof child, path, "x");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->x = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "y", &member)) return false;
  mln_ts_path(child, sizeof child, path, "y");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->y = (double)number;
    }
  }
  if (!mln_ts_member(call, value, "z", &member)) return false;
  mln_ts_path(child, sizeof child, path, "z");
  if (!mln_ts_is_absent(call, member)) {
    {
      double number = 0;
      if (!mln_ts_read_double(call, member, &number, child)) return false;
      out->z = (double)number;
    }
  }
  return true;
}

bool mln_ts_write_vulkan_borrowed_texture(
  mln_ts_call* call, napi_value value, mln_vulkan_borrowed_texture* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "image", &member)) return false;
  mln_ts_path(child, sizeof child, path, "image");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->image = (uint64_t)number;
    }
  }
  if (!mln_ts_member(call, value, "imageView", &member)) return false;
  mln_ts_path(child, sizeof child, path, "imageView");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->image_view = (uint64_t)number;
    }
  }
  return true;
}

bool mln_ts_write_vulkan_borrowed_texture_descriptor(
  mln_ts_call* call, napi_value value,
  mln_vulkan_borrowed_texture_descriptor* out, const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "extent", &member)) return false;
  mln_ts_path(child, sizeof child, path, "extent");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_logical_extent(call, member, &out->extent, child))
      return false;
  }
  if (!mln_ts_member(call, value, "physicalWidth", &member)) return false;
  mln_ts_path(child, sizeof child, path, "physicalWidth");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->physical_width = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "physicalHeight", &member)) return false;
  mln_ts_path(child, sizeof child, path, "physicalHeight");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->physical_height = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "context", &member)) return false;
  mln_ts_path(child, sizeof child, path, "context");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_vulkan_context_descriptor(
          call, member, &out->context, child
        ))
      return false;
  }
  if (!mln_ts_member(call, value, "textures", &member)) return false;
  mln_ts_path(child, sizeof child, path, "textures");
  if (!mln_ts_is_absent(call, member)) {
    {
      mln_vulkan_borrowed_texture* items = NULL;
      size_t count = 0;
      if (!mln_ts_write_list_vulkan_borrowed_texture(
            call, member, UINT32_MAX, &items, &count, child
          ))
        return false;
      out->textures = items;
      out->texture_count = (size_t)count;
    }
  }
  if (!mln_ts_member(call, value, "format", &member)) return false;
  mln_ts_path(child, sizeof child, path, "format");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->format = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "initialLayout", &member)) return false;
  mln_ts_path(child, sizeof child, path, "initialLayout");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->initial_layout = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "finalLayout", &member)) return false;
  mln_ts_path(child, sizeof child, path, "finalLayout");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->final_layout = (uint32_t)number;
    }
  }
  return true;
}

bool mln_ts_write_vulkan_context_descriptor(
  mln_ts_call* call, napi_value value, mln_vulkan_context_descriptor* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "instance", &member)) return false;
  mln_ts_path(child, sizeof child, path, "instance");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->instance = (void*)(uintptr_t)number;
    }
  }
  if (!mln_ts_member(call, value, "physicalDevice", &member)) return false;
  mln_ts_path(child, sizeof child, path, "physicalDevice");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->physical_device = (void*)(uintptr_t)number;
    }
  }
  if (!mln_ts_member(call, value, "device", &member)) return false;
  mln_ts_path(child, sizeof child, path, "device");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->device = (void*)(uintptr_t)number;
    }
  }
  if (!mln_ts_member(call, value, "graphicsQueue", &member)) return false;
  mln_ts_path(child, sizeof child, path, "graphicsQueue");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->graphics_queue = (void*)(uintptr_t)number;
    }
  }
  if (!mln_ts_member(call, value, "graphicsQueueFamilyIndex", &member))
    return false;
  mln_ts_path(child, sizeof child, path, "graphicsQueueFamilyIndex");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->graphics_queue_family_index = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "getInstanceProcAddr", &member)) return false;
  mln_ts_path(child, sizeof child, path, "getInstanceProcAddr");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->get_instance_proc_addr = (void*)(uintptr_t)number;
    }
  }
  if (!mln_ts_member(call, value, "getDeviceProcAddr", &member)) return false;
  mln_ts_path(child, sizeof child, path, "getDeviceProcAddr");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->get_device_proc_addr = (void*)(uintptr_t)number;
    }
  }
  return true;
}

bool mln_ts_write_vulkan_owned_texture_descriptor(
  mln_ts_call* call, napi_value value, mln_vulkan_owned_texture_descriptor* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "extent", &member)) return false;
  mln_ts_path(child, sizeof child, path, "extent");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_logical_extent(call, member, &out->extent, child))
      return false;
  }
  if (!mln_ts_member(call, value, "context", &member)) return false;
  mln_ts_path(child, sizeof child, path, "context");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_vulkan_context_descriptor(
          call, member, &out->context, child
        ))
      return false;
  }
  return true;
}

bool mln_ts_write_vulkan_surface_descriptor(
  mln_ts_call* call, napi_value value, mln_vulkan_surface_descriptor* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "extent", &member)) return false;
  mln_ts_path(child, sizeof child, path, "extent");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_logical_extent(call, member, &out->extent, child))
      return false;
  }
  if (!mln_ts_member(call, value, "context", &member)) return false;
  mln_ts_path(child, sizeof child, path, "context");
  if (!mln_ts_is_absent(call, member)) {
    if (!mln_ts_read_record(call, member, child)) return false;
    if (!mln_ts_write_vulkan_context_descriptor(
          call, member, &out->context, child
        ))
      return false;
  }
  if (!mln_ts_member(call, value, "surface", &member)) return false;
  mln_ts_path(child, sizeof child, path, "surface");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->surface = (uint64_t)number;
    }
  }
  return true;
}

bool mln_ts_write_webgl_context_descriptor(
  mln_ts_call* call, napi_value value, mln_webgl_context_descriptor* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "kind", &member)) return false;
  mln_ts_path(child, sizeof child, path, "kind");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(call, member, 0LL, 4294967295LL, &number, child))
        return false;
      out->kind = (uint32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "context", &member)) return false;
  mln_ts_path(child, sizeof child, path, "context");
  if (!mln_ts_is_absent(call, member)) {
    {
      int64_t number = 0;
      if (!mln_ts_read_int(
            call, member, -2147483648LL, 2147483647LL, &number, child
          ))
        return false;
      out->context = (int32_t)number;
    }
  }
  if (!mln_ts_member(call, value, "canvasSelector", &member)) return false;
  mln_ts_path(child, sizeof child, path, "canvasSelector");
  if (!mln_ts_read_text_view(call, member, &out->canvas_selector, child))
    return false;
  return true;
}

bool mln_ts_write_wgl_context_descriptor(
  mln_ts_call* call, napi_value value, mln_wgl_context_descriptor* out,
  const char* path
) {
  napi_value member;
  char child[256];
  (void)child;
  if (!mln_ts_member(call, value, "deviceContext", &member)) return false;
  mln_ts_path(child, sizeof child, path, "deviceContext");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->device_context = (void*)(uintptr_t)number;
    }
  }
  if (!mln_ts_member(call, value, "shareContext", &member)) return false;
  mln_ts_path(child, sizeof child, path, "shareContext");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->share_context = (void*)(uintptr_t)number;
    }
  }
  if (!mln_ts_member(call, value, "getProcAddress", &member)) return false;
  mln_ts_path(child, sizeof child, path, "getProcAddress");
  if (!mln_ts_is_absent(call, member)) {
    {
      uint64_t number = 0;
      if (!mln_ts_read_u64(call, member, &number, child)) return false;
      out->get_proc_address = (void*)(uintptr_t)number;
    }
  }
  return true;
}

const mln_ts_decoder mln_ts_decoders[] = {
  [MLN_TS_NO_DECODER] = NULL,
  [MLN_TS_DECODE_BUFFER_VIEW] = mln_ts_decode_buffer_view,
  [MLN_TS_DECODE_BUFFER_VIEW_OPTIONAL] = mln_ts_decode_buffer_view_optional,
  [MLN_TS_DECODE_CAMERA_OPTIONS] = mln_ts_decode_camera_options,
  [MLN_TS_DECODE_CAMERA_QUERY_RESULT] = mln_ts_decode_camera_query_result,
  [MLN_TS_DECODE_LAT_LNG_BOUNDS] = mln_ts_decode_lat_lng_bounds,
  [MLN_TS_DECODE_LAT_LNG_LIST] = mln_ts_decode_lat_lng_list,
  [MLN_TS_DECODE_OFFLINE_REGION_INFO] = mln_ts_decode_offline_region_info,
  [MLN_TS_DECODE_OFFLINE_REGION_INFO_LIST] =
    mln_ts_decode_offline_region_info_list,
  [MLN_TS_DECODE_OFFLINE_REGION_INFO_OPTIONAL] =
    mln_ts_decode_offline_region_info_optional,
  [MLN_TS_DECODE_OFFLINE_REGION_STATUS] = mln_ts_decode_offline_region_status,
  [MLN_TS_DECODE_RENDERED_FEATURE_LIST] = mln_ts_decode_rendered_feature_list,
  [MLN_TS_DECODE_SOURCE_FEATURE_LIST] = mln_ts_decode_source_feature_list,
  [MLN_TS_DECODE_STYLE_IMAGE_INFO_OPTIONAL] =
    mln_ts_decode_style_image_info_optional,
  [MLN_TS_DECODE_STYLE_LAYER_INFO_LIST] = mln_ts_decode_style_layer_info_list,
  [MLN_TS_DECODE_STYLE_LAYER_INFO_OPTIONAL] =
    mln_ts_decode_style_layer_info_optional,
  [MLN_TS_DECODE_STYLE_SOURCE_INFO_LIST] = mln_ts_decode_style_source_info_list,
  [MLN_TS_DECODE_STYLE_SOURCE_INFO_OPTIONAL] =
    mln_ts_decode_style_source_info_optional,
  [MLN_TS_DECODE_STYLE_TRANSITION_OPTIONS] =
    mln_ts_decode_style_transition_options,
  [MLN_TS_DECODE_TEXTURE_READBACK_RESULT] =
    mln_ts_decode_texture_readback_result,
};
const size_t mln_ts_decoder_count =
  sizeof mln_ts_decoders / sizeof mln_ts_decoders[0];
