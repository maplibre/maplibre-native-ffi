package org.maplibre.nativeffi;

/** DEM raster encoding for raster DEM style sources. */
public enum StyleRasterDemEncoding {
  MAPBOX(0),
  TERRARIUM(1);

  private final int nativeValue;

  StyleRasterDemEncoding(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }
}
