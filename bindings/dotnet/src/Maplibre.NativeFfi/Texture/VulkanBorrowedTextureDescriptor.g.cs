// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Vulkan attachment options for a borrowed texture target.
/// </summary>
/// <remarks>
/// See <c>mln_vulkan_borrowed_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
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
    public VulkanBorrowedTextureDescriptor()
        : this(new RenderTargetExtent(), 256, 256, default, default, default, default, default, 5)
    { }

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
