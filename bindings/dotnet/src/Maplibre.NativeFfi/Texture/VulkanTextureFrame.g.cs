// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Vulkan frame acquired from a texture ring.
/// </summary>
/// <remarks>
/// See <c>mln_vulkan_texture_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Generation">
/// Session generation that produced this frame.
/// </param>
/// <param name="Width">
/// Physical Vulkan image width in device pixels.
/// </param>
/// <param name="Height">
/// Physical Vulkan image height in device pixels.
/// </param>
/// <param name="ScaleFactor">
/// UI-to-device pixel scale used for this frame.
/// </param>
/// <param name="FrameId">
/// Opaque frame identity used to reject stale releases.
/// </param>
/// <param name="Slot">
/// Ring slot that holds this frame. For a borrowed target, the index of its
/// image in the descriptor's textures array.
/// </param>
/// <param name="Image">
/// Borrowed VkImage bit pattern. Valid until frame release.
/// </param>
/// <param name="ImageView">
/// Borrowed VkImageView bit pattern. Valid until frame release.
/// </param>
/// <param name="Device">
/// Borrowed VkDevice. Valid until frame release.
/// </param>
/// <param name="Format">
/// Backend-native VkFormat value.
/// </param>
/// <param name="Layout">
/// Backend-native VkImageLayout value that the image is in:
/// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for a session-owned ring, and the
/// descriptor's final_layout for a borrowed one.
/// </param>
public readonly partial record struct VulkanTextureFrame(
    ulong Generation,
    uint Width,
    uint Height,
    double ScaleFactor,
    ulong FrameId,
    uint Slot,
    ulong Image,
    ulong ImageView,
    NativePointer Device,
    uint Format,
    uint Layout
);
