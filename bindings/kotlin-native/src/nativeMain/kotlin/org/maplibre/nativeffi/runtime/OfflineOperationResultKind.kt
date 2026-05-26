package org.maplibre.nativeffi.runtime

/** Offline operation result kind reported by completion events. */
public enum class OfflineOperationResultKind(public val nativeValue: Int) {
  NONE(0),
  REGION(1),
  OPTIONAL_REGION(2),
  REGION_LIST(3),
  REGION_STATUS(4),
  UNKNOWN(-1);

  public companion object {
    internal fun fromNative(nativeValue: UInt): OfflineOperationResultKind =
      fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): OfflineOperationResultKind =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
