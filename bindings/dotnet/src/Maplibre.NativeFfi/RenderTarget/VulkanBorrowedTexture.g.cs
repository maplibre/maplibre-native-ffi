// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A Vulkan image of a borrowed ring.
/// </summary>
/// <remarks>
/// See <c>mln_vulkan_borrowed_texture</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct VulkanBorrowedTexture
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public VulkanBorrowedTexture(ulong Image, ulong ImageView)
    {
        this.Image = Image;
        this.ImageView = ImageView;
    }

    /// <summary>
    /// Your <c>VkImage</c>.
    /// </summary>
    public required ulong Image { get; init; }

    /// <summary>
    /// A 2D <c>VkImageView</c> of <c>image</c> with one mip level and one
    /// layer.
    /// </summary>
    public required ulong ImageView { get; init; }
}
