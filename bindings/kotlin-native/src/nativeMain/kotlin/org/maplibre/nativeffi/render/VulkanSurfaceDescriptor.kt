package org.maplibre.nativeffi.render

/** Mutable descriptor for Vulkan native surface render targets. */
public class VulkanSurfaceDescriptor(
  public var extent: RenderTargetExtent = RenderTargetExtent(),
  public var context: VulkanContextDescriptor = VulkanContextDescriptor(),
  public var surface: NativePointer = NativePointer.NULL,
) {
  public fun extent(extent: RenderTargetExtent): VulkanSurfaceDescriptor = apply {
    this.extent = extent
  }

  public fun context(context: VulkanContextDescriptor): VulkanSurfaceDescriptor = apply {
    this.context = context
  }

  public fun surface(surface: NativePointer): VulkanSurfaceDescriptor = apply {
    this.surface = surface
  }
}
