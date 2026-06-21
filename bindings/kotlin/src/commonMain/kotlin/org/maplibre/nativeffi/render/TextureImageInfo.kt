package org.maplibre.nativeffi.render

/** CPU texture readback metadata in physical pixels. */
public data class TextureImageInfo(
  public val width: Int,
  public val height: Int,
  public val stride: Int,
  public val byteLength: Long,
)
