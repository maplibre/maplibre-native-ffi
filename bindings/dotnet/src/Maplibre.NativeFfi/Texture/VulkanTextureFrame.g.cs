// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The Vulkan image of an acquired frame.
/// </summary>
/// <remarks>
/// See <c>mln_vulkan_texture_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Slot">
/// The image's index in the ring. For a borrowed ring, it indexes the
/// descriptor's <c>textures</c>.
/// </param>
/// <param name="Image">
/// The VkImage.
/// </param>
/// <param name="ImageView">
/// The VkImageView.
/// </param>
/// <param name="Device">
/// The image's VkDevice.
/// </param>
/// <param name="Format">
/// The image's VkFormat.
/// </param>
/// <param name="Layout">
/// The VkImageLayout that the frame left the image in.
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
