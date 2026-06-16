package org.maplibre.nativeffi.style;

/** Image-name property slots for location indicator layers. */
public enum LocationIndicatorImageKind {
  TOP(0),
  BEARING(1),
  SHADOW(2);

  private final int nativeValue;

  LocationIndicatorImageKind(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  int nativeValue() {
    return nativeValue;
  }
}
