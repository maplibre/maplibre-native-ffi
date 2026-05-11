package org.maplibre.nativeffi.map;

/** Tile level-of-detail algorithm used by map tile options. */
public enum TileLodMode {
  DEFAULT(0),
  DISTANCE(1),
  UNKNOWN(-1);

  private final int nativeValue;

  TileLodMode(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public static TileLodMode fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> DEFAULT;
      case 1 -> DISTANCE;
      default -> UNKNOWN;
    };
  }
}
