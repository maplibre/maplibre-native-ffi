package org.maplibre.nativeffi;

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

  public AnimationOptions setDurationMs(double durationMs) {
    if (!Double.isFinite(durationMs) || durationMs < 0) {
      throw new IllegalArgumentException("durationMs must be finite and non-negative");
    }
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

  public AnimationOptions setVelocity(double velocity) {
    if (!Double.isFinite(velocity) || velocity <= 0) {
      throw new IllegalArgumentException("velocity must be finite and positive");
    }
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

  public AnimationOptions setMinZoom(double minZoom) {
    this.minZoom = requireFinite(minZoom, "minZoom");
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

  public AnimationOptions setEasing(UnitBezier easing) {
    this.easing = Objects.requireNonNull(easing, "easing");
    return this;
  }

  public AnimationOptions clearEasing() {
    easing = null;
    return this;
  }

  private static double requireFinite(double value, String name) {
    if (!Double.isFinite(value)) {
      throw new IllegalArgumentException(name + " must be finite");
    }
    return value;
  }
}
