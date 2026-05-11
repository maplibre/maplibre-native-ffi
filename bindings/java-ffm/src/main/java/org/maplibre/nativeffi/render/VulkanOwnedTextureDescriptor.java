package org.maplibre.nativeffi.render;

import java.util.Objects;

/** Mutable descriptor for Vulkan session-owned texture render targets. */
public final class VulkanOwnedTextureDescriptor {
  private int width = 256;
  private int height = 256;
  private double scaleFactor = 1.0;
  private NativePointer instance = NativePointer.NULL;
  private NativePointer physicalDevice = NativePointer.NULL;
  private NativePointer device = NativePointer.NULL;
  private NativePointer graphicsQueue = NativePointer.NULL;
  private int graphicsQueueFamilyIndex;

  public int width() {
    return width;
  }

  public int height() {
    return height;
  }

  public VulkanOwnedTextureDescriptor size(int width, int height) {
    this.width = width;
    this.height = height;
    return this;
  }

  public double scaleFactor() {
    return scaleFactor;
  }

  public VulkanOwnedTextureDescriptor scaleFactor(double scaleFactor) {
    this.scaleFactor = scaleFactor;
    return this;
  }

  public NativePointer instance() {
    return instance;
  }

  public VulkanOwnedTextureDescriptor instance(NativePointer instance) {
    this.instance = Objects.requireNonNull(instance, "instance");
    return this;
  }

  public NativePointer physicalDevice() {
    return physicalDevice;
  }

  public VulkanOwnedTextureDescriptor physicalDevice(NativePointer physicalDevice) {
    this.physicalDevice = Objects.requireNonNull(physicalDevice, "physicalDevice");
    return this;
  }

  public NativePointer device() {
    return device;
  }

  public VulkanOwnedTextureDescriptor device(NativePointer device) {
    this.device = Objects.requireNonNull(device, "device");
    return this;
  }

  public NativePointer graphicsQueue() {
    return graphicsQueue;
  }

  public VulkanOwnedTextureDescriptor graphicsQueue(NativePointer graphicsQueue) {
    this.graphicsQueue = Objects.requireNonNull(graphicsQueue, "graphicsQueue");
    return this;
  }

  public int graphicsQueueFamilyIndex() {
    return graphicsQueueFamilyIndex;
  }

  public VulkanOwnedTextureDescriptor graphicsQueueFamilyIndex(int index) {
    this.graphicsQueueFamilyIndex = index;
    return this;
  }
}
