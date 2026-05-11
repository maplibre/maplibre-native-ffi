package org.maplibre.nativeffi.style;

import java.util.Objects;

/** Mutable descriptor for custom geometry sources. */
public final class CustomGeometrySourceOptions {
  private final CustomGeometrySourceCallback callback;
  private Double minZoom;
  private Double maxZoom;
  private Double tolerance;
  private Integer tileSize;
  private Integer buffer;
  private Boolean clip;
  private Boolean wrap;

  public CustomGeometrySourceOptions(CustomGeometrySourceCallback callback) {
    this.callback = Objects.requireNonNull(callback, "callback");
  }

  public CustomGeometrySourceCallback callback() {
    return callback;
  }

  public boolean hasMinZoom() {
    return minZoom != null;
  }

  public Double minZoom() {
    return minZoom;
  }

  public CustomGeometrySourceOptions setMinZoom(double minZoom) {
    this.minZoom = minZoom;
    return this;
  }

  public CustomGeometrySourceOptions clearMinZoom() {
    minZoom = null;
    return this;
  }

  public boolean hasMaxZoom() {
    return maxZoom != null;
  }

  public Double maxZoom() {
    return maxZoom;
  }

  public CustomGeometrySourceOptions setMaxZoom(double maxZoom) {
    this.maxZoom = maxZoom;
    return this;
  }

  public CustomGeometrySourceOptions clearMaxZoom() {
    maxZoom = null;
    return this;
  }

  public boolean hasTolerance() {
    return tolerance != null;
  }

  public Double tolerance() {
    return tolerance;
  }

  public CustomGeometrySourceOptions setTolerance(double tolerance) {
    this.tolerance = tolerance;
    return this;
  }

  public CustomGeometrySourceOptions clearTolerance() {
    tolerance = null;
    return this;
  }

  public boolean hasTileSize() {
    return tileSize != null;
  }

  public Integer tileSize() {
    return tileSize;
  }

  public CustomGeometrySourceOptions setTileSize(int tileSize) {
    this.tileSize = tileSize;
    return this;
  }

  public CustomGeometrySourceOptions clearTileSize() {
    tileSize = null;
    return this;
  }

  public boolean hasBuffer() {
    return buffer != null;
  }

  public Integer buffer() {
    return buffer;
  }

  public CustomGeometrySourceOptions setBuffer(int buffer) {
    this.buffer = buffer;
    return this;
  }

  public CustomGeometrySourceOptions clearBuffer() {
    buffer = null;
    return this;
  }

  public boolean hasClip() {
    return clip != null;
  }

  public Boolean clip() {
    return clip;
  }

  public CustomGeometrySourceOptions setClip(boolean clip) {
    this.clip = clip;
    return this;
  }

  public CustomGeometrySourceOptions clearClip() {
    clip = null;
    return this;
  }

  public boolean hasWrap() {
    return wrap != null;
  }

  public Boolean wrap() {
    return wrap;
  }

  public CustomGeometrySourceOptions setWrap(boolean wrap) {
    this.wrap = wrap;
    return this;
  }

  public CustomGeometrySourceOptions clearWrap() {
    wrap = null;
    return this;
  }
}
