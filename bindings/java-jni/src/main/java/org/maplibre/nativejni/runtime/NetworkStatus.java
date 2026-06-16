package org.maplibre.nativejni.runtime;

/** Process-global network reachability state used by Maplibre Native. */
public final class NetworkStatus {
  public static final NetworkStatus ONLINE = new NetworkStatus(1, "ONLINE");
  public static final NetworkStatus OFFLINE = new NetworkStatus(2, "OFFLINE");

  private final int nativeValue;
  private final String name;

  private NetworkStatus(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    if (name == null) {
      throw new IllegalArgumentException("Unknown network status cannot be used as an input");
    }
    return nativeValue;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static NetworkStatus fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 1 -> ONLINE;
      case 2 -> OFFLINE;
      default -> new NetworkStatus(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof NetworkStatus value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "NetworkStatus(" + nativeValue + ")";
  }
}
