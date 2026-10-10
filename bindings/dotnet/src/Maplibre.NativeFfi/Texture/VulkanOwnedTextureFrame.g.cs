// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Vulkan frame acquired from a session-owned texture target.
/// </summary>
/// <remarks>
/// See <c>mln_vulkan_owned_texture_frame</c> in the <see
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
/// Backend-native VkImageLayout value; Vulkan frames are host-sampleable.
/// </param>
public readonly partial record struct VulkanOwnedTextureFrame(
    ulong Generation,
    uint Width,
    uint Height,
    double ScaleFactor,
    ulong FrameId,
    ulong Image,
    ulong ImageView,
    NativePointer Device,
    uint Format,
    uint Layout
);
