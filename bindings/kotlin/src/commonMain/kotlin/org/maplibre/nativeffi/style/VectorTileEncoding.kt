package org.maplibre.nativeffi.style

import kotlin.jvm.JvmInline

/** Vector tile encoding for vector style sources, including unknown native values. */
@JvmInline
public value class VectorTileEncoding(public val nativeValue: Int) {
  public companion object {
    public val MVT: VectorTileEncoding = VectorTileEncoding(0)
    public val MLT: VectorTileEncoding = VectorTileEncoding(1)

    internal fun fromNative(nativeValue: UInt): VectorTileEncoding =
      VectorTileEncoding(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): VectorTileEncoding = VectorTileEncoding(nativeValue)
  }
}
