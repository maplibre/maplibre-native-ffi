// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Logical render target extent in UI pixels.
/// </summary>
/// <remarks>
/// See <c>mln_render_target_extent</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Width">
/// Logical map width in UI pixels. Defaults to 256.
/// </param>
/// <param name="Height">
/// Logical map height in UI pixels. Defaults to 256.
/// </param>
/// <param name="ScaleFactor">
/// UI-to-device pixel scale. Must be positive and finite. Defaults to 1.0.
/// </param>
public readonly partial record struct RenderTargetExtent(
    uint Width,
    uint Height,
    double ScaleFactor
)
{
    public RenderTargetExtent()
        : this(256, 256, 1.0) { }
}
