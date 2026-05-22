package org.maplibre.nativejni.render;

import java.util.Objects;

/** Mutable descriptor for Vulkan caller-owned texture render targets. */
public final class VulkanBorrowedTextureDescriptor {
  private RenderTargetExtent extent = new RenderTargetExtent();
  private VulkanContextDescriptor context = new VulkanContextDescriptor();
  private NativePointer image = NativePointer.NULL;
  private NativePointer imageView = NativePointer.NULL;
  private int format;
  private int initialLayout;
  private Integer finalLayout;

  public RenderTargetExtent extent() {
    return extent;
  }

  public VulkanBorrowedTextureDescriptor extent(RenderTargetExtent extent) {
    this.extent = Objects.requireNonNull(extent, "extent");
    return this;
  }

  public VulkanContextDescriptor context() {
    return context;
  }

  public VulkanBorrowedTextureDescriptor context(VulkanContextDescriptor context) {
    this.context = Objects.requireNonNull(context, "context");
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
