package org.maplibre.nativejni.style;

import java.util.Objects;
import org.maplibre.nativejni.geo.LatLngBounds;

/** Mutable descriptor for vector, raster, and raster DEM style tile sources. */
public final class TileSourceOptions {
  private Double minZoom;
  private Double maxZoom;
  private String attribution;
  private TileScheme scheme;
  private LatLngBounds bounds;
  private Integer tileSize;
  private VectorTileEncoding vectorEncoding;
  private RasterDemEncoding rasterDemEncoding;

  public boolean hasMinZoom() {
    return minZoom != null;
  }

  public Double minZoom() {
    return minZoom;
  }

  public TileSourceOptions minZoom(double minZoom) {
    this.minZoom = minZoom;
    return this;
  }

  public TileSourceOptions clearMinZoom() {
    minZoom = null;
    return this;
  }

  public boolean hasMaxZoom() {
    return maxZoom != null;
  }

  public Double maxZoom() {
    return maxZoom;
  }

  public TileSourceOptions maxZoom(double maxZoom) {
    this.maxZoom = maxZoom;
    return this;
  }

  public TileSourceOptions clearMaxZoom() {
    maxZoom = null;
    return this;
  }

  public boolean hasAttribution() {
    return attribution != null;
  }

  public String attribution() {
    return attribution;
  }

  public TileSourceOptions attribution(String attribution) {
    this.attribution = Objects.requireNonNull(attribution, "attribution");
    return this;
  }

  public TileSourceOptions clearAttribution() {
    attribution = null;
    return this;
  }

  public boolean hasScheme() {
    return scheme != null;
  }

  public TileScheme scheme() {
    return scheme;
  }

  public TileSourceOptions scheme(TileScheme scheme) {
    this.scheme = Objects.requireNonNull(scheme, "scheme");
    return this;
  }

  public TileSourceOptions clearScheme() {
    scheme = null;
    return this;
  }

  public boolean hasBounds() {
    return bounds != null;
  }

  public LatLngBounds bounds() {
    return bounds;
  }

  public TileSourceOptions bounds(LatLngBounds bounds) {
    this.bounds = Objects.requireNonNull(bounds, "bounds");
    return this;
  }

  public TileSourceOptions clearBounds() {
    bounds = null;
    return this;
  }

  public boolean hasTileSize() {
    return tileSize != null;
  }

  public Integer tileSize() {
    return tileSize;
  }

  public TileSourceOptions tileSize(int tileSize) {
    this.tileSize = tileSize;
    return this;
  }

  public TileSourceOptions clearTileSize() {
    tileSize = null;
    return this;
  }

  public boolean hasVectorEncoding() {
    return vectorEncoding != null;
  }

  public VectorTileEncoding vectorEncoding() {
    return vectorEncoding;
  }

  public TileSourceOptions vectorEncoding(VectorTileEncoding vectorEncoding) {
    this.vectorEncoding = Objects.requireNonNull(vectorEncoding, "vectorEncoding");
    return this;
  }

  public TileSourceOptions clearVectorEncoding() {
    vectorEncoding = null;
    return this;
  }

  public boolean hasRasterDemEncoding() {
    return rasterDemEncoding != null;
  }

  public RasterDemEncoding rasterDemEncoding() {
    return rasterDemEncoding;
  }

  public TileSourceOptions rasterDemEncoding(RasterDemEncoding rasterDemEncoding) {
    this.rasterDemEncoding = Objects.requireNonNull(rasterDemEncoding, "rasterDemEncoding");
    return this;
  }

  public TileSourceOptions clearRasterDemEncoding() {
    rasterDemEncoding = null;
    return this;
  }
}
