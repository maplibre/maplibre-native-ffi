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
    private set

  public var physicalDevice: NativePointer = physicalDevice
    private set

  public var device: NativePointer = device
    private set

  public var graphicsQueue: NativePointer = graphicsQueue
    private set

  public var getInstanceProcAddr: NativePointer = getInstanceProcAddr
    private set

  public var getDeviceProcAddr: NativePointer = getDeviceProcAddr
    private set

  public var graphicsQueueFamilyIndex: Int = graphicsQueueFamilyIndex
    private set

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
    require(index >= 0) { "graphicsQueueFamilyIndex must be non-negative" }
    this.graphicsQueueFamilyIndex = index
  }

  public fun getInstanceProcAddr(getInstanceProcAddr: NativePointer): VulkanContextDescriptor =
    apply {
      this.getInstanceProcAddr = getInstanceProcAddr
    }

  public fun getDeviceProcAddr(getDeviceProcAddr: NativePointer): VulkanContextDescriptor = apply {
    this.getDeviceProcAddr = getDeviceProcAddr
  }

  public fun procAddresses(
    getInstanceProcAddr: NativePointer,
    getDeviceProcAddr: NativePointer,
  ): VulkanContextDescriptor =
    getInstanceProcAddr(getInstanceProcAddr).getDeviceProcAddr(getDeviceProcAddr)
}
