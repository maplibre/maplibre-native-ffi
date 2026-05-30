package org.maplibre.nativeffi.map

/** Tile operation reported by runtime tile action events. */
public enum class TileOperation(public val nativeValue: Int) {
  REQUESTED_FROM_CACHE(0),
  REQUESTED_FROM_NETWORK(1),
  LOAD_FROM_NETWORK(2),
  LOAD_FROM_CACHE(3),
  START_PARSE(4),
  END_PARSE(5),
  ERROR(6),
  CANCELLED(7),
  NULL(8),
  UNKNOWN(-1);

  public companion object {
    internal fun fromNative(nativeValue: UInt): TileOperation = fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): TileOperation =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
