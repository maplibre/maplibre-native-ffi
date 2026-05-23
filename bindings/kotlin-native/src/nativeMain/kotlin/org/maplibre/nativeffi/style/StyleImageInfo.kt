package org.maplibre.nativeffi.style

/** Copied metadata for one runtime style image. */
public data class StyleImageInfo(
  public val width: UInt,
  public val height: UInt,
  public val stride: UInt,
  public val byteLength: ULong,
  public val pixelRatio: Float,
  public val sdf: Boolean,
)
