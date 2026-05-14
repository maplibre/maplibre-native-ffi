package org.maplibre.nativeffi.render;

import java.util.Objects;

/** Mutable descriptor for Vulkan session-owned texture render targets. */
public final class VulkanOwnedTextureDescriptor {
  private RenderTargetExtent extent = new RenderTargetExtent();
  private VulkanContextDescriptor context = new VulkanContextDescriptor();

  public RenderTargetExtent extent() {
    return extent;
  }

  public VulkanOwnedTextureDescriptor extent(RenderTargetExtent extent) {
    this.extent = Objects.requireNonNull(extent, "extent");
    return this;
  }

  public VulkanContextDescriptor context() {
    return context;
  }

  public VulkanOwnedTextureDescriptor context(VulkanContextDescriptor context) {
    this.context = Objects.requireNonNull(context, "context");
    return this;
  }
}
