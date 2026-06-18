package org.maplibre.nativejni.map;

import java.util.Objects;

/** Mutable descriptor for tile prefetch and level-of-detail controls. */
public final class TileOptions {
  private Integer prefetchZoomDelta;
  private Double lodMinRadius;
  private Double lodScale;
  private Double lodPitchThreshold;
  private Double lodZoomShift;
  private TileLodMode lodMode;

  public boolean hasPrefetchZoomDelta() {
    return prefetchZoomDelta != null;
  }

  public Integer prefetchZoomDelta() {
    return prefetchZoomDelta;
  }

  public TileOptions prefetchZoomDelta(int prefetchZoomDelta) {
    this.prefetchZoomDelta = prefetchZoomDelta;
    return this;
  }

  public TileOptions clearPrefetchZoomDelta() {
    prefetchZoomDelta = null;
    return this;
  }

  public boolean hasLodMinRadius() {
    return lodMinRadius != null;
  }

  public Double lodMinRadius() {
    return lodMinRadius;
  }

  public TileOptions lodMinRadius(double lodMinRadius) {
    this.lodMinRadius = lodMinRadius;
    return this;
  }

  public TileOptions clearLodMinRadius() {
    lodMinRadius = null;
    return this;
  }

  public boolean hasLodScale() {
    return lodScale != null;
  }

  public Double lodScale() {
    return lodScale;
  }

  public TileOptions lodScale(double lodScale) {
    this.lodScale = lodScale;
    return this;
  }

  public TileOptions clearLodScale() {
    lodScale = null;
    return this;
  }

  public boolean hasLodPitchThreshold() {
    return lodPitchThreshold != null;
  }

  public Double lodPitchThreshold() {
    return lodPitchThreshold;
  }

  public TileOptions lodPitchThreshold(double lodPitchThreshold) {
    this.lodPitchThreshold = lodPitchThreshold;
    return this;
  }

  public TileOptions clearLodPitchThreshold() {
    lodPitchThreshold = null;
    return this;
  }

  public boolean hasLodZoomShift() {
    return lodZoomShift != null;
  }

  public Double lodZoomShift() {
    return lodZoomShift;
  }

  public TileOptions lodZoomShift(double lodZoomShift) {
    this.lodZoomShift = lodZoomShift;
    return this;
  }

  public TileOptions clearLodZoomShift() {
    lodZoomShift = null;
    return this;
  }

  public boolean hasLodMode() {
    return lodMode != null;
  }

  public TileLodMode lodMode() {
    return lodMode;
  }

  public TileOptions lodMode(TileLodMode lodMode) {
    var value = Objects.requireNonNull(lodMode, "lodMode");
    value.nativeValue();
    this.lodMode = value;
    return this;
  }

  public TileOptions clearLodMode() {
    lodMode = null;
    return this;
  }
}
