package org.maplibre.nativeffi;

/** Mutable descriptor for runtime style image options. */
public final class StyleImageOptions {
  private Float pixelRatio;
  private Boolean sdf;

  public boolean hasPixelRatio() {
    return pixelRatio != null;
  }

  public Float pixelRatio() {
    return pixelRatio;
  }

  public StyleImageOptions setPixelRatio(float pixelRatio) {
    if (!Float.isFinite(pixelRatio) || pixelRatio <= 0.0f) {
      throw new IllegalArgumentException("pixelRatio must be finite and positive");
    }
    this.pixelRatio = pixelRatio;
    return this;
  }

  public StyleImageOptions clearPixelRatio() {
    pixelRatio = null;
    return this;
  }

  public boolean hasSdf() {
    return sdf != null;
  }

  public Boolean sdf() {
    return sdf;
  }

  public StyleImageOptions setSdf(boolean sdf) {
    this.sdf = sdf;
    return this;
  }

  public StyleImageOptions clearSdf() {
    sdf = null;
    return this;
  }
}
