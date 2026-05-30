package org.maplibre.nativeffi.runtime

/** Offline operation kind reported by completion events. */
public enum class OfflineOperationKind(public val nativeValue: Int) {
  AMBIENT_CACHE(1),
  REGION_CREATE(2),
  REGION_GET(3),
  REGIONS_LIST(4),
  REGIONS_MERGE_DATABASE(5),
  REGION_UPDATE_METADATA(6),
  REGION_GET_STATUS(7),
  REGION_SET_OBSERVED(8),
  REGION_SET_DOWNLOAD_STATE(9),
  REGION_INVALIDATE(10),
  REGION_DELETE(11),
  UNKNOWN(-1);

  public companion object {
    internal fun fromNative(nativeValue: UInt): OfflineOperationKind =
      fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): OfflineOperationKind =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
