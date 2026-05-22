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
    NativeLibrary.ensureLoaded();
    var fields = new boolean[MapHandle.CAMERA_FIELD_COUNT];
    var values = new double[MapHandle.CAMERA_VALUE_COUNT];
    Status.check(
        ProjectionNative.mln_map_projection_get_camera(state.requireLiveAddress(), fields, values));
    return MapHandle.cameraFromNative(fields, values);
  }

  public void setCamera(CameraOptions camera) {
    NativeLibrary.ensureLoaded();
    var nativeCamera = MapHandle.cameraToNative(camera);
    Status.check(
        ProjectionNative.mln_map_projection_set_camera(
            state.requireLiveAddress(), nativeCamera.fields(), nativeCamera.values()));
  }

  public void setVisibleCoordinates(List<LatLng> coordinates, EdgeInsets padding) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(coordinates, "coordinates");
    Objects.requireNonNull(padding, "padding");
    var coordinateValues = new double[coordinates.size() * 2];
    for (var index = 0; index < coordinates.size(); index++) {
      var coordinate = Objects.requireNonNull(coordinates.get(index), "coordinate");
      coordinateValues[index * 2] = coordinate.latitude();
      coordinateValues[index * 2 + 1] = coordinate.longitude();
    }
    var paddingValues =
        new double[] {padding.top(), padding.left(), padding.bottom(), padding.right()};
    Status.check(
        ProjectionNative.mln_map_projection_set_visible_coordinates(
            state.requireLiveAddress(), coordinateValues, paddingValues));
  }

  public void setVisibleGeometry(Geometry geometry, EdgeInsets padding) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(geometry, "geometry");
    Objects.requireNonNull(padding, "padding");
    var paddingValues =
        new double[] {padding.top(), padding.left(), padding.bottom(), padding.right()};
    Status.check(
        ProjectionNative.mln_map_projection_set_visible_geometry(
            state.requireLiveAddress(), geometry, paddingValues));
  }

  public ScreenPoint pixelForLatLng(LatLng coordinate) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(coordinate, "coordinate");
    var outPoint = new double[2];
    Status.check(
        ProjectionNative.mln_map_projection_pixel_for_lat_lng(
            state.requireLiveAddress(), coordinate.latitude(), coordinate.longitude(), outPoint));
    return new ScreenPoint(outPoint[0], outPoint[1]);
  }

  public LatLng latLngForPixel(ScreenPoint point) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(point, "point");
    var outCoordinate = new double[2];
    Status.check(
        ProjectionNative.mln_map_projection_lat_lng_for_pixel(
            state.requireLiveAddress(), point.x(), point.y(), outCoordinate));
    return new LatLng(outCoordinate[0], outCoordinate[1]);
  }

  public void close() {
    state.closeOnce(ProjectionNative::mln_map_projection_destroy);
  }

  public boolean isClosed() {
    return state.isReleased();
  }
}
