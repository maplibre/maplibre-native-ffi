package org.maplibre.nativeffi.render;

import java.util.Objects;

/** Mutable Vulkan backend context descriptor. */
public final class VulkanContextDescriptor {
  private NativePointer instance = NativePointer.NULL;
  private NativePointer physicalDevice = NativePointer.NULL;
  private NativePointer device = NativePointer.NULL;
  private NativePointer graphicsQueue = NativePointer.NULL;
  private int graphicsQueueFamilyIndex;

  public VulkanContextDescriptor() {}

  public VulkanContextDescriptor(
      NativePointer instance,
      NativePointer physicalDevice,
      NativePointer device,
      NativePointer graphicsQueue,
      int graphicsQueueFamilyIndex) {
    this.instance = Objects.requireNonNull(instance, "instance");
    this.physicalDevice = Objects.requireNonNull(physicalDevice, "physicalDevice");
    this.device = Objects.requireNonNull(device, "device");
    this.graphicsQueue = Objects.requireNonNull(graphicsQueue, "graphicsQueue");
    this.graphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
  }

  public NativePointer instance() {
    return instance;
  }

  public VulkanContextDescriptor instance(NativePointer instance) {
    this.instance = Objects.requireNonNull(instance, "instance");
    return this;
  }

  public NativePointer physicalDevice() {
    return physicalDevice;
  }

  public VulkanContextDescriptor physicalDevice(NativePointer physicalDevice) {
    this.physicalDevice = Objects.requireNonNull(physicalDevice, "physicalDevice");
    return this;
  }

  public NativePointer device() {
    return device;
  }

  public VulkanContextDescriptor device(NativePointer device) {
    this.device = Objects.requireNonNull(device, "device");
    return this;
  }

  public NativePointer graphicsQueue() {
    return graphicsQueue;
  }

  public VulkanContextDescriptor graphicsQueue(NativePointer graphicsQueue) {
    this.graphicsQueue = Objects.requireNonNull(graphicsQueue, "graphicsQueue");
    return this;
  }

  public int graphicsQueueFamilyIndex() {
    return graphicsQueueFamilyIndex;
  }

  public VulkanContextDescriptor graphicsQueueFamilyIndex(int index) {
    this.graphicsQueueFamilyIndex = index;
    return this;
  }
}
