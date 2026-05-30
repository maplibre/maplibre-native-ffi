package org.maplibre.nativeffi.style

import org.maplibre.nativeffi.render.PremultipliedRgba8Image

/** Copied runtime style image pixels with style image metadata. */
public data class StyleImage(
  public val image: PremultipliedRgba8Image,
  public val pixelRatio: Float,
  public val sdf: Boolean,
)
