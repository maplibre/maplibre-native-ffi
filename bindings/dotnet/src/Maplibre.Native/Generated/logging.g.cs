using System.Runtime.InteropServices;
using static Maplibre.Native.Internal.C.mln_log_severity;

namespace Maplibre.Native.Internal.C
{
  [NativeTypeName("uint32_t")]
  internal enum mln_log_severity : uint
  {
    MLN_LOG_SEVERITY_INFO = 1,
    MLN_LOG_SEVERITY_WARNING = 2,
    MLN_LOG_SEVERITY_ERROR = 3,
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_log_severity_mask : uint
  {
    MLN_LOG_SEVERITY_MASK_INFO = 1U << (int)(MLN_LOG_SEVERITY_INFO),
    MLN_LOG_SEVERITY_MASK_WARNING = 1U << (int)(MLN_LOG_SEVERITY_WARNING),
    MLN_LOG_SEVERITY_MASK_ERROR = 1U << (int)(MLN_LOG_SEVERITY_ERROR),
    MLN_LOG_SEVERITY_MASK_DEFAULT = MLN_LOG_SEVERITY_MASK_INFO | MLN_LOG_SEVERITY_MASK_WARNING,
    MLN_LOG_SEVERITY_MASK_ALL = MLN_LOG_SEVERITY_MASK_INFO | MLN_LOG_SEVERITY_MASK_WARNING | MLN_LOG_SEVERITY_MASK_ERROR,
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_log_event : uint
  {
    MLN_LOG_EVENT_GENERAL = 0,
    MLN_LOG_EVENT_SETUP = 1,
    MLN_LOG_EVENT_SHADER = 2,
    MLN_LOG_EVENT_PARSE_STYLE = 3,
    MLN_LOG_EVENT_PARSE_TILE = 4,
    MLN_LOG_EVENT_RENDER = 5,
    MLN_LOG_EVENT_STYLE = 6,
    MLN_LOG_EVENT_DATABASE = 7,
    MLN_LOG_EVENT_HTTP_REQUEST = 8,
    MLN_LOG_EVENT_SPRITE = 9,
    MLN_LOG_EVENT_IMAGE = 10,
    MLN_LOG_EVENT_OPENGL = 11,
    MLN_LOG_EVENT_JNI = 12,
    MLN_LOG_EVENT_ANDROID = 13,
    MLN_LOG_EVENT_CRASH = 14,
    MLN_LOG_EVENT_GLYPH = 15,
    MLN_LOG_EVENT_TIMING = 16,
  }

  internal static unsafe partial class NativeMethods
  {
    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_log_set_callback([NativeTypeName("mln_log_callback")] delegate* unmanaged[Cdecl]<void*, uint, uint, long, sbyte*, uint> callback, void* user_data);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_log_clear_callback();

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_log_set_async_severity_mask([NativeTypeName("uint32_t")] uint mask);
  }
}
