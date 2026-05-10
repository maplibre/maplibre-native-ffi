package org.maplibre.nativeffi;

import java.util.Objects;

/** Mutable descriptor for vector, raster, and raster DEM style tile sources. */
public final class StyleTileSourceOptions {
  private Double minZoom;
  private Double maxZoom;
  private String attribution;
  private StyleTileScheme scheme;
  private LatLngBounds bounds;
  private Integer tileSize;
  private StyleVectorTileEncoding vectorEncoding;
  private StyleRasterDemEncoding rasterDemEncoding;

  public boolean hasMinZoom() {
    return minZoom != null;
  }

  public Double minZoom() {
    return minZoom;
  }

  public StyleTileSourceOptions setMinZoom(double minZoom) {
    this.minZoom = requireZoom(minZoom, "minZoom");
    return this;
  }

  public StyleTileSourceOptions clearMinZoom() {
    minZoom = null;
    return this;
  }

  public boolean hasMaxZoom() {
    return maxZoom != null;
  }

  public Double maxZoom() {
    return maxZoom;
  }

  public StyleTileSourceOptions setMaxZoom(double maxZoom) {
    this.maxZoom = requireZoom(maxZoom, "maxZoom");
    return this;
  }

  public StyleTileSourceOptions clearMaxZoom() {
    maxZoom = null;
    return this;
  }

  public boolean hasAttribution() {
    return attribution != null;
  }

  public String attribution() {
    return attribution;
  }

  public StyleTileSourceOptions setAttribution(String attribution) {
    this.attribution = Objects.requireNonNull(attribution, "attribution");
    return this;
  }

  public StyleTileSourceOptions clearAttribution() {
    attribution = null;
    return this;
  }

  public boolean hasScheme() {
    return scheme != null;
  }

  public StyleTileScheme scheme() {
    return scheme;
  }

  public StyleTileSourceOptions setScheme(StyleTileScheme scheme) {
    this.scheme = Objects.requireNonNull(scheme, "scheme");
    return this;
  }

  public StyleTileSourceOptions clearScheme() {
    scheme = null;
    return this;
  }

  public boolean hasBounds() {
    return bounds != null;
  }

  public LatLngBounds bounds() {
    return bounds;
  }

  public StyleTileSourceOptions setBounds(LatLngBounds bounds) {
    this.bounds = Objects.requireNonNull(bounds, "bounds");
    return this;
  }

  public StyleTileSourceOptions clearBounds() {
    bounds = null;
    return this;
  }

  public boolean hasTileSize() {
    return tileSize != null;
  }

  public Integer tileSize() {
    return tileSize;
  }

  public StyleTileSourceOptions setTileSize(int tileSize) {
    if (tileSize < 1 || tileSize > 65535) {
      throw new IllegalArgumentException("tileSize must be in [1, 65535]");
    }
    this.tileSize = tileSize;
    return this;
  }

  public StyleTileSourceOptions clearTileSize() {
    tileSize = null;
    return this;
  }

  public boolean hasVectorEncoding() {
    return vectorEncoding != null;
  }

  public StyleVectorTileEncoding vectorEncoding() {
    return vectorEncoding;
  }

  public StyleTileSourceOptions setVectorEncoding(StyleVectorTileEncoding vectorEncoding) {
    this.vectorEncoding = Objects.requireNonNull(vectorEncoding, "vectorEncoding");
    return this;
  }

  public StyleTileSourceOptions clearVectorEncoding() {
    vectorEncoding = null;
    return this;
  }

  public boolean hasRasterDemEncoding() {
    return rasterDemEncoding != null;
  }

  public StyleRasterDemEncoding rasterDemEncoding() {
    return rasterDemEncoding;
  }

  public StyleTileSourceOptions setRasterDemEncoding(StyleRasterDemEncoding rasterDemEncoding) {
    this.rasterDemEncoding = Objects.requireNonNull(rasterDemEncoding, "rasterDemEncoding");
    return this;
  }

  public StyleTileSourceOptions clearRasterDemEncoding() {
    rasterDemEncoding = null;
    return this;
  }

  private static double requireZoom(double value, String name) {
    if (!Double.isFinite(value) || value < 0.0 || value > 255.0) {
      throw new IllegalArgumentException(name + " must be finite and in [0, 255]");
    }
    return value;
  }
}
