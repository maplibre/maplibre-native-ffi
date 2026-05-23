package org.maplibre.nativeffi.geo

/** Overscaled tile identity. */
public data class TileId(
  public val overscaledZ: UInt,
  public val wrap: Int,
  public val canonical: CanonicalTileId,
)
