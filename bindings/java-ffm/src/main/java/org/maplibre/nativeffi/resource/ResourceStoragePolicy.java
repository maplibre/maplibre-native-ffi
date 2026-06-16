package org.maplibre.nativeffi.resource;

/** Resource storage policy copied from a native resource request. */
public enum ResourceStoragePolicy {
  PERMANENT(0),
  VOLATILE(1),
  UNKNOWN(-1);

  private final int nativeValue;

  ResourceStoragePolicy(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  int nativeValue() {
    return nativeValue;
  }

  static ResourceStoragePolicy fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> PERMANENT;
      case 1 -> VOLATILE;
      default -> UNKNOWN;
    };
  }
}
