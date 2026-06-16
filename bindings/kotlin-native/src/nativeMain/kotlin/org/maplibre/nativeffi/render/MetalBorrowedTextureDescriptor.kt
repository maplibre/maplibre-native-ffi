package org.maplibre.nativeffi.render

/** Mutable descriptor for Metal caller-owned texture render targets. */
public class MetalBorrowedTextureDescriptor(extent: RenderTargetExtent, texture: NativePointer) {
  public var extent: RenderTargetExtent = extent

  public var texture: NativePointer = texture
}
