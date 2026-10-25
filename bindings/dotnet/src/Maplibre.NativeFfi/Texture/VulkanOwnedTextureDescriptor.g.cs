// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Vulkan attachment options for an owned texture target.
/// </summary>
/// <remarks>
/// See <c>mln_vulkan_owned_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct VulkanOwnedTextureDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public VulkanOwnedTextureDescriptor(LogicalExtent Extent, VulkanContextDescriptor Context)
    {
        this.Extent = Extent;
        this.Context = Context;
    }

    /// <summary>
    /// Logical texture extent. Required. A scale_factor that differs from the
    /// map's is accepted and logged as a warning.
    /// </summary>
    public required LogicalExtent Extent { get; init; }

    /// <summary>
    /// Borrowed Vulkan context. All handles are required.
    /// </summary>
    public required VulkanContextDescriptor Context { get; init; }
}
