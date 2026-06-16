package org.maplibre.nativeffi.map;

/** Viewport orientation mode used by map viewport options. */
public final class ViewportMode {
  public static final ViewportMode DEFAULT = new ViewportMode(0);
  public static final ViewportMode FLIPPED_Y = new ViewportMode(1);

  private final int rawValue;
  private final String name;

  public ViewportMode(int rawValue) {
    this.rawValue = rawValue;
    this.name =
        switch (rawValue) {
          case 0 -> "DEFAULT";
          case 1 -> "FLIPPED_Y";
          default -> "UNKNOWN(" + Integer.toUnsignedLong(rawValue) + ")";
        };
  }

  public int rawValue() {
    return rawValue;
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof ViewportMode that && rawValue == that.rawValue;
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
