using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C
{
    internal static partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_plugin_load_library(mln_buffer_view path, mln_buffer_view entry_point);
    }
}
