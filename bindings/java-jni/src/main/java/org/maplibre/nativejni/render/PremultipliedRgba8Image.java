package org.maplibre.nativejni.render;

import java.util.Arrays;
import java.util.Objects;

/** Caller-owned premultiplied RGBA8 pixels. */
public record PremultipliedRgba8Image(int width, int height, int stride, byte[] pixels) {
  public PremultipliedRgba8Image {
    Objects.requireNonNull(pixels, "pixels");
    if (width <= 0 || height <= 0) {
      throw new IllegalArgumentException("width and height must be positive");
    }
    if (stride < 0) {
      throw new IllegalArgumentException("stride must be non-negative");
    }
    var rowBytes = Math.multiplyExact((long) width, 4L);
    if (stride < rowBytes) {
      throw new IllegalArgumentException("stride must be at least width * 4");
    }
    var requiredBytes =
        height == 1
            ? rowBytes
            : Math.addExact(Math.multiplyExact((long) height - 1L, stride), rowBytes);
    if (pixels.length < requiredBytes) {
      throw new IllegalArgumentException("pixels length must include every row's pixel bytes");
    }
    pixels = pixels.clone();
  }

  public byte[] pixels() {
    return pixels.clone();
  }

  byte[] unsafePixels() {
    return pixels;
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof PremultipliedRgba8Image that
        && width == that.width
        && height == that.height
        && stride == that.stride
        && Arrays.equals(pixels, that.pixels);
  }

  @Override
  public int hashCode() {
    var result = Integer.hashCode(width);
    result = 31 * result + Integer.hashCode(height);
    result = 31 * result + Integer.hashCode(stride);
    result = 31 * result + Arrays.hashCode(pixels);
    return result;
  }

  @Override
  public String toString() {
    return "PremultipliedRgba8Image[width="
        + width
        + ", height="
        + height
        + ", stride="
        + stride
        + ", pixels="
        + pixels.length
        + " bytes]";
  }
}
