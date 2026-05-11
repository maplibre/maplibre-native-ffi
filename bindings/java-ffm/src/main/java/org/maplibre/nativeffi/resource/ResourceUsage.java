package org.maplibre.nativeffi.resource;

/** Resource usage copied from a native resource request. */
public enum ResourceUsage {
  ONLINE(0),
  OFFLINE(1),
  UNKNOWN(-1);

  private final int nativeValue;

  ResourceUsage(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public static ResourceUsage fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> ONLINE;
      case 1 -> OFFLINE;
      default -> UNKNOWN;
    };
  }
}
