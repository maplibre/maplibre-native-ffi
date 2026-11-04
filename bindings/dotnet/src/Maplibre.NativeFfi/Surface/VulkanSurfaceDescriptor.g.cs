// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A Vulkan surface to present to, for <c>MapHandle.AttachVulkanSurface()</c>.
/// </summary>
/// <remarks>
/// <para>The device must have VK_KHR_swapchain enabled, and the queue family of
/// <c>graphics_queue_family_index</c> must support presenting to the surface.
/// The session creates the swapchain. It uses the surface until a replacement
/// or release of the target completes, a detach or release of the session
/// completes, or an abandon of it returns.</para>
/// <para>See <c>mln_vulkan_surface_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct VulkanSurfaceDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public VulkanSurfaceDescriptor(
        LogicalSize LogicalSize,
        VulkanContextDescriptor Context,
        ulong Surface
    )
    {
        this.LogicalSize = LogicalSize;
        this.Context = Context;
        this.Surface = Surface;
    }

    public required LogicalSize LogicalSize { get; init; }
    public required VulkanContextDescriptor Context { get; init; }

    /// <summary>
    /// Your VkSurfaceKHR.
    /// </summary>
    public required ulong Surface { get; init; }
}
