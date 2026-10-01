// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

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
            using var call = Enter(null, "mln_vulkan_surface_descriptor_default");
            return CopyVulkanSurfaceDescriptor(
                NativeMethods.mln_vulkan_surface_descriptor_default()
            );
        }
    }
}
