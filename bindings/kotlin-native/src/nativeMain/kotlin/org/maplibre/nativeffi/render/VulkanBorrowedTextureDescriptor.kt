package org.maplibre.nativeffi.render

/** Mutable descriptor for Vulkan caller-owned texture render targets. */
public class VulkanBorrowedTextureDescriptor(
  public var extent: RenderTargetExtent = RenderTargetExtent(),
  public var context: VulkanContextDescriptor = VulkanContextDescriptor(),
  public var image: NativePointer = NativePointer.NULL,
  public var imageView: NativePointer = NativePointer.NULL,
  public var format: UInt = 0U,
  public var initialLayout: UInt = 0U,
) {
  public var finalLayout: UInt? = null
    private set

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

  public fun format(format: UInt): VulkanBorrowedTextureDescriptor = apply { this.format = format }

  public fun initialLayout(initialLayout: UInt): VulkanBorrowedTextureDescriptor = apply {
    this.initialLayout = initialLayout
  }

  public fun hasFinalLayout(): Boolean = finalLayout != null

  public fun finalLayout(finalLayout: UInt): VulkanBorrowedTextureDescriptor = apply {
    this.finalLayout = finalLayout
  }

  public fun clearFinalLayout(): VulkanBorrowedTextureDescriptor = apply { finalLayout = null }
}
