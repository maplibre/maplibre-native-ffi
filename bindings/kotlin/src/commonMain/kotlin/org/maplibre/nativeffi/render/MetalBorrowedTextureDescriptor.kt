package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.status.Status

/** Mutable descriptor for Metal caller-owned texture render targets. */
public class MetalBorrowedTextureDescriptor(
  extent: RenderTargetExtent,
  physicalWidth: Int,
  physicalHeight: Int,
  texture: NativePointer,
) {
  public var extent: RenderTargetExtent = extent

  /** Physical texture size in device pixels, stated by the texture's owner rather than derived. */
  public var physicalWidth: Int = physicalWidth
    set(value) {
      Status.requireArgument(value >= 0) { "physicalWidth must be non-negative" }
      field = value
    }

  /** Physical texture size in device pixels, stated by the texture's owner rather than derived. */
  public var physicalHeight: Int = physicalHeight
    set(value) {
      Status.requireArgument(value >= 0) { "physicalHeight must be non-negative" }
      field = value
    }

  public var texture: NativePointer = texture

  init {
    Status.requireArgument(physicalWidth >= 0) { "physicalWidth must be non-negative" }
    Status.requireArgument(physicalHeight >= 0) { "physicalHeight must be non-negative" }
  }
}
