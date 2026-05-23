package org.maplibre.nativeffi.render

/** CPU texture readback metadata in physical pixels. */
public data class TextureImageInfo(
  public val width: UInt,
  public val height: UInt,
  public val stride: UInt,
  public val byteLength: ULong,
)
