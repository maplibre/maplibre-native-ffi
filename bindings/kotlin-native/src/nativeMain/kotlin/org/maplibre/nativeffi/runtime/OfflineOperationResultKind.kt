package org.maplibre.nativeffi.runtime

/** Offline operation result kind reported by completion events. */
public enum class OfflineOperationResultKind(public val nativeValue: UInt) {
  NONE(0U),
  REGION(1U),
  OPTIONAL_REGION(2U),
  REGION_LIST(3U),
  REGION_STATUS(4U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): OfflineOperationResultKind =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
