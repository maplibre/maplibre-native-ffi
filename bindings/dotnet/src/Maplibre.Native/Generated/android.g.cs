using System.Runtime.InteropServices;

namespace Maplibre.Native.Internal.C
{
    internal static unsafe partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_android_init(void* jni_env, void* jni_class, void* context);
    }
}
