// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The image that <c>MapHandle.AttachOffscreen()</c> renders.
/// </summary>
/// <remarks>
/// See <c>mln_offscreen_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct OffscreenDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public OffscreenDescriptor(LogicalExtent Extent)
    {
        this.Extent = Extent;
    }

    public required LogicalExtent Extent { get; init; }
}
