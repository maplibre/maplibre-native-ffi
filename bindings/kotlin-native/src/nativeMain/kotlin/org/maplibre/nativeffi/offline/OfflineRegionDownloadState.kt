package org.maplibre.nativeffi.offline

/** Offline region download state. */
public enum class OfflineRegionDownloadState(public val nativeValue: Int) {
  INACTIVE(0),
  ACTIVE(1),
  UNKNOWN(-1);

  public companion object {
    internal fun fromNative(nativeValue: UInt): OfflineRegionDownloadState =
      fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): OfflineRegionDownloadState =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
