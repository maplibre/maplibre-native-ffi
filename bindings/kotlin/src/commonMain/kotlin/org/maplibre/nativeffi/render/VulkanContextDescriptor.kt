package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.status.Status

/** Mutable Vulkan backend context descriptor. */
public class VulkanContextDescriptor(
  instance: NativePointer,
  physicalDevice: NativePointer,
  device: NativePointer,
  graphicsQueue: NativePointer,
  graphicsQueueFamilyIndex: Int,
  getInstanceProcAddr: NativePointer,
  getDeviceProcAddr: NativePointer,
) {
  public var instance: NativePointer = instance

  public var physicalDevice: NativePointer = physicalDevice

  public var device: NativePointer = device

  public var graphicsQueue: NativePointer = graphicsQueue

  public var getInstanceProcAddr: NativePointer = getInstanceProcAddr

  public var getDeviceProcAddr: NativePointer = getDeviceProcAddr

  public var graphicsQueueFamilyIndex: Int = graphicsQueueFamilyIndex
    set(value) {
      Status.requireArgument(value >= 0) { "graphicsQueueFamilyIndex must be non-negative" }
      field = value
    }

  init {
    Status.requireArgument(graphicsQueueFamilyIndex >= 0) {
      "graphicsQueueFamilyIndex must be non-negative"
    }
  }
}
