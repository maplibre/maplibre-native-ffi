// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public static unsafe partial class MaplibreNative
{
    /// <summary>
    /// Returns the process-wide <c>mln_plugin_register_v1</c> entry point;
    /// never null.
    /// </summary>
    /// <remarks>
    /// See <c>mln_plugin_get_register_function_v1</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/plugin_8h.html">C API reference</see>.
    /// </remarks>
    public static NativePointer GetPluginRegisterFunctionV1()
    {
        using var call = Enter(null);
        var returned = NativeMethods.mln_plugin_get_register_function_v1();
        return NativePointer.FromNativeAddress((nint)returned);
    }
}
