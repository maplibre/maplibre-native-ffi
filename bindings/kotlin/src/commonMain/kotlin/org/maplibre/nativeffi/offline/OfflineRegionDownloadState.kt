package org.maplibre.nativeffi.offline

import kotlin.jvm.JvmInline

/** Offline region download state. */
@JvmInline
public value class OfflineRegionDownloadState(public val nativeValue: Int) {
  public companion object {
    public val INACTIVE: OfflineRegionDownloadState = OfflineRegionDownloadState(0)
    public val ACTIVE: OfflineRegionDownloadState = OfflineRegionDownloadState(1)

    internal fun fromNative(nativeValue: UInt): OfflineRegionDownloadState =
      fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): OfflineRegionDownloadState =
      OfflineRegionDownloadState(nativeValue)
  }

  internal val isKnown: Boolean
    get() = this == INACTIVE || this == ACTIVE
}
