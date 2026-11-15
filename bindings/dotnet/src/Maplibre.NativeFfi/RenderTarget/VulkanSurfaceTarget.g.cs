// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Your Vulkan surface to present to.
/// </summary>
/// <remarks>
/// <para>Make it with the instance of the Vulkan device that you attach on. The
/// device's queue family must support presenting to the surface. The session
/// creates the swapchain, and destroys it but not the surface.</para>
/// <para>See <c>mln_vulkan_surface_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct VulkanSurfaceTarget
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public VulkanSurfaceTarget(LogicalSize LogicalSize, ulong Surface)
    {
        this.LogicalSize = LogicalSize;
        this.Surface = Surface;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// Your <c>VkSurfaceKHR</c>.
    /// </summary>
    public required ulong Surface { get; init; }
}
