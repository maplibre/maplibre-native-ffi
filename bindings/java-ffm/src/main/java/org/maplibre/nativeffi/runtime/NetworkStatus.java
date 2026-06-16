package org.maplibre.nativeffi.runtime;

import org.maplibre.nativeffi.internal.c.MapLibreNativeC;

/** Process-global network reachability state used by Maplibre Native. */
public final class NetworkStatus {
  public static final NetworkStatus ONLINE =
      new NetworkStatus(MapLibreNativeC.MLN_NETWORK_STATUS_ONLINE());
  public static final NetworkStatus OFFLINE =
      new NetworkStatus(MapLibreNativeC.MLN_NETWORK_STATUS_OFFLINE());

  private final int rawValue;
  private final String name;

  public NetworkStatus(int rawValue) {
    this.rawValue = rawValue;
    if (rawValue == MapLibreNativeC.MLN_NETWORK_STATUS_ONLINE()) {
      name = "ONLINE";
    } else if (rawValue == MapLibreNativeC.MLN_NETWORK_STATUS_OFFLINE()) {
      name = "OFFLINE";
    } else {
      name = "UNKNOWN(" + Integer.toUnsignedLong(rawValue) + ")";
    }
  }

  public int rawValue() {
    return rawValue;
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof NetworkStatus that && rawValue == that.rawValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(rawValue);
  }

  @Override
  public String toString() {
    return name;
  }
}
