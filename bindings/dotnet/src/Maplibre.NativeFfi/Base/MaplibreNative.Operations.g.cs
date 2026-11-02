// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public static unsafe partial class MaplibreNative
{
    /// <summary>
    /// Returns the API epoch of the library.
    /// </summary>
    /// <remarks>
    /// <para>Headers and a library with different epochs aren't
    /// compatible.</para>
    /// <para>See <c>mln_c_version</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.</para>
    /// </remarks>
    public static uint CVersion()
    {
        EnterUnchecked();
        var returned = NativeMethods.mln_c_version();
        return returned;
    }

    /// <summary>
    /// Returns the render backends that this build of the library draws with.
    /// </summary>
    /// <remarks>
    /// See <c>mln_supported_render_backend_mask</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
    /// </remarks>
    public static RenderBackendFlag SupportedRenderBackendMask()
    {
        using var call = Enter(null);
        var returned = NativeMethods.mln_supported_render_backend_mask();
        return (RenderBackendFlag)returned;
    }
}
