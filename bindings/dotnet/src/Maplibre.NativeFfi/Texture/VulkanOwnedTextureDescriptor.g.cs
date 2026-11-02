// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A ring of Vulkan images that the session creates, for
/// <c>MapHandle.AttachVulkanOwnedTexture()</c>.
/// </summary>
/// <remarks>
/// <para>Each frame leaves its image in the layout that
/// <c>VulkanTextureFrame.Layout</c> reports, owned by the queue family of
/// <c>graphics_queue_family_index</c>.</para>
/// <para>See <c>mln_vulkan_owned_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct VulkanOwnedTextureDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public VulkanOwnedTextureDescriptor(LogicalExtent Extent, VulkanContextDescriptor Context)
    {
        this.Extent = Extent;
        this.Context = Context;
    }

    public required LogicalExtent Extent { get; init; }
    public required VulkanContextDescriptor Context { get; init; }
}
