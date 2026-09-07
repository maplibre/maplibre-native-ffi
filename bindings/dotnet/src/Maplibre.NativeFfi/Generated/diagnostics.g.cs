using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C
{
    internal static unsafe partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("const char *")]
        public static extern sbyte* mln_thread_last_error_message();
    }
}
