// Generated from the C headers by tools/bindgen. Do not edit.
#ifndef MAPLIBRE_NATIVE_FFI_NODE_GENERATED_ADDON_H
#define MAPLIBRE_NATIVE_FFI_NODE_GENERATED_ADDON_H

#include "maplibre_native_c/base.h"
#include "maplibre_native_c/camera.h"
#include "maplibre_native_c/graphics_device.h"
#include "maplibre_native_c/logging.h"
#include "maplibre_native_c/map.h"
#include "maplibre_native_c/plugin.h"
#include "maplibre_native_c/projection.h"
#include "maplibre_native_c/query.h"
#include "maplibre_native_c/render_session.h"
#include "maplibre_native_c/render_target.h"
#include "maplibre_native_c/runtime.h"
#include "maplibre_native_c/style.h"
#include "maplibre_native_c/style_expression.h"
#include "maplibre_native_c/surface.h"
#include "maplibre_native_c/texture.h"
#include "runtime.h"

/** The completion decoders, by the constant that an export records in
 * its completion. */
enum {
  MLN_TS_NO_DECODER,
  MLN_TS_DECODE_BUFFER_VIEW,
  MLN_TS_DECODE_BUFFER_VIEW_JSON,
  MLN_TS_DECODE_BUFFER_VIEW_JSON_OPTIONAL,
  MLN_TS_DECODE_CAMERA_OPTIONS,
  MLN_TS_DECODE_CAMERA_QUERY_RESULT,
  MLN_TS_DECODE_LAT_LNG_BOUNDS,
  MLN_TS_DECODE_LAT_LNG_LIST,
  MLN_TS_DECODE_OFFLINE_REGION_INFO,
  MLN_TS_DECODE_OFFLINE_REGION_INFO_LIST,
  MLN_TS_DECODE_OFFLINE_REGION_INFO_OPTIONAL,
  MLN_TS_DECODE_OFFLINE_REGION_STATUS,
  MLN_TS_DECODE_RENDERED_FEATURE_LIST,
  MLN_TS_DECODE_SOURCE_FEATURE_LIST,
  MLN_TS_DECODE_STYLE_IMAGE_INFO_OPTIONAL,
  MLN_TS_DECODE_STYLE_LAYER_INFO_LIST,
  MLN_TS_DECODE_STYLE_LAYER_INFO_OPTIONAL,
  MLN_TS_DECODE_STYLE_SOURCE_INFO_LIST,
  MLN_TS_DECODE_STYLE_SOURCE_INFO_OPTIONAL,
  MLN_TS_DECODE_STYLE_TRANSITION_OPTIONS,
  MLN_TS_DECODE_TEXTURE_READBACK_RESULT,
  MLN_TS_DECODER_END,
};
_Static_assert(
  MLN_TS_DECODER_END <= MLN_TS_MAX_DECODERS,
  "a completion names each decoder beside its token"
);

napi_value mln_ts_decode_buffer_view(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_buffer_view_json(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_buffer_view_json_optional(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_camera_options(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_camera_query_result(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_lat_lng_bounds(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_lat_lng_list(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_offline_region_info(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_offline_region_info_list(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_offline_region_info_optional(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_offline_region_status(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_rendered_feature_list(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_source_feature_list(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_style_image_info_optional(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_style_layer_info_list(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_style_layer_info_optional(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_style_source_info_list(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_style_source_info_optional(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_style_transition_options(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_decode_texture_readback_result(
  napi_env env, const mln_completion_result* result
);
napi_value mln_ts_delivery_payload(napi_env env, const mln_delivery* delivery);
napi_value mln_ts_read_bound_options(
  napi_env env, const mln_bound_options* source, size_t size
);
napi_value mln_ts_read_camera(
  napi_env env, const mln_camera* source, size_t size
);
napi_value mln_ts_read_camera_options(
  napi_env env, const mln_camera_options* source, size_t size
);
napi_value mln_ts_read_camera_query_result(
  napi_env env, const mln_camera_query_result* source, size_t size
);
napi_value mln_ts_read_camera_transition_end(
  napi_env env, const mln_camera_transition_end* source, size_t size
);
napi_value mln_ts_read_camera_transition_end_delivery(
  napi_env env, const mln_camera_transition_end_delivery* source, size_t size
);
napi_value mln_ts_read_canonical_tile_id(
  napi_env env, const mln_canonical_tile_id* source, size_t size
);
napi_value mln_ts_read_custom_source_tile_delivery(
  napi_env env, const mln_custom_source_tile_delivery* source, size_t size
);
napi_value mln_ts_read_edge_insets(
  napi_env env, const mln_edge_insets* source, size_t size
);
napi_value mln_ts_read_event_batch_view(
  napi_env env, const mln_event_batch_view* source, size_t size
);
napi_value mln_ts_read_event_batch_view_events(
  napi_env env, const mln_event_batch_view* source
);
napi_value mln_ts_read_free_camera_options(
  napi_env env, const mln_free_camera_options* source, size_t size
);
napi_value mln_ts_read_gpu_sync(
  napi_env env, const mln_gpu_sync* source, size_t size
);
napi_value mln_ts_read_image_content(
  napi_env env, const mln_image_content* source, size_t size
);
napi_value mln_ts_read_image_stretch(
  napi_env env, const mln_image_stretch* source, size_t size
);
napi_value mln_ts_read_items_lat_lng(
  napi_env env, const mln_lat_lng* items, size_t count
);
napi_value mln_ts_read_items_screen_point(
  napi_env env, const mln_screen_point* items, size_t count
);
napi_value mln_ts_read_lat_lng(
  napi_env env, const mln_lat_lng* source, size_t size
);
napi_value mln_ts_read_lat_lng_bounds(
  napi_env env, const mln_lat_lng_bounds* source, size_t size
);
napi_value mln_ts_read_log_delivery(
  napi_env env, const mln_log_delivery* source, size_t size
);
napi_value mln_ts_read_logical_extent(
  napi_env env, const mln_logical_extent* source, size_t size
);
napi_value mln_ts_read_map_snapshot(
  napi_env env, const mln_map_snapshot* source, size_t size
);
napi_value mln_ts_read_map_tile_options(
  napi_env env, const mln_map_tile_options* source, size_t size
);
napi_value mln_ts_read_map_view(
  napi_env env, const mln_map_view* source, size_t size
);
napi_value mln_ts_read_map_viewport_options(
  napi_env env, const mln_map_viewport_options* source, size_t size
);
napi_value mln_ts_read_metal_texture_frame(
  napi_env env, const mln_metal_texture_frame* source, size_t size
);
napi_value mln_ts_read_offline_geometry_region_definition(
  napi_env env, const mln_offline_geometry_region_definition* source,
  size_t size
);
napi_value mln_ts_read_offline_region_definition(
  napi_env env, const mln_offline_region_definition* source, size_t size
);
napi_value mln_ts_read_offline_region_definition_data(
  napi_env env, const mln_offline_region_definition* source, size_t size
);
napi_value mln_ts_read_offline_region_info(
  napi_env env, const mln_offline_region_info* source, size_t size
);
napi_value mln_ts_read_offline_region_status(
  napi_env env, const mln_offline_region_status* source, size_t size
);
napi_value mln_ts_read_offline_tile_pyramid_region_definition(
  napi_env env, const mln_offline_tile_pyramid_region_definition* source,
  size_t size
);
napi_value mln_ts_read_open_gl_texture_frame(
  napi_env env, const mln_open_gl_texture_frame* source, size_t size
);
napi_value mln_ts_read_projected_meters(
  napi_env env, const mln_projected_meters* source, size_t size
);
napi_value mln_ts_read_projection_mode(
  napi_env env, const mln_projection_mode* source, size_t size
);
napi_value mln_ts_read_quaternion(
  napi_env env, const mln_quaternion* source, size_t size
);
napi_value mln_ts_read_render_abandon_result(
  napi_env env, const mln_render_abandon_result* source, size_t size
);
napi_value mln_ts_read_render_frame_batch_view(
  napi_env env, const mln_render_frame_batch_view* source, size_t size
);
napi_value mln_ts_read_render_frame_batch_view_results(
  napi_env env, const mln_render_frame_batch_view* source
);
napi_value mln_ts_read_render_frame_result(
  napi_env env, const mln_render_frame_result* source, size_t size
);
napi_value mln_ts_read_render_session_capabilities(
  napi_env env, const mln_render_session_capabilities* source, size_t size
);
napi_value mln_ts_read_render_session_snapshot(
  napi_env env, const mln_render_session_snapshot* source, size_t size
);
napi_value mln_ts_read_rendered_feature(
  napi_env env, const mln_rendered_feature* source, size_t size
);
napi_value mln_ts_read_rendering_stats(
  napi_env env, const mln_rendering_stats* source, size_t size
);
napi_value mln_ts_read_resource_range(
  napi_env env, const mln_resource_range* source, size_t size
);
napi_value mln_ts_read_resource_request_cancel_delivery(
  napi_env env, const mln_resource_request_cancel_delivery* source, size_t size
);
napi_value mln_ts_read_resource_request_delivery(
  napi_env env, const mln_resource_request_delivery* source, size_t size
);
napi_value mln_ts_read_resource_request_info(
  napi_env env, const mln_resource_request_info* source, size_t size
);
napi_value mln_ts_read_resource_transform_delivery(
  napi_env env, const mln_resource_transform_delivery* source, size_t size
);
napi_value mln_ts_read_runtime_event(
  napi_env env, const mln_runtime_event* source, size_t size
);
napi_value mln_ts_read_runtime_event_delivery(
  napi_env env, const mln_runtime_event_delivery* source, size_t size
);
napi_value mln_ts_read_runtime_event_offline_region_response_error(
  napi_env env, const mln_runtime_event_offline_region_response_error* source,
  size_t size
);
napi_value mln_ts_read_runtime_event_offline_region_status(
  napi_env env, const mln_runtime_event_offline_region_status* source,
  size_t size
);
napi_value mln_ts_read_runtime_event_offline_region_tile_count_limit(
  napi_env env, const mln_runtime_event_offline_region_tile_count_limit* source,
  size_t size
);
napi_value mln_ts_read_runtime_event_payload(
  napi_env env, const mln_runtime_event* source, size_t size
);
napi_value mln_ts_read_runtime_event_render_frame(
  napi_env env, const mln_runtime_event_render_frame* source, size_t size
);
napi_value mln_ts_read_runtime_event_render_map(
  napi_env env, const mln_runtime_event_render_map* source, size_t size
);
napi_value mln_ts_read_runtime_event_tile_action(
  napi_env env, const mln_runtime_event_tile_action* source, size_t size
);
napi_value mln_ts_read_screen_point(
  napi_env env, const mln_screen_point* source, size_t size
);
napi_value mln_ts_read_source_feature(
  napi_env env, const mln_source_feature* source, size_t size
);
napi_value mln_ts_read_style_expression_info(
  napi_env env, const mln_style_expression_info* source, size_t size
);
napi_value mln_ts_read_style_image_info(
  napi_env env, const mln_style_image_info* source, size_t size
);
napi_value mln_ts_read_style_image_info_stretch_x(
  napi_env env, const mln_style_image_info* source
);
napi_value mln_ts_read_style_image_info_stretch_y(
  napi_env env, const mln_style_image_info* source
);
napi_value mln_ts_read_style_layer_info(
  napi_env env, const mln_style_layer_info* source, size_t size
);
napi_value mln_ts_read_style_source_info(
  napi_env env, const mln_style_source_info* source, size_t size
);
napi_value mln_ts_read_style_source_tile_info(
  napi_env env, const mln_style_source_tile_info* source, size_t size
);
napi_value mln_ts_read_style_source_tile_info_tile_urls(
  napi_env env, const mln_style_source_tile_info* source
);
napi_value mln_ts_read_style_transition_options(
  napi_env env, const mln_style_transition_options* source, size_t size
);
napi_value mln_ts_read_texture_image_info(
  napi_env env, const mln_texture_image_info* source, size_t size
);
napi_value mln_ts_read_texture_readback_result(
  napi_env env, const mln_texture_readback_result* source, size_t size
);
napi_value mln_ts_read_tile_id(
  napi_env env, const mln_tile_id* source, size_t size
);
napi_value mln_ts_read_vec3(napi_env env, const mln_vec3* source, size_t size);
napi_value mln_ts_read_vulkan_texture_frame(
  napi_env env, const mln_vulkan_texture_frame* source, size_t size
);
napi_value mln_ts_read_web_gpu_texture_frame(
  napi_env env, const mln_web_gpu_texture_frame* source, size_t size
);
bool mln_ts_write_animation_options(
  mln_ts_call* call, napi_value value, mln_animation_options* out,
  const char* path
);
bool mln_ts_write_bound_options(
  mln_ts_call* call, napi_value value, mln_bound_options* out, const char* path
);
bool mln_ts_write_camera(
  mln_ts_call* call, napi_value value, mln_camera* out, const char* path
);
bool mln_ts_write_camera_delta(
  mln_ts_call* call, napi_value value, mln_camera_delta* out, const char* path
);
bool mln_ts_write_camera_fit_options(
  mln_ts_call* call, napi_value value, mln_camera_fit_options* out,
  const char* path
);
bool mln_ts_write_camera_options(
  mln_ts_call* call, napi_value value, mln_camera_options* out, const char* path
);
bool mln_ts_write_camera_update(
  mln_ts_call* call, napi_value value, mln_camera_update* out, const char* path
);
bool mln_ts_write_canonical_tile_id(
  mln_ts_call* call, napi_value value, mln_canonical_tile_id* out,
  const char* path
);
bool mln_ts_write_custom_geometry_source_options(
  mln_ts_call* call, napi_value value, mln_custom_geometry_source_options* out,
  const char* path
);
bool mln_ts_write_custom_mvt_vector_source_options(
  mln_ts_call* call, napi_value value,
  mln_custom_mvt_vector_source_options* out, const char* path
);
bool mln_ts_write_edge_insets(
  mln_ts_call* call, napi_value value, mln_edge_insets* out, const char* path
);
bool mln_ts_write_egl_context_descriptor(
  mln_ts_call* call, napi_value value, mln_egl_context_descriptor* out,
  const char* path
);
bool mln_ts_write_feature_state_selector(
  mln_ts_call* call, napi_value value, mln_feature_state_selector* out,
  const char* path
);
bool mln_ts_write_frame_demand(
  mln_ts_call* call, napi_value value, mln_frame_demand* out, const char* path
);
bool mln_ts_write_free_camera_options(
  mln_ts_call* call, napi_value value, mln_free_camera_options* out,
  const char* path
);
bool mln_ts_write_geo_json_source_options(
  mln_ts_call* call, napi_value value, mln_geo_json_source_options* out,
  const char* path
);
bool mln_ts_write_gpu_sync(
  mln_ts_call* call, napi_value value, mln_gpu_sync* out, const char* path
);
bool mln_ts_write_graphics_device_options(
  mln_ts_call* call, napi_value value, mln_graphics_device_options* out,
  const char* path
);
bool mln_ts_write_graphics_device_texture_descriptor(
  mln_ts_call* call, napi_value value,
  mln_graphics_device_texture_descriptor* out, const char* path
);
bool mln_ts_write_http_header(
  mln_ts_call* call, napi_value value, mln_http_header* out, const char* path
);
bool mln_ts_write_http_header_rule(
  mln_ts_call* call, napi_value value, mln_http_header_rule* out,
  const char* path
);
bool mln_ts_write_http_header_transform(
  mln_ts_call* call, napi_value value, mln_http_header_transform* out,
  const char* path
);
bool mln_ts_write_image_content(
  mln_ts_call* call, napi_value value, mln_image_content* out, const char* path
);
bool mln_ts_write_image_stretch(
  mln_ts_call* call, napi_value value, mln_image_stretch* out, const char* path
);
bool mln_ts_write_lat_lng(
  mln_ts_call* call, napi_value value, mln_lat_lng* out, const char* path
);
bool mln_ts_write_lat_lng_bounds(
  mln_ts_call* call, napi_value value, mln_lat_lng_bounds* out, const char* path
);
bool mln_ts_write_list_buffer_view(
  mln_ts_call* call, napi_value value, size_t maximum, mln_buffer_view** out,
  size_t* out_count, const char* path
);
bool mln_ts_write_list_http_header(
  mln_ts_call* call, napi_value value, size_t maximum, mln_http_header** out,
  size_t* out_count, const char* path
);
bool mln_ts_write_list_http_header_rule(
  mln_ts_call* call, napi_value value, size_t maximum,
  mln_http_header_rule** out, size_t* out_count, const char* path
);
bool mln_ts_write_list_image_stretch(
  mln_ts_call* call, napi_value value, size_t maximum, mln_image_stretch** out,
  size_t* out_count, const char* path
);
bool mln_ts_write_list_lat_lng(
  mln_ts_call* call, napi_value value, size_t maximum, mln_lat_lng** out,
  size_t* out_count, const char* path
);
bool mln_ts_write_list_metal_borrowed_texture(
  mln_ts_call* call, napi_value value, size_t maximum,
  mln_metal_borrowed_texture** out, size_t* out_count, const char* path
);
bool mln_ts_write_list_open_gl_borrowed_texture(
  mln_ts_call* call, napi_value value, size_t maximum,
  mln_open_gl_borrowed_texture** out, size_t* out_count, const char* path
);
bool mln_ts_write_list_resource_route(
  mln_ts_call* call, napi_value value, size_t maximum, mln_resource_route** out,
  size_t* out_count, const char* path
);
bool mln_ts_write_list_screen_point(
  mln_ts_call* call, napi_value value, size_t maximum, mln_screen_point** out,
  size_t* out_count, const char* path
);
bool mln_ts_write_list_vulkan_borrowed_texture(
  mln_ts_call* call, napi_value value, size_t maximum,
  mln_vulkan_borrowed_texture** out, size_t* out_count, const char* path
);
bool mln_ts_write_list_web_gpu_borrowed_texture(
  mln_ts_call* call, napi_value value, size_t maximum,
  mln_web_gpu_borrowed_texture** out, size_t* out_count, const char* path
);
bool mln_ts_write_logical_extent(
  mln_ts_call* call, napi_value value, mln_logical_extent* out, const char* path
);
bool mln_ts_write_map_options(
  mln_ts_call* call, napi_value value, mln_map_options* out, const char* path
);
bool mln_ts_write_map_tile_options(
  mln_ts_call* call, napi_value value, mln_map_tile_options* out,
  const char* path
);
bool mln_ts_write_map_view(
  mln_ts_call* call, napi_value value, mln_map_view* out, const char* path
);
bool mln_ts_write_map_viewport_options(
  mln_ts_call* call, napi_value value, mln_map_viewport_options* out,
  const char* path
);
bool mln_ts_write_metal_borrowed_texture(
  mln_ts_call* call, napi_value value, mln_metal_borrowed_texture* out,
  const char* path
);
bool mln_ts_write_metal_borrowed_texture_descriptor(
  mln_ts_call* call, napi_value value,
  mln_metal_borrowed_texture_descriptor* out, const char* path
);
bool mln_ts_write_metal_context_descriptor(
  mln_ts_call* call, napi_value value, mln_metal_context_descriptor* out,
  const char* path
);
bool mln_ts_write_metal_owned_texture_descriptor(
  mln_ts_call* call, napi_value value, mln_metal_owned_texture_descriptor* out,
  const char* path
);
bool mln_ts_write_metal_surface_descriptor(
  mln_ts_call* call, napi_value value, mln_metal_surface_descriptor* out,
  const char* path
);
bool mln_ts_write_offline_geometry_region_definition(
  mln_ts_call* call, napi_value value,
  mln_offline_geometry_region_definition* out, const char* path
);
bool mln_ts_write_offline_region_definition(
  mln_ts_call* call, napi_value value, mln_offline_region_definition* out,
  const char* path
);
bool mln_ts_write_offline_tile_pyramid_region_definition(
  mln_ts_call* call, napi_value value,
  mln_offline_tile_pyramid_region_definition* out, const char* path
);
bool mln_ts_write_open_gl_borrowed_texture(
  mln_ts_call* call, napi_value value, mln_open_gl_borrowed_texture* out,
  const char* path
);
bool mln_ts_write_open_gl_borrowed_texture_descriptor(
  mln_ts_call* call, napi_value value,
  mln_open_gl_borrowed_texture_descriptor* out, const char* path
);
bool mln_ts_write_open_gl_context_descriptor(
  mln_ts_call* call, napi_value value, mln_open_gl_context_descriptor* out,
  const char* path
);
bool mln_ts_write_open_gl_owned_texture_descriptor(
  mln_ts_call* call, napi_value value,
  mln_open_gl_owned_texture_descriptor* out, const char* path
);
bool mln_ts_write_open_gl_surface_descriptor(
  mln_ts_call* call, napi_value value, mln_open_gl_surface_descriptor* out,
  const char* path
);
bool mln_ts_write_premultiplied_rgba8_image(
  mln_ts_call* call, napi_value value, mln_premultiplied_rgba8_image* out,
  const char* path
);
bool mln_ts_write_projected_meters(
  mln_ts_call* call, napi_value value, mln_projected_meters* out,
  const char* path
);
bool mln_ts_write_projection_mode(
  mln_ts_call* call, napi_value value, mln_projection_mode* out,
  const char* path
);
bool mln_ts_write_quaternion(
  mln_ts_call* call, napi_value value, mln_quaternion* out, const char* path
);
bool mln_ts_write_render_session_attach_options(
  mln_ts_call* call, napi_value value, mln_render_session_attach_options* out,
  const char* path
);
bool mln_ts_write_rendered_feature_query_options(
  mln_ts_call* call, napi_value value, mln_rendered_feature_query_options* out,
  const char* path
);
bool mln_ts_write_rendered_query_geometry(
  mln_ts_call* call, napi_value value, mln_rendered_query_geometry* out,
  const char* path
);
bool mln_ts_write_resource_provider(
  mln_ts_call* call, napi_value value, mln_resource_provider* out,
  const char* path
);
bool mln_ts_write_resource_response(
  mln_ts_call* call, napi_value value, mln_resource_response* out,
  const char* path
);
bool mln_ts_write_resource_route(
  mln_ts_call* call, napi_value value, mln_resource_route* out, const char* path
);
bool mln_ts_write_runtime_options(
  mln_ts_call* call, napi_value value, mln_runtime_options* out,
  const char* path
);
bool mln_ts_write_screen_box(
  mln_ts_call* call, napi_value value, mln_screen_box* out, const char* path
);
bool mln_ts_write_screen_line_string(
  mln_ts_call* call, napi_value value, mln_screen_line_string* out,
  const char* path
);
bool mln_ts_write_screen_point(
  mln_ts_call* call, napi_value value, mln_screen_point* out, const char* path
);
bool mln_ts_write_source_feature_query_options(
  mln_ts_call* call, napi_value value, mln_source_feature_query_options* out,
  const char* path
);
bool mln_ts_write_style_expression_inputs(
  mln_ts_call* call, napi_value value, mln_style_expression_inputs* out,
  const char* path
);
bool mln_ts_write_style_expression_options(
  mln_ts_call* call, napi_value value, mln_style_expression_options* out,
  const char* path
);
bool mln_ts_write_style_expression_type(
  mln_ts_call* call, napi_value value, mln_style_expression_type* out,
  const char* path
);
bool mln_ts_write_style_image_options(
  mln_ts_call* call, napi_value value, mln_style_image_options* out,
  const char* path
);
bool mln_ts_write_style_transition_options(
  mln_ts_call* call, napi_value value, mln_style_transition_options* out,
  const char* path
);
bool mln_ts_write_unit_bezier(
  mln_ts_call* call, napi_value value, mln_unit_bezier* out, const char* path
);
bool mln_ts_write_vec3(
  mln_ts_call* call, napi_value value, mln_vec3* out, const char* path
);
bool mln_ts_write_vulkan_borrowed_texture(
  mln_ts_call* call, napi_value value, mln_vulkan_borrowed_texture* out,
  const char* path
);
bool mln_ts_write_vulkan_borrowed_texture_descriptor(
  mln_ts_call* call, napi_value value,
  mln_vulkan_borrowed_texture_descriptor* out, const char* path
);
bool mln_ts_write_vulkan_context_descriptor(
  mln_ts_call* call, napi_value value, mln_vulkan_context_descriptor* out,
  const char* path
);
bool mln_ts_write_vulkan_owned_texture_descriptor(
  mln_ts_call* call, napi_value value, mln_vulkan_owned_texture_descriptor* out,
  const char* path
);
bool mln_ts_write_vulkan_surface_descriptor(
  mln_ts_call* call, napi_value value, mln_vulkan_surface_descriptor* out,
  const char* path
);
bool mln_ts_write_web_gl_context_descriptor(
  mln_ts_call* call, napi_value value, mln_web_gl_context_descriptor* out,
  const char* path
);
bool mln_ts_write_web_gpu_borrowed_texture(
  mln_ts_call* call, napi_value value, mln_web_gpu_borrowed_texture* out,
  const char* path
);
bool mln_ts_write_web_gpu_borrowed_texture_descriptor(
  mln_ts_call* call, napi_value value,
  mln_web_gpu_borrowed_texture_descriptor* out, const char* path
);
bool mln_ts_write_web_gpu_context_descriptor(
  mln_ts_call* call, napi_value value, mln_web_gpu_context_descriptor* out,
  const char* path
);
bool mln_ts_write_web_gpu_owned_texture_descriptor(
  mln_ts_call* call, napi_value value,
  mln_web_gpu_owned_texture_descriptor* out, const char* path
);
bool mln_ts_write_web_gpu_surface_descriptor(
  mln_ts_call* call, napi_value value, mln_web_gpu_surface_descriptor* out,
  const char* path
);
bool mln_ts_write_wgl_context_descriptor(
  mln_ts_call* call, napi_value value, mln_wgl_context_descriptor* out,
  const char* path
);

#endif
