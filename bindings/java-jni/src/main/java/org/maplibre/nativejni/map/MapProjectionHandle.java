package org.maplibre.nativejni.map;

import java.util.List;
import java.util.Objects;
import org.maplibre.nativejni.camera.CameraOptions;
import org.maplibre.nativejni.camera.EdgeInsets;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.ScreenPoint;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.internal.bridge.ProjectionNative;
import org.maplibre.nativejni.internal.lifecycle.HandleState;
import org.maplibre.nativejni.internal.loader.NativeLibrary;
import org.maplibre.nativejni.internal.status.Status;

/** API-parity scaffold for the Java JNI binding. */
public final class MapProjectionHandle implements AutoCloseable {
  private final HandleState state;

  private MapProjectionHandle(long handle) {
    this.state = new HandleState("MapProjectionHandle", handle);
  }

  private static UnsupportedOperationException unsupported() {
    return new UnsupportedOperationException(
        "MapProjectionHandle is not implemented by the JNI bridge yet");
  }

  public static MapProjectionHandle create(MapHandle map) {
    Objects.requireNonNull(map, "map");
    NativeLibrary.ensureLoaded();
    var outProjection = new long[1];
    Status.check(
        ProjectionNative.mln_map_projection_create(
            map.nativeAddress(InternalAccess.INSTANCE), outProjection));
    return new MapProjectionHandle(outProjection[0]);
  }

  public CameraOptions camera() {
    throw unsupported();
  }

  public void setCamera(CameraOptions camera) {
    throw unsupported();
  }

  public void setVisibleCoordinates(List<LatLng> coordinates, EdgeInsets padding) {
    throw unsupported();
  }

  public void setVisibleGeometry(Geometry geometry, EdgeInsets padding) {
    throw unsupported();
  }

  public ScreenPoint pixelForLatLng(LatLng coordinate) {
    throw unsupported();
  }

  public LatLng latLngForPixel(ScreenPoint point) {
    throw unsupported();
  }

  public void close() {
    state.closeOnce(ProjectionNative::mln_map_projection_destroy);
  }

  public boolean isClosed() {
    return state.isReleased();
  }
}
