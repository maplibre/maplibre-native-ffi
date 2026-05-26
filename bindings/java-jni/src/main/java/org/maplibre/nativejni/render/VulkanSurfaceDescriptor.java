package org.maplibre.nativejni.render;

import java.util.Objects;

/** Mutable descriptor for Vulkan native surface render targets. */
public final class VulkanSurfaceDescriptor {
  private RenderTargetExtent extent = new RenderTargetExtent();
  private VulkanContextDescriptor context = new VulkanContextDescriptor();
  private NativePointer surface = NativePointer.NULL;

  public RenderTargetExtent extent() {
    return extent;
  }

  public VulkanSurfaceDescriptor extent(RenderTargetExtent extent) {
    this.extent = Objects.requireNonNull(extent, "extent");
    return this;
  }

  public VulkanContextDescriptor context() {
    return context;
  }

  public VulkanSurfaceDescriptor context(VulkanContextDescriptor context) {
    this.context = Objects.requireNonNull(context, "context");
    return this;
  }

  public NativePointer surface() {
    return surface;
  }

  public VulkanSurfaceDescriptor surface(NativePointer surface) {
    this.surface = Objects.requireNonNull(surface, "surface");
    return this;
  }
}
