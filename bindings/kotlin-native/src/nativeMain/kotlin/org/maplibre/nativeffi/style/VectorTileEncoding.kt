package org.maplibre.nativeffi.style

/** Vector tile encoding for vector style sources. */
public enum class VectorTileEncoding(public val nativeValue: UInt) {
  MVT(0U),
  MLT(1U),
}
