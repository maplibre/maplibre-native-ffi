package org.maplibre.nativeffi.map

/** Tile operation reported by runtime tile action events. */
public enum class TileOperation(internal val nativeValue: UInt) {
  REQUESTED_FROM_CACHE(0U),
  REQUESTED_FROM_NETWORK(1U),
  LOAD_FROM_NETWORK(2U),
  LOAD_FROM_CACHE(3U),
  START_PARSE(4U),
  END_PARSE(5U),
  ERROR(6U),
  CANCELLED(7U),
  NULL(8U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): TileOperation =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
