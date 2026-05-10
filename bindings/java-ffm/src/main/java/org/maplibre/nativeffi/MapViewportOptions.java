package org.maplibre.nativeffi;

import java.util.Objects;

/** Mutable descriptor for live map viewport and render-transform controls. */
public final class MapViewportOptions {
  private NorthOrientation northOrientation;
  private ConstrainMode constrainMode;
  private ViewportMode viewportMode;
  private EdgeInsets frustumOffset;

  public boolean hasNorthOrientation() {
    return northOrientation != null;
  }

  public NorthOrientation northOrientation() {
    return northOrientation;
  }

  public MapViewportOptions setNorthOrientation(NorthOrientation northOrientation) {
    this.northOrientation = Objects.requireNonNull(northOrientation, "northOrientation");
    return this;
  }

  public MapViewportOptions clearNorthOrientation() {
    northOrientation = null;
    return this;
  }

  public boolean hasConstrainMode() {
    return constrainMode != null;
  }

  public ConstrainMode constrainMode() {
    return constrainMode;
  }

  public MapViewportOptions setConstrainMode(ConstrainMode constrainMode) {
    this.constrainMode = Objects.requireNonNull(constrainMode, "constrainMode");
    return this;
  }

  public MapViewportOptions clearConstrainMode() {
    constrainMode = null;
    return this;
  }

  public boolean hasViewportMode() {
    return viewportMode != null;
  }

  public ViewportMode viewportMode() {
    return viewportMode;
  }

  public MapViewportOptions setViewportMode(ViewportMode viewportMode) {
    this.viewportMode = Objects.requireNonNull(viewportMode, "viewportMode");
    return this;
  }

  public MapViewportOptions clearViewportMode() {
    viewportMode = null;
    return this;
  }

  public boolean hasFrustumOffset() {
    return frustumOffset != null;
  }

  public EdgeInsets frustumOffset() {
    return frustumOffset;
  }

  public MapViewportOptions setFrustumOffset(EdgeInsets frustumOffset) {
    this.frustumOffset = Objects.requireNonNull(frustumOffset, "frustumOffset");
    return this;
  }

  public MapViewportOptions clearFrustumOffset() {
    frustumOffset = null;
    return this;
  }
}
