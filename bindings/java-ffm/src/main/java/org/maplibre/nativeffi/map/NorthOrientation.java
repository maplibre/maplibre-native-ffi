package org.maplibre.nativeffi.map;

/** North orientation used by map viewport options. */
public enum NorthOrientation {
  UP(0),
  RIGHT(1),
  DOWN(2),
  LEFT(3),
  UNKNOWN(-1);

  private final int nativeValue;

  NorthOrientation(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public static NorthOrientation fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> UP;
      case 1 -> RIGHT;
      case 2 -> DOWN;
      case 3 -> LEFT;
      default -> UNKNOWN;
    };
  }
}
