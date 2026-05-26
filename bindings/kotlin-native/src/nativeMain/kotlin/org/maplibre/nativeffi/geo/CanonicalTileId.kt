package org.maplibre.nativeffi.geo

/** Canonical tile identity used by custom geometry source callbacks. */
public data class CanonicalTileId(public val z: Int, public val x: Long, public val y: Long)
