package org.maplibre.nativejni.map;

import java.lang.foreign.MemorySegment;
import java.util.EnumSet;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import org.maplibre.nativejni.camera.AnimationOptions;
import org.maplibre.nativejni.camera.BoundOptions;
import org.maplibre.nativejni.camera.CameraFitOptions;
import org.maplibre.nativejni.camera.CameraOptions;
import org.maplibre.nativejni.camera.FreeCameraOptions;
import org.maplibre.nativejni.geo.CanonicalTileId;
import org.maplibre.nativejni.geo.GeoJson;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.LatLngBounds;
import org.maplibre.nativejni.geo.ScreenPoint;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.internal.bridge.CameraNative;
import org.maplibre.nativejni.internal.bridge.MapNative;
import org.maplibre.nativejni.internal.lifecycle.HandleState;
import org.maplibre.nativejni.internal.loader.NativeLibrary;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.json.JsonValue;
import org.maplibre.nativejni.render.MetalBorrowedTextureDescriptor;
import org.maplibre.nativejni.render.MetalOwnedTextureDescriptor;
import org.maplibre.nativejni.render.MetalSurfaceDescriptor;
import org.maplibre.nativejni.render.PremultipliedRgba8Image;
import org.maplibre.nativejni.render.RenderSessionHandle;
import org.maplibre.nativejni.render.VulkanBorrowedTextureDescriptor;
import org.maplibre.nativejni.render.VulkanOwnedTextureDescriptor;
import org.maplibre.nativejni.render.VulkanSurfaceDescriptor;
import org.maplibre.nativejni.runtime.RuntimeHandle;
import org.maplibre.nativejni.style.CustomGeometrySourceOptions;
import org.maplibre.nativejni.style.LocationIndicatorImageKind;
import org.maplibre.nativejni.style.SourceInfo;
import org.maplibre.nativejni.style.SourceType;
import org.maplibre.nativejni.style.StyleImage;
import org.maplibre.nativejni.style.StyleImageInfo;
import org.maplibre.nativejni.style.StyleImageOptions;
import org.maplibre.nativejni.style.TileSourceOptions;

/** API-parity scaffold for the Java JNI binding. */
public final class MapHandle implements AutoCloseable {
  private final RuntimeHandle runtime;
  private final HandleState state;

  private MapHandle(RuntimeHandle runtime, long handle) {
    this.runtime = Objects.requireNonNull(runtime, "runtime");
    this.state = new HandleState("MapHandle", handle, runtime);
    runtime.registerMap(InternalAccess.INSTANCE, this);
  }

  private static UnsupportedOperationException unsupported() {
    return new UnsupportedOperationException("MapHandle is not implemented by the JNI bridge yet");
  }

  public static MapHandle create(RuntimeHandle runtime, MapOptions options) {
    Objects.requireNonNull(runtime, "runtime");
    Objects.requireNonNull(options, "options");
    NativeLibrary.ensureLoaded();
    var outMap = new long[1];
    Status.check(
        MapNative.mln_map_create(
            runtime.nativeHandle(InternalAccess.INSTANCE).address(),
            options.width() == null ? 512 : options.width(),
            options.height() == null ? 512 : options.height(),
            options.scaleFactor() == null ? 1.0 : options.scaleFactor(),
            options.mapMode() == null
                ? MapMode.CONTINUOUS.nativeValue()
                : options.mapMode().nativeValue(),
            outMap));
    return new MapHandle(runtime, outMap[0]);
  }

  public void setStyleUrl(String url) {
    NativeLibrary.ensureLoaded();
    Status.check(
        MapNative.mln_map_set_style_url(state.requireLiveAddress(), Objects.requireNonNull(url)));
  }

  public void setStyleJson(String json) {
    NativeLibrary.ensureLoaded();
    Status.check(
        MapNative.mln_map_set_style_json(state.requireLiveAddress(), Objects.requireNonNull(json)));
  }

  public void addStyleSourceJson(String sourceId, JsonValue sourceJson) {
    throw unsupported();
  }

  public boolean removeStyleSource(String sourceId) {
    throw unsupported();
  }

  public boolean styleSourceExists(String sourceId) {
    throw unsupported();
  }

  public Optional<SourceType> styleSourceType(String sourceId) {
    throw unsupported();
  }

  public Optional<SourceInfo> styleSourceInfo(String sourceId) {
    throw unsupported();
  }

  public List<String> styleSourceIds() {
    throw unsupported();
  }

  public void addGeoJsonSourceUrl(String sourceId, String url) {
    throw unsupported();
  }

  public void addGeoJsonSourceData(String sourceId, GeoJson data) {
    throw unsupported();
  }

  public void setGeoJsonSourceUrl(String sourceId, String url) {
    throw unsupported();
  }

  public void setGeoJsonSourceData(String sourceId, GeoJson data) {
    throw unsupported();
  }

  public void addCustomGeometrySource(String sourceId, CustomGeometrySourceOptions options) {
    throw unsupported();
  }

  public void setCustomGeometrySourceTileData(
      String sourceId, CanonicalTileId tileId, GeoJson data) {
    throw unsupported();
  }

  public void invalidateCustomGeometrySourceTile(String sourceId, CanonicalTileId tileId) {
    throw unsupported();
  }

  public void invalidateCustomGeometrySourceRegion(String sourceId, LatLngBounds bounds) {
    throw unsupported();
  }

  public void addVectorSourceUrl(String sourceId, String url) {
    throw unsupported();
  }

  public void addVectorSourceUrl(String sourceId, String url, TileSourceOptions options) {
    throw unsupported();
  }

  public void addVectorSourceTiles(String sourceId, List<String> tiles) {
    throw unsupported();
  }

  public void addVectorSourceTiles(String sourceId, List<String> tiles, TileSourceOptions options) {
    throw unsupported();
  }

  public void addRasterSourceUrl(String sourceId, String url) {
    throw unsupported();
  }

  public void addRasterSourceUrl(String sourceId, String url, TileSourceOptions options) {
    throw unsupported();
  }

  public void addRasterSourceTiles(String sourceId, List<String> tiles) {
    throw unsupported();
  }

  public void addRasterSourceTiles(String sourceId, List<String> tiles, TileSourceOptions options) {
    throw unsupported();
  }

  public void addRasterDemSourceUrl(String sourceId, String url) {
    throw unsupported();
  }

  public void addRasterDemSourceUrl(String sourceId, String url, TileSourceOptions options) {
    throw unsupported();
  }

  public void addRasterDemSourceTiles(String sourceId, List<String> tiles) {
    throw unsupported();
  }

  public void addRasterDemSourceTiles(
      String sourceId, List<String> tiles, TileSourceOptions options) {
    throw unsupported();
  }

  public void setStyleImage(String imageId, PremultipliedRgba8Image image) {
    throw unsupported();
  }

  public void setStyleImage(
      String imageId, PremultipliedRgba8Image image, StyleImageOptions options) {
    throw unsupported();
  }

  public boolean removeStyleImage(String imageId) {
    throw unsupported();
  }

  public boolean styleImageExists(String imageId) {
    throw unsupported();
  }

  public Optional<StyleImageInfo> styleImageInfo(String imageId) {
    throw unsupported();
  }

  public Optional<StyleImage> copyStyleImagePremultipliedRgba8(String imageId) {
    throw unsupported();
  }

  public void addImageSourceUrl(String sourceId, List<LatLng> coordinates, String url) {
    throw unsupported();
  }

  public void addImageSourceImage(
      String sourceId, List<LatLng> coordinates, PremultipliedRgba8Image image) {
    throw unsupported();
  }

  public void setImageSourceUrl(String sourceId, String url) {
    throw unsupported();
  }

  public void setImageSourceImage(String sourceId, PremultipliedRgba8Image image) {
    throw unsupported();
  }

  public void setImageSourceCoordinates(String sourceId, List<LatLng> coordinates) {
    throw unsupported();
  }

  public Optional<List<LatLng>> imageSourceCoordinates(String sourceId) {
    throw unsupported();
  }

  public void addStyleLayerJson(JsonValue layerJson) {
    throw unsupported();
  }

  public void addStyleLayerJson(JsonValue layerJson, String beforeLayerId) {
    throw unsupported();
  }

  public void addHillshadeLayer(String layerId, String sourceId) {
    throw unsupported();
  }

  public void addHillshadeLayer(String layerId, String sourceId, String beforeLayerId) {
    throw unsupported();
  }

  public void addColorReliefLayer(String layerId, String sourceId) {
    throw unsupported();
  }

  public void addColorReliefLayer(String layerId, String sourceId, String beforeLayerId) {
    throw unsupported();
  }

  public void addLocationIndicatorLayer(String layerId) {
    throw unsupported();
  }

  public void addLocationIndicatorLayer(String layerId, String beforeLayerId) {
    throw unsupported();
  }

  public void setLocationIndicatorLocation(String layerId, LatLng coordinate, double altitude) {
    throw unsupported();
  }

  public void setLocationIndicatorBearing(String layerId, double bearing) {
    throw unsupported();
  }

  public void setLocationIndicatorAccuracyRadius(String layerId, double radius) {
    throw unsupported();
  }

  public void setLocationIndicatorImageName(
      String layerId, LocationIndicatorImageKind imageKind, String imageId) {
    throw unsupported();
  }

  public boolean removeStyleLayer(String layerId) {
    throw unsupported();
  }

  public boolean styleLayerExists(String layerId) {
    throw unsupported();
  }

  public Optional<String> styleLayerType(String layerId) {
    throw unsupported();
  }

  public List<String> styleLayerIds() {
    throw unsupported();
  }

  public void moveStyleLayer(String layerId) {
    throw unsupported();
  }

  public void moveStyleLayer(String layerId, String beforeLayerId) {
    throw unsupported();
  }

  public Optional<JsonValue> styleLayerJson(String layerId) {
    throw unsupported();
  }

  public void setStyleLightJson(JsonValue lightJson) {
    throw unsupported();
  }

  public void setStyleLightProperty(String propertyName, JsonValue value) {
    throw unsupported();
  }

  public Optional<JsonValue> styleLightProperty(String propertyName) {
    throw unsupported();
  }

  public void setLayerProperty(String layerId, String propertyName, JsonValue value) {
    throw unsupported();
  }

  public Optional<JsonValue> layerProperty(String layerId, String propertyName) {
    throw unsupported();
  }

  public void setLayerFilter(String layerId, JsonValue filter) {
    throw unsupported();
  }

  public void clearLayerFilter(String layerId) {
    throw unsupported();
  }

  public Optional<JsonValue> layerFilter(String layerId) {
    throw unsupported();
  }

  public RenderSessionHandle attachMetalOwnedTexture(MetalOwnedTextureDescriptor descriptor) {
    throw unsupported();
  }

  public RenderSessionHandle attachMetalBorrowedTexture(MetalBorrowedTextureDescriptor descriptor) {
    throw unsupported();
  }

  public RenderSessionHandle attachVulkanOwnedTexture(VulkanOwnedTextureDescriptor descriptor) {
    throw unsupported();
  }

  public RenderSessionHandle attachVulkanBorrowedTexture(
      VulkanBorrowedTextureDescriptor descriptor) {
    throw unsupported();
  }

  public RenderSessionHandle attachMetalSurface(MetalSurfaceDescriptor descriptor) {
    throw unsupported();
  }

  public RenderSessionHandle attachVulkanSurface(VulkanSurfaceDescriptor descriptor) {
    throw unsupported();
  }

  public void requestRepaint() {
    NativeLibrary.ensureLoaded();
    Status.check(MapNative.mln_map_request_repaint(state.requireLiveAddress()));
  }

  public void requestStillImage() {
    NativeLibrary.ensureLoaded();
    Status.check(MapNative.mln_map_request_still_image(state.requireLiveAddress()));
  }

  public void setDebugOptions(Set<DebugOption> options) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(options, "options");
    var mask = 0;
    for (var option : options) {
      mask |= Objects.requireNonNull(option, "option").nativeMask();
    }
    Status.check(CameraNative.mln_map_set_debug_options(state.requireLiveAddress(), mask));
  }

  public EnumSet<DebugOption> debugOptions() {
    NativeLibrary.ensureLoaded();
    var outOptions = new int[1];
    Status.check(CameraNative.mln_map_get_debug_options(state.requireLiveAddress(), outOptions));
    var options = EnumSet.noneOf(DebugOption.class);
    for (var option : DebugOption.values()) {
      if ((outOptions[0] & option.nativeMask()) != 0) {
        options.add(option);
      }
    }
    return options;
  }

  public void setRenderingStatsViewEnabled(boolean enabled) {
    NativeLibrary.ensureLoaded();
    Status.check(
        CameraNative.mln_map_set_rendering_stats_view_enabled(state.requireLiveAddress(), enabled));
  }

  public boolean isRenderingStatsViewEnabled() {
    NativeLibrary.ensureLoaded();
    var outEnabled = new boolean[1];
    Status.check(
        CameraNative.mln_map_get_rendering_stats_view_enabled(
            state.requireLiveAddress(), outEnabled));
    return outEnabled[0];
  }

  public boolean isFullyLoaded() {
    NativeLibrary.ensureLoaded();
    var outLoaded = new boolean[1];
    Status.check(CameraNative.mln_map_is_fully_loaded(state.requireLiveAddress(), outLoaded));
    return outLoaded[0];
  }

  public void dumpDebugLogs() {
    NativeLibrary.ensureLoaded();
    Status.check(CameraNative.mln_map_dump_debug_logs(state.requireLiveAddress()));
  }

  public ViewportOptions viewportOptions() {
    throw unsupported();
  }

  public void setViewportOptions(ViewportOptions options) {
    throw unsupported();
  }

  public TileOptions tileOptions() {
    throw unsupported();
  }

  public void setTileOptions(TileOptions options) {
    throw unsupported();
  }

  public CameraOptions camera() {
    throw unsupported();
  }

  public void jumpTo(CameraOptions camera) {
    throw unsupported();
  }

  public void easeTo(CameraOptions camera) {
    throw unsupported();
  }

  public void easeTo(CameraOptions camera, AnimationOptions animation) {
    throw unsupported();
  }

  public void flyTo(CameraOptions camera) {
    throw unsupported();
  }

  public void flyTo(CameraOptions camera, AnimationOptions animation) {
    throw unsupported();
  }

  public void moveBy(double deltaX, double deltaY) {
    NativeLibrary.ensureLoaded();
    Status.check(CameraNative.mln_map_move_by(state.requireLiveAddress(), deltaX, deltaY));
  }

  public void moveByAnimated(double deltaX, double deltaY) {
    NativeLibrary.ensureLoaded();
    Status.check(CameraNative.mln_map_move_by_animated(state.requireLiveAddress(), deltaX, deltaY));
  }

  public void moveByAnimated(double deltaX, double deltaY, AnimationOptions animation) {
    throw unsupported();
  }

  public void scaleBy(double scale) {
    scaleByInternal(scale, null, false);
  }

  public void scaleBy(double scale, ScreenPoint anchor) {
    scaleByInternal(scale, Objects.requireNonNull(anchor, "anchor"), false);
  }

  public void scaleByAnimated(double scale) {
    scaleByInternal(scale, null, true);
  }

  public void scaleByAnimated(double scale, ScreenPoint anchor) {
    scaleByInternal(scale, Objects.requireNonNull(anchor, "anchor"), true);
  }

  public void scaleByAnimated(double scale, AnimationOptions animation) {
    throw unsupported();
  }

  public void scaleByAnimated(double scale, ScreenPoint anchor, AnimationOptions animation) {
    throw unsupported();
  }

  public void rotateBy(ScreenPoint first, ScreenPoint second) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(first, "first");
    Objects.requireNonNull(second, "second");
    Status.check(
        CameraNative.mln_map_rotate_by(
            state.requireLiveAddress(), first.x(), first.y(), second.x(), second.y()));
  }

  public void rotateByAnimated(ScreenPoint first, ScreenPoint second) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(first, "first");
    Objects.requireNonNull(second, "second");
    Status.check(
        CameraNative.mln_map_rotate_by_animated(
            state.requireLiveAddress(), first.x(), first.y(), second.x(), second.y()));
  }

  public void rotateByAnimated(ScreenPoint first, ScreenPoint second, AnimationOptions animation) {
    throw unsupported();
  }

  public void pitchBy(double pitch) {
    NativeLibrary.ensureLoaded();
    Status.check(CameraNative.mln_map_pitch_by(state.requireLiveAddress(), pitch));
  }

  public void pitchByAnimated(double pitch) {
    NativeLibrary.ensureLoaded();
    Status.check(CameraNative.mln_map_pitch_by_animated(state.requireLiveAddress(), pitch));
  }

  public void pitchByAnimated(double pitch, AnimationOptions animation) {
    throw unsupported();
  }

  public void cancelTransitions() {
    NativeLibrary.ensureLoaded();
    Status.check(CameraNative.mln_map_cancel_transitions(state.requireLiveAddress()));
  }

  private void scaleByInternal(double scale, ScreenPoint anchor, boolean animated) {
    NativeLibrary.ensureLoaded();
    var hasAnchor = anchor != null;
    var anchorX = hasAnchor ? anchor.x() : 0;
    var anchorY = hasAnchor ? anchor.y() : 0;
    Status.check(
        animated
            ? CameraNative.mln_map_scale_by_animated(
                state.requireLiveAddress(), scale, hasAnchor, anchorX, anchorY)
            : CameraNative.mln_map_scale_by(
                state.requireLiveAddress(), scale, hasAnchor, anchorX, anchorY));
  }

  public CameraOptions cameraForLatLngBounds(LatLngBounds bounds) {
    throw unsupported();
  }

  public CameraOptions cameraForLatLngBounds(LatLngBounds bounds, CameraFitOptions fitOptions) {
    throw unsupported();
  }

  public CameraOptions cameraForLatLngs(List<LatLng> coordinates) {
    throw unsupported();
  }

  public CameraOptions cameraForLatLngs(List<LatLng> coordinates, CameraFitOptions fitOptions) {
    throw unsupported();
  }

  public CameraOptions cameraForGeometry(Geometry geometry) {
    throw unsupported();
  }

  public CameraOptions cameraForGeometry(Geometry geometry, CameraFitOptions fitOptions) {
    throw unsupported();
  }

  public LatLngBounds latLngBoundsForCamera(CameraOptions camera) {
    throw unsupported();
  }

  public LatLngBounds latLngBoundsForCameraUnwrapped(CameraOptions camera) {
    throw unsupported();
  }

  public BoundOptions bounds() {
    throw unsupported();
  }

  public void setBounds(BoundOptions options) {
    throw unsupported();
  }

  public FreeCameraOptions freeCameraOptions() {
    throw unsupported();
  }

  public void setFreeCameraOptions(FreeCameraOptions options) {
    throw unsupported();
  }

  public ProjectionModeOptions projectionMode() {
    throw unsupported();
  }

  public void setProjectionMode(ProjectionModeOptions mode) {
    throw unsupported();
  }

  public ScreenPoint pixelForLatLng(LatLng coordinate) {
    throw unsupported();
  }

  public LatLng latLngForPixel(ScreenPoint point) {
    throw unsupported();
  }

  public List<ScreenPoint> pixelsForLatLngs(List<LatLng> coordinates) {
    throw unsupported();
  }

  public List<LatLng> latLngsForPixels(List<ScreenPoint> points) {
    throw unsupported();
  }

  public MapProjectionHandle createProjection() {
    return MapProjectionHandle.create(this);
  }

  public void close() {
    state.closeOnce(
        MapNative::mln_map_destroy, () -> runtime.unregisterMap(InternalAccess.INSTANCE, this));
  }

  public boolean isClosed() {
    return state.isReleased();
  }

  public RuntimeHandle runtime() {
    return runtime;
  }

  public MemorySegment nativeHandle(InternalAccess access) {
    Objects.requireNonNull(access, "access");
    return state.requireLiveSegment();
  }

  MemorySegment nativeHandle() {
    return state.requireLiveSegment();
  }

  public long nativeAddress(InternalAccess access) {
    Objects.requireNonNull(access, "access");
    return state.requireLiveAddress();
  }

  long nativeAddress() {
    return state.requireLiveAddress();
  }

  public void releaseDetachedCustomGeometrySources(InternalAccess access) {
    Objects.requireNonNull(access, "access");
  }

  void releaseDetachedCustomGeometrySources() {}

  int customGeometrySourceCountForTesting() {
    return 0;
  }
}
