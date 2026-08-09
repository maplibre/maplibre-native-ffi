using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C
{
    internal static unsafe partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_resize([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("uint32_t")] uint width, [NativeTypeName("uint32_t")] uint height, double scale_factor);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_render_update([NativeTypeName("mln_render_session")] MlnRenderSession session, bool* out_rendered);

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
        public static extern mln_status mln_render_session_set_feature_state([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("const mln_feature_state_selector *")] mln_feature_state_selector* selector, mln_buffer_view state);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_get_feature_state([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("const mln_feature_state_selector *")] mln_feature_state_selector* selector, [NativeTypeName("mln_buffer *")] ulong* out_state);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_remove_feature_state([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("const mln_feature_state_selector *")] mln_feature_state_selector* selector);
    }
}
