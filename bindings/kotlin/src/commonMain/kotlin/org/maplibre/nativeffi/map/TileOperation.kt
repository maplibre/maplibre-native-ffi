package org.maplibre.nativeffi.map

import kotlin.jvm.JvmInline

/**
 * Tile operation reported by runtime tile action events.
 *
 * This is an open domain: a value may have no named constant here, so a `when` over this type needs
 * an `else` branch. Unknown values keep their raw [nativeValue].
 */
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

    /**
     * No tile operation took place.
     *
     * Kotlin/Native writes companion properties into a generated Objective-C header. `NULL` is a C
     * macro, so this value is named `NULL_OP`.
     */
    public val NULL_OP: TileOperation = TileOperation(8)

    internal fun fromNative(nativeValue: UInt): TileOperation = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): TileOperation = TileOperation(nativeValue)
  }
}
