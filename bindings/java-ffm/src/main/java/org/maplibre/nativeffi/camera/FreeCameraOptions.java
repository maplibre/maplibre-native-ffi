package org.maplibre.nativeffi.camera;

import java.util.Objects;
import org.maplibre.nativeffi.geo.Quaternion;
import org.maplibre.nativeffi.geo.Vec3;

/** Mutable descriptor for free-camera position and orientation. */
public final class FreeCameraOptions {
  private Vec3 position;
  private Quaternion orientation;

  public boolean hasPosition() {
    return position != null;
  }

  public Vec3 position() {
    return position;
  }

  public FreeCameraOptions position(Vec3 position) {
    this.position = Objects.requireNonNull(position, "position");
    return this;
  }

  public FreeCameraOptions clearPosition() {
    position = null;
    return this;
  }

  public boolean hasOrientation() {
    return orientation != null;
  }

  public Quaternion orientation() {
    return orientation;
  }

  public FreeCameraOptions orientation(Quaternion orientation) {
    this.orientation = Objects.requireNonNull(orientation, "orientation");
    return this;
  }

  public FreeCameraOptions clearOrientation() {
    orientation = null;
    return this;
  }
}
