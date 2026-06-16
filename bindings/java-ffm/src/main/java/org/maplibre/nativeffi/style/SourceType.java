package org.maplibre.nativeffi.style;

/** Style source type value returned by native style source metadata. */
public final class SourceType {
  public static final SourceType UNKNOWN = new SourceType(0);
  public static final SourceType VECTOR = new SourceType(1);
  public static final SourceType RASTER = new SourceType(2);
  public static final SourceType RASTER_DEM = new SourceType(3);
  public static final SourceType GEOJSON = new SourceType(4);
  public static final SourceType IMAGE = new SourceType(5);
  public static final SourceType VIDEO = new SourceType(6);
  public static final SourceType ANNOTATIONS = new SourceType(7);
  public static final SourceType CUSTOM_VECTOR = new SourceType(8);

  private final int rawValue;
  private final String name;

  public SourceType(int rawValue) {
    this.rawValue = rawValue;
    this.name =
        switch (rawValue) {
          case 0 -> "UNKNOWN";
          case 1 -> "VECTOR";
          case 2 -> "RASTER";
          case 3 -> "RASTER_DEM";
          case 4 -> "GEOJSON";
          case 5 -> "IMAGE";
          case 6 -> "VIDEO";
          case 7 -> "ANNOTATIONS";
          case 8 -> "CUSTOM_VECTOR";
          default -> "UNKNOWN(" + Integer.toUnsignedLong(rawValue) + ")";
        };
  }

  public int rawValue() {
    return rawValue;
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof SourceType that && rawValue == that.rawValue;
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
