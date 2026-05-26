package org.maplibre.nativejni.style;

/** Style source type values returned by native style source metadata. */
public enum SourceType {
  UNKNOWN(0),
  VECTOR(1),
  RASTER(2),
  RASTER_DEM(3),
  GEOJSON(4),
  IMAGE(5),
  VIDEO(6),
  ANNOTATIONS(7),
  CUSTOM_VECTOR(8);

  private final int nativeValue;

  SourceType(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public static SourceType fromNative(int nativeValue) {
    for (var value : values()) {
      if (value.nativeValue == nativeValue) {
        return value;
      }
    }
    return UNKNOWN;
  }
}
