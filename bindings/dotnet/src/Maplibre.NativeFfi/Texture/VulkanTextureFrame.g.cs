// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The Vulkan image of an acquired frame.
/// </summary>
/// <remarks>
/// <para>The depth members are set when the frame has a depth image, as for
/// <c>MetalTextureFrame</c>. Otherwise they are zero.</para>
/// <para>See <c>mln_vulkan_texture_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="Slot">
/// The image's index in the descriptor's <c>textures</c>.
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
/// <param name="DepthImage">
/// The VkImage that holds the frame's depth and stencil.
/// </param>
/// <param name="DepthImageView">
/// A VkImageView of the depth image's depth and stencil aspects.
/// </param>
/// <param name="DepthFormat">
/// The depth image's VkFormat.
/// </param>
/// <param name="DepthLayout">
/// The VkImageLayout that the frame left the depth image in.
/// </param>
public readonly partial record struct VulkanTextureFrame(
    uint Width,
    uint Height,
    double ScaleFactor,
    uint Slot,
    ulong Image,
    ulong ImageView,
    NativePointer Device,
    uint Format,
    uint Layout,
    ulong DepthImage,
    ulong DepthImageView,
    uint DepthFormat,
    uint DepthLayout
);
