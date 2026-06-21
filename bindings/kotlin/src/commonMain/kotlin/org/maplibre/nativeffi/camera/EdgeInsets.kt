package org.maplibre.nativeffi.camera

/** Screen-space insets in logical map pixels. */
public data class EdgeInsets(
  public val top: Double,
  public val left: Double,
  public val bottom: Double,
  public val right: Double,
) {
  public companion object {
    public val ZERO: EdgeInsets = EdgeInsets(0.0, 0.0, 0.0, 0.0)
  }
}
