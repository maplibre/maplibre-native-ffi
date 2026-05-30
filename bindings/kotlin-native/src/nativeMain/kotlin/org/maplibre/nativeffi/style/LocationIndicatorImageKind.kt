package org.maplibre.nativeffi.style

/** Image-name property slots for location indicator layers. */
public enum class LocationIndicatorImageKind(public val nativeValue: Int) {
  TOP(0),
  BEARING(1),
  SHADOW(2),
}
