package org.maplibre.nativejni.style;

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

  public StyleImageOptions pixelRatio(float pixelRatio) {
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

  public StyleImageOptions sdf(boolean sdf) {
    this.sdf = sdf;
    return this;
  }

  public StyleImageOptions clearSdf() {
    sdf = null;
    return this;
  }
}
