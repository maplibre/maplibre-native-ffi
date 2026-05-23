package org.maplibre.nativeffi.render

/** Mutable logical render target extent. */
public class RenderTargetExtent(
  public var width: UInt = 256U,
  public var height: UInt = 256U,
  public var scaleFactor: Double = 1.0,
) {
  public fun size(width: UInt, height: UInt): RenderTargetExtent = apply {
    this.width = width
    this.height = height
  }

  public fun scaleFactor(scaleFactor: Double): RenderTargetExtent = apply {
    this.scaleFactor = scaleFactor
  }
}
