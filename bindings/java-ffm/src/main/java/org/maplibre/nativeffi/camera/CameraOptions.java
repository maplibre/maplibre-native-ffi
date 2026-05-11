package org.maplibre.nativeffi.camera;

import java.util.Objects;
import org.maplibre.nativeffi.geo.LatLng;
import org.maplibre.nativeffi.geo.ScreenPoint;

/** Mutable camera descriptor used for camera snapshots and commands. */
public final class CameraOptions {
  private LatLng center;
  private Double centerAltitude;
  private EdgeInsets padding;
  private ScreenPoint anchor;
  private Double zoom;
  private Double bearing;
  private Double pitch;
  private Double roll;
  private Double fieldOfView;

  public boolean hasCenter() {
    return center != null;
  }

  public LatLng center() {
    return center;
  }

  public CameraOptions setCenter(LatLng center) {
    this.center = Objects.requireNonNull(center, "center");
    return this;
  }

  public CameraOptions setCenter(double latitude, double longitude) {
    return setCenter(new LatLng(latitude, longitude));
  }

  public CameraOptions clearCenter() {
    center = null;
    return this;
  }

  public boolean hasCenterAltitude() {
    return centerAltitude != null;
  }

  public Double centerAltitude() {
    return centerAltitude;
  }

  public CameraOptions setCenterAltitude(double centerAltitude) {
    this.centerAltitude = centerAltitude;
    return this;
  }

  public CameraOptions clearCenterAltitude() {
    centerAltitude = null;
    return this;
  }

  public boolean hasPadding() {
    return padding != null;
  }

  public EdgeInsets padding() {
    return padding;
  }

  public CameraOptions setPadding(EdgeInsets padding) {
    this.padding = Objects.requireNonNull(padding, "padding");
    return this;
  }

  public CameraOptions clearPadding() {
    padding = null;
    return this;
  }

  public boolean hasAnchor() {
    return anchor != null;
  }

  public ScreenPoint anchor() {
    return anchor;
  }

  public CameraOptions setAnchor(ScreenPoint anchor) {
    this.anchor = Objects.requireNonNull(anchor, "anchor");
    return this;
  }

  public CameraOptions clearAnchor() {
    anchor = null;
    return this;
  }

  public boolean hasZoom() {
    return zoom != null;
  }

  public Double zoom() {
    return zoom;
  }

  public CameraOptions setZoom(double zoom) {
    this.zoom = zoom;
    return this;
  }

  public CameraOptions clearZoom() {
    zoom = null;
    return this;
  }

  public boolean hasBearing() {
    return bearing != null;
  }

  public Double bearing() {
    return bearing;
  }

  public CameraOptions setBearing(double bearing) {
    this.bearing = bearing;
    return this;
  }

  public CameraOptions clearBearing() {
    bearing = null;
    return this;
  }

  public boolean hasPitch() {
    return pitch != null;
  }

  public Double pitch() {
    return pitch;
  }

  public CameraOptions setPitch(double pitch) {
    this.pitch = pitch;
    return this;
  }

  public CameraOptions clearPitch() {
    pitch = null;
    return this;
  }

  public boolean hasRoll() {
    return roll != null;
  }

  public Double roll() {
    return roll;
  }

  public CameraOptions setRoll(double roll) {
    this.roll = roll;
    return this;
  }

  public CameraOptions clearRoll() {
    roll = null;
    return this;
  }

  public boolean hasFieldOfView() {
    return fieldOfView != null;
  }

  public Double fieldOfView() {
    return fieldOfView;
  }

  public CameraOptions setFieldOfView(double fieldOfView) {
    this.fieldOfView = fieldOfView;
    return this;
  }

  public CameraOptions clearFieldOfView() {
    fieldOfView = null;
    return this;
  }
}
