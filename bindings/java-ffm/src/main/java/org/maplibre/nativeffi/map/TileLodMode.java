package org.maplibre.nativeffi.map;

/** Tile level-of-detail algorithm used by map tile options. */
public final class TileLodMode {
  public static final TileLodMode DEFAULT = new TileLodMode(0);
  public static final TileLodMode DISTANCE = new TileLodMode(1);

  private final int rawValue;
  private final String name;

  public TileLodMode(int rawValue) {
    this.rawValue = rawValue;
    this.name =
        switch (rawValue) {
          case 0 -> "DEFAULT";
          case 1 -> "DISTANCE";
          default -> "UNKNOWN(" + Integer.toUnsignedLong(rawValue) + ")";
        };
  }

  public int rawValue() {
    return rawValue;
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof TileLodMode that && rawValue == that.rawValue;
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
