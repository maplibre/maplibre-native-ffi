package org.maplibre.nativeffi.render

/** Mutable Vulkan backend context descriptor. */
public class VulkanContextDescriptor(
  public var instance: NativePointer = NativePointer.NULL,
  public var physicalDevice: NativePointer = NativePointer.NULL,
  public var device: NativePointer = NativePointer.NULL,
  public var graphicsQueue: NativePointer = NativePointer.NULL,
  graphicsQueueFamilyIndex: Int = 0,
) {
  public var graphicsQueueFamilyIndex: Int = graphicsQueueFamilyIndex
    set(value) {
      require(value >= 0) { "graphicsQueueFamilyIndex must be non-negative" }
      field = value
    }

  init {
    require(graphicsQueueFamilyIndex >= 0) { "graphicsQueueFamilyIndex must be non-negative" }
  }

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

  public fun graphicsQueueFamilyIndex(index: Int): VulkanContextDescriptor = apply {
    this.graphicsQueueFamilyIndex = index
  }
}
