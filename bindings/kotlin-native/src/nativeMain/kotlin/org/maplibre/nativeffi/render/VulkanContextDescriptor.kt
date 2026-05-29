package org.maplibre.nativeffi.render

/** Mutable Vulkan backend context descriptor. */
public class VulkanContextDescriptor(
  instance: NativePointer = NativePointer.NULL,
  physicalDevice: NativePointer = NativePointer.NULL,
  device: NativePointer = NativePointer.NULL,
  graphicsQueue: NativePointer = NativePointer.NULL,
  graphicsQueueFamilyIndex: Int = 0,
  getInstanceProcAddr: NativePointer = NativePointer.NULL,
  getDeviceProcAddr: NativePointer = NativePointer.NULL,
) {
  public var instance: NativePointer = instance

  public var physicalDevice: NativePointer = physicalDevice

  public var device: NativePointer = device

  public var graphicsQueue: NativePointer = graphicsQueue

  public var getInstanceProcAddr: NativePointer = getInstanceProcAddr

  public var getDeviceProcAddr: NativePointer = getDeviceProcAddr

  public var graphicsQueueFamilyIndex: Int = graphicsQueueFamilyIndex
    set(value) {
      require(value >= 0) { "graphicsQueueFamilyIndex must be non-negative" }
      field = value
    }

  init {
    require(graphicsQueueFamilyIndex >= 0) { "graphicsQueueFamilyIndex must be non-negative" }
  }
}
