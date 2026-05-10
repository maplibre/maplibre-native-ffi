package org.maplibre.nativeffi;

/** Constraint mode used by map viewport options. */
public enum ConstrainMode {
  NONE(0),
  HEIGHT_ONLY(1),
  WIDTH_AND_HEIGHT(2),
  SCREEN(3),
  UNKNOWN(-1);

  private final int nativeValue;

  ConstrainMode(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    if (this == UNKNOWN) {
      throw new IllegalArgumentException("UNKNOWN constrain mode cannot be used as an input");
    }
    return nativeValue;
  }

  public static ConstrainMode fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> NONE;
      case 1 -> HEIGHT_ONLY;
      case 2 -> WIDTH_AND_HEIGHT;
      case 3 -> SCREEN;
      default -> UNKNOWN;
    };
  }
}
