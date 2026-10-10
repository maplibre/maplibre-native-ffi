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
public readonly partial record struct RenderTargetExtent(
    uint Width,
    uint Height,
    double ScaleFactor
)
{
    public RenderTargetExtent()
        : this(256, 256, 1.0) { }
}
