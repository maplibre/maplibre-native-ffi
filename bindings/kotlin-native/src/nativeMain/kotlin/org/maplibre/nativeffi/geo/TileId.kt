package org.maplibre.nativeffi.geo

/** Overscaled tile identity copied from native event payloads. */
public data class TileId(
  public val overscaledZ: Long,
  public val wrap: Int,
  public val canonicalZ: Long,
  public val canonicalX: Long,
  public val canonicalY: Long,
)
