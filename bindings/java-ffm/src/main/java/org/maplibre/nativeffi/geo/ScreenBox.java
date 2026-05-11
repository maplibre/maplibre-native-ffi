package org.maplibre.nativeffi.geo;

import java.util.Objects;

/** Screen-space box in logical map pixels. */
public record ScreenBox(ScreenPoint min, ScreenPoint max) {
  public ScreenBox {
    Objects.requireNonNull(min, "min");
    Objects.requireNonNull(max, "max");
  }
}
