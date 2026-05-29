package org.maplibre.nativeffi.render

/** Mutable descriptor for Vulkan native surface render targets. */
public class VulkanSurfaceDescriptor(
  extent: RenderTargetExtent = RenderTargetExtent(),
  context: VulkanContextDescriptor = VulkanContextDescriptor(),
  surface: NativePointer = NativePointer.NULL,
) {
  public var extent: RenderTargetExtent = extent
    private set

  public var context: VulkanContextDescriptor = context
    private set

  public var surface: NativePointer = surface
    private set

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
