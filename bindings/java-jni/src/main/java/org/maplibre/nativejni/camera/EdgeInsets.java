package org.maplibre.nativejni.camera;

/** Screen-space insets in logical map pixels. */
public record EdgeInsets(double top, double left, double bottom, double right) {
  public static final EdgeInsets ZERO = new EdgeInsets(0, 0, 0, 0);
}
