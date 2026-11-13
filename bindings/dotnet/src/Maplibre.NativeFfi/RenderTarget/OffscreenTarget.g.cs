// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// An image that the session allocates, renders at the map's scale factor, and
/// reads back with <c>RenderSessionHandle.ReadTextureAsync()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_offscreen_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct OffscreenTarget
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public OffscreenTarget(LogicalSize LogicalSize)
    {
        this.LogicalSize = LogicalSize;
    }

    public required LogicalSize LogicalSize { get; init; }
}
