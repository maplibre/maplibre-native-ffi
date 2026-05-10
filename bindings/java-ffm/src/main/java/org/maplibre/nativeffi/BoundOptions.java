package org.maplibre.nativeffi;

import java.util.Objects;

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

  public BoundOptions setBounds(LatLngBounds bounds) {
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

  public BoundOptions setMinZoom(double minZoom) {
    this.minZoom = requireFinite(minZoom, "minZoom");
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

  public BoundOptions setMaxZoom(double maxZoom) {
    this.maxZoom = requireFinite(maxZoom, "maxZoom");
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

  public BoundOptions setMinPitch(double minPitch) {
    this.minPitch = requireFinite(minPitch, "minPitch");
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

  public BoundOptions setMaxPitch(double maxPitch) {
    this.maxPitch = requireFinite(maxPitch, "maxPitch");
    return this;
  }

  public BoundOptions clearMaxPitch() {
    maxPitch = null;
    return this;
  }

  private static double requireFinite(double value, String name) {
    if (!Double.isFinite(value)) {
      throw new IllegalArgumentException(name + " must be finite");
    }
    return value;
  }
}
