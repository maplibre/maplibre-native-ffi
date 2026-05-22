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
import org.maplibre.nativejni.camera.EdgeInsets;
import org.maplibre.nativejni.camera.FreeCameraOptions;
import org.maplibre.nativejni.geo.CanonicalTileId;
import org.maplibre.nativejni.geo.GeoJson;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.LatLngBounds;
import org.maplibre.nativejni.geo.Quaternion;
import org.maplibre.nativejni.geo.ScreenPoint;
import org.maplibre.nativejni.geo.Vec3;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.internal.bridge.CameraNative;
import org.maplibre.nativejni.internal.bridge.MapNative;
import org.maplibre.nativejni.internal.bridge.StyleNative;
import org.maplibre.nativejni.internal.lifecycle.HandleState;
import org.maplibre.nativejni.internal.loader.NativeLibrary;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.internal.struct.MapStructs;
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
  static final int CAMERA_FIELD_COUNT = 9;
  static final int CAMERA_VALUE_COUNT = 14;
  private static final int BOUND_FIELD_COUNT = 5;
  private static final int BOUND_VALUE_COUNT = 8;
  private static final int FIT_FIELD_COUNT = 3;
  private static final int FIT_VALUE_COUNT = 6;
  private static final int FREE_CAMERA_FIELD_COUNT = 2;
  private static final int FREE_CAMERA_VALUE_COUNT = 7;
  private static final int PROJECTION_MODE_FIELD_COUNT = 3;
  private static final int PROJECTION_MODE_BOOLEAN_COUNT = 1;
  private static final int PROJECTION_MODE_VALUE_COUNT = 2;
  private static final int ANIMATION_FIELD_COUNT = 4;
  private static final int ANIMATION_VALUE_COUNT = 7;

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
    NativeLibrary.ensureLoaded();
    var outRemoved = new boolean[1];
    Status.check(
        StyleNative.mln_map_remove_style_source(
            state.requireLiveAddress(), Objects.requireNonNull(sourceId, "sourceId"), outRemoved));
    return outRemoved[0];
  }

  public boolean styleSourceExists(String sourceId) {
    NativeLibrary.ensureLoaded();
    var outExists = new boolean[1];
    Status.check(
        StyleNative.mln_map_style_source_exists(
            state.requireLiveAddress(), Objects.requireNonNull(sourceId, "sourceId"), outExists));
    return outExists[0];
  }

  public Optional<SourceType> styleSourceType(String sourceId) {
    NativeLibrary.ensureLoaded();
    var outSourceType = new int[1];
    var outFound = new boolean[1];
    Status.check(
        StyleNative.mln_map_get_style_source_type(
            state.requireLiveAddress(),
            Objects.requireNonNull(sourceId, "sourceId"),
            outSourceType,
            outFound));
    return outFound[0] ? Optional.of(SourceType.fromNative(outSourceType[0])) : Optional.empty();
  }

  public Optional<SourceInfo> styleSourceInfo(String sourceId) {
    throw unsupported();
  }

  public List<String> styleSourceIds() {
    NativeLibrary.ensureLoaded();
    var outSourceIds = new Object[1];
    Status.check(
        StyleNative.mln_map_list_style_source_ids(state.requireLiveAddress(), outSourceIds));
    return List.of((String[]) outSourceIds[0]);
  }

  public void addGeoJsonSourceUrl(String sourceId, String url) {
    NativeLibrary.ensureLoaded();
    Status.check(
        StyleNative.mln_map_add_geojson_source_url(
            state.requireLiveAddress(),
            Objects.requireNonNull(sourceId, "sourceId"),
            Objects.requireNonNull(url, "url")));
  }

  public void addGeoJsonSourceData(String sourceId, GeoJson data) {
    throw unsupported();
  }

  public void setGeoJsonSourceUrl(String sourceId, String url) {
    NativeLibrary.ensureLoaded();
    Status.check(
        StyleNative.mln_map_set_geojson_source_url(
            state.requireLiveAddress(),
            Objects.requireNonNull(sourceId, "sourceId"),
            Objects.requireNonNull(url, "url")));
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
    NativeLibrary.ensureLoaded();
    Status.check(
        StyleNative.mln_map_add_vector_source_url(
            state.requireLiveAddress(),
            Objects.requireNonNull(sourceId, "sourceId"),
            Objects.requireNonNull(url, "url")));
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
    NativeLibrary.ensureLoaded();
    Status.check(
        StyleNative.mln_map_add_raster_source_url(
            state.requireLiveAddress(),
            Objects.requireNonNull(sourceId, "sourceId"),
            Objects.requireNonNull(url, "url")));
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
    NativeLibrary.ensureLoaded();
    Status.check(
        StyleNative.mln_map_add_raster_dem_source_url(
            state.requireLiveAddress(),
            Objects.requireNonNull(sourceId, "sourceId"),
            Objects.requireNonNull(url, "url")));
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
    NativeLibrary.ensureLoaded();
    var outRemoved = new boolean[1];
    Status.check(
        StyleNative.mln_map_remove_style_layer(
            state.requireLiveAddress(), Objects.requireNonNull(layerId, "layerId"), outRemoved));
    return outRemoved[0];
  }

  public boolean styleLayerExists(String layerId) {
    NativeLibrary.ensureLoaded();
    var outExists = new boolean[1];
    Status.check(
        StyleNative.mln_map_style_layer_exists(
            state.requireLiveAddress(), Objects.requireNonNull(layerId, "layerId"), outExists));
    return outExists[0];
  }

  public Optional<String> styleLayerType(String layerId) {
    NativeLibrary.ensureLoaded();
    var outLayerType = new String[1];
    var outFound = new boolean[1];
    Status.check(
        StyleNative.mln_map_get_style_layer_type(
            state.requireLiveAddress(),
            Objects.requireNonNull(layerId, "layerId"),
            outLayerType,
            outFound));
    return outFound[0] ? Optional.of(outLayerType[0]) : Optional.empty();
  }

  public List<String> styleLayerIds() {
    NativeLibrary.ensureLoaded();
    var outLayerIds = new Object[1];
    Status.check(StyleNative.mln_map_list_style_layer_ids(state.requireLiveAddress(), outLayerIds));
    return List.of((String[]) outLayerIds[0]);
  }

  public void moveStyleLayer(String layerId) {
    moveStyleLayer(layerId, "");
  }

  public void moveStyleLayer(String layerId, String beforeLayerId) {
    NativeLibrary.ensureLoaded();
    Status.check(
        StyleNative.mln_map_move_style_layer(
            state.requireLiveAddress(),
            Objects.requireNonNull(layerId, "layerId"),
            Objects.requireNonNull(beforeLayerId, "beforeLayerId")));
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
    NativeLibrary.ensureLoaded();
    var fields = new boolean[CAMERA_FIELD_COUNT];
    var values = new double[CAMERA_VALUE_COUNT];
    Status.check(CameraNative.mln_map_get_camera(state.requireLiveAddress(), fields, values));
    return cameraFromNative(fields, values);
  }

  public void jumpTo(CameraOptions camera) {
    NativeLibrary.ensureLoaded();
    var nativeCamera = cameraToNative(camera);
    Status.check(
        CameraNative.mln_map_jump_to(
            state.requireLiveAddress(), nativeCamera.fields(), nativeCamera.values()));
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
    NativeLibrary.ensureLoaded();
    Status.check(CameraNative.mln_map_move_by(state.requireLiveAddress(), deltaX, deltaY));
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
    scaleByInternal(scale, Objects.requireNonNull(anchor, "anchor"), false);
  }

  public void scaleByAnimated(double scale) {
    scaleByInternal(scale, null, true);
  }

  public void scaleByAnimated(double scale, ScreenPoint anchor) {
    scaleByInternal(scale, Objects.requireNonNull(anchor, "anchor"), true);
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
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(first, "first");
    Objects.requireNonNull(second, "second");
    Status.check(
        CameraNative.mln_map_rotate_by(
            state.requireLiveAddress(), first.x(), first.y(), second.x(), second.y()));
  }

  public void rotateByAnimated(ScreenPoint first, ScreenPoint second) {
    rotateByAnimatedInternal(first, second, null, false);
  }

  public void rotateByAnimated(ScreenPoint first, ScreenPoint second, AnimationOptions animation) {
    rotateByAnimatedInternal(first, second, Objects.requireNonNull(animation, "animation"), true);
  }

  public void pitchBy(double pitch) {
    NativeLibrary.ensureLoaded();
    Status.check(CameraNative.mln_map_pitch_by(state.requireLiveAddress(), pitch));
  }

  public void pitchByAnimated(double pitch) {
    pitchByAnimatedInternal(pitch, null, false);
  }

  public void pitchByAnimated(double pitch, AnimationOptions animation) {
    pitchByAnimatedInternal(pitch, Objects.requireNonNull(animation, "animation"), true);
  }

  public void cancelTransitions() {
    NativeLibrary.ensureLoaded();
    Status.check(CameraNative.mln_map_cancel_transitions(state.requireLiveAddress()));
  }

  private void easeToInternal(
      CameraOptions camera, AnimationOptions animation, boolean hasAnimation) {
    NativeLibrary.ensureLoaded();
    var nativeCamera = cameraToNative(camera);
    var nativeAnimation = animationToNative(animation, hasAnimation);
    Status.check(
        CameraNative.mln_map_ease_to(
            state.requireLiveAddress(),
            nativeCamera.fields(),
            nativeCamera.values(),
            hasAnimation,
            nativeAnimation.fields(),
            nativeAnimation.values()));
  }

  private void flyToInternal(
      CameraOptions camera, AnimationOptions animation, boolean hasAnimation) {
    NativeLibrary.ensureLoaded();
    var nativeCamera = cameraToNative(camera);
    var nativeAnimation = animationToNative(animation, hasAnimation);
    Status.check(
        CameraNative.mln_map_fly_to(
            state.requireLiveAddress(),
            nativeCamera.fields(),
            nativeCamera.values(),
            hasAnimation,
            nativeAnimation.fields(),
            nativeAnimation.values()));
  }

  private void moveByAnimatedInternal(
      double deltaX, double deltaY, AnimationOptions animation, boolean hasAnimation) {
    NativeLibrary.ensureLoaded();
    var nativeAnimation = animationToNative(animation, hasAnimation);
    Status.check(
        CameraNative.mln_map_move_by_animated(
            state.requireLiveAddress(),
            deltaX,
            deltaY,
            hasAnimation,
            nativeAnimation.fields(),
            nativeAnimation.values()));
  }

  private void scaleByInternal(double scale, ScreenPoint anchor, boolean animated) {
    if (animated) {
      scaleByAnimatedInternal(scale, anchor, anchor != null, null, false);
      return;
    }
    NativeLibrary.ensureLoaded();
    var hasAnchor = anchor != null;
    var anchorX = hasAnchor ? anchor.x() : 0;
    var anchorY = hasAnchor ? anchor.y() : 0;
    Status.check(
        CameraNative.mln_map_scale_by(
            state.requireLiveAddress(), scale, hasAnchor, anchorX, anchorY));
  }

  private void scaleByAnimatedInternal(
      double scale,
      ScreenPoint anchor,
      boolean hasAnchor,
      AnimationOptions animation,
      boolean hasAnimation) {
    NativeLibrary.ensureLoaded();
    var anchorX = hasAnchor ? anchor.x() : 0;
    var anchorY = hasAnchor ? anchor.y() : 0;
    var nativeAnimation = animationToNative(animation, hasAnimation);
    Status.check(
        CameraNative.mln_map_scale_by_animated(
            state.requireLiveAddress(),
            scale,
            hasAnchor,
            anchorX,
            anchorY,
            hasAnimation,
            nativeAnimation.fields(),
            nativeAnimation.values()));
  }

  private void rotateByAnimatedInternal(
      ScreenPoint first, ScreenPoint second, AnimationOptions animation, boolean hasAnimation) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(first, "first");
    Objects.requireNonNull(second, "second");
    var nativeAnimation = animationToNative(animation, hasAnimation);
    Status.check(
        CameraNative.mln_map_rotate_by_animated(
            state.requireLiveAddress(),
            first.x(),
            first.y(),
            second.x(),
            second.y(),
            hasAnimation,
            nativeAnimation.fields(),
            nativeAnimation.values()));
  }

  private void pitchByAnimatedInternal(
      double pitch, AnimationOptions animation, boolean hasAnimation) {
    NativeLibrary.ensureLoaded();
    var nativeAnimation = animationToNative(animation, hasAnimation);
    Status.check(
        CameraNative.mln_map_pitch_by_animated(
            state.requireLiveAddress(),
            pitch,
            hasAnimation,
            nativeAnimation.fields(),
            nativeAnimation.values()));
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

  public CameraOptions cameraForGeometry(Geometry geometry) {
    throw unsupported();
  }

  public CameraOptions cameraForGeometry(Geometry geometry, CameraFitOptions fitOptions) {
    throw unsupported();
  }

  public LatLngBounds latLngBoundsForCamera(CameraOptions camera) {
    return latLngBoundsForCameraInternal(camera, false);
  }

  public LatLngBounds latLngBoundsForCameraUnwrapped(CameraOptions camera) {
    return latLngBoundsForCameraInternal(camera, true);
  }

  private CameraOptions cameraForLatLngBoundsInternal(
      LatLngBounds bounds, CameraFitOptions fitOptions, boolean hasFitOptions) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(bounds, "bounds");
    var nativeFit = fitToNative(fitOptions, hasFitOptions);
    var fields = new boolean[CAMERA_FIELD_COUNT];
    var values = new double[CAMERA_VALUE_COUNT];
    Status.check(
        CameraNative.mln_map_camera_for_lat_lng_bounds(
            state.requireLiveAddress(),
            bounds.southwest().latitude(),
            bounds.southwest().longitude(),
            bounds.northeast().latitude(),
            bounds.northeast().longitude(),
            hasFitOptions,
            nativeFit.fields(),
            nativeFit.values(),
            fields,
            values));
    return cameraFromNative(fields, values);
  }

  private CameraOptions cameraForLatLngsInternal(
      List<LatLng> coordinates, CameraFitOptions fitOptions, boolean hasFitOptions) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(coordinates, "coordinates");
    var coordinateValues = new double[coordinates.size() * 2];
    for (var index = 0; index < coordinates.size(); index++) {
      var coordinate = Objects.requireNonNull(coordinates.get(index), "coordinate");
      coordinateValues[index * 2] = coordinate.latitude();
      coordinateValues[index * 2 + 1] = coordinate.longitude();
    }
    var nativeFit = fitToNative(fitOptions, hasFitOptions);
    var fields = new boolean[CAMERA_FIELD_COUNT];
    var values = new double[CAMERA_VALUE_COUNT];
    Status.check(
        CameraNative.mln_map_camera_for_lat_lngs(
            state.requireLiveAddress(),
            coordinateValues,
            hasFitOptions,
            nativeFit.fields(),
            nativeFit.values(),
            fields,
            values));
    return cameraFromNative(fields, values);
  }

  private LatLngBounds latLngBoundsForCameraInternal(CameraOptions camera, boolean unwrapped) {
    NativeLibrary.ensureLoaded();
    var nativeCamera = cameraToNative(camera);
    var boundsValues = new double[4];
    var status =
        unwrapped
            ? CameraNative.mln_map_lat_lng_bounds_for_camera_unwrapped(
                state.requireLiveAddress(),
                nativeCamera.fields(),
                nativeCamera.values(),
                boundsValues)
            : CameraNative.mln_map_lat_lng_bounds_for_camera(
                state.requireLiveAddress(),
                nativeCamera.fields(),
                nativeCamera.values(),
                boundsValues);
    Status.check(status);
    return new LatLngBounds(
        new LatLng(boundsValues[0], boundsValues[1]), new LatLng(boundsValues[2], boundsValues[3]));
  }

  public BoundOptions bounds() {
    NativeLibrary.ensureLoaded();
    var fields = new boolean[BOUND_FIELD_COUNT];
    var values = new double[BOUND_VALUE_COUNT];
    Status.check(CameraNative.mln_map_get_bounds(state.requireLiveAddress(), fields, values));
    return boundsFromNative(fields, values);
  }

  public void setBounds(BoundOptions options) {
    NativeLibrary.ensureLoaded();
    var nativeBounds = boundsToNative(options);
    Status.check(
        CameraNative.mln_map_set_bounds(
            state.requireLiveAddress(), nativeBounds.fields(), nativeBounds.values()));
  }

  public FreeCameraOptions freeCameraOptions() {
    NativeLibrary.ensureLoaded();
    var fields = new boolean[FREE_CAMERA_FIELD_COUNT];
    var values = new double[FREE_CAMERA_VALUE_COUNT];
    Status.check(
        CameraNative.mln_map_get_free_camera_options(state.requireLiveAddress(), fields, values));
    return freeCameraFromNative(fields, values);
  }

  public void setFreeCameraOptions(FreeCameraOptions options) {
    NativeLibrary.ensureLoaded();
    var nativeFreeCamera = freeCameraToNative(options);
    Status.check(
        CameraNative.mln_map_set_free_camera_options(
            state.requireLiveAddress(), nativeFreeCamera.fields(), nativeFreeCamera.values()));
  }

  public ProjectionModeOptions projectionMode() {
    NativeLibrary.ensureLoaded();
    var fields = new boolean[PROJECTION_MODE_FIELD_COUNT];
    var booleans = new boolean[PROJECTION_MODE_BOOLEAN_COUNT];
    var values = new double[PROJECTION_MODE_VALUE_COUNT];
    Status.check(
        CameraNative.mln_map_get_projection_mode(
            state.requireLiveAddress(), fields, booleans, values));
    return projectionModeFromNative(fields, booleans, values);
  }

  public void setProjectionMode(ProjectionModeOptions mode) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(mode, "mode");
    var fields = new boolean[PROJECTION_MODE_FIELD_COUNT];
    var booleans = new boolean[PROJECTION_MODE_BOOLEAN_COUNT];
    var values = new double[PROJECTION_MODE_VALUE_COUNT];
    fields[0] = mode.hasAxonometric();
    booleans[0] = mode.hasAxonometric() && mode.axonometric();
    fields[1] = mode.hasXSkew();
    values[0] = mode.hasXSkew() ? mode.xSkew() : 0;
    fields[2] = mode.hasYSkew();
    values[1] = mode.hasYSkew() ? mode.ySkew() : 0;
    Status.check(
        CameraNative.mln_map_set_projection_mode(
            state.requireLiveAddress(), fields, booleans, values));
  }

  public ScreenPoint pixelForLatLng(LatLng coordinate) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(coordinate, "coordinate");
    var outPoint = new double[2];
    Status.check(
        CameraNative.mln_map_pixel_for_lat_lng(
            state.requireLiveAddress(), coordinate.latitude(), coordinate.longitude(), outPoint));
    return new ScreenPoint(outPoint[0], outPoint[1]);
  }

  public LatLng latLngForPixel(ScreenPoint point) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(point, "point");
    var outCoordinate = new double[2];
    Status.check(
        CameraNative.mln_map_lat_lng_for_pixel(
            state.requireLiveAddress(), point.x(), point.y(), outCoordinate));
    return new LatLng(outCoordinate[0], outCoordinate[1]);
  }

  public List<ScreenPoint> pixelsForLatLngs(List<LatLng> coordinates) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(coordinates, "coordinates");
    var coordinateValues = new double[coordinates.size() * 2];
    for (var index = 0; index < coordinates.size(); index++) {
      var coordinate = Objects.requireNonNull(coordinates.get(index), "coordinate");
      coordinateValues[index * 2] = coordinate.latitude();
      coordinateValues[index * 2 + 1] = coordinate.longitude();
    }
    var pointValues = new double[coordinateValues.length];
    Status.check(
        CameraNative.mln_map_pixels_for_lat_lngs(
            state.requireLiveAddress(), coordinateValues, pointValues));
    var points = new java.util.ArrayList<ScreenPoint>(coordinates.size());
    for (var index = 0; index < coordinates.size(); index++) {
      points.add(new ScreenPoint(pointValues[index * 2], pointValues[index * 2 + 1]));
    }
    return List.copyOf(points);
  }

  public List<LatLng> latLngsForPixels(List<ScreenPoint> points) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(points, "points");
    var pointValues = new double[points.size() * 2];
    for (var index = 0; index < points.size(); index++) {
      var point = Objects.requireNonNull(points.get(index), "point");
      pointValues[index * 2] = point.x();
      pointValues[index * 2 + 1] = point.y();
    }
    var coordinateValues = new double[pointValues.length];
    Status.check(
        CameraNative.mln_map_lat_lngs_for_pixels(
            state.requireLiveAddress(), pointValues, coordinateValues));
    var coordinates = new java.util.ArrayList<LatLng>(points.size());
    for (var index = 0; index < points.size(); index++) {
      coordinates.add(new LatLng(coordinateValues[index * 2], coordinateValues[index * 2 + 1]));
    }
    return List.copyOf(coordinates);
  }

  static NativeOptions cameraToNative(CameraOptions camera) {
    var cameraValue = MapStructs.cameraOptions(camera);
    var fields = new boolean[CAMERA_FIELD_COUNT];
    var values = new double[CAMERA_VALUE_COUNT];
    fields[0] = cameraValue.hasCenter();
    if (fields[0]) {
      values[0] = cameraValue.center().latitude();
      values[1] = cameraValue.center().longitude();
    }
    fields[1] = cameraValue.hasCenterAltitude();
    values[2] = cameraValue.centerAltitude();
    fields[2] = cameraValue.hasPadding();
    if (fields[2]) {
      values[3] = cameraValue.padding().top();
      values[4] = cameraValue.padding().left();
      values[5] = cameraValue.padding().bottom();
      values[6] = cameraValue.padding().right();
    }
    fields[3] = cameraValue.hasAnchor();
    if (fields[3]) {
      values[7] = cameraValue.anchor().x();
      values[8] = cameraValue.anchor().y();
    }
    fields[4] = cameraValue.hasZoom();
    values[9] = cameraValue.zoom();
    fields[5] = cameraValue.hasBearing();
    values[10] = cameraValue.bearing();
    fields[6] = cameraValue.hasPitch();
    values[11] = cameraValue.pitch();
    fields[7] = cameraValue.hasRoll();
    values[12] = cameraValue.roll();
    fields[8] = cameraValue.hasFieldOfView();
    values[13] = cameraValue.fieldOfView();
    return new NativeOptions(fields, values);
  }

  static CameraOptions cameraFromNative(boolean[] fields, double[] values) {
    var camera = new CameraOptions();
    if (fields[0]) {
      camera.center(values[0], values[1]);
    }
    if (fields[1]) {
      camera.centerAltitude(values[2]);
    }
    if (fields[2]) {
      camera.padding(new EdgeInsets(values[3], values[4], values[5], values[6]));
    }
    if (fields[3]) {
      camera.anchor(new ScreenPoint(values[7], values[8]));
    }
    if (fields[4]) {
      camera.zoom(values[9]);
    }
    if (fields[5]) {
      camera.bearing(values[10]);
    }
    if (fields[6]) {
      camera.pitch(values[11]);
    }
    if (fields[7]) {
      camera.roll(values[12]);
    }
    if (fields[8]) {
      camera.fieldOfView(values[13]);
    }
    return camera;
  }

  private static NativeOptions fitToNative(CameraFitOptions options, boolean hasOptions) {
    var fields = new boolean[FIT_FIELD_COUNT];
    var values = new double[FIT_VALUE_COUNT];
    if (hasOptions) {
      var fitOptions = MapStructs.cameraFitOptions(options);
      fields[0] = fitOptions.hasPadding();
      if (fields[0]) {
        values[0] = fitOptions.padding().top();
        values[1] = fitOptions.padding().left();
        values[2] = fitOptions.padding().bottom();
        values[3] = fitOptions.padding().right();
      }
      fields[1] = fitOptions.hasBearing();
      values[4] = fitOptions.bearing();
      fields[2] = fitOptions.hasPitch();
      values[5] = fitOptions.pitch();
    }
    return new NativeOptions(fields, values);
  }

  private static NativeOptions boundsToNative(BoundOptions options) {
    var boundsValue = MapStructs.boundOptions(options);
    var fields = new boolean[BOUND_FIELD_COUNT];
    var values = new double[BOUND_VALUE_COUNT];
    fields[0] = boundsValue.hasBounds();
    if (fields[0]) {
      values[0] = boundsValue.bounds().southwest().latitude();
      values[1] = boundsValue.bounds().southwest().longitude();
      values[2] = boundsValue.bounds().northeast().latitude();
      values[3] = boundsValue.bounds().northeast().longitude();
    }
    fields[1] = boundsValue.hasMinZoom();
    values[4] = boundsValue.minZoom();
    fields[2] = boundsValue.hasMaxZoom();
    values[5] = boundsValue.maxZoom();
    fields[3] = boundsValue.hasMinPitch();
    values[6] = boundsValue.minPitch();
    fields[4] = boundsValue.hasMaxPitch();
    values[7] = boundsValue.maxPitch();
    return new NativeOptions(fields, values);
  }

  private static BoundOptions boundsFromNative(boolean[] fields, double[] values) {
    var options = new BoundOptions();
    if (fields[0]) {
      options.bounds(
          new LatLngBounds(new LatLng(values[0], values[1]), new LatLng(values[2], values[3])));
    }
    if (fields[1]) {
      options.minZoom(values[4]);
    }
    if (fields[2]) {
      options.maxZoom(values[5]);
    }
    if (fields[3]) {
      options.minPitch(values[6]);
    }
    if (fields[4]) {
      options.maxPitch(values[7]);
    }
    return options;
  }

  private static NativeOptions freeCameraToNative(FreeCameraOptions options) {
    var freeCamera = MapStructs.freeCameraOptions(options);
    var fields = new boolean[FREE_CAMERA_FIELD_COUNT];
    var values = new double[FREE_CAMERA_VALUE_COUNT];
    fields[0] = freeCamera.hasPosition();
    if (fields[0]) {
      values[0] = freeCamera.position().x();
      values[1] = freeCamera.position().y();
      values[2] = freeCamera.position().z();
    }
    fields[1] = freeCamera.hasOrientation();
    if (fields[1]) {
      values[3] = freeCamera.orientation().x();
      values[4] = freeCamera.orientation().y();
      values[5] = freeCamera.orientation().z();
      values[6] = freeCamera.orientation().w();
    }
    return new NativeOptions(fields, values);
  }

  private static FreeCameraOptions freeCameraFromNative(boolean[] fields, double[] values) {
    var options = new FreeCameraOptions();
    if (fields[0]) {
      options.position(new Vec3(values[0], values[1], values[2]));
    }
    if (fields[1]) {
      options.orientation(new Quaternion(values[3], values[4], values[5], values[6]));
    }
    return options;
  }

  private static ProjectionModeOptions projectionModeFromNative(
      boolean[] fields, boolean[] booleans, double[] values) {
    var mode = new ProjectionModeOptions();
    if (fields[0]) {
      mode.axonometric(booleans[0]);
    }
    if (fields[1]) {
      mode.xSkew(values[0]);
    }
    if (fields[2]) {
      mode.ySkew(values[1]);
    }
    return mode;
  }

  private static NativeOptions animationToNative(AnimationOptions animation, boolean hasAnimation) {
    var fields = new boolean[ANIMATION_FIELD_COUNT];
    var values = new double[ANIMATION_VALUE_COUNT];
    if (hasAnimation) {
      var animationValue = MapStructs.animationOptions(animation);
      fields[0] = animationValue.hasDurationMs();
      values[0] = animationValue.durationMs();
      fields[1] = animationValue.hasVelocity();
      values[1] = animationValue.velocity();
      fields[2] = animationValue.hasMinZoom();
      values[2] = animationValue.minZoom();
      fields[3] = animationValue.hasEasing();
      if (fields[3]) {
        values[3] = animationValue.easing().x1();
        values[4] = animationValue.easing().y1();
        values[5] = animationValue.easing().x2();
        values[6] = animationValue.easing().y2();
      }
    }
    return new NativeOptions(fields, values);
  }

  record NativeOptions(boolean[] fields, double[] values) {}

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
