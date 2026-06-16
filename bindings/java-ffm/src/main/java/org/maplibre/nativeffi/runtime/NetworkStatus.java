package org.maplibre.nativeffi.runtime;

/** Process-global network reachability state used by Maplibre Native. */
public final class NetworkStatus {
  public static final NetworkStatus ONLINE = new NetworkStatus(1);
  public static final NetworkStatus OFFLINE = new NetworkStatus(2);

  private final int rawValue;
  private final String name;

  public NetworkStatus(int rawValue) {
    this.rawValue = rawValue;
    this.name =
        switch (rawValue) {
          case 1 -> "ONLINE";
          case 2 -> "OFFLINE";
          default -> "UNKNOWN(" + Integer.toUnsignedLong(rawValue) + ")";
        };
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
