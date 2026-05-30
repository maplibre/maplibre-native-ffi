package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.error.NativeErrorException

/** Process-global network reachability state used by Maplibre Native. */
public enum class NetworkStatus(public val nativeValue: Int) {
  ONLINE(1),
  OFFLINE(2);

  public companion object {
    internal fun fromNative(nativeValue: UInt): NetworkStatus = fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): NetworkStatus =
      entries.firstOrNull { it.nativeValue == nativeValue }
        ?: throw NativeErrorException(0, "Unknown native network status: $nativeValue")
  }
}
