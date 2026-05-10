package org.maplibre.nativeffi;

/** Cubic unit-bezier easing curve. */
public record UnitBezier(double x1, double y1, double x2, double y2) {
  public UnitBezier {
    requireFinite(x1, "x1");
    requireFinite(y1, "y1");
    requireFinite(x2, "x2");
    requireFinite(y2, "y2");
  }

  private static void requireFinite(double value, String name) {
    if (!Double.isFinite(value)) {
      throw new IllegalArgumentException(name + " must be finite");
    }
  }
}
