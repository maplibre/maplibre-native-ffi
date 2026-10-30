// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// The queue family that holds a borrowed Vulkan image between frames.
/// </summary>
/// <remarks>
/// See <c>mln_vulkan_image_owner</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
public enum VulkanImageOwner : uint
{
    /// <summary>
    /// The context's graphics queue family keeps the image.
    /// </summary>
    GraphicsQueue = 0,

    /// <summary>
    /// VK_QUEUE_FAMILY_EXTERNAL: another Vulkan instance or API that shares the
    /// image's memory.
    /// </summary>
    External = 1,

    /// <summary>
    /// VK_QUEUE_FAMILY_FOREIGN_EXT, such as a compositor. The device must
    /// enable VK_EXT_queue_family_foreign.
    /// </summary>
    Foreign = 2,
}
