package org.maplibre.nativeffi.map

/** Map debug overlay options. */
public enum class DebugOption(public val nativeMask: Int) {
  TILE_BORDERS(1 shl 1),
  PARSE_STATUS(1 shl 2),
  TIMESTAMPS(1 shl 3),
  COLLISION(1 shl 4),
  OVERDRAW(1 shl 5),
  STENCIL_CLIP(1 shl 6),
  DEPTH_BUFFER(1 shl 7),
}
