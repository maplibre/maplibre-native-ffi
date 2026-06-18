package org.maplibre.nativejni.map;

/** Constraint mode used by map viewport options. */
public final class ConstrainMode {
  public static final ConstrainMode NONE = new ConstrainMode(0, "NONE");
  public static final ConstrainMode HEIGHT_ONLY = new ConstrainMode(1, "HEIGHT_ONLY");
  public static final ConstrainMode WIDTH_AND_HEIGHT = new ConstrainMode(2, "WIDTH_AND_HEIGHT");
  public static final ConstrainMode SCREEN = new ConstrainMode(3, "SCREEN");

  private final int nativeValue;
  private final String name;

  private ConstrainMode(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    if (name == null) {
      throw new IllegalArgumentException("Unknown constrain mode cannot be used as an input");
    }
    return nativeValue;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static ConstrainMode fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> NONE;
      case 1 -> HEIGHT_ONLY;
      case 2 -> WIDTH_AND_HEIGHT;
      case 3 -> SCREEN;
      default -> new ConstrainMode(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof ConstrainMode value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "ConstrainMode(" + nativeValue + ")";
  }
}
