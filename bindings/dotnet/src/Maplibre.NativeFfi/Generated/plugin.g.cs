using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C
{
    internal static partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("mln_plugin_register_function_v1")]
        public static extern nint mln_plugin_get_register_function_v1();
    }
}
