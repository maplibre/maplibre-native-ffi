package org.maplibre.nativejni.style;

import java.util.Objects;
import org.maplibre.nativejni.render.PremultipliedRgba8Image;

/** Copied runtime style image pixels with style image metadata. */
public record StyleImage(PremultipliedRgba8Image image, float pixelRatio, boolean sdf) {
  public StyleImage {
    Objects.requireNonNull(image, "image");
  }
}
