using System.Runtime.InteropServices;

namespace Maplibre.Native.Internal.C
{
  internal static unsafe partial class NativeMethods
  {
    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_map_projection_create(mln_map* map, mln_map_projection** out_projection);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_map_projection_destroy(mln_map_projection* projection);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_map_projection_get_camera(mln_map_projection* projection, mln_camera_options* out_camera);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_map_projection_set_camera(mln_map_projection* projection, [NativeTypeName("const mln_camera_options *")] mln_camera_options* camera);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_map_projection_set_visible_coordinates(mln_map_projection* projection, [NativeTypeName("const mln_lat_lng *")] mln_lat_lng* coordinates, [NativeTypeName("size_t")] nuint coordinate_count, mln_edge_insets padding);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_map_projection_set_visible_geometry(mln_map_projection* projection, [NativeTypeName("const mln_geometry *")] mln_geometry* geometry, mln_edge_insets padding);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_map_projection_pixel_for_lat_lng(mln_map_projection* projection, mln_lat_lng coordinate, mln_screen_point* out_point);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_map_projection_lat_lng_for_pixel(mln_map_projection* projection, mln_screen_point point, mln_lat_lng* out_coordinate);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_projected_meters_for_lat_lng(mln_lat_lng coordinate, mln_projected_meters* out_meters);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_lat_lng_for_projected_meters(mln_projected_meters meters, mln_lat_lng* out_coordinate);
  }
}
