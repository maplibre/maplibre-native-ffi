package org.maplibre.nativeffi.resource;

/** Resource loading method copied from a native resource request. */
public enum ResourceLoadingMethod {
  ALL(0),
  CACHE_ONLY(1),
  NETWORK_ONLY(2),
  UNKNOWN(-1);

  private final int nativeValue;

  ResourceLoadingMethod(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public static ResourceLoadingMethod fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> ALL;
      case 1 -> CACHE_ONLY;
      case 2 -> NETWORK_ONLY;
      default -> UNKNOWN;
    };
  }
}
