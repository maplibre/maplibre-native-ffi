package org.maplibre.nativeffi.style

/** Mutable descriptor for runtime style image options. */
public class StyleImageOptions {
  public var pixelRatio: Float? = null
    private set

  public var sdf: Boolean? = null
    private set

  public fun hasPixelRatio(): Boolean = pixelRatio != null

  public fun pixelRatio(pixelRatio: Float): StyleImageOptions = apply {
    this.pixelRatio = pixelRatio
  }

  public fun clearPixelRatio(): StyleImageOptions = apply { pixelRatio = null }

  public fun hasSdf(): Boolean = sdf != null

  public fun sdf(sdf: Boolean): StyleImageOptions = apply { this.sdf = sdf }

  public fun clearSdf(): StyleImageOptions = apply { sdf = null }
}
