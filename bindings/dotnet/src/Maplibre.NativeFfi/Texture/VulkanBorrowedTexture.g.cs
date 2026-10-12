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
/// <param name="Image">
/// Borrowed VkImage. Required.
/// </param>
/// <param name="ImageView">
/// Borrowed VkImageView for image. Required. The view must be a 2D color view
/// that matches image and the descriptor's format.
/// </param>
public readonly partial record struct VulkanBorrowedTexture(ulong Image, ulong ImageView);
