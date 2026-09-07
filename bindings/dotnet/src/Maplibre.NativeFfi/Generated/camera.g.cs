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
        public static extern mln_camera_delta mln_camera_delta_default();

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
        public static extern mln_status mln_map_set_debug_options([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("uint32_t")] uint options, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_rendering_stats_view_enabled([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("bool")] byte enabled, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_dump_debug_logs([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_viewport_options([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_map_viewport_options *")] mln_map_viewport_options* options, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_tile_options([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_map_tile_options *")] mln_map_tile_options* options, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_snapshot_get([NativeTypeName("mln_map")] MlnMap map, mln_camera_options* out_camera, [NativeTypeName("uint64_t *")] ulong* out_generation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_update_camera([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_camera_update *")] mln_camera_update* update, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_apply_camera_delta([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_camera_delta *")] mln_camera_delta* delta, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_cancel_transitions([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_query([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_for_lat_lng_bounds([NativeTypeName("mln_map")] MlnMap map, mln_lat_lng_bounds bounds, [NativeTypeName("const mln_camera_fit_options *")] mln_camera_fit_options* fit_options, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_for_lat_lngs([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_lat_lng *")] mln_lat_lng* coordinates, [NativeTypeName("size_t")] nuint coordinate_count, [NativeTypeName("const mln_camera_fit_options *")] mln_camera_fit_options* fit_options, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_camera_for_geometry([NativeTypeName("mln_map")] MlnMap map, mln_buffer_view geometry, [NativeTypeName("const mln_camera_fit_options *")] mln_camera_fit_options* fit_options, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lng_bounds_for_camera([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_camera_options *")] mln_camera_options* camera, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lng_bounds_for_camera_unwrapped([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_camera_options *")] mln_camera_options* camera, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_bounds([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_bound_options *")] mln_bound_options* options, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_free_camera_options([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_free_camera_options *")] mln_free_camera_options* options, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_projection_mode([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_projection_mode *")] mln_projection_mode* mode, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_pixel_for_lat_lng([NativeTypeName("mln_map")] MlnMap map, mln_lat_lng coordinate, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lng_for_pixel([NativeTypeName("mln_map")] MlnMap map, mln_screen_point point, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lng_for_pixel_unwrapped([NativeTypeName("mln_map")] MlnMap map, mln_screen_point point, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_pixels_for_lat_lngs([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_lat_lng *")] mln_lat_lng* coordinates, [NativeTypeName("size_t")] nuint coordinate_count, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lngs_for_pixels([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_screen_point *")] mln_screen_point* points, [NativeTypeName("size_t")] nuint point_count, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_lat_lngs_for_pixels_unwrapped([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_screen_point *")] mln_screen_point* points, [NativeTypeName("size_t")] nuint point_count, [NativeTypeName("const mln_completion *")] mln_completion* completion);
    }
}
