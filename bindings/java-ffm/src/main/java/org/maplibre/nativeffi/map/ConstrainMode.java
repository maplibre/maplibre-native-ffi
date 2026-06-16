package org.maplibre.nativeffi.map;

/** Constraint mode used by map viewport options. */
public final class ConstrainMode {
  public static final ConstrainMode NONE = new ConstrainMode(0);
  public static final ConstrainMode HEIGHT_ONLY = new ConstrainMode(1);
  public static final ConstrainMode WIDTH_AND_HEIGHT = new ConstrainMode(2);
  public static final ConstrainMode SCREEN = new ConstrainMode(3);

  private final int rawValue;
  private final String name;

  public ConstrainMode(int rawValue) {
    this.rawValue = rawValue;
    this.name =
        switch (rawValue) {
          case 0 -> "NONE";
          case 1 -> "HEIGHT_ONLY";
          case 2 -> "WIDTH_AND_HEIGHT";
          case 3 -> "SCREEN";
          default -> "UNKNOWN(" + Integer.toUnsignedLong(rawValue) + ")";
        };
  }

  public int rawValue() {
    return rawValue;
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof ConstrainMode that && rawValue == that.rawValue;
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
