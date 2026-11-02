// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The texture that <c>MapHandle.AttachGraphicsDeviceTexture()</c> draws into.
/// </summary>
/// <remarks>
/// See <c>mln_graphics_device_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct GraphicsDeviceTextureDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public GraphicsDeviceTextureDescriptor(LogicalExtent Extent)
    {
        this.Extent = Extent;
    }

    public required LogicalExtent Extent { get; init; }
}
