// Generated from the C headers by tools/bindgen. Do not edit.
using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C;

internal static unsafe partial class NativeMethods
{
    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_acquired_frame_dispose(
        MlnAcquiredFrame frame,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_acquired_frame_get_metal_texture(
        MlnAcquiredFrame frame,
        mln_metal_owned_texture_frame* out_frame,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_acquired_frame_get_opengl_texture(
        MlnAcquiredFrame frame,
        mln_opengl_owned_texture_frame* out_frame,
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
        mln_vulkan_owned_texture_frame* out_frame,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_acquired_frame_get_webgpu_texture(
        MlnAcquiredFrame frame,
        mln_webgpu_owned_texture_frame* out_frame,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_acquired_frame_release(
        MlnAcquiredFrame* frame,
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
        void* jni_class,
        void* context,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_animation_options mln_animation_options_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_bound_options mln_bound_options_default();

    [LibraryImport(LibraryName)]
    internal static partial void mln_buffer_destroy(MlnBuffer buffer);

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_buffer_get(
        MlnBuffer buffer,
        mln_buffer_view* out_view,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial uint mln_c_version();

    [LibraryImport(LibraryName)]
    internal static partial mln_camera_delta mln_camera_delta_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_camera_fit_options mln_camera_fit_options_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_camera_options mln_camera_options_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_camera_update mln_camera_update_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_custom_geometry_source_options mln_custom_geometry_source_options_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_custom_mvt_vector_source_options mln_custom_mvt_vector_source_options_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_event_batch_get(
        MlnEventBatch batch,
        mln_runtime_event_batch_view* out_view,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial void mln_event_batch_release(MlnEventBatch batch);

    [LibraryImport(LibraryName)]
    internal static partial mln_frame_demand mln_frame_demand_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_free_camera_options mln_free_camera_options_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_geojson_source_data_create(
        mln_buffer_view data,
        mln_geojson_source_options* options,
        MlnGeojsonSourceData* out_data,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial void mln_geojson_source_data_destroy(MlnGeojsonSourceData data);

    [LibraryImport(LibraryName)]
    internal static partial mln_geojson_source_options mln_geojson_source_options_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_gpu_sync mln_gpu_sync_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_http_header_transform_response_set(
        mln_http_header_transform_response* response,
        sbyte* name,
        nuint name_size,
        sbyte* value,
        nuint value_size,
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
        delegate* unmanaged[Cdecl]<void*, uint, uint, long, sbyte*, uint> callback,
        void* user_data,
        delegate* unmanaged[Cdecl]<void*, void> release_user_data,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_add_color_relief_layer(
        MlnMap map,
        mln_buffer_view layer_id,
        mln_buffer_view source_id,
        mln_buffer_view before_layer_id,
        mln_completion* completion,
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
    internal static partial mln_status mln_map_add_custom_mvt_vector_source(
        MlnMap map,
        mln_buffer_view source_id,
        mln_custom_mvt_vector_source_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_add_geojson_source_data(
        MlnMap map,
        mln_buffer_view source_id,
        MlnGeojsonSourceData data,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_add_geojson_source_url(
        MlnMap map,
        mln_buffer_view source_id,
        mln_buffer_view url,
        mln_geojson_source_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_add_hillshade_layer(
        MlnMap map,
        mln_buffer_view layer_id,
        mln_buffer_view source_id,
        mln_buffer_view before_layer_id,
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
    internal static partial mln_status mln_map_add_image_source_url(
        MlnMap map,
        mln_buffer_view source_id,
        mln_lat_lng* coordinates,
        nuint coordinate_count,
        mln_buffer_view url,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_add_location_indicator_layer(
        MlnMap map,
        mln_buffer_view layer_id,
        mln_buffer_view before_layer_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_add_raster_dem_source_tiles(
        MlnMap map,
        mln_buffer_view source_id,
        mln_buffer_view* tiles,
        nuint tile_count,
        mln_style_tile_source_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_add_raster_dem_source_url(
        MlnMap map,
        mln_buffer_view source_id,
        mln_buffer_view url,
        mln_style_tile_source_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_add_raster_source_tiles(
        MlnMap map,
        mln_buffer_view source_id,
        mln_buffer_view* tiles,
        nuint tile_count,
        mln_style_tile_source_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_add_raster_source_url(
        MlnMap map,
        mln_buffer_view source_id,
        mln_buffer_view url,
        mln_style_tile_source_options* options,
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
    internal static partial mln_status mln_map_add_vector_source_tiles(
        MlnMap map,
        mln_buffer_view source_id,
        mln_buffer_view* tiles,
        nuint tile_count,
        mln_style_tile_source_options* options,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_add_vector_source_url(
        MlnMap map,
        mln_buffer_view source_id,
        mln_buffer_view url,
        mln_style_tile_source_options* options,
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
    internal static partial mln_status mln_map_camera_query(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_camera_snapshot_get(
        MlnMap map,
        mln_camera_options* out_camera,
        ulong* out_generation,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_cancel_transitions(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_create(
        MlnRuntime runtime,
        mln_map_options* options,
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
    internal static partial mln_status mln_map_get_layer_filter(
        MlnMap map,
        mln_buffer_view layer_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_get_layer_property(
        MlnMap map,
        mln_buffer_view layer_id,
        mln_buffer_view property_name,
        mln_completion* completion,
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
    internal static partial mln_status mln_map_invalidate_custom_mvt_vector_source_tile(
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
    internal static partial mln_status mln_map_lat_lng_bounds_for_camera_unwrapped(
        MlnMap map,
        mln_camera_options* camera,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_lat_lng_for_pixel(
        MlnMap map,
        mln_screen_point point,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_lat_lng_for_pixel_unwrapped(
        MlnMap map,
        mln_screen_point point,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_lat_lngs_for_pixels(
        MlnMap map,
        mln_screen_point* points,
        nuint point_count,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_lat_lngs_for_pixels_unwrapped(
        MlnMap map,
        mln_screen_point* points,
        nuint point_count,
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
    internal static partial mln_status mln_map_loaded_style_json(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_meters_per_pixel_at_latitude(
        MlnMap map,
        double latitude,
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
    internal static partial mln_map_options mln_map_options_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_pixel_for_lat_lng(
        MlnMap map,
        mln_lat_lng coordinate,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_pixels_for_lat_lngs(
        MlnMap map,
        mln_lat_lng* coordinates,
        nuint coordinate_count,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_projection_close(
        MlnMapProjection projection,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_projection_create(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_projection_get_camera(
        MlnMapProjection projection,
        mln_camera_options* out_camera,
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
    internal static partial mln_status mln_map_projection_set_camera(
        MlnMapProjection projection,
        mln_camera_options* camera,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_projection_set_visible_coordinates(
        MlnMapProjection projection,
        mln_lat_lng* coordinates,
        nuint coordinate_count,
        mln_edge_insets padding,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_projection_set_visible_geometry(
        MlnMapProjection projection,
        mln_buffer_view geometry,
        mln_edge_insets padding,
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
        mln_logical_extent extent,
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
    internal static partial mln_status mln_map_set_custom_mvt_vector_source_tile_data(
        MlnMap map,
        mln_buffer_view source_id,
        mln_canonical_tile_id tile_id,
        mln_buffer_view data,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_custom_mvt_vector_source_tile_error(
        MlnMap map,
        mln_buffer_view source_id,
        mln_canonical_tile_id tile_id,
        mln_buffer_view message,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_debug_options(
        MlnMap map,
        uint options,
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
    internal static partial mln_status mln_map_set_geojson_source_data(
        MlnMap map,
        mln_buffer_view source_id,
        MlnGeojsonSourceData data,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_geojson_source_synchronous_tiling(
        MlnMap map,
        mln_buffer_view source_id,
        byte enabled,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_geojson_source_url(
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
    internal static partial mln_status mln_map_set_layer_filter(
        MlnMap map,
        mln_buffer_view layer_id,
        mln_buffer_view* filter,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_layer_max_zoom(
        MlnMap map,
        mln_buffer_view layer_id,
        double max_zoom,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_layer_min_zoom(
        MlnMap map,
        mln_buffer_view layer_id,
        double min_zoom,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_layer_property(
        MlnMap map,
        mln_buffer_view layer_id,
        mln_buffer_view property_name,
        mln_buffer_view value,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_layer_source_id(
        MlnMap map,
        mln_buffer_view layer_id,
        mln_buffer_view source_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_layer_source_layer(
        MlnMap map,
        mln_buffer_view layer_id,
        mln_buffer_view source_layer,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_layer_visibility(
        MlnMap map,
        mln_buffer_view layer_id,
        uint visibility,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_location_indicator_accuracy_radius(
        MlnMap map,
        mln_buffer_view layer_id,
        double radius,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_location_indicator_bearing(
        MlnMap map,
        mln_buffer_view layer_id,
        double bearing,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_location_indicator_image_name(
        MlnMap map,
        mln_buffer_view layer_id,
        uint image_kind,
        mln_buffer_view image_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_location_indicator_location(
        MlnMap map,
        mln_buffer_view layer_id,
        mln_lat_lng coordinate,
        double altitude,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_set_projection_mode(
        MlnMap map,
        mln_projection_mode* mode,
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
        sbyte* url,
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
    internal static partial mln_status mln_map_snapshot_get(
        MlnMap map,
        mln_map_snapshot* out_snapshot,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_style_url(
        MlnMap map,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_map_tile_options mln_map_tile_options_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_map_update_camera(
        MlnMap map,
        mln_camera_update* update,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_map_viewport_options mln_map_viewport_options_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_metal_borrowed_texture_attach(
        MlnMap map,
        mln_metal_borrowed_texture_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_metal_borrowed_texture_descriptor mln_metal_borrowed_texture_descriptor_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_metal_borrowed_texture_set_target(
        MlnRenderSession session,
        mln_metal_borrowed_texture_descriptor* descriptor,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_metal_owned_texture_attach(
        MlnMap map,
        mln_metal_owned_texture_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_metal_owned_texture_descriptor mln_metal_owned_texture_descriptor_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_metal_surface_attach(
        MlnMap map,
        mln_metal_surface_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_metal_surface_descriptor mln_metal_surface_descriptor_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_metal_surface_set_target(
        MlnRenderSession session,
        mln_metal_surface_descriptor* descriptor,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_network_status_get(
        uint* out_status,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_network_status_set(
        uint status,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_opengl_borrowed_texture_attach(
        MlnMap map,
        mln_opengl_borrowed_texture_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_opengl_borrowed_texture_descriptor mln_opengl_borrowed_texture_descriptor_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_opengl_borrowed_texture_set_target(
        MlnRenderSession session,
        mln_opengl_borrowed_texture_descriptor* descriptor,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_opengl_owned_texture_attach(
        MlnMap map,
        mln_opengl_owned_texture_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_opengl_owned_texture_descriptor mln_opengl_owned_texture_descriptor_default();

    [LibraryImport(LibraryName)]
    internal static partial uint mln_opengl_supported_context_provider_mask();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_opengl_surface_attach(
        MlnMap map,
        mln_opengl_surface_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_opengl_surface_descriptor mln_opengl_surface_descriptor_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_opengl_surface_set_target(
        MlnRenderSession session,
        mln_opengl_surface_descriptor* descriptor,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial void* mln_plugin_get_register_function_v1();

    [LibraryImport(LibraryName)]
    internal static partial mln_premultiplied_rgba8_image mln_premultiplied_rgba8_image_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_projected_meters_for_lat_lng(
        mln_lat_lng coordinate,
        mln_projected_meters* out_meters,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_projection_mode mln_projection_mode_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_frame_batch_count(
        MlnRenderFrameBatch batch,
        nuint* out_count,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_frame_batch_get(
        MlnRenderFrameBatch batch,
        nuint index,
        mln_render_frame_result* out_result,
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
    internal static partial mln_render_session_attach_options mln_render_session_attach_options_default();

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
    internal static partial mln_status mln_render_session_destroy(
        MlnRenderSession session,
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
    internal static partial mln_status mln_render_session_get_snapshot(
        MlnRenderSession session,
        mln_render_session_snapshot* out_snapshot,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_projection_create(
        MlnRenderSession session,
        MlnMapProjection* out_projection,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_query_feature_extensions(
        MlnRenderSession session,
        mln_buffer_view source_id,
        mln_buffer_view feature,
        mln_buffer_view extension,
        mln_buffer_view extension_field,
        mln_buffer_view* arguments,
        mln_completion* completion,
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
    internal static partial mln_status mln_render_session_reduce_memory_use(
        MlnRenderSession session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_request_frame(
        MlnRenderSession session,
        mln_frame_demand* demand,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_resize(
        MlnRenderSession session,
        mln_render_target_extent* extent,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_session_service_driver_work(
        MlnRenderSession session,
        nuint max_work,
        nuint* out_serviced,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_render_target_extent_physical_size(
        mln_render_target_extent* extent,
        uint* out_width,
        uint* out_height,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_rendered_feature_query_options mln_rendered_feature_query_options_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_rendered_query_geometry mln_rendered_query_geometry_box(
        mln_screen_box box
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_rendered_query_geometry mln_rendered_query_geometry_line_string(
        mln_screen_point* points,
        nuint point_count
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_rendered_query_geometry mln_rendered_query_geometry_point(
        mln_screen_point point
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_resource_request_cancelled(
        MlnResourceRequest handle,
        bool* out_cancelled,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_resource_request_complete(
        MlnResourceRequest handle,
        mln_resource_response* response,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial void mln_resource_request_release(MlnResourceRequest handle);

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_resource_request_set_cancel_callback(
        MlnResourceRequest handle,
        delegate* unmanaged[Cdecl]<void*, void> callback,
        void* user_data,
        delegate* unmanaged[Cdecl]<void*, void> release_user_data,
        bool* out_cancelled,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_resource_request_wait_until_retired(
        MlnResourceRequest handle,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_resource_transform_response_set_url(
        mln_resource_transform_response* response,
        sbyte* url,
        nuint url_size,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_barrier(
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
    internal static partial mln_status mln_runtime_dispose(
        MlnRuntime runtime,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_drain_events(
        MlnRuntime runtime,
        MlnEventBatch* out_batch,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_get_event_mask(
        MlnRuntime runtime,
        ulong* out_mask,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_offline_region_create(
        MlnRuntime runtime,
        mln_offline_region_definition* definition,
        byte* metadata,
        nuint metadata_size,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_offline_region_delete(
        MlnRuntime runtime,
        long region_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_offline_region_get(
        MlnRuntime runtime,
        long region_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_offline_region_get_status(
        MlnRuntime runtime,
        long region_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_offline_region_invalidate(
        MlnRuntime runtime,
        long region_id,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_offline_region_set_download_state(
        MlnRuntime runtime,
        long region_id,
        uint state,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_offline_region_set_observed(
        MlnRuntime runtime,
        long region_id,
        byte observed,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_offline_region_update_metadata(
        MlnRuntime runtime,
        long region_id,
        byte* metadata,
        nuint metadata_size,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_offline_regions_list(
        MlnRuntime runtime,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_offline_regions_merge_database(
        MlnRuntime runtime,
        sbyte* side_database_path,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_runtime_options mln_runtime_options_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_release(
        MlnRuntime runtime,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_run_ambient_cache_operation(
        MlnRuntime runtime,
        uint operation,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_runtime_set_event_mask(
        MlnRuntime runtime,
        ulong mask,
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
    internal static partial mln_source_feature_query_options mln_source_feature_query_options_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_style_image_options mln_style_image_options_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_style_tile_source_options mln_style_tile_source_options_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_style_transition_options mln_style_transition_options_default();

    [LibraryImport(LibraryName)]
    internal static partial uint mln_supported_render_backend_mask();

    [LibraryImport(LibraryName)]
    internal static partial mln_texture_image_info mln_texture_image_info_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_texture_read_premultiplied_rgba8(
        MlnRenderSession session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_vulkan_borrowed_texture_attach(
        MlnMap map,
        mln_vulkan_borrowed_texture_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_vulkan_borrowed_texture_descriptor mln_vulkan_borrowed_texture_descriptor_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_vulkan_borrowed_texture_set_target(
        MlnRenderSession session,
        mln_vulkan_borrowed_texture_descriptor* descriptor,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_vulkan_owned_texture_attach(
        MlnMap map,
        mln_vulkan_owned_texture_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_vulkan_owned_texture_descriptor mln_vulkan_owned_texture_descriptor_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_vulkan_surface_attach(
        MlnMap map,
        mln_vulkan_surface_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_vulkan_surface_descriptor mln_vulkan_surface_descriptor_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_vulkan_surface_set_target(
        MlnRenderSession session,
        mln_vulkan_surface_descriptor* descriptor,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_webgpu_borrowed_texture_attach(
        MlnMap map,
        mln_webgpu_borrowed_texture_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_webgpu_borrowed_texture_descriptor mln_webgpu_borrowed_texture_descriptor_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_webgpu_borrowed_texture_set_target(
        MlnRenderSession session,
        mln_webgpu_borrowed_texture_descriptor* descriptor,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_webgpu_owned_texture_attach(
        MlnMap map,
        mln_webgpu_owned_texture_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_webgpu_owned_texture_descriptor mln_webgpu_owned_texture_descriptor_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_webgpu_surface_attach(
        MlnMap map,
        mln_webgpu_surface_descriptor* descriptor,
        mln_render_session_attach_options* options,
        MlnRenderSession* out_session,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );

    [LibraryImport(LibraryName)]
    internal static partial mln_webgpu_surface_descriptor mln_webgpu_surface_descriptor_default();

    [LibraryImport(LibraryName)]
    internal static partial mln_status mln_webgpu_surface_set_target(
        MlnRenderSession session,
        mln_webgpu_surface_descriptor* descriptor,
        mln_completion* completion,
        mln_diagnostic* out_diagnostic
    );
}
