using System.Runtime.InteropServices;

namespace Maplibre.Native.Internal.C
{
  [NativeTypeName("uint32_t")]
  internal enum mln_camera_option_field : uint
  {
    MLN_CAMERA_OPTION_CENTER = 1U << 0,
    MLN_CAMERA_OPTION_ZOOM = 1U << 1,
    MLN_CAMERA_OPTION_BEARING = 1U << 2,
    MLN_CAMERA_OPTION_PITCH = 1U << 3,
    MLN_CAMERA_OPTION_CENTER_ALTITUDE = 1U << 4,
    MLN_CAMERA_OPTION_PADDING = 1U << 5,
    MLN_CAMERA_OPTION_ANCHOR = 1U << 6,
    MLN_CAMERA_OPTION_ROLL = 1U << 7,
    MLN_CAMERA_OPTION_FOV = 1U << 8,
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_animation_option_field : uint
  {
    MLN_ANIMATION_OPTION_DURATION = 1U << 0,
    MLN_ANIMATION_OPTION_VELOCITY = 1U << 1,
    MLN_ANIMATION_OPTION_MIN_ZOOM = 1U << 2,
    MLN_ANIMATION_OPTION_EASING = 1U << 3,
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_camera_fit_option_field : uint
  {
    MLN_CAMERA_FIT_OPTION_PADDING = 1U << 0,
    MLN_CAMERA_FIT_OPTION_BEARING = 1U << 1,
    MLN_CAMERA_FIT_OPTION_PITCH = 1U << 2,
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_bound_option_field : uint
  {
    MLN_BOUND_OPTION_BOUNDS = 1U << 0,
    MLN_BOUND_OPTION_MIN_ZOOM = 1U << 1,
    MLN_BOUND_OPTION_MAX_ZOOM = 1U << 2,
    MLN_BOUND_OPTION_MIN_PITCH = 1U << 3,
    MLN_BOUND_OPTION_MAX_PITCH = 1U << 4,
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_free_camera_option_field : uint
  {
    MLN_FREE_CAMERA_OPTION_POSITION = 1U << 0,
    MLN_FREE_CAMERA_OPTION_ORIENTATION = 1U << 1,
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_projection_mode_field : uint
  {
    MLN_PROJECTION_MODE_AXONOMETRIC = 1U << 0,
    MLN_PROJECTION_MODE_X_SKEW = 1U << 1,
    MLN_PROJECTION_MODE_Y_SKEW = 1U << 2,
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_map_debug_option : uint
  {
    MLN_MAP_DEBUG_TILE_BORDERS = 1U << 1,
    MLN_MAP_DEBUG_PARSE_STATUS = 1U << 2,
    MLN_MAP_DEBUG_TIMESTAMPS = 1U << 3,
    MLN_MAP_DEBUG_COLLISION = 1U << 4,
    MLN_MAP_DEBUG_OVERDRAW = 1U << 5,
    MLN_MAP_DEBUG_STENCIL_CLIP = 1U << 6,
    MLN_MAP_DEBUG_DEPTH_BUFFER = 1U << 7,
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_north_orientation : uint
  {
    MLN_NORTH_ORIENTATION_UP = 0,
    MLN_NORTH_ORIENTATION_RIGHT = 1,
    MLN_NORTH_ORIENTATION_DOWN = 2,
    MLN_NORTH_ORIENTATION_LEFT = 3,
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_constrain_mode : uint
  {
    MLN_CONSTRAIN_MODE_NONE = 0,
    MLN_CONSTRAIN_MODE_HEIGHT_ONLY = 1,
    MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT = 2,
    MLN_CONSTRAIN_MODE_SCREEN = 3,
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_viewport_mode : uint
  {
    MLN_VIEWPORT_MODE_DEFAULT = 0,
    MLN_VIEWPORT_MODE_FLIPPED_Y = 1,
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_map_viewport_option_field : uint
  {
    MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION = 1U << 0,
    MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE = 1U << 1,
    MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE = 1U << 2,
    MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET = 1U << 3,
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_tile_lod_mode : uint
  {
    MLN_TILE_LOD_MODE_DEFAULT = 0,
    MLN_TILE_LOD_MODE_DISTANCE = 1,
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_map_tile_option_field : uint
  {
    MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA = 1U << 0,
    MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS = 1U << 1,
    MLN_MAP_TILE_OPTION_LOD_SCALE = 1U << 2,
    MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD = 1U << 3,
    MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT = 1U << 4,
    MLN_MAP_TILE_OPTION_LOD_MODE = 1U << 5,
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_map_mode : uint
  {
    MLN_MAP_MODE_CONTINUOUS = 0,
    MLN_MAP_MODE_STATIC = 1,
    MLN_MAP_MODE_TILE = 2,
  }

  internal partial struct mln_map_options
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint width;

    [NativeTypeName("uint32_t")]
    public uint height;

    public double scale_factor;

    [NativeTypeName("uint32_t")]
    public uint map_mode;
  }

  internal partial struct mln_screen_point
  {
    public double x;

    public double y;
  }

  internal partial struct mln_edge_insets
  {
    public double top;

    public double left;

    public double bottom;

    public double right;
  }

  internal partial struct mln_camera_options
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint fields;

    public double latitude;

    public double longitude;

    public double center_altitude;

    public mln_edge_insets padding;

    public mln_screen_point anchor;

    public double zoom;

    public double bearing;

    public double pitch;

    public double roll;

    public double field_of_view;
  }

  internal partial struct mln_unit_bezier
  {
    public double x1;

    public double y1;

    public double x2;

    public double y2;
  }

  internal partial struct mln_animation_options
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint fields;

    public double duration_ms;

    public double velocity;

    public double min_zoom;

    public mln_unit_bezier easing;
  }

  internal partial struct mln_camera_fit_options
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint fields;

    public mln_edge_insets padding;

    public double bearing;

    public double pitch;
  }

  internal partial struct mln_vec3
  {
    public double x;

    public double y;

    public double z;
  }

  internal partial struct mln_quaternion
  {
    public double x;

    public double y;

    public double z;

    public double w;
  }

  internal partial struct mln_free_camera_options
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint fields;

    public mln_vec3 position;

    public mln_quaternion orientation;
  }

  internal partial struct mln_lat_lng
  {
    public double latitude;

    public double longitude;
  }

  internal unsafe partial struct mln_string_view
  {
    [NativeTypeName("const char *")]
    public sbyte* data;

    [NativeTypeName("size_t")]
    public nuint size;
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_geometry_type : uint
  {
    MLN_GEOMETRY_TYPE_EMPTY = 0,
    MLN_GEOMETRY_TYPE_POINT = 1,
    MLN_GEOMETRY_TYPE_LINE_STRING = 2,
    MLN_GEOMETRY_TYPE_POLYGON = 3,
    MLN_GEOMETRY_TYPE_MULTI_POINT = 4,
    MLN_GEOMETRY_TYPE_MULTI_LINE_STRING = 5,
    MLN_GEOMETRY_TYPE_MULTI_POLYGON = 6,
    MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION = 7,
  }

  internal unsafe partial struct mln_coordinate_span
  {
    [NativeTypeName("const mln_lat_lng *")]
    public mln_lat_lng* coordinates;

    [NativeTypeName("size_t")]
    public nuint coordinate_count;
  }

  internal unsafe partial struct mln_polygon_geometry
  {
    [NativeTypeName("const mln_coordinate_span *")]
    public mln_coordinate_span* rings;

    [NativeTypeName("size_t")]
    public nuint ring_count;
  }

  internal unsafe partial struct mln_multi_line_geometry
  {
    [NativeTypeName("const mln_coordinate_span *")]
    public mln_coordinate_span* lines;

    [NativeTypeName("size_t")]
    public nuint line_count;
  }

  internal unsafe partial struct mln_multi_polygon_geometry
  {
    [NativeTypeName("const mln_polygon_geometry *")]
    public mln_polygon_geometry* polygons;

    [NativeTypeName("size_t")]
    public nuint polygon_count;
  }

  internal unsafe partial struct mln_geometry_collection
  {
    [NativeTypeName("const mln_geometry *")]
    public mln_geometry* geometries;

    [NativeTypeName("size_t")]
    public nuint geometry_count;
  }

  internal partial struct mln_geometry
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint type;

    [NativeTypeName("__AnonymousRecord_map_L316_C3")]
    public _data_e__Union data;

    [StructLayout(LayoutKind.Explicit)]
    internal partial struct _data_e__Union
    {
      [FieldOffset(0)]
      public mln_lat_lng point;

      [FieldOffset(0)]
      public mln_coordinate_span line_string;

      [FieldOffset(0)]
      public mln_polygon_geometry polygon;

      [FieldOffset(0)]
      public mln_coordinate_span multi_point;

      [FieldOffset(0)]
      public mln_multi_line_geometry multi_line_string;

      [FieldOffset(0)]
      public mln_multi_polygon_geometry multi_polygon;

      [FieldOffset(0)]
      public mln_geometry_collection geometry_collection;
    }
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_json_value_type : uint
  {
    MLN_JSON_VALUE_TYPE_NULL = 0,
    MLN_JSON_VALUE_TYPE_BOOL = 1,
    MLN_JSON_VALUE_TYPE_UINT = 2,
    MLN_JSON_VALUE_TYPE_INT = 3,
    MLN_JSON_VALUE_TYPE_DOUBLE = 4,
    MLN_JSON_VALUE_TYPE_STRING = 5,
    MLN_JSON_VALUE_TYPE_ARRAY = 6,
    MLN_JSON_VALUE_TYPE_OBJECT = 7,
  }

  internal unsafe partial struct mln_json_array
  {
    [NativeTypeName("const mln_json_value *")]
    public mln_json_value* values;

    [NativeTypeName("size_t")]
    public nuint value_count;
  }

  internal unsafe partial struct mln_json_member
  {
    public mln_string_view key;

    [NativeTypeName("const mln_json_value *")]
    public mln_json_value* value;
  }

  internal unsafe partial struct mln_json_object
  {
    [NativeTypeName("const mln_json_member *")]
    public mln_json_member* members;

    [NativeTypeName("size_t")]
    public nuint member_count;
  }

  internal partial struct mln_json_value
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint type;

    [NativeTypeName("__AnonymousRecord_map_L373_C3")]
    public _data_e__Union data;

    [StructLayout(LayoutKind.Explicit)]
    internal partial struct _data_e__Union
    {
      [FieldOffset(0)]
      [NativeTypeName("bool")]
      public byte bool_value;

      [FieldOffset(0)]
      [NativeTypeName("uint64_t")]
      public ulong uint_value;

      [FieldOffset(0)]
      [NativeTypeName("int64_t")]
      public long int_value;

      [FieldOffset(0)]
      public double double_value;

      [FieldOffset(0)]
      public mln_string_view string_value;

      [FieldOffset(0)]
      public mln_json_array array_value;

      [FieldOffset(0)]
      public mln_json_object object_value;
    }
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_feature_state_selector_field : uint
  {
    MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID = 1U << 0,
    MLN_FEATURE_STATE_SELECTOR_FEATURE_ID = 1U << 1,
    MLN_FEATURE_STATE_SELECTOR_STATE_KEY = 1U << 2,
  }

  internal partial struct mln_feature_state_selector
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint fields;

    public mln_string_view source_id;

    public mln_string_view source_layer_id;

    public mln_string_view feature_id;

    public mln_string_view state_key;
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_feature_identifier_type : uint
  {
    MLN_FEATURE_IDENTIFIER_TYPE_NULL = 0,
    MLN_FEATURE_IDENTIFIER_TYPE_UINT = 1,
    MLN_FEATURE_IDENTIFIER_TYPE_INT = 2,
    MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE = 3,
    MLN_FEATURE_IDENTIFIER_TYPE_STRING = 4,
  }

  internal unsafe partial struct mln_feature
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("const mln_geometry *")]
    public mln_geometry* geometry;

    [NativeTypeName("const mln_json_member *")]
    public mln_json_member* properties;

    [NativeTypeName("size_t")]
    public nuint property_count;

    [NativeTypeName("uint32_t")]
    public uint identifier_type;

    [NativeTypeName("__AnonymousRecord_map_L428_C3")]
    public _identifier_e__Union identifier;

    [StructLayout(LayoutKind.Explicit)]
    internal partial struct _identifier_e__Union
    {
      [FieldOffset(0)]
      [NativeTypeName("uint64_t")]
      public ulong uint_value;

      [FieldOffset(0)]
      [NativeTypeName("int64_t")]
      public long int_value;

      [FieldOffset(0)]
      public double double_value;

      [FieldOffset(0)]
      public mln_string_view string_value;
    }
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_geojson_type : uint
  {
    MLN_GEOJSON_TYPE_GEOMETRY = 1,
    MLN_GEOJSON_TYPE_FEATURE = 2,
    MLN_GEOJSON_TYPE_FEATURE_COLLECTION = 3,
  }

  internal unsafe partial struct mln_feature_collection
  {
    [NativeTypeName("const mln_feature *")]
    public mln_feature* features;

    [NativeTypeName("size_t")]
    public nuint feature_count;
  }

  internal partial struct mln_geojson
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint type;

    [NativeTypeName("__AnonymousRecord_map_L459_C3")]
    public _data_e__Union data;

    [StructLayout(LayoutKind.Explicit)]
    internal unsafe partial struct _data_e__Union
    {
      [FieldOffset(0)]
      [NativeTypeName("const mln_geometry *")]
      public mln_geometry* geometry;

      [FieldOffset(0)]
      [NativeTypeName("const mln_feature *")]
      public mln_feature* feature;

      [FieldOffset(0)]
      public mln_feature_collection feature_collection;
    }
  }

  internal partial struct mln_lat_lng_bounds
  {
    public mln_lat_lng southwest;

    public mln_lat_lng northeast;
  }

  internal partial struct mln_bound_options
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint fields;

    public mln_lat_lng_bounds bounds;

    public double min_zoom;

    public double max_zoom;

    public double min_pitch;

    public double max_pitch;
  }

  internal unsafe partial struct mln_offline_tile_pyramid_region_definition
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("const char *")]
    public sbyte* style_url;

    public mln_lat_lng_bounds bounds;

    public double min_zoom;

    public double max_zoom;

    public float pixel_ratio;

    [NativeTypeName("bool")]
    public byte include_ideographs;
  }

  internal unsafe partial struct mln_offline_geometry_region_definition
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("const char *")]
    public sbyte* style_url;

    [NativeTypeName("const mln_geometry *")]
    public mln_geometry* geometry;

    public double min_zoom;

    public double max_zoom;

    public float pixel_ratio;

    [NativeTypeName("bool")]
    public byte include_ideographs;
  }

  internal partial struct mln_offline_region_definition
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint type;

    [NativeTypeName("__AnonymousRecord_map_L529_C3")]
    public _data_e__Union data;

    [StructLayout(LayoutKind.Explicit)]
    internal partial struct _data_e__Union
    {
      [FieldOffset(0)]
      public mln_offline_tile_pyramid_region_definition tile_pyramid;

      [FieldOffset(0)]
      public mln_offline_geometry_region_definition geometry;
    }
  }

  internal unsafe partial struct mln_offline_region_info
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("mln_offline_region_id")]
    public long id;

    public mln_offline_region_definition definition;

    [NativeTypeName("const uint8_t *")]
    public byte* metadata;

    [NativeTypeName("size_t")]
    public nuint metadata_size;
  }

  internal partial struct mln_projected_meters
  {
    public double northing;

    public double easting;
  }

  internal partial struct mln_projection_mode
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint fields;

    [NativeTypeName("bool")]
    public byte axonometric;

    public double x_skew;

    public double y_skew;
  }

  internal partial struct mln_map_viewport_options
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint fields;

    [NativeTypeName("uint32_t")]
    public uint north_orientation;

    [NativeTypeName("uint32_t")]
    public uint constrain_mode;

    [NativeTypeName("uint32_t")]
    public uint viewport_mode;

    public mln_edge_insets frustum_offset;
  }

  internal partial struct mln_map_tile_options
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint fields;

    [NativeTypeName("uint32_t")]
    public uint prefetch_zoom_delta;

    public double lod_min_radius;

    public double lod_scale;

    public double lod_pitch_threshold;

    public double lod_zoom_shift;

    [NativeTypeName("uint32_t")]
    public uint lod_mode;
  }

  internal static unsafe partial class NativeMethods
  {
    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_runtime_offline_region_create_start(mln_runtime* runtime, [NativeTypeName("const mln_offline_region_definition *")] mln_offline_region_definition* definition, [NativeTypeName("const uint8_t *")] byte* metadata, [NativeTypeName("size_t")] nuint metadata_size, [NativeTypeName("mln_offline_operation_id *")] ulong* out_operation_id);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_runtime_offline_region_get_start(mln_runtime* runtime, [NativeTypeName("mln_offline_region_id")] long region_id, [NativeTypeName("mln_offline_operation_id *")] ulong* out_operation_id);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_runtime_offline_regions_list_start(mln_runtime* runtime, [NativeTypeName("mln_offline_operation_id *")] ulong* out_operation_id);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_runtime_offline_regions_merge_database_start(mln_runtime* runtime, [NativeTypeName("const char *")] sbyte* side_database_path, [NativeTypeName("mln_offline_operation_id *")] ulong* out_operation_id);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_runtime_offline_region_update_metadata_start(mln_runtime* runtime, [NativeTypeName("mln_offline_region_id")] long region_id, [NativeTypeName("const uint8_t *")] byte* metadata, [NativeTypeName("size_t")] nuint metadata_size, [NativeTypeName("mln_offline_operation_id *")] ulong* out_operation_id);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_runtime_offline_region_get_status_start(mln_runtime* runtime, [NativeTypeName("mln_offline_region_id")] long region_id, [NativeTypeName("mln_offline_operation_id *")] ulong* out_operation_id);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_runtime_offline_region_set_observed_start(mln_runtime* runtime, [NativeTypeName("mln_offline_region_id")] long region_id, [NativeTypeName("bool")] byte observed, [NativeTypeName("mln_offline_operation_id *")] ulong* out_operation_id);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_runtime_offline_region_set_download_state_start(mln_runtime* runtime, [NativeTypeName("mln_offline_region_id")] long region_id, [NativeTypeName("uint32_t")] uint state, [NativeTypeName("mln_offline_operation_id *")] ulong* out_operation_id);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_runtime_offline_region_invalidate_start(mln_runtime* runtime, [NativeTypeName("mln_offline_region_id")] long region_id, [NativeTypeName("mln_offline_operation_id *")] ulong* out_operation_id);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_runtime_offline_region_delete_start(mln_runtime* runtime, [NativeTypeName("mln_offline_region_id")] long region_id, [NativeTypeName("mln_offline_operation_id *")] ulong* out_operation_id);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_runtime_offline_region_create_take_result(mln_runtime* runtime, [NativeTypeName("mln_offline_operation_id")] ulong operation_id, mln_offline_region_snapshot** out_region);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_runtime_offline_region_get_take_result(mln_runtime* runtime, [NativeTypeName("mln_offline_operation_id")] ulong operation_id, mln_offline_region_snapshot** out_region, bool* out_found);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_runtime_offline_regions_list_take_result(mln_runtime* runtime, [NativeTypeName("mln_offline_operation_id")] ulong operation_id, mln_offline_region_list** out_regions);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_runtime_offline_regions_merge_database_take_result(mln_runtime* runtime, [NativeTypeName("mln_offline_operation_id")] ulong operation_id, mln_offline_region_list** out_regions);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_runtime_offline_region_update_metadata_take_result(mln_runtime* runtime, [NativeTypeName("mln_offline_operation_id")] ulong operation_id, mln_offline_region_snapshot** out_region);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_runtime_offline_region_get_status_take_result(mln_runtime* runtime, [NativeTypeName("mln_offline_operation_id")] ulong operation_id, mln_offline_region_status* out_status);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_offline_region_snapshot_get([NativeTypeName("const mln_offline_region_snapshot *")] mln_offline_region_snapshot* snapshot, mln_offline_region_info* out_info);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern void mln_offline_region_snapshot_destroy(mln_offline_region_snapshot* snapshot);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_offline_region_list_count([NativeTypeName("const mln_offline_region_list *")] mln_offline_region_list* list, [NativeTypeName("size_t *")] nuint* out_count);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_offline_region_list_get([NativeTypeName("const mln_offline_region_list *")] mln_offline_region_list* list, [NativeTypeName("size_t")] nuint index, mln_offline_region_info* out_info);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern void mln_offline_region_list_destroy(mln_offline_region_list* list);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_map_options mln_map_options_default();

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_map_create(mln_runtime* runtime, [NativeTypeName("const mln_map_options *")] mln_map_options* options, mln_map** out_map);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_map_request_repaint(mln_map* map);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_map_request_still_image(mln_map* map);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_map_destroy(mln_map* map);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_map_set_style_url(mln_map* map, [NativeTypeName("const char *")] sbyte* url);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_map_set_style_json(mln_map* map, [NativeTypeName("const char *")] sbyte* json);
  }
}
