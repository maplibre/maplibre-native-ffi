package org.maplibre.nativeffi.runtime

import kotlin.jvm.JvmInline

/** Offline operation kind reported by completion events. */
@JvmInline
public value class OfflineOperationKind(public val nativeValue: Int) {
  public companion object {
    public val AMBIENT_CACHE: OfflineOperationKind = OfflineOperationKind(1)
    public val REGION_CREATE: OfflineOperationKind = OfflineOperationKind(2)
    public val REGION_GET: OfflineOperationKind = OfflineOperationKind(3)
    public val REGIONS_LIST: OfflineOperationKind = OfflineOperationKind(4)
    public val REGIONS_MERGE_DATABASE: OfflineOperationKind = OfflineOperationKind(5)
    public val REGION_UPDATE_METADATA: OfflineOperationKind = OfflineOperationKind(6)
    public val REGION_GET_STATUS: OfflineOperationKind = OfflineOperationKind(7)
    public val REGION_SET_OBSERVED: OfflineOperationKind = OfflineOperationKind(8)
    public val REGION_SET_DOWNLOAD_STATE: OfflineOperationKind = OfflineOperationKind(9)
    public val REGION_INVALIDATE: OfflineOperationKind = OfflineOperationKind(10)
    public val REGION_DELETE: OfflineOperationKind = OfflineOperationKind(11)

    internal fun fromNative(nativeValue: UInt): OfflineOperationKind =
      fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): OfflineOperationKind =
      OfflineOperationKind(nativeValue)
  }
}
