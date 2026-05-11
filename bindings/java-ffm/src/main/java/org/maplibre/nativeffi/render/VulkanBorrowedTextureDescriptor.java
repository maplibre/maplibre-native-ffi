package org.maplibre.nativeffi.render;

import java.util.Objects;

/** Mutable descriptor for Vulkan caller-owned texture render targets. */
public final class VulkanBorrowedTextureDescriptor {
  private int width = 256;
  private int height = 256;
  private double scaleFactor = 1.0;
  private NativePointer instance = NativePointer.NULL;
  private NativePointer physicalDevice = NativePointer.NULL;
  private NativePointer device = NativePointer.NULL;
  private NativePointer graphicsQueue = NativePointer.NULL;
  private int graphicsQueueFamilyIndex;
  private NativePointer image = NativePointer.NULL;
  private NativePointer imageView = NativePointer.NULL;
  private int format;
  private int initialLayout;
  private Integer finalLayout;

  public int width() {
    return width;
  }

  public int height() {
    return height;
  }

  public VulkanBorrowedTextureDescriptor size(int width, int height) {
    this.width = width;
    this.height = height;
    return this;
  }

  public double scaleFactor() {
    return scaleFactor;
  }

  public VulkanBorrowedTextureDescriptor scaleFactor(double scaleFactor) {
    this.scaleFactor = scaleFactor;
    return this;
  }

  public NativePointer instance() {
    return instance;
  }

  public VulkanBorrowedTextureDescriptor instance(NativePointer instance) {
    this.instance = Objects.requireNonNull(instance, "instance");
    return this;
  }

  public NativePointer physicalDevice() {
    return physicalDevice;
  }

  public VulkanBorrowedTextureDescriptor physicalDevice(NativePointer physicalDevice) {
    this.physicalDevice = Objects.requireNonNull(physicalDevice, "physicalDevice");
    return this;
  }

  public NativePointer device() {
    return device;
  }

  public VulkanBorrowedTextureDescriptor device(NativePointer device) {
    this.device = Objects.requireNonNull(device, "device");
    return this;
  }

  public NativePointer graphicsQueue() {
    return graphicsQueue;
  }

  public VulkanBorrowedTextureDescriptor graphicsQueue(NativePointer graphicsQueue) {
    this.graphicsQueue = Objects.requireNonNull(graphicsQueue, "graphicsQueue");
    return this;
  }

  public int graphicsQueueFamilyIndex() {
    return graphicsQueueFamilyIndex;
  }

  public VulkanBorrowedTextureDescriptor graphicsQueueFamilyIndex(int index) {
    this.graphicsQueueFamilyIndex = index;
    return this;
  }

  public NativePointer image() {
    return image;
  }

  public VulkanBorrowedTextureDescriptor image(NativePointer image) {
    this.image = Objects.requireNonNull(image, "image");
    return this;
  }

  public NativePointer imageView() {
    return imageView;
  }

  public VulkanBorrowedTextureDescriptor imageView(NativePointer imageView) {
    this.imageView = Objects.requireNonNull(imageView, "imageView");
    return this;
  }

  public int format() {
    return format;
  }

  public VulkanBorrowedTextureDescriptor format(int format) {
    this.format = format;
    return this;
  }

  public int initialLayout() {
    return initialLayout;
  }

  public VulkanBorrowedTextureDescriptor initialLayout(int initialLayout) {
    this.initialLayout = initialLayout;
    return this;
  }

  public boolean hasFinalLayout() {
    return finalLayout != null;
  }

  public Integer finalLayout() {
    return finalLayout;
  }

  public VulkanBorrowedTextureDescriptor finalLayout(int finalLayout) {
    this.finalLayout = finalLayout;
    return this;
  }

  public VulkanBorrowedTextureDescriptor clearFinalLayout() {
    finalLayout = null;
    return this;
  }
}
