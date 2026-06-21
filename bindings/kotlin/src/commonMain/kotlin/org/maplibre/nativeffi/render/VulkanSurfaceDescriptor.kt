package org.maplibre.nativeffi.render

/** Mutable descriptor for Vulkan native surface render targets. */
public class VulkanSurfaceDescriptor(
  extent: RenderTargetExtent,
  context: VulkanContextDescriptor,
  surface: NativePointer,
) {
  public var extent: RenderTargetExtent = extent

  public var context: VulkanContextDescriptor = context

  public var surface: NativePointer = surface
}
