// Generated from the C headers by tools/bindgen. Do not edit.
// ignore_for_file: camel_case_types, constant_identifier_names, non_constant_identifier_names
@DefaultAsset(nativeAssetId)
library;

import 'dart:ffi';

import 'native_asset.dart';

typedef mln_acquired_frame = Uint64;
typedef mln_delivery_batch = Uint64;
typedef mln_delivery_queue = Uint64;
typedef mln_geojson_source_data = Uint64;
typedef mln_map = Uint64;
typedef mln_map_projection = Uint64;
typedef mln_render_frame_batch = Uint64;
typedef mln_render_session = Uint64;
typedef mln_resource_request = Uint64;
typedef mln_resource_transform_request = Uint64;
typedef mln_runtime = Uint64;
typedef mln_style_expression = Uint64;

typedef mln_camera_transition_end_callback =
    Pointer<NativeFunction<mln_camera_transition_end_callbackFunction>>;
typedef mln_camera_transition_end_callbackFunction =
    Void Function(
      Pointer<Void> user_data,
      Pointer<mln_camera_transition_end> end,
    );
typedef mln_completion_callback =
    Pointer<NativeFunction<mln_completion_callbackFunction>>;
typedef mln_completion_callbackFunction =
    Void Function(
      Pointer<Void> user_data,
      Pointer<mln_completion_result> result,
    );
typedef mln_custom_source_cancel_tile_callback =
    Pointer<NativeFunction<mln_custom_source_cancel_tile_callbackFunction>>;
typedef mln_custom_source_cancel_tile_callbackFunction =
    Void Function(Pointer<Void> user_data, mln_canonical_tile_id tile_id);
typedef mln_custom_source_fetch_tile_callback =
    Pointer<NativeFunction<mln_custom_source_fetch_tile_callbackFunction>>;
typedef mln_custom_source_fetch_tile_callbackFunction =
    Void Function(Pointer<Void> user_data, mln_canonical_tile_id tile_id);
typedef mln_http_header_transform_callback =
    Pointer<NativeFunction<mln_http_header_transform_callbackFunction>>;
typedef mln_http_header_transform_callbackFunction =
    Int32 Function(
      Pointer<Void> user_data,
      Uint32 kind,
      Pointer<Char> url,
      Pointer<mln_http_header_transform_response> out_response,
    );
typedef mln_log_callback = Pointer<NativeFunction<mln_log_callbackFunction>>;
typedef mln_log_callbackFunction =
    Uint32 Function(
      Pointer<Void> user_data,
      Uint32 severity,
      Uint32 event,
      Int64 code,
      Pointer<Char> message,
    );
typedef mln_queue_lock_callback =
    Pointer<NativeFunction<mln_queue_lock_callbackFunction>>;
typedef mln_queue_lock_callbackFunction =
    Void Function(Pointer<Void> user_data);
typedef mln_resource_provider_callback =
    Pointer<NativeFunction<mln_resource_provider_callbackFunction>>;
typedef mln_resource_provider_callbackFunction =
    Void Function(
      Pointer<Void> user_data,
      Pointer<mln_resource_request_info> request,
      mln_resource_request handle,
    );
typedef mln_resource_request_cancel_callback =
    Pointer<NativeFunction<mln_resource_request_cancel_callbackFunction>>;
typedef mln_resource_request_cancel_callbackFunction =
    Void Function(Pointer<Void> user_data, mln_resource_request handle);
typedef mln_resource_transform_callback =
    Pointer<NativeFunction<mln_resource_transform_callbackFunction>>;
typedef mln_resource_transform_callbackFunction =
    Void Function(
      Pointer<Void> user_data,
      Uint32 kind,
      Pointer<Char> url,
      mln_resource_transform_request request,
    );
typedef mln_runtime_event_callback =
    Pointer<NativeFunction<mln_runtime_event_callbackFunction>>;
typedef mln_runtime_event_callbackFunction =
    Void Function(
      Pointer<Void> user_data,
      Pointer<mln_event_batch_view> events,
    );
typedef mln_user_data_release =
    Pointer<NativeFunction<mln_user_data_releaseFunction>>;
typedef mln_user_data_releaseFunction = Void Function(Pointer<Void> user_data);
typedef mln_wake_callback = Pointer<NativeFunction<mln_wake_callbackFunction>>;
typedef mln_wake_callbackFunction = Void Function(Pointer<Void> user_data);

final class mln_animation_options extends Struct {
  @Uint32()
  external int fields;
  @Double()
  external double duration_ms;
  @Double()
  external double velocity;
  @Double()
  external double min_zoom;
  external mln_unit_bezier easing;
  @Uint64()
  external int transition_id;
  external mln_camera_transition_handler end_handler;
}

final class mln_bound_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external mln_lat_lng_bounds bounds;
  @Double()
  external double min_zoom;
  @Double()
  external double max_zoom;
  @Double()
  external double min_pitch;
  @Double()
  external double max_pitch;
}

final class mln_buffer_view extends Struct {
  external Pointer<Void> data;
  @Size()
  external int size;
}

final class mln_camera_delta extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external mln_screen_point offset;
  @Double()
  external double scale;
  @Double()
  external double bearing;
  @Double()
  external double pitch;
  external mln_screen_point anchor;
  external mln_animation_options animation;
  @Uint32()
  external int gesture_phase;
  @Uint32()
  external int reserved;
}

final class mln_camera_fit_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external mln_edge_insets padding;
  @Double()
  external double bearing;
  @Double()
  external double pitch;
}

final class mln_camera_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external mln_lat_lng center;
  @Double()
  external double center_altitude;
  external mln_edge_insets padding;
  external mln_screen_point anchor;
  @Double()
  external double zoom;
  @Double()
  external double bearing;
  @Double()
  external double pitch;
  @Double()
  external double roll;
  @Double()
  external double field_of_view;
}

final class mln_camera_query_result extends Struct {
  @Uint64()
  external int generation;
  external mln_camera_options camera;
}

final class mln_camera_transition_end extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int outcome;
  @Uint64()
  external int generation;
}

final class mln_camera_transition_end_delivery extends Struct {
  external Pointer<mln_camera_transition_end> end;
}

final class mln_camera_transition_handler extends Struct {
  external mln_camera_transition_end_callback callback;
  external Pointer<Void> user_data;
  external mln_user_data_release release_user_data;
  @Uint64()
  external int queue;
}

final class mln_camera_update extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int mode;
  external mln_camera_options camera;
  external mln_animation_options animation;
  @Uint32()
  external int gesture_phase;
  @Uint32()
  external int reserved;
}

final class mln_canonical_tile_id extends Struct {
  @Uint32()
  external int z;
  @Uint32()
  external int x;
  @Uint32()
  external int y;
}

final class mln_completion extends Struct {
  @Uint32()
  external int size;
  external mln_completion_callback callback;
  external Pointer<Void> user_data;
  external mln_user_data_release release_user_data;
  @Uint64()
  external int queue;
}

final class mln_completion_result extends Struct {
  @Uint32()
  external int size;
  @Int32()
  external int status;
  @Uint32()
  external int disposition;
  @Uint32()
  external int value_size;
  @Uint64()
  external int generation;
  @Uint64()
  external int render_update_generation;
  external mln_buffer_view diagnostic;
  external Pointer<Void> value;
  @Size()
  external int value_count;
}

final class mln_custom_geometry_source_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external mln_custom_source_fetch_tile_callback fetch_tile;
  external mln_custom_source_cancel_tile_callback cancel_tile;
  external Pointer<Void> user_data;
  @Double()
  external double min_zoom;
  @Double()
  external double max_zoom;
  @Double()
  external double tolerance;
  @Uint32()
  external int tile_size;
  @Uint32()
  external int buffer;
  @Bool()
  external bool clip;
  @Bool()
  external bool wrap;
  external mln_user_data_release release_user_data;
  @Uint64()
  external int queue;
}

final class mln_custom_mvt_vector_source_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external mln_custom_source_fetch_tile_callback fetch_tile;
  external mln_custom_source_cancel_tile_callback cancel_tile;
  external Pointer<Void> user_data;
  @Double()
  external double min_zoom;
  @Double()
  external double max_zoom;
  external mln_user_data_release release_user_data;
  @Uint64()
  external int queue;
}

final class mln_custom_source_tile_delivery extends Struct {
  external mln_canonical_tile_id tile_id;
}

final class mln_delivery extends Struct {
  @Uint32()
  external int type;
  @Uint32()
  external int reserved;
  external Pointer<Void> user_data;
  external mln_delivery_payload payload;
}

final class mln_delivery_batch_view extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int delivery_size;
  external Pointer<mln_delivery> deliveries;
  @Size()
  external int delivery_count;
}

final class mln_delivery_payload extends Union {
  external mln_completion_result completion;
  external mln_log_delivery log_record;
  external mln_resource_request_delivery resource_request;
  external mln_resource_request_cancel_delivery resource_request_cancel;
  external mln_camera_transition_end_delivery camera_transition_end;
  external mln_custom_source_tile_delivery custom_source_fetch_tile;
  external mln_custom_source_tile_delivery custom_source_cancel_tile;
  external mln_runtime_event_delivery runtime_events;
  external mln_resource_transform_delivery resource_transform;
}

final class mln_delivery_queue_options extends Struct {
  @Uint32()
  external int size;
  external mln_wake wake;
}

final class mln_diagnostic extends Struct {
  @Uint32()
  external int size;
  @Array(4096)
  external Array<Char> message;
}

final class mln_edge_insets extends Struct {
  @Double()
  external double top;
  @Double()
  external double left;
  @Double()
  external double bottom;
  @Double()
  external double right;
}

final class mln_egl_context_descriptor extends Struct {
  external Pointer<Void> display;
  external Pointer<Void> config;
  external Pointer<Void> share_context;
  @Uint32()
  external int client_api;
  external Pointer<Void> get_proc_address;
}

final class mln_event_batch_view extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int event_size;
  external Pointer<mln_runtime_event> events;
  @Size()
  external int event_count;
}

final class mln_feature_state_selector extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external mln_buffer_view source_id;
  external mln_buffer_view source_layer_id;
  external mln_buffer_view feature_id;
  external mln_buffer_view state_key;
}

final class mln_frame_demand extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int flags;
  @Uint64()
  external int token;
}

final class mln_free_camera_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external mln_vec3 position;
  external mln_quaternion orientation;
}

final class mln_geojson_source_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  @Double()
  external double min_zoom;
  @Double()
  external double max_zoom;
  @Double()
  external double tolerance;
  @Double()
  external double cluster_max_zoom;
  external mln_buffer_view cluster_properties;
  @Uint32()
  external int buffer;
  @Uint32()
  external int cluster_radius;
  @Uint32()
  external int cluster_min_points;
  @Bool()
  external bool line_metrics;
  @Bool()
  external bool cluster;
  @Bool()
  external bool synchronous_tiling;
}

final class mln_gpu_sync extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int kind;
  @Uint64()
  external int object;
}

final class mln_http_header extends Struct {
  external mln_buffer_view name;
  external mln_buffer_view value;
}

final class mln_http_header_rule extends Struct {
  @Uint32()
  external int kinds;
  @Uint32()
  external int flags;
  external mln_buffer_view url;
  external Pointer<mln_http_header> headers;
  @Size()
  external int header_count;
}

final class mln_http_header_transform extends Struct {
  @Uint32()
  external int size;
  external mln_http_header_transform_callback callback;
  external Pointer<Void> user_data;
  external mln_user_data_release release_user_data;
  external Pointer<mln_http_header_rule> rules;
  @Size()
  external int rule_count;
}

final class mln_http_header_transform_response extends Struct {
  @Uint32()
  external int size;
  external Pointer<Void> context;
}

final class mln_image_content extends Struct {
  @Float()
  external double left;
  @Float()
  external double top;
  @Float()
  external double right;
  @Float()
  external double bottom;
}

final class mln_image_stretch extends Struct {
  @Float()
  external double from;
  @Float()
  external double to;
}

final class mln_lat_lng extends Struct {
  @Double()
  external double latitude;
  @Double()
  external double longitude;
}

final class mln_lat_lng_bounds extends Struct {
  external mln_lat_lng southwest;
  external mln_lat_lng northeast;
}

final class mln_log_delivery extends Struct {
  @Uint32()
  external int severity;
  @Uint32()
  external int event;
  @Int64()
  external int code;
  external Pointer<Char> message;
}

final class mln_log_handler extends Struct {
  @Uint32()
  external int size;
  external mln_log_callback callback;
  external Pointer<Void> user_data;
  external mln_user_data_release release_user_data;
  @Uint64()
  external int queue;
}

final class mln_logical_extent extends Struct {
  @Uint32()
  external int width;
  @Uint32()
  external int height;
  @Double()
  external double scale_factor;
}

final class mln_map_options extends Struct {
  @Uint32()
  external int size;
  external mln_logical_extent initial_extent;
  @Uint32()
  external int map_mode;
  @Bool()
  external bool fast_pfor_enabled;
  @Uint64()
  external int event_mask;
}

final class mln_map_snapshot extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int debug_options;
  @Uint64()
  external int generation;
  @Bool()
  external bool fully_loaded;
  @Bool()
  external bool rendering_stats_view_enabled;
  @Bool()
  external bool gesture_in_progress;
  @Bool()
  external bool transition_in_progress;
  @Uint64()
  external int event_mask;
  @Uint64()
  external int latest_render_update_generation;
  external mln_map_tile_options tile;
  external mln_bound_options bounds;
  external mln_free_camera_options free_camera;
  external mln_map_view view;
}

final class mln_map_tile_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  @Uint32()
  external int prefetch_zoom_delta;
  @Double()
  external double lod_min_radius;
  @Double()
  external double lod_scale;
  @Double()
  external double lod_pitch_threshold;
  @Double()
  external double lod_zoom_shift;
  @Uint32()
  external int lod_mode;
}

final class mln_map_view extends Struct {
  @Uint32()
  external int size;
  external mln_camera_options camera;
  external mln_logical_extent extent;
  external mln_projection_mode projection_mode;
  external mln_map_viewport_options viewport;
}

final class mln_map_viewport_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  @Uint32()
  external int north_orientation;
  @Uint32()
  external int constrain_mode;
  @Uint32()
  external int viewport_mode;
  external mln_edge_insets frustum_offset;
}

final class mln_metal_borrowed_texture extends Struct {
  external Pointer<Void> texture;
}

final class mln_metal_borrowed_texture_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_logical_extent extent;
  @Uint32()
  external int physical_width;
  @Uint32()
  external int physical_height;
  external Pointer<mln_metal_borrowed_texture> textures;
  @Size()
  external int texture_count;
}

final class mln_metal_context_descriptor extends Struct {
  external Pointer<Void> device;
}

final class mln_metal_owned_texture_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_logical_extent extent;
  external mln_metal_context_descriptor context;
  @Uint64()
  external int pixel_format;
}

final class mln_metal_surface_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_logical_extent extent;
  external mln_metal_context_descriptor context;
  external Pointer<Void> layer;
}

final class mln_metal_texture_frame extends Struct {
  @Uint32()
  external int size;
  @Uint64()
  external int generation;
  @Uint32()
  external int width;
  @Uint32()
  external int height;
  @Double()
  external double scale_factor;
  @Uint64()
  external int frame_id;
  @Uint32()
  external int slot;
  external Pointer<Void> texture;
  external Pointer<Void> device;
  @Uint64()
  external int pixel_format;
}

final class mln_offline_geometry_region_definition extends Struct {
  external mln_buffer_view style_url;
  external mln_buffer_view geometry;
  @Double()
  external double min_zoom;
  @Double()
  external double max_zoom;
  @Float()
  external double pixel_ratio;
  @Bool()
  external bool include_ideographs;
}

final class mln_offline_region_definition extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int type;
  external mln_offline_region_definition_data data;
}

final class mln_offline_region_definition_data extends Union {
  external mln_offline_tile_pyramid_region_definition tile_pyramid;
  external mln_offline_geometry_region_definition geometry;
}

final class mln_offline_region_info extends Struct {
  @Int64()
  external int id;
  external mln_offline_region_definition definition;
  external Pointer<Uint8> metadata;
  @Size()
  external int metadata_size;
}

final class mln_offline_region_status extends Struct {
  @Uint32()
  external int download_state;
  @Uint64()
  external int completed_resource_count;
  @Uint64()
  external int completed_resource_size;
  @Uint64()
  external int completed_tile_count;
  @Uint64()
  external int required_tile_count;
  @Uint64()
  external int completed_tile_size;
  @Uint64()
  external int required_resource_count;
  @Bool()
  external bool required_resource_count_is_precise;
  @Bool()
  external bool complete;
}

final class mln_offline_tile_pyramid_region_definition extends Struct {
  external mln_buffer_view style_url;
  external mln_lat_lng_bounds bounds;
  @Double()
  external double min_zoom;
  @Double()
  external double max_zoom;
  @Float()
  external double pixel_ratio;
  @Bool()
  external bool include_ideographs;
}

final class mln_opengl_borrowed_texture extends Struct {
  @Uint32()
  external int texture;
}

final class mln_opengl_borrowed_texture_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_logical_extent extent;
  @Uint32()
  external int physical_width;
  @Uint32()
  external int physical_height;
  external mln_opengl_context_descriptor context;
  external Pointer<mln_opengl_borrowed_texture> textures;
  @Size()
  external int texture_count;
  @Uint32()
  external int target;
}

final class mln_opengl_context_descriptor extends Struct {
  @Uint32()
  external int platform;
  @Uint32()
  external int ownership;
  external mln_opengl_context_descriptor_data data;
}

final class mln_opengl_context_descriptor_data extends Union {
  external mln_wgl_context_descriptor wgl;
  external mln_egl_context_descriptor egl;
  external mln_webgl_context_descriptor webgl;
}

final class mln_opengl_owned_texture_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_logical_extent extent;
  external mln_opengl_context_descriptor context;
}

final class mln_opengl_surface_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_logical_extent extent;
  external mln_opengl_context_descriptor context;
  external Pointer<Void> surface;
}

final class mln_opengl_texture_frame extends Struct {
  @Uint32()
  external int size;
  @Uint64()
  external int generation;
  @Uint32()
  external int width;
  @Uint32()
  external int height;
  @Double()
  external double scale_factor;
  @Uint64()
  external int frame_id;
  @Uint32()
  external int slot;
  @Uint32()
  external int texture;
  @Uint32()
  external int target;
  @Uint32()
  external int internal_format;
  @Uint32()
  external int format;
  @Uint32()
  external int type;
}

final class mln_premultiplied_rgba8_image extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int width;
  @Uint32()
  external int height;
  @Uint32()
  external int stride;
  external Pointer<Uint8> pixels;
  @Size()
  external int byte_length;
}

final class mln_projected_meters extends Struct {
  @Double()
  external double northing;
  @Double()
  external double easting;
}

final class mln_projection_mode extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  @Bool()
  external bool axonometric;
  @Double()
  external double x_skew;
  @Double()
  external double y_skew;
}

final class mln_quaternion extends Struct {
  @Double()
  external double x;
  @Double()
  external double y;
  @Double()
  external double z;
  @Double()
  external double w;
}

final class mln_queue_lock extends Struct {
  external mln_queue_lock_callback lock;
  external mln_queue_lock_callback unlock;
  external Pointer<Void> user_data;
  external mln_user_data_release release_user_data;
}

final class mln_render_abandon_result extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int disposition;
  @Uint32()
  external int quarantined_resource_count;
  @Uint32()
  external int reserved;
}

final class mln_render_frame_batch_view extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int result_size;
  external Pointer<mln_render_frame_result> results;
  @Size()
  external int result_count;
}

final class mln_render_frame_result extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int disposition;
  @Uint64()
  external int token;
  @Uint64()
  external int map_update_generation;
  @Uint64()
  external int generation;
  @Uint64()
  external int frame_generation;
  @Bool()
  external bool needs_repaint;
  external mln_map_view view;
}

final class mln_render_session_attach_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int driver;
  @Uint32()
  external int requested_texture_ring_depth;
  @Uint32()
  external int producer_sync_kind;
  external mln_wake frame_wake;
  external mln_wake driver_work_wake;
  external mln_queue_lock queue_lock;
}

final class mln_render_session_capabilities extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int driver;
  @Uint32()
  external int texture_ring_depth;
  @Uint32()
  external int flags;
}

final class mln_render_session_snapshot extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int state;
  external mln_logical_extent extent;
  @Uint64()
  external int generation;
  @Uint64()
  external int map_update_generation;
  @Uint64()
  external int rendered_update_generation;
  @Uint64()
  external int frame_generation;
  @Uint32()
  external int acquired_frame_count;
  @Bool()
  external bool pending_changes;
}

final class mln_rendered_feature extends Struct {
  @Uint32()
  external int fields;
  external mln_buffer_view feature;
  external mln_buffer_view layer_id;
  external mln_buffer_view source_id;
  external mln_buffer_view source_layer_id;
  external mln_buffer_view state;
}

final class mln_rendered_feature_query_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external Pointer<mln_buffer_view> layer_ids;
  @Size()
  external int layer_id_count;
  external mln_buffer_view filter;
}

final class mln_rendered_query_geometry extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int type;
  external mln_rendered_query_geometry_data data;
}

final class mln_rendered_query_geometry_data extends Union {
  external mln_screen_point point;
  external mln_screen_box box;
  external mln_screen_line_string line_string;
}

final class mln_rendering_stats extends Struct {
  @Double()
  external double encoding_time;
  @Double()
  external double rendering_time;
  @Int64()
  external int frame_count;
  @Int64()
  external int draw_call_count;
  @Int64()
  external int total_draw_call_count;
}

final class mln_resource_provider extends Struct {
  @Uint32()
  external int size;
  external mln_resource_provider_callback callback;
  external Pointer<Void> user_data;
  external mln_user_data_release release_user_data;
  @Uint64()
  external int queue;
  external Pointer<mln_resource_route> routes;
  @Size()
  external int route_count;
}

final class mln_resource_range extends Struct {
  @Uint64()
  external int start;
  @Uint64()
  external int end;
}

final class mln_resource_request_cancel_delivery extends Struct {
  @Uint64()
  external int handle;
}

final class mln_resource_request_cancel_handler extends Struct {
  @Uint32()
  external int size;
  external mln_resource_request_cancel_callback callback;
  external Pointer<Void> user_data;
  external mln_user_data_release release_user_data;
  @Uint64()
  external int queue;
}

final class mln_resource_request_delivery extends Struct {
  external Pointer<mln_resource_request_info> request;
  @Uint64()
  external int handle;
}

final class mln_resource_request_info extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external Pointer<Char> requested_url;
  external Pointer<Char> resolved_url;
  @Uint32()
  external int kind;
  @Uint32()
  external int loading_method;
  @Uint32()
  external int priority;
  @Uint32()
  external int usage;
  @Uint32()
  external int storage_policy;
  external mln_resource_range range;
  @Int64()
  external int prior_modified_unix_ms;
  @Int64()
  external int prior_expires_unix_ms;
  external Pointer<Char> prior_etag;
  external Pointer<Uint8> prior_data;
  @Size()
  external int prior_data_size;
}

final class mln_resource_response extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  @Uint32()
  external int status;
  @Uint32()
  external int error_reason;
  external Pointer<Uint8> bytes;
  @Size()
  external int byte_count;
  external mln_buffer_view error_message;
  @Bool()
  external bool must_revalidate;
  @Int64()
  external int modified_unix_ms;
  @Int64()
  external int expires_unix_ms;
  external mln_buffer_view etag;
  @Int64()
  external int retry_after_unix_ms;
}

final class mln_resource_route extends Struct {
  @Uint32()
  external int kinds;
  @Uint32()
  external int flags;
  external mln_buffer_view url;
}

final class mln_resource_transform extends Struct {
  @Uint32()
  external int size;
  external mln_resource_transform_callback callback;
  external Pointer<Void> user_data;
  external mln_user_data_release release_user_data;
  @Uint64()
  external int queue;
}

final class mln_resource_transform_delivery extends Struct {
  @Uint32()
  external int kind;
  external Pointer<Char> url;
  @Uint64()
  external int request;
}

final class mln_runtime_event extends Struct {
  @Uint32()
  external int type;
  @Uint32()
  external int source_type;
  @Uint64()
  external int source;
  @Uint64()
  external int generation;
  @Int32()
  external int code;
  @Uint32()
  external int payload_type;
  external mln_buffer_view message;
  external mln_runtime_event_payload payload;
}

final class mln_runtime_event_delivery extends Struct {
  external Pointer<mln_event_batch_view> events;
}

final class mln_runtime_event_handler extends Struct {
  @Uint32()
  external int size;
  external mln_runtime_event_callback callback;
  external Pointer<Void> user_data;
  external mln_user_data_release release_user_data;
  @Uint64()
  external int queue;
}

final class mln_runtime_event_offline_region_response_error extends Struct {
  @Int64()
  external int region_id;
  @Uint32()
  external int reason;
}

final class mln_runtime_event_offline_region_status extends Struct {
  @Int64()
  external int region_id;
  external mln_offline_region_status status;
}

final class mln_runtime_event_offline_region_tile_count_limit extends Struct {
  @Int64()
  external int region_id;
  @Uint64()
  external int limit;
}

final class mln_runtime_event_payload extends Union {
  external mln_runtime_event_render_frame render_frame;
  external mln_runtime_event_render_map render_map;
  external mln_runtime_event_tile_action tile_action;
  external mln_runtime_event_offline_region_status offline_region_status;
  external mln_runtime_event_offline_region_response_error
  offline_region_response_error;
  external mln_runtime_event_offline_region_tile_count_limit
  offline_region_tile_count_limit;
}

final class mln_runtime_event_render_frame extends Struct {
  @Uint32()
  external int mode;
  @Bool()
  external bool needs_repaint;
  @Bool()
  external bool placement_changed;
  external mln_rendering_stats stats;
}

final class mln_runtime_event_render_map extends Struct {
  @Uint32()
  external int mode;
}

final class mln_runtime_event_tile_action extends Struct {
  @Uint32()
  external int operation;
  external mln_tile_id tile_id;
}

final class mln_runtime_options extends Struct {
  @Uint32()
  external int size;
  external mln_buffer_view asset_path;
  external mln_buffer_view cache_path;
}

final class mln_screen_box extends Struct {
  external mln_screen_point min;
  external mln_screen_point max;
}

final class mln_screen_line_string extends Struct {
  external Pointer<mln_screen_point> points;
  @Size()
  external int point_count;
}

final class mln_screen_point extends Struct {
  @Double()
  external double x;
  @Double()
  external double y;
}

final class mln_source_feature extends Struct {
  @Uint32()
  external int fields;
  external mln_buffer_view feature;
  external mln_buffer_view source_layer_id;
}

final class mln_source_feature_query_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external Pointer<mln_buffer_view> source_layer_ids;
  @Size()
  external int source_layer_id_count;
  external mln_buffer_view filter;
}

final class mln_style_expression_info extends Struct {
  @Uint32()
  external int size;
  @Bool()
  external bool is_feature_constant;
  @Bool()
  external bool is_zoom_constant;
}

final class mln_style_expression_inputs extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  @Double()
  external double zoom;
  @Double()
  external double color_ramp_parameter;
  @Double()
  external double elevation;
  external mln_buffer_view feature;
  external mln_buffer_view feature_state;
  external mln_buffer_view global_state;
  external mln_canonical_tile_id canonical_tile_id;
  external Pointer<mln_buffer_view> available_images;
  @Size()
  external int available_image_count;
}

final class mln_style_expression_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external mln_style_expression_type expected_type;
}

final class mln_style_expression_type extends Struct {
  @Uint32()
  external int kind;
  @Uint32()
  external int item_kind;
  @Uint32()
  external int item_count;
}

final class mln_style_image_info extends Struct {
  @Uint32()
  external int width;
  @Uint32()
  external int height;
  @Uint32()
  external int fields;
  external mln_buffer_view pixels;
  external Pointer<mln_image_stretch> stretch_x;
  @Size()
  external int stretch_x_count;
  external Pointer<mln_image_stretch> stretch_y;
  @Size()
  external int stretch_y_count;
  external mln_image_content content;
  @Uint32()
  external int text_fit_width;
  @Uint32()
  external int text_fit_height;
  @Float()
  external double pixel_ratio;
  @Bool()
  external bool sdf;
}

final class mln_style_image_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external Pointer<mln_image_stretch> stretch_x;
  @Size()
  external int stretch_x_count;
  external Pointer<mln_image_stretch> stretch_y;
  @Size()
  external int stretch_y_count;
  external mln_image_content content;
  @Uint32()
  external int text_fit_width;
  @Uint32()
  external int text_fit_height;
  @Float()
  external double pixel_ratio;
  @Bool()
  external bool sdf;
}

final class mln_style_layer_info extends Struct {
  external mln_buffer_view id;
  external mln_buffer_view type;
  external mln_buffer_view source_id;
  external mln_buffer_view source_layer;
  @Double()
  external double min_zoom;
  @Double()
  external double max_zoom;
  @Uint32()
  external int visibility;
}

final class mln_style_source_info extends Struct {
  external mln_buffer_view id;
  @Uint32()
  external int type;
  @Uint32()
  external int fields;
  @Bool()
  external bool is_volatile;
  external mln_buffer_view attribution;
  external mln_buffer_view url;
  external mln_style_source_tile_info tilejson;
  external mln_lat_lng_bounds bounds;
  @Uint32()
  external int tile_size;
  @Uint32()
  external int vector_encoding;
  @Uint32()
  external int raster_encoding;
}

final class mln_style_source_tile_info extends Struct {
  external Pointer<mln_buffer_view> tile_urls;
  @Size()
  external int tile_url_count;
  @Double()
  external double min_zoom;
  @Double()
  external double max_zoom;
  @Uint32()
  external int scheme;
}

final class mln_style_transition_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  @Double()
  external double duration_ms;
  @Double()
  external double delay_ms;
  @Bool()
  external bool enable_placement_transitions;
}

final class mln_texture_image_info extends Struct {
  @Uint32()
  external int width;
  @Uint32()
  external int height;
  @Uint32()
  external int stride;
  @Size()
  external int byte_length;
}

final class mln_texture_readback_result extends Struct {
  external mln_buffer_view data;
  external mln_texture_image_info info;
}

final class mln_tile_id extends Struct {
  @Uint32()
  external int overscaled_z;
  @Int32()
  external int wrap;
  @Uint32()
  external int canonical_z;
  @Uint32()
  external int canonical_x;
  @Uint32()
  external int canonical_y;
}

final class mln_unit_bezier extends Struct {
  @Double()
  external double x1;
  @Double()
  external double y1;
  @Double()
  external double x2;
  @Double()
  external double y2;
}

final class mln_vec3 extends Struct {
  @Double()
  external double x;
  @Double()
  external double y;
  @Double()
  external double z;
}

final class mln_vulkan_borrowed_texture extends Struct {
  @Uint64()
  external int image;
  @Uint64()
  external int image_view;
}

final class mln_vulkan_borrowed_texture_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_logical_extent extent;
  @Uint32()
  external int physical_width;
  @Uint32()
  external int physical_height;
  external mln_vulkan_context_descriptor context;
  external Pointer<mln_vulkan_borrowed_texture> textures;
  @Size()
  external int texture_count;
  @Uint32()
  external int format;
  @Uint32()
  external int initial_layout;
  @Uint32()
  external int final_layout;
}

final class mln_vulkan_context_descriptor extends Struct {
  external Pointer<Void> instance;
  external Pointer<Void> physical_device;
  external Pointer<Void> device;
  external Pointer<Void> graphics_queue;
  @Uint32()
  external int graphics_queue_family_index;
  external Pointer<Void> get_instance_proc_addr;
  external Pointer<Void> get_device_proc_addr;
}

final class mln_vulkan_owned_texture_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_logical_extent extent;
  external mln_vulkan_context_descriptor context;
}

final class mln_vulkan_surface_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_logical_extent extent;
  external mln_vulkan_context_descriptor context;
  @Uint64()
  external int surface;
}

final class mln_vulkan_texture_frame extends Struct {
  @Uint32()
  external int size;
  @Uint64()
  external int generation;
  @Uint32()
  external int width;
  @Uint32()
  external int height;
  @Double()
  external double scale_factor;
  @Uint64()
  external int frame_id;
  @Uint32()
  external int slot;
  @Uint64()
  external int image;
  @Uint64()
  external int image_view;
  external Pointer<Void> device;
  @Uint32()
  external int format;
  @Uint32()
  external int layout;
}

final class mln_wake extends Struct {
  external mln_wake_callback callback;
  external Pointer<Void> user_data;
  external mln_user_data_release release_user_data;
}

final class mln_webgl_context_descriptor extends Struct {
  @Uint32()
  external int kind;
  @Int32()
  external int context;
  external mln_buffer_view canvas_selector;
}

final class mln_webgpu_borrowed_texture extends Struct {
  external Pointer<Void> texture;
  external Pointer<Void> texture_view;
}

final class mln_webgpu_borrowed_texture_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_logical_extent extent;
  @Uint32()
  external int physical_width;
  @Uint32()
  external int physical_height;
  external mln_webgpu_context_descriptor context;
  external Pointer<mln_webgpu_borrowed_texture> textures;
  @Size()
  external int texture_count;
  @Uint32()
  external int format;
}

final class mln_webgpu_context_descriptor extends Struct {
  external Pointer<Void> instance;
  external Pointer<Void> device;
  external Pointer<Void> queue;
}

final class mln_webgpu_owned_texture_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_logical_extent extent;
  external mln_webgpu_context_descriptor context;
}

final class mln_webgpu_surface_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_logical_extent extent;
  external mln_webgpu_context_descriptor context;
  external Pointer<Void> surface;
  @Uint32()
  external int format;
}

final class mln_webgpu_texture_frame extends Struct {
  @Uint32()
  external int size;
  @Uint64()
  external int generation;
  @Uint32()
  external int width;
  @Uint32()
  external int height;
  @Double()
  external double scale_factor;
  @Uint64()
  external int frame_id;
  @Uint32()
  external int slot;
  external Pointer<Void> texture;
  external Pointer<Void> texture_view;
  external Pointer<Void> device;
  @Uint32()
  external int format;
}

final class mln_wgl_context_descriptor extends Struct {
  external Pointer<Void> device_context;
  external Pointer<Void> share_context;
  external Pointer<Void> get_proc_address;
}

// mln_ambient_cache_operation
const MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE = 1;
const MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE = 2;
const MLN_AMBIENT_CACHE_OPERATION_INVALIDATE = 3;
const MLN_AMBIENT_CACHE_OPERATION_CLEAR = 4;

// mln_animation_option_field
const MLN_ANIMATION_OPTION_DURATION = 1;
const MLN_ANIMATION_OPTION_VELOCITY = 2;
const MLN_ANIMATION_OPTION_MIN_ZOOM = 4;
const MLN_ANIMATION_OPTION_EASING = 8;
const MLN_ANIMATION_OPTION_TRANSITION_ID = 16;

// mln_bound_option_field
const MLN_BOUND_OPTION_BOUNDS = 1;
const MLN_BOUND_OPTION_MIN_ZOOM = 2;
const MLN_BOUND_OPTION_MAX_ZOOM = 4;
const MLN_BOUND_OPTION_MIN_PITCH = 8;
const MLN_BOUND_OPTION_MAX_PITCH = 16;
const MLN_BOUND_OPTION_UNBOUNDED = 32;

// mln_camera_change_mode
const MLN_CAMERA_CHANGE_MODE_IMMEDIATE = 0;
const MLN_CAMERA_CHANGE_MODE_ANIMATED = 1;

// mln_camera_delta_field
const MLN_CAMERA_DELTA_OFFSET = 1;
const MLN_CAMERA_DELTA_SCALE = 2;
const MLN_CAMERA_DELTA_BEARING = 4;
const MLN_CAMERA_DELTA_PITCH = 8;
const MLN_CAMERA_DELTA_ANCHOR = 16;

// mln_camera_fit_option_field
const MLN_CAMERA_FIT_OPTION_PADDING = 1;
const MLN_CAMERA_FIT_OPTION_BEARING = 2;
const MLN_CAMERA_FIT_OPTION_PITCH = 4;

// mln_camera_option_field
const MLN_CAMERA_OPTION_CENTER = 1;
const MLN_CAMERA_OPTION_ZOOM = 2;
const MLN_CAMERA_OPTION_BEARING = 4;
const MLN_CAMERA_OPTION_PITCH = 8;
const MLN_CAMERA_OPTION_CENTER_ALTITUDE = 16;
const MLN_CAMERA_OPTION_PADDING = 32;
const MLN_CAMERA_OPTION_ANCHOR = 64;
const MLN_CAMERA_OPTION_ROLL = 128;
const MLN_CAMERA_OPTION_FOV = 256;

// mln_camera_transition_outcome
const MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED = 0;
const MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED = 1;
const MLN_CAMERA_TRANSITION_OUTCOME_CLOSED = 2;

// mln_camera_update_mode
const MLN_CAMERA_UPDATE_MODE_JUMP = 0;
const MLN_CAMERA_UPDATE_MODE_EASE = 1;
const MLN_CAMERA_UPDATE_MODE_FLY = 2;

// mln_command_disposition
const MLN_COMMAND_DISPOSITION_COMMITTED = 0;
const MLN_COMMAND_DISPOSITION_FAILED = 1;
const MLN_COMMAND_DISPOSITION_CANCELLED = 2;

// mln_constrain_mode
const MLN_CONSTRAIN_MODE_NONE = 0;
const MLN_CONSTRAIN_MODE_HEIGHT_ONLY = 1;
const MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT = 2;
const MLN_CONSTRAIN_MODE_SCREEN = 3;

// mln_custom_geometry_source_option_field
const MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM = 1;
const MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM = 2;
const MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE = 4;
const MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE = 8;
const MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER = 16;
const MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP = 32;
const MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP = 64;

// mln_custom_mvt_vector_source_option_field
const MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM = 1;
const MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM = 2;

// mln_delivery_type
const MLN_DELIVERY_TYPE_COMPLETION = 1;
const MLN_DELIVERY_TYPE_LOG_RECORD = 2;
const MLN_DELIVERY_TYPE_RESOURCE_REQUEST = 3;
const MLN_DELIVERY_TYPE_RETIRED = 4;
const MLN_DELIVERY_TYPE_RESOURCE_REQUEST_CANCEL = 5;
const MLN_DELIVERY_TYPE_CAMERA_TRANSITION_END = 6;
const MLN_DELIVERY_TYPE_CUSTOM_SOURCE_FETCH_TILE = 7;
const MLN_DELIVERY_TYPE_CUSTOM_SOURCE_CANCEL_TILE = 8;
const MLN_DELIVERY_TYPE_RUNTIME_EVENTS = 9;
const MLN_DELIVERY_TYPE_RESOURCE_TRANSFORM = 10;

// mln_feature_state_selector_field
const MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID = 1;
const MLN_FEATURE_STATE_SELECTOR_FEATURE_ID = 2;
const MLN_FEATURE_STATE_SELECTOR_STATE_KEY = 4;

// mln_frame_demand_flag
const MLN_FRAME_DEMAND_IF_NEEDED = 1;
const MLN_FRAME_DEMAND_PRESENT = 2;
const MLN_FRAME_DEMAND_WAIT_FOR_UPDATE = 4;

// mln_free_camera_option_field
const MLN_FREE_CAMERA_OPTION_POSITION = 1;
const MLN_FREE_CAMERA_OPTION_ORIENTATION = 2;

// mln_geojson_source_option_field
const MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM = 1;
const MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM = 2;
const MLN_GEOJSON_SOURCE_OPTION_TOLERANCE = 4;
const MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM = 8;
const MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES = 16;
const MLN_GEOJSON_SOURCE_OPTION_BUFFER = 32;
const MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS = 64;
const MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS = 128;
const MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS = 256;
const MLN_GEOJSON_SOURCE_OPTION_CLUSTER = 512;
const MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING = 1024;

// mln_gesture_phase
const MLN_GESTURE_PHASE_NONE = 0;
const MLN_GESTURE_PHASE_BEGIN = 1;
const MLN_GESTURE_PHASE_UPDATE = 2;
const MLN_GESTURE_PHASE_END = 3;
const MLN_GESTURE_PHASE_CANCEL = 4;

// mln_gpu_sync_kind
const MLN_GPU_SYNC_CPU_COMPLETE = 0;
const MLN_GPU_SYNC_OPENGL_FENCE = 3;

// mln_log_event
const MLN_LOG_EVENT_GENERAL = 0;
const MLN_LOG_EVENT_SETUP = 1;
const MLN_LOG_EVENT_SHADER = 2;
const MLN_LOG_EVENT_PARSE_STYLE = 3;
const MLN_LOG_EVENT_PARSE_TILE = 4;
const MLN_LOG_EVENT_RENDER = 5;
const MLN_LOG_EVENT_STYLE = 6;
const MLN_LOG_EVENT_DATABASE = 7;
const MLN_LOG_EVENT_HTTP_REQUEST = 8;
const MLN_LOG_EVENT_SPRITE = 9;
const MLN_LOG_EVENT_IMAGE = 10;
const MLN_LOG_EVENT_GRAPHICS_BACKEND = 11;
const MLN_LOG_EVENT_JNI = 12;
const MLN_LOG_EVENT_ANDROID = 13;
const MLN_LOG_EVENT_CRASH = 14;
const MLN_LOG_EVENT_GLYPH = 15;
const MLN_LOG_EVENT_TIMING = 16;

// mln_log_severity
const MLN_LOG_SEVERITY_INFO = 1;
const MLN_LOG_SEVERITY_WARNING = 2;
const MLN_LOG_SEVERITY_ERROR = 3;

// mln_log_severity_mask
const MLN_LOG_SEVERITY_MASK_INFO = 2;
const MLN_LOG_SEVERITY_MASK_WARNING = 4;
const MLN_LOG_SEVERITY_MASK_ERROR = 8;
const MLN_LOG_SEVERITY_MASK_DEFAULT = 6;
const MLN_LOG_SEVERITY_MASK_ALL = 14;

// mln_map_debug_option
const MLN_MAP_DEBUG_TILE_BORDERS = 2;
const MLN_MAP_DEBUG_PARSE_STATUS = 4;
const MLN_MAP_DEBUG_TIMESTAMPS = 8;
const MLN_MAP_DEBUG_COLLISION = 16;
const MLN_MAP_DEBUG_OVERDRAW = 32;
const MLN_MAP_DEBUG_STENCIL_CLIP = 64;
const MLN_MAP_DEBUG_DEPTH_BUFFER = 128;

// mln_map_mode
const MLN_MAP_MODE_CONTINUOUS = 0;
const MLN_MAP_MODE_STATIC = 1;
const MLN_MAP_MODE_TILE = 2;

// mln_map_tile_option_field
const MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA = 1;
const MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS = 2;
const MLN_MAP_TILE_OPTION_LOD_SCALE = 4;
const MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD = 8;
const MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT = 16;
const MLN_MAP_TILE_OPTION_LOD_MODE = 32;

// mln_map_viewport_option_field
const MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION = 1;
const MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE = 2;
const MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE = 4;
const MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET = 8;

// mln_network_status
const MLN_NETWORK_STATUS_ONLINE = 1;
const MLN_NETWORK_STATUS_OFFLINE = 2;

// mln_north_orientation
const MLN_NORTH_ORIENTATION_UP = 0;
const MLN_NORTH_ORIENTATION_RIGHT = 1;
const MLN_NORTH_ORIENTATION_DOWN = 2;
const MLN_NORTH_ORIENTATION_LEFT = 3;

// mln_offline_region_definition_type
const MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID = 1;
const MLN_OFFLINE_REGION_DEFINITION_GEOMETRY = 2;

// mln_offline_region_download_state
const MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE = 0;
const MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE = 1;

// mln_opengl_client_api
const MLN_OPENGL_CLIENT_API_UNSPECIFIED = 0;
const MLN_OPENGL_CLIENT_API_GL = 1;
const MLN_OPENGL_CLIENT_API_GLES = 2;

// mln_opengl_context_ownership
const MLN_OPENGL_CONTEXT_OWNERSHIP_SHARED = 0;
const MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED = 1;

// mln_opengl_context_platform
const MLN_OPENGL_CONTEXT_PLATFORM_UNSPECIFIED = 0;
const MLN_OPENGL_CONTEXT_PLATFORM_WGL = 1;
const MLN_OPENGL_CONTEXT_PLATFORM_EGL = 2;
const MLN_OPENGL_CONTEXT_PLATFORM_WEBGL = 3;

// mln_opengl_context_provider_flag
const MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WGL = 1;
const MLN_OPENGL_CONTEXT_PROVIDER_FLAG_EGL = 2;
const MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WEBGL = 4;

// mln_projection_mode_field
const MLN_PROJECTION_MODE_AXONOMETRIC = 1;
const MLN_PROJECTION_MODE_X_SKEW = 2;
const MLN_PROJECTION_MODE_Y_SKEW = 4;

// mln_render_abandon_disposition
const MLN_RENDER_ABANDON_DISPOSITION_CLEAN = 0;
const MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED = 1;

// mln_render_backend_flag
const MLN_RENDER_BACKEND_FLAG_METAL = 1;
const MLN_RENDER_BACKEND_FLAG_VULKAN = 2;
const MLN_RENDER_BACKEND_FLAG_OPENGL = 4;
const MLN_RENDER_BACKEND_FLAG_WEBGPU = 8;

// mln_render_driver_kind
const MLN_RENDER_DRIVER_CORE_WORKER = 1;
const MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD = 2;

// mln_render_mode
const MLN_RENDER_MODE_PARTIAL = 0;
const MLN_RENDER_MODE_FULL = 1;

// mln_render_result
const MLN_RENDER_RESULT_RENDERED = 0;
const MLN_RENDER_RESULT_NO_UPDATE = 1;
const MLN_RENDER_RESULT_TARGET_NOT_READY = 3;
const MLN_RENDER_RESULT_SUPERSEDED = 4;

// mln_render_session_capability_flag
const MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION = 1;
const MLN_RENDER_SESSION_CAPABILITY_READBACK = 2;
const MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC = 4;
const MLN_RENDER_SESSION_CAPABILITY_PRESENTATION = 8;

// mln_render_session_state
const MLN_RENDER_SESSION_STATE_ATTACHING = 1;
const MLN_RENDER_SESSION_STATE_ATTACHED = 2;
const MLN_RENDER_SESSION_STATE_DETACHING = 3;
const MLN_RENDER_SESSION_STATE_DETACHED = 4;
const MLN_RENDER_SESSION_STATE_TARGET_LOST = 5;
const MLN_RENDER_SESSION_STATE_ABANDONED = 6;

// mln_rendered_feature_field
const MLN_RENDERED_FEATURE_SOURCE_ID = 1;
const MLN_RENDERED_FEATURE_SOURCE_LAYER_ID = 2;
const MLN_RENDERED_FEATURE_STATE = 4;

// mln_rendered_feature_query_option_field
const MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS = 1;
const MLN_RENDERED_FEATURE_QUERY_OPTION_FILTER = 2;

// mln_rendered_query_geometry_type
const MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT = 1;
const MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX = 2;
const MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING = 3;

// mln_resource_error_reason
const MLN_RESOURCE_ERROR_REASON_NONE = 0;
const MLN_RESOURCE_ERROR_REASON_NOT_FOUND = 1;
const MLN_RESOURCE_ERROR_REASON_SERVER = 2;
const MLN_RESOURCE_ERROR_REASON_CONNECTION = 3;
const MLN_RESOURCE_ERROR_REASON_RATE_LIMIT = 4;
const MLN_RESOURCE_ERROR_REASON_OTHER = 5;

// mln_resource_kind
const MLN_RESOURCE_KIND_UNKNOWN = 0;
const MLN_RESOURCE_KIND_STYLE = 1;
const MLN_RESOURCE_KIND_SOURCE = 2;
const MLN_RESOURCE_KIND_TILE = 3;
const MLN_RESOURCE_KIND_GLYPHS = 4;
const MLN_RESOURCE_KIND_SPRITE_IMAGE = 5;
const MLN_RESOURCE_KIND_SPRITE_JSON = 6;
const MLN_RESOURCE_KIND_IMAGE = 7;

// mln_resource_kind_mask
const MLN_RESOURCE_KIND_MASK_UNKNOWN = 1;
const MLN_RESOURCE_KIND_MASK_STYLE = 2;
const MLN_RESOURCE_KIND_MASK_SOURCE = 4;
const MLN_RESOURCE_KIND_MASK_TILE = 8;
const MLN_RESOURCE_KIND_MASK_GLYPHS = 16;
const MLN_RESOURCE_KIND_MASK_SPRITE_IMAGE = 32;
const MLN_RESOURCE_KIND_MASK_SPRITE_JSON = 64;
const MLN_RESOURCE_KIND_MASK_IMAGE = 128;
const MLN_RESOURCE_KIND_MASK_ALL = 255;

// mln_resource_loading_method
const MLN_RESOURCE_LOADING_METHOD_ALL = 0;
const MLN_RESOURCE_LOADING_METHOD_CACHE_ONLY = 1;
const MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY = 2;

// mln_resource_priority
const MLN_RESOURCE_PRIORITY_REGULAR = 0;
const MLN_RESOURCE_PRIORITY_LOW = 1;

// mln_resource_request_field
const MLN_RESOURCE_REQUEST_RANGE = 1;
const MLN_RESOURCE_REQUEST_PRIOR_MODIFIED = 2;
const MLN_RESOURCE_REQUEST_PRIOR_EXPIRES = 4;

// mln_resource_response_field
const MLN_RESOURCE_RESPONSE_MODIFIED = 1;
const MLN_RESOURCE_RESPONSE_EXPIRES = 2;
const MLN_RESOURCE_RESPONSE_RETRY_AFTER = 4;

// mln_resource_response_status
const MLN_RESOURCE_RESPONSE_STATUS_OK = 0;
const MLN_RESOURCE_RESPONSE_STATUS_ERROR = 1;
const MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT = 2;
const MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED = 3;

// mln_resource_storage_policy
const MLN_RESOURCE_STORAGE_POLICY_PERMANENT = 0;
const MLN_RESOURCE_STORAGE_POLICY_VOLATILE = 1;

// mln_resource_usage
const MLN_RESOURCE_USAGE_ONLINE = 0;
const MLN_RESOURCE_USAGE_OFFLINE = 1;

// mln_runtime_event_mask
const MLN_RUNTIME_EVENT_MASK_NONE = 0;
const MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_WILL_CHANGE = 2;
const MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_IS_CHANGING = 4;
const MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_DID_CHANGE = 8;
const MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED = 16;
const MLN_RUNTIME_EVENT_MASK_MAP_LOADING_STARTED = 32;
const MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FINISHED = 64;
const MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED = 128;
const MLN_RUNTIME_EVENT_MASK_MAP_IDLE = 256;
const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_ERROR = 1024;
const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_STARTED = 8192;
const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED = 16384;
const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_STARTED = 32768;
const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_FINISHED = 65536;
const MLN_RUNTIME_EVENT_MASK_MAP_STYLE_IMAGE_MISSING = 131072;
const MLN_RUNTIME_EVENT_MASK_MAP_TILE_ACTION = 262144;
const MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_STATUS_CHANGED = 524288;
const MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_RESPONSE_ERROR = 1048576;
const MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 2097152;
const MLN_RUNTIME_EVENT_MASK_MAP_SOURCE_CHANGED = 8388608;
const MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS = 8906238;
const MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS = 3670016;
const MLN_RUNTIME_EVENT_MASK_ALL = 12576254;

// mln_runtime_event_payload_type
const MLN_RUNTIME_EVENT_PAYLOAD_NONE = 0;
const MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME = 1;
const MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP = 2;
const MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION = 4;
const MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS = 5;
const MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR = 6;
const MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT = 7;

// mln_runtime_event_source_type
const MLN_RUNTIME_EVENT_SOURCE_RUNTIME = 0;
const MLN_RUNTIME_EVENT_SOURCE_MAP = 1;

// mln_runtime_event_type
const MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE = 1;
const MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING = 2;
const MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE = 3;
const MLN_RUNTIME_EVENT_MAP_STYLE_LOADED = 4;
const MLN_RUNTIME_EVENT_MAP_LOADING_STARTED = 5;
const MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED = 6;
const MLN_RUNTIME_EVENT_MAP_LOADING_FAILED = 7;
const MLN_RUNTIME_EVENT_MAP_IDLE = 8;
const MLN_RUNTIME_EVENT_MAP_RENDER_ERROR = 10;
const MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED = 13;
const MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED = 14;
const MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED = 15;
const MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED = 16;
const MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING = 17;
const MLN_RUNTIME_EVENT_MAP_TILE_ACTION = 18;
const MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED = 19;
const MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR = 20;
const MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 21;
const MLN_RUNTIME_EVENT_MAP_SOURCE_CHANGED = 23;

// mln_source_feature_field
const MLN_SOURCE_FEATURE_SOURCE_LAYER_ID = 1;

// mln_source_feature_query_option_field
const MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS = 1;
const MLN_SOURCE_FEATURE_QUERY_OPTION_FILTER = 2;

// mln_status
const MLN_STATUS_OK = 0;
const MLN_STATUS_INVALID_ARGUMENT = -1;
const MLN_STATUS_INVALID_STATE = -2;
const MLN_STATUS_WRONG_THREAD = -3;
const MLN_STATUS_UNSUPPORTED = -4;
const MLN_STATUS_NATIVE_ERROR = -5;
const MLN_STATUS_CANCELLED = -6;
const MLN_STATUS_BUSY = -7;
const MLN_STATUS_TARGET_LOST = -8;
const MLN_STATUS_NOT_READY = -9;
const MLN_STATUS_NOT_FOUND = -10;

// mln_style_expression_input_field
const MLN_STYLE_EXPRESSION_INPUT_ZOOM = 1;
const MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER = 2;
const MLN_STYLE_EXPRESSION_INPUT_ELEVATION = 4;
const MLN_STYLE_EXPRESSION_INPUT_FEATURE = 8;
const MLN_STYLE_EXPRESSION_INPUT_FEATURE_STATE = 16;
const MLN_STYLE_EXPRESSION_INPUT_GLOBAL_STATE = 32;
const MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID = 64;

// mln_style_expression_option_field
const MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE = 1;

// mln_style_expression_type_kind
const MLN_STYLE_EXPRESSION_TYPE_VALUE = 0;
const MLN_STYLE_EXPRESSION_TYPE_NUMBER = 1;
const MLN_STYLE_EXPRESSION_TYPE_STRING = 2;
const MLN_STYLE_EXPRESSION_TYPE_BOOLEAN = 3;
const MLN_STYLE_EXPRESSION_TYPE_COLOR = 4;
const MLN_STYLE_EXPRESSION_TYPE_OBJECT = 5;
const MLN_STYLE_EXPRESSION_TYPE_FORMATTED = 6;
const MLN_STYLE_EXPRESSION_TYPE_IMAGE = 7;
const MLN_STYLE_EXPRESSION_TYPE_PADDING = 8;
const MLN_STYLE_EXPRESSION_TYPE_VARIABLE_ANCHOR_OFFSET_COLLECTION = 9;
const MLN_STYLE_EXPRESSION_TYPE_ARRAY = 10;

// mln_style_image_info_field
const MLN_STYLE_IMAGE_INFO_CONTENT = 1;
const MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH = 2;
const MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT = 4;

// mln_style_image_option_field
const MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO = 1;
const MLN_STYLE_IMAGE_OPTION_SDF = 2;
const MLN_STYLE_IMAGE_OPTION_STRETCH_X = 4;
const MLN_STYLE_IMAGE_OPTION_STRETCH_Y = 8;
const MLN_STYLE_IMAGE_OPTION_CONTENT = 16;
const MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH = 32;
const MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT = 64;

// mln_style_image_text_fit
const MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK = 0;
const MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY = 1;
const MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL = 2;

// mln_style_layer_visibility
const MLN_STYLE_LAYER_VISIBILITY_VISIBLE = 0;
const MLN_STYLE_LAYER_VISIBILITY_NONE = 1;

// mln_style_raster_dem_encoding
const MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX = 0;
const MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM = 1;

// mln_style_source_info_field
const MLN_STYLE_SOURCE_INFO_URL = 1;
const MLN_STYLE_SOURCE_INFO_TILEJSON = 2;
const MLN_STYLE_SOURCE_INFO_BOUNDS = 4;
const MLN_STYLE_SOURCE_INFO_TILE_SIZE = 8;
const MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING = 16;
const MLN_STYLE_SOURCE_INFO_RASTER_ENCODING = 32;
const MLN_STYLE_SOURCE_INFO_ATTRIBUTION = 64;

// mln_style_source_type
const MLN_STYLE_SOURCE_TYPE_UNKNOWN = 0;
const MLN_STYLE_SOURCE_TYPE_VECTOR = 1;
const MLN_STYLE_SOURCE_TYPE_RASTER = 2;
const MLN_STYLE_SOURCE_TYPE_RASTER_DEM = 3;
const MLN_STYLE_SOURCE_TYPE_GEOJSON = 4;
const MLN_STYLE_SOURCE_TYPE_IMAGE = 5;
const MLN_STYLE_SOURCE_TYPE_VIDEO = 6;
const MLN_STYLE_SOURCE_TYPE_ANNOTATIONS = 7;
const MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR = 8;
const MLN_STYLE_SOURCE_TYPE_CUSTOM_MVT_VECTOR = 9;

// mln_style_tile_scheme
const MLN_STYLE_TILE_SCHEME_XYZ = 0;
const MLN_STYLE_TILE_SCHEME_TMS = 1;

// mln_style_transition_option_field
const MLN_STYLE_TRANSITION_OPTION_DURATION = 1;
const MLN_STYLE_TRANSITION_OPTION_DELAY = 2;
const MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS = 4;

// mln_style_vector_tile_encoding
const MLN_STYLE_VECTOR_TILE_ENCODING_MVT = 0;
const MLN_STYLE_VECTOR_TILE_ENCODING_MLT = 1;

// mln_tile_lod_mode
const MLN_TILE_LOD_MODE_DEFAULT = 0;
const MLN_TILE_LOD_MODE_DISTANCE = 1;

// mln_tile_operation
const MLN_TILE_OPERATION_REQUESTED_FROM_CACHE = 0;
const MLN_TILE_OPERATION_REQUESTED_FROM_NETWORK = 1;
const MLN_TILE_OPERATION_LOAD_FROM_NETWORK = 2;
const MLN_TILE_OPERATION_LOAD_FROM_CACHE = 3;
const MLN_TILE_OPERATION_START_PARSE = 4;
const MLN_TILE_OPERATION_END_PARSE = 5;
const MLN_TILE_OPERATION_ERROR = 6;
const MLN_TILE_OPERATION_CANCELLED = 7;
const MLN_TILE_OPERATION_NULL = 8;

// mln_url_match_flags
const MLN_URL_MATCH_EXACT = 0;
const MLN_URL_MATCH_GLOB = 1;

// mln_viewport_mode
const MLN_VIEWPORT_MODE_DEFAULT = 0;
const MLN_VIEWPORT_MODE_FLIPPED_Y = 1;

// mln_webgl_context_kind
const MLN_WEBGL_CONTEXT_EXISTING = 0;
const MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS = 1;

@Native<Int32 Function(mln_acquired_frame, Pointer<mln_diagnostic>)>()
external int mln_acquired_frame_discard(
  int frame,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_acquired_frame,
    Pointer<mln_metal_texture_frame>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_acquired_frame_get_metal_texture(
  int frame,
  Pointer<mln_metal_texture_frame> out_frame,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_acquired_frame,
    Pointer<mln_opengl_texture_frame>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_acquired_frame_get_opengl_texture(
  int frame,
  Pointer<mln_opengl_texture_frame> out_frame,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_acquired_frame,
    Pointer<mln_gpu_sync>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_acquired_frame_get_producer_sync(
  int frame,
  Pointer<mln_gpu_sync> out_sync,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_acquired_frame,
    Pointer<mln_render_frame_result>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_acquired_frame_get_result(
  int frame,
  Pointer<mln_render_frame_result> out_result,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_acquired_frame,
    Pointer<mln_vulkan_texture_frame>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_acquired_frame_get_vulkan_texture(
  int frame,
  Pointer<mln_vulkan_texture_frame> out_frame,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_acquired_frame,
    Pointer<mln_gpu_sync>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_acquired_frame_release(
  int frame,
  Pointer<mln_gpu_sync> consumer_completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_acquired_frame,
    Pointer<Pointer<Void>>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_acquired_frame_view_begin(
  int frame,
  Pointer<Pointer<Void>> out_scope,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Void Function(Pointer<Void>)>()
external void mln_acquired_frame_view_end(Pointer<Void> scope);

@Native<Int32 Function(Pointer<Void>, Pointer<Void>, Pointer<mln_diagnostic>)>()
external int mln_android_init(
  Pointer<Void> jni_env,
  Pointer<Void> context,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_animation_options Function()>()
external mln_animation_options mln_animation_options_default();

@Native<
  Int32 Function(
    Pointer<Void>,
    Int64,
    Pointer<mln_wake>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_binding_dart_wake_create(
  Pointer<Void> post_cobject,
  int port,
  Pointer<mln_wake> out_wake,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Void Function(Pointer<Void>)>()
external void mln_binding_owner_finalize(Pointer<Void> token);

@Native<Pointer<Void> Function(Uint64)>()
external Pointer<Void> mln_binding_owner_token_create(int handle);

@Native<Void Function(Pointer<Void>)>()
external void mln_binding_owner_token_destroy(Pointer<Void> token);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_render_abandon_result>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_binding_render_session_abandon_at_exit(
  int session,
  Pointer<mln_render_abandon_result> out_result,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_bound_options Function()>()
external mln_bound_options mln_bound_options_default();

@Native<Uint32 Function()>()
external int mln_c_version();

@Native<mln_camera_delta Function()>()
external mln_camera_delta mln_camera_delta_default();

@Native<mln_camera_fit_options Function()>()
external mln_camera_fit_options mln_camera_fit_options_default();

@Native<mln_camera_options Function()>()
external mln_camera_options mln_camera_options_default();

@Native<mln_camera_update Function()>()
external mln_camera_update mln_camera_update_default();

@Native<mln_custom_geometry_source_options Function()>()
external mln_custom_geometry_source_options
mln_custom_geometry_source_options_default();

@Native<mln_custom_mvt_vector_source_options Function()>()
external mln_custom_mvt_vector_source_options
mln_custom_mvt_vector_source_options_default();

@Native<Void Function(mln_delivery_batch)>()
external void mln_delivery_batch_release(int batch);

@Native<Int32 Function(mln_delivery_batch, Size, Pointer<mln_diagnostic>)>()
external int mln_delivery_batch_take(
  int batch,
  int index,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    Pointer<mln_delivery_queue_options>,
    Pointer<mln_delivery_queue>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_delivery_queue_create(
  Pointer<mln_delivery_queue_options> options,
  Pointer<mln_delivery_queue> out_queue,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_delivery_queue,
    Pointer<mln_delivery_batch>,
    Pointer<mln_delivery_batch_view>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_delivery_queue_drain(
  int queue,
  Pointer<mln_delivery_batch> out_batch,
  Pointer<mln_delivery_batch_view> out_view,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Void Function(mln_delivery_queue)>()
external void mln_delivery_queue_release(int queue);

@Native<mln_frame_demand Function()>()
external mln_frame_demand mln_frame_demand_default();

@Native<mln_free_camera_options Function()>()
external mln_free_camera_options mln_free_camera_options_default();

@Native<
  Int32 Function(
    mln_buffer_view,
    Pointer<mln_geojson_source_options>,
    Pointer<mln_geojson_source_data>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_geojson_source_data_create(
  mln_buffer_view data,
  Pointer<mln_geojson_source_options> options,
  Pointer<mln_geojson_source_data> out_data,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Void Function(mln_geojson_source_data)>()
external void mln_geojson_source_data_release(int data);

@Native<mln_geojson_source_options Function()>()
external mln_geojson_source_options mln_geojson_source_options_default();

@Native<mln_gpu_sync Function()>()
external mln_gpu_sync mln_gpu_sync_default();

@Native<mln_http_header_rule Function()>()
external mln_http_header_rule mln_http_header_rule_default();

@Native<
  Int32 Function(
    Pointer<mln_http_header_transform_response>,
    mln_buffer_view,
    mln_buffer_view,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_http_header_transform_response_set_header(
  Pointer<mln_http_header_transform_response> response,
  mln_buffer_view name,
  mln_buffer_view value,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_projected_meters,
    Pointer<mln_lat_lng>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_lat_lng_for_projected_meters(
  mln_projected_meters meters,
  Pointer<mln_lat_lng> out_coordinate,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Int32 Function(Pointer<mln_diagnostic>)>()
external int mln_log_clear_callback(Pointer<mln_diagnostic> out_diagnostic);

@Native<Int32 Function(Uint32, Pointer<mln_diagnostic>)>()
external int mln_log_set_async_severity_mask(
  int mask,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Int32 Function(Pointer<mln_log_handler>, Pointer<mln_diagnostic>)>()
external int mln_log_set_callback(
  Pointer<mln_log_handler> handler,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_logical_extent Function()>()
external mln_logical_extent mln_logical_extent_default();

@Native<
  Int32 Function(
    mln_logical_extent,
    Pointer<Uint32>,
    Pointer<Uint32>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_logical_extent_physical_size(
  mln_logical_extent extent,
  Pointer<Uint32> out_width,
  Pointer<Uint32> out_height,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_custom_geometry_source_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_add_custom_geometry_source(
  int map,
  mln_buffer_view source_id,
  Pointer<mln_custom_geometry_source_options> options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_custom_mvt_vector_source_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_add_custom_mvt_vector_source(
  int map,
  mln_buffer_view source_id,
  Pointer<mln_custom_mvt_vector_source_options> options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_geojson_source_data,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_add_geojson_source_data(
  int map,
  mln_buffer_view source_id,
  int data,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_buffer_view,
    Pointer<mln_geojson_source_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_add_geojson_source_url(
  int map,
  mln_buffer_view source_id,
  mln_buffer_view url,
  Pointer<mln_geojson_source_options> options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_lat_lng>,
    Size,
    Pointer<mln_premultiplied_rgba8_image>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_add_image_source_image(
  int map,
  mln_buffer_view source_id,
  Pointer<mln_lat_lng> coordinates,
  int coordinate_count,
  Pointer<mln_premultiplied_rgba8_image> image,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_lat_lng>,
    Size,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_add_image_source_url(
  int map,
  mln_buffer_view source_id,
  Pointer<mln_lat_lng> coordinates,
  int coordinate_count,
  mln_buffer_view url,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_add_style_layer_json(
  int map,
  mln_buffer_view layer_json,
  mln_buffer_view before_layer_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_add_style_source_json(
  int map,
  mln_buffer_view source_id,
  mln_buffer_view source_json,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_camera_delta>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_apply_camera_delta(
  int map,
  Pointer<mln_camera_delta> delta,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_metal_borrowed_texture_descriptor>,
    Pointer<mln_render_session_attach_options>,
    Pointer<mln_render_session>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_attach_metal_borrowed_texture(
  int map,
  Pointer<mln_metal_borrowed_texture_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_metal_owned_texture_descriptor>,
    Pointer<mln_render_session_attach_options>,
    Pointer<mln_render_session>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_attach_metal_owned_texture(
  int map,
  Pointer<mln_metal_owned_texture_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_metal_surface_descriptor>,
    Pointer<mln_render_session_attach_options>,
    Pointer<mln_render_session>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_attach_metal_surface(
  int map,
  Pointer<mln_metal_surface_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_opengl_borrowed_texture_descriptor>,
    Pointer<mln_render_session_attach_options>,
    Pointer<mln_render_session>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_attach_opengl_borrowed_texture(
  int map,
  Pointer<mln_opengl_borrowed_texture_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_opengl_owned_texture_descriptor>,
    Pointer<mln_render_session_attach_options>,
    Pointer<mln_render_session>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_attach_opengl_owned_texture(
  int map,
  Pointer<mln_opengl_owned_texture_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_opengl_surface_descriptor>,
    Pointer<mln_render_session_attach_options>,
    Pointer<mln_render_session>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_attach_opengl_surface(
  int map,
  Pointer<mln_opengl_surface_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_vulkan_borrowed_texture_descriptor>,
    Pointer<mln_render_session_attach_options>,
    Pointer<mln_render_session>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_attach_vulkan_borrowed_texture(
  int map,
  Pointer<mln_vulkan_borrowed_texture_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_vulkan_owned_texture_descriptor>,
    Pointer<mln_render_session_attach_options>,
    Pointer<mln_render_session>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_attach_vulkan_owned_texture(
  int map,
  Pointer<mln_vulkan_owned_texture_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_vulkan_surface_descriptor>,
    Pointer<mln_render_session_attach_options>,
    Pointer<mln_render_session>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_attach_vulkan_surface(
  int map,
  Pointer<mln_vulkan_surface_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_begin_command_group(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_camera_fit_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_camera_for_geometry(
  int map,
  mln_buffer_view geometry,
  Pointer<mln_camera_fit_options> fit_options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_lat_lng_bounds,
    Pointer<mln_camera_fit_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_camera_for_lat_lng_bounds(
  int map,
  mln_lat_lng_bounds bounds,
  Pointer<mln_camera_fit_options> fit_options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_lat_lng>,
    Size,
    Pointer<mln_camera_fit_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_camera_for_lat_lngs(
  int map,
  Pointer<mln_lat_lng> coordinates,
  int coordinate_count,
  Pointer<mln_camera_fit_options> fit_options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Uint64,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_cancel_camera_transition(
  int map,
  int transition_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_cancel_transitions(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Int32 Function(mln_map, Pointer<mln_diagnostic>)>()
external int mln_map_dispose(int map, Pointer<mln_diagnostic> out_diagnostic);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_dump_debug_logs(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_end_command_group(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_get_camera(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_feature_state_selector>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_get_feature_state(
  int map,
  Pointer<mln_feature_state_selector> selector,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_get_global_state(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_get_image_source_coordinates(
  int map,
  mln_buffer_view source_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_get_loaded_style_json(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_map_snapshot>, Pointer<mln_diagnostic>)
>()
external int mln_map_get_snapshot(
  int map,
  Pointer<mln_map_snapshot> out_snapshot,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_get_style_image(
  int map,
  mln_buffer_view image_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_get_style_layer(
  int map,
  mln_buffer_view layer_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_get_style_layer_json(
  int map,
  mln_buffer_view layer_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_get_style_layer_property(
  int map,
  mln_buffer_view layer_id,
  mln_buffer_view property_name,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_get_style_light_property(
  int map,
  mln_buffer_view property_name,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_get_style_source(
  int map,
  mln_buffer_view source_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_get_style_transition_options(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_get_style_url(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_lat_lng_bounds,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_invalidate_custom_geometry_source_region(
  int map,
  mln_buffer_view source_id,
  mln_lat_lng_bounds bounds,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_canonical_tile_id,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_invalidate_custom_geometry_source_tile(
  int map,
  mln_buffer_view source_id,
  mln_canonical_tile_id tile_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_canonical_tile_id,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_invalidate_custom_mvt_vector_source_tile(
  int map,
  mln_buffer_view source_id,
  mln_canonical_tile_id tile_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_camera_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_lat_lng_bounds_for_camera(
  int map,
  Pointer<mln_camera_options> camera,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_camera_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_lat_lng_bounds_for_camera_unwrapped(
  int map,
  Pointer<mln_camera_options> camera,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_list_style_layers(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_list_style_sources(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_move_style_layer(
  int map,
  mln_buffer_view layer_id,
  mln_buffer_view before_layer_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_map_options Function()>()
external mln_map_options mln_map_options_default();

@Native<
  Int32 Function(
    Pointer<mln_map_view>,
    Pointer<mln_map_projection>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_projection_create(
  Pointer<mln_map_view> view,
  Pointer<mln_map_projection> out_projection,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map_projection,
    Pointer<mln_camera_options>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_projection_get_camera(
  int projection,
  Pointer<mln_camera_options> out_camera,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map_projection,
    mln_screen_point,
    Pointer<mln_lat_lng>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_projection_lat_lng_for_pixel(
  int projection,
  mln_screen_point point,
  Pointer<mln_lat_lng> out_coordinate,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map_projection,
    mln_screen_point,
    Pointer<mln_lat_lng>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_projection_lat_lng_for_pixel_unwrapped(
  int projection,
  mln_screen_point point,
  Pointer<mln_lat_lng> out_coordinate,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map_projection,
    Pointer<mln_screen_point>,
    Size,
    Pointer<mln_lat_lng>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_projection_lat_lngs_for_pixels(
  int projection,
  Pointer<mln_screen_point> points,
  int point_count,
  Pointer<mln_lat_lng> out_coordinates,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map_projection,
    Pointer<mln_screen_point>,
    Size,
    Pointer<mln_lat_lng>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_projection_lat_lngs_for_pixels_unwrapped(
  int projection,
  Pointer<mln_screen_point> points,
  int point_count,
  Pointer<mln_lat_lng> out_coordinates,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map_projection,
    Double,
    Pointer<Double>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_projection_meters_per_pixel_at_latitude(
  int projection,
  double latitude,
  Pointer<Double> out_meters_per_pixel,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map_projection,
    mln_lat_lng,
    Pointer<mln_screen_point>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_projection_pixel_for_lat_lng(
  int projection,
  mln_lat_lng coordinate,
  Pointer<mln_screen_point> out_point,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map_projection,
    Pointer<mln_lat_lng>,
    Size,
    Pointer<mln_screen_point>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_projection_pixels_for_lat_lngs(
  int projection,
  Pointer<mln_lat_lng> coordinates,
  int coordinate_count,
  Pointer<mln_screen_point> out_points,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Void Function(mln_map_projection)>()
external void mln_map_projection_release(int projection);

@Native<
  Int32 Function(
    mln_map_projection,
    Pointer<mln_camera_options>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_projection_set_camera(
  int projection,
  Pointer<mln_camera_options> camera,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_release(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_feature_state_selector>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_remove_feature_state(
  int map,
  Pointer<mln_feature_state_selector> selector,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_remove_style_image(
  int map,
  mln_buffer_view image_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_remove_style_layer(
  int map,
  mln_buffer_view layer_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_remove_style_source(
  int map,
  mln_buffer_view source_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_request_repaint(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_request_still_image(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_logical_extent,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_resize(
  int map,
  mln_logical_extent extent,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_bound_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_bounds(
  int map,
  Pointer<mln_bound_options> options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_canonical_tile_id,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_custom_geometry_source_tile_data(
  int map,
  mln_buffer_view source_id,
  mln_canonical_tile_id tile_id,
  mln_buffer_view data,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_canonical_tile_id,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_custom_geometry_source_tile_error(
  int map,
  mln_buffer_view source_id,
  mln_canonical_tile_id tile_id,
  mln_buffer_view message,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_canonical_tile_id,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_custom_mvt_vector_source_tile_data(
  int map,
  mln_buffer_view source_id,
  mln_canonical_tile_id tile_id,
  mln_buffer_view data,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_canonical_tile_id,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_custom_mvt_vector_source_tile_error(
  int map,
  mln_buffer_view source_id,
  mln_canonical_tile_id tile_id,
  mln_buffer_view message,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Uint32,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_debug_options(
  int map,
  int options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Uint64,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_event_mask(
  int map,
  int mask,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_feature_state_selector>,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_feature_state(
  int map,
  Pointer<mln_feature_state_selector> selector,
  mln_buffer_view state,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_free_camera_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_free_camera_options(
  int map,
  Pointer<mln_free_camera_options> options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_geojson_source_data,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_geojson_source_data(
  int map,
  mln_buffer_view source_id,
  int data,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Bool,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_geojson_source_synchronous_tiling(
  int map,
  mln_buffer_view source_id,
  bool enabled,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_geojson_source_url(
  int map,
  mln_buffer_view source_id,
  mln_buffer_view url,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_global_state_property(
  int map,
  mln_buffer_view property_name,
  mln_buffer_view value,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_lat_lng>,
    Size,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_image_source_coordinates(
  int map,
  mln_buffer_view source_id,
  Pointer<mln_lat_lng> coordinates,
  int coordinate_count,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_premultiplied_rgba8_image>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_image_source_image(
  int map,
  mln_buffer_view source_id,
  Pointer<mln_premultiplied_rgba8_image> image,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_image_source_url(
  int map,
  mln_buffer_view source_id,
  mln_buffer_view url,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_projection_mode>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_projection_mode(
  int map,
  Pointer<mln_projection_mode> mode,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Bool,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_rendering_stats_view_enabled(
  int map,
  bool enabled,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_premultiplied_rgba8_image>,
    Pointer<mln_style_image_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_style_image(
  int map,
  mln_buffer_view image_id,
  Pointer<mln_premultiplied_rgba8_image> image,
  Pointer<mln_style_image_options> options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_style_json(
  int map,
  mln_buffer_view json,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_buffer_view,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_style_layer_property(
  int map,
  mln_buffer_view layer_id,
  mln_buffer_view property_name,
  mln_buffer_view value,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_style_light_json(
  int map,
  mln_buffer_view light_json,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_style_light_property(
  int map,
  mln_buffer_view property_name,
  mln_buffer_view value,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Bool,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_style_source_volatile(
  int map,
  mln_buffer_view source_id,
  bool is_volatile,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_style_transition_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_style_transition_options(
  int map,
  Pointer<mln_style_transition_options> options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_style_url(
  int map,
  mln_buffer_view url,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_map_tile_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_tile_options(
  int map,
  Pointer<mln_map_tile_options> options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_map_viewport_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_viewport_options(
  int map,
  Pointer<mln_map_viewport_options> options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_map_tile_options Function()>()
external mln_map_tile_options mln_map_tile_options_default();

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_camera_update>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_update_camera(
  int map,
  Pointer<mln_camera_update> update,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_map_viewport_options Function()>()
external mln_map_viewport_options mln_map_viewport_options_default();

@Native<mln_metal_borrowed_texture_descriptor Function()>()
external mln_metal_borrowed_texture_descriptor
mln_metal_borrowed_texture_descriptor_default();

@Native<mln_metal_owned_texture_descriptor Function()>()
external mln_metal_owned_texture_descriptor
mln_metal_owned_texture_descriptor_default();

@Native<mln_metal_surface_descriptor Function()>()
external mln_metal_surface_descriptor mln_metal_surface_descriptor_default();

@Native<Int32 Function(Pointer<Uint32>, Pointer<mln_diagnostic>)>()
external int mln_network_get_status(
  Pointer<Uint32> out_status,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Int32 Function(Uint32, Pointer<mln_diagnostic>)>()
external int mln_network_set_status(
  int status,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_opengl_borrowed_texture_descriptor Function()>()
external mln_opengl_borrowed_texture_descriptor
mln_opengl_borrowed_texture_descriptor_default();

@Native<mln_opengl_owned_texture_descriptor Function()>()
external mln_opengl_owned_texture_descriptor
mln_opengl_owned_texture_descriptor_default();

@Native<Uint32 Function()>()
external int mln_opengl_supported_context_provider_mask();

@Native<mln_opengl_surface_descriptor Function()>()
external mln_opengl_surface_descriptor mln_opengl_surface_descriptor_default();

@Native<Pointer<Void> Function()>()
external Pointer<Void> mln_plugin_get_register_function_v1();

@Native<mln_premultiplied_rgba8_image Function()>()
external mln_premultiplied_rgba8_image mln_premultiplied_rgba8_image_default();

@Native<
  Int32 Function(
    mln_lat_lng,
    Pointer<mln_projected_meters>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_projected_meters_for_lat_lng(
  mln_lat_lng coordinate,
  Pointer<mln_projected_meters> out_meters,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_projection_mode Function()>()
external mln_projection_mode mln_projection_mode_default();

@Native<
  Int32 Function(
    mln_render_frame_batch,
    Pointer<mln_render_frame_batch_view>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_frame_batch_get(
  int batch,
  Pointer<mln_render_frame_batch_view> out_view,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Void Function(mln_render_frame_batch)>()
external void mln_render_frame_batch_release(int batch);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_render_abandon_result>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_abandon(
  int session,
  Pointer<mln_render_abandon_result> out_result,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_acquired_frame>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_acquire_frame(
  int session,
  Pointer<mln_acquired_frame> out_frame,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_render_session_attach_options Function()>()
external mln_render_session_attach_options
mln_render_session_attach_options_default();

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_barrier(
  int session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_clear_data(
  int session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_detach(
  int session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_dispose(
  int session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_render_frame_batch>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_drain_frame_results(
  int session,
  Pointer<mln_render_frame_batch> out_batch,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_dump_debug_logs(
  int session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_render_session_capabilities>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_get_capabilities(
  int session,
  Pointer<mln_render_session_capabilities> out_capabilities,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_render_session_snapshot>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_get_snapshot(
  int session,
  Pointer<mln_render_session_snapshot> out_snapshot,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    mln_buffer_view,
    mln_buffer_view,
    mln_buffer_view,
    mln_buffer_view,
    Pointer<mln_buffer_view>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_query_feature_extensions(
  int session,
  mln_buffer_view source_id,
  mln_buffer_view feature,
  mln_buffer_view extension,
  mln_buffer_view extension_field,
  Pointer<mln_buffer_view> arguments,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_rendered_query_geometry>,
    Pointer<mln_rendered_feature_query_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_query_rendered_features(
  int session,
  Pointer<mln_rendered_query_geometry> geometry,
  Pointer<mln_rendered_feature_query_options> options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    mln_buffer_view,
    Pointer<mln_source_feature_query_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_query_source_features(
  int session,
  mln_buffer_view source_id,
  Pointer<mln_source_feature_query_options> options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_read_texture(
  int session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_reduce_memory_use(
  int session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_frame_demand>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_request_frame(
  int session,
  Pointer<mln_frame_demand> demand,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    mln_logical_extent,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_resize(
  int session,
  mln_logical_extent extent,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Int32 Function(mln_render_session, Pointer<mln_diagnostic>)>()
external int mln_render_session_service_driver_work(
  int session,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_metal_borrowed_texture_descriptor>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_set_metal_borrowed_texture_target(
  int session,
  Pointer<mln_metal_borrowed_texture_descriptor> descriptor,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_metal_surface_descriptor>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_set_metal_surface_target(
  int session,
  Pointer<mln_metal_surface_descriptor> descriptor,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_opengl_borrowed_texture_descriptor>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_set_opengl_borrowed_texture_target(
  int session,
  Pointer<mln_opengl_borrowed_texture_descriptor> descriptor,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_opengl_surface_descriptor>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_set_opengl_surface_target(
  int session,
  Pointer<mln_opengl_surface_descriptor> descriptor,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_vulkan_borrowed_texture_descriptor>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_set_vulkan_borrowed_texture_target(
  int session,
  Pointer<mln_vulkan_borrowed_texture_descriptor> descriptor,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_vulkan_surface_descriptor>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_set_vulkan_surface_target(
  int session,
  Pointer<mln_vulkan_surface_descriptor> descriptor,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_rendered_feature_query_options Function()>()
external mln_rendered_feature_query_options
mln_rendered_feature_query_options_default();

@Native<
  Int32 Function(
    mln_resource_request,
    Pointer<mln_resource_response>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_resource_request_complete(
  int handle,
  Pointer<mln_resource_response> response,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_resource_request, Pointer<Bool>, Pointer<mln_diagnostic>)
>()
external int mln_resource_request_is_cancelled(
  int handle,
  Pointer<Bool> out_cancelled,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Int32 Function(mln_resource_request, Pointer<mln_diagnostic>)>()
external int mln_resource_request_pass_through(
  int handle,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Void Function(mln_resource_request)>()
external void mln_resource_request_release(int handle);

@Native<
  Int32 Function(
    mln_resource_request,
    Pointer<mln_resource_request_cancel_handler>,
    Pointer<Bool>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_resource_request_set_cancel_callback(
  int handle,
  Pointer<mln_resource_request_cancel_handler> handler,
  Pointer<Bool> out_cancelled,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_resource_route Function()>()
external mln_resource_route mln_resource_route_default();

@Native<
  Int32 Function(
    mln_resource_transform_request,
    mln_buffer_view,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_resource_transform_request_complete(
  int request,
  mln_buffer_view url,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Void Function(mln_resource_transform_request)>()
external void mln_resource_transform_request_release(int request);

@Native<
  Int32 Function(mln_runtime, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_runtime_clear_event_handler(
  int runtime,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_runtime, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_runtime_clear_http_header_transform(
  int runtime,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_runtime, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_runtime_clear_resource_provider(
  int runtime,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_runtime, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_runtime_clear_resource_transform(
  int runtime,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    Pointer<mln_runtime_options>,
    Pointer<mln_runtime>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_create(
  Pointer<mln_runtime_options> options,
  Pointer<mln_runtime> out_runtime,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Pointer<mln_map_options>,
    Pointer<mln_map>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_create_map(
  int runtime,
  Pointer<mln_map_options> options,
  Pointer<mln_map> out_map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Pointer<mln_offline_region_definition>,
    Pointer<Uint8>,
    Size,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_create_offline_region(
  int runtime,
  Pointer<mln_offline_region_definition> definition,
  Pointer<Uint8> metadata,
  int metadata_size,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Int64,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_delete_offline_region(
  int runtime,
  int region_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Int32 Function(mln_runtime, Pointer<mln_diagnostic>)>()
external int mln_runtime_dispose(
  int runtime,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Int32 Function(mln_runtime, Pointer<Uint64>, Pointer<mln_diagnostic>)>()
external int mln_runtime_get_event_mask(
  int runtime,
  Pointer<Uint64> out_mask,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Int64,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_get_offline_region(
  int runtime,
  int region_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Int64,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_get_offline_region_status(
  int runtime,
  int region_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Int64,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_invalidate_offline_region(
  int runtime,
  int region_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_runtime, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_runtime_list_offline_regions(
  int runtime,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_merge_offline_regions(
  int runtime,
  mln_buffer_view side_database_path,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_runtime_options Function()>()
external mln_runtime_options mln_runtime_options_default();

@Native<
  Int32 Function(mln_runtime, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_runtime_release(
  int runtime,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Uint32,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_run_ambient_cache_operation(
  int runtime,
  int operation,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Pointer<mln_runtime_event_handler>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_set_event_handler(
  int runtime,
  Pointer<mln_runtime_event_handler> handler,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Int32 Function(mln_runtime, Uint64, Pointer<mln_diagnostic>)>()
external int mln_runtime_set_event_mask(
  int runtime,
  int mask,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Pointer<mln_http_header_transform>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_set_http_header_transform(
  int runtime,
  Pointer<mln_http_header_transform> transform,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Uint64,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_set_maximum_ambient_cache_size(
  int runtime,
  int size,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Int64,
    Uint32,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_set_offline_region_download_state(
  int runtime,
  int region_id,
  int state,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Int64,
    Bool,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_set_offline_region_observed(
  int runtime,
  int region_id,
  bool observed,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Pointer<mln_resource_provider>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_set_resource_provider(
  int runtime,
  Pointer<mln_resource_provider> provider,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Pointer<mln_resource_transform>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_set_resource_transform(
  int runtime,
  Pointer<mln_resource_transform> transform,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Int64,
    Pointer<Uint8>,
    Size,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_update_offline_region_metadata(
  int runtime,
  int region_id,
  Pointer<Uint8> metadata,
  int metadata_size,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_source_feature_query_options Function()>()
external mln_source_feature_query_options
mln_source_feature_query_options_default();

@Native<
  Int32 Function(
    mln_buffer_view,
    Pointer<mln_style_expression_options>,
    Pointer<mln_style_expression>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_style_expression_create(
  mln_buffer_view expression,
  Pointer<mln_style_expression_options> options,
  Pointer<mln_style_expression> out_expression,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_style_expression,
    Pointer<mln_style_expression_inputs>,
    Pointer<Char>,
    Size,
    Pointer<Size>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_style_expression_evaluate(
  int expression,
  Pointer<mln_style_expression_inputs> inputs,
  Pointer<Char> out_json,
  int json_capacity,
  Pointer<Size> out_json_length,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_style_expression,
    Pointer<mln_style_expression_info>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_style_expression_get_info(
  int expression,
  Pointer<mln_style_expression_info> out_info,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_style_expression_inputs Function()>()
external mln_style_expression_inputs mln_style_expression_inputs_default();

@Native<mln_style_expression_options Function()>()
external mln_style_expression_options mln_style_expression_options_default();

@Native<Void Function(mln_style_expression)>()
external void mln_style_expression_release(int expression);

@Native<mln_style_image_options Function()>()
external mln_style_image_options mln_style_image_options_default();

@Native<mln_style_transition_options Function()>()
external mln_style_transition_options mln_style_transition_options_default();

@Native<Uint32 Function()>()
external int mln_supported_render_backend_mask();

@Native<mln_vulkan_borrowed_texture_descriptor Function()>()
external mln_vulkan_borrowed_texture_descriptor
mln_vulkan_borrowed_texture_descriptor_default();

@Native<mln_vulkan_owned_texture_descriptor Function()>()
external mln_vulkan_owned_texture_descriptor
mln_vulkan_owned_texture_descriptor_default();

@Native<mln_vulkan_surface_descriptor Function()>()
external mln_vulkan_surface_descriptor mln_vulkan_surface_descriptor_default();
