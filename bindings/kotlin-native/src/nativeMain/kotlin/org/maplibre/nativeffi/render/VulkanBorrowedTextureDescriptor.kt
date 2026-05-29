package org.maplibre.nativeffi.render

/** Mutable descriptor for Vulkan caller-owned texture render targets. */
public class VulkanBorrowedTextureDescriptor(
  extent: RenderTargetExtent = RenderTargetExtent(),
  context: VulkanContextDescriptor = VulkanContextDescriptor(),
  image: NativePointer = NativePointer.NULL,
  imageView: NativePointer = NativePointer.NULL,
  format: Int = 0,
  initialLayout: Int = 0,
) {
  public var extent: RenderTargetExtent = extent

  public var context: VulkanContextDescriptor = context

  public var image: NativePointer = image

  public var imageView: NativePointer = imageView

  public var format: Int = format
    set(value) {
      require(value >= 0) { "format must be non-negative" }
      field = value
    }

  public var initialLayout: Int = initialLayout
    set(value) {
      require(value >= 0) { "initialLayout must be non-negative" }
      field = value
    }

  public var finalLayout: Int? = null
    set(value) {
      value?.let { require(it >= 0) { "finalLayout must be non-negative" } }
      field = value
    }

  init {
    require(format >= 0) { "format must be non-negative" }
    require(initialLayout >= 0) { "initialLayout must be non-negative" }
  }
}
