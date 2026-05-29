package org.maplibre.nativeffi.render

/** Mutable logical render target extent. */
public class RenderTargetExtent(width: Int = 256, height: Int = 256, scaleFactor: Double = 1.0) {
  public var width: Int = width
    private set

  public var height: Int = height
    private set

  public var scaleFactor: Double = scaleFactor
    private set

  init {
    require(width >= 0) { "width must be non-negative" }
    require(height >= 0) { "height must be non-negative" }
  }

  public fun size(width: Int, height: Int): RenderTargetExtent = apply {
    require(width >= 0) { "width must be non-negative" }
    require(height >= 0) { "height must be non-negative" }
    this.width = width
    this.height = height
  }

  public fun scaleFactor(scaleFactor: Double): RenderTargetExtent = apply {
    this.scaleFactor = scaleFactor
  }
}
