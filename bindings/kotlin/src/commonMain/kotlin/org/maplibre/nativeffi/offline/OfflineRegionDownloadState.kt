package org.maplibre.nativeffi.offline

import kotlin.jvm.JvmInline

/**
 * Offline region download state.
 *
 * This is an open domain: a value may have no named constant here, so a `when` over this type needs
 * an `else` branch. Unknown values keep their raw [nativeValue].
 */
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
