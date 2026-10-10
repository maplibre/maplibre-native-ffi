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
public readonly partial record struct VulkanContextDescriptor(
    NativePointer Instance,
    NativePointer PhysicalDevice,
    NativePointer Device,
    NativePointer GraphicsQueue,
    uint GraphicsQueueFamilyIndex,
    NativePointer GetInstanceProcAddr,
    NativePointer GetDeviceProcAddr
);
