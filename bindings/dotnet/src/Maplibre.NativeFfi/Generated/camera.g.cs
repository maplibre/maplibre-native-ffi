using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C
{
    internal static unsafe partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_camera_options mln_camera_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_animation_options mln_animation_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_camera_update mln_camera_update_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_camera_fit_options mln_camera_fit_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_bound_options mln_bound_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_free_camera_options mln_free_camera_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_projection_mode mln_projection_mode_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_map_viewport_options mln_map_viewport_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_map_tile_options mln_map_tile_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_debug_options([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("uint32_t")] uint options, [NativeTypeName("uint64_t *")] ulong* out_command_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_debug_options_start([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_debug_options_take_result([NativeTypeName("mln_operation")] MlnOperation operation, [NativeTypeName("uint32_t *")] uint* out_options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_rendering_stats_view_enabled([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("bool")] byte enabled, [NativeTypeName("uint64_t *")] ulong* out_command_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_rendering_stats_view_enabled_start([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_rendering_stats_view_enabled_take_result([NativeTypeName("mln_operation")] MlnOperation operation, bool* out_enabled);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_is_fully_loaded_start([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_is_fully_loaded_take_result([NativeTypeName("mln_operation")] MlnOperation operation, bool* out_loaded);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_dump_debug_logs([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("uint64_t *")] ulong* out_command_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_viewport_options_start([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_viewport_options_take_result([NativeTypeName("mln_operation")] MlnOperation operation, mln_map_viewport_options* out_options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_viewport_options([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_map_viewport_options *")] mln_map_viewport_options* options, [NativeTypeName("uint64_t *")] ulong* out_command_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_tile_options_start([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_tile_options_take_result([NativeTypeName("mln_operation")] MlnOperation operation, mln_map_tile_options* out_options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_tile_options([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_map_tile_options *")] mln_map_tile_options* options, [NativeTypeName("uint64_t *")] ulong* out_command_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_snapshot_get([NativeTypeName("mln_map")] MlnMap map, mln_camera_options* out_camera, [NativeTypeName("uint64_t *")] ulong* out_generation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_update_camera([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_camera_update *")] mln_camera_update* update, [NativeTypeName("uint64_t *")] ulong* out_command_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_query_start([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_query_take_result([NativeTypeName("mln_operation")] MlnOperation operation, mln_camera_query_result* out_result);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_for_lat_lng_bounds_start([NativeTypeName("mln_map")] MlnMap map, mln_lat_lng_bounds bounds, [NativeTypeName("const mln_camera_fit_options *")] mln_camera_fit_options* fit_options, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_for_lat_lng_bounds_take_result([NativeTypeName("mln_operation")] MlnOperation operation, mln_camera_options* out_camera);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_for_lat_lngs_start([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_lat_lng *")] mln_lat_lng* coordinates, [NativeTypeName("size_t")] nuint coordinate_count, [NativeTypeName("const mln_camera_fit_options *")] mln_camera_fit_options* fit_options, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_for_lat_lngs_take_result([NativeTypeName("mln_operation")] MlnOperation operation, mln_camera_options* out_camera);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_for_geometry_start([NativeTypeName("mln_map")] MlnMap map, mln_buffer_view geometry, [NativeTypeName("const mln_camera_fit_options *")] mln_camera_fit_options* fit_options, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_for_geometry_take_result([NativeTypeName("mln_operation")] MlnOperation operation, mln_camera_options* out_camera);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lng_bounds_for_camera_start([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_camera_options *")] mln_camera_options* camera, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lng_bounds_for_camera_take_result([NativeTypeName("mln_operation")] MlnOperation operation, mln_lat_lng_bounds* out_bounds);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lng_bounds_for_camera_unwrapped_start([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_camera_options *")] mln_camera_options* camera, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lng_bounds_for_camera_unwrapped_take_result([NativeTypeName("mln_operation")] MlnOperation operation, mln_lat_lng_bounds* out_bounds);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_bounds_start([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_bounds_take_result([NativeTypeName("mln_operation")] MlnOperation operation, mln_bound_options* out_options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_bounds([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_bound_options *")] mln_bound_options* options, [NativeTypeName("uint64_t *")] ulong* out_command_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_free_camera_options_start([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_free_camera_options_take_result([NativeTypeName("mln_operation")] MlnOperation operation, mln_free_camera_options* out_options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_free_camera_options([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_free_camera_options *")] mln_free_camera_options* options, [NativeTypeName("uint64_t *")] ulong* out_command_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_projection_mode([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_projection_mode *")] mln_projection_mode* mode, [NativeTypeName("uint64_t *")] ulong* out_command_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_pixel_for_lat_lng_start([NativeTypeName("mln_map")] MlnMap map, mln_lat_lng coordinate, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_pixel_for_lat_lng_take_result([NativeTypeName("mln_operation")] MlnOperation operation, mln_screen_point* out_point);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lng_for_pixel_start([NativeTypeName("mln_map")] MlnMap map, mln_screen_point point, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lng_for_pixel_take_result([NativeTypeName("mln_operation")] MlnOperation operation, mln_lat_lng* out_coordinate);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_pixels_for_lat_lngs_start([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_lat_lng *")] mln_lat_lng* coordinates, [NativeTypeName("size_t")] nuint coordinate_count, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_pixels_for_lat_lngs_take_result([NativeTypeName("mln_operation")] MlnOperation operation, mln_screen_point* out_points, [NativeTypeName("size_t")] nuint point_capacity, [NativeTypeName("size_t *")] nuint* out_point_count);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lngs_for_pixels_start([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_screen_point *")] mln_screen_point* points, [NativeTypeName("size_t")] nuint point_count, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lngs_for_pixels_take_result([NativeTypeName("mln_operation")] MlnOperation operation, mln_lat_lng* out_coordinates, [NativeTypeName("size_t")] nuint coordinate_capacity, [NativeTypeName("size_t *")] nuint* out_coordinate_count);
    }
}
