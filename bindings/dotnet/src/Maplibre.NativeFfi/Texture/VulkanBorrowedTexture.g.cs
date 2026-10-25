// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One caller-owned Vulkan image of a borrowed texture ring.
/// </summary>
/// <remarks>
/// See <c>mln_vulkan_borrowed_texture</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
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
    /// Borrowed VkImage. Required.
    /// </summary>
    public required ulong Image { get; init; }

    /// <summary>
    /// Borrowed VkImageView for image. Required. The view must be a 2D color
    /// view that matches image and the descriptor's format.
    /// </summary>
    public required ulong ImageView { get; init; }
}
