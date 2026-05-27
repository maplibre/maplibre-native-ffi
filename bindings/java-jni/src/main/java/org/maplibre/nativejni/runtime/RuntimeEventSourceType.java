package org.maplibre.nativejni.runtime;

/** Source kind for a copied runtime event. */
public enum RuntimeEventSourceType {
  RUNTIME(0),
  MAP(1),
  UNKNOWN(-1);

  private final int nativeValue;

  RuntimeEventSourceType(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public static RuntimeEventSourceType fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> RUNTIME;
      case 1 -> MAP;
      default -> UNKNOWN;
    };
  }
}
