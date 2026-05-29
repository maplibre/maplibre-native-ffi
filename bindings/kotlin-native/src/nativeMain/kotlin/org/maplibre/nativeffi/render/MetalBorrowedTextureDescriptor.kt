package org.maplibre.nativeffi.render

/** Mutable descriptor for Metal caller-owned texture render targets. */
public class MetalBorrowedTextureDescriptor(
  extent: RenderTargetExtent = RenderTargetExtent(),
  texture: NativePointer = NativePointer.NULL,
) {
  public var extent: RenderTargetExtent = extent
    private set

  public var texture: NativePointer = texture
    private set

  public fun extent(extent: RenderTargetExtent): MetalBorrowedTextureDescriptor = apply {
    this.extent = extent
  }

  public fun texture(texture: NativePointer): MetalBorrowedTextureDescriptor = apply {
    this.texture = texture
  }
}
