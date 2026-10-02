// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

public readonly partial record struct VulkanBorrowedTextureDescriptor(
    RenderTargetExtent Extent,
    uint PhysicalWidth,
    uint PhysicalHeight,
    VulkanContextDescriptor Context,
    ulong Image,
    ulong ImageView,
    uint Format,
    uint InitialLayout,
    uint FinalLayout
)
{
    public static VulkanBorrowedTextureDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(
                null,
                "mln_vulkan_borrowed_texture_descriptor_default"
            );
            return GeneratedValues.CopyVulkanBorrowedTextureDescriptor(
                NativeMethods.mln_vulkan_borrowed_texture_descriptor_default()
            );
        }
    }
}
