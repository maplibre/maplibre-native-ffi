package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.status.Status

/** Mutable logical render target extent. */
public class RenderTargetExtent(width: Int, height: Int, scaleFactor: Double) {
  public var width: Int = width
    set(value) {
      Status.requireArgument(value >= 0) { "width must be non-negative" }
      field = value
    }

  public var height: Int = height
    set(value) {
      Status.requireArgument(value >= 0) { "height must be non-negative" }
      field = value
    }

  public var scaleFactor: Double = scaleFactor
    set(value) {
      Status.requireArgument(value.isFinite() && value > 0.0) {
        "scaleFactor must be finite and positive"
      }
      field = value
    }

  init {
    Status.requireArgument(width >= 0) { "width must be non-negative" }
    Status.requireArgument(height >= 0) { "height must be non-negative" }
    Status.requireArgument(scaleFactor.isFinite() && scaleFactor > 0.0) {
      "scaleFactor must be finite and positive"
    }
  }
}

/** Physical render target size in device pixels. */
public class PhysicalRenderTargetSize(public val width: Int, public val height: Int)

/**
 * Returns this extent's physical device-pixel size as `ceil(logical * scaleFactor)` per dimension.
 *
 * Session-owned texture targets and surface targets are sized this way. Borrowed texture targets
 * state their physical size instead, because not every physical size is reachable from a logical
 * extent.
 */
public expect fun RenderTargetExtent.physicalSize(): PhysicalRenderTargetSize
