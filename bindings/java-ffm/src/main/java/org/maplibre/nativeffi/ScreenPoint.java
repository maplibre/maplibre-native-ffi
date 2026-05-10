package org.maplibre.nativeffi;

/** Screen-space point in logical map pixels. */
public record ScreenPoint(double x, double y) {
  public ScreenPoint {
    requireFinite(x, "x");
    requireFinite(y, "y");
  }

  private static void requireFinite(double value, String name) {
    if (!Double.isFinite(value)) {
      throw new IllegalArgumentException(name + " must be finite");
    }
  }
}
