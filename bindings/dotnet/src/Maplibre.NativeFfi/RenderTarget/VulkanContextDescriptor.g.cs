// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public readonly partial record struct VulkanContextDescriptor(
    NativePointer Instance,
    NativePointer PhysicalDevice,
    NativePointer Device,
    NativePointer GraphicsQueue,
    uint GraphicsQueueFamilyIndex,
    NativePointer GetInstanceProcAddr,
    NativePointer GetDeviceProcAddr
);
