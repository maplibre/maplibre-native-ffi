// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public static unsafe partial class MaplibreNative
{
    /// <summary>
    /// Computes the physical device-pixel size of a logical extent.
    /// </summary>
    /// <remarks>
    /// See <c>mln_logical_extent_physical_size</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
    /// </remarks>
    public static (uint Width, uint Height) LogicalExtentPhysicalSize(LogicalExtent extent)
    {
        using var call = Enter(null);
        uint outWidth = default;
        uint outHeight = default;
        Check(
            NativeMethods.mln_logical_extent_physical_size(
                NativeLogicalExtent(extent),
                &outWidth,
                &outHeight,
                Diagnostic
            )
        );
        return (outWidth, outHeight);
    }

    /// <summary>
    /// Returns OpenGL context providers supported by this build.
    /// </summary>
    /// <remarks>
    /// See <c>mln_open_gl_supported_context_provider_mask</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
    /// </remarks>
    public static OpenGlContextProviderFlag OpenGlSupportedContextProviderMask()
    {
        using var call = Enter(null);
        var returned = NativeMethods.mln_open_gl_supported_context_provider_mask();
        return (OpenGlContextProviderFlag)returned;
    }
}
