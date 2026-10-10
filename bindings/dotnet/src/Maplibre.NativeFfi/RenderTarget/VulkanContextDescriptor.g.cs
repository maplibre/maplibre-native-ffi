// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Vulkan backend context fields shared by Vulkan render targets.
/// </summary>
/// <remarks>
/// See <c>mln_vulkan_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Instance">
/// Borrowed VkInstance. Required.
/// </param>
/// <param name="PhysicalDevice">
/// Borrowed VkPhysicalDevice. Required.
/// </param>
/// <param name="Device">
/// Borrowed VkDevice. Required.
/// </param>
/// <param name="GraphicsQueue">
/// Borrowed graphics VkQueue. Required.
/// </param>
/// <param name="GraphicsQueueFamilyIndex">
/// Queue family index for graphics_queue. Must support graphics commands.
/// </param>
/// <param name="GetInstanceProcAddr">
/// PFN_vkGetInstanceProcAddr for the loader that created the Vulkan handles.
/// </param>
/// <param name="GetDeviceProcAddr">
/// PFN_vkGetDeviceProcAddr for the loader that created the Vulkan device.
/// </param>
public readonly partial record struct VulkanContextDescriptor(
    NativePointer Instance,
    NativePointer PhysicalDevice,
    NativePointer Device,
    NativePointer GraphicsQueue,
    uint GraphicsQueueFamilyIndex,
    NativePointer GetInstanceProcAddr,
    NativePointer GetDeviceProcAddr
);
