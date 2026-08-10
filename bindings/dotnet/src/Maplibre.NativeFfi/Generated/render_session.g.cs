using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C
{
    [NativeTypeName("uint32_t")]
    internal enum mln_render_result : uint
    {
        MLN_RENDER_RESULT_RENDERED = 0,
        MLN_RENDER_RESULT_NO_UPDATE,
        MLN_RENDER_RESULT_SIZE_PENDING,
        MLN_RENDER_RESULT_TARGET_NOT_READY,
    }

    internal static unsafe partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_resize([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("uint32_t")] uint width, [NativeTypeName("uint32_t")] uint height, double scale_factor);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_render_update([NativeTypeName("mln_render_session")] MlnRenderSession session, mln_render_result* out_result);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_detach([NativeTypeName("mln_render_session")] MlnRenderSession session);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_destroy([NativeTypeName("mln_render_session")] MlnRenderSession session);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_reduce_memory_use([NativeTypeName("mln_render_session")] MlnRenderSession session);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_clear_data([NativeTypeName("mln_render_session")] MlnRenderSession session);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_dump_debug_logs([NativeTypeName("mln_render_session")] MlnRenderSession session);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_set_feature_state([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("const mln_feature_state_selector *")] mln_feature_state_selector* selector, [NativeTypeName("const mln_json_value *")] mln_json_value* state);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_get_feature_state([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("const mln_feature_state_selector *")] mln_feature_state_selector* selector, [NativeTypeName("mln_json_snapshot *")] MlnJsonSnapshot* out_state);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_remove_feature_state([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("const mln_feature_state_selector *")] mln_feature_state_selector* selector);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_json_snapshot_get([NativeTypeName("mln_json_snapshot")] MlnJsonSnapshot snapshot, [NativeTypeName("const mln_json_value **")] mln_json_value** out_value);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_json_snapshot_destroy([NativeTypeName("mln_json_snapshot")] MlnJsonSnapshot snapshot);
    }
}
