package org.maplibre.nativeffi.offline

/** Offline region download state. */
public enum class OfflineRegionDownloadState(public val nativeValue: UInt) {
  INACTIVE(0U),
  ACTIVE(1U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): OfflineRegionDownloadState =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
