package org.maplibre.nativeffi.render

/** Mutable descriptor for Metal caller-owned texture render targets. */
public class MetalBorrowedTextureDescriptor(
  extent: RenderTargetExtent = RenderTargetExtent(),
  texture: NativePointer = NativePointer.NULL,
) {
  public var extent: RenderTargetExtent = extent

  public var texture: NativePointer = texture
}
