// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A new Vulkan surface for
/// <c>RenderSessionHandle.SetVulkanSurfaceTargetAsync()</c>.
/// </summary>
/// <remarks>
/// <para>The session presents to it with the context that it attached with, so
/// the queue family of that context's <c>graphics_queue_family_index</c> must
/// support presenting to the surface. It uses the surface until a replacement
/// or release of the target completes, a detach or release of the session
/// completes, or an abandon of it returns.</para>
/// <para>See <c>mln_vulkan_surface_replacement</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct VulkanSurfaceReplacement
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public VulkanSurfaceReplacement(LogicalSize LogicalSize, ulong Surface)
    {
        this.LogicalSize = LogicalSize;
        this.Surface = Surface;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// Your VkSurfaceKHR.
    /// </summary>
    public required ulong Surface { get; init; }
}
