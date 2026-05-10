package org.maplibre.nativeffi;

/** Tile URL coordinate scheme for vector, raster, and raster DEM sources. */
public enum StyleTileScheme {
  XYZ(0),
  TMS(1);

  private final int nativeValue;

  StyleTileScheme(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }
}
