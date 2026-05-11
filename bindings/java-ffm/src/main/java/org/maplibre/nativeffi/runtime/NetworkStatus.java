package org.maplibre.nativeffi.runtime;

import org.maplibre.nativeffi.error.NativeErrorException;

/** Process-global network reachability state used by Maplibre Native. */
public enum NetworkStatus {
  ONLINE(1),
  OFFLINE(2);

  private final int nativeValue;

  NetworkStatus(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public static NetworkStatus fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 1 -> ONLINE;
      case 2 -> OFFLINE;
      default -> throw new NativeErrorException(0, "Unknown native network status: " + nativeValue);
    };
  }
}
