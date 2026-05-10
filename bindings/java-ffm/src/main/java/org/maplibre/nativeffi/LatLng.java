package org.maplibre.nativeffi;

/** Geographic coordinate in degrees. */
public record LatLng(double latitude, double longitude) {
  public LatLng {
    requireFinite(latitude, "latitude");
    requireFinite(longitude, "longitude");
    if (latitude < -90.0 || latitude > 90.0) {
      throw new IllegalArgumentException("latitude must be in [-90, 90]");
    }
  }

  private static void requireFinite(double value, String name) {
    if (!Double.isFinite(value)) {
      throw new IllegalArgumentException(name + " must be finite");
    }
  }
}
