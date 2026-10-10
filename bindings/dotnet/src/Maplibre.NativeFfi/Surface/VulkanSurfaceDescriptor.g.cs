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
