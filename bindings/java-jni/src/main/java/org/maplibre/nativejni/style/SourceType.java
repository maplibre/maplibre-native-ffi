package org.maplibre.nativejni.style;

/** Style source type values returned by native style source metadata. */
public final class SourceType {
  public static final SourceType UNKNOWN = new SourceType(0, "UNKNOWN");
  public static final SourceType VECTOR = new SourceType(1, "VECTOR");
  public static final SourceType RASTER = new SourceType(2, "RASTER");
  public static final SourceType RASTER_DEM = new SourceType(3, "RASTER_DEM");
  public static final SourceType GEOJSON = new SourceType(4, "GEOJSON");
  public static final SourceType IMAGE = new SourceType(5, "IMAGE");
  public static final SourceType VIDEO = new SourceType(6, "VIDEO");
  public static final SourceType ANNOTATIONS = new SourceType(7, "ANNOTATIONS");
  public static final SourceType CUSTOM_VECTOR = new SourceType(8, "CUSTOM_VECTOR");

  private final int nativeValue;
  private final String name;

  private SourceType(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static SourceType fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> UNKNOWN;
      case 1 -> VECTOR;
      case 2 -> RASTER;
      case 3 -> RASTER_DEM;
      case 4 -> GEOJSON;
      case 5 -> IMAGE;
      case 6 -> VIDEO;
      case 7 -> ANNOTATIONS;
      case 8 -> CUSTOM_VECTOR;
      default -> new SourceType(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof SourceType value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "SourceType(" + nativeValue + ")";
  }
}
