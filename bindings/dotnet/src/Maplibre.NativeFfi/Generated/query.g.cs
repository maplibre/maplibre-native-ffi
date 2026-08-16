using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C
{
    [NativeTypeName("uint32_t")]
    internal enum mln_rendered_query_geometry_type : uint
    {
        MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT = 1,
        MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX = 2,
        MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING = 3,
    }

    internal partial struct mln_screen_box
    {
        public mln_screen_point min;

        public mln_screen_point max;
    }

    internal unsafe partial struct mln_screen_line_string
    {
        [NativeTypeName("const mln_screen_point *")]
        public mln_screen_point* points;

        [NativeTypeName("size_t")]
        public nuint point_count;
    }

    internal partial struct mln_rendered_query_geometry
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint type;

        [NativeTypeName("__AnonymousRecord_query_L51_C3")]
        public _data_e__Union data;

        [StructLayout(LayoutKind.Explicit)]
        internal partial struct _data_e__Union
        {
            [FieldOffset(0)]
            public mln_screen_point point;

            [FieldOffset(0)]
            public mln_screen_box box;

            [FieldOffset(0)]
            public mln_screen_line_string line_string;
        }
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_rendered_feature_query_option_field : uint
    {
        MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS = 1U << 0,
    }

    internal unsafe partial struct mln_rendered_feature_query_options
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint fields;

        [NativeTypeName("const mln_buffer_view *")]
        public mln_buffer_view* layer_ids;

        [NativeTypeName("size_t")]
        public nuint layer_id_count;

        [NativeTypeName("const mln_buffer_view *")]
        public mln_buffer_view* filter;
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_source_feature_query_option_field : uint
    {
        MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS = 1U << 0,
    }

    internal unsafe partial struct mln_source_feature_query_options
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint fields;

        [NativeTypeName("const mln_buffer_view *")]
        public mln_buffer_view* source_layer_ids;

        [NativeTypeName("size_t")]
        public nuint source_layer_id_count;

        [NativeTypeName("const mln_buffer_view *")]
        public mln_buffer_view* filter;
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_queried_feature_field : uint
    {
        MLN_QUERIED_FEATURE_SOURCE_ID = 1U << 0,
        MLN_QUERIED_FEATURE_SOURCE_LAYER_ID = 1U << 1,
        MLN_QUERIED_FEATURE_STATE = 1U << 2,
    }

    internal partial struct mln_queried_feature
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint fields;

        public mln_buffer_view feature;

        public mln_buffer_view source_id;

        public mln_buffer_view source_layer_id;

        public mln_buffer_view state;
    }

    internal static unsafe partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_rendered_feature_query_options mln_rendered_feature_query_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_source_feature_query_options mln_source_feature_query_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_rendered_query_geometry mln_rendered_query_geometry_point(mln_screen_point point);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_rendered_query_geometry mln_rendered_query_geometry_box(mln_screen_box box);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_rendered_query_geometry mln_rendered_query_geometry_line_string([NativeTypeName("const mln_screen_point *")] mln_screen_point* points, [NativeTypeName("size_t")] nuint point_count);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_queried_feature mln_queried_feature_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_queried_feature_list_count([NativeTypeName("mln_queried_feature_list")] MlnQueriedFeatureList list, [NativeTypeName("size_t *")] nuint* out_count);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_queried_feature_list_get([NativeTypeName("mln_queried_feature_list")] MlnQueriedFeatureList list, [NativeTypeName("size_t")] nuint index, mln_queried_feature* out_feature);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_queried_feature_list_destroy([NativeTypeName("mln_queried_feature_list")] MlnQueriedFeatureList list);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_query_rendered_features_start([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("const mln_rendered_query_geometry *")] mln_rendered_query_geometry* geometry, [NativeTypeName("const mln_rendered_feature_query_options *")] mln_rendered_feature_query_options* options, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_query_source_features_start([NativeTypeName("mln_render_session")] MlnRenderSession session, mln_buffer_view source_id, [NativeTypeName("const mln_source_feature_query_options *")] mln_source_feature_query_options* options, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_query_feature_extensions_start([NativeTypeName("mln_render_session")] MlnRenderSession session, mln_buffer_view source_id, mln_buffer_view feature, mln_buffer_view extension, mln_buffer_view extension_field, [NativeTypeName("const mln_buffer_view *")] mln_buffer_view* arguments, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_query_features_take_result([NativeTypeName("mln_operation")] MlnOperation operation, [NativeTypeName("mln_queried_feature_list *")] MlnQueriedFeatureList* out_result);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_query_take_result([NativeTypeName("mln_operation")] MlnOperation operation, [NativeTypeName("mln_buffer *")] MlnBuffer* out_result);
    }
}
