// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Vulkan attachment options for a native surface.
/// </summary>
/// <remarks>
/// See <c>mln_vulkan_surface_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Extent">
/// Logical surface extent.
/// </param>
/// <param name="Context">
/// Borrowed Vulkan context. All handles are required. The device must support
/// VK_KHR_swapchain, and the queue family must support graphics and
/// presentation to this descriptor's surface.
/// </param>
/// <param name="Surface">
/// Borrowed VkSurfaceKHR bit pattern. Required.
/// </param>
public readonly partial record struct VulkanSurfaceDescriptor(
    RenderTargetExtent Extent,
    VulkanContextDescriptor Context,
    ulong Surface
)
{
    public VulkanSurfaceDescriptor()
        : this(new RenderTargetExtent(), default, default) { }

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
