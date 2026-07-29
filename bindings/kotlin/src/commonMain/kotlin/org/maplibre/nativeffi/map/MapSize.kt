package org.maplibre.nativeffi.map

/** The map's logical viewport size in UI pixels together with its pixel ratio. */
public data class MapSize(
  public val width: Int,
  public val height: Int,
  public val scaleFactor: Double,
)
