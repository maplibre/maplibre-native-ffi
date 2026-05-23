package org.maplibre.nativeffi.runtime

/** Offline operation kind reported by completion events. */
public enum class OfflineOperationKind(internal val nativeValue: UInt) {
  AMBIENT_CACHE(1U),
  REGION_CREATE(2U),
  REGION_GET(3U),
  REGIONS_LIST(4U),
  REGIONS_MERGE_DATABASE(5U),
  REGION_UPDATE_METADATA(6U),
  REGION_GET_STATUS(7U),
  REGION_SET_OBSERVED(8U),
  REGION_SET_DOWNLOAD_STATE(9U),
  REGION_INVALIDATE(10U),
  REGION_DELETE(11U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): OfflineOperationKind =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
