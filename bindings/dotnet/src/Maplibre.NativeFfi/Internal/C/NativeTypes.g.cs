// Generated from the C headers by tools/bindgen. Do not edit.
using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C;

internal readonly record struct MlnAcquiredFrame(ulong Value) : IMlnHandle;

internal readonly record struct MlnDeliveryQueue(ulong Value) : IMlnHandle;

internal readonly record struct MlnGeoJsonSourceData(ulong Value) : IMlnHandle;

internal readonly record struct MlnGraphicsDevice(ulong Value) : IMlnHandle;

internal readonly record struct MlnMap(ulong Value) : IMlnHandle;

internal readonly record struct MlnMapProjection(ulong Value) : IMlnHandle;

internal readonly record struct MlnRenderFrameBatch(ulong Value) : IMlnHandle;

internal readonly record struct MlnRenderSession(ulong Value) : IMlnHandle;

internal readonly record struct MlnResourceRequest(ulong Value) : IMlnHandle;

internal readonly record struct MlnResourceTransformRequest(ulong Value) : IMlnHandle;

internal readonly record struct MlnRuntime(ulong Value) : IMlnHandle;

internal readonly record struct MlnStyleExpression(ulong Value) : IMlnHandle;

internal unsafe struct mln_animation_options
{
    public mln_animation_option_field fields;
    public double duration_ms;
    public double velocity;
    public double min_zoom;
    public mln_unit_bezier easing;
    public ulong transition_id;
    public mln_camera_transition_handler end_handler;
}

internal unsafe struct mln_bound_options
{
    public uint size;
    public mln_bound_option_field fields;
    public mln_lat_lng_bounds bounds;
    public double min_zoom;
    public double max_zoom;
    public double min_pitch;
    public double max_pitch;
}

internal unsafe struct mln_buffer_view
{
    public void* data;
    public nuint size;
}

internal unsafe struct mln_camera
{
    public mln_lat_lng center;
    public double center_altitude;
    public mln_edge_insets padding;
    public double zoom;
    public double bearing;
    public double pitch;
    public double roll;
    public double field_of_view;
}

internal unsafe struct mln_camera_delta
{
    public uint size;
    public mln_camera_delta_field fields;
    public mln_screen_point offset;
    public double scale;
    public double bearing;
    public double pitch;
    public mln_screen_point anchor;
    public mln_animation_options animation;
    public mln_gesture_phase gesture_phase;
    public uint reserved;
}

internal unsafe struct mln_camera_fit_options
{
    public uint size;
    public mln_camera_fit_option_field fields;
    public mln_edge_insets padding;
    public double bearing;
    public double pitch;
}

internal unsafe struct mln_camera_options
{
    public uint size;
    public mln_camera_option_field fields;
    public mln_lat_lng center;
    public double center_altitude;
    public mln_edge_insets padding;
    public mln_screen_point anchor;
    public double zoom;
    public double bearing;
    public double pitch;
    public double roll;
    public double field_of_view;
}

internal unsafe struct mln_camera_query_result
{
    public ulong generation;
    public mln_camera camera;
}

internal unsafe struct mln_camera_transition_end
{
    public uint size;
    public mln_camera_transition_outcome outcome;
    public ulong generation;
}

internal unsafe struct mln_camera_transition_handler
{
    public delegate* unmanaged[Cdecl]<void*, mln_camera_transition_end*, void> callback;
    public void* user_data;
    public delegate* unmanaged[Cdecl]<void*, void> release_user_data;
    public MlnDeliveryQueue queue;
}

internal unsafe struct mln_camera_update
{
    public uint size;
    public mln_camera_update_mode mode;
    public mln_camera_options camera;
    public mln_animation_options animation;
    public mln_gesture_phase gesture_phase;
    public uint reserved;
}

internal unsafe struct mln_canonical_tile_id
{
    public uint z;
    public uint x;
    public uint y;
}

internal unsafe struct mln_completion
{
    public uint size;
    public delegate* unmanaged[Cdecl]<void*, mln_completion_result*, void> callback;
    public void* user_data;
    public delegate* unmanaged[Cdecl]<void*, void> release_user_data;
    public MlnDeliveryQueue queue;
}

internal unsafe struct mln_completion_result
{
    public uint size;
    public mln_status status;
    public mln_command_disposition disposition;
    public uint value_size;
    public ulong generation;
    public ulong render_update_generation;
    public mln_buffer_view diagnostic;
    public void* value;
    public nuint value_count;
}

internal unsafe struct mln_custom_geometry_source_options
{
    public uint size;
    public mln_custom_geometry_source_option_field fields;
    public delegate* unmanaged[Cdecl]<void*, mln_canonical_tile_id, void> fetch_tile;
    public delegate* unmanaged[Cdecl]<void*, mln_canonical_tile_id, void> cancel_tile;
    public void* user_data;
    public double min_zoom;
    public double max_zoom;
    public double tolerance;
    public uint tile_size;
    public uint buffer;
    public byte clip;
    public byte wrap;
    public delegate* unmanaged[Cdecl]<void*, void> release_user_data;
    public MlnDeliveryQueue queue;
}

internal unsafe struct mln_custom_vector_source_options
{
    public uint size;
    public mln_custom_vector_source_option_field fields;
    public delegate* unmanaged[Cdecl]<void*, mln_canonical_tile_id, void> fetch_tile;
    public delegate* unmanaged[Cdecl]<void*, mln_canonical_tile_id, void> cancel_tile;
    public void* user_data;
    public double min_zoom;
    public double max_zoom;
    public delegate* unmanaged[Cdecl]<void*, void> release_user_data;
    public MlnDeliveryQueue queue;
}

internal unsafe struct mln_diagnostic
{
    public uint size;
    public MessageArray message;

    [System.Runtime.CompilerServices.InlineArray(4096)]
    public struct MessageArray
    {
        private sbyte element;
    }
}

internal unsafe struct mln_edge_insets
{
    public double top;
    public double left;
    public double bottom;
    public double right;
}

internal unsafe struct mln_egl_context_descriptor
{
    public void* display;
    public void* config;
    public void* share_context;
    public mln_open_gl_client_api client_api;
    public void* get_proc_address;
}

internal unsafe struct mln_event_batch_view
{
    public uint size;
    public uint event_size;
    public mln_runtime_event* events;
    public nuint event_count;
}

internal unsafe struct mln_feature_state_selector
{
    public uint size;
    public mln_feature_state_selector_field fields;
    public mln_buffer_view source_id;
    public mln_buffer_view source_layer_id;
    public mln_buffer_view feature_id;
    public mln_buffer_view state_key;
}

internal unsafe struct mln_frame_demand
{
    public uint size;
    public uint flags;
    public ulong token;
}

internal unsafe struct mln_free_camera_options
{
    public uint size;
    public mln_free_camera_option_field fields;
    public mln_vec3 position;
    public mln_quaternion orientation;
}

internal unsafe struct mln_geo_json_source_options
{
    public uint size;
    public mln_geo_json_source_option_field fields;
    public double min_zoom;
    public double max_zoom;
    public double tolerance;
    public double cluster_max_zoom;
    public mln_buffer_view cluster_properties;
    public uint buffer;
    public uint cluster_radius;
    public uint cluster_min_points;
    public byte line_metrics;
    public byte cluster;
    public byte synchronous_tiling;
}

internal unsafe struct mln_gpu_sync
{
    public uint size;
    public mln_gpu_sync_kind kind;
    public ulong handle;
}

internal unsafe struct mln_graphics_device_options
{
    public uint size;
    public mln_graphics_device_option_field fields;
    public uint adapter_index;
    public byte single_context;
}

internal unsafe struct mln_graphics_device_texture_descriptor
{
    public uint size;
    public mln_logical_extent extent;
}

internal unsafe struct mln_http_header
{
    public mln_buffer_view name;
    public mln_buffer_view value;
}

internal unsafe struct mln_http_header_rule
{
    public uint size;
    public uint kinds;
    public uint flags;
    public mln_buffer_view url;
    public mln_http_header* headers;
    public nuint header_count;
}

internal unsafe struct mln_http_header_transform
{
    public uint size;
    public delegate* unmanaged[Cdecl]<
        void*,
        mln_resource_kind,
        mln_buffer_view,
        mln_http_header_transform_response*,
        mln_status> callback;
    public void* user_data;
    public delegate* unmanaged[Cdecl]<void*, void> release_user_data;
    public mln_http_header_rule* rules;
    public nuint rule_count;
}

internal unsafe struct mln_http_header_transform_response
{
    public uint size;
    public void* context;
}

internal unsafe struct mln_image_content
{
    public float left;
    public float top;
    public float right;
    public float bottom;
}

internal unsafe struct mln_image_stretch
{
    public float from;
    public float to;
}

internal unsafe struct mln_lat_lng
{
    public double latitude;
    public double longitude;
}

internal unsafe struct mln_lat_lng_bounds
{
    public mln_lat_lng southwest;
    public mln_lat_lng northeast;
}

internal unsafe struct mln_log_handler
{
    public uint size;
    public delegate* unmanaged[Cdecl]<
        void*,
        mln_log_severity,
        mln_log_event,
        long,
        mln_buffer_view,
        byte> callback;
    public void* user_data;
    public delegate* unmanaged[Cdecl]<void*, void> release_user_data;
    public MlnDeliveryQueue queue;
}

internal unsafe struct mln_logical_extent
{
    public uint width;
    public uint height;
    public double scale_factor;
}

internal unsafe struct mln_map_options
{
    public uint size;
    public mln_map_option_field fields;
    public mln_logical_extent initial_extent;
    public mln_map_mode map_mode;
    public byte fast_pfor_enabled;
    public ulong event_mask;
}

internal unsafe struct mln_map_snapshot
{
    public uint size;
    public uint debug_options;
    public ulong generation;
    public byte fully_loaded;
    public byte rendering_stats_view_enabled;
    public byte gesture_in_progress;
    public byte transition_in_progress;
    public ulong event_mask;
    public ulong latest_render_update_generation;
    public mln_map_tile_options tile;
    public mln_bound_options bounds;
    public mln_free_camera_options free_camera;
    public mln_map_view view;
}

internal unsafe struct mln_map_tile_options
{
    public uint size;
    public mln_map_tile_option_field fields;
    public uint prefetch_zoom_delta;
    public double lod_min_radius;
    public double lod_scale;
    public double lod_pitch_threshold;
    public double lod_zoom_shift;
    public mln_tile_lod_mode lod_mode;
}

internal unsafe struct mln_map_view
{
    public uint size;
    public mln_camera camera;
    public mln_logical_extent extent;
    public mln_projection_mode projection_mode;
    public mln_map_viewport_options viewport;
}

internal unsafe struct mln_map_viewport_options
{
    public uint size;
    public mln_map_viewport_option_field fields;
    public mln_north_orientation north_orientation;
    public mln_constrain_mode constrain_mode;
    public mln_viewport_mode viewport_mode;
    public mln_edge_insets frustum_offset;
}

internal unsafe struct mln_metal_borrowed_texture
{
    public void* texture;
}

internal unsafe struct mln_metal_borrowed_texture_descriptor
{
    public uint size;
    public mln_logical_extent extent;
    public uint physical_width;
    public uint physical_height;
    public mln_metal_borrowed_texture* textures;
    public nuint texture_count;
}

internal unsafe struct mln_metal_context_descriptor
{
    public void* device;
}

internal unsafe struct mln_metal_owned_texture_descriptor
{
    public uint size;
    public mln_logical_extent extent;
    public mln_metal_context_descriptor context;
    public ulong pixel_format;
}

internal unsafe struct mln_metal_surface_descriptor
{
    public uint size;
    public mln_logical_extent extent;
    public mln_metal_context_descriptor context;
    public void* layer;
}

internal unsafe struct mln_metal_texture_frame
{
    public uint size;
    public ulong generation;
    public uint width;
    public uint height;
    public double scale_factor;
    public ulong frame_id;
    public uint slot;
    public void* texture;
    public void* device;
    public ulong pixel_format;
}

internal unsafe struct mln_offline_geometry_region_definition
{
    public mln_buffer_view style_url;
    public mln_buffer_view geometry;
    public double min_zoom;
    public double max_zoom;
    public float pixel_ratio;
    public byte include_ideographs;
}

internal unsafe struct mln_offline_region_definition
{
    public uint size;
    public mln_offline_region_definition_type type;
    public mln_offline_region_definition_data data;
}

[StructLayout(LayoutKind.Explicit)]
internal unsafe struct mln_offline_region_definition_data
{
    [FieldOffset(0)]
    public mln_offline_tile_pyramid_region_definition tile_pyramid;

    [FieldOffset(0)]
    public mln_offline_geometry_region_definition geometry;
}

internal unsafe struct mln_offline_region_info
{
    public long id;
    public mln_offline_region_definition definition;
    public byte* metadata;
    public nuint metadata_size;
}

internal unsafe struct mln_offline_region_status
{
    public mln_offline_region_download_state download_state;
    public ulong completed_resource_count;
    public ulong completed_resource_size;
    public ulong completed_tile_count;
    public ulong required_tile_count;
    public ulong completed_tile_size;
    public ulong required_resource_count;
    public byte required_resource_count_is_precise;
    public byte complete;
}

internal unsafe struct mln_offline_tile_pyramid_region_definition
{
    public mln_buffer_view style_url;
    public mln_lat_lng_bounds bounds;
    public double min_zoom;
    public double max_zoom;
    public float pixel_ratio;
    public byte include_ideographs;
}

internal unsafe struct mln_open_gl_borrowed_texture
{
    public uint texture;
}

internal unsafe struct mln_open_gl_borrowed_texture_descriptor
{
    public uint size;
    public mln_logical_extent extent;
    public uint physical_width;
    public uint physical_height;
    public mln_open_gl_context_descriptor context;
    public mln_open_gl_borrowed_texture* textures;
    public nuint texture_count;
    public uint target;
}

internal unsafe struct mln_open_gl_context_descriptor
{
    public mln_open_gl_context_platform platform;
    public mln_open_gl_context_ownership ownership;
    public mln_open_gl_context_descriptor_data data;
}

[StructLayout(LayoutKind.Explicit)]
internal unsafe struct mln_open_gl_context_descriptor_data
{
    [FieldOffset(0)]
    public mln_wgl_context_descriptor wgl;

    [FieldOffset(0)]
    public mln_egl_context_descriptor egl;

    [FieldOffset(0)]
    public mln_web_gl_context_descriptor web_gl;
}

internal unsafe struct mln_open_gl_owned_texture_descriptor
{
    public uint size;
    public mln_logical_extent extent;
    public mln_open_gl_context_descriptor context;
}

internal unsafe struct mln_open_gl_surface_descriptor
{
    public uint size;
    public mln_logical_extent extent;
    public mln_open_gl_context_descriptor context;
    public void* surface;
}

internal unsafe struct mln_open_gl_texture_frame
{
    public uint size;
    public ulong generation;
    public uint width;
    public uint height;
    public double scale_factor;
    public ulong frame_id;
    public uint slot;
    public uint texture;
    public uint target;
    public uint internal_format;
    public uint format;
    public uint type;
}

internal unsafe struct mln_premultiplied_rgba8_image
{
    public uint size;
    public uint width;
    public uint height;
    public uint stride;
    public byte* pixels;
    public nuint byte_length;
}

internal unsafe struct mln_projected_meters
{
    public double northing;
    public double easting;
}

internal unsafe struct mln_projection_mode
{
    public uint size;
    public mln_projection_mode_field fields;
    public byte axonometric;
    public double x_skew;
    public double y_skew;
}

internal unsafe struct mln_quaternion
{
    public double x;
    public double y;
    public double z;
    public double w;
}

internal unsafe struct mln_queue_lock
{
    public delegate* unmanaged[Cdecl]<void*, void> @lock;
    public delegate* unmanaged[Cdecl]<void*, void> unlock;
    public void* user_data;
    public delegate* unmanaged[Cdecl]<void*, void> release_user_data;
}

internal unsafe struct mln_render_abandon_result
{
    public uint size;
    public mln_render_abandon_disposition disposition;
    public uint quarantined_resource_count;
    public uint reserved;
}

internal unsafe struct mln_render_frame_batch_view
{
    public uint size;
    public uint result_size;
    public mln_render_frame_result* results;
    public nuint result_count;
}

internal unsafe struct mln_render_frame_result
{
    public uint size;
    public mln_render_result disposition;
    public ulong token;
    public ulong map_update_generation;
    public ulong generation;
    public ulong frame_generation;
    public byte needs_repaint;
    public mln_map_view view;
}

internal unsafe struct mln_render_session_attach_options
{
    public uint size;
    public mln_render_driver_kind driver;
    public uint requested_texture_ring_depth;
    public mln_gpu_sync_kind producer_sync_kind;
    public mln_wake frame_wake;
    public mln_wake driver_work_wake;
    public mln_queue_lock queue_lock;
}

internal unsafe struct mln_render_session_capabilities
{
    public uint size;
    public mln_render_driver_kind driver;
    public uint texture_ring_depth;
    public uint flags;
}

internal unsafe struct mln_render_session_snapshot
{
    public uint size;
    public mln_render_session_state state;
    public mln_logical_extent extent;
    public ulong generation;
    public ulong map_update_generation;
    public ulong rendered_update_generation;
    public ulong frame_generation;
    public uint acquired_frame_count;
    public byte pending_changes;
}

internal unsafe struct mln_rendered_feature
{
    public mln_buffer_view feature;
    public mln_buffer_view layer_id;
    public mln_buffer_view source_id;
    public mln_buffer_view source_layer_id;
    public mln_buffer_view state;
}

internal unsafe struct mln_rendered_feature_query_options
{
    public uint size;
    public mln_rendered_feature_query_option_field fields;
    public mln_buffer_view* layer_ids;
    public nuint layer_id_count;
    public mln_buffer_view filter;
}

internal unsafe struct mln_rendered_query_geometry
{
    public uint size;
    public mln_rendered_query_geometry_type type;
    public mln_rendered_query_geometry_data data;
}

[StructLayout(LayoutKind.Explicit)]
internal unsafe struct mln_rendered_query_geometry_data
{
    [FieldOffset(0)]
    public mln_screen_point point;

    [FieldOffset(0)]
    public mln_screen_box box;

    [FieldOffset(0)]
    public mln_screen_line_string line_string;
}

internal unsafe struct mln_rendering_stats
{
    public double encoding_time;
    public double rendering_time;
    public long frame_count;
    public long draw_call_count;
    public long total_draw_call_count;
}

internal unsafe struct mln_resource_provider
{
    public uint size;
    public delegate* unmanaged[Cdecl]<
        void*,
        mln_resource_request_info*,
        MlnResourceRequest,
        void> callback;
    public void* user_data;
    public delegate* unmanaged[Cdecl]<void*, void> release_user_data;
    public MlnDeliveryQueue queue;
    public mln_resource_route* routes;
    public nuint route_count;
}

internal unsafe struct mln_resource_range
{
    public ulong start;
    public ulong end;
}

internal unsafe struct mln_resource_request_cancel_handler
{
    public uint size;
    public delegate* unmanaged[Cdecl]<void*, MlnResourceRequest, void> callback;
    public void* user_data;
    public delegate* unmanaged[Cdecl]<void*, void> release_user_data;
    public MlnDeliveryQueue queue;
}

internal unsafe struct mln_resource_request_info
{
    public uint size;
    public mln_resource_request_field fields;
    public mln_buffer_view requested_url;
    public mln_buffer_view resolved_url;
    public mln_resource_kind kind;
    public mln_resource_loading_method loading_method;
    public mln_resource_priority priority;
    public mln_resource_usage usage;
    public mln_resource_storage_policy storage_policy;
    public mln_resource_range range;
    public long prior_modified_unix_ms;
    public long prior_expires_unix_ms;
    public mln_buffer_view prior_etag;
    public byte* prior_data;
    public nuint prior_data_size;
}

internal unsafe struct mln_resource_response
{
    public uint size;
    public mln_resource_response_field fields;
    public mln_resource_response_status status;
    public mln_resource_error_reason error_reason;
    public byte* bytes;
    public nuint byte_count;
    public mln_buffer_view error_message;
    public byte must_revalidate;
    public long modified_unix_ms;
    public long expires_unix_ms;
    public mln_buffer_view etag;
    public long retry_after_unix_ms;
}

internal unsafe struct mln_resource_route
{
    public uint size;
    public uint kinds;
    public uint flags;
    public mln_buffer_view url;
}

internal unsafe struct mln_resource_transform
{
    public uint size;
    public delegate* unmanaged[Cdecl]<
        void*,
        mln_resource_kind,
        mln_buffer_view,
        MlnResourceTransformRequest,
        void> callback;
    public void* user_data;
    public delegate* unmanaged[Cdecl]<void*, void> release_user_data;
    public MlnDeliveryQueue queue;
}

internal unsafe struct mln_runtime_event
{
    public mln_runtime_event_type type;
    public mln_runtime_event_source_type source_type;
    public ulong source;
    public ulong generation;
    public uint code;
    public mln_runtime_event_payload_type payload_type;
    public mln_buffer_view message;
    public mln_runtime_event_payload payload;
}

internal unsafe struct mln_runtime_event_glyphs
{
    public mln_buffer_view* font_stack;
    public nuint font_count;
    public uint first;
    public uint last;
    public byte font_face;
}

internal unsafe struct mln_runtime_event_handler
{
    public uint size;
    public delegate* unmanaged[Cdecl]<void*, mln_event_batch_view*, void> callback;
    public void* user_data;
    public delegate* unmanaged[Cdecl]<void*, void> release_user_data;
    public MlnDeliveryQueue queue;
}

internal unsafe struct mln_runtime_event_offline_region_response_error
{
    public long region_id;
    public mln_resource_error_reason reason;
}

internal unsafe struct mln_runtime_event_offline_region_status
{
    public long region_id;
    public mln_offline_region_status status;
}

internal unsafe struct mln_runtime_event_offline_region_tile_count_limit
{
    public long region_id;
    public ulong limit;
}

[StructLayout(LayoutKind.Explicit)]
internal unsafe struct mln_runtime_event_payload
{
    [FieldOffset(0)]
    public mln_runtime_event_render_frame render_frame;

    [FieldOffset(0)]
    public mln_runtime_event_render_map render_map;

    [FieldOffset(0)]
    public mln_runtime_event_tile_action tile_action;

    [FieldOffset(0)]
    public mln_runtime_event_offline_region_status offline_region_status;

    [FieldOffset(0)]
    public mln_runtime_event_offline_region_response_error offline_region_response_error;

    [FieldOffset(0)]
    public mln_runtime_event_offline_region_tile_count_limit offline_region_tile_count_limit;

    [FieldOffset(0)]
    public mln_runtime_event_glyphs glyphs;

    [FieldOffset(0)]
    public mln_runtime_event_sprite sprite;
}

internal unsafe struct mln_runtime_event_render_frame
{
    public mln_render_mode mode;
    public byte needs_repaint;
    public byte placement_changed;
    public mln_rendering_stats stats;
}

internal unsafe struct mln_runtime_event_render_map
{
    public mln_render_mode mode;
}

internal unsafe struct mln_runtime_event_sprite
{
    public mln_buffer_view id;
    public mln_buffer_view url;
}

internal unsafe struct mln_runtime_event_tile_action
{
    public mln_tile_operation operation;
    public mln_tile_id tile_id;
}

internal unsafe struct mln_runtime_options
{
    public uint size;
    public mln_buffer_view asset_path;
    public mln_buffer_view cache_path;
}

internal unsafe struct mln_screen_box
{
    public mln_screen_point min;
    public mln_screen_point max;
}

internal unsafe struct mln_screen_line_string
{
    public mln_screen_point* points;
    public nuint point_count;
}

internal unsafe struct mln_screen_point
{
    public double x;
    public double y;
}

internal unsafe struct mln_source_feature
{
    public mln_buffer_view feature;
    public mln_buffer_view source_layer_id;
}

internal unsafe struct mln_source_feature_query_options
{
    public uint size;
    public mln_source_feature_query_option_field fields;
    public mln_buffer_view* source_layer_ids;
    public nuint source_layer_id_count;
    public mln_buffer_view filter;
}

internal unsafe struct mln_style_expression_info
{
    public uint size;
    public byte is_feature_constant;
    public byte is_zoom_constant;
}

internal unsafe struct mln_style_expression_inputs
{
    public uint size;
    public mln_style_expression_input_field fields;
    public double zoom;
    public double color_ramp_parameter;
    public double elevation;
    public mln_buffer_view feature;
    public mln_buffer_view feature_state;
    public mln_buffer_view global_state;
    public mln_canonical_tile_id canonical_tile_id;
    public mln_buffer_view* available_images;
    public nuint available_image_count;
}

internal unsafe struct mln_style_expression_options
{
    public uint size;
    public mln_style_expression_option_field fields;
    public mln_style_expression_type expected_type;
}

internal unsafe struct mln_style_expression_type
{
    public mln_style_expression_type_kind kind;
    public mln_style_expression_type_kind item_kind;
    public uint item_count;
}

internal unsafe struct mln_style_image_info
{
    public uint width;
    public uint height;
    public mln_style_image_info_field fields;
    public mln_buffer_view pixels;
    public mln_image_stretch* stretch_x;
    public nuint stretch_x_count;
    public mln_image_stretch* stretch_y;
    public nuint stretch_y_count;
    public mln_image_content content;
    public mln_style_image_text_fit text_fit_width;
    public mln_style_image_text_fit text_fit_height;
    public float pixel_ratio;
    public byte sdf;
    public byte keep_unused;
}

internal unsafe struct mln_style_image_options
{
    public uint size;
    public mln_style_image_option_field fields;
    public mln_image_stretch* stretch_x;
    public nuint stretch_x_count;
    public mln_image_stretch* stretch_y;
    public nuint stretch_y_count;
    public mln_image_content content;
    public mln_style_image_text_fit text_fit_width;
    public mln_style_image_text_fit text_fit_height;
    public float pixel_ratio;
    public byte sdf;
    public byte keep_unused;
}

internal unsafe struct mln_style_layer_info
{
    public mln_buffer_view id;
    public mln_buffer_view type;
    public mln_buffer_view source_id;
    public mln_buffer_view source_layer;
    public double min_zoom;
    public double max_zoom;
    public mln_style_layer_visibility visibility;
}

internal unsafe struct mln_style_source_info
{
    public mln_buffer_view id;
    public mln_style_source_type type;
    public mln_style_source_info_field fields;
    public byte is_volatile;
    public mln_buffer_view attribution;
    public mln_buffer_view url;
    public mln_style_source_tile_info tile_json;
    public mln_lat_lng_bounds bounds;
    public uint tile_size;
    public mln_style_vector_tile_encoding vector_encoding;
    public mln_style_raster_dem_encoding raster_encoding;
}

internal unsafe struct mln_style_source_tile_info
{
    public mln_buffer_view* tile_urls;
    public nuint tile_url_count;
    public double min_zoom;
    public double max_zoom;
    public mln_style_tile_scheme scheme;
}

internal unsafe struct mln_style_transition_options
{
    public uint size;
    public mln_style_transition_option_field fields;
    public double duration_ms;
    public double delay_ms;
    public byte enable_placement_transitions;
}

internal unsafe struct mln_texture_image_info
{
    public uint width;
    public uint height;
    public uint stride;
    public nuint byte_length;
}

internal unsafe struct mln_texture_readback_result
{
    public mln_buffer_view data;
    public mln_texture_image_info info;
}

internal unsafe struct mln_tile_id
{
    public uint overscaled_z;
    public int wrap;
    public uint canonical_z;
    public uint canonical_x;
    public uint canonical_y;
}

internal unsafe struct mln_unit_bezier
{
    public double x1;
    public double y1;
    public double x2;
    public double y2;
}

internal unsafe struct mln_vec3
{
    public double x;
    public double y;
    public double z;
}

internal unsafe struct mln_vulkan_borrowed_texture
{
    public ulong image;
    public ulong image_view;
}

internal unsafe struct mln_vulkan_borrowed_texture_descriptor
{
    public uint size;
    public mln_logical_extent extent;
    public uint physical_width;
    public uint physical_height;
    public mln_vulkan_context_descriptor context;
    public mln_vulkan_borrowed_texture* textures;
    public nuint texture_count;
    public uint format;
    public uint initial_layout;
    public uint final_layout;
}

internal unsafe struct mln_vulkan_context_descriptor
{
    public void* instance;
    public void* physical_device;
    public void* device;
    public void* graphics_queue;
    public uint graphics_queue_family_index;
    public void* get_instance_proc_addr;
    public void* get_device_proc_addr;
}

internal unsafe struct mln_vulkan_owned_texture_descriptor
{
    public uint size;
    public mln_logical_extent extent;
    public mln_vulkan_context_descriptor context;
}

internal unsafe struct mln_vulkan_surface_descriptor
{
    public uint size;
    public mln_logical_extent extent;
    public mln_vulkan_context_descriptor context;
    public ulong surface;
}

internal unsafe struct mln_vulkan_texture_frame
{
    public uint size;
    public ulong generation;
    public uint width;
    public uint height;
    public double scale_factor;
    public ulong frame_id;
    public uint slot;
    public ulong image;
    public ulong image_view;
    public void* device;
    public uint format;
    public uint layout;
}

internal unsafe struct mln_wake
{
    public delegate* unmanaged[Cdecl]<void*, void> callback;
    public void* user_data;
    public delegate* unmanaged[Cdecl]<void*, void> release_user_data;
    public MlnDeliveryQueue queue;
}

internal unsafe struct mln_web_gl_context_descriptor
{
    public mln_web_gl_context_kind kind;
    public int context;
    public mln_buffer_view canvas_selector;
}

internal unsafe struct mln_web_gpu_borrowed_texture
{
    public void* texture;
    public void* texture_view;
}

internal unsafe struct mln_web_gpu_borrowed_texture_descriptor
{
    public uint size;
    public mln_logical_extent extent;
    public uint physical_width;
    public uint physical_height;
    public mln_web_gpu_context_descriptor context;
    public mln_web_gpu_borrowed_texture* textures;
    public nuint texture_count;
    public uint format;
}

internal unsafe struct mln_web_gpu_context_descriptor
{
    public void* instance;
    public void* device;
    public void* queue;
}

internal unsafe struct mln_web_gpu_owned_texture_descriptor
{
    public uint size;
    public mln_logical_extent extent;
    public mln_web_gpu_context_descriptor context;
}

internal unsafe struct mln_web_gpu_surface_descriptor
{
    public uint size;
    public mln_logical_extent extent;
    public mln_web_gpu_context_descriptor context;
    public void* surface;
    public uint format;
}

internal unsafe struct mln_web_gpu_texture_frame
{
    public uint size;
    public ulong generation;
    public uint width;
    public uint height;
    public double scale_factor;
    public ulong frame_id;
    public uint slot;
    public void* texture;
    public void* texture_view;
    public void* device;
    public uint format;
}

internal unsafe struct mln_wgl_context_descriptor
{
    public void* device_context;
    public void* share_context;
    public void* get_proc_address;
}

internal enum mln_ambient_cache_operation : uint
{
    MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE = 1,
    MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE = 2,
    MLN_AMBIENT_CACHE_OPERATION_INVALIDATE = 3,
    MLN_AMBIENT_CACHE_OPERATION_CLEAR = 4,
}

internal enum mln_animation_option_field : uint
{
    MLN_ANIMATION_OPTION_DURATION = 1,
    MLN_ANIMATION_OPTION_VELOCITY = 2,
    MLN_ANIMATION_OPTION_MIN_ZOOM = 4,
    MLN_ANIMATION_OPTION_EASING = 8,
    MLN_ANIMATION_OPTION_TRANSITION_ID = 16,
}

internal enum mln_bound_option_field : uint
{
    MLN_BOUND_OPTION_BOUNDS = 1,
    MLN_BOUND_OPTION_MIN_ZOOM = 2,
    MLN_BOUND_OPTION_MAX_ZOOM = 4,
    MLN_BOUND_OPTION_MIN_PITCH = 8,
    MLN_BOUND_OPTION_MAX_PITCH = 16,
    MLN_BOUND_OPTION_UNBOUNDED = 32,
}

internal enum mln_camera_change_mode : uint
{
    MLN_CAMERA_CHANGE_MODE_IMMEDIATE = 0,
    MLN_CAMERA_CHANGE_MODE_ANIMATED = 1,
}

internal enum mln_camera_delta_field : uint
{
    MLN_CAMERA_DELTA_OFFSET = 1,
    MLN_CAMERA_DELTA_SCALE = 2,
    MLN_CAMERA_DELTA_BEARING = 4,
    MLN_CAMERA_DELTA_PITCH = 8,
    MLN_CAMERA_DELTA_ANCHOR = 16,
}

internal enum mln_camera_fit_option_field : uint
{
    MLN_CAMERA_FIT_OPTION_PADDING = 1,
    MLN_CAMERA_FIT_OPTION_BEARING = 2,
    MLN_CAMERA_FIT_OPTION_PITCH = 4,
}

internal enum mln_camera_option_field : uint
{
    MLN_CAMERA_OPTION_CENTER = 1,
    MLN_CAMERA_OPTION_ZOOM = 2,
    MLN_CAMERA_OPTION_BEARING = 4,
    MLN_CAMERA_OPTION_PITCH = 8,
    MLN_CAMERA_OPTION_CENTER_ALTITUDE = 16,
    MLN_CAMERA_OPTION_PADDING = 32,
    MLN_CAMERA_OPTION_ANCHOR = 64,
    MLN_CAMERA_OPTION_ROLL = 128,
    MLN_CAMERA_OPTION_FOV = 256,
}

internal enum mln_camera_transition_outcome : uint
{
    MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED = 0,
    MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED = 1,
    MLN_CAMERA_TRANSITION_OUTCOME_CLOSED = 2,
}

internal enum mln_camera_update_mode : uint
{
    MLN_CAMERA_UPDATE_MODE_JUMP = 0,
    MLN_CAMERA_UPDATE_MODE_EASE = 1,
    MLN_CAMERA_UPDATE_MODE_FLY = 2,
}

internal enum mln_command_disposition : uint
{
    MLN_COMMAND_DISPOSITION_COMMITTED = 0,
    MLN_COMMAND_DISPOSITION_FAILED = 1,
    MLN_COMMAND_DISPOSITION_CANCELLED = 2,
}

internal enum mln_constrain_mode : uint
{
    MLN_CONSTRAIN_MODE_NONE = 0,
    MLN_CONSTRAIN_MODE_HEIGHT_ONLY = 1,
    MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT = 2,
    MLN_CONSTRAIN_MODE_SCREEN = 3,
}

internal enum mln_custom_geometry_source_option_field : uint
{
    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM = 1,
    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM = 2,
    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE = 4,
    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE = 8,
    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER = 16,
    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP = 32,
    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP = 64,
}

internal enum mln_custom_vector_source_option_field : uint
{
    MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM = 1,
    MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM = 2,
}

internal enum mln_feature_state_selector_field : uint
{
    MLN_FEATURE_STATE_SELECTOR_FEATURE_ID = 1,
}

internal enum mln_frame_demand_flag : uint
{
    MLN_FRAME_DEMAND_FORCE = 1,
    MLN_FRAME_DEMAND_PRESENT = 2,
    MLN_FRAME_DEMAND_WAIT_FOR_UPDATE = 4,
}

internal enum mln_free_camera_option_field : uint
{
    MLN_FREE_CAMERA_OPTION_POSITION = 1,
    MLN_FREE_CAMERA_OPTION_ORIENTATION = 2,
}

internal enum mln_geo_json_source_option_field : uint
{
    MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM = 1,
    MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM = 2,
    MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE = 4,
    MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM = 8,
    MLN_GEO_JSON_SOURCE_OPTION_BUFFER = 16,
    MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS = 32,
    MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS = 64,
    MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS = 128,
    MLN_GEO_JSON_SOURCE_OPTION_CLUSTER = 256,
    MLN_GEO_JSON_SOURCE_OPTION_SYNCHRONOUS_TILING = 512,
}

internal enum mln_gesture_phase : uint
{
    MLN_GESTURE_PHASE_NONE = 0,
    MLN_GESTURE_PHASE_BEGIN = 1,
    MLN_GESTURE_PHASE_UPDATE = 2,
    MLN_GESTURE_PHASE_END = 3,
    MLN_GESTURE_PHASE_CANCEL = 4,
}

internal enum mln_gpu_sync_kind : uint
{
    MLN_GPU_SYNC_CPU_COMPLETE = 0,
    MLN_GPU_SYNC_OPEN_GL_FENCE = 1,
}

internal enum mln_graphics_device_option_field : uint
{
    MLN_GRAPHICS_DEVICE_OPTION_ADAPTER_INDEX = 1,
    MLN_GRAPHICS_DEVICE_OPTION_SINGLE_CONTEXT = 2,
}

internal enum mln_log_event : uint
{
    MLN_LOG_EVENT_GENERAL = 0,
    MLN_LOG_EVENT_SETUP = 1,
    MLN_LOG_EVENT_SHADER = 2,
    MLN_LOG_EVENT_PARSE_STYLE = 3,
    MLN_LOG_EVENT_PARSE_TILE = 4,
    MLN_LOG_EVENT_RENDER = 5,
    MLN_LOG_EVENT_STYLE = 6,
    MLN_LOG_EVENT_DATABASE = 7,
    MLN_LOG_EVENT_HTTP_REQUEST = 8,
    MLN_LOG_EVENT_SPRITE = 9,
    MLN_LOG_EVENT_IMAGE = 10,
    MLN_LOG_EVENT_GRAPHICS_BACKEND = 11,
    MLN_LOG_EVENT_JNI = 12,
    MLN_LOG_EVENT_ANDROID = 13,
    MLN_LOG_EVENT_CRASH = 14,
    MLN_LOG_EVENT_GLYPH = 15,
    MLN_LOG_EVENT_TIMING = 16,
}

internal enum mln_log_severity : uint
{
    MLN_LOG_SEVERITY_INFO = 1,
    MLN_LOG_SEVERITY_WARNING = 2,
    MLN_LOG_SEVERITY_ERROR = 3,
}

internal enum mln_log_severity_flag : uint
{
    MLN_LOG_SEVERITY_FLAG_INFO = 2,
    MLN_LOG_SEVERITY_FLAG_WARNING = 4,
    MLN_LOG_SEVERITY_FLAG_ERROR = 8,
    MLN_LOG_SEVERITY_FLAG_DEFAULT = 6,
    MLN_LOG_SEVERITY_FLAG_ALL = 14,
}

internal enum mln_map_debug_flag : uint
{
    MLN_MAP_DEBUG_TILE_BORDERS = 2,
    MLN_MAP_DEBUG_PARSE_STATUS = 4,
    MLN_MAP_DEBUG_TIMESTAMPS = 8,
    MLN_MAP_DEBUG_COLLISION = 16,
    MLN_MAP_DEBUG_OVERDRAW = 32,
    MLN_MAP_DEBUG_STENCIL_CLIP = 64,
    MLN_MAP_DEBUG_DEPTH_BUFFER = 128,
}

internal enum mln_map_mode : uint
{
    MLN_MAP_MODE_CONTINUOUS = 0,
    MLN_MAP_MODE_STATIC = 1,
    MLN_MAP_MODE_TILE = 2,
}

internal enum mln_map_option_field : uint
{
    MLN_MAP_OPTION_EVENT_MASK = 1,
}

internal enum mln_map_tile_option_field : uint
{
    MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA = 1,
    MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS = 2,
    MLN_MAP_TILE_OPTION_LOD_SCALE = 4,
    MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD = 8,
    MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT = 16,
    MLN_MAP_TILE_OPTION_LOD_MODE = 32,
}

internal enum mln_map_viewport_option_field : uint
{
    MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION = 1,
    MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE = 2,
    MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE = 4,
    MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET = 8,
}

internal enum mln_network_status : uint
{
    MLN_NETWORK_STATUS_ONLINE = 1,
    MLN_NETWORK_STATUS_OFFLINE = 2,
}

internal enum mln_north_orientation : uint
{
    MLN_NORTH_ORIENTATION_UP = 0,
    MLN_NORTH_ORIENTATION_RIGHT = 1,
    MLN_NORTH_ORIENTATION_DOWN = 2,
    MLN_NORTH_ORIENTATION_LEFT = 3,
}

internal enum mln_offline_region_definition_type : uint
{
    MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID = 1,
    MLN_OFFLINE_REGION_DEFINITION_GEOMETRY = 2,
}

internal enum mln_offline_region_download_state : uint
{
    MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE = 0,
    MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE = 1,
}

internal enum mln_open_gl_client_api : uint
{
    MLN_OPEN_GL_CLIENT_API_UNSPECIFIED = 0,
    MLN_OPEN_GL_CLIENT_API_GL = 1,
    MLN_OPEN_GL_CLIENT_API_GLES = 2,
}

internal enum mln_open_gl_context_ownership : uint
{
    MLN_OPEN_GL_CONTEXT_OWNERSHIP_SHARED = 0,
    MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED = 1,
}

internal enum mln_open_gl_context_platform : uint
{
    MLN_OPEN_GL_CONTEXT_PLATFORM_UNSPECIFIED = 0,
    MLN_OPEN_GL_CONTEXT_PLATFORM_WGL = 1,
    MLN_OPEN_GL_CONTEXT_PLATFORM_EGL = 2,
    MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL = 3,
}

internal enum mln_open_gl_context_provider_flag : uint
{
    MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WGL = 1,
    MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_EGL = 2,
    MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WEB_GL = 4,
}

internal enum mln_projection_mode_field : uint
{
    MLN_PROJECTION_MODE_AXONOMETRIC = 1,
    MLN_PROJECTION_MODE_X_SKEW = 2,
    MLN_PROJECTION_MODE_Y_SKEW = 4,
}

internal enum mln_render_abandon_disposition : uint
{
    MLN_RENDER_ABANDON_DISPOSITION_CLEAN = 0,
    MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED = 1,
}

internal enum mln_render_backend_flag : uint
{
    MLN_RENDER_BACKEND_FLAG_METAL = 1,
    MLN_RENDER_BACKEND_FLAG_VULKAN = 2,
    MLN_RENDER_BACKEND_FLAG_OPEN_GL = 4,
    MLN_RENDER_BACKEND_FLAG_WEB_GPU = 8,
}

internal enum mln_render_driver_kind : uint
{
    MLN_RENDER_DRIVER_CORE_WORKER = 1,
    MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD = 2,
}

internal enum mln_render_mode : uint
{
    MLN_RENDER_MODE_PARTIAL = 0,
    MLN_RENDER_MODE_FULL = 1,
}

internal enum mln_render_result : uint
{
    MLN_RENDER_RESULT_RENDERED = 0,
    MLN_RENDER_RESULT_NO_UPDATE = 1,
    MLN_RENDER_RESULT_TARGET_NOT_READY = 2,
    MLN_RENDER_RESULT_SUPERSEDED = 3,
}

internal enum mln_render_session_capability_flag : uint
{
    MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION = 1,
    MLN_RENDER_SESSION_CAPABILITY_READBACK = 2,
    MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC = 4,
    MLN_RENDER_SESSION_CAPABILITY_PRESENTATION = 8,
}

internal enum mln_render_session_state : uint
{
    MLN_RENDER_SESSION_STATE_ATTACHING = 1,
    MLN_RENDER_SESSION_STATE_ATTACHED = 2,
    MLN_RENDER_SESSION_STATE_DETACHING = 3,
    MLN_RENDER_SESSION_STATE_DETACHED = 4,
    MLN_RENDER_SESSION_STATE_TARGET_LOST = 5,
    MLN_RENDER_SESSION_STATE_ABANDONED = 6,
}

internal enum mln_rendered_feature_query_option_field : uint
{
    MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS = 1,
}

internal enum mln_rendered_query_geometry_type : uint
{
    MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT = 1,
    MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX = 2,
    MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING = 3,
}

internal enum mln_resource_error_reason : uint
{
    MLN_RESOURCE_ERROR_REASON_NONE = 0,
    MLN_RESOURCE_ERROR_REASON_NOT_FOUND = 1,
    MLN_RESOURCE_ERROR_REASON_SERVER = 2,
    MLN_RESOURCE_ERROR_REASON_CONNECTION = 3,
    MLN_RESOURCE_ERROR_REASON_RATE_LIMIT = 4,
    MLN_RESOURCE_ERROR_REASON_OTHER = 5,
}

internal enum mln_resource_kind : uint
{
    MLN_RESOURCE_KIND_UNKNOWN = 0,
    MLN_RESOURCE_KIND_STYLE = 1,
    MLN_RESOURCE_KIND_SOURCE = 2,
    MLN_RESOURCE_KIND_TILE = 3,
    MLN_RESOURCE_KIND_GLYPHS = 4,
    MLN_RESOURCE_KIND_SPRITE_IMAGE = 5,
    MLN_RESOURCE_KIND_SPRITE_JSON = 6,
    MLN_RESOURCE_KIND_IMAGE = 7,
}

internal enum mln_resource_kind_flag : uint
{
    MLN_RESOURCE_KIND_FLAG_UNKNOWN = 1,
    MLN_RESOURCE_KIND_FLAG_STYLE = 2,
    MLN_RESOURCE_KIND_FLAG_SOURCE = 4,
    MLN_RESOURCE_KIND_FLAG_TILE = 8,
    MLN_RESOURCE_KIND_FLAG_GLYPHS = 16,
    MLN_RESOURCE_KIND_FLAG_SPRITE_IMAGE = 32,
    MLN_RESOURCE_KIND_FLAG_SPRITE_JSON = 64,
    MLN_RESOURCE_KIND_FLAG_IMAGE = 128,
    MLN_RESOURCE_KIND_FLAG_ALL = 255,
}

internal enum mln_resource_loading_method : uint
{
    MLN_RESOURCE_LOADING_METHOD_ALL = 0,
    MLN_RESOURCE_LOADING_METHOD_CACHE_ONLY = 1,
    MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY = 2,
}

internal enum mln_resource_priority : uint
{
    MLN_RESOURCE_PRIORITY_REGULAR = 0,
    MLN_RESOURCE_PRIORITY_LOW = 1,
}

internal enum mln_resource_request_field : uint
{
    MLN_RESOURCE_REQUEST_RANGE = 1,
    MLN_RESOURCE_REQUEST_PRIOR_MODIFIED = 2,
    MLN_RESOURCE_REQUEST_PRIOR_EXPIRES = 4,
}

internal enum mln_resource_response_field : uint
{
    MLN_RESOURCE_RESPONSE_MODIFIED = 1,
    MLN_RESOURCE_RESPONSE_EXPIRES = 2,
    MLN_RESOURCE_RESPONSE_RETRY_AFTER = 4,
}

internal enum mln_resource_response_status : uint
{
    MLN_RESOURCE_RESPONSE_STATUS_OK = 0,
    MLN_RESOURCE_RESPONSE_STATUS_ERROR = 1,
    MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT = 2,
    MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED = 3,
}

internal enum mln_resource_storage_policy : uint
{
    MLN_RESOURCE_STORAGE_POLICY_PERMANENT = 0,
    MLN_RESOURCE_STORAGE_POLICY_VOLATILE = 1,
}

internal enum mln_resource_usage : uint
{
    MLN_RESOURCE_USAGE_ONLINE = 0,
    MLN_RESOURCE_USAGE_OFFLINE = 1,
}

internal enum mln_runtime_event_flag : ulong
{
    MLN_RUNTIME_EVENT_FLAG_NONE = 0,
    MLN_RUNTIME_EVENT_FLAG_MAP_CAMERA_WILL_CHANGE = 2,
    MLN_RUNTIME_EVENT_FLAG_MAP_CAMERA_IS_CHANGING = 4,
    MLN_RUNTIME_EVENT_FLAG_MAP_CAMERA_DID_CHANGE = 8,
    MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_LOADED = 16,
    MLN_RUNTIME_EVENT_FLAG_MAP_LOADING_STARTED = 32,
    MLN_RUNTIME_EVENT_FLAG_MAP_LOADING_FINISHED = 64,
    MLN_RUNTIME_EVENT_FLAG_MAP_LOADING_FAILED = 128,
    MLN_RUNTIME_EVENT_FLAG_MAP_IDLE = 256,
    MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_ERROR = 512,
    MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_FRAME_STARTED = 1024,
    MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_FRAME_FINISHED = 2048,
    MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_MAP_STARTED = 4096,
    MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_MAP_FINISHED = 8192,
    MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_IMAGE_MISSING = 16384,
    MLN_RUNTIME_EVENT_FLAG_MAP_TILE_ACTION = 32768,
    MLN_RUNTIME_EVENT_FLAG_MAP_SOURCE_CHANGED = 65536,
    MLN_RUNTIME_EVENT_FLAG_OFFLINE_REGION_STATUS_CHANGED = 131072,
    MLN_RUNTIME_EVENT_FLAG_OFFLINE_REGION_RESPONSE_ERROR = 262144,
    MLN_RUNTIME_EVENT_FLAG_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 524288,
    MLN_RUNTIME_EVENT_FLAG_MAP_GLYPHS_REQUESTED = 1048576,
    MLN_RUNTIME_EVENT_FLAG_MAP_GLYPHS_LOADED = 2097152,
    MLN_RUNTIME_EVENT_FLAG_MAP_GLYPHS_ERROR = 4194304,
    MLN_RUNTIME_EVENT_FLAG_MAP_SPRITE_REQUESTED = 8388608,
    MLN_RUNTIME_EVENT_FLAG_MAP_SPRITE_LOADED = 16777216,
    MLN_RUNTIME_EVENT_FLAG_MAP_SPRITE_ERROR = 33554432,
    MLN_RUNTIME_EVENT_FLAG_ALL_MAP_EVENTS = 66191358,
    MLN_RUNTIME_EVENT_FLAG_ALL_RUNTIME_EVENTS = 917504,
    MLN_RUNTIME_EVENT_FLAG_ALL = 67108862,
}

internal enum mln_runtime_event_payload_type : uint
{
    MLN_RUNTIME_EVENT_PAYLOAD_NONE = 0,
    MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME = 1,
    MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP = 2,
    MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION = 3,
    MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS = 4,
    MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR = 5,
    MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT = 6,
    MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS = 7,
    MLN_RUNTIME_EVENT_PAYLOAD_SPRITE = 8,
}

internal enum mln_runtime_event_source_type : uint
{
    MLN_RUNTIME_EVENT_SOURCE_RUNTIME = 0,
    MLN_RUNTIME_EVENT_SOURCE_MAP = 1,
}

internal enum mln_runtime_event_type : uint
{
    MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE = 1,
    MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING = 2,
    MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE = 3,
    MLN_RUNTIME_EVENT_MAP_STYLE_LOADED = 4,
    MLN_RUNTIME_EVENT_MAP_LOADING_STARTED = 5,
    MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED = 6,
    MLN_RUNTIME_EVENT_MAP_LOADING_FAILED = 7,
    MLN_RUNTIME_EVENT_MAP_IDLE = 8,
    MLN_RUNTIME_EVENT_MAP_RENDER_ERROR = 9,
    MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED = 10,
    MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED = 11,
    MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED = 12,
    MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED = 13,
    MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING = 14,
    MLN_RUNTIME_EVENT_MAP_TILE_ACTION = 15,
    MLN_RUNTIME_EVENT_MAP_SOURCE_CHANGED = 16,
    MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED = 17,
    MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR = 18,
    MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 19,
    MLN_RUNTIME_EVENT_MAP_GLYPHS_REQUESTED = 20,
    MLN_RUNTIME_EVENT_MAP_GLYPHS_LOADED = 21,
    MLN_RUNTIME_EVENT_MAP_GLYPHS_ERROR = 22,
    MLN_RUNTIME_EVENT_MAP_SPRITE_REQUESTED = 23,
    MLN_RUNTIME_EVENT_MAP_SPRITE_LOADED = 24,
    MLN_RUNTIME_EVENT_MAP_SPRITE_ERROR = 25,
}

internal enum mln_source_feature_query_option_field : uint
{
    MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS = 1,
}

internal enum mln_status : int
{
    MLN_STATUS_OK = 0,
    MLN_STATUS_INVALID_ARGUMENT = -1,
    MLN_STATUS_INVALID_STATE = -2,
    MLN_STATUS_WRONG_THREAD = -3,
    MLN_STATUS_UNSUPPORTED = -4,
    MLN_STATUS_NATIVE_ERROR = -5,
    MLN_STATUS_CANCELLED = -6,
    MLN_STATUS_BUSY = -7,
    MLN_STATUS_TARGET_LOST = -8,
    MLN_STATUS_NOT_READY = -9,
    MLN_STATUS_NOT_FOUND = -10,
}

internal enum mln_style_expression_input_field : uint
{
    MLN_STYLE_EXPRESSION_INPUT_ZOOM = 1,
    MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER = 2,
    MLN_STYLE_EXPRESSION_INPUT_ELEVATION = 4,
    MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID = 8,
}

internal enum mln_style_expression_option_field : uint
{
    MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE = 1,
}

internal enum mln_style_expression_type_kind : uint
{
    MLN_STYLE_EXPRESSION_TYPE_VALUE = 0,
    MLN_STYLE_EXPRESSION_TYPE_NUMBER = 1,
    MLN_STYLE_EXPRESSION_TYPE_STRING = 2,
    MLN_STYLE_EXPRESSION_TYPE_BOOLEAN = 3,
    MLN_STYLE_EXPRESSION_TYPE_COLOR = 4,
    MLN_STYLE_EXPRESSION_TYPE_OBJECT = 5,
    MLN_STYLE_EXPRESSION_TYPE_FORMATTED = 6,
    MLN_STYLE_EXPRESSION_TYPE_IMAGE = 7,
    MLN_STYLE_EXPRESSION_TYPE_PADDING = 8,
    MLN_STYLE_EXPRESSION_TYPE_VARIABLE_ANCHOR_OFFSET_COLLECTION = 9,
    MLN_STYLE_EXPRESSION_TYPE_ARRAY = 10,
}

internal enum mln_style_image_info_field : uint
{
    MLN_STYLE_IMAGE_INFO_CONTENT = 1,
    MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH = 2,
    MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT = 4,
}

internal enum mln_style_image_option_field : uint
{
    MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO = 1,
    MLN_STYLE_IMAGE_OPTION_SDF = 2,
    MLN_STYLE_IMAGE_OPTION_STRETCH_X = 4,
    MLN_STYLE_IMAGE_OPTION_STRETCH_Y = 8,
    MLN_STYLE_IMAGE_OPTION_CONTENT = 16,
    MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH = 32,
    MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT = 64,
    MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED = 128,
}

internal enum mln_style_image_text_fit : uint
{
    MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK = 0,
    MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY = 1,
    MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL = 2,
}

internal enum mln_style_layer_visibility : uint
{
    MLN_STYLE_LAYER_VISIBILITY_VISIBLE = 0,
    MLN_STYLE_LAYER_VISIBILITY_NONE = 1,
}

internal enum mln_style_raster_dem_encoding : uint
{
    MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX = 0,
    MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM = 1,
}

internal enum mln_style_source_info_field : uint
{
    MLN_STYLE_SOURCE_INFO_TILE_JSON = 1,
    MLN_STYLE_SOURCE_INFO_BOUNDS = 2,
    MLN_STYLE_SOURCE_INFO_TILE_SIZE = 4,
    MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING = 8,
    MLN_STYLE_SOURCE_INFO_RASTER_ENCODING = 16,
}

internal enum mln_style_source_type : uint
{
    MLN_STYLE_SOURCE_TYPE_UNKNOWN = 0,
    MLN_STYLE_SOURCE_TYPE_VECTOR = 1,
    MLN_STYLE_SOURCE_TYPE_RASTER = 2,
    MLN_STYLE_SOURCE_TYPE_RASTER_DEM = 3,
    MLN_STYLE_SOURCE_TYPE_GEO_JSON = 4,
    MLN_STYLE_SOURCE_TYPE_IMAGE = 5,
    MLN_STYLE_SOURCE_TYPE_VIDEO = 6,
    MLN_STYLE_SOURCE_TYPE_ANNOTATIONS = 7,
    MLN_STYLE_SOURCE_TYPE_CUSTOM_GEOMETRY = 8,
    MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR = 9,
}

internal enum mln_style_tile_scheme : uint
{
    MLN_STYLE_TILE_SCHEME_XYZ = 0,
    MLN_STYLE_TILE_SCHEME_TMS = 1,
}

internal enum mln_style_transition_option_field : uint
{
    MLN_STYLE_TRANSITION_OPTION_DURATION = 1,
    MLN_STYLE_TRANSITION_OPTION_DELAY = 2,
    MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS = 4,
}

internal enum mln_style_vector_tile_encoding : uint
{
    MLN_STYLE_VECTOR_TILE_ENCODING_MVT = 0,
    MLN_STYLE_VECTOR_TILE_ENCODING_MLT = 1,
}

internal enum mln_tile_lod_mode : uint
{
    MLN_TILE_LOD_MODE_DEFAULT = 0,
    MLN_TILE_LOD_MODE_DISTANCE = 1,
}

internal enum mln_tile_operation : uint
{
    MLN_TILE_OPERATION_REQUESTED_FROM_CACHE = 0,
    MLN_TILE_OPERATION_REQUESTED_FROM_NETWORK = 1,
    MLN_TILE_OPERATION_LOAD_FROM_NETWORK = 2,
    MLN_TILE_OPERATION_LOAD_FROM_CACHE = 3,
    MLN_TILE_OPERATION_START_PARSE = 4,
    MLN_TILE_OPERATION_END_PARSE = 5,
    MLN_TILE_OPERATION_ERROR = 6,
    MLN_TILE_OPERATION_CANCELLED = 7,
    MLN_TILE_OPERATION_NULL = 8,
}

internal enum mln_url_match_flag : uint
{
    MLN_URL_MATCH_EXACT = 0,
    MLN_URL_MATCH_GLOB = 1,
}

internal enum mln_viewport_mode : uint
{
    MLN_VIEWPORT_MODE_DEFAULT = 0,
    MLN_VIEWPORT_MODE_FLIPPED_Y = 1,
}

internal enum mln_web_gl_context_kind : uint
{
    MLN_WEB_GL_CONTEXT_EXISTING = 0,
    MLN_WEB_GL_CONTEXT_TRANSFERRED_CANVAS = 1,
}
