package org.maplibre.nativeffi.runtime

import kotlin.jvm.JvmInline

/** Process-global network reachability state used by Maplibre Native. */
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
