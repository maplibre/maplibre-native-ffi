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
public readonly partial record struct VulkanSurfaceDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public VulkanSurfaceDescriptor(
        LogicalExtent Extent,
        VulkanContextDescriptor Context,
        ulong Surface
    )
    {
        this.Extent = Extent;
        this.Context = Context;
        this.Surface = Surface;
    }

    /// <summary>
    /// Logical surface extent. Required. A scale_factor that differs from the
    /// map's is accepted and logged as a warning.
    /// </summary>
    public required LogicalExtent Extent { get; init; }

    /// <summary>
    /// Borrowed Vulkan context. All handles are required. The device must
    /// support VK_KHR_swapchain, and the queue family must support graphics and
    /// presentation to this descriptor's surface.
    /// </summary>
    public required VulkanContextDescriptor Context { get; init; }

    /// <summary>
    /// Borrowed VkSurfaceKHR bit pattern. Required.
    /// </summary>
    public required ulong Surface { get; init; }
}
