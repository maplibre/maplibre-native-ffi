package org.maplibre.nativeffi.style

/** Image-name property slots for location indicator layers. */
public enum class LocationIndicatorImageKind(internal val nativeValue: UInt) {
  TOP(0U),
  BEARING(1U),
  SHADOW(2U),
}
