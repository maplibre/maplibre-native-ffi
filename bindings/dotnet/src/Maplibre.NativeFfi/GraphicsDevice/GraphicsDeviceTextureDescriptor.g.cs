// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Attachment options for a graphics device texture target.
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

    /// <summary>
    /// Logical texture extent. Required. A scale_factor that differs from the
    /// map's is accepted and logged as a warning.
    /// </summary>
    public required LogicalExtent Extent { get; init; }
}
