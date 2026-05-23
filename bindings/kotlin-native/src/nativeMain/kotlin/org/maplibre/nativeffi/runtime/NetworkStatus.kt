package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.error.NativeErrorException

/** Process-global network reachability state used by Maplibre Native. */
public enum class NetworkStatus(public val nativeValue: UInt) {
  ONLINE(1U),
  OFFLINE(2U);

  public companion object {
    public fun fromNative(nativeValue: UInt): NetworkStatus =
      entries.firstOrNull { it.nativeValue == nativeValue }
        ?: throw NativeErrorException(0, "Unknown native network status: $nativeValue")
  }
}
