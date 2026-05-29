package org.maplibre.nativeffi.render

/** Mutable descriptor for Vulkan native surface render targets. */
public class VulkanSurfaceDescriptor(
  extent: RenderTargetExtent = RenderTargetExtent(),
  context: VulkanContextDescriptor = VulkanContextDescriptor(),
  surface: NativePointer = NativePointer.NULL,
) {
  public var extent: RenderTargetExtent = extent

  public var context: VulkanContextDescriptor = context

  public var surface: NativePointer = surface
}
