package org.maplibre.nativeffi.style

/** Copied metadata for one runtime style image. */
public data class StyleImageInfo(
  public val width: Int,
  public val height: Int,
  public val stride: Int,
  public val byteLength: Long,
  public val pixelRatio: Float,
  public val sdf: Boolean,
  /**
   * Interval counts for the stretchable axes. Read the intervals themselves with
   * `MapHandle.styleImageStretches`.
   */
  public val stretchXCount: Long = 0,
  public val stretchYCount: Long = 0,
  /** Content box, absent when the image carries none. */
  public val content: ImageContent? = null,
  public val textFitWidth: StyleImageTextFit? = null,
  public val textFitHeight: StyleImageTextFit? = null,
)
