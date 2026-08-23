using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C
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
        MLN_ANIMATION_OPTION_TRANSITION_ID = 1U << 4,
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
        MLN_BOUND_OPTION_UNBOUNDED = 1U << 5,
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

    internal partial struct mln_logical_extent
    {
        [NativeTypeName("uint32_t")]
        public uint width;

        [NativeTypeName("uint32_t")]
        public uint height;

        public double scale_factor;
    }

    internal partial struct mln_map_options
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        public mln_logical_extent initial_extent;

        [NativeTypeName("uint32_t")]
        public uint map_mode;

        [NativeTypeName("bool")]
        public byte fast_pfor_enabled;

        [NativeTypeName("uint64_t")]
        public ulong event_mask;
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

        [NativeTypeName("uint64_t")]
        public ulong transition_id;
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_camera_delta_kind : uint
    {
        MLN_CAMERA_DELTA_MOVE = 0,
        MLN_CAMERA_DELTA_SCALE = 1,
        MLN_CAMERA_DELTA_BEARING = 2,
        MLN_CAMERA_DELTA_PITCH = 3,
    }

    internal partial struct mln_camera_delta
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint kind;

        public mln_screen_point offset;

        public double amount;

        [NativeTypeName("bool")]
        public byte has_anchor;

        public mln_screen_point anchor;

        public mln_animation_options animation;
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_camera_update_mode : uint
    {
        MLN_CAMERA_UPDATE_MODE_JUMP = 0,
        MLN_CAMERA_UPDATE_MODE_EASE = 1,
        MLN_CAMERA_UPDATE_MODE_FLY = 2,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_gesture_phase : uint
    {
        MLN_GESTURE_PHASE_NONE = 0,
        MLN_GESTURE_PHASE_BEGIN = 1,
        MLN_GESTURE_PHASE_UPDATE = 2,
        MLN_GESTURE_PHASE_END = 3,
        MLN_GESTURE_PHASE_CANCEL = 4,
    }

    internal partial struct mln_camera_update
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint mode;

        public mln_camera_options camera;

        public mln_animation_options animation;

        [NativeTypeName("uint32_t")]
        public uint gesture_phase;

        [NativeTypeName("uint32_t")]
        public uint reserved;
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

        public mln_buffer_view source_id;

        public mln_buffer_view source_layer_id;

        public mln_buffer_view feature_id;

        public mln_buffer_view state_key;
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

        public mln_buffer_view geometry;

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

        [NativeTypeName("__AnonymousRecord_map_L460_C3")]
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

    internal partial struct mln_map_snapshot
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint debug_options;

        [NativeTypeName("uint64_t")]
        public ulong generation;

        public mln_camera_options camera;

        public mln_logical_extent logical_extent;

        public mln_projection_mode projection_mode;

        public mln_map_viewport_options viewport;

        [NativeTypeName("bool")]
        public byte fully_loaded;

        [NativeTypeName("bool")]
        public byte rendering_stats_view_enabled;

        [NativeTypeName("bool")]
        public byte repaint_demand;

        [NativeTypeName("uint8_t")]
        public byte reserved_flags;

        [NativeTypeName("uint64_t")]
        public ulong event_mask;

        [NativeTypeName("uint64_t")]
        public ulong latest_render_update_generation;

        public mln_map_tile_options tile;

        public mln_bound_options bounds;

        public mln_free_camera_options free_camera;
    }

    internal partial struct mln_camera_query_result
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint reserved;

        [NativeTypeName("uint64_t")]
        public ulong generation;

        public mln_camera_options camera;
    }

    internal static unsafe partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_offline_region_create([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("const mln_offline_region_definition *")] mln_offline_region_definition* definition, [NativeTypeName("const uint8_t *")] byte* metadata, [NativeTypeName("size_t")] nuint metadata_size, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_offline_region_get([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("mln_offline_region_id")] long region_id, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_offline_regions_list([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_offline_regions_merge_database([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("const char *")] sbyte* side_database_path, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_offline_region_update_metadata([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("mln_offline_region_id")] long region_id, [NativeTypeName("const uint8_t *")] byte* metadata, [NativeTypeName("size_t")] nuint metadata_size, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_offline_region_get_status([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("mln_offline_region_id")] long region_id, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_offline_region_set_observed([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("mln_offline_region_id")] long region_id, [NativeTypeName("bool")] byte observed, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_offline_region_set_download_state([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("mln_offline_region_id")] long region_id, [NativeTypeName("uint32_t")] uint state, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_offline_region_invalidate([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("mln_offline_region_id")] long region_id, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_offline_region_delete([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("mln_offline_region_id")] long region_id, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_offline_region_snapshot_get([NativeTypeName("mln_offline_region_snapshot")] MlnOfflineRegionSnapshot snapshot, mln_offline_region_info* out_info);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_offline_region_snapshot_destroy([NativeTypeName("mln_offline_region_snapshot")] MlnOfflineRegionSnapshot snapshot);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_offline_region_list_count([NativeTypeName("mln_offline_region_list")] MlnOfflineRegionList list, [NativeTypeName("size_t *")] nuint* out_count);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_offline_region_list_get([NativeTypeName("mln_offline_region_list")] MlnOfflineRegionList list, [NativeTypeName("size_t")] nuint index, mln_offline_region_info* out_info);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_offline_region_list_destroy([NativeTypeName("mln_offline_region_list")] MlnOfflineRegionList list);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_map_options mln_map_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_create([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("const mln_map_options *")] mln_map_options* options, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_snapshot_get([NativeTypeName("mln_map")] MlnMap map, mln_map_snapshot* out_snapshot);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_resize([NativeTypeName("mln_map")] MlnMap map, mln_logical_extent extent, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_request_repaint([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_feature_state([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_feature_state_selector *")] mln_feature_state_selector* selector, mln_buffer_view state, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_feature_state([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_feature_state_selector *")] mln_feature_state_selector* selector, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_remove_feature_state([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_feature_state_selector *")] mln_feature_state_selector* selector, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_request_still_image([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_release([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_style_url([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const char *")] sbyte* url, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_style_json([NativeTypeName("mln_map")] MlnMap map, mln_buffer_view json, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_loaded_style_json([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_style_url([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_completion *")] mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_event_mask([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("uint64_t")] ulong mask, [NativeTypeName("const mln_completion *")] mln_completion* completion);
    }
}
