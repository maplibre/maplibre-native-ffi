// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public readonly partial record struct VulkanOwnedTextureDescriptor(
    RenderTargetExtent Extent,
    VulkanContextDescriptor Context
)
{
    public VulkanOwnedTextureDescriptor()
        : this(new RenderTargetExtent(), default) { }

    public static VulkanOwnedTextureDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_vulkan_owned_texture_descriptor_default");
            return GeneratedValues.CopyVulkanOwnedTextureDescriptor(
                NativeMethods.mln_vulkan_owned_texture_descriptor_default()
            );
        }
    }
}
