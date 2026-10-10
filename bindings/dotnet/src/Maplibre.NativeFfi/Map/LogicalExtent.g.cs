// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Logical map extent in UI pixels and device-pixel scale.
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
/// Device pixels per UI pixel. Defaults to 1.0. The renderer takes it at map
/// creation, so <c>mln_map_resize()</c> accepts only the value the map was
/// created with.
/// </param>
public readonly partial record struct LogicalExtent(uint Width, uint Height, double ScaleFactor)
{
    public LogicalExtent()
        : this(256, 256, 1.0) { }
}
