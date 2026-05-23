package org.maplibre.nativeffi.map

/** Map debug overlay options. */
public enum class DebugOption(internal val nativeMask: UInt) {
  TILE_BORDERS(1U shl 1),
  PARSE_STATUS(1U shl 2),
  TIMESTAMPS(1U shl 3),
  COLLISION(1U shl 4),
  OVERDRAW(1U shl 5),
  STENCIL_CLIP(1U shl 6),
  DEPTH_BUFFER(1U shl 7),
}
