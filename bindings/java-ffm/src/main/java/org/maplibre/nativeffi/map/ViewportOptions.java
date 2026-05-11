package org.maplibre.nativeffi.map;

import java.util.Objects;
import org.maplibre.nativeffi.camera.EdgeInsets;

/** Mutable descriptor for live map viewport and render-transform controls. */
public final class ViewportOptions {
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

  public ViewportOptions setNorthOrientation(NorthOrientation northOrientation) {
    this.northOrientation = Objects.requireNonNull(northOrientation, "northOrientation");
    return this;
  }

  public ViewportOptions clearNorthOrientation() {
    northOrientation = null;
    return this;
  }

  public boolean hasConstrainMode() {
    return constrainMode != null;
  }

  public ConstrainMode constrainMode() {
    return constrainMode;
  }

  public ViewportOptions setConstrainMode(ConstrainMode constrainMode) {
    this.constrainMode = Objects.requireNonNull(constrainMode, "constrainMode");
    return this;
  }

  public ViewportOptions clearConstrainMode() {
    constrainMode = null;
    return this;
  }

  public boolean hasViewportMode() {
    return viewportMode != null;
  }

  public ViewportMode viewportMode() {
    return viewportMode;
  }

  public ViewportOptions setViewportMode(ViewportMode viewportMode) {
    this.viewportMode = Objects.requireNonNull(viewportMode, "viewportMode");
    return this;
  }

  public ViewportOptions clearViewportMode() {
    viewportMode = null;
    return this;
  }

  public boolean hasFrustumOffset() {
    return frustumOffset != null;
  }

  public EdgeInsets frustumOffset() {
    return frustumOffset;
  }

  public ViewportOptions setFrustumOffset(EdgeInsets frustumOffset) {
    this.frustumOffset = Objects.requireNonNull(frustumOffset, "frustumOffset");
    return this;
  }

  public ViewportOptions clearFrustumOffset() {
    frustumOffset = null;
    return this;
  }
}
