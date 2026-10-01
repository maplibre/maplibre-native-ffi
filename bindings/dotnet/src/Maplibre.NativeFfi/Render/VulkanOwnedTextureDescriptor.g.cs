// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

public readonly partial record struct VulkanOwnedTextureDescriptor(
    RenderTargetExtent Extent,
    VulkanContextDescriptor Context
)
{
    public static VulkanOwnedTextureDescriptor Default
    {
        get
        {
            using var call = Enter(null, "mln_vulkan_owned_texture_descriptor_default");
            return CopyVulkanOwnedTextureDescriptor(
                NativeMethods.mln_vulkan_owned_texture_descriptor_default()
            );
        }
    }
}
