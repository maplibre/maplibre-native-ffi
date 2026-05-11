package org.maplibre.nativeffi.map;

import java.util.Objects;

/** Mutable descriptor used when creating a {@link MapHandle}. */
public final class MapOptions {
  private Integer width;
  private Integer height;
  private Double scaleFactor;
  private MapMode mapMode;

  public Integer width() {
    return width;
  }

  public Integer height() {
    return height;
  }

  public MapOptions setSize(int width, int height) {
    this.width = width;
    this.height = height;
    return this;
  }

  public Double scaleFactor() {
    return scaleFactor;
  }

  public MapOptions setScaleFactor(double scaleFactor) {
    this.scaleFactor = scaleFactor;
    return this;
  }

  public MapOptions clearScaleFactor() {
    scaleFactor = null;
    return this;
  }

  public MapMode mapMode() {
    return mapMode;
  }

  public MapOptions setMapMode(MapMode mapMode) {
    this.mapMode = Objects.requireNonNull(mapMode, "mapMode");
    return this;
  }

  public MapOptions clearMapMode() {
    mapMode = null;
    return this;
  }
}
