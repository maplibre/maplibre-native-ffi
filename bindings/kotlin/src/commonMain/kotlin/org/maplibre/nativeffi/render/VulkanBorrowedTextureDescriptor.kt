package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.status.Status

/** Mutable descriptor for Vulkan caller-owned texture render targets. */
public class VulkanBorrowedTextureDescriptor(
  extent: RenderTargetExtent,
  context: VulkanContextDescriptor,
  image: NativePointer,
  imageView: NativePointer,
  format: Int,
  initialLayout: Int,
) {
  public var extent: RenderTargetExtent = extent

  public var context: VulkanContextDescriptor = context

  public var image: NativePointer = image

  public var imageView: NativePointer = imageView

  public var format: Int = format
    set(value) {
      Status.requireArgument(value >= 0) { "format must be non-negative" }
      field = value
    }

  public var initialLayout: Int = initialLayout
    set(value) {
      Status.requireArgument(value >= 0) { "initialLayout must be non-negative" }
      field = value
    }

  public var finalLayout: Int? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "finalLayout must be non-negative" } }
      field = value
    }

  init {
    Status.requireArgument(format >= 0) { "format must be non-negative" }
    Status.requireArgument(initialLayout >= 0) { "initialLayout must be non-negative" }
  }
}
