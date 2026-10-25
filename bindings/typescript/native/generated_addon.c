// Generated from the C headers by tools/bindgen. Do not edit.
#include <string.h>

#include "generated_addon.h"

static napi_value mln_ts_export_mln_acquired_frame_discard(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_acquired_frame arg_frame = 0;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "frame")) goto done;
    arg_frame = (mln_acquired_frame)number;
  }
  if (!mln_ts_read_owner(call, call->argv[1])) goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_acquired_frame_discard(arg_frame, &diagnostic);
  if (status == MLN_STATUS_OK) mln_ts_consume(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_acquired_frame_get_metal_texture(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_acquired_frame arg_frame = 0;
  mln_metal_texture_frame arg_out_frame;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "frame")) goto done;
    arg_frame = (mln_acquired_frame)number;
  }
  memset(&arg_out_frame, 0, sizeof arg_out_frame);
  arg_out_frame.size = sizeof arg_out_frame;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_acquired_frame_get_metal_texture(
    arg_frame, &arg_out_frame, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_read_metal_texture_frame(
    env, &(arg_out_frame), sizeof(arg_out_frame)
  );
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_acquired_frame_get_opengl_texture(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_acquired_frame arg_frame = 0;
  mln_opengl_texture_frame arg_out_frame;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "frame")) goto done;
    arg_frame = (mln_acquired_frame)number;
  }
  memset(&arg_out_frame, 0, sizeof arg_out_frame);
  arg_out_frame.size = sizeof arg_out_frame;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_acquired_frame_get_opengl_texture(
    arg_frame, &arg_out_frame, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_read_opengl_texture_frame(
    env, &(arg_out_frame), sizeof(arg_out_frame)
  );
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_acquired_frame_get_producer_sync(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_acquired_frame arg_frame = 0;
  mln_gpu_sync arg_out_sync;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "frame")) goto done;
    arg_frame = (mln_acquired_frame)number;
  }
  arg_out_sync = mln_gpu_sync_default();
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_acquired_frame_get_producer_sync(arg_frame, &arg_out_sync, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result =
    mln_ts_read_gpu_sync(env, &(arg_out_sync), sizeof(arg_out_sync));
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_acquired_frame_get_result(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_acquired_frame arg_frame = 0;
  mln_render_frame_result arg_out_result;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "frame")) goto done;
    arg_frame = (mln_acquired_frame)number;
  }
  memset(&arg_out_result, 0, sizeof arg_out_result);
  arg_out_result.size = sizeof arg_out_result;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_acquired_frame_get_result(arg_frame, &arg_out_result, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_read_render_frame_result(
    env, &(arg_out_result), sizeof(arg_out_result)
  );
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_acquired_frame_get_vulkan_texture(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_acquired_frame arg_frame = 0;
  mln_vulkan_texture_frame arg_out_frame;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "frame")) goto done;
    arg_frame = (mln_acquired_frame)number;
  }
  memset(&arg_out_frame, 0, sizeof arg_out_frame);
  arg_out_frame.size = sizeof arg_out_frame;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_acquired_frame_get_vulkan_texture(
    arg_frame, &arg_out_frame, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_read_vulkan_texture_frame(
    env, &(arg_out_frame), sizeof(arg_out_frame)
  );
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_acquired_frame_release(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_acquired_frame arg_frame = 0;
  mln_gpu_sync arg_consumer_completion;
  const void* present_consumer_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "frame")) goto done;
    arg_frame = (mln_acquired_frame)number;
  }
  if (!mln_ts_read_owner(call, call->argv[1])) goto done;
  if (!mln_ts_is_absent(call, call->argv[2])) {
    arg_consumer_completion = mln_gpu_sync_default();
    if (!mln_ts_read_record(call, call->argv[2], "consumerCompletion"))
      goto done;
    if (!mln_ts_write_gpu_sync(
          call, call->argv[2], &arg_consumer_completion, "consumerCompletion"
        ))
      goto done;
    present_consumer_completion = &arg_consumer_completion;
  }
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_acquired_frame_release(
    arg_frame, present_consumer_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_consume(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_animation_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_animation_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_animation_options_default();
  call->result = mln_ts_read_animation_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_bound_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_bound_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_bound_options_default();
  call->result = mln_ts_read_bound_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_c_version(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  uint32_t result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_c_version();
  call->result = mln_ts_number(env, (double)(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_camera_delta_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_camera_delta result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_camera_delta_default();
  call->result = mln_ts_read_camera_delta(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_camera_fit_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_camera_fit_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_camera_fit_options_default();
  call->result = mln_ts_read_camera_fit_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_camera_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_camera_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_camera_options_default();
  call->result = mln_ts_read_camera_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_camera_update_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_camera_update result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_camera_update_default();
  call->result = mln_ts_read_camera_update(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_custom_geometry_source_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_custom_geometry_source_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_custom_geometry_source_options_default();
  call->result =
    mln_ts_read_custom_geometry_source_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_custom_mvt_vector_source_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_custom_mvt_vector_source_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_custom_mvt_vector_source_options_default();
  call->result = mln_ts_read_custom_mvt_vector_source_options(
    env, &(result), sizeof(result)
  );
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_frame_demand_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_frame_demand result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_frame_demand_default();
  call->result = mln_ts_read_frame_demand(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_free_camera_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_free_camera_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_free_camera_options_default();
  call->result =
    mln_ts_read_free_camera_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_geojson_source_data_create(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_buffer_view arg_data = {0};
  mln_geojson_source_options arg_options;
  const void* present_options = NULL;
  mln_geojson_source_data arg_out_data = MLN_HANDLE_NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  if (!mln_ts_read_json_view(call, call->argv[0], &arg_data, "data")) goto done;
  if (!mln_ts_is_absent(call, call->argv[1])) {
    arg_options = mln_geojson_source_options_default();
    if (!mln_ts_read_record(call, call->argv[1], "options")) goto done;
    if (!mln_ts_write_geojson_source_options(
          call, call->argv[1], &arg_options, "options"
        ))
      goto done;
    present_options = &arg_options;
  }
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_geojson_source_data_create(
    arg_data, present_options, &arg_out_data, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)arg_out_data);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_geojson_source_data_release(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_geojson_source_data arg_data = 0;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "data")) goto done;
    arg_data = (mln_geojson_source_data)number;
  }
  if (!mln_ts_read_owner(call, call->argv[1])) goto done;
  mln_geojson_source_data_release(arg_data);
  mln_ts_consume(call);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_geojson_source_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_geojson_source_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_geojson_source_options_default();
  call->result =
    mln_ts_read_geojson_source_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_gpu_sync_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_gpu_sync result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_gpu_sync_default();
  call->result = mln_ts_read_gpu_sync(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_http_header_rule_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_http_header_rule result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_http_header_rule_default();
  call->result = mln_ts_read_http_header_rule(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_lat_lng_for_projected_meters(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_projected_meters arg_meters;
  mln_lat_lng arg_out_coordinate;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  memset(&arg_meters, 0, sizeof arg_meters);
  if (!mln_ts_is_absent(call, call->argv[0])) {
    if (!mln_ts_read_record(call, call->argv[0], "meters")) goto done;
    if (!mln_ts_write_projected_meters(
          call, call->argv[0], &arg_meters, "meters"
        ))
      goto done;
  }
  memset(&arg_out_coordinate, 0, sizeof arg_out_coordinate);
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_lat_lng_for_projected_meters(
    arg_meters, &arg_out_coordinate, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result =
    mln_ts_read_lat_lng(env, &(arg_out_coordinate), sizeof(arg_out_coordinate));
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_log_clear_callback(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_log_clear_callback(&diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_log_set_async_severity_mask(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_log_severity_flags arg_mask;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  memset(&arg_mask, 0, sizeof arg_mask);
  {
    int64_t number = 0;
    if (!mln_ts_read_int(
          call, call->argv[0], 0LL, 4294967295LL, &number, "mask"
        ))
      goto done;
    arg_mask = (mln_log_severity_flags)number;
  }
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_log_set_async_severity_mask(arg_mask, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_log_set_callback(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_log_handler arg_handler;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  memset(&arg_handler, 0, sizeof arg_handler);
  arg_handler.size = sizeof arg_handler;
  if (!mln_ts_read_function(call, call->argv[0], "handler")) goto done;
  if (!mln_ts_read_registration(
        call, call->argv[0], &arg_handler.user_data, &arg_handler.queue,
        "handler"
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_log_set_callback(&arg_handler, &diagnostic);
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_logical_extent_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_logical_extent result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_logical_extent_default();
  call->result = mln_ts_read_logical_extent(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_logical_extent_physical_size(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_logical_extent arg_extent;
  uint32_t arg_out_width;
  uint32_t arg_out_height;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  arg_extent = mln_logical_extent_default();
  if (!mln_ts_is_absent(call, call->argv[0])) {
    if (!mln_ts_read_record(call, call->argv[0], "extent")) goto done;
    if (!mln_ts_write_logical_extent(
          call, call->argv[0], &arg_extent, "extent"
        ))
      goto done;
  }
  memset(&arg_out_width, 0, sizeof arg_out_width);
  memset(&arg_out_height, 0, sizeof arg_out_height);
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_logical_extent_physical_size(
    arg_extent, &arg_out_width, &arg_out_height, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_object(env);
  if (!mln_ts_set(
        env, call->result, "width", mln_ts_number(env, (double)(arg_out_width))
      ))
    goto done;
  if (!mln_ts_set(
        env, call->result, "height",
        mln_ts_number(env, (double)(arg_out_height))
      ))
    goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_add_custom_geometry_source(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_custom_geometry_source_options arg_options;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  arg_options = mln_custom_geometry_source_options_default();
  if (!mln_ts_is_absent(call, call->argv[2])) {
    if (!mln_ts_read_record(call, call->argv[2], "options")) goto done;
    if (!mln_ts_write_custom_geometry_source_options(
          call, call->argv[2], &arg_options, "options"
        ))
      goto done;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_add_custom_geometry_source(
    arg_map, arg_source_id, &arg_options, present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_add_custom_mvt_vector_source(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_custom_mvt_vector_source_options arg_options;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  arg_options = mln_custom_mvt_vector_source_options_default();
  if (!mln_ts_is_absent(call, call->argv[2])) {
    if (!mln_ts_read_record(call, call->argv[2], "options")) goto done;
    if (!mln_ts_write_custom_mvt_vector_source_options(
          call, call->argv[2], &arg_options, "options"
        ))
      goto done;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_add_custom_mvt_vector_source(
    arg_map, arg_source_id, &arg_options, present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_add_geojson_source_data(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  uint64_t arg_data = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  if (!mln_ts_read_u64(call, call->argv[2], &arg_data, "data")) goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_add_geojson_source_data(
    arg_map, arg_source_id, arg_data, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_add_geojson_source_url(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_buffer_view arg_url = {0};
  mln_geojson_source_options arg_options;
  const void* present_options = NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 5)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  if (!mln_ts_read_text_view(call, call->argv[2], &arg_url, "url")) goto done;
  if (!mln_ts_is_absent(call, call->argv[3])) {
    arg_options = mln_geojson_source_options_default();
    if (!mln_ts_read_record(call, call->argv[3], "options")) goto done;
    if (!mln_ts_write_geojson_source_options(
          call, call->argv[3], &arg_options, "options"
        ))
      goto done;
    present_options = &arg_options;
  }
  if (!mln_ts_read_completion(
        call, call->argv[4], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_add_geojson_source_url(
    arg_map, arg_source_id, arg_url, present_options, present_completion,
    &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_add_image_source_image(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  size_t count_coordinates = 0;
  mln_lat_lng* arg_coordinates = NULL;
  mln_premultiplied_rgba8_image arg_image;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 5)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  {
    mln_lat_lng* items = NULL;
    size_t count = 0;
    if (!mln_ts_write_list_lat_lng(
          call, call->argv[2], UINT32_MAX, &items, &count, "coordinates"
        ))
      goto done;
    arg_coordinates = items;
    count_coordinates = (size_t)count;
  }
  arg_image = mln_premultiplied_rgba8_image_default();
  if (!mln_ts_read_record(call, call->argv[3], "image")) goto done;
  if (!mln_ts_write_premultiplied_rgba8_image(
        call, call->argv[3], &arg_image, "image"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[4], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_add_image_source_image(
    arg_map, arg_source_id, arg_coordinates, (size_t)count_coordinates,
    &arg_image, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_add_image_source_url(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  size_t count_coordinates = 0;
  mln_lat_lng* arg_coordinates = NULL;
  mln_buffer_view arg_url = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 5)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  {
    mln_lat_lng* items = NULL;
    size_t count = 0;
    if (!mln_ts_write_list_lat_lng(
          call, call->argv[2], UINT32_MAX, &items, &count, "coordinates"
        ))
      goto done;
    arg_coordinates = items;
    count_coordinates = (size_t)count;
  }
  if (!mln_ts_read_text_view(call, call->argv[3], &arg_url, "url")) goto done;
  if (!mln_ts_read_completion(
        call, call->argv[4], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_add_image_source_url(
    arg_map, arg_source_id, arg_coordinates, (size_t)count_coordinates, arg_url,
    present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_add_style_layer_json(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_layer_json = {0};
  mln_buffer_view arg_before_layer_id = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_json_view(call, call->argv[1], &arg_layer_json, "layerJson"))
    goto done;
  if (!mln_ts_read_text_view(
        call, call->argv[2], &arg_before_layer_id, "beforeLayerId"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_add_style_layer_json(
    arg_map, arg_layer_json, arg_before_layer_id, present_completion,
    &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_add_style_source_json(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_buffer_view arg_source_json = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  if (!mln_ts_read_json_view(
        call, call->argv[2], &arg_source_json, "sourceJson"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_add_style_source_json(
    arg_map, arg_source_id, arg_source_json, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_apply_camera_delta(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_camera_delta arg_delta;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_delta = mln_camera_delta_default();
  if (!mln_ts_read_record(call, call->argv[1], "delta")) goto done;
  if (!mln_ts_write_camera_delta(call, call->argv[1], &arg_delta, "delta"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_apply_camera_delta(
    arg_map, &arg_delta, present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_attach_metal_borrowed_texture(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_metal_borrowed_texture_descriptor arg_descriptor;
  mln_render_session_attach_options arg_options;
  mln_render_session arg_out_session = MLN_HANDLE_NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_descriptor = mln_metal_borrowed_texture_descriptor_default();
  if (!mln_ts_read_record(call, call->argv[1], "descriptor")) goto done;
  if (!mln_ts_write_metal_borrowed_texture_descriptor(
        call, call->argv[1], &arg_descriptor, "descriptor"
      ))
    goto done;
  arg_options = mln_render_session_attach_options_default();
  if (!mln_ts_read_record(call, call->argv[2], "options")) goto done;
  if (!mln_ts_write_render_session_attach_options(
        call, call->argv[2], &arg_options, "options"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_attach_metal_borrowed_texture(
    arg_map, &arg_descriptor, &arg_options, &arg_out_session,
    present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)arg_out_session);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_attach_metal_owned_texture(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_metal_owned_texture_descriptor arg_descriptor;
  mln_render_session_attach_options arg_options;
  mln_render_session arg_out_session = MLN_HANDLE_NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_descriptor = mln_metal_owned_texture_descriptor_default();
  if (!mln_ts_read_record(call, call->argv[1], "descriptor")) goto done;
  if (!mln_ts_write_metal_owned_texture_descriptor(
        call, call->argv[1], &arg_descriptor, "descriptor"
      ))
    goto done;
  arg_options = mln_render_session_attach_options_default();
  if (!mln_ts_read_record(call, call->argv[2], "options")) goto done;
  if (!mln_ts_write_render_session_attach_options(
        call, call->argv[2], &arg_options, "options"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_attach_metal_owned_texture(
    arg_map, &arg_descriptor, &arg_options, &arg_out_session,
    present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)arg_out_session);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_attach_metal_surface(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_metal_surface_descriptor arg_descriptor;
  mln_render_session_attach_options arg_options;
  mln_render_session arg_out_session = MLN_HANDLE_NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_descriptor = mln_metal_surface_descriptor_default();
  if (!mln_ts_read_record(call, call->argv[1], "descriptor")) goto done;
  if (!mln_ts_write_metal_surface_descriptor(
        call, call->argv[1], &arg_descriptor, "descriptor"
      ))
    goto done;
  arg_options = mln_render_session_attach_options_default();
  if (!mln_ts_read_record(call, call->argv[2], "options")) goto done;
  if (!mln_ts_write_render_session_attach_options(
        call, call->argv[2], &arg_options, "options"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_attach_metal_surface(
    arg_map, &arg_descriptor, &arg_options, &arg_out_session,
    present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)arg_out_session);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_attach_opengl_borrowed_texture(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_opengl_borrowed_texture_descriptor arg_descriptor;
  mln_render_session_attach_options arg_options;
  mln_render_session arg_out_session = MLN_HANDLE_NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_descriptor = mln_opengl_borrowed_texture_descriptor_default();
  if (!mln_ts_read_record(call, call->argv[1], "descriptor")) goto done;
  if (!mln_ts_write_opengl_borrowed_texture_descriptor(
        call, call->argv[1], &arg_descriptor, "descriptor"
      ))
    goto done;
  arg_options = mln_render_session_attach_options_default();
  if (!mln_ts_read_record(call, call->argv[2], "options")) goto done;
  if (!mln_ts_write_render_session_attach_options(
        call, call->argv[2], &arg_options, "options"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_attach_opengl_borrowed_texture(
    arg_map, &arg_descriptor, &arg_options, &arg_out_session,
    present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)arg_out_session);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_attach_opengl_owned_texture(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_opengl_owned_texture_descriptor arg_descriptor;
  mln_render_session_attach_options arg_options;
  mln_render_session arg_out_session = MLN_HANDLE_NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_descriptor = mln_opengl_owned_texture_descriptor_default();
  if (!mln_ts_read_record(call, call->argv[1], "descriptor")) goto done;
  if (!mln_ts_write_opengl_owned_texture_descriptor(
        call, call->argv[1], &arg_descriptor, "descriptor"
      ))
    goto done;
  arg_options = mln_render_session_attach_options_default();
  if (!mln_ts_read_record(call, call->argv[2], "options")) goto done;
  if (!mln_ts_write_render_session_attach_options(
        call, call->argv[2], &arg_options, "options"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_attach_opengl_owned_texture(
    arg_map, &arg_descriptor, &arg_options, &arg_out_session,
    present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)arg_out_session);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_attach_opengl_surface(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_opengl_surface_descriptor arg_descriptor;
  mln_render_session_attach_options arg_options;
  mln_render_session arg_out_session = MLN_HANDLE_NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_descriptor = mln_opengl_surface_descriptor_default();
  if (!mln_ts_read_record(call, call->argv[1], "descriptor")) goto done;
  if (!mln_ts_write_opengl_surface_descriptor(
        call, call->argv[1], &arg_descriptor, "descriptor"
      ))
    goto done;
  arg_options = mln_render_session_attach_options_default();
  if (!mln_ts_read_record(call, call->argv[2], "options")) goto done;
  if (!mln_ts_write_render_session_attach_options(
        call, call->argv[2], &arg_options, "options"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_attach_opengl_surface(
    arg_map, &arg_descriptor, &arg_options, &arg_out_session,
    present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)arg_out_session);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_attach_vulkan_borrowed_texture(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_vulkan_borrowed_texture_descriptor arg_descriptor;
  mln_render_session_attach_options arg_options;
  mln_render_session arg_out_session = MLN_HANDLE_NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_descriptor = mln_vulkan_borrowed_texture_descriptor_default();
  if (!mln_ts_read_record(call, call->argv[1], "descriptor")) goto done;
  if (!mln_ts_write_vulkan_borrowed_texture_descriptor(
        call, call->argv[1], &arg_descriptor, "descriptor"
      ))
    goto done;
  arg_options = mln_render_session_attach_options_default();
  if (!mln_ts_read_record(call, call->argv[2], "options")) goto done;
  if (!mln_ts_write_render_session_attach_options(
        call, call->argv[2], &arg_options, "options"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_attach_vulkan_borrowed_texture(
    arg_map, &arg_descriptor, &arg_options, &arg_out_session,
    present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)arg_out_session);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_attach_vulkan_owned_texture(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_vulkan_owned_texture_descriptor arg_descriptor;
  mln_render_session_attach_options arg_options;
  mln_render_session arg_out_session = MLN_HANDLE_NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_descriptor = mln_vulkan_owned_texture_descriptor_default();
  if (!mln_ts_read_record(call, call->argv[1], "descriptor")) goto done;
  if (!mln_ts_write_vulkan_owned_texture_descriptor(
        call, call->argv[1], &arg_descriptor, "descriptor"
      ))
    goto done;
  arg_options = mln_render_session_attach_options_default();
  if (!mln_ts_read_record(call, call->argv[2], "options")) goto done;
  if (!mln_ts_write_render_session_attach_options(
        call, call->argv[2], &arg_options, "options"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_attach_vulkan_owned_texture(
    arg_map, &arg_descriptor, &arg_options, &arg_out_session,
    present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)arg_out_session);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_attach_vulkan_surface(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_vulkan_surface_descriptor arg_descriptor;
  mln_render_session_attach_options arg_options;
  mln_render_session arg_out_session = MLN_HANDLE_NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_descriptor = mln_vulkan_surface_descriptor_default();
  if (!mln_ts_read_record(call, call->argv[1], "descriptor")) goto done;
  if (!mln_ts_write_vulkan_surface_descriptor(
        call, call->argv[1], &arg_descriptor, "descriptor"
      ))
    goto done;
  arg_options = mln_render_session_attach_options_default();
  if (!mln_ts_read_record(call, call->argv[2], "options")) goto done;
  if (!mln_ts_write_render_session_attach_options(
        call, call->argv[2], &arg_options, "options"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_attach_vulkan_surface(
    arg_map, &arg_descriptor, &arg_options, &arg_out_session,
    present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)arg_out_session);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_begin_command_group(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_map_begin_command_group(arg_map, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_camera_for_geometry(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_geometry = {0};
  mln_camera_fit_options arg_fit_options;
  const void* present_fit_options = NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_json_view(call, call->argv[1], &arg_geometry, "geometry"))
    goto done;
  if (!mln_ts_is_absent(call, call->argv[2])) {
    arg_fit_options = mln_camera_fit_options_default();
    if (!mln_ts_read_record(call, call->argv[2], "fitOptions")) goto done;
    if (!mln_ts_write_camera_fit_options(
          call, call->argv[2], &arg_fit_options, "fitOptions"
        ))
      goto done;
    present_fit_options = &arg_fit_options;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_DECODE_CAMERA_OPTIONS,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_camera_for_geometry(
    arg_map, arg_geometry, present_fit_options, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_camera_for_lat_lng_bounds(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_lat_lng_bounds arg_bounds;
  mln_camera_fit_options arg_fit_options;
  const void* present_fit_options = NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  memset(&arg_bounds, 0, sizeof arg_bounds);
  if (!mln_ts_is_absent(call, call->argv[1])) {
    if (!mln_ts_read_record(call, call->argv[1], "bounds")) goto done;
    if (!mln_ts_write_lat_lng_bounds(
          call, call->argv[1], &arg_bounds, "bounds"
        ))
      goto done;
  }
  if (!mln_ts_is_absent(call, call->argv[2])) {
    arg_fit_options = mln_camera_fit_options_default();
    if (!mln_ts_read_record(call, call->argv[2], "fitOptions")) goto done;
    if (!mln_ts_write_camera_fit_options(
          call, call->argv[2], &arg_fit_options, "fitOptions"
        ))
      goto done;
    present_fit_options = &arg_fit_options;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_DECODE_CAMERA_OPTIONS,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_camera_for_lat_lng_bounds(
    arg_map, arg_bounds, present_fit_options, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_camera_for_lat_lngs(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  size_t count_coordinates = 0;
  mln_lat_lng* arg_coordinates = NULL;
  mln_camera_fit_options arg_fit_options;
  const void* present_fit_options = NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  {
    mln_lat_lng* items = NULL;
    size_t count = 0;
    if (!mln_ts_write_list_lat_lng(
          call, call->argv[1], UINT32_MAX, &items, &count, "coordinates"
        ))
      goto done;
    arg_coordinates = items;
    count_coordinates = (size_t)count;
  }
  if (!mln_ts_is_absent(call, call->argv[2])) {
    arg_fit_options = mln_camera_fit_options_default();
    if (!mln_ts_read_record(call, call->argv[2], "fitOptions")) goto done;
    if (!mln_ts_write_camera_fit_options(
          call, call->argv[2], &arg_fit_options, "fitOptions"
        ))
      goto done;
    present_fit_options = &arg_fit_options;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_DECODE_CAMERA_OPTIONS,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_camera_for_lat_lngs(
    arg_map, arg_coordinates, (size_t)count_coordinates, present_fit_options,
    present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_cancel_camera_transition(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  uint64_t arg_transition_id;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  memset(&arg_transition_id, 0, sizeof arg_transition_id);
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[1], &number, "transitionId"))
      goto done;
    arg_transition_id = (uint64_t)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_cancel_camera_transition(
    arg_map, arg_transition_id, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_cancel_transitions(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_cancel_transitions(arg_map, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_dispose(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_owner(call, call->argv[1])) goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_dispose(arg_map, &diagnostic);
  if (status == MLN_STATUS_OK) mln_ts_consume(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_dump_debug_logs(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_dump_debug_logs(arg_map, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_end_command_group(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_end_command_group(arg_map, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_get_camera(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], false, MLN_TS_DECODE_CAMERA_QUERY_RESULT,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_get_camera(arg_map, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_get_feature_state(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_feature_state_selector arg_selector;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  memset(&arg_selector, 0, sizeof arg_selector);
  arg_selector.size = sizeof arg_selector;
  if (!mln_ts_read_record(call, call->argv[1], "selector")) goto done;
  if (!mln_ts_write_feature_state_selector(
        call, call->argv[1], &arg_selector, "selector"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_DECODE_BUFFER_VIEW_JSON,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_get_feature_state(
    arg_map, &arg_selector, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_get_global_state(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], false, MLN_TS_DECODE_BUFFER_VIEW_JSON,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_get_global_state(arg_map, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_get_image_source_coordinates(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_DECODE_LAT_LNG_LIST, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_get_image_source_coordinates(
    arg_map, arg_source_id, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_get_loaded_style_json(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], false, MLN_TS_DECODE_BUFFER_VIEW_JSON,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_map_get_loaded_style_json(arg_map, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_get_snapshot(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_map_snapshot arg_out_snapshot;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  memset(&arg_out_snapshot, 0, sizeof arg_out_snapshot);
  arg_out_snapshot.size = sizeof arg_out_snapshot;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_get_snapshot(arg_map, &arg_out_snapshot, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_read_map_snapshot(
    env, &(arg_out_snapshot), sizeof(arg_out_snapshot)
  );
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_get_style_image(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_image_id = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_image_id, "imageId"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_DECODE_STYLE_IMAGE_INFO_OPTIONAL,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_get_style_image(
    arg_map, arg_image_id, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_get_style_layer(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_layer_id = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_layer_id, "layerId"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_DECODE_STYLE_LAYER_INFO_OPTIONAL,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_get_style_layer(
    arg_map, arg_layer_id, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_get_style_layer_json(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_layer_id = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_layer_id, "layerId"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_DECODE_BUFFER_VIEW_JSON_OPTIONAL,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_get_style_layer_json(
    arg_map, arg_layer_id, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_get_style_layer_property(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_layer_id = {0};
  mln_buffer_view arg_property_name = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_layer_id, "layerId"))
    goto done;
  if (!mln_ts_read_text_view(
        call, call->argv[2], &arg_property_name, "propertyName"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_DECODE_BUFFER_VIEW_JSON_OPTIONAL,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_get_style_layer_property(
    arg_map, arg_layer_id, arg_property_name, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_get_style_light_property(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_property_name = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(
        call, call->argv[1], &arg_property_name, "propertyName"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_DECODE_BUFFER_VIEW_JSON_OPTIONAL,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_get_style_light_property(
    arg_map, arg_property_name, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_get_style_source(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_DECODE_STYLE_SOURCE_INFO_OPTIONAL,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_get_style_source(
    arg_map, arg_source_id, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_get_style_transition_options(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], false, MLN_TS_DECODE_STYLE_TRANSITION_OPTIONS,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_get_style_transition_options(
    arg_map, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_get_style_url(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], false, MLN_TS_DECODE_BUFFER_VIEW, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_get_style_url(arg_map, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value
mln_ts_export_mln_map_invalidate_custom_geometry_source_region(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_lat_lng_bounds arg_bounds;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  memset(&arg_bounds, 0, sizeof arg_bounds);
  if (!mln_ts_is_absent(call, call->argv[2])) {
    if (!mln_ts_read_record(call, call->argv[2], "bounds")) goto done;
    if (!mln_ts_write_lat_lng_bounds(
          call, call->argv[2], &arg_bounds, "bounds"
        ))
      goto done;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_invalidate_custom_geometry_source_region(
    arg_map, arg_source_id, arg_bounds, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_invalidate_custom_geometry_source_tile(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_canonical_tile_id arg_tile_id;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  memset(&arg_tile_id, 0, sizeof arg_tile_id);
  if (!mln_ts_is_absent(call, call->argv[2])) {
    if (!mln_ts_read_record(call, call->argv[2], "tileId")) goto done;
    if (!mln_ts_write_canonical_tile_id(
          call, call->argv[2], &arg_tile_id, "tileId"
        ))
      goto done;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_invalidate_custom_geometry_source_tile(
    arg_map, arg_source_id, arg_tile_id, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value
mln_ts_export_mln_map_invalidate_custom_mvt_vector_source_tile(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_canonical_tile_id arg_tile_id;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  memset(&arg_tile_id, 0, sizeof arg_tile_id);
  if (!mln_ts_is_absent(call, call->argv[2])) {
    if (!mln_ts_read_record(call, call->argv[2], "tileId")) goto done;
    if (!mln_ts_write_canonical_tile_id(
          call, call->argv[2], &arg_tile_id, "tileId"
        ))
      goto done;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_invalidate_custom_mvt_vector_source_tile(
    arg_map, arg_source_id, arg_tile_id, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_lat_lng_bounds_for_camera(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_camera_options arg_camera;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_camera = mln_camera_options_default();
  if (!mln_ts_read_record(call, call->argv[1], "camera")) goto done;
  if (!mln_ts_write_camera_options(call, call->argv[1], &arg_camera, "camera"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_DECODE_LAT_LNG_BOUNDS,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_lat_lng_bounds_for_camera(
    arg_map, &arg_camera, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_lat_lng_bounds_for_camera_unwrapped(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_camera_options arg_camera;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_camera = mln_camera_options_default();
  if (!mln_ts_read_record(call, call->argv[1], "camera")) goto done;
  if (!mln_ts_write_camera_options(call, call->argv[1], &arg_camera, "camera"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_DECODE_LAT_LNG_BOUNDS,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_lat_lng_bounds_for_camera_unwrapped(
    arg_map, &arg_camera, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_list_style_layers(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], false, MLN_TS_DECODE_STYLE_LAYER_INFO_LIST,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_list_style_layers(arg_map, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_list_style_sources(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], false, MLN_TS_DECODE_STYLE_SOURCE_INFO_LIST,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_list_style_sources(arg_map, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_move_style_layer(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_layer_id = {0};
  mln_buffer_view arg_before_layer_id = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_layer_id, "layerId"))
    goto done;
  if (!mln_ts_read_text_view(
        call, call->argv[2], &arg_before_layer_id, "beforeLayerId"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_move_style_layer(
    arg_map, arg_layer_id, arg_before_layer_id, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_map_options_default();
  call->result = mln_ts_read_map_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_projection_create(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map_view arg_view;
  mln_map_projection arg_out_projection = MLN_HANDLE_NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  memset(&arg_view, 0, sizeof arg_view);
  arg_view.size = sizeof arg_view;
  if (!mln_ts_read_record(call, call->argv[0], "view")) goto done;
  if (!mln_ts_write_map_view(call, call->argv[0], &arg_view, "view")) goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_map_projection_create(&arg_view, &arg_out_projection, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)arg_out_projection);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_projection_get_camera(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map_projection arg_projection = 0;
  mln_camera_options arg_out_camera;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "projection")) goto done;
    arg_projection = (mln_map_projection)number;
  }
  arg_out_camera = mln_camera_options_default();
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_map_projection_get_camera(arg_projection, &arg_out_camera, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result =
    mln_ts_read_camera_options(env, &(arg_out_camera), sizeof(arg_out_camera));
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_projection_lat_lng_for_pixel(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map_projection arg_projection = 0;
  mln_screen_point arg_point;
  mln_lat_lng arg_out_coordinate;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "projection")) goto done;
    arg_projection = (mln_map_projection)number;
  }
  memset(&arg_point, 0, sizeof arg_point);
  if (!mln_ts_is_absent(call, call->argv[1])) {
    if (!mln_ts_read_record(call, call->argv[1], "point")) goto done;
    if (!mln_ts_write_screen_point(call, call->argv[1], &arg_point, "point"))
      goto done;
  }
  memset(&arg_out_coordinate, 0, sizeof arg_out_coordinate);
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_projection_lat_lng_for_pixel(
    arg_projection, arg_point, &arg_out_coordinate, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result =
    mln_ts_read_lat_lng(env, &(arg_out_coordinate), sizeof(arg_out_coordinate));
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_projection_lat_lng_for_pixel_unwrapped(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map_projection arg_projection = 0;
  mln_screen_point arg_point;
  mln_lat_lng arg_out_coordinate;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "projection")) goto done;
    arg_projection = (mln_map_projection)number;
  }
  memset(&arg_point, 0, sizeof arg_point);
  if (!mln_ts_is_absent(call, call->argv[1])) {
    if (!mln_ts_read_record(call, call->argv[1], "point")) goto done;
    if (!mln_ts_write_screen_point(call, call->argv[1], &arg_point, "point"))
      goto done;
  }
  memset(&arg_out_coordinate, 0, sizeof arg_out_coordinate);
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_projection_lat_lng_for_pixel_unwrapped(
    arg_projection, arg_point, &arg_out_coordinate, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result =
    mln_ts_read_lat_lng(env, &(arg_out_coordinate), sizeof(arg_out_coordinate));
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_projection_lat_lngs_for_pixels(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map_projection arg_projection = 0;
  size_t count_points = 0;
  mln_screen_point* arg_points = NULL;
  mln_lat_lng* arg_out_coordinates = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "projection")) goto done;
    arg_projection = (mln_map_projection)number;
  }
  {
    mln_screen_point* items = NULL;
    size_t count = 0;
    if (!mln_ts_write_list_screen_point(
          call, call->argv[1], UINT32_MAX, &items, &count, "points"
        ))
      goto done;
    arg_points = items;
    count_points = (size_t)count;
  }
  arg_out_coordinates =
    mln_ts_allocate(call, sizeof *arg_out_coordinates * count_points);
  if (arg_out_coordinates == NULL) goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_projection_lat_lngs_for_pixels(
    arg_projection, arg_points, (size_t)count_points, arg_out_coordinates,
    &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result =
    mln_ts_read_items_lat_lng(env, arg_out_coordinates, count_points);
done:
  return mln_ts_end(call);
}

static napi_value
mln_ts_export_mln_map_projection_lat_lngs_for_pixels_unwrapped(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map_projection arg_projection = 0;
  size_t count_points = 0;
  mln_screen_point* arg_points = NULL;
  mln_lat_lng* arg_out_coordinates = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "projection")) goto done;
    arg_projection = (mln_map_projection)number;
  }
  {
    mln_screen_point* items = NULL;
    size_t count = 0;
    if (!mln_ts_write_list_screen_point(
          call, call->argv[1], UINT32_MAX, &items, &count, "points"
        ))
      goto done;
    arg_points = items;
    count_points = (size_t)count;
  }
  arg_out_coordinates =
    mln_ts_allocate(call, sizeof *arg_out_coordinates * count_points);
  if (arg_out_coordinates == NULL) goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_projection_lat_lngs_for_pixels_unwrapped(
    arg_projection, arg_points, (size_t)count_points, arg_out_coordinates,
    &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result =
    mln_ts_read_items_lat_lng(env, arg_out_coordinates, count_points);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_projection_meters_per_pixel_at_latitude(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map_projection arg_projection = 0;
  double arg_latitude;
  double arg_out_meters_per_pixel;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "projection")) goto done;
    arg_projection = (mln_map_projection)number;
  }
  memset(&arg_latitude, 0, sizeof arg_latitude);
  {
    double number = 0;
    if (!mln_ts_read_double(call, call->argv[1], &number, "latitude"))
      goto done;
    arg_latitude = (double)number;
  }
  memset(&arg_out_meters_per_pixel, 0, sizeof arg_out_meters_per_pixel);
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_projection_meters_per_pixel_at_latitude(
    arg_projection, arg_latitude, &arg_out_meters_per_pixel, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_number(env, (double)(arg_out_meters_per_pixel));
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_projection_pixel_for_lat_lng(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map_projection arg_projection = 0;
  mln_lat_lng arg_coordinate;
  mln_screen_point arg_out_point;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "projection")) goto done;
    arg_projection = (mln_map_projection)number;
  }
  memset(&arg_coordinate, 0, sizeof arg_coordinate);
  if (!mln_ts_is_absent(call, call->argv[1])) {
    if (!mln_ts_read_record(call, call->argv[1], "coordinate")) goto done;
    if (!mln_ts_write_lat_lng(
          call, call->argv[1], &arg_coordinate, "coordinate"
        ))
      goto done;
  }
  memset(&arg_out_point, 0, sizeof arg_out_point);
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_projection_pixel_for_lat_lng(
    arg_projection, arg_coordinate, &arg_out_point, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result =
    mln_ts_read_screen_point(env, &(arg_out_point), sizeof(arg_out_point));
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_projection_pixels_for_lat_lngs(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map_projection arg_projection = 0;
  size_t count_coordinates = 0;
  mln_lat_lng* arg_coordinates = NULL;
  mln_screen_point* arg_out_points = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "projection")) goto done;
    arg_projection = (mln_map_projection)number;
  }
  {
    mln_lat_lng* items = NULL;
    size_t count = 0;
    if (!mln_ts_write_list_lat_lng(
          call, call->argv[1], UINT32_MAX, &items, &count, "coordinates"
        ))
      goto done;
    arg_coordinates = items;
    count_coordinates = (size_t)count;
  }
  arg_out_points =
    mln_ts_allocate(call, sizeof *arg_out_points * count_coordinates);
  if (arg_out_points == NULL) goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_projection_pixels_for_lat_lngs(
    arg_projection, arg_coordinates, (size_t)count_coordinates, arg_out_points,
    &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result =
    mln_ts_read_items_screen_point(env, arg_out_points, count_coordinates);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_projection_release(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map_projection arg_projection = 0;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "projection")) goto done;
    arg_projection = (mln_map_projection)number;
  }
  if (!mln_ts_read_owner(call, call->argv[1])) goto done;
  mln_map_projection_release(arg_projection);
  mln_ts_consume(call);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_projection_set_camera(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map_projection arg_projection = 0;
  mln_camera_options arg_camera;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "projection")) goto done;
    arg_projection = (mln_map_projection)number;
  }
  arg_camera = mln_camera_options_default();
  if (!mln_ts_read_record(call, call->argv[1], "camera")) goto done;
  if (!mln_ts_write_camera_options(call, call->argv[1], &arg_camera, "camera"))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_map_projection_set_camera(arg_projection, &arg_camera, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_release(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_owner(call, call->argv[1])) goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_release(arg_map, present_completion, &diagnostic);
  if (status == MLN_STATUS_OK) mln_ts_consume(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_remove_feature_state(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_feature_state_selector arg_selector;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  memset(&arg_selector, 0, sizeof arg_selector);
  arg_selector.size = sizeof arg_selector;
  if (!mln_ts_read_record(call, call->argv[1], "selector")) goto done;
  if (!mln_ts_write_feature_state_selector(
        call, call->argv[1], &arg_selector, "selector"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_remove_feature_state(
    arg_map, &arg_selector, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_remove_style_image(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_image_id = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_image_id, "imageId"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_remove_style_image(
    arg_map, arg_image_id, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_remove_style_layer(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_layer_id = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_layer_id, "layerId"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_remove_style_layer(
    arg_map, arg_layer_id, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_remove_style_source(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_remove_style_source(
    arg_map, arg_source_id, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_request_repaint(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_request_repaint(arg_map, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_request_still_image(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_map_request_still_image(arg_map, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_resize(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_logical_extent arg_extent;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_extent = mln_logical_extent_default();
  if (!mln_ts_is_absent(call, call->argv[1])) {
    if (!mln_ts_read_record(call, call->argv[1], "extent")) goto done;
    if (!mln_ts_write_logical_extent(
          call, call->argv[1], &arg_extent, "extent"
        ))
      goto done;
  }
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_resize(arg_map, arg_extent, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_bounds(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_bound_options arg_options;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_options = mln_bound_options_default();
  if (!mln_ts_read_record(call, call->argv[1], "options")) goto done;
  if (!mln_ts_write_bound_options(call, call->argv[1], &arg_options, "options"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_map_set_bounds(arg_map, &arg_options, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_custom_geometry_source_tile_data(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_canonical_tile_id arg_tile_id;
  mln_buffer_view arg_data = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 5)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  memset(&arg_tile_id, 0, sizeof arg_tile_id);
  if (!mln_ts_is_absent(call, call->argv[2])) {
    if (!mln_ts_read_record(call, call->argv[2], "tileId")) goto done;
    if (!mln_ts_write_canonical_tile_id(
          call, call->argv[2], &arg_tile_id, "tileId"
        ))
      goto done;
  }
  if (!mln_ts_read_json_view(call, call->argv[3], &arg_data, "data")) goto done;
  if (!mln_ts_read_completion(
        call, call->argv[4], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_custom_geometry_source_tile_data(
    arg_map, arg_source_id, arg_tile_id, arg_data, present_completion,
    &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_custom_geometry_source_tile_error(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_canonical_tile_id arg_tile_id;
  mln_buffer_view arg_message = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 5)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  memset(&arg_tile_id, 0, sizeof arg_tile_id);
  if (!mln_ts_is_absent(call, call->argv[2])) {
    if (!mln_ts_read_record(call, call->argv[2], "tileId")) goto done;
    if (!mln_ts_write_canonical_tile_id(
          call, call->argv[2], &arg_tile_id, "tileId"
        ))
      goto done;
  }
  if (!mln_ts_read_text_view(call, call->argv[3], &arg_message, "message"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[4], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_custom_geometry_source_tile_error(
    arg_map, arg_source_id, arg_tile_id, arg_message, present_completion,
    &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_custom_mvt_vector_source_tile_data(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_canonical_tile_id arg_tile_id;
  mln_buffer_view arg_data = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 5)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  memset(&arg_tile_id, 0, sizeof arg_tile_id);
  if (!mln_ts_is_absent(call, call->argv[2])) {
    if (!mln_ts_read_record(call, call->argv[2], "tileId")) goto done;
    if (!mln_ts_write_canonical_tile_id(
          call, call->argv[2], &arg_tile_id, "tileId"
        ))
      goto done;
  }
  if (!mln_ts_read_bytes_view(call, call->argv[3], &arg_data, "data"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[4], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_custom_mvt_vector_source_tile_data(
    arg_map, arg_source_id, arg_tile_id, arg_data, present_completion,
    &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_custom_mvt_vector_source_tile_error(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_canonical_tile_id arg_tile_id;
  mln_buffer_view arg_message = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 5)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  memset(&arg_tile_id, 0, sizeof arg_tile_id);
  if (!mln_ts_is_absent(call, call->argv[2])) {
    if (!mln_ts_read_record(call, call->argv[2], "tileId")) goto done;
    if (!mln_ts_write_canonical_tile_id(
          call, call->argv[2], &arg_tile_id, "tileId"
        ))
      goto done;
  }
  if (!mln_ts_read_text_view(call, call->argv[3], &arg_message, "message"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[4], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_custom_mvt_vector_source_tile_error(
    arg_map, arg_source_id, arg_tile_id, arg_message, present_completion,
    &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_debug_options(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_map_debug_flags arg_options;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  memset(&arg_options, 0, sizeof arg_options);
  {
    int64_t number = 0;
    if (!mln_ts_read_int(
          call, call->argv[1], 0LL, 4294967295LL, &number, "options"
        ))
      goto done;
    arg_options = (mln_map_debug_flags)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_debug_options(
    arg_map, arg_options, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_event_mask(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_runtime_event_flags arg_mask;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  memset(&arg_mask, 0, sizeof arg_mask);
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[1], &number, "mask")) goto done;
    arg_mask = (mln_runtime_event_flags)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_map_set_event_mask(arg_map, arg_mask, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_feature_state(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_feature_state_selector arg_selector;
  mln_buffer_view arg_state = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  memset(&arg_selector, 0, sizeof arg_selector);
  arg_selector.size = sizeof arg_selector;
  if (!mln_ts_read_record(call, call->argv[1], "selector")) goto done;
  if (!mln_ts_write_feature_state_selector(
        call, call->argv[1], &arg_selector, "selector"
      ))
    goto done;
  if (!mln_ts_read_json_view(call, call->argv[2], &arg_state, "state"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_feature_state(
    arg_map, &arg_selector, arg_state, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_free_camera_options(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_free_camera_options arg_options;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_options = mln_free_camera_options_default();
  if (!mln_ts_read_record(call, call->argv[1], "options")) goto done;
  if (!mln_ts_write_free_camera_options(
        call, call->argv[1], &arg_options, "options"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_free_camera_options(
    arg_map, &arg_options, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_geojson_source_data(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  uint64_t arg_data = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  if (!mln_ts_read_u64(call, call->argv[2], &arg_data, "data")) goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_geojson_source_data(
    arg_map, arg_source_id, arg_data, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_geojson_source_synchronous_tiling(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  bool arg_enabled;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  memset(&arg_enabled, 0, sizeof arg_enabled);
  {
    bool flag = false;
    if (!mln_ts_read_bool(call, call->argv[2], &flag, "enabled")) goto done;
    arg_enabled = flag;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_geojson_source_synchronous_tiling(
    arg_map, arg_source_id, arg_enabled, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_geojson_source_url(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_buffer_view arg_url = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  if (!mln_ts_read_text_view(call, call->argv[2], &arg_url, "url")) goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_geojson_source_url(
    arg_map, arg_source_id, arg_url, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_global_state_property(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_property_name = {0};
  mln_buffer_view arg_value = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(
        call, call->argv[1], &arg_property_name, "propertyName"
      ))
    goto done;
  if (!mln_ts_read_json_view(call, call->argv[2], &arg_value, "value"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_global_state_property(
    arg_map, arg_property_name, arg_value, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_image_source_coordinates(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  size_t count_coordinates = 0;
  mln_lat_lng* arg_coordinates = NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  {
    mln_lat_lng* items = NULL;
    size_t count = 0;
    if (!mln_ts_write_list_lat_lng(
          call, call->argv[2], UINT32_MAX, &items, &count, "coordinates"
        ))
      goto done;
    arg_coordinates = items;
    count_coordinates = (size_t)count;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_image_source_coordinates(
    arg_map, arg_source_id, arg_coordinates, (size_t)count_coordinates,
    present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_image_source_image(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_premultiplied_rgba8_image arg_image;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  arg_image = mln_premultiplied_rgba8_image_default();
  if (!mln_ts_read_record(call, call->argv[2], "image")) goto done;
  if (!mln_ts_write_premultiplied_rgba8_image(
        call, call->argv[2], &arg_image, "image"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_image_source_image(
    arg_map, arg_source_id, &arg_image, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_image_source_url(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  mln_buffer_view arg_url = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  if (!mln_ts_read_text_view(call, call->argv[2], &arg_url, "url")) goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_image_source_url(
    arg_map, arg_source_id, arg_url, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_projection_mode(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_projection_mode arg_mode;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_mode = mln_projection_mode_default();
  if (!mln_ts_read_record(call, call->argv[1], "mode")) goto done;
  if (!mln_ts_write_projection_mode(call, call->argv[1], &arg_mode, "mode"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_projection_mode(
    arg_map, &arg_mode, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_rendering_stats_view_enabled(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  bool arg_enabled;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  memset(&arg_enabled, 0, sizeof arg_enabled);
  {
    bool flag = false;
    if (!mln_ts_read_bool(call, call->argv[1], &flag, "enabled")) goto done;
    arg_enabled = flag;
  }
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_rendering_stats_view_enabled(
    arg_map, arg_enabled, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_style_image(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_image_id = {0};
  mln_premultiplied_rgba8_image arg_image;
  mln_style_image_options arg_options;
  const void* present_options = NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 5)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_image_id, "imageId"))
    goto done;
  arg_image = mln_premultiplied_rgba8_image_default();
  if (!mln_ts_read_record(call, call->argv[2], "image")) goto done;
  if (!mln_ts_write_premultiplied_rgba8_image(
        call, call->argv[2], &arg_image, "image"
      ))
    goto done;
  if (!mln_ts_is_absent(call, call->argv[3])) {
    arg_options = mln_style_image_options_default();
    if (!mln_ts_read_record(call, call->argv[3], "options")) goto done;
    if (!mln_ts_write_style_image_options(
          call, call->argv[3], &arg_options, "options"
        ))
      goto done;
    present_options = &arg_options;
  }
  if (!mln_ts_read_completion(
        call, call->argv[4], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_style_image(
    arg_map, arg_image_id, &arg_image, present_options, present_completion,
    &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_style_json(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_json = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_json_view(call, call->argv[1], &arg_json, "json")) goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_map_set_style_json(arg_map, arg_json, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_style_layer_property(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_layer_id = {0};
  mln_buffer_view arg_property_name = {0};
  mln_buffer_view arg_value = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 5)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_layer_id, "layerId"))
    goto done;
  if (!mln_ts_read_text_view(
        call, call->argv[2], &arg_property_name, "propertyName"
      ))
    goto done;
  if (!mln_ts_read_json_view(call, call->argv[3], &arg_value, "value"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[4], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_style_layer_property(
    arg_map, arg_layer_id, arg_property_name, arg_value, present_completion,
    &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_style_light_json(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_light_json = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_json_view(call, call->argv[1], &arg_light_json, "lightJson"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_style_light_json(
    arg_map, arg_light_json, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_style_light_property(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_property_name = {0};
  mln_buffer_view arg_value = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(
        call, call->argv[1], &arg_property_name, "propertyName"
      ))
    goto done;
  if (!mln_ts_read_json_view(call, call->argv[2], &arg_value, "value"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_style_light_property(
    arg_map, arg_property_name, arg_value, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_style_source_volatile(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_source_id = {0};
  bool arg_is_volatile;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  memset(&arg_is_volatile, 0, sizeof arg_is_volatile);
  {
    bool flag = false;
    if (!mln_ts_read_bool(call, call->argv[2], &flag, "isVolatile")) goto done;
    arg_is_volatile = flag;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_style_source_volatile(
    arg_map, arg_source_id, arg_is_volatile, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_style_transition_options(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_style_transition_options arg_options;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_options = mln_style_transition_options_default();
  if (!mln_ts_read_record(call, call->argv[1], "options")) goto done;
  if (!mln_ts_write_style_transition_options(
        call, call->argv[1], &arg_options, "options"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_style_transition_options(
    arg_map, &arg_options, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_style_url(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_buffer_view arg_url = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_url, "url")) goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_map_set_style_url(arg_map, arg_url, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_tile_options(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_map_tile_options arg_options;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_options = mln_map_tile_options_default();
  if (!mln_ts_read_record(call, call->argv[1], "options")) goto done;
  if (!mln_ts_write_map_tile_options(
        call, call->argv[1], &arg_options, "options"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_tile_options(
    arg_map, &arg_options, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_set_viewport_options(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_map_viewport_options arg_options;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_options = mln_map_viewport_options_default();
  if (!mln_ts_read_record(call, call->argv[1], "options")) goto done;
  if (!mln_ts_write_map_viewport_options(
        call, call->argv[1], &arg_options, "options"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_set_viewport_options(
    arg_map, &arg_options, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_tile_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map_tile_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_map_tile_options_default();
  call->result = mln_ts_read_map_tile_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_update_camera(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map arg_map = 0;
  mln_camera_update arg_update;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "map")) goto done;
    arg_map = (mln_map)number;
  }
  arg_update = mln_camera_update_default();
  if (!mln_ts_read_record(call, call->argv[1], "update")) goto done;
  if (!mln_ts_write_camera_update(call, call->argv[1], &arg_update, "update"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_map_update_camera(
    arg_map, &arg_update, present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_map_viewport_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_map_viewport_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_map_viewport_options_default();
  call->result =
    mln_ts_read_map_viewport_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_metal_borrowed_texture_descriptor_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_metal_borrowed_texture_descriptor result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_metal_borrowed_texture_descriptor_default();
  call->result = mln_ts_read_metal_borrowed_texture_descriptor(
    env, &(result), sizeof(result)
  );
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_metal_owned_texture_descriptor_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_metal_owned_texture_descriptor result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_metal_owned_texture_descriptor_default();
  call->result =
    mln_ts_read_metal_owned_texture_descriptor(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_metal_surface_descriptor_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_metal_surface_descriptor result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_metal_surface_descriptor_default();
  call->result =
    mln_ts_read_metal_surface_descriptor(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_network_get_status(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_network_status arg_out_status;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  memset(&arg_out_status, 0, sizeof arg_out_status);
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_network_get_status(&arg_out_status, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_number(env, (double)(arg_out_status));
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_network_set_status(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_network_status arg_status;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  memset(&arg_status, 0, sizeof arg_status);
  {
    int64_t number = 0;
    if (!mln_ts_read_int(
          call, call->argv[0], 0LL, 4294967295LL, &number, "status"
        ))
      goto done;
    arg_status = (mln_network_status)number;
  }
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_network_set_status(arg_status, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_opengl_borrowed_texture_descriptor_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_opengl_borrowed_texture_descriptor result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_opengl_borrowed_texture_descriptor_default();
  call->result = mln_ts_read_opengl_borrowed_texture_descriptor(
    env, &(result), sizeof(result)
  );
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_opengl_owned_texture_descriptor_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_opengl_owned_texture_descriptor result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_opengl_owned_texture_descriptor_default();
  call->result =
    mln_ts_read_opengl_owned_texture_descriptor(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_opengl_supported_context_provider_mask(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_opengl_context_provider_flags result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_opengl_supported_context_provider_mask();
  call->result = mln_ts_number(env, (double)(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_opengl_surface_descriptor_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_opengl_surface_descriptor result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_opengl_surface_descriptor_default();
  call->result =
    mln_ts_read_opengl_surface_descriptor(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_plugin_get_register_function_v1(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_plugin_register_function_v1 result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_plugin_get_register_function_v1();
  call->result = mln_ts_u64(env, (uint64_t)(uintptr_t)(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_premultiplied_rgba8_image_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_premultiplied_rgba8_image result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_premultiplied_rgba8_image_default();
  call->result =
    mln_ts_read_premultiplied_rgba8_image(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_projected_meters_for_lat_lng(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_lat_lng arg_coordinate;
  mln_projected_meters arg_out_meters;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  memset(&arg_coordinate, 0, sizeof arg_coordinate);
  if (!mln_ts_is_absent(call, call->argv[0])) {
    if (!mln_ts_read_record(call, call->argv[0], "coordinate")) goto done;
    if (!mln_ts_write_lat_lng(
          call, call->argv[0], &arg_coordinate, "coordinate"
        ))
      goto done;
  }
  memset(&arg_out_meters, 0, sizeof arg_out_meters);
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_projected_meters_for_lat_lng(
    arg_coordinate, &arg_out_meters, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_read_projected_meters(
    env, &(arg_out_meters), sizeof(arg_out_meters)
  );
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_projection_mode_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_projection_mode result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_projection_mode_default();
  call->result = mln_ts_read_projection_mode(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_abandon(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_render_abandon_result arg_out_result;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  memset(&arg_out_result, 0, sizeof arg_out_result);
  arg_out_result.size = sizeof arg_out_result;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_render_session_abandon(arg_session, &arg_out_result, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_read_render_abandon_result(
    env, &(arg_out_result), sizeof(arg_out_result)
  );
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_acquire_frame(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_acquired_frame arg_out_frame = MLN_HANDLE_NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_render_session_acquire_frame(arg_session, &arg_out_frame, &diagnostic);
  if (status == MLN_STATUS_NOT_READY) goto done;
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)arg_out_frame);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_attach_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session_attach_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_render_session_attach_options_default();
  call->result =
    mln_ts_read_render_session_attach_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_barrier(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_render_session_barrier(arg_session, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_clear_data(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_render_session_clear_data(arg_session, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_detach(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_render_session_detach(arg_session, present_completion, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_dispose(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  if (!mln_ts_read_owner(call, call->argv[1])) goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_render_session_dispose(arg_session, present_completion, &diagnostic);
  if (status == MLN_STATUS_OK) mln_ts_consume(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_drain_frame_results(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_render_frame_batch arg_out_batch = MLN_HANDLE_NULL;
  mln_render_frame_batch_view arg_out_view;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  memset(&arg_out_view, 0, sizeof arg_out_view);
  arg_out_view.size = sizeof arg_out_view;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_drain_frame_results(
    arg_session, &arg_out_batch, &arg_out_view, &diagnostic
  );
  if (status == MLN_STATUS_NOT_READY) goto done;
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_read_render_frame_batch_view(
    env, &(arg_out_view), sizeof(arg_out_view)
  );
  mln_render_frame_batch_release(arg_out_batch);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_dump_debug_logs(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_dump_debug_logs(
    arg_session, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_get_capabilities(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_render_session_capabilities arg_out_capabilities;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  memset(&arg_out_capabilities, 0, sizeof arg_out_capabilities);
  arg_out_capabilities.size = sizeof arg_out_capabilities;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_get_capabilities(
    arg_session, &arg_out_capabilities, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_read_render_session_capabilities(
    env, &(arg_out_capabilities), sizeof(arg_out_capabilities)
  );
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_get_snapshot(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_render_session_snapshot arg_out_snapshot;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  memset(&arg_out_snapshot, 0, sizeof arg_out_snapshot);
  arg_out_snapshot.size = sizeof arg_out_snapshot;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_get_snapshot(
    arg_session, &arg_out_snapshot, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_read_render_session_snapshot(
    env, &(arg_out_snapshot), sizeof(arg_out_snapshot)
  );
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_query_feature_extensions(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_buffer_view arg_source_id = {0};
  mln_buffer_view arg_feature = {0};
  mln_buffer_view arg_extension = {0};
  mln_buffer_view arg_extension_field = {0};
  mln_buffer_view arg_arguments = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 7)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  if (!mln_ts_read_json_view(call, call->argv[2], &arg_feature, "feature"))
    goto done;
  if (!mln_ts_read_text_view(call, call->argv[3], &arg_extension, "extension"))
    goto done;
  if (!mln_ts_read_text_view(
        call, call->argv[4], &arg_extension_field, "extensionField"
      ))
    goto done;
  if (!mln_ts_read_json_view(
        call, call->argv[5], &arg_arguments, "argumentsValue"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[6], false, MLN_TS_DECODE_BUFFER_VIEW_JSON,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_query_feature_extensions(
    arg_session, arg_source_id, arg_feature, arg_extension, arg_extension_field,
    arg_arguments, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_query_rendered_features(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_rendered_query_geometry arg_geometry;
  mln_rendered_feature_query_options arg_options;
  const void* present_options = NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  memset(&arg_geometry, 0, sizeof arg_geometry);
  arg_geometry.size = sizeof arg_geometry;
  if (!mln_ts_read_record(call, call->argv[1], "geometry")) goto done;
  if (!mln_ts_write_rendered_query_geometry(
        call, call->argv[1], &arg_geometry, "geometry"
      ))
    goto done;
  if (!mln_ts_is_absent(call, call->argv[2])) {
    arg_options = mln_rendered_feature_query_options_default();
    if (!mln_ts_read_record(call, call->argv[2], "options")) goto done;
    if (!mln_ts_write_rendered_feature_query_options(
          call, call->argv[2], &arg_options, "options"
        ))
      goto done;
    present_options = &arg_options;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_DECODE_RENDERED_FEATURE_LIST,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_query_rendered_features(
    arg_session, &arg_geometry, present_options, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_query_source_features(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_buffer_view arg_source_id = {0};
  mln_source_feature_query_options arg_options;
  const void* present_options = NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  if (!mln_ts_read_text_view(call, call->argv[1], &arg_source_id, "sourceId"))
    goto done;
  if (!mln_ts_is_absent(call, call->argv[2])) {
    arg_options = mln_source_feature_query_options_default();
    if (!mln_ts_read_record(call, call->argv[2], "options")) goto done;
    if (!mln_ts_write_source_feature_query_options(
          call, call->argv[2], &arg_options, "options"
        ))
      goto done;
    present_options = &arg_options;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_DECODE_SOURCE_FEATURE_LIST,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_query_source_features(
    arg_session, arg_source_id, present_options, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_read_texture(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], false, MLN_TS_DECODE_TEXTURE_READBACK_RESULT,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_read_texture(
    arg_session, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_reduce_memory_use(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_reduce_memory_use(
    arg_session, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_request_frame(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_frame_demand arg_demand;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  arg_demand = mln_frame_demand_default();
  if (!mln_ts_read_record(call, call->argv[1], "demand")) goto done;
  if (!mln_ts_write_frame_demand(call, call->argv[1], &arg_demand, "demand"))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_render_session_request_frame(arg_session, &arg_demand, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_resize(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_logical_extent arg_extent;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  arg_extent = mln_logical_extent_default();
  if (!mln_ts_is_absent(call, call->argv[1])) {
    if (!mln_ts_read_record(call, call->argv[1], "extent")) goto done;
    if (!mln_ts_write_logical_extent(
          call, call->argv[1], &arg_extent, "extent"
        ))
      goto done;
  }
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_resize(
    arg_session, arg_extent, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_service_driver_work(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_service_driver_work(arg_session, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value
mln_ts_export_mln_render_session_set_metal_borrowed_texture_target(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_metal_borrowed_texture_descriptor arg_descriptor;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  arg_descriptor = mln_metal_borrowed_texture_descriptor_default();
  if (!mln_ts_read_record(call, call->argv[1], "descriptor")) goto done;
  if (!mln_ts_write_metal_borrowed_texture_descriptor(
        call, call->argv[1], &arg_descriptor, "descriptor"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_set_metal_borrowed_texture_target(
    arg_session, &arg_descriptor, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_set_metal_surface_target(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_metal_surface_descriptor arg_descriptor;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  arg_descriptor = mln_metal_surface_descriptor_default();
  if (!mln_ts_read_record(call, call->argv[1], "descriptor")) goto done;
  if (!mln_ts_write_metal_surface_descriptor(
        call, call->argv[1], &arg_descriptor, "descriptor"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_set_metal_surface_target(
    arg_session, &arg_descriptor, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value
mln_ts_export_mln_render_session_set_opengl_borrowed_texture_target(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_opengl_borrowed_texture_descriptor arg_descriptor;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  arg_descriptor = mln_opengl_borrowed_texture_descriptor_default();
  if (!mln_ts_read_record(call, call->argv[1], "descriptor")) goto done;
  if (!mln_ts_write_opengl_borrowed_texture_descriptor(
        call, call->argv[1], &arg_descriptor, "descriptor"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_set_opengl_borrowed_texture_target(
    arg_session, &arg_descriptor, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_set_opengl_surface_target(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_opengl_surface_descriptor arg_descriptor;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  arg_descriptor = mln_opengl_surface_descriptor_default();
  if (!mln_ts_read_record(call, call->argv[1], "descriptor")) goto done;
  if (!mln_ts_write_opengl_surface_descriptor(
        call, call->argv[1], &arg_descriptor, "descriptor"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_set_opengl_surface_target(
    arg_session, &arg_descriptor, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value
mln_ts_export_mln_render_session_set_vulkan_borrowed_texture_target(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_vulkan_borrowed_texture_descriptor arg_descriptor;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  arg_descriptor = mln_vulkan_borrowed_texture_descriptor_default();
  if (!mln_ts_read_record(call, call->argv[1], "descriptor")) goto done;
  if (!mln_ts_write_vulkan_borrowed_texture_descriptor(
        call, call->argv[1], &arg_descriptor, "descriptor"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_set_vulkan_borrowed_texture_target(
    arg_session, &arg_descriptor, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_render_session_set_vulkan_surface_target(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_session arg_session = 0;
  mln_vulkan_surface_descriptor arg_descriptor;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "session")) goto done;
    arg_session = (mln_render_session)number;
  }
  arg_descriptor = mln_vulkan_surface_descriptor_default();
  if (!mln_ts_read_record(call, call->argv[1], "descriptor")) goto done;
  if (!mln_ts_write_vulkan_surface_descriptor(
        call, call->argv[1], &arg_descriptor, "descriptor"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_render_session_set_vulkan_surface_target(
    arg_session, &arg_descriptor, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_rendered_feature_query_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_rendered_feature_query_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_rendered_feature_query_options_default();
  call->result =
    mln_ts_read_rendered_feature_query_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_resource_request_complete(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_resource_request arg_handle = 0;
  mln_resource_response arg_response;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "handle")) goto done;
    arg_handle = (mln_resource_request)number;
  }
  if (!mln_ts_read_owner(call, call->argv[1])) goto done;
  memset(&arg_response, 0, sizeof arg_response);
  arg_response.size = sizeof arg_response;
  if (!mln_ts_read_record(call, call->argv[2], "response")) goto done;
  if (!mln_ts_write_resource_response(
        call, call->argv[2], &arg_response, "response"
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_resource_request_complete(arg_handle, &arg_response, &diagnostic);
  if (status == MLN_STATUS_OK) mln_ts_consume(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_resource_request_is_cancelled(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_resource_request arg_handle = 0;
  bool arg_out_cancelled;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "handle")) goto done;
    arg_handle = (mln_resource_request)number;
  }
  memset(&arg_out_cancelled, 0, sizeof arg_out_cancelled);
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_resource_request_is_cancelled(
    arg_handle, &arg_out_cancelled, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_bool(env, arg_out_cancelled);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_resource_request_pass_through(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_resource_request arg_handle = 0;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "handle")) goto done;
    arg_handle = (mln_resource_request)number;
  }
  if (!mln_ts_read_owner(call, call->argv[1])) goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_resource_request_pass_through(arg_handle, &diagnostic);
  if (status == MLN_STATUS_OK) mln_ts_consume(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_resource_request_release(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_resource_request arg_handle = 0;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "handle")) goto done;
    arg_handle = (mln_resource_request)number;
  }
  if (!mln_ts_read_owner(call, call->argv[1])) goto done;
  mln_resource_request_release(arg_handle);
  mln_ts_consume(call);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_resource_request_set_cancel_callback(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_resource_request arg_handle = 0;
  mln_resource_request_cancel_handler arg_handler;
  bool arg_out_cancelled;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "handle")) goto done;
    arg_handle = (mln_resource_request)number;
  }
  memset(&arg_handler, 0, sizeof arg_handler);
  arg_handler.size = sizeof arg_handler;
  if (!mln_ts_read_function(call, call->argv[1], "handler")) goto done;
  if (!mln_ts_read_registration(
        call, call->argv[1], &arg_handler.user_data, &arg_handler.queue,
        "handler"
      ))
    goto done;
  memset(&arg_out_cancelled, 0, sizeof arg_out_cancelled);
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_resource_request_set_cancel_callback(
    arg_handle, &arg_handler, &arg_out_cancelled, &diagnostic
  );
  if (status == MLN_STATUS_OK && !arg_out_cancelled) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_bool(env, arg_out_cancelled);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_resource_route_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_resource_route result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_resource_route_default();
  call->result = mln_ts_read_resource_route(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_resource_transform_request_complete(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_resource_transform_request arg_request = 0;
  mln_buffer_view arg_url = {0};
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "request")) goto done;
    arg_request = (mln_resource_transform_request)number;
  }
  if (!mln_ts_read_owner(call, call->argv[1])) goto done;
  if (!mln_ts_read_text_view(call, call->argv[2], &arg_url, "url")) goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_resource_transform_request_complete(arg_request, arg_url, &diagnostic);
  if (status == MLN_STATUS_OK) mln_ts_consume(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_resource_transform_request_release(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_resource_transform_request arg_request = 0;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "request")) goto done;
    arg_request = (mln_resource_transform_request)number;
  }
  if (!mln_ts_read_owner(call, call->argv[1])) goto done;
  mln_resource_transform_request_release(arg_request);
  mln_ts_consume(call);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_clear_event_handler(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_clear_event_handler(
    arg_runtime, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_clear_http_header_transform(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_clear_http_header_transform(
    arg_runtime, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_clear_resource_provider(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_clear_resource_provider(
    arg_runtime, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_clear_resource_transform(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_clear_resource_transform(
    arg_runtime, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_create(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime_options arg_options;
  mln_runtime arg_out_runtime = MLN_HANDLE_NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  arg_options = mln_runtime_options_default();
  if (!mln_ts_read_record(call, call->argv[0], "options")) goto done;
  if (!mln_ts_write_runtime_options(
        call, call->argv[0], &arg_options, "options"
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_create(&arg_options, &arg_out_runtime, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)arg_out_runtime);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_create_map(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_map_options arg_options;
  mln_map arg_out_map = MLN_HANDLE_NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  arg_options = mln_map_options_default();
  if (!mln_ts_read_record(call, call->argv[1], "options")) goto done;
  if (!mln_ts_write_map_options(call, call->argv[1], &arg_options, "options"))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_create_map(
    arg_runtime, &arg_options, &arg_out_map, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)arg_out_map);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_create_offline_region(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_offline_region_definition arg_definition;
  size_t count_metadata = 0;
  const void* arg_metadata = NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  memset(&arg_definition, 0, sizeof arg_definition);
  arg_definition.size = sizeof arg_definition;
  if (!mln_ts_read_record(call, call->argv[1], "definition")) goto done;
  if (!mln_ts_write_offline_region_definition(
        call, call->argv[1], &arg_definition, "definition"
      ))
    goto done;
  {
    const void* data = NULL;
    size_t size = 0;
    if (!mln_ts_read_sized(
          call, call->argv[2], true, UINT32_MAX, &data, &size, "metadata"
        ))
      goto done;
    arg_metadata = data;
    count_metadata = (size_t)size;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_DECODE_OFFLINE_REGION_INFO,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_create_offline_region(
    arg_runtime, &arg_definition, (const uint8_t*)arg_metadata,
    (size_t)count_metadata, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_delete_offline_region(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_offline_region_id arg_region_id;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  memset(&arg_region_id, 0, sizeof arg_region_id);
  {
    int64_t number = 0;
    if (!mln_ts_read_i64(call, call->argv[1], &number, "regionId")) goto done;
    arg_region_id = (int64_t)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_delete_offline_region(
    arg_runtime, arg_region_id, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_dispose(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  if (!mln_ts_read_owner(call, call->argv[1])) goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_dispose(arg_runtime, &diagnostic);
  if (status == MLN_STATUS_OK) mln_ts_consume(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_get_event_mask(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_runtime_event_flags arg_out_mask;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  memset(&arg_out_mask, 0, sizeof arg_out_mask);
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_get_event_mask(arg_runtime, &arg_out_mask, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)(arg_out_mask));
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_get_offline_region(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_offline_region_id arg_region_id;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  memset(&arg_region_id, 0, sizeof arg_region_id);
  {
    int64_t number = 0;
    if (!mln_ts_read_i64(call, call->argv[1], &number, "regionId")) goto done;
    arg_region_id = (int64_t)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_DECODE_OFFLINE_REGION_INFO_OPTIONAL,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_get_offline_region(
    arg_runtime, arg_region_id, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_get_offline_region_status(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_offline_region_id arg_region_id;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  memset(&arg_region_id, 0, sizeof arg_region_id);
  {
    int64_t number = 0;
    if (!mln_ts_read_i64(call, call->argv[1], &number, "regionId")) goto done;
    arg_region_id = (int64_t)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_DECODE_OFFLINE_REGION_STATUS,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_get_offline_region_status(
    arg_runtime, arg_region_id, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_invalidate_offline_region(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_offline_region_id arg_region_id;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  memset(&arg_region_id, 0, sizeof arg_region_id);
  {
    int64_t number = 0;
    if (!mln_ts_read_i64(call, call->argv[1], &number, "regionId")) goto done;
    arg_region_id = (int64_t)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_invalidate_offline_region(
    arg_runtime, arg_region_id, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_list_offline_regions(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[1], false, MLN_TS_DECODE_OFFLINE_REGION_INFO_LIST,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_list_offline_regions(
    arg_runtime, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_merge_offline_regions(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_buffer_view arg_side_database_path = {0};
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  if (!mln_ts_read_text_view(
        call, call->argv[1], &arg_side_database_path, "sideDatabasePath"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_DECODE_OFFLINE_REGION_INFO_LIST,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_merge_offline_regions(
    arg_runtime, arg_side_database_path, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_runtime_options_default();
  call->result = mln_ts_read_runtime_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_release(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  if (!mln_ts_read_owner(call, call->argv[1])) goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_release(arg_runtime, present_completion, &diagnostic);
  if (status == MLN_STATUS_OK) mln_ts_consume(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_run_ambient_cache_operation(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_ambient_cache_operation arg_operation;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  memset(&arg_operation, 0, sizeof arg_operation);
  {
    int64_t number = 0;
    if (!mln_ts_read_int(
          call, call->argv[1], 0LL, 4294967295LL, &number, "operation"
        ))
      goto done;
    arg_operation = (mln_ambient_cache_operation)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_run_ambient_cache_operation(
    arg_runtime, arg_operation, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_set_event_handler(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_runtime_event_handler arg_handler;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  memset(&arg_handler, 0, sizeof arg_handler);
  arg_handler.size = sizeof arg_handler;
  if (!mln_ts_is_absent(call, call->argv[1])) {
    if (!mln_ts_read_function(call, call->argv[1], "handler")) goto done;
    if (!mln_ts_read_registration(
          call, call->argv[1], &arg_handler.user_data, &arg_handler.queue,
          "handler"
        ))
      goto done;
  }
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_set_event_handler(
    arg_runtime, &arg_handler, present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_set_event_mask(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_runtime_event_flags arg_mask;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  memset(&arg_mask, 0, sizeof arg_mask);
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[1], &number, "mask")) goto done;
    arg_mask = (mln_runtime_event_flags)number;
  }
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_set_event_mask(arg_runtime, arg_mask, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_set_http_header_transform(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_http_header_transform arg_transform;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  memset(&arg_transform, 0, sizeof arg_transform);
  arg_transform.size = sizeof arg_transform;
  if (!mln_ts_read_record(call, call->argv[1], "transform")) goto done;
  if (!mln_ts_write_http_header_transform(
        call, call->argv[1], &arg_transform, "transform"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_set_http_header_transform(
    arg_runtime, &arg_transform, present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_set_maximum_ambient_cache_size(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  uint64_t arg_size;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  memset(&arg_size, 0, sizeof arg_size);
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[1], &number, "size")) goto done;
    arg_size = (uint64_t)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[2], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_set_maximum_ambient_cache_size(
    arg_runtime, arg_size, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_set_offline_region_download_state(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_offline_region_id arg_region_id;
  mln_offline_region_download_state arg_state;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  memset(&arg_region_id, 0, sizeof arg_region_id);
  {
    int64_t number = 0;
    if (!mln_ts_read_i64(call, call->argv[1], &number, "regionId")) goto done;
    arg_region_id = (int64_t)number;
  }
  memset(&arg_state, 0, sizeof arg_state);
  {
    int64_t number = 0;
    if (!mln_ts_read_int(
          call, call->argv[2], 0LL, 4294967295LL, &number, "state"
        ))
      goto done;
    arg_state = (mln_offline_region_download_state)number;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_set_offline_region_download_state(
    arg_runtime, arg_region_id, arg_state, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_set_offline_region_observed(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_offline_region_id arg_region_id;
  bool arg_observed;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  memset(&arg_region_id, 0, sizeof arg_region_id);
  {
    int64_t number = 0;
    if (!mln_ts_read_i64(call, call->argv[1], &number, "regionId")) goto done;
    arg_region_id = (int64_t)number;
  }
  memset(&arg_observed, 0, sizeof arg_observed);
  {
    bool flag = false;
    if (!mln_ts_read_bool(call, call->argv[2], &flag, "observed")) goto done;
    arg_observed = flag;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_set_offline_region_observed(
    arg_runtime, arg_region_id, arg_observed, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_set_resource_provider(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_resource_provider arg_provider;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  memset(&arg_provider, 0, sizeof arg_provider);
  arg_provider.size = sizeof arg_provider;
  if (!mln_ts_read_record(call, call->argv[1], "provider")) goto done;
  if (!mln_ts_write_resource_provider(
        call, call->argv[1], &arg_provider, "provider"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_set_resource_provider(
    arg_runtime, &arg_provider, present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_set_resource_transform(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_resource_transform arg_transform;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 3)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  memset(&arg_transform, 0, sizeof arg_transform);
  arg_transform.size = sizeof arg_transform;
  if (!mln_ts_read_function(call, call->argv[1], "transform")) goto done;
  if (!mln_ts_read_registration(
        call, call->argv[1], &arg_transform.user_data, &arg_transform.queue,
        "transform"
      ))
    goto done;
  if (!mln_ts_read_completion(
        call, call->argv[2], true, MLN_TS_NO_DECODER, &arg_completion,
        &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_set_resource_transform(
    arg_runtime, &arg_transform, present_completion, &diagnostic
  );
  if (status == MLN_STATUS_OK) mln_ts_accept(call);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_runtime_update_offline_region_metadata(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_runtime arg_runtime = 0;
  mln_offline_region_id arg_region_id;
  size_t count_metadata = 0;
  const void* arg_metadata = NULL;
  mln_completion arg_completion;
  const mln_completion* present_completion = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 4)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "runtime")) goto done;
    arg_runtime = (mln_runtime)number;
  }
  memset(&arg_region_id, 0, sizeof arg_region_id);
  {
    int64_t number = 0;
    if (!mln_ts_read_i64(call, call->argv[1], &number, "regionId")) goto done;
    arg_region_id = (int64_t)number;
  }
  {
    const void* data = NULL;
    size_t size = 0;
    if (!mln_ts_read_sized(
          call, call->argv[2], true, UINT32_MAX, &data, &size, "metadata"
        ))
      goto done;
    arg_metadata = data;
    count_metadata = (size_t)size;
  }
  if (!mln_ts_read_completion(
        call, call->argv[3], false, MLN_TS_DECODE_OFFLINE_REGION_INFO,
        &arg_completion, &present_completion
      ))
    goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_runtime_update_offline_region_metadata(
    arg_runtime, arg_region_id, (const uint8_t*)arg_metadata,
    (size_t)count_metadata, present_completion, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_source_feature_query_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_source_feature_query_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_source_feature_query_options_default();
  call->result =
    mln_ts_read_source_feature_query_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_style_expression_create(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_buffer_view arg_expression = {0};
  mln_style_expression_options arg_options;
  const void* present_options = NULL;
  mln_style_expression arg_out_expression = MLN_HANDLE_NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  if (!mln_ts_read_json_view(
        call, call->argv[0], &arg_expression, "expression"
      ))
    goto done;
  if (!mln_ts_is_absent(call, call->argv[1])) {
    arg_options = mln_style_expression_options_default();
    if (!mln_ts_read_record(call, call->argv[1], "options")) goto done;
    if (!mln_ts_write_style_expression_options(
          call, call->argv[1], &arg_options, "options"
        ))
      goto done;
    present_options = &arg_options;
  }
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_style_expression_create(
    arg_expression, present_options, &arg_out_expression, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)arg_out_expression);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_style_expression_evaluate(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_style_expression arg_expression = 0;
  mln_style_expression_inputs arg_inputs;
  const void* present_inputs = NULL;
  char* arg_out_json = NULL;
  size_t arg_json_capacity = 256;
  size_t arg_out_json_length = 0;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "expression")) goto done;
    arg_expression = (mln_style_expression)number;
  }
  if (!mln_ts_is_absent(call, call->argv[1])) {
    arg_inputs = mln_style_expression_inputs_default();
    if (!mln_ts_read_record(call, call->argv[1], "inputs")) goto done;
    if (!mln_ts_write_style_expression_inputs(
          call, call->argv[1], &arg_inputs, "inputs"
        ))
      goto done;
    present_inputs = &arg_inputs;
  }
  arg_out_json = mln_ts_allocate(call, arg_json_capacity);
  if (arg_out_json == NULL) goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  for (;;) {
    status = mln_style_expression_evaluate(
      arg_expression, present_inputs, arg_out_json, arg_json_capacity,
      &arg_out_json_length, &diagnostic
    );
    if (status != MLN_STATUS_OK || arg_out_json_length < arg_json_capacity)
      break;
    arg_json_capacity = arg_out_json_length + 1;
    arg_out_json = mln_ts_allocate(call, arg_json_capacity);
    if (arg_out_json == NULL) goto done;
  }
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_json(env, arg_out_json, arg_out_json_length);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_style_expression_get_info(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_style_expression arg_expression = 0;
  mln_style_expression_info arg_out_info;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "expression")) goto done;
    arg_expression = (mln_style_expression)number;
  }
  memset(&arg_out_info, 0, sizeof arg_out_info);
  arg_out_info.size = sizeof arg_out_info;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status =
    mln_style_expression_get_info(arg_expression, &arg_out_info, &diagnostic);
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_read_style_expression_info(
    env, &(arg_out_info), sizeof(arg_out_info)
  );
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_style_expression_inputs_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_style_expression_inputs result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_style_expression_inputs_default();
  call->result =
    mln_ts_read_style_expression_inputs(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_style_expression_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_style_expression_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_style_expression_options_default();
  call->result =
    mln_ts_read_style_expression_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_style_expression_release(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_style_expression arg_expression = 0;
  if (!mln_ts_begin(call, env, info, 2)) return NULL;
  {
    uint64_t number = 0;
    if (!mln_ts_read_u64(call, call->argv[0], &number, "expression")) goto done;
    arg_expression = (mln_style_expression)number;
  }
  if (!mln_ts_read_owner(call, call->argv[1])) goto done;
  mln_style_expression_release(arg_expression);
  mln_ts_consume(call);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_style_image_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_style_image_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_style_image_options_default();
  call->result =
    mln_ts_read_style_image_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_style_transition_options_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_style_transition_options result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_style_transition_options_default();
  call->result =
    mln_ts_read_style_transition_options(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_supported_render_backend_mask(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_render_backend_flags result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_supported_render_backend_mask();
  call->result = mln_ts_number(env, (double)(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_vulkan_borrowed_texture_descriptor_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_vulkan_borrowed_texture_descriptor result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_vulkan_borrowed_texture_descriptor_default();
  call->result = mln_ts_read_vulkan_borrowed_texture_descriptor(
    env, &(result), sizeof(result)
  );
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_vulkan_owned_texture_descriptor_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_vulkan_owned_texture_descriptor result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_vulkan_owned_texture_descriptor_default();
  call->result =
    mln_ts_read_vulkan_owned_texture_descriptor(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_vulkan_surface_descriptor_default(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  mln_vulkan_surface_descriptor result;
  if (!mln_ts_begin(call, env, info, 0)) return NULL;
  result = mln_vulkan_surface_descriptor_default();
  call->result =
    mln_ts_read_vulkan_surface_descriptor(env, &(result), sizeof(result));
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_acquired_frame_view_begin(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  uint64_t handle = 0;
  void* scope = NULL;
  mln_diagnostic diagnostic;
  mln_status status;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  if (!mln_ts_read_u64(call, call->argv[0], &handle, "handle")) goto done;
  diagnostic.size = sizeof diagnostic;
  diagnostic.message[0] = '\0';
  status = mln_acquired_frame_view_begin(
    (mln_acquired_frame)handle, &scope, &diagnostic
  );
  if (!mln_ts_check(call, status, &diagnostic)) goto done;
  call->result = mln_ts_u64(env, (uint64_t)(uintptr_t)scope);
done:
  return mln_ts_end(call);
}

static napi_value mln_ts_export_mln_acquired_frame_view_end(
  napi_env env, napi_callback_info info
) {
  mln_ts_call call_state;
  mln_ts_call* call = &call_state;
  uint64_t scope = 0;
  if (!mln_ts_begin(call, env, info, 1)) return NULL;
  if (mln_ts_read_u64(call, call->argv[0], &scope, "scope")) {
    mln_acquired_frame_view_end((void*)(uintptr_t)scope);
  }
  return mln_ts_end(call);
}

static const mln_ts_export exports[] = {
  {"mln_acquired_frame_discard", mln_ts_export_mln_acquired_frame_discard},
  {"mln_acquired_frame_get_metal_texture",
   mln_ts_export_mln_acquired_frame_get_metal_texture},
  {"mln_acquired_frame_get_opengl_texture",
   mln_ts_export_mln_acquired_frame_get_opengl_texture},
  {"mln_acquired_frame_get_producer_sync",
   mln_ts_export_mln_acquired_frame_get_producer_sync},
  {"mln_acquired_frame_get_result",
   mln_ts_export_mln_acquired_frame_get_result},
  {"mln_acquired_frame_get_vulkan_texture",
   mln_ts_export_mln_acquired_frame_get_vulkan_texture},
  {"mln_acquired_frame_release", mln_ts_export_mln_acquired_frame_release},
  {"mln_acquired_frame_view_begin",
   mln_ts_export_mln_acquired_frame_view_begin},
  {"mln_acquired_frame_view_end", mln_ts_export_mln_acquired_frame_view_end},
  {"mln_animation_options_default",
   mln_ts_export_mln_animation_options_default},
  {"mln_bound_options_default", mln_ts_export_mln_bound_options_default},
  {"mln_c_version", mln_ts_export_mln_c_version},
  {"mln_camera_delta_default", mln_ts_export_mln_camera_delta_default},
  {"mln_camera_fit_options_default",
   mln_ts_export_mln_camera_fit_options_default},
  {"mln_camera_options_default", mln_ts_export_mln_camera_options_default},
  {"mln_camera_update_default", mln_ts_export_mln_camera_update_default},
  {"mln_custom_geometry_source_options_default",
   mln_ts_export_mln_custom_geometry_source_options_default},
  {"mln_custom_mvt_vector_source_options_default",
   mln_ts_export_mln_custom_mvt_vector_source_options_default},
  {"mln_frame_demand_default", mln_ts_export_mln_frame_demand_default},
  {"mln_free_camera_options_default",
   mln_ts_export_mln_free_camera_options_default},
  {"mln_geojson_source_data_create",
   mln_ts_export_mln_geojson_source_data_create},
  {"mln_geojson_source_data_release",
   mln_ts_export_mln_geojson_source_data_release},
  {"mln_geojson_source_options_default",
   mln_ts_export_mln_geojson_source_options_default},
  {"mln_gpu_sync_default", mln_ts_export_mln_gpu_sync_default},
  {"mln_http_header_rule_default", mln_ts_export_mln_http_header_rule_default},
  {"mln_lat_lng_for_projected_meters",
   mln_ts_export_mln_lat_lng_for_projected_meters},
  {"mln_log_clear_callback", mln_ts_export_mln_log_clear_callback},
  {"mln_log_set_async_severity_mask",
   mln_ts_export_mln_log_set_async_severity_mask},
  {"mln_log_set_callback", mln_ts_export_mln_log_set_callback},
  {"mln_logical_extent_default", mln_ts_export_mln_logical_extent_default},
  {"mln_logical_extent_physical_size",
   mln_ts_export_mln_logical_extent_physical_size},
  {"mln_map_add_custom_geometry_source",
   mln_ts_export_mln_map_add_custom_geometry_source},
  {"mln_map_add_custom_mvt_vector_source",
   mln_ts_export_mln_map_add_custom_mvt_vector_source},
  {"mln_map_add_geojson_source_data",
   mln_ts_export_mln_map_add_geojson_source_data},
  {"mln_map_add_geojson_source_url",
   mln_ts_export_mln_map_add_geojson_source_url},
  {"mln_map_add_image_source_image",
   mln_ts_export_mln_map_add_image_source_image},
  {"mln_map_add_image_source_url", mln_ts_export_mln_map_add_image_source_url},
  {"mln_map_add_style_layer_json", mln_ts_export_mln_map_add_style_layer_json},
  {"mln_map_add_style_source_json",
   mln_ts_export_mln_map_add_style_source_json},
  {"mln_map_apply_camera_delta", mln_ts_export_mln_map_apply_camera_delta},
  {"mln_map_attach_metal_borrowed_texture",
   mln_ts_export_mln_map_attach_metal_borrowed_texture},
  {"mln_map_attach_metal_owned_texture",
   mln_ts_export_mln_map_attach_metal_owned_texture},
  {"mln_map_attach_metal_surface", mln_ts_export_mln_map_attach_metal_surface},
  {"mln_map_attach_opengl_borrowed_texture",
   mln_ts_export_mln_map_attach_opengl_borrowed_texture},
  {"mln_map_attach_opengl_owned_texture",
   mln_ts_export_mln_map_attach_opengl_owned_texture},
  {"mln_map_attach_opengl_surface",
   mln_ts_export_mln_map_attach_opengl_surface},
  {"mln_map_attach_vulkan_borrowed_texture",
   mln_ts_export_mln_map_attach_vulkan_borrowed_texture},
  {"mln_map_attach_vulkan_owned_texture",
   mln_ts_export_mln_map_attach_vulkan_owned_texture},
  {"mln_map_attach_vulkan_surface",
   mln_ts_export_mln_map_attach_vulkan_surface},
  {"mln_map_begin_command_group", mln_ts_export_mln_map_begin_command_group},
  {"mln_map_camera_for_geometry", mln_ts_export_mln_map_camera_for_geometry},
  {"mln_map_camera_for_lat_lng_bounds",
   mln_ts_export_mln_map_camera_for_lat_lng_bounds},
  {"mln_map_camera_for_lat_lngs", mln_ts_export_mln_map_camera_for_lat_lngs},
  {"mln_map_cancel_camera_transition",
   mln_ts_export_mln_map_cancel_camera_transition},
  {"mln_map_cancel_transitions", mln_ts_export_mln_map_cancel_transitions},
  {"mln_map_dispose", mln_ts_export_mln_map_dispose},
  {"mln_map_dump_debug_logs", mln_ts_export_mln_map_dump_debug_logs},
  {"mln_map_end_command_group", mln_ts_export_mln_map_end_command_group},
  {"mln_map_get_camera", mln_ts_export_mln_map_get_camera},
  {"mln_map_get_feature_state", mln_ts_export_mln_map_get_feature_state},
  {"mln_map_get_global_state", mln_ts_export_mln_map_get_global_state},
  {"mln_map_get_image_source_coordinates",
   mln_ts_export_mln_map_get_image_source_coordinates},
  {"mln_map_get_loaded_style_json",
   mln_ts_export_mln_map_get_loaded_style_json},
  {"mln_map_get_snapshot", mln_ts_export_mln_map_get_snapshot},
  {"mln_map_get_style_image", mln_ts_export_mln_map_get_style_image},
  {"mln_map_get_style_layer", mln_ts_export_mln_map_get_style_layer},
  {"mln_map_get_style_layer_json", mln_ts_export_mln_map_get_style_layer_json},
  {"mln_map_get_style_layer_property",
   mln_ts_export_mln_map_get_style_layer_property},
  {"mln_map_get_style_light_property",
   mln_ts_export_mln_map_get_style_light_property},
  {"mln_map_get_style_source", mln_ts_export_mln_map_get_style_source},
  {"mln_map_get_style_transition_options",
   mln_ts_export_mln_map_get_style_transition_options},
  {"mln_map_get_style_url", mln_ts_export_mln_map_get_style_url},
  {"mln_map_invalidate_custom_geometry_source_region",
   mln_ts_export_mln_map_invalidate_custom_geometry_source_region},
  {"mln_map_invalidate_custom_geometry_source_tile",
   mln_ts_export_mln_map_invalidate_custom_geometry_source_tile},
  {"mln_map_invalidate_custom_mvt_vector_source_tile",
   mln_ts_export_mln_map_invalidate_custom_mvt_vector_source_tile},
  {"mln_map_lat_lng_bounds_for_camera",
   mln_ts_export_mln_map_lat_lng_bounds_for_camera},
  {"mln_map_lat_lng_bounds_for_camera_unwrapped",
   mln_ts_export_mln_map_lat_lng_bounds_for_camera_unwrapped},
  {"mln_map_list_style_layers", mln_ts_export_mln_map_list_style_layers},
  {"mln_map_list_style_sources", mln_ts_export_mln_map_list_style_sources},
  {"mln_map_move_style_layer", mln_ts_export_mln_map_move_style_layer},
  {"mln_map_options_default", mln_ts_export_mln_map_options_default},
  {"mln_map_projection_create", mln_ts_export_mln_map_projection_create},
  {"mln_map_projection_get_camera",
   mln_ts_export_mln_map_projection_get_camera},
  {"mln_map_projection_lat_lng_for_pixel",
   mln_ts_export_mln_map_projection_lat_lng_for_pixel},
  {"mln_map_projection_lat_lng_for_pixel_unwrapped",
   mln_ts_export_mln_map_projection_lat_lng_for_pixel_unwrapped},
  {"mln_map_projection_lat_lngs_for_pixels",
   mln_ts_export_mln_map_projection_lat_lngs_for_pixels},
  {"mln_map_projection_lat_lngs_for_pixels_unwrapped",
   mln_ts_export_mln_map_projection_lat_lngs_for_pixels_unwrapped},
  {"mln_map_projection_meters_per_pixel_at_latitude",
   mln_ts_export_mln_map_projection_meters_per_pixel_at_latitude},
  {"mln_map_projection_pixel_for_lat_lng",
   mln_ts_export_mln_map_projection_pixel_for_lat_lng},
  {"mln_map_projection_pixels_for_lat_lngs",
   mln_ts_export_mln_map_projection_pixels_for_lat_lngs},
  {"mln_map_projection_release", mln_ts_export_mln_map_projection_release},
  {"mln_map_projection_set_camera",
   mln_ts_export_mln_map_projection_set_camera},
  {"mln_map_release", mln_ts_export_mln_map_release},
  {"mln_map_remove_feature_state", mln_ts_export_mln_map_remove_feature_state},
  {"mln_map_remove_style_image", mln_ts_export_mln_map_remove_style_image},
  {"mln_map_remove_style_layer", mln_ts_export_mln_map_remove_style_layer},
  {"mln_map_remove_style_source", mln_ts_export_mln_map_remove_style_source},
  {"mln_map_request_repaint", mln_ts_export_mln_map_request_repaint},
  {"mln_map_request_still_image", mln_ts_export_mln_map_request_still_image},
  {"mln_map_resize", mln_ts_export_mln_map_resize},
  {"mln_map_set_bounds", mln_ts_export_mln_map_set_bounds},
  {"mln_map_set_custom_geometry_source_tile_data",
   mln_ts_export_mln_map_set_custom_geometry_source_tile_data},
  {"mln_map_set_custom_geometry_source_tile_error",
   mln_ts_export_mln_map_set_custom_geometry_source_tile_error},
  {"mln_map_set_custom_mvt_vector_source_tile_data",
   mln_ts_export_mln_map_set_custom_mvt_vector_source_tile_data},
  {"mln_map_set_custom_mvt_vector_source_tile_error",
   mln_ts_export_mln_map_set_custom_mvt_vector_source_tile_error},
  {"mln_map_set_debug_options", mln_ts_export_mln_map_set_debug_options},
  {"mln_map_set_event_mask", mln_ts_export_mln_map_set_event_mask},
  {"mln_map_set_feature_state", mln_ts_export_mln_map_set_feature_state},
  {"mln_map_set_free_camera_options",
   mln_ts_export_mln_map_set_free_camera_options},
  {"mln_map_set_geojson_source_data",
   mln_ts_export_mln_map_set_geojson_source_data},
  {"mln_map_set_geojson_source_synchronous_tiling",
   mln_ts_export_mln_map_set_geojson_source_synchronous_tiling},
  {"mln_map_set_geojson_source_url",
   mln_ts_export_mln_map_set_geojson_source_url},
  {"mln_map_set_global_state_property",
   mln_ts_export_mln_map_set_global_state_property},
  {"mln_map_set_image_source_coordinates",
   mln_ts_export_mln_map_set_image_source_coordinates},
  {"mln_map_set_image_source_image",
   mln_ts_export_mln_map_set_image_source_image},
  {"mln_map_set_image_source_url", mln_ts_export_mln_map_set_image_source_url},
  {"mln_map_set_projection_mode", mln_ts_export_mln_map_set_projection_mode},
  {"mln_map_set_rendering_stats_view_enabled",
   mln_ts_export_mln_map_set_rendering_stats_view_enabled},
  {"mln_map_set_style_image", mln_ts_export_mln_map_set_style_image},
  {"mln_map_set_style_json", mln_ts_export_mln_map_set_style_json},
  {"mln_map_set_style_layer_property",
   mln_ts_export_mln_map_set_style_layer_property},
  {"mln_map_set_style_light_json", mln_ts_export_mln_map_set_style_light_json},
  {"mln_map_set_style_light_property",
   mln_ts_export_mln_map_set_style_light_property},
  {"mln_map_set_style_source_volatile",
   mln_ts_export_mln_map_set_style_source_volatile},
  {"mln_map_set_style_transition_options",
   mln_ts_export_mln_map_set_style_transition_options},
  {"mln_map_set_style_url", mln_ts_export_mln_map_set_style_url},
  {"mln_map_set_tile_options", mln_ts_export_mln_map_set_tile_options},
  {"mln_map_set_viewport_options", mln_ts_export_mln_map_set_viewport_options},
  {"mln_map_tile_options_default", mln_ts_export_mln_map_tile_options_default},
  {"mln_map_update_camera", mln_ts_export_mln_map_update_camera},
  {"mln_map_viewport_options_default",
   mln_ts_export_mln_map_viewport_options_default},
  {"mln_metal_borrowed_texture_descriptor_default",
   mln_ts_export_mln_metal_borrowed_texture_descriptor_default},
  {"mln_metal_owned_texture_descriptor_default",
   mln_ts_export_mln_metal_owned_texture_descriptor_default},
  {"mln_metal_surface_descriptor_default",
   mln_ts_export_mln_metal_surface_descriptor_default},
  {"mln_network_get_status", mln_ts_export_mln_network_get_status},
  {"mln_network_set_status", mln_ts_export_mln_network_set_status},
  {"mln_opengl_borrowed_texture_descriptor_default",
   mln_ts_export_mln_opengl_borrowed_texture_descriptor_default},
  {"mln_opengl_owned_texture_descriptor_default",
   mln_ts_export_mln_opengl_owned_texture_descriptor_default},
  {"mln_opengl_supported_context_provider_mask",
   mln_ts_export_mln_opengl_supported_context_provider_mask},
  {"mln_opengl_surface_descriptor_default",
   mln_ts_export_mln_opengl_surface_descriptor_default},
  {"mln_plugin_get_register_function_v1",
   mln_ts_export_mln_plugin_get_register_function_v1},
  {"mln_premultiplied_rgba8_image_default",
   mln_ts_export_mln_premultiplied_rgba8_image_default},
  {"mln_projected_meters_for_lat_lng",
   mln_ts_export_mln_projected_meters_for_lat_lng},
  {"mln_projection_mode_default", mln_ts_export_mln_projection_mode_default},
  {"mln_render_session_abandon", mln_ts_export_mln_render_session_abandon},
  {"mln_render_session_acquire_frame",
   mln_ts_export_mln_render_session_acquire_frame},
  {"mln_render_session_attach_options_default",
   mln_ts_export_mln_render_session_attach_options_default},
  {"mln_render_session_barrier", mln_ts_export_mln_render_session_barrier},
  {"mln_render_session_clear_data",
   mln_ts_export_mln_render_session_clear_data},
  {"mln_render_session_detach", mln_ts_export_mln_render_session_detach},
  {"mln_render_session_dispose", mln_ts_export_mln_render_session_dispose},
  {"mln_render_session_drain_frame_results",
   mln_ts_export_mln_render_session_drain_frame_results},
  {"mln_render_session_dump_debug_logs",
   mln_ts_export_mln_render_session_dump_debug_logs},
  {"mln_render_session_get_capabilities",
   mln_ts_export_mln_render_session_get_capabilities},
  {"mln_render_session_get_snapshot",
   mln_ts_export_mln_render_session_get_snapshot},
  {"mln_render_session_query_feature_extensions",
   mln_ts_export_mln_render_session_query_feature_extensions},
  {"mln_render_session_query_rendered_features",
   mln_ts_export_mln_render_session_query_rendered_features},
  {"mln_render_session_query_source_features",
   mln_ts_export_mln_render_session_query_source_features},
  {"mln_render_session_read_texture",
   mln_ts_export_mln_render_session_read_texture},
  {"mln_render_session_reduce_memory_use",
   mln_ts_export_mln_render_session_reduce_memory_use},
  {"mln_render_session_request_frame",
   mln_ts_export_mln_render_session_request_frame},
  {"mln_render_session_resize", mln_ts_export_mln_render_session_resize},
  {"mln_render_session_service_driver_work",
   mln_ts_export_mln_render_session_service_driver_work},
  {"mln_render_session_set_metal_borrowed_texture_target",
   mln_ts_export_mln_render_session_set_metal_borrowed_texture_target},
  {"mln_render_session_set_metal_surface_target",
   mln_ts_export_mln_render_session_set_metal_surface_target},
  {"mln_render_session_set_opengl_borrowed_texture_target",
   mln_ts_export_mln_render_session_set_opengl_borrowed_texture_target},
  {"mln_render_session_set_opengl_surface_target",
   mln_ts_export_mln_render_session_set_opengl_surface_target},
  {"mln_render_session_set_vulkan_borrowed_texture_target",
   mln_ts_export_mln_render_session_set_vulkan_borrowed_texture_target},
  {"mln_render_session_set_vulkan_surface_target",
   mln_ts_export_mln_render_session_set_vulkan_surface_target},
  {"mln_rendered_feature_query_options_default",
   mln_ts_export_mln_rendered_feature_query_options_default},
  {"mln_resource_request_complete",
   mln_ts_export_mln_resource_request_complete},
  {"mln_resource_request_is_cancelled",
   mln_ts_export_mln_resource_request_is_cancelled},
  {"mln_resource_request_pass_through",
   mln_ts_export_mln_resource_request_pass_through},
  {"mln_resource_request_release", mln_ts_export_mln_resource_request_release},
  {"mln_resource_request_set_cancel_callback",
   mln_ts_export_mln_resource_request_set_cancel_callback},
  {"mln_resource_route_default", mln_ts_export_mln_resource_route_default},
  {"mln_resource_transform_request_complete",
   mln_ts_export_mln_resource_transform_request_complete},
  {"mln_resource_transform_request_release",
   mln_ts_export_mln_resource_transform_request_release},
  {"mln_runtime_clear_event_handler",
   mln_ts_export_mln_runtime_clear_event_handler},
  {"mln_runtime_clear_http_header_transform",
   mln_ts_export_mln_runtime_clear_http_header_transform},
  {"mln_runtime_clear_resource_provider",
   mln_ts_export_mln_runtime_clear_resource_provider},
  {"mln_runtime_clear_resource_transform",
   mln_ts_export_mln_runtime_clear_resource_transform},
  {"mln_runtime_create", mln_ts_export_mln_runtime_create},
  {"mln_runtime_create_map", mln_ts_export_mln_runtime_create_map},
  {"mln_runtime_create_offline_region",
   mln_ts_export_mln_runtime_create_offline_region},
  {"mln_runtime_delete_offline_region",
   mln_ts_export_mln_runtime_delete_offline_region},
  {"mln_runtime_dispose", mln_ts_export_mln_runtime_dispose},
  {"mln_runtime_get_event_mask", mln_ts_export_mln_runtime_get_event_mask},
  {"mln_runtime_get_offline_region",
   mln_ts_export_mln_runtime_get_offline_region},
  {"mln_runtime_get_offline_region_status",
   mln_ts_export_mln_runtime_get_offline_region_status},
  {"mln_runtime_invalidate_offline_region",
   mln_ts_export_mln_runtime_invalidate_offline_region},
  {"mln_runtime_list_offline_regions",
   mln_ts_export_mln_runtime_list_offline_regions},
  {"mln_runtime_merge_offline_regions",
   mln_ts_export_mln_runtime_merge_offline_regions},
  {"mln_runtime_options_default", mln_ts_export_mln_runtime_options_default},
  {"mln_runtime_release", mln_ts_export_mln_runtime_release},
  {"mln_runtime_run_ambient_cache_operation",
   mln_ts_export_mln_runtime_run_ambient_cache_operation},
  {"mln_runtime_set_event_handler",
   mln_ts_export_mln_runtime_set_event_handler},
  {"mln_runtime_set_event_mask", mln_ts_export_mln_runtime_set_event_mask},
  {"mln_runtime_set_http_header_transform",
   mln_ts_export_mln_runtime_set_http_header_transform},
  {"mln_runtime_set_maximum_ambient_cache_size",
   mln_ts_export_mln_runtime_set_maximum_ambient_cache_size},
  {"mln_runtime_set_offline_region_download_state",
   mln_ts_export_mln_runtime_set_offline_region_download_state},
  {"mln_runtime_set_offline_region_observed",
   mln_ts_export_mln_runtime_set_offline_region_observed},
  {"mln_runtime_set_resource_provider",
   mln_ts_export_mln_runtime_set_resource_provider},
  {"mln_runtime_set_resource_transform",
   mln_ts_export_mln_runtime_set_resource_transform},
  {"mln_runtime_update_offline_region_metadata",
   mln_ts_export_mln_runtime_update_offline_region_metadata},
  {"mln_source_feature_query_options_default",
   mln_ts_export_mln_source_feature_query_options_default},
  {"mln_style_expression_create", mln_ts_export_mln_style_expression_create},
  {"mln_style_expression_evaluate",
   mln_ts_export_mln_style_expression_evaluate},
  {"mln_style_expression_get_info",
   mln_ts_export_mln_style_expression_get_info},
  {"mln_style_expression_inputs_default",
   mln_ts_export_mln_style_expression_inputs_default},
  {"mln_style_expression_options_default",
   mln_ts_export_mln_style_expression_options_default},
  {"mln_style_expression_release", mln_ts_export_mln_style_expression_release},
  {"mln_style_image_options_default",
   mln_ts_export_mln_style_image_options_default},
  {"mln_style_transition_options_default",
   mln_ts_export_mln_style_transition_options_default},
  {"mln_supported_render_backend_mask",
   mln_ts_export_mln_supported_render_backend_mask},
  {"mln_vulkan_borrowed_texture_descriptor_default",
   mln_ts_export_mln_vulkan_borrowed_texture_descriptor_default},
  {"mln_vulkan_owned_texture_descriptor_default",
   mln_ts_export_mln_vulkan_owned_texture_descriptor_default},
  {"mln_vulkan_surface_descriptor_default",
   mln_ts_export_mln_vulkan_surface_descriptor_default},
};

NAPI_MODULE_EXPORT napi_value
napi_register_module_v1(napi_env env, napi_value exports_object) {
  return mln_ts_module_init(
    env, exports_object, exports, sizeof exports / sizeof exports[0]
  );
}
