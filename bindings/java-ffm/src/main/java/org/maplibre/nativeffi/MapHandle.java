package org.maplibre.nativeffi;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.util.EnumSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import org.maplibre.nativeffi.internal.HandleState;
import org.maplibre.nativeffi.internal.MemoryUtil;
import org.maplibre.nativeffi.internal.NativeAccess;
import org.maplibre.nativeffi.internal.Status;
import org.maplibre.nativeffi.internal.Structs;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_lat_lng;
import org.maplibre.nativeffi.internal.c.mln_lat_lng_bounds;
import org.maplibre.nativeffi.internal.c.mln_screen_point;

/** Owned native map handle. Close it on the map owner thread. */
public final class MapHandle implements AutoCloseable {
  private final RuntimeHandle runtime;
  private final HandleState state;

  private MapHandle(RuntimeHandle runtime, MemorySegment handle) {
    this.runtime = runtime;
    this.state = new HandleState("MapHandle", handle, runtime);
  }

  public static MapHandle create(RuntimeHandle runtime, MapOptions options) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(runtime, "runtime");
    Objects.requireNonNull(options, "options");
    try (var arena = Arena.ofConfined()) {
      var outMap = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_map_create(
              runtime.nativeHandle(), Structs.mapOptions(options, arena), outMap));
      var map = new MapHandle(runtime, outMap.get(ValueLayout.ADDRESS, 0));
      runtime.registerMap(map);
      return map;
    }
  }

  public void setStyleUrl(String url) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_style_url(
              state.requireLive(), MemoryUtil.allocateCString(arena, Objects.requireNonNull(url))));
    }
  }

  public void setStyleJson(String json) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_style_json(
              state.requireLive(),
              MemoryUtil.allocateCString(arena, Objects.requireNonNull(json))));
    }
  }

  public void requestRepaint() {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_map_request_repaint(state.requireLive()));
  }

  public void requestStillImage() {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_map_request_still_image(state.requireLive()));
  }

  public void setDebugOptions(Set<MapDebugOption> options) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(options, "options");
    var mask = 0;
    for (var option : options) {
      mask |= Objects.requireNonNull(option, "option").nativeMask();
    }
    Status.check(MapLibreNativeC.mln_map_set_debug_options(state.requireLive(), mask));
  }

  public EnumSet<MapDebugOption> debugOptions() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outOptions = arena.allocate(ValueLayout.JAVA_INT);
      Status.check(MapLibreNativeC.mln_map_get_debug_options(state.requireLive(), outOptions));
      var mask = outOptions.get(ValueLayout.JAVA_INT, 0);
      var options = EnumSet.noneOf(MapDebugOption.class);
      for (var option : MapDebugOption.values()) {
        if ((mask & option.nativeMask()) != 0) {
          options.add(option);
        }
      }
      return options;
    }
  }

  public void setRenderingStatsViewEnabled(boolean enabled) {
    NativeAccess.ensureLoaded();
    Status.check(
        MapLibreNativeC.mln_map_set_rendering_stats_view_enabled(state.requireLive(), enabled));
  }

  public boolean isRenderingStatsViewEnabled() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outEnabled = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(
          MapLibreNativeC.mln_map_get_rendering_stats_view_enabled(
              state.requireLive(), outEnabled));
      return outEnabled.get(ValueLayout.JAVA_BOOLEAN, 0);
    }
  }

  public boolean isFullyLoaded() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outLoaded = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(MapLibreNativeC.mln_map_is_fully_loaded(state.requireLive(), outLoaded));
      return outLoaded.get(ValueLayout.JAVA_BOOLEAN, 0);
    }
  }

  public void dumpDebugLogs() {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_map_dump_debug_logs(state.requireLive()));
  }

  public MapViewportOptions viewportOptions() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outOptions = MapLibreNativeC.mln_map_viewport_options_default(arena);
      Status.check(MapLibreNativeC.mln_map_get_viewport_options(state.requireLive(), outOptions));
      return Structs.viewportOptions(outOptions);
    }
  }

  public void setViewportOptions(MapViewportOptions options) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(options, "options");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_viewport_options(
              state.requireLive(), Structs.viewportOptions(options, arena)));
    }
  }

  public MapTileOptions tileOptions() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outOptions = MapLibreNativeC.mln_map_tile_options_default(arena);
      Status.check(MapLibreNativeC.mln_map_get_tile_options(state.requireLive(), outOptions));
      return Structs.tileOptions(outOptions);
    }
  }

  public void setTileOptions(MapTileOptions options) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(options, "options");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_tile_options(
              state.requireLive(), Structs.tileOptions(options, arena)));
    }
  }

  public CameraOptions camera() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outCamera = MapLibreNativeC.mln_camera_options_default(arena);
      Status.check(MapLibreNativeC.mln_map_get_camera(state.requireLive(), outCamera));
      return Structs.cameraOptions(outCamera);
    }
  }

  public void jumpTo(CameraOptions camera) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(camera, "camera");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_jump_to(
              state.requireLive(), Structs.cameraOptions(camera, arena)));
    }
  }

  public void easeTo(CameraOptions camera) {
    easeToInternal(camera, null, false);
  }

  public void easeTo(CameraOptions camera, AnimationOptions animation) {
    easeToInternal(camera, Objects.requireNonNull(animation, "animation"), true);
  }

  public void flyTo(CameraOptions camera) {
    flyToInternal(camera, null, false);
  }

  public void flyTo(CameraOptions camera, AnimationOptions animation) {
    flyToInternal(camera, Objects.requireNonNull(animation, "animation"), true);
  }

  public void moveBy(double deltaX, double deltaY) {
    NativeAccess.ensureLoaded();
    requireFinite(deltaX, "deltaX");
    requireFinite(deltaY, "deltaY");
    Status.check(MapLibreNativeC.mln_map_move_by(state.requireLive(), deltaX, deltaY));
  }

  public void moveByAnimated(double deltaX, double deltaY) {
    moveByAnimatedInternal(deltaX, deltaY, null, false);
  }

  public void moveByAnimated(double deltaX, double deltaY, AnimationOptions animation) {
    moveByAnimatedInternal(deltaX, deltaY, Objects.requireNonNull(animation, "animation"), true);
  }

  public void scaleBy(double scale) {
    scaleByInternal(scale, null, false);
  }

  public void scaleBy(double scale, ScreenPoint anchor) {
    scaleByInternal(scale, Objects.requireNonNull(anchor, "anchor"), true);
  }

  public void scaleByAnimated(double scale) {
    scaleByAnimatedInternal(scale, null, false, null, false);
  }

  public void scaleByAnimated(double scale, ScreenPoint anchor) {
    scaleByAnimatedInternal(scale, Objects.requireNonNull(anchor, "anchor"), true, null, false);
  }

  public void scaleByAnimated(double scale, AnimationOptions animation) {
    scaleByAnimatedInternal(
        scale, null, false, Objects.requireNonNull(animation, "animation"), true);
  }

  public void scaleByAnimated(double scale, ScreenPoint anchor, AnimationOptions animation) {
    scaleByAnimatedInternal(
        scale,
        Objects.requireNonNull(anchor, "anchor"),
        true,
        Objects.requireNonNull(animation, "animation"),
        true);
  }

  public void rotateBy(ScreenPoint first, ScreenPoint second) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(first, "first");
    Objects.requireNonNull(second, "second");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_rotate_by(
              state.requireLive(),
              Structs.screenPoint(first, arena),
              Structs.screenPoint(second, arena)));
    }
  }

  public void rotateByAnimated(ScreenPoint first, ScreenPoint second) {
    rotateByAnimatedInternal(first, second, null, false);
  }

  public void rotateByAnimated(ScreenPoint first, ScreenPoint second, AnimationOptions animation) {
    rotateByAnimatedInternal(first, second, Objects.requireNonNull(animation, "animation"), true);
  }

  public void pitchBy(double pitch) {
    NativeAccess.ensureLoaded();
    requireFinite(pitch, "pitch");
    Status.check(MapLibreNativeC.mln_map_pitch_by(state.requireLive(), pitch));
  }

  public void pitchByAnimated(double pitch) {
    pitchByAnimatedInternal(pitch, null, false);
  }

  public void pitchByAnimated(double pitch, AnimationOptions animation) {
    pitchByAnimatedInternal(pitch, Objects.requireNonNull(animation, "animation"), true);
  }

  public void cancelTransitions() {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_map_cancel_transitions(state.requireLive()));
  }

  public CameraOptions cameraForLatLngBounds(LatLngBounds bounds) {
    return cameraForLatLngBoundsInternal(bounds, null, false);
  }

  public CameraOptions cameraForLatLngBounds(LatLngBounds bounds, CameraFitOptions fitOptions) {
    return cameraForLatLngBoundsInternal(
        bounds, Objects.requireNonNull(fitOptions, "fitOptions"), true);
  }

  public CameraOptions cameraForLatLngs(List<LatLng> coordinates) {
    return cameraForLatLngsInternal(coordinates, null, false);
  }

  public CameraOptions cameraForLatLngs(List<LatLng> coordinates, CameraFitOptions fitOptions) {
    return cameraForLatLngsInternal(
        coordinates, Objects.requireNonNull(fitOptions, "fitOptions"), true);
  }

  public LatLngBounds latLngBoundsForCamera(CameraOptions camera) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(camera, "camera");
    try (var arena = Arena.ofConfined()) {
      var outBounds = mln_lat_lng_bounds.allocate(arena);
      Status.check(
          MapLibreNativeC.mln_map_lat_lng_bounds_for_camera(
              state.requireLive(), Structs.cameraOptions(camera, arena), outBounds));
      return Structs.latLngBounds(outBounds);
    }
  }

  public LatLngBounds latLngBoundsForCameraUnwrapped(CameraOptions camera) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(camera, "camera");
    try (var arena = Arena.ofConfined()) {
      var outBounds = mln_lat_lng_bounds.allocate(arena);
      Status.check(
          MapLibreNativeC.mln_map_lat_lng_bounds_for_camera_unwrapped(
              state.requireLive(), Structs.cameraOptions(camera, arena), outBounds));
      return Structs.latLngBounds(outBounds);
    }
  }

  public BoundOptions bounds() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outOptions = MapLibreNativeC.mln_bound_options_default(arena);
      Status.check(MapLibreNativeC.mln_map_get_bounds(state.requireLive(), outOptions));
      return Structs.boundOptions(outOptions);
    }
  }

  public void setBounds(BoundOptions options) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(options, "options");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_bounds(
              state.requireLive(), Structs.boundOptions(options, arena)));
    }
  }

  public FreeCameraOptions freeCameraOptions() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outOptions = MapLibreNativeC.mln_free_camera_options_default(arena);
      Status.check(
          MapLibreNativeC.mln_map_get_free_camera_options(state.requireLive(), outOptions));
      return Structs.freeCameraOptions(outOptions);
    }
  }

  public void setFreeCameraOptions(FreeCameraOptions options) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(options, "options");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_free_camera_options(
              state.requireLive(), Structs.freeCameraOptions(options, arena)));
    }
  }

  public ProjectionModeOptions projectionMode() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outMode = MapLibreNativeC.mln_projection_mode_default(arena);
      Status.check(MapLibreNativeC.mln_map_get_projection_mode(state.requireLive(), outMode));
      return Structs.projectionModeOptions(outMode);
    }
  }

  public void setProjectionMode(ProjectionModeOptions mode) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(mode, "mode");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_projection_mode(
              state.requireLive(), Structs.projectionModeOptions(mode, arena)));
    }
  }

  public ScreenPoint pixelForLatLng(LatLng coordinate) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(coordinate, "coordinate");
    try (var arena = Arena.ofConfined()) {
      var outPoint = mln_screen_point.allocate(arena);
      Status.check(
          MapLibreNativeC.mln_map_pixel_for_lat_lng(
              state.requireLive(), Structs.latLng(coordinate, arena), outPoint));
      return Structs.screenPoint(outPoint);
    }
  }

  public LatLng latLngForPixel(ScreenPoint point) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(point, "point");
    try (var arena = Arena.ofConfined()) {
      var outCoordinate = mln_lat_lng.allocate(arena);
      Status.check(
          MapLibreNativeC.mln_map_lat_lng_for_pixel(
              state.requireLive(), Structs.screenPoint(point, arena), outCoordinate));
      return Structs.latLng(outCoordinate);
    }
  }

  public List<ScreenPoint> pixelsForLatLngs(List<LatLng> coordinates) {
    NativeAccess.ensureLoaded();
    var copiedCoordinates = List.copyOf(Objects.requireNonNull(coordinates, "coordinates"));
    try (var arena = Arena.ofConfined()) {
      var outPoints =
          copiedCoordinates.isEmpty()
              ? MemorySegment.NULL
              : mln_screen_point.allocateArray(copiedCoordinates.size(), arena);
      Status.check(
          MapLibreNativeC.mln_map_pixels_for_lat_lngs(
              state.requireLive(),
              copiedCoordinates.isEmpty()
                  ? MemorySegment.NULL
                  : Structs.latLngArray(copiedCoordinates, arena),
              copiedCoordinates.size(),
              outPoints));
      return copiedCoordinates.isEmpty()
          ? List.of()
          : Structs.screenPointArray(outPoints, copiedCoordinates.size());
    }
  }

  public List<LatLng> latLngsForPixels(List<ScreenPoint> points) {
    NativeAccess.ensureLoaded();
    var copiedPoints = List.copyOf(Objects.requireNonNull(points, "points"));
    try (var arena = Arena.ofConfined()) {
      var outCoordinates =
          copiedPoints.isEmpty()
              ? MemorySegment.NULL
              : mln_lat_lng.allocateArray(copiedPoints.size(), arena);
      Status.check(
          MapLibreNativeC.mln_map_lat_lngs_for_pixels(
              state.requireLive(),
              copiedPoints.isEmpty()
                  ? MemorySegment.NULL
                  : Structs.screenPointArray(copiedPoints, arena),
              copiedPoints.size(),
              outCoordinates));
      return copiedPoints.isEmpty()
          ? List.of()
          : Structs.latLngArray(outCoordinates, copiedPoints.size());
    }
  }

  public MapProjectionHandle createProjection() {
    return MapProjectionHandle.create(this);
  }

  private void easeToInternal(
      CameraOptions camera, AnimationOptions animation, boolean hasAnimation) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(camera, "camera");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_ease_to(
              state.requireLive(),
              Structs.cameraOptions(camera, arena),
              hasAnimation ? Structs.animationOptions(animation, arena) : MemorySegment.NULL));
    }
  }

  private void flyToInternal(
      CameraOptions camera, AnimationOptions animation, boolean hasAnimation) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(camera, "camera");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_fly_to(
              state.requireLive(),
              Structs.cameraOptions(camera, arena),
              hasAnimation ? Structs.animationOptions(animation, arena) : MemorySegment.NULL));
    }
  }

  private void moveByAnimatedInternal(
      double deltaX, double deltaY, AnimationOptions animation, boolean hasAnimation) {
    NativeAccess.ensureLoaded();
    requireFinite(deltaX, "deltaX");
    requireFinite(deltaY, "deltaY");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_move_by_animated(
              state.requireLive(),
              deltaX,
              deltaY,
              hasAnimation ? Structs.animationOptions(animation, arena) : MemorySegment.NULL));
    }
  }

  private void scaleByInternal(double scale, ScreenPoint anchor, boolean hasAnchor) {
    NativeAccess.ensureLoaded();
    requirePositiveFinite(scale, "scale");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_scale_by(
              state.requireLive(),
              scale,
              hasAnchor ? Structs.screenPoint(anchor, arena) : MemorySegment.NULL));
    }
  }

  private void scaleByAnimatedInternal(
      double scale,
      ScreenPoint anchor,
      boolean hasAnchor,
      AnimationOptions animation,
      boolean hasAnimation) {
    NativeAccess.ensureLoaded();
    requirePositiveFinite(scale, "scale");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_scale_by_animated(
              state.requireLive(),
              scale,
              hasAnchor ? Structs.screenPoint(anchor, arena) : MemorySegment.NULL,
              hasAnimation ? Structs.animationOptions(animation, arena) : MemorySegment.NULL));
    }
  }

  private void rotateByAnimatedInternal(
      ScreenPoint first, ScreenPoint second, AnimationOptions animation, boolean hasAnimation) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(first, "first");
    Objects.requireNonNull(second, "second");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_rotate_by_animated(
              state.requireLive(),
              Structs.screenPoint(first, arena),
              Structs.screenPoint(second, arena),
              hasAnimation ? Structs.animationOptions(animation, arena) : MemorySegment.NULL));
    }
  }

  private void pitchByAnimatedInternal(
      double pitch, AnimationOptions animation, boolean hasAnimation) {
    NativeAccess.ensureLoaded();
    requireFinite(pitch, "pitch");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_pitch_by_animated(
              state.requireLive(),
              pitch,
              hasAnimation ? Structs.animationOptions(animation, arena) : MemorySegment.NULL));
    }
  }

  private CameraOptions cameraForLatLngBoundsInternal(
      LatLngBounds bounds, CameraFitOptions fitOptions, boolean hasFitOptions) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(bounds, "bounds");
    try (var arena = Arena.ofConfined()) {
      var outCamera = MapLibreNativeC.mln_camera_options_default(arena);
      Status.check(
          MapLibreNativeC.mln_map_camera_for_lat_lng_bounds(
              state.requireLive(),
              Structs.latLngBounds(bounds, arena),
              hasFitOptions ? Structs.cameraFitOptions(fitOptions, arena) : MemorySegment.NULL,
              outCamera));
      return Structs.cameraOptions(outCamera);
    }
  }

  private CameraOptions cameraForLatLngsInternal(
      List<LatLng> coordinates, CameraFitOptions fitOptions, boolean hasFitOptions) {
    NativeAccess.ensureLoaded();
    var copiedCoordinates = List.copyOf(Objects.requireNonNull(coordinates, "coordinates"));
    if (copiedCoordinates.isEmpty()) {
      throw new IllegalArgumentException("coordinates must not be empty");
    }
    try (var arena = Arena.ofConfined()) {
      var outCamera = MapLibreNativeC.mln_camera_options_default(arena);
      Status.check(
          MapLibreNativeC.mln_map_camera_for_lat_lngs(
              state.requireLive(),
              Structs.latLngArray(copiedCoordinates, arena),
              copiedCoordinates.size(),
              hasFitOptions ? Structs.cameraFitOptions(fitOptions, arena) : MemorySegment.NULL,
              outCamera));
      return Structs.cameraOptions(outCamera);
    }
  }

  @Override
  public void close() {
    NativeAccess.ensureLoaded();
    state.closeOnce(MapLibreNativeC::mln_map_destroy, () -> runtime.unregisterMap(this));
  }

  public boolean isClosed() {
    return state.isReleased();
  }

  public RuntimeHandle runtime() {
    return runtime;
  }

  MemorySegment nativeHandle() {
    return state.requireLive();
  }

  long nativeAddress() {
    return state.address();
  }

  private static double requireFinite(double value, String name) {
    if (!Double.isFinite(value)) {
      throw new IllegalArgumentException(name + " must be finite");
    }
    return value;
  }

  private static double requirePositiveFinite(double value, String name) {
    if (!Double.isFinite(value) || value <= 0) {
      throw new IllegalArgumentException(name + " must be finite and positive");
    }
    return value;
  }
}
