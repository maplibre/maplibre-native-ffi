package org.maplibre.nativeffi;

/** Screen-space insets in logical map pixels. */
public record EdgeInsets(double top, double left, double bottom, double right) {
  public static final EdgeInsets ZERO = new EdgeInsets(0, 0, 0, 0);

  public EdgeInsets {
    requireNonNegativeFinite(top, "top");
    requireNonNegativeFinite(left, "left");
    requireNonNegativeFinite(bottom, "bottom");
    requireNonNegativeFinite(right, "right");
  }

  private static void requireNonNegativeFinite(double value, String name) {
    if (!Double.isFinite(value) || value < 0) {
      throw new IllegalArgumentException(name + " must be finite and non-negative");
    }
  }
}
