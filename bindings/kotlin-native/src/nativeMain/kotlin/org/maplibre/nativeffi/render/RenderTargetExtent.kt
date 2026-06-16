package org.maplibre.nativeffi.render

/** Mutable logical render target extent. */
public class RenderTargetExtent(width: Int, height: Int, scaleFactor: Double) {
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

  public var scaleFactor: Double = scaleFactor

  init {
    require(width >= 0) { "width must be non-negative" }
    require(height >= 0) { "height must be non-negative" }
  }
}
