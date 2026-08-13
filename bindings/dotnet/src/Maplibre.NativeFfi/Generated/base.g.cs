using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C
{
    [NativeTypeName("int32_t")]
    internal enum mln_status
    {
        MLN_STATUS_OK = 0,
        MLN_STATUS_INVALID_ARGUMENT = -1,
        MLN_STATUS_INVALID_STATE = -2,
        MLN_STATUS_WRONG_THREAD = -3,
        MLN_STATUS_UNSUPPORTED = -4,
        MLN_STATUS_NATIVE_ERROR = -5,
        MLN_STATUS_CANCELLED = -6,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_render_backend_flag : uint
    {
        MLN_RENDER_BACKEND_FLAG_METAL = 1U << 0,
        MLN_RENDER_BACKEND_FLAG_VULKAN = 1U << 1,
        MLN_RENDER_BACKEND_FLAG_OPENGL = 1U << 2,
        MLN_RENDER_BACKEND_FLAG_WEBGPU = 1U << 3,
    }

    internal unsafe partial struct mln_buffer_view
    {
        [NativeTypeName("const void *")]
        public void* data;

        [NativeTypeName("size_t")]
        public nuint size;
    }

    internal static unsafe partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_buffer_get([NativeTypeName("mln_buffer")] MlnBuffer buffer, mln_buffer_view* out_view);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_buffer_destroy([NativeTypeName("mln_buffer")] MlnBuffer buffer);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint32_t")]
        public static extern uint mln_c_version();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint32_t")]
        public static extern uint mln_supported_render_backend_mask();
    }
}
