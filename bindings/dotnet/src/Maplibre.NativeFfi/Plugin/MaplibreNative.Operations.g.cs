// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public static unsafe partial class MaplibreNative
{
    /// <summary>
    /// Returns the function that registers a plugin's style layer types.
    /// </summary>
    /// <remarks>
    /// <para>Pass the function to the plugin's registration entry point before
    /// loading a style that uses the plugin's layer types. Plugin callbacks run
    /// on library threads and can keep running while the process exits, so they
    /// must not use state that the process destroys at exit.</para>
    /// <para>See <c>mln_plugin_get_register_function_v1</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/plugin_8h.html">C API reference</see>.</para>
    /// </remarks>
    public static NativePointer GetPluginRegisterFunctionV1()
    {
        using var call = Enter(null);
        var returned = NativeMethods.mln_plugin_get_register_function_v1();
        return NativePointer.FromNativeAddress((nint)returned);
    }
}
