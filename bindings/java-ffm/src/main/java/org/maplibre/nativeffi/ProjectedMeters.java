package org.maplibre.nativeffi;

/** Spherical Mercator coordinate in projected meters. */
public record ProjectedMeters(double northing, double easting) {
  public ProjectedMeters {
    requireFinite(northing, "northing");
    requireFinite(easting, "easting");
  }

  private static void requireFinite(double value, String name) {
    if (!Double.isFinite(value)) {
      throw new IllegalArgumentException(name + " must be finite");
    }
  }
}
