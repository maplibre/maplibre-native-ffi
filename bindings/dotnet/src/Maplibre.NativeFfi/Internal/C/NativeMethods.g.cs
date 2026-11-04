// Generated from the C headers by tools/bindgen. Do not edit.
using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C;

internal static unsafe partial class NativeMethods
{
    internal const uint MLN_C_VERSION = 0;

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_acquired_frame_discard(
        MlnAcquiredFrame frame,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_acquired_frame_get_metal_texture(
        MlnAcquiredFrame frame,
        mln_metal_texture_frame* out_frame,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_acquired_frame_get_open_gl_texture(
        MlnAcquiredFrame frame,
        mln_open_gl_texture_frame* out_frame,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_acquired_frame_get_producer_sync(
        MlnAcquiredFrame frame,
        mln_gpu_sync* out_sync,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_acquired_frame_get_result(
        MlnAcquiredFrame frame,
        mln_render_frame_result* out_result,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_acquired_frame_get_vulkan_texture(
        MlnAcquiredFrame frame,
        mln_vulkan_texture_frame* out_frame,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_acquired_frame_release(
        MlnAcquiredFrame frame,
        mln_gpu_sync* consumer_completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_acquired_frame_view_begin(
        MlnAcquiredFrame frame,
        void** out_scope,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial void mln_acquired_frame_view_end(void* scope);

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_android_init(
        void* jni_env,
        void* context,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial uint mln_c_version();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_camera_for_tile(
        mln_canonical_tile_id tile,
        mln_logical_size logical_size,
        mln_camera_options* out_camera,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_geo_json_source_data_create(
        mln_buffer_view data,
        mln_geo_json_source_options* options,
        MlnGeoJsonSourceData* out_data,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial void mln_geo_json_source_data_release(MlnGeoJsonSourceData data);

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_http_header_transform_response_set_header(
        mln_http_header_transform_response* response,
        mln_buffer_view name,
        mln_buffer_view value,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_lat_lng_for_projected_meters(
        mln_projected_meters meters,
        mln_lat_lng* out_coordinate,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_log_clear_callback(mln_diagnostic* out_diagnostic);

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_log_set_async_severity_mask(
        uint mask,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_log_set_callback(
        mln_log_handler* handler,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_logical_extent_physical_size(
        mln_logical_extent extent,
        uint* out_width,
        uint* out_height,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_add_custom_geometry_source(
        MlnMap map,
        mln_buffer_view source_id,
        mln_custom_geometry_source_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_add_custom_vector_source(
        MlnMap map,
        mln_buffer_view source_id,
        mln_custom_vector_source_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_add_geo_json_source_data(
        MlnMap map,
        mln_buffer_view source_id,
        MlnGeoJsonSourceData data,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_add_image_source_image(
        MlnMap map,
        mln_buffer_view source_id,
        mln_lat_lng* coordinates,
        nuint coordinate_count,
        mln_premultiplied_rgba8_image* image,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_add_style_layer_json(
        MlnMap map,
        mln_buffer_view layer_json,
        mln_buffer_view before_layer_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_add_style_source_json(
        MlnMap map,
        mln_buffer_view source_id,
        mln_buffer_view source_json,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_apply_camera_delta(
        MlnMap map,
        mln_camera_delta* delta,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_attach_metal_borrowed_texture(
        MlnMap map,
        mln_metal_borrowed_texture_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_attach_metal_surface(
        MlnMap map,
        mln_metal_surface_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_attach_offscreen(
        MlnMap map,
        mln_offscreen_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnOffscreenDevice device,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_attach_open_gl_borrowed_texture(
        MlnMap map,
        mln_open_gl_borrowed_texture_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_attach_open_gl_surface(
        MlnMap map,
        mln_open_gl_surface_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_attach_vulkan_borrowed_texture(
        MlnMap map,
        mln_vulkan_borrowed_texture_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_attach_vulkan_surface(
        MlnMap map,
        mln_vulkan_surface_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_begin_command_group(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_camera_for_geometry(
        MlnMap map,
        mln_buffer_view geometry,
        mln_camera_fit_options* fit_options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_camera_for_lat_lng_bounds(
        MlnMap map,
        mln_lat_lng_bounds bounds,
        mln_camera_fit_options* fit_options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_camera_for_lat_lngs(
        MlnMap map,
        mln_lat_lng* coordinates,
        nuint coordinate_count,
        mln_camera_fit_options* fit_options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_cancel_camera_transitions(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_cancel_camera_transitions_with_id(
        MlnMap map,
        ulong transition_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_dispose(MlnMap map, mln_diagnostic* out_diagnostic);

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_dump_debug_logs(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_end_command_group(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_fit_camera_to_geometry(
        MlnMap map,
        mln_buffer_view geometry,
        mln_camera_fit_update* update,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_fit_camera_to_lat_lng_bounds(
        MlnMap map,
        mln_lat_lng_bounds bounds,
        mln_camera_fit_update* update,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_fit_camera_to_lat_lngs(
        MlnMap map,
        mln_lat_lng* coordinates,
        nuint coordinate_count,
        mln_camera_fit_update* update,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_get_camera(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_get_feature_state(
        MlnMap map,
        mln_feature_state_selector* selector,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_get_global_state(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_get_image_source_coordinates(
        MlnMap map,
        mln_buffer_view source_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_get_loaded_style_json(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_get_snapshot(
        MlnMap map,
        mln_map_snapshot* out_snapshot,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_get_style_image(
        MlnMap map,
        mln_buffer_view image_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_get_style_layer(
        MlnMap map,
        mln_buffer_view layer_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_get_style_layer_json(
        MlnMap map,
        mln_buffer_view layer_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_get_style_layer_property(
        MlnMap map,
        mln_buffer_view layer_id,
        mln_buffer_view property_name,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_get_style_light_property(
        MlnMap map,
        mln_buffer_view property_name,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_get_style_source(
        MlnMap map,
        mln_buffer_view source_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_get_style_transition_options(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_get_style_url(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_invalidate_custom_geometry_source_region(
        MlnMap map,
        mln_buffer_view source_id,
        mln_lat_lng_bounds bounds,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_invalidate_custom_geometry_source_tile(
        MlnMap map,
        mln_buffer_view source_id,
        mln_canonical_tile_id tile_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_invalidate_custom_vector_source_region(
        MlnMap map,
        mln_buffer_view source_id,
        mln_lat_lng_bounds bounds,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_invalidate_custom_vector_source_tile(
        MlnMap map,
        mln_buffer_view source_id,
        mln_canonical_tile_id tile_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_lat_lng_bounds_for_camera(
        MlnMap map,
        mln_camera_options* camera,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_list_style_layers(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_list_style_sources(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_move_style_layer(
        MlnMap map,
        mln_buffer_view layer_id,
        mln_buffer_view before_layer_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_projection_create(
        mln_map_view* view,
        MlnMapProjection* out_projection,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_projection_get_camera(
        MlnMapProjection projection,
        mln_map_projection_camera_result* out_result,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_projection_lat_lng_for_pixel(
        MlnMapProjection projection,
        mln_screen_point point,
        mln_lat_lng* out_coordinate,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_projection_lat_lng_for_pixel_unwrapped(
        MlnMapProjection projection,
        mln_screen_point point,
        mln_lat_lng* out_coordinate,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_projection_lat_lngs_for_pixels(
        MlnMapProjection projection,
        mln_screen_point* points,
        nuint point_count,
        mln_lat_lng* out_coordinates,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_projection_lat_lngs_for_pixels_unwrapped(
        MlnMapProjection projection,
        mln_screen_point* points,
        nuint point_count,
        mln_lat_lng* out_coordinates,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_projection_meters_per_pixel_at_latitude(
        MlnMapProjection projection,
        double latitude,
        double* out_meters_per_pixel,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_projection_pixel_for_lat_lng(
        MlnMapProjection projection,
        mln_lat_lng coordinate,
        mln_screen_point* out_point,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_projection_pixels_for_lat_lngs(
        MlnMapProjection projection,
        mln_lat_lng* coordinates,
        nuint coordinate_count,
        mln_screen_point* out_points,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial void mln_map_projection_release(MlnMapProjection projection);

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_projection_set_camera(
        MlnMapProjection projection,
        mln_camera_options* camera,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_release(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_remove_feature_state(
        MlnMap map,
        mln_feature_state_selector* selector,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_remove_style_image(
        MlnMap map,
        mln_buffer_view image_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_remove_style_layer(
        MlnMap map,
        mln_buffer_view layer_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_remove_style_source(
        MlnMap map,
        mln_buffer_view source_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_request_repaint(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_request_still_image(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_resize(
        MlnMap map,
        mln_logical_size logical_size,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_bounds(
        MlnMap map,
        mln_bound_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_custom_geometry_source_tile_data(
        MlnMap map,
        mln_buffer_view source_id,
        mln_canonical_tile_id tile_id,
        mln_buffer_view data,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_custom_geometry_source_tile_error(
        MlnMap map,
        mln_buffer_view source_id,
        mln_canonical_tile_id tile_id,
        mln_buffer_view message,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_custom_vector_source_tile_data(
        MlnMap map,
        mln_buffer_view source_id,
        mln_canonical_tile_id tile_id,
        mln_style_vector_tile_encoding encoding,
        mln_buffer_view data,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_custom_vector_source_tile_error(
        MlnMap map,
        mln_buffer_view source_id,
        mln_canonical_tile_id tile_id,
        mln_buffer_view message,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_debug_flags(
        MlnMap map,
        uint flags,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_event_mask(
        MlnMap map,
        ulong mask,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_feature_state(
        MlnMap map,
        mln_feature_state_selector* selector,
        mln_buffer_view state,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_free_camera_options(
        MlnMap map,
        mln_free_camera_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_geo_json_source_data(
        MlnMap map,
        mln_buffer_view source_id,
        MlnGeoJsonSourceData data,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_geo_json_source_synchronous_tiling(
        MlnMap map,
        mln_buffer_view source_id,
        byte enabled,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_geo_json_source_url(
        MlnMap map,
        mln_buffer_view source_id,
        mln_buffer_view url,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_global_state_property(
        MlnMap map,
        mln_buffer_view property_name,
        mln_buffer_view value,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_image_source_coordinates(
        MlnMap map,
        mln_buffer_view source_id,
        mln_lat_lng* coordinates,
        nuint coordinate_count,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_image_source_image(
        MlnMap map,
        mln_buffer_view source_id,
        mln_premultiplied_rgba8_image* image,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_image_source_url(
        MlnMap map,
        mln_buffer_view source_id,
        mln_buffer_view url,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_projection_mode(
        MlnMap map,
        mln_projection_mode_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_rendering_stats_view_enabled(
        MlnMap map,
        byte enabled,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_style_image(
        MlnMap map,
        mln_buffer_view image_id,
        mln_premultiplied_rgba8_image* image,
        mln_style_image_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_style_json(
        MlnMap map,
        mln_buffer_view json,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_style_layer_property(
        MlnMap map,
        mln_buffer_view layer_id,
        mln_buffer_view property_name,
        mln_buffer_view value,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_style_light_json(
        MlnMap map,
        mln_buffer_view light_json,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_style_light_property(
        MlnMap map,
        mln_buffer_view property_name,
        mln_buffer_view value,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_style_source_volatile(
        MlnMap map,
        mln_buffer_view source_id,
        byte is_volatile,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_style_transition_options(
        MlnMap map,
        mln_style_transition_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_style_url(
        MlnMap map,
        mln_buffer_view url,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_tile_options(
        MlnMap map,
        mln_map_tile_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_viewport_options(
        MlnMap map,
        mln_map_viewport_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_update_camera(
        MlnMap map,
        mln_camera_update* update,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_network_get_status(
        mln_network_status* out_status,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_network_set_status(
        mln_network_status status,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_offscreen_device_create(
        mln_offscreen_device_options* options,
        MlnOffscreenDevice* out_device,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial void mln_offscreen_device_release(MlnOffscreenDevice device);

    [LibraryImport(LibraryName)]
    internal static partial uint mln_open_gl_supported_context_provider_mask();

    [LibraryImport(LibraryName)]
    internal static partial void* mln_plugin_get_register_function_v1();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_projected_meters_for_lat_lng(
        mln_lat_lng coordinate,
        mln_projected_meters* out_meters,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial void mln_render_frame_batch_release(MlnRenderFrameBatch batch);

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_abandon(
        MlnRenderSession session,
        mln_render_abandon_result* out_result,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_acquire_frame(
        MlnRenderSession session,
        MlnAcquiredFrame* out_frame,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_barrier(
        MlnRenderSession session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_clear_data(
        MlnRenderSession session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_demand_frame(
        MlnRenderSession session,
        mln_frame_demand* demand,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_detach(
        MlnRenderSession session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_dispose(
        MlnRenderSession session,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_drain_frame_results(
        MlnRenderSession session,
        MlnRenderFrameBatch* out_batch,
        mln_render_frame_batch_view* out_view,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_dump_debug_logs(
        MlnRenderSession session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_get_capabilities(
        MlnRenderSession session,
        mln_render_session_capabilities* out_capabilities,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_get_cluster_children(
        MlnRenderSession session,
        mln_buffer_view source_id,
        uint cluster_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_get_cluster_expansion_zoom(
        MlnRenderSession session,
        mln_buffer_view source_id,
        uint cluster_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_get_cluster_leaves(
        MlnRenderSession session,
        mln_buffer_view source_id,
        uint cluster_id,
        uint limit,
        uint offset,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_get_failure(
        MlnRenderSession session,
        sbyte* out_message,
        nuint message_capacity,
        nuint* out_message_length,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_get_snapshot(
        MlnRenderSession session,
        mln_render_session_snapshot* out_snapshot,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_query_rendered_features(
        MlnRenderSession session,
        mln_rendered_query_geometry* geometry,
        mln_rendered_feature_query_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_query_source_features(
        MlnRenderSession session,
        mln_buffer_view source_id,
        mln_source_feature_query_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_read_texture(
        MlnRenderSession session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_reduce_memory_use(
        MlnRenderSession session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_release(
        MlnRenderSession session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_release_target(
        MlnRenderSession session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_resize(
        MlnRenderSession session,
        mln_logical_size logical_size,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_service_driver_work(
        MlnRenderSession session,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_set_metal_borrowed_texture_target(
        MlnRenderSession session,
        mln_metal_borrowed_texture_replacement* replacement,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_set_metal_surface_target(
        MlnRenderSession session,
        mln_metal_surface_replacement* replacement,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_set_open_gl_borrowed_texture_target(
        MlnRenderSession session,
        mln_open_gl_borrowed_texture_replacement* replacement,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_set_open_gl_surface_target(
        MlnRenderSession session,
        mln_open_gl_surface_replacement* replacement,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_set_vulkan_borrowed_texture_target(
        MlnRenderSession session,
        mln_vulkan_borrowed_texture_replacement* replacement,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_set_vulkan_surface_target(
        MlnRenderSession session,
        mln_vulkan_surface_replacement* replacement,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_resource_request_complete(
        MlnResourceRequest handle,
        mln_resource_response* response,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_resource_request_is_cancelled(
        MlnResourceRequest handle,
        bool* out_cancelled,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_resource_request_pass_through(
        MlnResourceRequest handle,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial void mln_resource_request_release(MlnResourceRequest handle);

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_resource_request_set_cancel_callback(
        MlnResourceRequest handle,
        mln_resource_request_cancel_handler* handler,
        bool* out_cancelled,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_resource_transform_request_complete(
        MlnResourceTransformRequest request,
        mln_buffer_view url,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_resource_transform_request_fail(
        MlnResourceTransformRequest request,
        mln_resource_transform_error* error,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial void mln_resource_transform_request_release(
        MlnResourceTransformRequest request
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_clear_event_handler(
        MlnRuntime runtime,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_clear_http_header_transform(
        MlnRuntime runtime,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_clear_resource_provider(
        MlnRuntime runtime,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_clear_resource_transform(
        MlnRuntime runtime,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_create(
        mln_runtime_options* options,
        MlnRuntime* out_runtime,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_create_map(
        MlnRuntime runtime,
        mln_map_options* options,
        MlnMap* out_map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_create_offline_region(
        MlnRuntime runtime,
        mln_offline_region_definition* definition,
        byte* metadata,
        nuint metadata_size,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_delete_offline_region(
        MlnRuntime runtime,
        long region_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_dispose(
        MlnRuntime runtime,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_get_offline_region(
        MlnRuntime runtime,
        long region_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_get_offline_region_status(
        MlnRuntime runtime,
        long region_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_invalidate_offline_region(
        MlnRuntime runtime,
        long region_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_list_offline_regions(
        MlnRuntime runtime,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_merge_offline_regions(
        MlnRuntime runtime,
        mln_buffer_view side_database_path,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_release(
        MlnRuntime runtime,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_run_cache_database_operation(
        MlnRuntime runtime,
        mln_cache_database_operation operation,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_set_event_handler(
        MlnRuntime runtime,
        mln_runtime_event_handler* handler,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_set_http_header_transform(
        MlnRuntime runtime,
        mln_http_header_transform* transform,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_set_maximum_ambient_cache_size(
        MlnRuntime runtime,
        ulong size,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_set_offline_region_download_state(
        MlnRuntime runtime,
        long region_id,
        mln_offline_region_download_state state,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_set_offline_region_observed(
        MlnRuntime runtime,
        long region_id,
        ulong events,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_set_resource_provider(
        MlnRuntime runtime,
        mln_resource_provider* provider,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_set_resource_transform(
        MlnRuntime runtime,
        mln_resource_transform* transform,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_update_offline_region_metadata(
        MlnRuntime runtime,
        long region_id,
        byte* metadata,
        nuint metadata_size,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_style_expression_create(
        mln_buffer_view expression,
        mln_style_expression_options* options,
        MlnStyleExpression* out_expression,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_style_expression_evaluate(
        MlnStyleExpression expression,
        mln_style_expression_inputs* inputs,
        sbyte* out_json,
        nuint json_capacity,
        nuint* out_json_length,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_style_expression_get_info(
        MlnStyleExpression expression,
        mln_style_expression_info* out_info,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial void mln_style_expression_release(MlnStyleExpression expression);

    [LibraryImport(LibraryName)]
    internal static partial uint mln_supported_render_backend_mask();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_vulkan_device_create(
        mln_vulkan_device_options* options,
        MlnVulkanDevice* out_device,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_vulkan_device_get_context(
        MlnVulkanDevice device,
        mln_vulkan_device_context_result* out_result,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial void mln_vulkan_device_release(MlnVulkanDevice device);
}
