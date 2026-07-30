using System.Runtime.InteropServices;

namespace Maplibre.Native.Internal.C
{
    internal static unsafe partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_camera_options mln_camera_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_animation_options mln_animation_options_default();

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
        public static extern mln_status mln_map_set_debug_options([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("uint32_t")] uint options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_debug_options([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("uint32_t *")] uint* out_options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_rendering_stats_view_enabled([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("bool")] byte enabled);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_rendering_stats_view_enabled([NativeTypeName("mln_map")] MlnMap map, bool* out_enabled);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_is_fully_loaded([NativeTypeName("mln_map")] MlnMap map, bool* out_loaded);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_dump_debug_logs([NativeTypeName("mln_map")] MlnMap map);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_viewport_options([NativeTypeName("mln_map")] MlnMap map, mln_map_viewport_options* out_options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_viewport_options([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_map_viewport_options *")] mln_map_viewport_options* options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_tile_options([NativeTypeName("mln_map")] MlnMap map, mln_map_tile_options* out_options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_tile_options([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_map_tile_options *")] mln_map_tile_options* options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_camera([NativeTypeName("mln_map")] MlnMap map, mln_camera_options* out_camera);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_jump_to([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_camera_options *")] mln_camera_options* camera);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_ease_to([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_camera_options *")] mln_camera_options* camera, [NativeTypeName("const mln_animation_options *")] mln_animation_options* animation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_fly_to([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_camera_options *")] mln_camera_options* camera, [NativeTypeName("const mln_animation_options *")] mln_animation_options* animation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_move_by([NativeTypeName("mln_map")] MlnMap map, double delta_x, double delta_y);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_move_by_animated([NativeTypeName("mln_map")] MlnMap map, double delta_x, double delta_y, [NativeTypeName("const mln_animation_options *")] mln_animation_options* animation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_scale_by([NativeTypeName("mln_map")] MlnMap map, double scale, [NativeTypeName("const mln_screen_point *")] mln_screen_point* anchor);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_scale_by_animated([NativeTypeName("mln_map")] MlnMap map, double scale, [NativeTypeName("const mln_screen_point *")] mln_screen_point* anchor, [NativeTypeName("const mln_animation_options *")] mln_animation_options* animation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_rotate_by([NativeTypeName("mln_map")] MlnMap map, mln_screen_point first, mln_screen_point second);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_rotate_by_animated([NativeTypeName("mln_map")] MlnMap map, mln_screen_point first, mln_screen_point second, [NativeTypeName("const mln_animation_options *")] mln_animation_options* animation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_pitch_by([NativeTypeName("mln_map")] MlnMap map, double pitch);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_pitch_by_animated([NativeTypeName("mln_map")] MlnMap map, double pitch, [NativeTypeName("const mln_animation_options *")] mln_animation_options* animation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_cancel_transitions([NativeTypeName("mln_map")] MlnMap map);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_for_lat_lng_bounds([NativeTypeName("mln_map")] MlnMap map, mln_lat_lng_bounds bounds, [NativeTypeName("const mln_camera_fit_options *")] mln_camera_fit_options* fit_options, mln_camera_options* out_camera);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_for_lat_lngs([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_lat_lng *")] mln_lat_lng* coordinates, [NativeTypeName("size_t")] nuint coordinate_count, [NativeTypeName("const mln_camera_fit_options *")] mln_camera_fit_options* fit_options, mln_camera_options* out_camera);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_for_geometry([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_geometry *")] mln_geometry* geometry, [NativeTypeName("const mln_camera_fit_options *")] mln_camera_fit_options* fit_options, mln_camera_options* out_camera);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lng_bounds_for_camera([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_camera_options *")] mln_camera_options* camera, mln_lat_lng_bounds* out_bounds);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lng_bounds_for_camera_unwrapped([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_camera_options *")] mln_camera_options* camera, mln_lat_lng_bounds* out_bounds);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_bounds([NativeTypeName("mln_map")] MlnMap map, mln_bound_options* out_options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_bounds([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_bound_options *")] mln_bound_options* options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_free_camera_options([NativeTypeName("mln_map")] MlnMap map, mln_free_camera_options* out_options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_free_camera_options([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_free_camera_options *")] mln_free_camera_options* options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_projection_mode([NativeTypeName("mln_map")] MlnMap map, mln_projection_mode* out_mode);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_projection_mode([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_projection_mode *")] mln_projection_mode* mode);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_pixel_for_lat_lng([NativeTypeName("mln_map")] MlnMap map, mln_lat_lng coordinate, mln_screen_point* out_point);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lng_for_pixel([NativeTypeName("mln_map")] MlnMap map, mln_screen_point point, mln_lat_lng* out_coordinate);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_pixels_for_lat_lngs([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_lat_lng *")] mln_lat_lng* coordinates, [NativeTypeName("size_t")] nuint coordinate_count, mln_screen_point* out_points);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lngs_for_pixels([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_screen_point *")] mln_screen_point* points, [NativeTypeName("size_t")] nuint point_count, mln_lat_lng* out_coordinates);
    }
}
