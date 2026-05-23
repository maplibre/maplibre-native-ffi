package org.maplibre.nativeffi.render

/** Mutable descriptor for Metal caller-owned texture render targets. */
public class MetalBorrowedTextureDescriptor(
  public var extent: RenderTargetExtent = RenderTargetExtent(),
  public var texture: NativePointer = NativePointer.NULL,
) {
  public fun extent(extent: RenderTargetExtent): MetalBorrowedTextureDescriptor = apply {
    this.extent = extent
  }

  public fun texture(texture: NativePointer): MetalBorrowedTextureDescriptor = apply {
    this.texture = texture
  }
}
