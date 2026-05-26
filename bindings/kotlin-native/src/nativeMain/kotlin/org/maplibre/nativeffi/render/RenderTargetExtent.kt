package org.maplibre.nativeffi.render

/** Mutable logical render target extent. */
public class RenderTargetExtent(
  width: Int = 256,
  height: Int = 256,
  public var scaleFactor: Double = 1.0,
) {
  public var width: Int = width
    set(value) {
      require(value >= 0) { "width must be non-negative" }
      field = value
    }

  public var height: Int = height
    set(value) {
      require(value >= 0) { "height must be non-negative" }
      field = value
    }

  init {
    require(width >= 0) { "width must be non-negative" }
    require(height >= 0) { "height must be non-negative" }
  }

  public fun size(width: Int, height: Int): RenderTargetExtent = apply {
    this.width = width
    this.height = height
  }

  public fun scaleFactor(scaleFactor: Double): RenderTargetExtent = apply {
    this.scaleFactor = scaleFactor
  }
}
