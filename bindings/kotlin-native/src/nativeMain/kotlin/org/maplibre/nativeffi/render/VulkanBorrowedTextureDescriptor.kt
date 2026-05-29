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
    private set

  public var context: VulkanContextDescriptor = context
    private set

  public var image: NativePointer = image
    private set

  public var imageView: NativePointer = imageView
    private set

  public var format: Int = format
    private set

  public var initialLayout: Int = initialLayout
    private set

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

  public fun format(format: Int): VulkanBorrowedTextureDescriptor = apply {
    require(format >= 0) { "format must be non-negative" }
    this.format = format
  }

  public fun initialLayout(initialLayout: Int): VulkanBorrowedTextureDescriptor = apply {
    require(initialLayout >= 0) { "initialLayout must be non-negative" }
    this.initialLayout = initialLayout
  }

  public fun hasFinalLayout(): Boolean = finalLayout != null

  public fun finalLayout(finalLayout: Int): VulkanBorrowedTextureDescriptor = apply {
    require(finalLayout >= 0) { "finalLayout must be non-negative" }
    this.finalLayout = finalLayout
  }

  public fun clearFinalLayout(): VulkanBorrowedTextureDescriptor = apply { finalLayout = null }
}
