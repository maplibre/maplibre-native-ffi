// Generated from the C headers by tools/bindgen. Do not edit.
// ignore_for_file: camel_case_types, constant_identifier_names, non_constant_identifier_names
@DefaultAsset(nativeAssetId)
library;

import 'dart:ffi';

import 'native_asset.dart';

export 'native_abi.dart';

typedef mln_acquired_frame = Uint64;
typedef mln_buffer = Uint64;
typedef mln_event_batch = Uint64;
typedef mln_geojson_source_data = Uint64;
typedef mln_map = Uint64;
typedef mln_map_projection = Uint64;
typedef mln_render_frame_batch = Uint64;
typedef mln_render_session = Uint64;
typedef mln_resource_request_handle = Uint64;
typedef mln_runtime = Uint64;

typedef mln_adapter_completion_listener =
    Pointer<NativeFunction<mln_adapter_completion_listenerFunction>>;
typedef mln_adapter_completion_listenerFunction =
    Void Function(
      Pointer<Void> user_data,
      Pointer<mln_adapter_completion_record> record,
    );
typedef mln_adapter_deferred_call_listener =
    Pointer<NativeFunction<mln_adapter_deferred_call_listenerFunction>>;
typedef mln_adapter_deferred_call_listenerFunction =
    Void Function(
      Pointer<Void> user_data,
      Pointer<mln_adapter_deferred_call_record> record,
    );
typedef mln_completion_callback =
    Pointer<NativeFunction<mln_completion_callbackFunction>>;
typedef mln_completion_callbackFunction =
    Void Function(
      Pointer<Void> user_data,
      Pointer<mln_completion_result> result,
    );
typedef mln_completion_release =
    Pointer<NativeFunction<mln_completion_releaseFunction>>;
typedef mln_completion_releaseFunction = Void Function(Pointer<Void> user_data);
typedef mln_custom_geometry_source_release_callback =
    Pointer<
      NativeFunction<mln_custom_geometry_source_release_callbackFunction>
    >;
typedef mln_custom_geometry_source_release_callbackFunction =
    Void Function(Pointer<Void> user_data);
typedef mln_custom_geometry_source_tile_callback =
    Pointer<NativeFunction<mln_custom_geometry_source_tile_callbackFunction>>;
typedef mln_custom_geometry_source_tile_callbackFunction =
    Void Function(Pointer<Void> user_data, mln_canonical_tile_id tile_id);
typedef mln_custom_mvt_vector_source_release_callback =
    Pointer<
      NativeFunction<mln_custom_mvt_vector_source_release_callbackFunction>
    >;
typedef mln_custom_mvt_vector_source_release_callbackFunction =
    Void Function(Pointer<Void> user_data);
typedef mln_custom_mvt_vector_source_tile_callback =
    Pointer<NativeFunction<mln_custom_mvt_vector_source_tile_callbackFunction>>;
typedef mln_custom_mvt_vector_source_tile_callbackFunction =
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
typedef mln_log_callback_release =
    Pointer<NativeFunction<mln_log_callback_releaseFunction>>;
typedef mln_log_callback_releaseFunction =
    Void Function(Pointer<Void> user_data);
typedef mln_queue_lock_callback =
    Pointer<NativeFunction<mln_queue_lock_callbackFunction>>;
typedef mln_queue_lock_callbackFunction =
    Void Function(Pointer<Void> user_data);
typedef mln_queue_lock_release =
    Pointer<NativeFunction<mln_queue_lock_releaseFunction>>;
typedef mln_queue_lock_releaseFunction = Void Function(Pointer<Void> user_data);
typedef mln_resource_provider_callback =
    Pointer<NativeFunction<mln_resource_provider_callbackFunction>>;
typedef mln_resource_provider_callbackFunction =
    Uint32 Function(
      Pointer<Void> user_data,
      Pointer<mln_resource_request> request,
      mln_resource_request_handle handle,
    );
typedef mln_resource_request_cancel_callback =
    Pointer<NativeFunction<mln_resource_request_cancel_callbackFunction>>;
typedef mln_resource_request_cancel_callbackFunction =
    Void Function(Pointer<Void> user_data);
typedef mln_resource_transform_callback =
    Pointer<NativeFunction<mln_resource_transform_callbackFunction>>;
typedef mln_resource_transform_callbackFunction =
    Int32 Function(
      Pointer<Void> user_data,
      Uint32 kind,
      Pointer<Char> url,
      Pointer<mln_resource_transform_response> out_response,
    );
typedef mln_runtime_callback_release =
    Pointer<NativeFunction<mln_runtime_callback_releaseFunction>>;
typedef mln_runtime_callback_releaseFunction =
    Void Function(Pointer<Void> user_data);
typedef mln_wake_callback = Pointer<NativeFunction<mln_wake_callbackFunction>>;
typedef mln_wake_callbackFunction = Void Function(Pointer<Void> user_data);
typedef mln_wake_release = Pointer<NativeFunction<mln_wake_releaseFunction>>;
typedef mln_wake_releaseFunction = Void Function(Pointer<Void> user_data);

final class mln_adapter_completion_record extends Struct {
  external Pointer<Void> owner;
  external mln_completion_result result;
}

final class mln_adapter_deferred_call_record extends Struct {
  external Pointer<Void> owner;
  @Uint32()
  external int callback;
  external Pointer<Void> arguments;
}

final class mln_adapter_http_header extends Struct {
  external Pointer<Char> name;
  external Pointer<Char> value;
}

final class mln_adapter_http_header_transform_rule extends Struct {
  @Uint32()
  external int kind;
  @Uint32()
  external int flags;
  external Pointer<Char> url;
  external Pointer<mln_adapter_http_header> headers;
  @Size()
  external int header_count;
}

final class mln_adapter_http_header_transform_rules extends Struct {
  external Pointer<mln_adapter_http_header_transform_rule> rules;
  @Size()
  external int count;
}

final class mln_adapter_log_callback_arguments extends Struct {
  @Uint32()
  external int severity;
  @Uint32()
  external int event;
  @Int64()
  external int code;
  external Pointer<Char> message;
}

final class mln_adapter_resource_provider_callback_arguments extends Struct {
  external Pointer<mln_resource_request> request;
  @Uint64()
  external int handle;
}

final class mln_adapter_resource_provider_rule extends Struct {
  @Uint32()
  external int kind;
  @Uint32()
  external int flags;
  external Pointer<Char> requested_url;
  external mln_resource_response response;
}

final class mln_adapter_resource_provider_rules extends Struct {
  external Pointer<mln_adapter_resource_provider_rule> rules;
  @Size()
  external int count;
}

final class mln_adapter_resource_rewrite_rule extends Struct {
  @Uint32()
  external int kind;
  @Uint32()
  external int flags;
  external Pointer<Char> url;
  external Pointer<Char> replacement_url;
}

final class mln_adapter_resource_rewrite_rules extends Struct {
  external Pointer<mln_adapter_resource_rewrite_rule> rules;
  @Size()
  external int count;
}

final class mln_adapter_resource_route extends Struct {
  @Uint32()
  external int kind;
  @Uint32()
  external int flags;
  external Pointer<Char> url;
}

final class mln_adapter_routed_resource_provider extends Struct {
  external Pointer<mln_adapter_resource_route> routes;
  @Size()
  external int route_count;
  external mln_resource_provider_callback callback;
  external Pointer<Void> user_data;
}

final class mln_animation_options extends Struct {
  @Uint32()
  external int size;
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
  external int kind;
  external mln_screen_point offset;
  @Double()
  external double amount;
  @Bool()
  external bool has_anchor;
  external mln_screen_point anchor;
  external mln_animation_options animation;
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
  @Double()
  external double latitude;
  @Double()
  external double longitude;
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
  @Uint32()
  external int size;
  @Uint32()
  external int reserved;
  @Uint64()
  external int generation;
  external mln_camera_options camera;
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
  external mln_completion_release release_user_data;
}

final class mln_completion_result extends Struct {
  @Uint32()
  external int size;
  @Int32()
  external int status;
  @Uint32()
  external int disposition;
  @Uint32()
  external int reserved;
  @Uint64()
  external int generation;
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
  external mln_custom_geometry_source_tile_callback fetch_tile;
  external mln_custom_geometry_source_tile_callback cancel_tile;
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
  external mln_custom_geometry_source_release_callback release_user_data;
}

final class mln_custom_mvt_vector_source_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external mln_custom_mvt_vector_source_tile_callback fetch_tile;
  external mln_custom_mvt_vector_source_tile_callback cancel_tile;
  external Pointer<Void> user_data;
  @Double()
  external double min_zoom;
  @Double()
  external double max_zoom;
  external mln_custom_mvt_vector_source_release_callback release_user_data;
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
  @Uint32()
  external int size;
  external Pointer<Void> display;
  external Pointer<Void> config;
  external Pointer<Void> share_context;
  @Uint32()
  external int client_api;
  external Pointer<Void> get_proc_address;
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
  @Uint64()
  external int coalescing_boundary;
  @Uint64()
  external int timeout_ns;
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
  external int tile_size;
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
  @Uint64()
  external int value;
}

final class mln_http_header_transform extends Struct {
  @Uint32()
  external int size;
  external mln_http_header_transform_callback callback;
  external Pointer<Void> user_data;
  external mln_runtime_callback_release release_user_data;
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
  external mln_camera_options camera;
  external mln_logical_extent logical_extent;
  external mln_projection_mode projection_mode;
  external mln_map_viewport_options viewport;
  @Bool()
  external bool fully_loaded;
  @Bool()
  external bool rendering_stats_view_enabled;
  @Bool()
  external bool repaint_demand;
  @Bool()
  external bool gesture_in_progress;
  @Uint64()
  external int event_mask;
  @Uint64()
  external int latest_render_update_generation;
  external mln_map_tile_options tile;
  external mln_bound_options bounds;
  external mln_free_camera_options free_camera;
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

final class mln_metal_borrowed_texture_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_render_target_extent extent;
  @Uint32()
  external int physical_width;
  @Uint32()
  external int physical_height;
  external Pointer<Void> texture;
}

final class mln_metal_context_descriptor extends Struct {
  @Uint32()
  external int size;
  external Pointer<Void> device;
}

final class mln_metal_owned_texture_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_render_target_extent extent;
  external mln_metal_context_descriptor context;
}

final class mln_metal_owned_texture_frame extends Struct {
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
  external Pointer<Void> texture;
  external Pointer<Void> device;
  @Uint64()
  external int pixel_format;
}

final class mln_metal_surface_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_render_target_extent extent;
  external mln_metal_context_descriptor context;
  external Pointer<Void> layer;
}

final class mln_offline_geometry_region_definition extends Struct {
  @Uint32()
  external int size;
  external Pointer<Char> style_url;
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
  @Uint32()
  external int size;
  @Int64()
  external int id;
  external mln_offline_region_definition definition;
  external Pointer<Uint8> metadata;
  @Size()
  external int metadata_size;
}

final class mln_offline_region_status extends Struct {
  @Uint32()
  external int size;
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
  @Uint32()
  external int size;
  external Pointer<Char> style_url;
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

final class mln_opengl_borrowed_texture_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_render_target_extent extent;
  @Uint32()
  external int physical_width;
  @Uint32()
  external int physical_height;
  external mln_opengl_context_descriptor context;
  @Uint32()
  external int texture;
  @Uint32()
  external int target;
}

final class mln_opengl_context_descriptor extends Struct {
  @Uint32()
  external int size;
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
  external mln_render_target_extent extent;
  external mln_opengl_context_descriptor context;
}

final class mln_opengl_owned_texture_frame extends Struct {
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

final class mln_opengl_surface_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_render_target_extent extent;
  external mln_opengl_context_descriptor context;
  external Pointer<Void> surface;
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

final class mln_queried_feature extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external mln_buffer_view feature;
  external mln_buffer_view source_id;
  external mln_buffer_view source_layer_id;
  external mln_buffer_view state;
}

final class mln_queue_lock extends Struct {
  @Uint32()
  external int size;
  external mln_queue_lock_callback lock;
  external mln_queue_lock_callback unlock;
  external Pointer<Void> user_data;
  external mln_queue_lock_release release_user_data;
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
  external int extent_generation;
  @Uint64()
  external int frame_generation;
  @Bool()
  external bool needs_repaint;
}

final class mln_render_session_attach_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int driver;
  @Uint32()
  external int requested_texture_ring_depth;
  @Uint32()
  external int reserved;
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
  @Uint32()
  external int driver;
  @Uint32()
  external int latest_result;
  external mln_render_target_extent extent;
  @Uint64()
  external int generation;
  @Uint64()
  external int map_update_generation;
  @Uint64()
  external int rendered_update_generation;
  @Uint64()
  external int extent_generation;
  @Uint64()
  external int frame_generation;
  @Uint64()
  external int latest_demand_token;
  @Uint32()
  external int pending_demand_count;
  @Uint32()
  external int acquired_frame_count;
  @Bool()
  external bool target_ready;
  @Bool()
  external bool pending_changes;
}

final class mln_render_target_extent extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int width;
  @Uint32()
  external int height;
  @Double()
  external double scale_factor;
}

final class mln_rendered_feature_query_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external Pointer<mln_buffer_view> layer_ids;
  @Size()
  external int layer_id_count;
  external Pointer<mln_buffer_view> filter;
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
  external mln_runtime_callback_release release_user_data;
}

final class mln_resource_request extends Struct {
  @Uint32()
  external int size;
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
  @Bool()
  external bool has_range;
  @Uint64()
  external int range_start;
  @Uint64()
  external int range_end;
  @Bool()
  external bool has_prior_modified;
  @Int64()
  external int prior_modified_unix_ms;
  @Bool()
  external bool has_prior_expires;
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
  external int status;
  @Uint32()
  external int error_reason;
  external Pointer<Uint8> bytes;
  @Size()
  external int byte_count;
  external Pointer<Char> error_message;
  @Bool()
  external bool must_revalidate;
  @Bool()
  external bool has_modified;
  @Int64()
  external int modified_unix_ms;
  @Bool()
  external bool has_expires;
  @Int64()
  external int expires_unix_ms;
  external Pointer<Char> etag;
  @Bool()
  external bool has_retry_after;
  @Int64()
  external int retry_after_unix_ms;
}

final class mln_resource_transform extends Struct {
  @Uint32()
  external int size;
  external mln_resource_transform_callback callback;
  external Pointer<Void> user_data;
  external mln_runtime_callback_release release_user_data;
}

final class mln_resource_transform_response extends Struct {
  @Uint32()
  external int size;
  external Pointer<Char> url;
  external Pointer<Void> context;
}

final class mln_runtime_event extends Struct {
  @Uint32()
  external int type;
  @Uint32()
  external int source_type;
  @Uint64()
  external int source;
  @Int32()
  external int code;
  @Uint32()
  external int payload_type;
  @Uint64()
  external int message_offset;
  @Uint32()
  external int message_size;
  external mln_runtime_event_payload payload;
}

final class mln_runtime_event_batch_view extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int event_size;
  external Pointer<mln_runtime_event> events;
  @Size()
  external int event_count;
  external Pointer<Char> messages;
  @Size()
  external int messages_size;
}

final class mln_runtime_event_camera_transition_finished extends Struct {
  @Uint64()
  external int transition_id;
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
  external mln_runtime_event_camera_transition_finished
  camera_transition_finished;
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
  @Uint32()
  external int flags;
  external Pointer<Char> asset_path;
  external Pointer<Char> cache_path;
  @Uint64()
  external int event_mask;
  external mln_wake event_wake;
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

final class mln_source_feature_query_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  external Pointer<mln_buffer_view> source_layer_ids;
  @Size()
  external int source_layer_id_count;
  external Pointer<mln_buffer_view> filter;
}

final class mln_style_image_info extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int width;
  @Uint32()
  external int height;
  @Uint32()
  external int stride;
  @Size()
  external int byte_length;
  @Size()
  external int stretch_x_count;
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
  @Bool()
  external bool has_content;
  @Bool()
  external bool has_text_fit_width;
  @Bool()
  external bool has_text_fit_height;
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

final class mln_style_image_result extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int reserved;
  external mln_style_image_info info;
  external mln_buffer_view pixels;
  external Pointer<mln_image_stretch> stretch_x;
  @Size()
  external int stretch_x_count;
  external Pointer<mln_image_stretch> stretch_y;
  @Size()
  external int stretch_y_count;
}

final class mln_style_image_stretches_result extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int reserved;
  external Pointer<mln_image_stretch> stretch_x;
  @Size()
  external int stretch_x_count;
  external Pointer<mln_image_stretch> stretch_y;
  @Size()
  external int stretch_y_count;
}

final class mln_style_layer_entry extends Struct {
  @Uint32()
  external int size;
  external mln_buffer_view id;
  external mln_buffer_view type;
  external mln_buffer_view source_id;
  external mln_buffer_view source_layer;
}

final class mln_style_layer_info extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int reserved;
  external mln_buffer_view type;
  @Double()
  external double min_zoom;
  @Double()
  external double max_zoom;
  @Uint32()
  external int visibility;
}

final class mln_style_layer_result extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int reserved;
  external mln_style_layer_info info;
  external mln_buffer_view source_id;
  external mln_buffer_view source_layer;
}

final class mln_style_source_info extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int type;
  @Uint32()
  external int fields;
  @Size()
  external int id_size;
  @Bool()
  external bool is_volatile;
  @Bool()
  external bool has_attribution;
  @Size()
  external int attribution_size;
  @Size()
  external int url_size;
  @Size()
  external int tile_count;
  @Double()
  external double min_zoom;
  @Double()
  external double max_zoom;
  @Uint32()
  external int scheme;
  external mln_lat_lng_bounds bounds;
  @Uint32()
  external int tile_size;
  @Uint32()
  external int vector_encoding;
  @Uint32()
  external int raster_encoding;
}

final class mln_style_source_result extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int reserved;
  external mln_style_source_info info;
  external mln_buffer_view attribution;
  external mln_buffer_view url;
  external Pointer<mln_buffer_view> tile_urls;
  @Size()
  external int tile_url_count;
}

final class mln_style_source_tile_info extends Struct {
  @Size()
  external int tile_count;
  @Double()
  external double min_zoom;
  @Double()
  external double max_zoom;
  @Uint32()
  external int scheme;
}

final class mln_style_source_tile_urls_result extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int reserved;
  external Pointer<mln_buffer_view> tile_urls;
  @Size()
  external int tile_url_count;
}

final class mln_style_tile_source_options extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int fields;
  @Double()
  external double min_zoom;
  @Double()
  external double max_zoom;
  external mln_buffer_view attribution;
  @Uint32()
  external int scheme;
  external mln_lat_lng_bounds bounds;
  @Uint32()
  external int tile_size;
  @Uint32()
  external int vector_encoding;
  @Uint32()
  external int raster_encoding;
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
  external int size;
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
  @Uint32()
  external int size;
  @Uint32()
  external int reserved;
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

final class mln_vulkan_borrowed_texture_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_render_target_extent extent;
  @Uint32()
  external int physical_width;
  @Uint32()
  external int physical_height;
  external mln_vulkan_context_descriptor context;
  @Uint64()
  external int image;
  @Uint64()
  external int image_view;
  @Uint32()
  external int format;
  @Uint32()
  external int initial_layout;
  @Uint32()
  external int final_layout;
}

final class mln_vulkan_context_descriptor extends Struct {
  @Uint32()
  external int size;
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
  external mln_render_target_extent extent;
  external mln_vulkan_context_descriptor context;
}

final class mln_vulkan_owned_texture_frame extends Struct {
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

final class mln_vulkan_surface_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_render_target_extent extent;
  external mln_vulkan_context_descriptor context;
  @Uint64()
  external int surface;
}

final class mln_wake extends Struct {
  @Uint32()
  external int size;
  external mln_wake_callback callback;
  external Pointer<Void> user_data;
  external mln_wake_release release_user_data;
}

final class mln_webgl_context_descriptor extends Struct {
  @Uint32()
  external int size;
  @Uint32()
  external int kind;
  @Int32()
  external int context;
  external mln_buffer_view canvas_selector;
}

final class mln_webgpu_borrowed_texture_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_render_target_extent extent;
  @Uint32()
  external int physical_width;
  @Uint32()
  external int physical_height;
  external mln_webgpu_context_descriptor context;
  external Pointer<Void> texture;
  external Pointer<Void> texture_view;
  @Uint32()
  external int format;
}

final class mln_webgpu_context_descriptor extends Struct {
  @Uint32()
  external int size;
  external Pointer<Void> instance;
  external Pointer<Void> device;
  external Pointer<Void> queue;
}

final class mln_webgpu_owned_texture_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_render_target_extent extent;
  external mln_webgpu_context_descriptor context;
}

final class mln_webgpu_owned_texture_frame extends Struct {
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
  external Pointer<Void> texture;
  external Pointer<Void> texture_view;
  external Pointer<Void> device;
  @Uint32()
  external int format;
}

final class mln_webgpu_surface_descriptor extends Struct {
  @Uint32()
  external int size;
  external mln_render_target_extent extent;
  external mln_webgpu_context_descriptor context;
  external Pointer<Void> surface;
  @Uint32()
  external int format;
}

final class mln_wgl_context_descriptor extends Struct {
  @Uint32()
  external int size;
  external Pointer<Void> device_context;
  external Pointer<Void> share_context;
  external Pointer<Void> get_proc_address;
}

// mln_adapter_completion_copy_kind
const MLN_ADAPTER_COMPLETION_COPY_FLAT = 0;
const MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW = 3303399434;
const MLN_ADAPTER_COMPLETION_COPY_CAMERA_OPTIONS = 1729514601;
const MLN_ADAPTER_COMPLETION_COPY_CAMERA_QUERY_RESULT = 1485572681;
const MLN_ADAPTER_COMPLETION_COPY_LAT_LNG = 2638194669;
const MLN_ADAPTER_COMPLETION_COPY_LAT_LNG_BOUNDS = 3400515811;
const MLN_ADAPTER_COMPLETION_COPY_MAP = 438078448;
const MLN_ADAPTER_COMPLETION_COPY_MAP_PROJECTION = 3555078466;
const MLN_ADAPTER_COMPLETION_COPY_OFFLINE_REGION_INFO = 3939645993;
const MLN_ADAPTER_COMPLETION_COPY_OFFLINE_REGION_STATUS = 1567541687;
const MLN_ADAPTER_COMPLETION_COPY_QUERIED_FEATURE = 3048968095;
const MLN_ADAPTER_COMPLETION_COPY_SCREEN_POINT = 990046368;
const MLN_ADAPTER_COMPLETION_COPY_STYLE_IMAGE_RESULT = 2311975790;
const MLN_ADAPTER_COMPLETION_COPY_STYLE_IMAGE_STRETCHES_RESULT = 167536911;
const MLN_ADAPTER_COMPLETION_COPY_STYLE_LAYER_ENTRY = 2945408873;
const MLN_ADAPTER_COMPLETION_COPY_STYLE_LAYER_RESULT = 2005255953;
const MLN_ADAPTER_COMPLETION_COPY_STYLE_SOURCE_RESULT = 514529690;
const MLN_ADAPTER_COMPLETION_COPY_STYLE_SOURCE_TILE_URLS_RESULT = 3638232521;
const MLN_ADAPTER_COMPLETION_COPY_STYLE_TRANSITION_OPTIONS = 221419390;
const MLN_ADAPTER_COMPLETION_COPY_TEXTURE_READBACK_RESULT = 2875519289;

// mln_adapter_dart_port_callback
const MLN_ADAPTER_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE =
    3644896267;
const MLN_ADAPTER_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_CANCEL_TILE =
    433183623;
const MLN_ADAPTER_DART_PORT_CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_FETCH_TILE =
    658252347;
const MLN_ADAPTER_DART_PORT_CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_CANCEL_TILE =
    1073125309;
const MLN_ADAPTER_DART_PORT_WAKE_CALLBACK = 2393247646;
const MLN_ADAPTER_DART_PORT_RESOURCE_REQUEST_SET_CANCEL_CALLBACK_CALLBACK =
    1605404209;

// mln_adapter_deferred_callback
const MLN_ADAPTER_DEFERRED_LOG_CALLBACK = 2203584336;
const MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK = 2143245793;

// mln_adapter_resource_route_flags
const MLN_ADAPTER_RESOURCE_ROUTE_FLAGS_NONE = 0;
const MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB = 1;
const MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL = 2;

// mln_adapter_url_match_flags
const MLN_ADAPTER_URL_MATCH_FLAGS_NONE = 0;
const MLN_ADAPTER_URL_MATCH_GLOB = 1;

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

// mln_camera_delta_kind
const MLN_CAMERA_DELTA_MOVE = 0;
const MLN_CAMERA_DELTA_SCALE = 1;
const MLN_CAMERA_DELTA_BEARING = 2;
const MLN_CAMERA_DELTA_PITCH = 3;

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

// mln_camera_update_mode
const MLN_CAMERA_UPDATE_MODE_JUMP = 0;
const MLN_CAMERA_UPDATE_MODE_EASE = 1;
const MLN_CAMERA_UPDATE_MODE_FLY = 2;

// mln_command_disposition
const MLN_COMMAND_DISPOSITION_COMMITTED = 0;
const MLN_COMMAND_DISPOSITION_SUPERSEDED = 1;
const MLN_COMMAND_DISPOSITION_FAILED = 2;
const MLN_COMMAND_DISPOSITION_CANCELLED = 3;

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

// mln_feature_state_selector_field
const MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID = 1;
const MLN_FEATURE_STATE_SELECTOR_FEATURE_ID = 2;
const MLN_FEATURE_STATE_SELECTOR_STATE_KEY = 4;

// mln_frame_demand_flag
const MLN_FRAME_DEMAND_IF_NEEDED = 1;
const MLN_FRAME_DEMAND_PRESENT = 2;

// mln_free_camera_option_field
const MLN_FREE_CAMERA_OPTION_POSITION = 1;
const MLN_FREE_CAMERA_OPTION_ORIENTATION = 2;

// mln_geojson_source_option_field
const MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM = 1;
const MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM = 2;
const MLN_GEOJSON_SOURCE_OPTION_TOLERANCE = 4;
const MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM = 8;
const MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES = 16;
const MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE = 32;
const MLN_GEOJSON_SOURCE_OPTION_BUFFER = 64;
const MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS = 128;
const MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS = 256;
const MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS = 512;
const MLN_GEOJSON_SOURCE_OPTION_CLUSTER = 1024;
const MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING = 2048;

// mln_gesture_phase
const MLN_GESTURE_PHASE_NONE = 0;
const MLN_GESTURE_PHASE_BEGIN = 1;
const MLN_GESTURE_PHASE_UPDATE = 2;
const MLN_GESTURE_PHASE_END = 3;
const MLN_GESTURE_PHASE_CANCEL = 4;

// mln_gpu_sync_kind
const MLN_GPU_SYNC_CPU_COMPLETE = 0;
const MLN_GPU_SYNC_METAL_SHARED_EVENT = 1;
const MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE = 2;
const MLN_GPU_SYNC_OPENGL_FENCE = 3;
const MLN_GPU_SYNC_WEBGPU_TOKEN = 4;

// mln_location_indicator_image_kind
const MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP = 0;
const MLN_LOCATION_INDICATOR_IMAGE_KIND_BEARING = 1;
const MLN_LOCATION_INDICATOR_IMAGE_KIND_SHADOW = 2;

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

// mln_queried_feature_field
const MLN_QUERIED_FEATURE_SOURCE_ID = 1;
const MLN_QUERIED_FEATURE_SOURCE_LAYER_ID = 2;
const MLN_QUERIED_FEATURE_STATE = 4;

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
const MLN_RENDER_RESULT_SIZE_PENDING = 2;
const MLN_RENDER_RESULT_TARGET_NOT_READY = 3;
const MLN_RENDER_RESULT_SUPERSEDED = 4;
const MLN_RENDER_RESULT_DEADLINE_MISSED = 5;

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

// mln_rendered_feature_query_option_field
const MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS = 1;

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

// mln_resource_loading_method
const MLN_RESOURCE_LOADING_METHOD_ALL = 0;
const MLN_RESOURCE_LOADING_METHOD_CACHE_ONLY = 1;
const MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY = 2;

// mln_resource_priority
const MLN_RESOURCE_PRIORITY_REGULAR = 0;
const MLN_RESOURCE_PRIORITY_LOW = 1;

// mln_resource_provider_decision
const MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH = 0;
const MLN_RESOURCE_PROVIDER_DECISION_HANDLE = 1;

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
const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE = 512;
const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_ERROR = 1024;
const MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FINISHED = 2048;
const MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FAILED = 4096;
const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_STARTED = 8192;
const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED = 16384;
const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_STARTED = 32768;
const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_FINISHED = 65536;
const MLN_RUNTIME_EVENT_MASK_MAP_STYLE_IMAGE_MISSING = 131072;
const MLN_RUNTIME_EVENT_MASK_MAP_TILE_ACTION = 262144;
const MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_TRANSITION_FINISHED = 4194304;
const MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_STATUS_CHANGED = 524288;
const MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_RESPONSE_ERROR = 1048576;
const MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 2097152;
const MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS = 4718590;
const MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS = 3670016;
const MLN_RUNTIME_EVENT_MASK_ALL = 8388606;

// mln_runtime_event_payload_type
const MLN_RUNTIME_EVENT_PAYLOAD_NONE = 0;
const MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME = 1;
const MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP = 2;
const MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION = 4;
const MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS = 5;
const MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR = 6;
const MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT = 7;
const MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED = 9;

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
const MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE = 9;
const MLN_RUNTIME_EVENT_MAP_RENDER_ERROR = 10;
const MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED = 11;
const MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED = 12;
const MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED = 13;
const MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED = 14;
const MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED = 15;
const MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED = 16;
const MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING = 17;
const MLN_RUNTIME_EVENT_MAP_TILE_ACTION = 18;
const MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED = 19;
const MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR = 20;
const MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 21;
const MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED = 22;

// mln_source_feature_query_option_field
const MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS = 1;

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

// mln_style_tile_source_option_field
const MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM = 1;
const MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM = 2;
const MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION = 4;
const MLN_STYLE_TILE_SOURCE_OPTION_SCHEME = 8;
const MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS = 16;
const MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE = 32;
const MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING = 64;
const MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING = 128;

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

// mln_viewport_mode
const MLN_VIEWPORT_MODE_DEFAULT = 0;
const MLN_VIEWPORT_MODE_FLIPPED_Y = 1;

// mln_webgl_context_kind
const MLN_WEBGL_CONTEXT_EXISTING = 0;
const MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS = 1;

@Native<Int32 Function(mln_acquired_frame, Pointer<mln_diagnostic>)>()
external int mln_acquired_frame_dispose(
  int frame,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_acquired_frame,
    Pointer<mln_metal_owned_texture_frame>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_acquired_frame_get_metal_texture(
  int frame,
  Pointer<mln_metal_owned_texture_frame> out_frame,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_acquired_frame,
    Pointer<mln_opengl_owned_texture_frame>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_acquired_frame_get_opengl_texture(
  int frame,
  Pointer<mln_opengl_owned_texture_frame> out_frame,
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
    Pointer<mln_vulkan_owned_texture_frame>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_acquired_frame_get_vulkan_texture(
  int frame,
  Pointer<mln_vulkan_owned_texture_frame> out_frame,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_acquired_frame,
    Pointer<mln_webgpu_owned_texture_frame>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_acquired_frame_get_webgpu_texture(
  int frame,
  Pointer<mln_webgpu_owned_texture_frame> out_frame,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    Pointer<mln_acquired_frame>,
    Pointer<mln_gpu_sync>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_acquired_frame_release(
  Pointer<mln_acquired_frame> frame,
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

@Native<Int32 Function(Pointer<Void>, Uint64, Pointer<mln_diagnostic>)>()
external int mln_adapter_arena_adopt_handle(
  Pointer<Void> arena,
  int handle,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    Pointer<Void>,
    mln_runtime_callback_release,
    Pointer<Void>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_adapter_arena_adopt_release(
  Pointer<Void> arena,
  mln_runtime_callback_release release,
  Pointer<Void> context,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Pointer<Void> Function(Pointer<Void>, Size, Size)>()
external Pointer<Void> mln_adapter_arena_allocate(
  Pointer<Void> arena,
  int size,
  int alignment,
);

@Native<Pointer<Void> Function()>()
external Pointer<Void> mln_adapter_arena_create();

@Native<Void Function(Pointer<Void>)>()
external void mln_adapter_arena_destroy(Pointer<Void> arena);

@Native<
  Int32 Function(
    Uint32,
    Size,
    mln_adapter_completion_listener,
    Pointer<Void>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_adapter_completion_create(
  int copy_kind,
  int element_size,
  mln_adapter_completion_listener listener,
  Pointer<Void> user_data,
  Pointer<mln_completion> out_completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Void Function(Pointer<mln_adapter_completion_record>)>()
external void mln_adapter_completion_record_adopt(
  Pointer<mln_adapter_completion_record> record,
);

@Native<Void Function(Pointer<mln_adapter_completion_record>)>()
external void mln_adapter_completion_record_destroy(
  Pointer<mln_adapter_completion_record> record,
);

@Native<Void Function(Pointer<mln_completion>)>()
external void mln_adapter_completion_reject(Pointer<mln_completion> completion);

@Native<
  Void Function(
    mln_custom_geometry_source_tile_callback,
    mln_custom_geometry_source_tile_callback,
    Pointer<Void>,
  )
>()
external void mln_adapter_custom_geometry_callbacks_retire(
  mln_custom_geometry_source_tile_callback fetch_tile,
  mln_custom_geometry_source_tile_callback cancel_tile,
  Pointer<Void> user_data,
);

@Native<
  Void Function(
    mln_custom_mvt_vector_source_tile_callback,
    mln_custom_mvt_vector_source_tile_callback,
    Pointer<Void>,
  )
>()
external void mln_adapter_custom_mvt_vector_callbacks_retire(
  mln_custom_mvt_vector_source_tile_callback fetch_tile,
  mln_custom_mvt_vector_source_tile_callback cancel_tile,
  Pointer<Void> user_data,
);

@Native<
  Int32 Function(
    Uint32,
    Size,
    Pointer<Void>,
    Int64,
    Int64,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_adapter_dart_completion_create(
  int copy_kind,
  int element_size,
  Pointer<Void> post_cobject,
  int port,
  int token,
  Pointer<mln_completion> out_completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    Uint32,
    Pointer<Void>,
    Int64,
    Pointer<Pointer<Void>>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_adapter_dart_deferred_callback_create(
  int callback,
  Pointer<Void> post_cobject,
  int port,
  Pointer<Pointer<Void>> out_context,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Pointer<Void> Function(Pointer<Void>, Int64)>()
external Pointer<Void> mln_adapter_dart_port_create(
  Pointer<Void> post_cobject,
  int port,
);

@Native<Pointer<Void> Function(Uint32)>()
external Pointer<Void> mln_adapter_dart_port_function(int id);

@Native<Void Function(Pointer<Void>)>()
external void mln_adapter_dart_port_release(Pointer<Void> context);

@Native<Void Function(Pointer<Void>)>()
external void mln_adapter_dart_release(Pointer<Void> context);

@Native<
  Int32 Function(
    Pointer<Void>,
    Int64,
    Pointer<Void>,
    Pointer<Void>,
    Pointer<Uint64>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_adapter_dart_release_register(
  Pointer<Void> post_cobject,
  int port,
  Pointer<Void> context,
  Pointer<Void> arena,
  Pointer<Uint64> out_registration,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    Pointer<Void>,
    Int64,
    Pointer<mln_wake>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_adapter_dart_wake_create(
  Pointer<Void> post_cobject,
  int port,
  Pointer<mln_wake> out_wake,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Void Function(Pointer<mln_adapter_deferred_call_record>)>()
external void mln_adapter_deferred_call_record_adopt(
  Pointer<mln_adapter_deferred_call_record> record,
);

@Native<Void Function(Pointer<mln_adapter_deferred_call_record>)>()
external void mln_adapter_deferred_call_record_destroy(
  Pointer<mln_adapter_deferred_call_record> record,
);

@Native<
  Int32 Function(
    Uint32,
    mln_adapter_deferred_call_listener,
    Pointer<Void>,
    Pointer<Pointer<Void>>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_adapter_deferred_callback_create(
  int callback,
  mln_adapter_deferred_call_listener listener,
  Pointer<Void> listener_user_data,
  Pointer<Pointer<Void>> out_context,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Pointer<Void> Function(Uint32)>()
external Pointer<Void> mln_adapter_deferred_callback_function(int callback);

@Native<Void Function(Pointer<Void>)>()
external void mln_adapter_deferred_callback_release(Pointer<Void> context);

@Native<
  Int32 Function(
    Pointer<Void>,
    Uint32,
    Pointer<Char>,
    Pointer<mln_http_header_transform_response>,
  )
>()
external int mln_adapter_http_header_transform_callback(
  Pointer<Void> user_data,
  int kind,
  Pointer<Char> url,
  Pointer<mln_http_header_transform_response> out_response,
);

@Native<Int32 Function(Pointer<Char>, Pointer<Char>, Pointer<mln_diagnostic>)>()
external int mln_adapter_http_header_validate(
  Pointer<Char> name,
  Pointer<Char> value,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Void Function(Pointer<Void>)>()
external void mln_adapter_owner_finalize(Pointer<Void> token);

@Native<Pointer<Void> Function(Uint64)>()
external Pointer<Void> mln_adapter_owner_token_create(int handle);

@Native<Void Function(Pointer<Void>)>()
external void mln_adapter_owner_token_destroy(Pointer<Void> token);

@Native<
  Uint32 Function(
    Pointer<Void>,
    Pointer<mln_resource_request>,
    mln_resource_request_handle,
  )
>()
external int mln_adapter_resource_provider_rules_callback(
  Pointer<Void> user_data,
  Pointer<mln_resource_request> request,
  int handle,
);

@Native<
  Int32 Function(
    Pointer<Void>,
    Uint32,
    Pointer<Char>,
    Pointer<mln_resource_transform_response>,
  )
>()
external int mln_adapter_resource_transform_rewrite_callback(
  Pointer<Void> user_data,
  int kind,
  Pointer<Char> url,
  Pointer<mln_resource_transform_response> out_response,
);

@Native<
  Uint32 Function(
    Pointer<Void>,
    Pointer<mln_resource_request>,
    mln_resource_request_handle,
  )
>()
external int mln_adapter_routed_resource_provider_callback(
  Pointer<Void> user_data,
  Pointer<mln_resource_request> request,
  int handle,
);

@Native<
  Int32 Function(
    Pointer<Void>,
    Pointer<Void>,
    Pointer<Void>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_android_init(
  Pointer<Void> jni_env,
  Pointer<Void> jni_class,
  Pointer<Void> context,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_animation_options Function()>()
external mln_animation_options mln_animation_options_default();

@Native<mln_bound_options Function()>()
external mln_bound_options mln_bound_options_default();

@Native<Void Function(mln_buffer)>()
external void mln_buffer_destroy(int buffer);

@Native<
  Int32 Function(mln_buffer, Pointer<mln_buffer_view>, Pointer<mln_diagnostic>)
>()
external int mln_buffer_get(
  int buffer,
  Pointer<mln_buffer_view> out_view,
  Pointer<mln_diagnostic> out_diagnostic,
);

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

@Native<
  Int32 Function(
    mln_event_batch,
    Pointer<mln_runtime_event_batch_view>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_event_batch_get(
  int batch,
  Pointer<mln_runtime_event_batch_view> out_view,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Void Function(mln_event_batch)>()
external void mln_event_batch_release(int batch);

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
external void mln_geojson_source_data_destroy(int data);

@Native<mln_geojson_source_options Function()>()
external mln_geojson_source_options mln_geojson_source_options_default();

@Native<mln_gpu_sync Function()>()
external mln_gpu_sync mln_gpu_sync_default();

@Native<
  Int32 Function(
    Pointer<mln_http_header_transform_response>,
    Pointer<Char>,
    Size,
    Pointer<Char>,
    Size,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_http_header_transform_response_set(
  Pointer<mln_http_header_transform_response> response,
  Pointer<Char> name,
  int name_size,
  Pointer<Char> value,
  int value_size,
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

@Native<
  Int32 Function(
    mln_log_callback,
    Pointer<Void>,
    mln_log_callback_release,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_log_set_callback(
  mln_log_callback callback,
  Pointer<Void> user_data,
  mln_log_callback_release release_user_data,
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
external int mln_map_add_color_relief_layer(
  int map,
  mln_buffer_view layer_id,
  mln_buffer_view source_id,
  mln_buffer_view before_layer_id,
  Pointer<mln_completion> completion,
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
    mln_buffer_view,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_add_hillshade_layer(
  int map,
  mln_buffer_view layer_id,
  mln_buffer_view source_id,
  mln_buffer_view before_layer_id,
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
external int mln_map_add_location_indicator_layer(
  int map,
  mln_buffer_view layer_id,
  mln_buffer_view before_layer_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_buffer_view>,
    Size,
    Pointer<mln_style_tile_source_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_add_raster_dem_source_tiles(
  int map,
  mln_buffer_view source_id,
  Pointer<mln_buffer_view> tiles,
  int tile_count,
  Pointer<mln_style_tile_source_options> options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_buffer_view,
    Pointer<mln_style_tile_source_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_add_raster_dem_source_url(
  int map,
  mln_buffer_view source_id,
  mln_buffer_view url,
  Pointer<mln_style_tile_source_options> options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_buffer_view>,
    Size,
    Pointer<mln_style_tile_source_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_add_raster_source_tiles(
  int map,
  mln_buffer_view source_id,
  Pointer<mln_buffer_view> tiles,
  int tile_count,
  Pointer<mln_style_tile_source_options> options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_buffer_view,
    Pointer<mln_style_tile_source_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_add_raster_source_url(
  int map,
  mln_buffer_view source_id,
  mln_buffer_view url,
  Pointer<mln_style_tile_source_options> options,
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
    mln_buffer_view,
    Pointer<mln_buffer_view>,
    Size,
    Pointer<mln_style_tile_source_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_add_vector_source_tiles(
  int map,
  mln_buffer_view source_id,
  Pointer<mln_buffer_view> tiles,
  int tile_count,
  Pointer<mln_style_tile_source_options> options,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_buffer_view,
    Pointer<mln_style_tile_source_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_add_vector_source_url(
  int map,
  mln_buffer_view source_id,
  mln_buffer_view url,
  Pointer<mln_style_tile_source_options> options,
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
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_camera_query(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_camera_options>,
    Pointer<Uint64>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_camera_snapshot_get(
  int map,
  Pointer<mln_camera_options> out_camera,
  Pointer<Uint64> out_generation,
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

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_copy_layer_source_id(
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
external int mln_map_copy_layer_source_layer(
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
external int mln_map_copy_style_image_premultiplied_rgba8(
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
external int mln_map_copy_style_image_stretches(
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
external int mln_map_copy_style_source_attribution(
  int map,
  mln_buffer_view source_id,
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
external int mln_map_copy_style_source_url(
  int map,
  mln_buffer_view source_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Pointer<mln_map_options>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_create(
  int runtime,
  Pointer<mln_map_options> options,
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
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_get_layer_filter(
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
external int mln_map_get_layer_property(
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
external int mln_map_get_style_image_info(
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
external int mln_map_get_style_layer_info(
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
external int mln_map_get_style_source_info(
  int map,
  mln_buffer_view source_id,
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
external int mln_map_get_style_source_tile_urls(
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
  Int32 Function(
    mln_map,
    mln_screen_point,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_lat_lng_for_pixel(
  int map,
  mln_screen_point point,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_screen_point,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_lat_lng_for_pixel_unwrapped(
  int map,
  mln_screen_point point,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_screen_point>,
    Size,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_lat_lngs_for_pixels(
  int map,
  Pointer<mln_screen_point> points,
  int point_count,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_screen_point>,
    Size,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_lat_lngs_for_pixels_unwrapped(
  int map,
  Pointer<mln_screen_point> points,
  int point_count,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_list_style_layer_ids(
  int map,
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
external int mln_map_list_style_source_ids(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_loaded_style_json(
  int map,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Double,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_meters_per_pixel_at_latitude(
  int map,
  double latitude,
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
    mln_map,
    mln_lat_lng,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_pixel_for_lat_lng(
  int map,
  mln_lat_lng coordinate,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_lat_lng>,
    Size,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_pixels_for_lat_lngs(
  int map,
  Pointer<mln_lat_lng> coordinates,
  int coordinate_count,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Int32 Function(mln_map_projection, Pointer<mln_diagnostic>)>()
external int mln_map_projection_close(
  int projection,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_projection_create(
  int map,
  Pointer<mln_completion> completion,
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
  Int32 Function(
    mln_map_projection,
    Pointer<mln_lat_lng>,
    Size,
    mln_edge_insets,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_projection_set_visible_coordinates(
  int projection,
  Pointer<mln_lat_lng> coordinates,
  int coordinate_count,
  mln_edge_insets padding,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map_projection,
    mln_buffer_view,
    mln_edge_insets,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_projection_set_visible_geometry(
  int projection,
  mln_buffer_view geometry,
  mln_edge_insets padding,
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
    mln_buffer_view,
    Pointer<mln_buffer_view>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_layer_filter(
  int map,
  mln_buffer_view layer_id,
  Pointer<mln_buffer_view> filter,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Double,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_layer_max_zoom(
  int map,
  mln_buffer_view layer_id,
  double max_zoom,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Double,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_layer_min_zoom(
  int map,
  mln_buffer_view layer_id,
  double min_zoom,
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
external int mln_map_set_layer_property(
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
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_layer_source_id(
  int map,
  mln_buffer_view layer_id,
  mln_buffer_view source_id,
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
external int mln_map_set_layer_source_layer(
  int map,
  mln_buffer_view layer_id,
  mln_buffer_view source_layer,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Uint32,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_layer_visibility(
  int map,
  mln_buffer_view layer_id,
  int visibility,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Double,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_location_indicator_accuracy_radius(
  int map,
  mln_buffer_view layer_id,
  double radius,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Double,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_location_indicator_bearing(
  int map,
  mln_buffer_view layer_id,
  double bearing,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    Uint32,
    mln_buffer_view,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_location_indicator_image_name(
  int map,
  mln_buffer_view layer_id,
  int image_kind,
  mln_buffer_view image_id,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    mln_buffer_view,
    mln_lat_lng,
    Double,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_location_indicator_location(
  int map,
  mln_buffer_view layer_id,
  mln_lat_lng coordinate,
  double altitude,
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
    Pointer<Char>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_map_set_style_url(
  int map,
  Pointer<Char> url,
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

@Native<
  Int32 Function(mln_map, Pointer<mln_map_snapshot>, Pointer<mln_diagnostic>)
>()
external int mln_map_snapshot_get(
  int map,
  Pointer<mln_map_snapshot> out_snapshot,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_map, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_map_style_url(
  int map,
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
external int mln_metal_borrowed_texture_attach(
  int map,
  Pointer<mln_metal_borrowed_texture_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_metal_borrowed_texture_descriptor Function()>()
external mln_metal_borrowed_texture_descriptor
mln_metal_borrowed_texture_descriptor_default();

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_metal_borrowed_texture_descriptor>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_metal_borrowed_texture_set_target(
  int session,
  Pointer<mln_metal_borrowed_texture_descriptor> descriptor,
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
external int mln_metal_owned_texture_attach(
  int map,
  Pointer<mln_metal_owned_texture_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_metal_owned_texture_descriptor Function()>()
external mln_metal_owned_texture_descriptor
mln_metal_owned_texture_descriptor_default();

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
external int mln_metal_surface_attach(
  int map,
  Pointer<mln_metal_surface_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_metal_surface_descriptor Function()>()
external mln_metal_surface_descriptor mln_metal_surface_descriptor_default();

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_metal_surface_descriptor>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_metal_surface_set_target(
  int session,
  Pointer<mln_metal_surface_descriptor> descriptor,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Int32 Function(Pointer<Uint32>, Pointer<mln_diagnostic>)>()
external int mln_network_status_get(
  Pointer<Uint32> out_status,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Int32 Function(Uint32, Pointer<mln_diagnostic>)>()
external int mln_network_status_set(
  int status,
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
external int mln_opengl_borrowed_texture_attach(
  int map,
  Pointer<mln_opengl_borrowed_texture_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_opengl_borrowed_texture_descriptor Function()>()
external mln_opengl_borrowed_texture_descriptor
mln_opengl_borrowed_texture_descriptor_default();

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_opengl_borrowed_texture_descriptor>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_opengl_borrowed_texture_set_target(
  int session,
  Pointer<mln_opengl_borrowed_texture_descriptor> descriptor,
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
external int mln_opengl_owned_texture_attach(
  int map,
  Pointer<mln_opengl_owned_texture_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_opengl_owned_texture_descriptor Function()>()
external mln_opengl_owned_texture_descriptor
mln_opengl_owned_texture_descriptor_default();

@Native<Uint32 Function()>()
external int mln_opengl_supported_context_provider_mask();

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
external int mln_opengl_surface_attach(
  int map,
  Pointer<mln_opengl_surface_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_opengl_surface_descriptor Function()>()
external mln_opengl_surface_descriptor mln_opengl_surface_descriptor_default();

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_opengl_surface_descriptor>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_opengl_surface_set_target(
  int session,
  Pointer<mln_opengl_surface_descriptor> descriptor,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

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
  Int32 Function(mln_render_frame_batch, Pointer<Size>, Pointer<mln_diagnostic>)
>()
external int mln_render_frame_batch_count(
  int batch,
  Pointer<Size> out_count,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_frame_batch,
    Size,
    Pointer<mln_render_frame_result>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_frame_batch_get(
  int batch,
  int index,
  Pointer<mln_render_frame_result> out_result,
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

@Native<Int32 Function(mln_render_session, Pointer<mln_diagnostic>)>()
external int mln_render_session_destroy(
  int session,
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

@Native<Int32 Function(mln_render_session, Pointer<mln_diagnostic>)>()
external int mln_render_session_dispose(
  int session,
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
    Pointer<mln_map_projection>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_projection_create(
  int session,
  Pointer<mln_map_projection> out_projection,
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
    Pointer<mln_render_target_extent>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_resize(
  int session,
  Pointer<mln_render_target_extent> extent,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_render_session,
    Size,
    Pointer<Size>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_session_service_driver_work(
  int session,
  int max_work,
  Pointer<Size> out_serviced,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    Pointer<mln_render_target_extent>,
    Pointer<Uint32>,
    Pointer<Uint32>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_render_target_extent_physical_size(
  Pointer<mln_render_target_extent> extent,
  Pointer<Uint32> out_width,
  Pointer<Uint32> out_height,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_rendered_feature_query_options Function()>()
external mln_rendered_feature_query_options
mln_rendered_feature_query_options_default();

@Native<mln_rendered_query_geometry Function(mln_screen_box)>()
external mln_rendered_query_geometry mln_rendered_query_geometry_box(
  mln_screen_box box,
);

@Native<mln_rendered_query_geometry Function(Pointer<mln_screen_point>, Size)>()
external mln_rendered_query_geometry mln_rendered_query_geometry_line_string(
  Pointer<mln_screen_point> points,
  int point_count,
);

@Native<mln_rendered_query_geometry Function(mln_screen_point)>()
external mln_rendered_query_geometry mln_rendered_query_geometry_point(
  mln_screen_point point,
);

@Native<
  Int32 Function(
    mln_resource_request_handle,
    Pointer<Bool>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_resource_request_cancelled(
  int handle,
  Pointer<Bool> out_cancelled,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_resource_request_handle,
    Pointer<mln_resource_response>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_resource_request_complete(
  int handle,
  Pointer<mln_resource_response> response,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Void Function(mln_resource_request_handle)>()
external void mln_resource_request_release(int handle);

@Native<
  Int32 Function(
    mln_resource_request_handle,
    mln_resource_request_cancel_callback,
    Pointer<Void>,
    mln_runtime_callback_release,
    Pointer<Bool>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_resource_request_set_cancel_callback(
  int handle,
  mln_resource_request_cancel_callback callback,
  Pointer<Void> user_data,
  mln_runtime_callback_release release_user_data,
  Pointer<Bool> out_cancelled,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<Int32 Function(mln_resource_request_handle, Pointer<mln_diagnostic>)>()
external int mln_resource_request_wait_until_retired(
  int handle,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    Pointer<mln_resource_transform_response>,
    Pointer<Char>,
    Size,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_resource_transform_response_set_url(
  Pointer<mln_resource_transform_response> response,
  Pointer<Char> url,
  int url_size,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_runtime, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_runtime_barrier(
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

@Native<Int32 Function(mln_runtime, Pointer<mln_diagnostic>)>()
external int mln_runtime_dispose(
  int runtime,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_runtime, Pointer<mln_event_batch>, Pointer<mln_diagnostic>)
>()
external int mln_runtime_drain_events(
  int runtime,
  Pointer<mln_event_batch> out_batch,
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
    Pointer<mln_offline_region_definition>,
    Pointer<Uint8>,
    Size,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_offline_region_create(
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
external int mln_runtime_offline_region_delete(
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
external int mln_runtime_offline_region_get(
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
external int mln_runtime_offline_region_get_status(
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
external int mln_runtime_offline_region_invalidate(
  int runtime,
  int region_id,
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
external int mln_runtime_offline_region_set_download_state(
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
external int mln_runtime_offline_region_set_observed(
  int runtime,
  int region_id,
  bool observed,
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
external int mln_runtime_offline_region_update_metadata(
  int runtime,
  int region_id,
  Pointer<Uint8> metadata,
  int metadata_size,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(mln_runtime, Pointer<mln_completion>, Pointer<mln_diagnostic>)
>()
external int mln_runtime_offline_regions_list(
  int runtime,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_runtime,
    Pointer<Char>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_runtime_offline_regions_merge_database(
  int runtime,
  Pointer<Char> side_database_path,
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

@Native<mln_source_feature_query_options Function()>()
external mln_source_feature_query_options
mln_source_feature_query_options_default();

@Native<mln_style_image_info Function()>()
external mln_style_image_info mln_style_image_info_default();

@Native<mln_style_image_options Function()>()
external mln_style_image_options mln_style_image_options_default();

@Native<mln_style_tile_source_options Function()>()
external mln_style_tile_source_options mln_style_tile_source_options_default();

@Native<mln_style_transition_options Function()>()
external mln_style_transition_options mln_style_transition_options_default();

@Native<Uint32 Function()>()
external int mln_supported_render_backend_mask();

@Native<mln_texture_image_info Function()>()
external mln_texture_image_info mln_texture_image_info_default();

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_texture_read_premultiplied_rgba8(
  int session,
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
external int mln_vulkan_borrowed_texture_attach(
  int map,
  Pointer<mln_vulkan_borrowed_texture_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_vulkan_borrowed_texture_descriptor Function()>()
external mln_vulkan_borrowed_texture_descriptor
mln_vulkan_borrowed_texture_descriptor_default();

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_vulkan_borrowed_texture_descriptor>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_vulkan_borrowed_texture_set_target(
  int session,
  Pointer<mln_vulkan_borrowed_texture_descriptor> descriptor,
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
external int mln_vulkan_owned_texture_attach(
  int map,
  Pointer<mln_vulkan_owned_texture_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_vulkan_owned_texture_descriptor Function()>()
external mln_vulkan_owned_texture_descriptor
mln_vulkan_owned_texture_descriptor_default();

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
external int mln_vulkan_surface_attach(
  int map,
  Pointer<mln_vulkan_surface_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_vulkan_surface_descriptor Function()>()
external mln_vulkan_surface_descriptor mln_vulkan_surface_descriptor_default();

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_vulkan_surface_descriptor>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_vulkan_surface_set_target(
  int session,
  Pointer<mln_vulkan_surface_descriptor> descriptor,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_webgpu_borrowed_texture_descriptor>,
    Pointer<mln_render_session_attach_options>,
    Pointer<mln_render_session>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_webgpu_borrowed_texture_attach(
  int map,
  Pointer<mln_webgpu_borrowed_texture_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_webgpu_borrowed_texture_descriptor Function()>()
external mln_webgpu_borrowed_texture_descriptor
mln_webgpu_borrowed_texture_descriptor_default();

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_webgpu_borrowed_texture_descriptor>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_webgpu_borrowed_texture_set_target(
  int session,
  Pointer<mln_webgpu_borrowed_texture_descriptor> descriptor,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_webgpu_owned_texture_descriptor>,
    Pointer<mln_render_session_attach_options>,
    Pointer<mln_render_session>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_webgpu_owned_texture_attach(
  int map,
  Pointer<mln_webgpu_owned_texture_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_webgpu_owned_texture_descriptor Function()>()
external mln_webgpu_owned_texture_descriptor
mln_webgpu_owned_texture_descriptor_default();

@Native<
  Int32 Function(
    mln_map,
    Pointer<mln_webgpu_surface_descriptor>,
    Pointer<mln_render_session_attach_options>,
    Pointer<mln_render_session>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_webgpu_surface_attach(
  int map,
  Pointer<mln_webgpu_surface_descriptor> descriptor,
  Pointer<mln_render_session_attach_options> options,
  Pointer<mln_render_session> out_session,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);

@Native<mln_webgpu_surface_descriptor Function()>()
external mln_webgpu_surface_descriptor mln_webgpu_surface_descriptor_default();

@Native<
  Int32 Function(
    mln_render_session,
    Pointer<mln_webgpu_surface_descriptor>,
    Pointer<mln_completion>,
    Pointer<mln_diagnostic>,
  )
>()
external int mln_webgpu_surface_set_target(
  int session,
  Pointer<mln_webgpu_surface_descriptor> descriptor,
  Pointer<mln_completion> completion,
  Pointer<mln_diagnostic> out_diagnostic,
);
