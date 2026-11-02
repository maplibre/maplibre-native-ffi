// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// The queue family that a borrowed image goes to after each frame.
/// </summary>
/// <remarks>
/// <para>With <c>VulkanImageOwner.External</c> or
/// <c>VulkanImageOwner.Foreign</c>, acquire the image on your side before you
/// read it. You don't need to release it back.</para>
/// <para>See <c>mln_vulkan_image_owner</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
/// </remarks>
public enum VulkanImageOwner : uint
{
    /// <summary>
    /// The image stays with the queue family of
    /// <c>graphics_queue_family_index</c>.
    /// </summary>
    GraphicsQueue = 0,

    /// <summary>
    /// Each frame releases the image to VK_QUEUE_FAMILY_EXTERNAL.
    /// </summary>
    External = 1,

    /// <summary>
    /// Each frame releases the image to VK_QUEUE_FAMILY_FOREIGN_EXT.
    /// </summary>
    Foreign = 2,
}
