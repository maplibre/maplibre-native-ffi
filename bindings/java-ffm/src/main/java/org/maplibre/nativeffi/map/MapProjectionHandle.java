package org.maplibre.nativeffi.map;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.util.List;
import java.util.Objects;
import org.maplibre.nativeffi.camera.CameraOptions;
import org.maplibre.nativeffi.camera.EdgeInsets;
import org.maplibre.nativeffi.geo.Geometry;
import org.maplibre.nativeffi.geo.LatLng;
import org.maplibre.nativeffi.geo.ScreenPoint;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_lat_lng;
import org.maplibre.nativeffi.internal.c.mln_screen_point;
import org.maplibre.nativeffi.internal.lifecycle.HandleState;
import org.maplibre.nativeffi.internal.loader.NativeAccess;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.internal.status.Status;
import org.maplibre.nativeffi.internal.struct.CoreStructs;
import org.maplibre.nativeffi.internal.struct.MapStructs;
import org.maplibre.nativeffi.internal.struct.ValueStructs;

/** Owned standalone projection snapshot created from a map. */
public final class MapProjectionHandle implements AutoCloseable {
  private final HandleState state;

  private MapProjectionHandle(MemorySegment handle) {
    this.state = new HandleState("MapProjectionHandle", handle);
  }

  public static MapProjectionHandle create(MapHandle map) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(map, "map");
    try (var arena = Arena.ofConfined()) {
      var outProjection = MemoryUtil.allocatePointer(arena);
      Status.check(MapLibreNativeC.mln_map_projection_create(map.nativeHandle(), outProjection));
      return new MapProjectionHandle(outProjection.get(ValueLayout.ADDRESS, 0));
    }
  }

  public CameraOptions camera() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outCamera = MapLibreNativeC.mln_camera_options_default(arena);
      Status.check(MapLibreNativeC.mln_map_projection_get_camera(state.requireLive(), outCamera));
      return MapStructs.cameraOptions(outCamera);
    }
  }

  public void setCamera(CameraOptions camera) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(camera, "camera");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_projection_set_camera(
              state.requireLive(), MapStructs.cameraOptions(camera, arena)));
    }
  }

  public void setVisibleCoordinates(List<LatLng> coordinates, EdgeInsets padding) {
    NativeAccess.ensureLoaded();
    var copiedCoordinates = List.copyOf(Objects.requireNonNull(coordinates, "coordinates"));
    Objects.requireNonNull(padding, "padding");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_projection_set_visible_coordinates(
              state.requireLive(),
              copiedCoordinates.isEmpty()
                  ? MemorySegment.NULL
                  : CoreStructs.latLngArray(copiedCoordinates, arena),
              copiedCoordinates.size(),
              CoreStructs.edgeInsets(padding, arena)));
    }
  }

  public void setVisibleGeometry(Geometry geometry, EdgeInsets padding) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(geometry, "geometry");
    Objects.requireNonNull(padding, "padding");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_projection_set_visible_geometry(
              state.requireLive(),
              ValueStructs.geometry(geometry, arena),
              CoreStructs.edgeInsets(padding, arena)));
    }
  }

  public ScreenPoint pixelForLatLng(LatLng coordinate) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(coordinate, "coordinate");
    try (var arena = Arena.ofConfined()) {
      var outPoint = mln_screen_point.allocate(arena);
      Status.check(
          MapLibreNativeC.mln_map_projection_pixel_for_lat_lng(
              state.requireLive(), CoreStructs.latLng(coordinate, arena), outPoint));
      return CoreStructs.screenPoint(outPoint);
    }
  }

  public LatLng latLngForPixel(ScreenPoint point) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(point, "point");
    try (var arena = Arena.ofConfined()) {
      var outCoordinate = mln_lat_lng.allocate(arena);
      Status.check(
          MapLibreNativeC.mln_map_projection_lat_lng_for_pixel(
              state.requireLive(), CoreStructs.screenPoint(point, arena), outCoordinate));
      return CoreStructs.latLng(outCoordinate);
    }
  }

  @Override
  public void close() {
    NativeAccess.ensureLoaded();
    state.closeOnce(MapLibreNativeC::mln_map_projection_destroy);
  }

  public boolean isClosed() {
    return state.isReleased();
  }
}
