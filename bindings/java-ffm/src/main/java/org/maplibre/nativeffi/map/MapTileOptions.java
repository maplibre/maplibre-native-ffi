package org.maplibre.nativeffi.map;

import java.util.Objects;

/** Mutable descriptor for tile prefetch and level-of-detail controls. */
public final class MapTileOptions {
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

  public MapTileOptions setPrefetchZoomDelta(int prefetchZoomDelta) {
    this.prefetchZoomDelta = prefetchZoomDelta;
    return this;
  }

  public MapTileOptions clearPrefetchZoomDelta() {
    prefetchZoomDelta = null;
    return this;
  }

  public boolean hasLodMinRadius() {
    return lodMinRadius != null;
  }

  public Double lodMinRadius() {
    return lodMinRadius;
  }

  public MapTileOptions setLodMinRadius(double lodMinRadius) {
    this.lodMinRadius = lodMinRadius;
    return this;
  }

  public MapTileOptions clearLodMinRadius() {
    lodMinRadius = null;
    return this;
  }

  public boolean hasLodScale() {
    return lodScale != null;
  }

  public Double lodScale() {
    return lodScale;
  }

  public MapTileOptions setLodScale(double lodScale) {
    this.lodScale = lodScale;
    return this;
  }

  public MapTileOptions clearLodScale() {
    lodScale = null;
    return this;
  }

  public boolean hasLodPitchThreshold() {
    return lodPitchThreshold != null;
  }

  public Double lodPitchThreshold() {
    return lodPitchThreshold;
  }

  public MapTileOptions setLodPitchThreshold(double lodPitchThreshold) {
    this.lodPitchThreshold = lodPitchThreshold;
    return this;
  }

  public MapTileOptions clearLodPitchThreshold() {
    lodPitchThreshold = null;
    return this;
  }

  public boolean hasLodZoomShift() {
    return lodZoomShift != null;
  }

  public Double lodZoomShift() {
    return lodZoomShift;
  }

  public MapTileOptions setLodZoomShift(double lodZoomShift) {
    this.lodZoomShift = lodZoomShift;
    return this;
  }

  public MapTileOptions clearLodZoomShift() {
    lodZoomShift = null;
    return this;
  }

  public boolean hasLodMode() {
    return lodMode != null;
  }

  public TileLodMode lodMode() {
    return lodMode;
  }

  public MapTileOptions setLodMode(TileLodMode lodMode) {
    this.lodMode = Objects.requireNonNull(lodMode, "lodMode");
    return this;
  }

  public MapTileOptions clearLodMode() {
    lodMode = null;
    return this;
  }
}
