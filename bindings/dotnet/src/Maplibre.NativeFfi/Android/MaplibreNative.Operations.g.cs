// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public static unsafe partial class MaplibreNative
{
    /// <summary>
    /// Initializes Android platform services.
    /// </summary>
    /// <remarks>
    /// See <c>mln_android_init</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html">C API reference</see>.
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    public static void AndroidInit(NativePointer jniEnv, NativePointer context)
    {
        using var call = Enter(null);
        Check(
            NativeMethods.mln_android_init(
                (void*)jniEnv.Address,
                (void*)context.Address,
                Diagnostic
            )
        );
    }
}
