// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public static unsafe partial class Maplibre
{
    /// <summary>
    /// Reports the C ABI contract version. The value is 0 while the ABI is
    /// unstable, and will increment on each SemVer major release.
    /// </summary>
    /// <remarks>
    /// See <c>mln_c_version</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
    /// </remarks>
    public static uint CVersion()
    {
        using var call = Enter(null);
        var returned = NativeMethods.mln_c_version();
        return returned;
    }

    /// <summary>
    /// Reports the render backends available in this native library build.
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
