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
/// <param name="Extent">
/// Logical texture extent. The map viewport uses width and height and the
/// renderer uses scale_factor; the physical size is stated separately below.
/// </param>
/// <param name="PhysicalWidth">
/// Physical image width in device pixels. Must be positive. Defaults to 256.
/// </param>
/// <param name="PhysicalHeight">
/// Physical image height in device pixels. Must be positive. Defaults to 256.
/// </param>
/// <param name="Context">
/// Borrowed Vulkan context. All handles are required.
/// </param>
/// <param name="Image">
/// Borrowed VkImage. Required.
/// </param>
/// <param name="ImageView">
/// Borrowed VkImageView for image. Required.
/// </param>
/// <param name="Format">
/// Backend-native VkFormat value for image. VK_FORMAT_UNDEFINED is invalid.
/// </param>
/// <param name="InitialLayout">
/// Backend-native VkImageLayout value expected at render-pass begin.
/// </param>
/// <param name="FinalLayout">
/// Backend-native VkImageLayout value left after rendering succeeds. Defaults
/// to 5, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
/// </param>
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
