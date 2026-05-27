package org.maplibre.nativejni.camera;

import java.util.Objects;

/** Mutable descriptor for camera fitting queries. */
public final class CameraFitOptions {
  private EdgeInsets padding;
  private Double bearing;
  private Double pitch;

  public boolean hasPadding() {
    return padding != null;
  }

  public EdgeInsets padding() {
    return padding;
  }

  public CameraFitOptions padding(EdgeInsets padding) {
    this.padding = Objects.requireNonNull(padding, "padding");
    return this;
  }

  public CameraFitOptions clearPadding() {
    padding = null;
    return this;
  }

  public boolean hasBearing() {
    return bearing != null;
  }

  public Double bearing() {
    return bearing;
  }

  public CameraFitOptions bearing(double bearing) {
    this.bearing = bearing;
    return this;
  }

  public CameraFitOptions clearBearing() {
    bearing = null;
    return this;
  }

  public boolean hasPitch() {
    return pitch != null;
  }

  public Double pitch() {
    return pitch;
  }

  public CameraFitOptions pitch(double pitch) {
    this.pitch = pitch;
    return this;
  }

  public CameraFitOptions clearPitch() {
    pitch = null;
    return this;
  }
}
