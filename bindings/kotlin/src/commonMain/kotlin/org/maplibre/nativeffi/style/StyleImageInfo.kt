package org.maplibre.nativeffi.style

/** Copied metadata for one runtime style image. */
public data class StyleImageInfo(
  public val width: Int,
  public val height: Int,
  public val stride: Int,
  public val byteLength: Long,
  public val pixelRatio: Float,
  public val sdf: Boolean,
)
