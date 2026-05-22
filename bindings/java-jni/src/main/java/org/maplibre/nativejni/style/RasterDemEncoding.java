package org.maplibre.nativejni.style;

/** DEM raster encoding for raster DEM style sources. */
public enum RasterDemEncoding {
  MAPBOX(0),
  TERRARIUM(1);

  private final int nativeValue;

  RasterDemEncoding(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }
}
