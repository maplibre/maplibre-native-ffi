package org.maplibre.nativeffi.style

/** Vector tile encoding for vector style sources. */
public enum class VectorTileEncoding(public val nativeValue: Int) {
  MVT(0),
  MLT(1),
}
