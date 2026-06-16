package org.maplibre.nativeffi.map;

/** North orientation used by map viewport options. */
public final class NorthOrientation {
  public static final NorthOrientation UP = new NorthOrientation(0);
  public static final NorthOrientation RIGHT = new NorthOrientation(1);
  public static final NorthOrientation DOWN = new NorthOrientation(2);
  public static final NorthOrientation LEFT = new NorthOrientation(3);

  private final int rawValue;
  private final String name;

  public NorthOrientation(int rawValue) {
    this.rawValue = rawValue;
    this.name =
        switch (rawValue) {
          case 0 -> "UP";
          case 1 -> "RIGHT";
          case 2 -> "DOWN";
          case 3 -> "LEFT";
          default -> "UNKNOWN(" + Integer.toUnsignedLong(rawValue) + ")";
        };
  }

  public int rawValue() {
    return rawValue;
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof NorthOrientation that && rawValue == that.rawValue;
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
