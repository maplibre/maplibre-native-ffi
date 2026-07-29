package org.maplibre.nativeffi.runtime

import kotlin.jvm.JvmInline

/**
 * Process-global network reachability state used by Maplibre Native.
 *
 * This is an open domain: MapLibre Native may report a value that has no named constant here, so a
 * `when` over this type needs an `else` branch. Unknown values are preserved as their raw
 * [nativeValue] rather than collapsed to a known constant.
 */
@JvmInline
public value class NetworkStatus(public val nativeValue: Int) {
  public companion object {
    public val ONLINE: NetworkStatus = NetworkStatus(1)
    public val OFFLINE: NetworkStatus = NetworkStatus(2)

    internal fun fromNative(nativeValue: UInt): NetworkStatus = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): NetworkStatus = NetworkStatus(nativeValue)
  }

  internal val isKnown: Boolean
    get() = this == ONLINE || this == OFFLINE
}
