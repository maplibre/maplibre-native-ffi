// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

public readonly partial record struct VulkanSurfaceDescriptor(
    RenderTargetExtent Extent,
    VulkanContextDescriptor Context,
    ulong Surface
)
{
    public static VulkanSurfaceDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_vulkan_surface_descriptor_default");
            return GeneratedValues.CopyVulkanSurfaceDescriptor(
                NativeMethods.mln_vulkan_surface_descriptor_default()
            );
        }
    }
}
