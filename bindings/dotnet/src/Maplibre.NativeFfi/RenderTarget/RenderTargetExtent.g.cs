// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public readonly partial record struct RenderTargetExtent(
    uint Width,
    uint Height,
    double ScaleFactor
)
{
    public RenderTargetExtent()
        : this(256, 256, 1.0) { }
}
