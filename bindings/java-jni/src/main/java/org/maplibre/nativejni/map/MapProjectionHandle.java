package org.maplibre.nativejni.map;

import java.util.List;
import java.util.Objects;
import org.maplibre.nativejni.camera.CameraOptions;
import org.maplibre.nativejni.camera.EdgeInsets;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.ScreenPoint;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.JavaCppValues;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.lifecycle.HandleState;
import org.maplibre.nativejni.internal.loader.NativeLibrary;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.internal.struct.CoreStructs;
import org.maplibre.nativejni.internal.struct.MapStructs;

/** Owned standalone projection snapshot created from a map. */
public final class MapProjectionHandle implements AutoCloseable {
  private final HandleState state;

  private MapProjectionHandle(long handle) {
    this.state = new HandleState("MapProjectionHandle", handle);
  }

  public static MapProjectionHandle create(MapHandle map) {
    Objects.requireNonNull(map, "map");
    NativeLibrary.ensureLoaded();
    var outProjection = JavaCppSupport.outPointer(MaplibreNativeC.mln_map_projection.class);
    Status.check(
        MaplibreNativeC.mln_map_projection_create(
            JavaCppSupport.map(map.nativeAddress(InternalAccess.INSTANCE)), outProjection));
    return new MapProjectionHandle(
        JavaCppSupport.outAddress(outProjection, MaplibreNativeC.mln_map_projection.class));
  }

  public CameraOptions camera() {
    NativeLibrary.ensureLoaded();
    var outCamera = MaplibreNativeC.mln_camera_options_default();
    Status.check(
        MaplibreNativeC.mln_map_projection_get_camera(
            JavaCppSupport.projection(state.requireLiveAddress()), outCamera));
    return MapStructs.cameraOptions(outCamera);
  }

  public void setCamera(CameraOptions camera) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(camera, "camera");
    var nativeCamera = MapStructs.nativeCameraOptions(camera);
    try {
      Status.check(
          MaplibreNativeC.mln_map_projection_set_camera(
              JavaCppSupport.projection(state.requireLiveAddress()), nativeCamera));
    } finally {
      nativeCamera.close();
    }
  }

  public void setVisibleCoordinates(List<LatLng> coordinates, EdgeInsets padding) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(coordinates, "coordinates");
    Objects.requireNonNull(padding, "padding");
    var copiedCoordinates = List.copyOf(coordinates);
    try (var nativeCoordinates = CoreStructs.latLngArray(copiedCoordinates);
        var nativePadding = CoreStructs.nativeEdgeInsets(padding)) {
      Status.check(
          MaplibreNativeC.mln_map_projection_set_visible_coordinates(
              JavaCppSupport.projection(state.requireLiveAddress()),
              nativeCoordinates.coordinates(),
              nativeCoordinates.count(),
              nativePadding));
    }
  }

  public void setVisibleGeometry(Geometry geometry, EdgeInsets padding) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(geometry, "geometry");
    Objects.requireNonNull(padding, "padding");
    try (var nativeGeometry = JavaCppValues.geometry(geometry);
        var nativePadding = CoreStructs.nativeEdgeInsets(padding)) {
      Status.check(
          MaplibreNativeC.mln_map_projection_set_visible_geometry(
              JavaCppSupport.projection(state.requireLiveAddress()),
              nativeGeometry.value(),
              nativePadding));
    }
  }

  public ScreenPoint pixelForLatLng(LatLng coordinate) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(coordinate, "coordinate");
    try (var nativeCoordinate = CoreStructs.nativeLatLng(coordinate)) {
      var outPoint = new MaplibreNativeC.mln_screen_point();
      Status.check(
          MaplibreNativeC.mln_map_projection_pixel_for_lat_lng(
              JavaCppSupport.projection(state.requireLiveAddress()), nativeCoordinate, outPoint));
      return CoreStructs.screenPoint(outPoint);
    }
  }

  public LatLng latLngForPixel(ScreenPoint point) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(point, "point");
    try (var nativePoint = CoreStructs.nativeScreenPoint(point)) {
      var outCoordinate = new MaplibreNativeC.mln_lat_lng();
      Status.check(
          MaplibreNativeC.mln_map_projection_lat_lng_for_pixel(
              JavaCppSupport.projection(state.requireLiveAddress()), nativePoint, outCoordinate));
      return CoreStructs.latLng(outCoordinate);
    }
  }

  public void close() {
    state.closeOnce(
        address -> MaplibreNativeC.mln_map_projection_destroy(JavaCppSupport.projection(address)));
  }

  public boolean isClosed() {
    return state.isReleased();
  }
}
