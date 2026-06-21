package org.maplibre.nativeffi.runtime

import kotlin.jvm.JvmInline

/** Offline operation result kind reported by completion events. */
@JvmInline
public value class OfflineOperationResultKind(public val nativeValue: Int) {
  public companion object {
    public val NONE: OfflineOperationResultKind = OfflineOperationResultKind(0)
    public val REGION: OfflineOperationResultKind = OfflineOperationResultKind(1)
    public val OPTIONAL_REGION: OfflineOperationResultKind = OfflineOperationResultKind(2)
    public val REGION_LIST: OfflineOperationResultKind = OfflineOperationResultKind(3)
    public val REGION_STATUS: OfflineOperationResultKind = OfflineOperationResultKind(4)

    internal fun fromNative(nativeValue: UInt): OfflineOperationResultKind =
      fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): OfflineOperationResultKind =
      OfflineOperationResultKind(nativeValue)
  }
}
