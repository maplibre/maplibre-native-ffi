// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Logical extent in UI pixels and the device-pixel scale.
/// </summary>
/// <remarks>
/// See <c>mln_logical_extent</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Width">
/// Width in UI pixels. Defaults to 256.
/// </param>
/// <param name="Height">
/// Height in UI pixels. Defaults to 256.
/// </param>
/// <param name="ScaleFactor">
/// Device pixels per UI pixel. Defaults to 1.0.
/// </param>
public readonly partial record struct LogicalExtent(uint Width, uint Height, double ScaleFactor)
{
    public LogicalExtent()
        : this(default!, default!, default!)
    {
        this = Default;
    }

    public static LogicalExtent Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyLogicalExtent(NativeMethods.mln_logical_extent_default());
        }
    }
}
