using System.Runtime.InteropServices;

namespace Maplibre.Native.Internal.C
{
    [NativeTypeName("uint32_t")]
    internal enum mln_style_source_type : uint
    {
        MLN_STYLE_SOURCE_TYPE_UNKNOWN = 0,
        MLN_STYLE_SOURCE_TYPE_VECTOR = 1,
        MLN_STYLE_SOURCE_TYPE_RASTER = 2,
        MLN_STYLE_SOURCE_TYPE_RASTER_DEM = 3,
        MLN_STYLE_SOURCE_TYPE_GEOJSON = 4,
        MLN_STYLE_SOURCE_TYPE_IMAGE = 5,
        MLN_STYLE_SOURCE_TYPE_VIDEO = 6,
        MLN_STYLE_SOURCE_TYPE_ANNOTATIONS = 7,
        MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR = 8,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_style_tile_source_option_field : uint
    {
        MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM = 1U << 0,
        MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM = 1U << 1,
        MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION = 1U << 2,
        MLN_STYLE_TILE_SOURCE_OPTION_SCHEME = 1U << 3,
        MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS = 1U << 4,
        MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE = 1U << 5,
        MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING = 1U << 6,
        MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING = 1U << 7,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_style_tile_scheme : uint
    {
        MLN_STYLE_TILE_SCHEME_XYZ = 0,
        MLN_STYLE_TILE_SCHEME_TMS = 1,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_style_vector_tile_encoding : uint
    {
        MLN_STYLE_VECTOR_TILE_ENCODING_MVT = 0,
        MLN_STYLE_VECTOR_TILE_ENCODING_MLT = 1,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_style_raster_dem_encoding : uint
    {
        MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX = 0,
        MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM = 1,
    }

    internal partial struct mln_image_stretch
    {
        public float from;

        public float to;
    }

    internal partial struct mln_image_content
    {
        public float left;

        public float top;

        public float right;

        public float bottom;
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_style_image_text_fit : uint
    {
        MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK = 0,
        MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY = 1,
        MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL = 2,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_style_layer_visibility : uint
    {
        MLN_STYLE_LAYER_VISIBILITY_VISIBLE = 0,
        MLN_STYLE_LAYER_VISIBILITY_NONE = 1,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_geojson_source_option_field : uint
    {
        MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM = 1U << 0,
        MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM = 1U << 1,
        MLN_GEOJSON_SOURCE_OPTION_TOLERANCE = 1U << 2,
        MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM = 1U << 3,
        MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES = 1U << 4,
        MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE = 1U << 5,
        MLN_GEOJSON_SOURCE_OPTION_BUFFER = 1U << 6,
        MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS = 1U << 7,
        MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS = 1U << 8,
        MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS = 1U << 9,
        MLN_GEOJSON_SOURCE_OPTION_CLUSTER = 1U << 10,
        MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_UPDATE = 1U << 11,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_custom_geometry_source_option_field : uint
    {
        MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM = 1U << 0,
        MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM = 1U << 1,
        MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE = 1U << 2,
        MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE = 1U << 3,
        MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER = 1U << 4,
        MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP = 1U << 5,
        MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP = 1U << 6,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_style_image_option_field : uint
    {
        MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO = 1U << 0,
        MLN_STYLE_IMAGE_OPTION_SDF = 1U << 1,
        MLN_STYLE_IMAGE_OPTION_STRETCH_X = 1U << 2,
        MLN_STYLE_IMAGE_OPTION_STRETCH_Y = 1U << 3,
        MLN_STYLE_IMAGE_OPTION_CONTENT = 1U << 4,
        MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH = 1U << 5,
        MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT = 1U << 6,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_style_transition_option_field : uint
    {
        MLN_STYLE_TRANSITION_OPTION_DURATION = 1U << 0,
        MLN_STYLE_TRANSITION_OPTION_DELAY = 1U << 1,
        MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS = 1U << 2,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_location_indicator_image_kind : uint
    {
        MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP = 0,
        MLN_LOCATION_INDICATOR_IMAGE_KIND_BEARING = 1,
        MLN_LOCATION_INDICATOR_IMAGE_KIND_SHADOW = 2,
    }

    internal partial struct mln_style_source_info
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint type;

        [NativeTypeName("size_t")]
        public nuint id_size;

        [NativeTypeName("bool")]
        public byte is_volatile;

        [NativeTypeName("bool")]
        public byte has_attribution;

        [NativeTypeName("size_t")]
        public nuint attribution_size;
    }

    internal partial struct mln_style_tile_source_options
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint fields;

        public double min_zoom;

        public double max_zoom;

        public mln_string_view attribution;

        [NativeTypeName("uint32_t")]
        public uint scheme;

        public mln_lat_lng_bounds bounds;

        [NativeTypeName("uint32_t")]
        public uint tile_size;

        [NativeTypeName("uint32_t")]
        public uint vector_encoding;

        [NativeTypeName("uint32_t")]
        public uint raster_encoding;
    }

    internal unsafe partial struct mln_geojson_source_options
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint fields;

        public double min_zoom;

        public double max_zoom;

        public double tolerance;

        public double cluster_max_zoom;

        [NativeTypeName("const mln_json_value *")]
        public mln_json_value* cluster_properties;

        [NativeTypeName("uint32_t")]
        public uint tile_size;

        [NativeTypeName("uint32_t")]
        public uint buffer;

        [NativeTypeName("uint32_t")]
        public uint cluster_radius;

        [NativeTypeName("uint32_t")]
        public uint cluster_min_points;

        [NativeTypeName("bool")]
        public byte line_metrics;

        [NativeTypeName("bool")]
        public byte cluster;

        [NativeTypeName("bool")]
        public byte synchronous_update;
    }

    internal partial struct mln_canonical_tile_id
    {
        [NativeTypeName("uint32_t")]
        public uint z;

        [NativeTypeName("uint32_t")]
        public uint x;

        [NativeTypeName("uint32_t")]
        public uint y;
    }

    internal unsafe partial struct mln_custom_geometry_source_options
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint fields;

        [NativeTypeName("mln_custom_geometry_source_tile_callback")]
        public delegate* unmanaged[Cdecl]<void*, mln_canonical_tile_id, void> fetch_tile;

        [NativeTypeName("mln_custom_geometry_source_tile_callback")]
        public delegate* unmanaged[Cdecl]<void*, mln_canonical_tile_id, void> cancel_tile;

        public void* user_data;

        public double min_zoom;

        public double max_zoom;

        public double tolerance;

        [NativeTypeName("uint32_t")]
        public uint tile_size;

        [NativeTypeName("uint32_t")]
        public uint buffer;

        [NativeTypeName("bool")]
        public byte clip;

        [NativeTypeName("bool")]
        public byte wrap;
    }

    internal unsafe partial struct mln_premultiplied_rgba8_image
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint width;

        [NativeTypeName("uint32_t")]
        public uint height;

        [NativeTypeName("uint32_t")]
        public uint stride;

        [NativeTypeName("const uint8_t *")]
        public byte* pixels;

        [NativeTypeName("size_t")]
        public nuint byte_length;
    }

    internal unsafe partial struct mln_style_image_options
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint fields;

        [NativeTypeName("const mln_image_stretch *")]
        public mln_image_stretch* stretch_x;

        [NativeTypeName("size_t")]
        public nuint stretch_x_count;

        [NativeTypeName("const mln_image_stretch *")]
        public mln_image_stretch* stretch_y;

        [NativeTypeName("size_t")]
        public nuint stretch_y_count;

        public mln_image_content content;

        [NativeTypeName("uint32_t")]
        public uint text_fit_width;

        [NativeTypeName("uint32_t")]
        public uint text_fit_height;

        public float pixel_ratio;

        [NativeTypeName("bool")]
        public byte sdf;
    }

    internal partial struct mln_style_image_info
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint width;

        [NativeTypeName("uint32_t")]
        public uint height;

        [NativeTypeName("uint32_t")]
        public uint stride;

        [NativeTypeName("size_t")]
        public nuint byte_length;

        [NativeTypeName("size_t")]
        public nuint stretch_x_count;

        [NativeTypeName("size_t")]
        public nuint stretch_y_count;

        public mln_image_content content;

        [NativeTypeName("uint32_t")]
        public uint text_fit_width;

        [NativeTypeName("uint32_t")]
        public uint text_fit_height;

        public float pixel_ratio;

        [NativeTypeName("bool")]
        public byte sdf;

        [NativeTypeName("bool")]
        public byte has_content;

        [NativeTypeName("bool")]
        public byte has_text_fit_width;

        [NativeTypeName("bool")]
        public byte has_text_fit_height;
    }

    internal partial struct mln_style_transition_options
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint fields;

        public double duration_ms;

        public double delay_ms;

        [NativeTypeName("bool")]
        public byte enable_placement_transitions;
    }

    internal static unsafe partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_style_tile_source_options mln_style_tile_source_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_geojson_source_options mln_geojson_source_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_custom_geometry_source_options mln_custom_geometry_source_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_premultiplied_rgba8_image mln_premultiplied_rgba8_image_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_style_image_options mln_style_image_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_style_image_info mln_style_image_info_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_style_transition_options mln_style_transition_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_style_id_list_count([NativeTypeName("mln_style_id_list")] MlnStyleIdList list, [NativeTypeName("size_t *")] nuint* out_count);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_style_id_list_get([NativeTypeName("mln_style_id_list")] MlnStyleIdList list, [NativeTypeName("size_t")] nuint index, mln_string_view* out_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_style_id_list_destroy([NativeTypeName("mln_style_id_list")] MlnStyleIdList list);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_add_style_source_json([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, [NativeTypeName("const mln_json_value *")] mln_json_value* source_json);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_remove_style_source([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, bool* out_removed);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_style_source_exists([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, bool* out_exists);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_style_source_type([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, [NativeTypeName("uint32_t *")] uint* out_source_type, bool* out_found);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_style_source_info([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, mln_style_source_info* out_info, bool* out_found);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_copy_style_source_attribution([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, [NativeTypeName("char *")] sbyte* out_attribution, [NativeTypeName("size_t")] nuint attribution_capacity, [NativeTypeName("size_t *")] nuint* out_attribution_size, bool* out_found);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_list_style_source_ids([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("mln_style_id_list *")] MlnStyleIdList* out_source_ids);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_add_geojson_source_url([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, mln_string_view url, [NativeTypeName("const mln_geojson_source_options *")] mln_geojson_source_options* options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_add_geojson_source_data([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, [NativeTypeName("const mln_geojson *")] mln_geojson* data, [NativeTypeName("const mln_geojson_source_options *")] mln_geojson_source_options* options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_geojson_source_url([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, mln_string_view url);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_geojson_source_data([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, [NativeTypeName("const mln_geojson *")] mln_geojson* data);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_add_vector_source_url([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, mln_string_view url, [NativeTypeName("const mln_style_tile_source_options *")] mln_style_tile_source_options* options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_add_vector_source_tiles([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, [NativeTypeName("const mln_string_view *")] mln_string_view* tiles, [NativeTypeName("size_t")] nuint tile_count, [NativeTypeName("const mln_style_tile_source_options *")] mln_style_tile_source_options* options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_add_raster_source_url([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, mln_string_view url, [NativeTypeName("const mln_style_tile_source_options *")] mln_style_tile_source_options* options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_add_raster_source_tiles([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, [NativeTypeName("const mln_string_view *")] mln_string_view* tiles, [NativeTypeName("size_t")] nuint tile_count, [NativeTypeName("const mln_style_tile_source_options *")] mln_style_tile_source_options* options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_add_raster_dem_source_url([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, mln_string_view url, [NativeTypeName("const mln_style_tile_source_options *")] mln_style_tile_source_options* options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_add_raster_dem_source_tiles([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, [NativeTypeName("const mln_string_view *")] mln_string_view* tiles, [NativeTypeName("size_t")] nuint tile_count, [NativeTypeName("const mln_style_tile_source_options *")] mln_style_tile_source_options* options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_add_custom_geometry_source([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, [NativeTypeName("const mln_custom_geometry_source_options *")] mln_custom_geometry_source_options* options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_custom_geometry_source_tile_data([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, mln_canonical_tile_id tile_id, [NativeTypeName("const mln_geojson *")] mln_geojson* data);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_invalidate_custom_geometry_source_tile([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, mln_canonical_tile_id tile_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_invalidate_custom_geometry_source_region([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, mln_lat_lng_bounds bounds);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_style_image([NativeTypeName("mln_map")] MlnMap map, mln_string_view image_id, [NativeTypeName("const mln_premultiplied_rgba8_image *")] mln_premultiplied_rgba8_image* image, [NativeTypeName("const mln_style_image_options *")] mln_style_image_options* options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_remove_style_image([NativeTypeName("mln_map")] MlnMap map, mln_string_view image_id, bool* out_removed);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_style_image_exists([NativeTypeName("mln_map")] MlnMap map, mln_string_view image_id, bool* out_exists);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_style_image_info([NativeTypeName("mln_map")] MlnMap map, mln_string_view image_id, mln_style_image_info* out_info, bool* out_found);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_copy_style_image_premultiplied_rgba8([NativeTypeName("mln_map")] MlnMap map, mln_string_view image_id, [NativeTypeName("uint8_t *")] byte* out_pixels, [NativeTypeName("size_t")] nuint pixel_capacity, [NativeTypeName("size_t *")] nuint* out_byte_length, bool* out_found);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_copy_style_image_stretches([NativeTypeName("mln_map")] MlnMap map, mln_string_view image_id, mln_image_stretch* out_stretch_x, [NativeTypeName("size_t")] nuint stretch_x_capacity, [NativeTypeName("size_t *")] nuint* out_stretch_x_count, mln_image_stretch* out_stretch_y, [NativeTypeName("size_t")] nuint stretch_y_capacity, [NativeTypeName("size_t *")] nuint* out_stretch_y_count, bool* out_found);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_add_image_source_url([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, [NativeTypeName("const mln_lat_lng *")] mln_lat_lng* coordinates, [NativeTypeName("size_t")] nuint coordinate_count, mln_string_view url);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_add_image_source_image([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, [NativeTypeName("const mln_lat_lng *")] mln_lat_lng* coordinates, [NativeTypeName("size_t")] nuint coordinate_count, [NativeTypeName("const mln_premultiplied_rgba8_image *")] mln_premultiplied_rgba8_image* image);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_image_source_url([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, mln_string_view url);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_image_source_image([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, [NativeTypeName("const mln_premultiplied_rgba8_image *")] mln_premultiplied_rgba8_image* image);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_image_source_coordinates([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, [NativeTypeName("const mln_lat_lng *")] mln_lat_lng* coordinates, [NativeTypeName("size_t")] nuint coordinate_count);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_image_source_coordinates([NativeTypeName("mln_map")] MlnMap map, mln_string_view source_id, mln_lat_lng* out_coordinates, [NativeTypeName("size_t")] nuint coordinate_capacity, [NativeTypeName("size_t *")] nuint* out_coordinate_count, bool* out_found);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_add_hillshade_layer([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, mln_string_view source_id, mln_string_view before_layer_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_add_color_relief_layer([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, mln_string_view source_id, mln_string_view before_layer_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_add_location_indicator_layer([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, mln_string_view before_layer_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_location_indicator_location([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, mln_lat_lng coordinate, double altitude);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_location_indicator_bearing([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, double bearing);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_location_indicator_accuracy_radius([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, double radius);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_location_indicator_image_name([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, [NativeTypeName("uint32_t")] uint image_kind, mln_string_view image_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_add_style_layer_json([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_json_value *")] mln_json_value* layer_json, mln_string_view before_layer_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_remove_style_layer([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, bool* out_removed);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_style_layer_exists([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, bool* out_exists);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_style_layer_type([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, mln_string_view* out_layer_type, bool* out_found);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_list_style_layer_ids([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("mln_style_id_list *")] MlnStyleIdList* out_layer_ids);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_move_style_layer([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, mln_string_view before_layer_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_style_layer_json([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, [NativeTypeName("mln_json_snapshot *")] MlnJsonSnapshot* out_layer, bool* out_found);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_style_light_json([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_json_value *")] mln_json_value* light_json);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_style_light_property([NativeTypeName("mln_map")] MlnMap map, mln_string_view property_name, [NativeTypeName("const mln_json_value *")] mln_json_value* value);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_style_light_property([NativeTypeName("mln_map")] MlnMap map, mln_string_view property_name, [NativeTypeName("mln_json_snapshot *")] MlnJsonSnapshot* out_value);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_style_transition_options([NativeTypeName("mln_map")] MlnMap map, [NativeTypeName("const mln_style_transition_options *")] mln_style_transition_options* options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_style_transition_options([NativeTypeName("mln_map")] MlnMap map, mln_style_transition_options* out_options);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_layer_property([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, mln_string_view property_name, [NativeTypeName("const mln_json_value *")] mln_json_value* value);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_layer_property([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, mln_string_view property_name, [NativeTypeName("mln_json_snapshot *")] MlnJsonSnapshot* out_value);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_layer_filter([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, [NativeTypeName("const mln_json_value *")] mln_json_value* filter);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_layer_filter([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, [NativeTypeName("mln_json_snapshot *")] MlnJsonSnapshot* out_filter);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_layer_source_layer([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, mln_string_view source_layer);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_copy_layer_source_layer([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, [NativeTypeName("char *")] sbyte* out_source_layer, [NativeTypeName("size_t")] nuint source_layer_capacity, [NativeTypeName("size_t *")] nuint* out_source_layer_size);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_layer_source_id([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, mln_string_view source_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_copy_layer_source_id([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, [NativeTypeName("char *")] sbyte* out_source_id, [NativeTypeName("size_t")] nuint source_id_capacity, [NativeTypeName("size_t *")] nuint* out_source_id_size);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_layer_min_zoom([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, double min_zoom);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_layer_min_zoom([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, double* out_min_zoom);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_layer_max_zoom([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, double max_zoom);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_layer_max_zoom([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, double* out_max_zoom);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_set_layer_visibility([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, [NativeTypeName("uint32_t")] uint visibility);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_map_get_layer_visibility([NativeTypeName("mln_map")] MlnMap map, mln_string_view layer_id, [NativeTypeName("uint32_t *")] uint* out_visibility);
    }
}
