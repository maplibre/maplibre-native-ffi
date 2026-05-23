package org.maplibre.nativeffi.render

/** Mutable Vulkan backend context descriptor. */
public class VulkanContextDescriptor(
  public var instance: NativePointer = NativePointer.NULL,
  public var physicalDevice: NativePointer = NativePointer.NULL,
  public var device: NativePointer = NativePointer.NULL,
  public var graphicsQueue: NativePointer = NativePointer.NULL,
  public var graphicsQueueFamilyIndex: UInt = 0U,
) {
  public fun instance(instance: NativePointer): VulkanContextDescriptor = apply {
    this.instance = instance
  }

  public fun physicalDevice(physicalDevice: NativePointer): VulkanContextDescriptor = apply {
    this.physicalDevice = physicalDevice
  }

  public fun device(device: NativePointer): VulkanContextDescriptor = apply { this.device = device }

  public fun graphicsQueue(graphicsQueue: NativePointer): VulkanContextDescriptor = apply {
    this.graphicsQueue = graphicsQueue
  }

  public fun graphicsQueueFamilyIndex(index: UInt): VulkanContextDescriptor = apply {
    this.graphicsQueueFamilyIndex = index
  }
}
