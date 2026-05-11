package org.maplibre.nativeffi.geo;

import java.util.Objects;

/** Geographic bounds in degrees. */
public record LatLngBounds(LatLng southwest, LatLng northeast) {
  public LatLngBounds {
    Objects.requireNonNull(southwest, "southwest");
    Objects.requireNonNull(northeast, "northeast");
  }
}
