using System.Runtime.InteropServices;

namespace Maplibre.Native.Internal.C
{
  internal static unsafe partial class NativeMethods
  {
    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_render_session_resize(mln_render_session* session, [NativeTypeName("uint32_t")] uint width, [NativeTypeName("uint32_t")] uint height, double scale_factor);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_render_session_render_update(mln_render_session* session);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_render_session_detach(mln_render_session* session);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_render_session_destroy(mln_render_session* session);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_render_session_reduce_memory_use(mln_render_session* session);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_render_session_clear_data(mln_render_session* session);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_render_session_dump_debug_logs(mln_render_session* session);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_render_session_set_feature_state(mln_render_session* session, [NativeTypeName("const mln_feature_state_selector *")] mln_feature_state_selector* selector, [NativeTypeName("const mln_json_value *")] mln_json_value* state);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_render_session_get_feature_state(mln_render_session* session, [NativeTypeName("const mln_feature_state_selector *")] mln_feature_state_selector* selector, mln_json_snapshot** out_state);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_render_session_remove_feature_state(mln_render_session* session, [NativeTypeName("const mln_feature_state_selector *")] mln_feature_state_selector* selector);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_json_snapshot_get([NativeTypeName("const mln_json_snapshot *")] mln_json_snapshot* snapshot, [NativeTypeName("const mln_json_value **")] mln_json_value** out_value);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern void mln_json_snapshot_destroy(mln_json_snapshot* snapshot);
  }
}
