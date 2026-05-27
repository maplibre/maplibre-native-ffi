package org.maplibre.nativejni.resource;

/** Resource storage policy copied from a native resource request. */
public enum ResourceStoragePolicy {
  PERMANENT(0),
  VOLATILE(1),
  UNKNOWN(-1);

  private final int nativeValue;

  ResourceStoragePolicy(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public static ResourceStoragePolicy fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> PERMANENT;
      case 1 -> VOLATILE;
      default -> UNKNOWN;
    };
  }
}
