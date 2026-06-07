using System.Runtime.InteropServices;

namespace Maplibre.Native.Internal.C
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
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_render_backend_flag : uint
  {
    MLN_RENDER_BACKEND_FLAG_METAL = 1U << 0,
    MLN_RENDER_BACKEND_FLAG_VULKAN = 1U << 1,
    MLN_RENDER_BACKEND_FLAG_OPENGL = 1U << 2,
  }

  internal partial struct mln_runtime
  {
  }

  internal partial struct mln_map
  {
  }

  internal partial struct mln_map_projection
  {
  }

  internal partial struct mln_offline_region_snapshot
  {
  }

  internal partial struct mln_offline_region_list
  {
  }

  internal partial struct mln_json_snapshot
  {
  }

  internal partial struct mln_resource_request_handle
  {
  }

  internal partial struct mln_render_session
  {
  }

  internal static partial class NativeMethods
  {
    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("uint32_t")]
    public static extern uint mln_c_version();

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("uint32_t")]
    public static extern uint mln_supported_render_backend_mask();
  }
}
