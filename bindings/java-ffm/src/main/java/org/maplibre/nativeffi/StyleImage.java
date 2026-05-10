package org.maplibre.nativeffi;

import java.util.Objects;

/** Copied runtime style image pixels with style image metadata. */
public record StyleImage(PremultipliedRgba8Image image, float pixelRatio, boolean sdf) {
  public StyleImage {
    Objects.requireNonNull(image, "image");
    if (!Float.isFinite(pixelRatio) || pixelRatio <= 0.0f) {
      throw new IllegalArgumentException("pixelRatio must be finite and positive");
    }
  }
}
