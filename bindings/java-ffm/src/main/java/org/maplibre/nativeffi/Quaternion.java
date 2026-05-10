package org.maplibre.nativeffi;

/** Quaternion stored as x, y, z, w components. */
public record Quaternion(double x, double y, double z, double w) {
  public Quaternion {
    requireFinite(x, "x");
    requireFinite(y, "y");
    requireFinite(z, "z");
    requireFinite(w, "w");
    if (x == 0.0 && y == 0.0 && z == 0.0 && w == 0.0) {
      throw new IllegalArgumentException("quaternion must not be zero length");
    }
  }

  private static void requireFinite(double value, String name) {
    if (!Double.isFinite(value)) {
      throw new IllegalArgumentException(name + " must be finite");
    }
  }
}
