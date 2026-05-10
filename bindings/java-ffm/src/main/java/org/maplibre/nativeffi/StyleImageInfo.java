package org.maplibre.nativeffi;

/** Copied metadata for one runtime style image. */
public record StyleImageInfo(
    int width, int height, int stride, long byteLength, float pixelRatio, boolean sdf) {
  public StyleImageInfo {
    if (width < 0 || height < 0 || stride < 0 || byteLength < 0) {
      throw new IllegalArgumentException("image dimensions and byteLength must be non-negative");
    }
  }
}
