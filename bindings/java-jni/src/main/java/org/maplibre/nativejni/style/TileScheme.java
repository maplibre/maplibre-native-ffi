package org.maplibre.nativejni.style;

/** Tile URL coordinate scheme for vector, raster, and raster DEM sources. */
public enum TileScheme {
  XYZ(0),
  TMS(1);

  private final int nativeValue;

  TileScheme(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }
}
