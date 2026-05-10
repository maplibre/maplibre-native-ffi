package org.maplibre.nativeffi;

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

  public VulkanOwnedTextureDescriptor setSize(int width, int height) {
    if (width <= 0 || height <= 0) {
      throw new IllegalArgumentException("width and height must be positive");
    }
    this.width = width;
    this.height = height;
    return this;
  }

  public double scaleFactor() {
    return scaleFactor;
  }

  public VulkanOwnedTextureDescriptor setScaleFactor(double scaleFactor) {
    if (!Double.isFinite(scaleFactor) || scaleFactor <= 0.0) {
      throw new IllegalArgumentException("scaleFactor must be finite and positive");
    }
    this.scaleFactor = scaleFactor;
    return this;
  }

  public NativePointer instance() {
    return instance;
  }

  public VulkanOwnedTextureDescriptor setInstance(NativePointer instance) {
    this.instance = Objects.requireNonNull(instance, "instance");
    return this;
  }

  public NativePointer physicalDevice() {
    return physicalDevice;
  }

  public VulkanOwnedTextureDescriptor setPhysicalDevice(NativePointer physicalDevice) {
    this.physicalDevice = Objects.requireNonNull(physicalDevice, "physicalDevice");
    return this;
  }

  public NativePointer device() {
    return device;
  }

  public VulkanOwnedTextureDescriptor setDevice(NativePointer device) {
    this.device = Objects.requireNonNull(device, "device");
    return this;
  }

  public NativePointer graphicsQueue() {
    return graphicsQueue;
  }

  public VulkanOwnedTextureDescriptor setGraphicsQueue(NativePointer graphicsQueue) {
    this.graphicsQueue = Objects.requireNonNull(graphicsQueue, "graphicsQueue");
    return this;
  }

  public int graphicsQueueFamilyIndex() {
    return graphicsQueueFamilyIndex;
  }

  public VulkanOwnedTextureDescriptor setGraphicsQueueFamilyIndex(int index) {
    if (index < 0) {
      throw new IllegalArgumentException("graphicsQueueFamilyIndex must be non-negative");
    }
    this.graphicsQueueFamilyIndex = index;
    return this;
  }
}
