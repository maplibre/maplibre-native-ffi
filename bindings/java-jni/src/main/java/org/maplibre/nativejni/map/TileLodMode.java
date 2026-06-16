package org.maplibre.nativejni.map;

/** Tile level-of-detail algorithm used by map tile options. */
public final class TileLodMode {
  public static final TileLodMode DEFAULT = new TileLodMode(0, "DEFAULT");
  public static final TileLodMode DISTANCE = new TileLodMode(1, "DISTANCE");

  private final int nativeValue;
  private final String name;

  private TileLodMode(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    if (name == null) {
      throw new IllegalArgumentException("Unknown tile LOD mode cannot be used as an input");
    }
    return nativeValue;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static TileLodMode fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> DEFAULT;
      case 1 -> DISTANCE;
      default -> new TileLodMode(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof TileLodMode value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "TileLodMode(" + nativeValue + ")";
  }
}
