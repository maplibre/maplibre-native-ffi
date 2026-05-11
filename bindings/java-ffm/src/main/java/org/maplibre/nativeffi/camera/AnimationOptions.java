package org.maplibre.nativeffi.camera;

import java.util.Objects;

/** Mutable descriptor for animated camera transitions. */
public final class AnimationOptions {
  private Double durationMs;
  private Double velocity;
  private Double minZoom;
  private UnitBezier easing;

  public boolean hasDurationMs() {
    return durationMs != null;
  }

  public Double durationMs() {
    return durationMs;
  }

  public AnimationOptions durationMs(double durationMs) {
    this.durationMs = durationMs;
    return this;
  }

  public AnimationOptions clearDurationMs() {
    durationMs = null;
    return this;
  }

  public boolean hasVelocity() {
    return velocity != null;
  }

  public Double velocity() {
    return velocity;
  }

  public AnimationOptions velocity(double velocity) {
    this.velocity = velocity;
    return this;
  }

  public AnimationOptions clearVelocity() {
    velocity = null;
    return this;
  }

  public boolean hasMinZoom() {
    return minZoom != null;
  }

  public Double minZoom() {
    return minZoom;
  }

  public AnimationOptions minZoom(double minZoom) {
    this.minZoom = minZoom;
    return this;
  }

  public AnimationOptions clearMinZoom() {
    minZoom = null;
    return this;
  }

  public boolean hasEasing() {
    return easing != null;
  }

  public UnitBezier easing() {
    return easing;
  }

  public AnimationOptions easing(UnitBezier easing) {
    this.easing = Objects.requireNonNull(easing, "easing");
    return this;
  }

  public AnimationOptions clearEasing() {
    easing = null;
    return this;
  }
}
