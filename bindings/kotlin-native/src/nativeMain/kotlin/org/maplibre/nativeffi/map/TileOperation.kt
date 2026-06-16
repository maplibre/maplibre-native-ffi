package org.maplibre.nativeffi.map

import kotlin.jvm.JvmInline

/** Tile operation reported by runtime tile action events. */
@JvmInline
public value class TileOperation(public val nativeValue: Int) {
  public companion object {
    public val REQUESTED_FROM_CACHE: TileOperation = TileOperation(0)
    public val REQUESTED_FROM_NETWORK: TileOperation = TileOperation(1)
    public val LOAD_FROM_NETWORK: TileOperation = TileOperation(2)
    public val LOAD_FROM_CACHE: TileOperation = TileOperation(3)
    public val START_PARSE: TileOperation = TileOperation(4)
    public val END_PARSE: TileOperation = TileOperation(5)
    public val ERROR: TileOperation = TileOperation(6)
    public val CANCELLED: TileOperation = TileOperation(7)
    public val NULL: TileOperation = TileOperation(8)

    internal fun fromNative(nativeValue: UInt): TileOperation = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): TileOperation = TileOperation(nativeValue)
  }
}
