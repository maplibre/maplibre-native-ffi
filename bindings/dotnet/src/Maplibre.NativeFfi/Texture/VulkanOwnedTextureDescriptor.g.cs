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
