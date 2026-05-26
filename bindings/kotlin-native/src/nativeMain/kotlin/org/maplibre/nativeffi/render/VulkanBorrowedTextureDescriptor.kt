package org.maplibre.nativeffi.render

/** Mutable descriptor for Vulkan caller-owned texture render targets. */
public class VulkanBorrowedTextureDescriptor(
  public var extent: RenderTargetExtent = RenderTargetExtent(),
  public var context: VulkanContextDescriptor = VulkanContextDescriptor(),
  public var image: NativePointer = NativePointer.NULL,
  public var imageView: NativePointer = NativePointer.NULL,
  format: Int = 0,
  initialLayout: Int = 0,
) {
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
    private set

  init {
    require(format >= 0) { "format must be non-negative" }
    require(initialLayout >= 0) { "initialLayout must be non-negative" }
  }

  public fun extent(extent: RenderTargetExtent): VulkanBorrowedTextureDescriptor = apply {
    this.extent = extent
  }

  public fun context(context: VulkanContextDescriptor): VulkanBorrowedTextureDescriptor = apply {
    this.context = context
  }

  public fun image(image: NativePointer): VulkanBorrowedTextureDescriptor = apply {
    this.image = image
  }

  public fun imageView(imageView: NativePointer): VulkanBorrowedTextureDescriptor = apply {
    this.imageView = imageView
  }

  public fun format(format: Int): VulkanBorrowedTextureDescriptor = apply { this.format = format }

  public fun initialLayout(initialLayout: Int): VulkanBorrowedTextureDescriptor = apply {
    this.initialLayout = initialLayout
  }

  public fun hasFinalLayout(): Boolean = finalLayout != null

  public fun finalLayout(finalLayout: Int): VulkanBorrowedTextureDescriptor = apply {
    require(finalLayout >= 0) { "finalLayout must be non-negative" }
    this.finalLayout = finalLayout
  }

  public fun clearFinalLayout(): VulkanBorrowedTextureDescriptor = apply { finalLayout = null }
}
