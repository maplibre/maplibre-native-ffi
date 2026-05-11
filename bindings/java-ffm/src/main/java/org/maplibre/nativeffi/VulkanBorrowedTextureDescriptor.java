package org.maplibre.nativeffi;

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

  public VulkanBorrowedTextureDescriptor setSize(int width, int height) {
    this.width = width;
    this.height = height;
    return this;
  }

  public double scaleFactor() {
    return scaleFactor;
  }

  public VulkanBorrowedTextureDescriptor setScaleFactor(double scaleFactor) {
    this.scaleFactor = scaleFactor;
    return this;
  }

  public NativePointer instance() {
    return instance;
  }

  public VulkanBorrowedTextureDescriptor setInstance(NativePointer instance) {
    this.instance = Objects.requireNonNull(instance, "instance");
    return this;
  }

  public NativePointer physicalDevice() {
    return physicalDevice;
  }

  public VulkanBorrowedTextureDescriptor setPhysicalDevice(NativePointer physicalDevice) {
    this.physicalDevice = Objects.requireNonNull(physicalDevice, "physicalDevice");
    return this;
  }

  public NativePointer device() {
    return device;
  }

  public VulkanBorrowedTextureDescriptor setDevice(NativePointer device) {
    this.device = Objects.requireNonNull(device, "device");
    return this;
  }

  public NativePointer graphicsQueue() {
    return graphicsQueue;
  }

  public VulkanBorrowedTextureDescriptor setGraphicsQueue(NativePointer graphicsQueue) {
    this.graphicsQueue = Objects.requireNonNull(graphicsQueue, "graphicsQueue");
    return this;
  }

  public int graphicsQueueFamilyIndex() {
    return graphicsQueueFamilyIndex;
  }

  public VulkanBorrowedTextureDescriptor setGraphicsQueueFamilyIndex(int index) {
    this.graphicsQueueFamilyIndex = index;
    return this;
  }

  public NativePointer image() {
    return image;
  }

  public VulkanBorrowedTextureDescriptor setImage(NativePointer image) {
    this.image = Objects.requireNonNull(image, "image");
    return this;
  }

  public NativePointer imageView() {
    return imageView;
  }

  public VulkanBorrowedTextureDescriptor setImageView(NativePointer imageView) {
    this.imageView = Objects.requireNonNull(imageView, "imageView");
    return this;
  }

  public int format() {
    return format;
  }

  public VulkanBorrowedTextureDescriptor setFormat(int format) {
    this.format = format;
    return this;
  }

  public int initialLayout() {
    return initialLayout;
  }

  public VulkanBorrowedTextureDescriptor setInitialLayout(int initialLayout) {
    this.initialLayout = initialLayout;
    return this;
  }

  public boolean hasFinalLayout() {
    return finalLayout != null;
  }

  public Integer finalLayout() {
    return finalLayout;
  }

  public VulkanBorrowedTextureDescriptor setFinalLayout(int finalLayout) {
    this.finalLayout = finalLayout;
    return this;
  }

  public VulkanBorrowedTextureDescriptor clearFinalLayout() {
    finalLayout = null;
    return this;
  }
}
