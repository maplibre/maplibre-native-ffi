package org.maplibre.nativejni.resource;

/** Resource request priority copied from a native resource request. */
public enum ResourcePriority {
  REGULAR(0),
  LOW(1),
  UNKNOWN(-1);

  private final int nativeValue;

  ResourcePriority(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public static ResourcePriority fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> REGULAR;
      case 1 -> LOW;
      default -> UNKNOWN;
    };
  }
}
