package org.maplibre.nativeffi.camera;

import java.util.Objects;
import org.maplibre.nativeffi.geo.LatLngBounds;

/** Mutable descriptor for map camera constraint options. */
public final class BoundOptions {
  private LatLngBounds bounds;
  private Double minZoom;
  private Double maxZoom;
  private Double minPitch;
  private Double maxPitch;

  public boolean hasBounds() {
    return bounds != null;
  }

  public LatLngBounds bounds() {
    return bounds;
  }

  public BoundOptions bounds(LatLngBounds bounds) {
    this.bounds = Objects.requireNonNull(bounds, "bounds");
    return this;
  }

  public BoundOptions clearBounds() {
    bounds = null;
    return this;
  }

  public boolean hasMinZoom() {
    return minZoom != null;
  }

  public Double minZoom() {
    return minZoom;
  }

  public BoundOptions minZoom(double minZoom) {
    this.minZoom = minZoom;
    return this;
  }

  public BoundOptions clearMinZoom() {
    minZoom = null;
    return this;
  }

  public boolean hasMaxZoom() {
    return maxZoom != null;
  }

  public Double maxZoom() {
    return maxZoom;
  }

  public BoundOptions maxZoom(double maxZoom) {
    this.maxZoom = maxZoom;
    return this;
  }

  public BoundOptions clearMaxZoom() {
    maxZoom = null;
    return this;
  }

  public boolean hasMinPitch() {
    return minPitch != null;
  }

  public Double minPitch() {
    return minPitch;
  }

  public BoundOptions minPitch(double minPitch) {
    this.minPitch = minPitch;
    return this;
  }

  public BoundOptions clearMinPitch() {
    minPitch = null;
    return this;
  }

  public boolean hasMaxPitch() {
    return maxPitch != null;
  }

  public Double maxPitch() {
    return maxPitch;
  }

  public BoundOptions maxPitch(double maxPitch) {
    this.maxPitch = maxPitch;
    return this;
  }

  public BoundOptions clearMaxPitch() {
    maxPitch = null;
    return this;
  }
}
