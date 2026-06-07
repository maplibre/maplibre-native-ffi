using System.Runtime.InteropServices;

namespace Maplibre.Native.Internal.C
{
  internal partial struct mln_feature_query_result
  {
  }

  internal partial struct mln_feature_extension_result
  {
  }

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

    [NativeTypeName("__AnonymousRecord_query_L47_C3")]
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

    [NativeTypeName("const mln_string_view *")]
    public mln_string_view* layer_ids;

    [NativeTypeName("size_t")]
    public nuint layer_id_count;

    [NativeTypeName("const mln_json_value *")]
    public mln_json_value* filter;
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

    [NativeTypeName("const mln_string_view *")]
    public mln_string_view* source_layer_ids;

    [NativeTypeName("size_t")]
    public nuint source_layer_id_count;

    [NativeTypeName("const mln_json_value *")]
    public mln_json_value* filter;
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_queried_feature_field : uint
  {
    MLN_QUERIED_FEATURE_SOURCE_ID = 1U << 0,
    MLN_QUERIED_FEATURE_SOURCE_LAYER_ID = 1U << 1,
    MLN_QUERIED_FEATURE_STATE = 1U << 2,
  }

  internal unsafe partial struct mln_queried_feature
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint fields;

    public mln_feature feature;

    public mln_string_view source_id;

    public mln_string_view source_layer_id;

    [NativeTypeName("const mln_json_value *")]
    public mln_json_value* state;
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_feature_extension_result_type : uint
  {
    MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE = 1,
    MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION = 2,
  }

  internal partial struct mln_feature_extension_result_info
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint type;

    [NativeTypeName("__AnonymousRecord_query_L119_C3")]
    public _data_e__Union data;

    [StructLayout(LayoutKind.Explicit)]
    internal unsafe partial struct _data_e__Union
    {
      [FieldOffset(0)]
      [NativeTypeName("const mln_json_value *")]
      public mln_json_value* value;

      [FieldOffset(0)]
      public mln_feature_collection feature_collection;
    }
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
    public static extern mln_status mln_render_session_query_rendered_features(mln_render_session* session, [NativeTypeName("const mln_rendered_query_geometry *")] mln_rendered_query_geometry* geometry, [NativeTypeName("const mln_rendered_feature_query_options *")] mln_rendered_feature_query_options* options, mln_feature_query_result** out_result);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_render_session_query_source_features(mln_render_session* session, mln_string_view source_id, [NativeTypeName("const mln_source_feature_query_options *")] mln_source_feature_query_options* options, mln_feature_query_result** out_result);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_render_session_query_feature_extensions(mln_render_session* session, mln_string_view source_id, [NativeTypeName("const mln_feature *")] mln_feature* feature, mln_string_view extension, mln_string_view extension_field, [NativeTypeName("const mln_json_value *")] mln_json_value* arguments, mln_feature_extension_result** out_result);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_feature_query_result_count([NativeTypeName("const mln_feature_query_result *")] mln_feature_query_result* result, [NativeTypeName("size_t *")] nuint* out_count);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_feature_query_result_get([NativeTypeName("const mln_feature_query_result *")] mln_feature_query_result* result, [NativeTypeName("size_t")] nuint index, mln_queried_feature* out_feature);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern void mln_feature_query_result_destroy(mln_feature_query_result* result);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_feature_extension_result_get([NativeTypeName("const mln_feature_extension_result *")] mln_feature_extension_result* result, mln_feature_extension_result_info* out_info);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern void mln_feature_extension_result_destroy(mln_feature_extension_result* result);
  }
}
