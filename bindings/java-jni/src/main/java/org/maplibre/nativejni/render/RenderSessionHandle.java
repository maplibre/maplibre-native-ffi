package org.maplibre.nativejni.render;

import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import org.bytedeco.javacpp.PointerPointer;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.geo.Feature;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.JavaCppValues;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.lifecycle.HandleState;
import org.maplibre.nativejni.internal.loader.NativeLibrary;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.internal.struct.QueryStructs;
import org.maplibre.nativejni.internal.struct.RenderStructs;
import org.maplibre.nativejni.json.JsonValue;
import org.maplibre.nativejni.map.MapHandle;
import org.maplibre.nativejni.query.FeatureExtensionResult;
import org.maplibre.nativejni.query.FeatureStateSelector;
import org.maplibre.nativejni.query.QueriedFeature;
import org.maplibre.nativejni.query.RenderedFeatureQueryOptions;
import org.maplibre.nativejni.query.RenderedQueryGeometry;
import org.maplibre.nativejni.query.SourceFeatureQueryOptions;

/** Owned native render session handle. Close it on the map owner thread. */
public final class RenderSessionHandle implements AutoCloseable {
  private final MapHandle map;
  private final HandleState state;

  private RenderSessionHandle(MapHandle map, long handle) {
    this.map = Objects.requireNonNull(map, "map");
    this.state = new HandleState("RenderSessionHandle", handle, map);
  }

  public static RenderSessionHandle attachMetalOwnedTexture(
      MapHandle map, MetalOwnedTextureDescriptor descriptor) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(map, "map");
    var outSession = JavaCppSupport.outPointer(MaplibreNativeC.mln_render_session.class);
    Status.check(
        MaplibreNativeC.mln_metal_owned_texture_attach(
            JavaCppSupport.map(map.nativeAddress(InternalAccess.INSTANCE)),
            RenderStructs.nativeMetalOwnedTextureDescriptor(descriptor),
            outSession));
    return new RenderSessionHandle(map, sessionAddress(outSession));
  }

  public static RenderSessionHandle attachMetalBorrowedTexture(
      MapHandle map, MetalBorrowedTextureDescriptor descriptor) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(map, "map");
    var outSession = JavaCppSupport.outPointer(MaplibreNativeC.mln_render_session.class);
    Status.check(
        MaplibreNativeC.mln_metal_borrowed_texture_attach(
            JavaCppSupport.map(map.nativeAddress(InternalAccess.INSTANCE)),
            RenderStructs.nativeMetalBorrowedTextureDescriptor(descriptor),
            outSession));
    return new RenderSessionHandle(map, sessionAddress(outSession));
  }

  public static RenderSessionHandle attachVulkanOwnedTexture(
      MapHandle map, VulkanOwnedTextureDescriptor descriptor) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(map, "map");
    var outSession = JavaCppSupport.outPointer(MaplibreNativeC.mln_render_session.class);
    Status.check(
        MaplibreNativeC.mln_vulkan_owned_texture_attach(
            JavaCppSupport.map(map.nativeAddress(InternalAccess.INSTANCE)),
            RenderStructs.nativeVulkanOwnedTextureDescriptor(descriptor),
            outSession));
    return new RenderSessionHandle(map, sessionAddress(outSession));
  }

  public static RenderSessionHandle attachVulkanBorrowedTexture(
      MapHandle map, VulkanBorrowedTextureDescriptor descriptor) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(map, "map");
    var outSession = JavaCppSupport.outPointer(MaplibreNativeC.mln_render_session.class);
    Status.check(
        MaplibreNativeC.mln_vulkan_borrowed_texture_attach(
            JavaCppSupport.map(map.nativeAddress(InternalAccess.INSTANCE)),
            RenderStructs.nativeVulkanBorrowedTextureDescriptor(descriptor),
            outSession));
    return new RenderSessionHandle(map, sessionAddress(outSession));
  }

  public static RenderSessionHandle attachOpenGLOwnedTexture(
      MapHandle map, OpenGLOwnedTextureDescriptor descriptor) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(map, "map");
    var outSession = JavaCppSupport.outPointer(MaplibreNativeC.mln_render_session.class);
    Status.check(
        MaplibreNativeC.mln_opengl_owned_texture_attach(
            JavaCppSupport.map(map.nativeAddress(InternalAccess.INSTANCE)),
            RenderStructs.nativeOpenGLOwnedTextureDescriptor(descriptor),
            outSession));
    return new RenderSessionHandle(map, sessionAddress(outSession));
  }

  public static RenderSessionHandle attachOpenGLBorrowedTexture(
      MapHandle map, OpenGLBorrowedTextureDescriptor descriptor) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(map, "map");
    var outSession = JavaCppSupport.outPointer(MaplibreNativeC.mln_render_session.class);
    Status.check(
        MaplibreNativeC.mln_opengl_borrowed_texture_attach(
            JavaCppSupport.map(map.nativeAddress(InternalAccess.INSTANCE)),
            RenderStructs.nativeOpenGLBorrowedTextureDescriptor(descriptor),
            outSession));
    return new RenderSessionHandle(map, sessionAddress(outSession));
  }

  public static RenderSessionHandle attachMetalSurface(
      MapHandle map, MetalSurfaceDescriptor descriptor) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(map, "map");
    var outSession = JavaCppSupport.outPointer(MaplibreNativeC.mln_render_session.class);
    Status.check(
        MaplibreNativeC.mln_metal_surface_attach(
            JavaCppSupport.map(map.nativeAddress(InternalAccess.INSTANCE)),
            RenderStructs.nativeMetalSurfaceDescriptor(descriptor),
            outSession));
    return new RenderSessionHandle(map, sessionAddress(outSession));
  }

  public static RenderSessionHandle attachVulkanSurface(
      MapHandle map, VulkanSurfaceDescriptor descriptor) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(map, "map");
    var outSession = JavaCppSupport.outPointer(MaplibreNativeC.mln_render_session.class);
    Status.check(
        MaplibreNativeC.mln_vulkan_surface_attach(
            JavaCppSupport.map(map.nativeAddress(InternalAccess.INSTANCE)),
            RenderStructs.nativeVulkanSurfaceDescriptor(descriptor),
            outSession));
    return new RenderSessionHandle(map, sessionAddress(outSession));
  }

  public static RenderSessionHandle attachOpenGLSurface(
      MapHandle map, OpenGLSurfaceDescriptor descriptor) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(map, "map");
    var outSession = JavaCppSupport.outPointer(MaplibreNativeC.mln_render_session.class);
    Status.check(
        MaplibreNativeC.mln_opengl_surface_attach(
            JavaCppSupport.map(map.nativeAddress(InternalAccess.INSTANCE)),
            RenderStructs.nativeOpenGLSurfaceDescriptor(descriptor),
            outSession));
    return new RenderSessionHandle(map, sessionAddress(outSession));
  }

  public void resize(int width, int height, double scaleFactor) {
    NativeLibrary.ensureLoaded();
    if (width < 0 || height < 0) {
      JavaCppSupport.setThreadDiagnostic("render target width and height must be non-negative");
      Status.check(MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT);
    }
    if (!Double.isFinite(scaleFactor) || scaleFactor <= 0.0) {
      JavaCppSupport.setThreadDiagnostic("render target scale factor must be positive and finite");
      Status.check(MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT);
    }
    Status.check(
        MaplibreNativeC.mln_render_session_resize(
            JavaCppSupport.renderSession(state.requireLiveAddress()), width, height, scaleFactor));
  }

  public void renderUpdate() {
    NativeLibrary.ensureLoaded();
    Status.check(
        MaplibreNativeC.mln_render_session_render_update(
            JavaCppSupport.renderSession(state.requireLiveAddress())));
  }

  public void detach() {
    NativeLibrary.ensureLoaded();
    Status.check(
        MaplibreNativeC.mln_render_session_detach(
            JavaCppSupport.renderSession(state.requireLiveAddress())));
  }

  public void reduceMemoryUse() {
    NativeLibrary.ensureLoaded();
    Status.check(
        MaplibreNativeC.mln_render_session_reduce_memory_use(
            JavaCppSupport.renderSession(state.requireLiveAddress())));
  }

  public void clearData() {
    NativeLibrary.ensureLoaded();
    Status.check(
        MaplibreNativeC.mln_render_session_clear_data(
            JavaCppSupport.renderSession(state.requireLiveAddress())));
  }

  public void dumpDebugLogs() {
    NativeLibrary.ensureLoaded();
    Status.check(
        MaplibreNativeC.mln_render_session_dump_debug_logs(
            JavaCppSupport.renderSession(state.requireLiveAddress())));
  }

  public void setFeatureState(FeatureStateSelector selector, JsonValue value) {
    NativeLibrary.ensureLoaded();
    try (var nativeSelector =
            QueryStructs.featureStateSelector(Objects.requireNonNull(selector, "selector"));
        var nativeValue = JavaCppValues.json(Objects.requireNonNull(value, "value"))) {
      Status.check(
          MaplibreNativeC.mln_render_session_set_feature_state(
              JavaCppSupport.renderSession(state.requireLiveAddress()),
              nativeSelector.selector(),
              nativeValue.value()));
    }
  }

  public JsonValue getFeatureState(FeatureStateSelector selector) {
    NativeLibrary.ensureLoaded();
    try (var nativeSelector =
        QueryStructs.featureStateSelector(Objects.requireNonNull(selector, "selector"))) {
      var outState = JavaCppSupport.outPointer(MaplibreNativeC.mln_json_snapshot.class);
      Status.check(
          MaplibreNativeC.mln_render_session_get_feature_state(
              JavaCppSupport.renderSession(state.requireLiveAddress()),
              nativeSelector.selector(),
              outState));
      var jsonSnapshot =
          QueryStructs.jsonSnapshot(
              JavaCppSupport.outAddress(outState, MaplibreNativeC.mln_json_snapshot.class));
      return jsonSnapshot == null ? JsonValue.object(List.of()) : jsonSnapshot;
    }
  }

  public void removeFeatureState(FeatureStateSelector selector) {
    NativeLibrary.ensureLoaded();
    try (var nativeSelector =
        QueryStructs.featureStateSelector(Objects.requireNonNull(selector, "selector"))) {
      Status.check(
          MaplibreNativeC.mln_render_session_remove_feature_state(
              JavaCppSupport.renderSession(state.requireLiveAddress()), nativeSelector.selector()));
    }
  }

  public List<QueriedFeature> queryRenderedFeatures(RenderedQueryGeometry geometry) {
    return queryRenderedFeaturesInternal(geometry, null);
  }

  public List<QueriedFeature> queryRenderedFeatures(
      RenderedQueryGeometry geometry, RenderedFeatureQueryOptions options) {
    return queryRenderedFeaturesInternal(geometry, Objects.requireNonNull(options, "options"));
  }

  public List<QueriedFeature> querySourceFeatures(String sourceId) {
    return querySourceFeaturesInternal(sourceId, null);
  }

  public List<QueriedFeature> querySourceFeatures(
      String sourceId, SourceFeatureQueryOptions options) {
    return querySourceFeaturesInternal(sourceId, Objects.requireNonNull(options, "options"));
  }

  public FeatureExtensionResult queryFeatureExtension(
      String sourceId, Feature feature, String extension, String extensionField) {
    return queryFeatureExtension(sourceId, feature, extension, extensionField, null);
  }

  public FeatureExtensionResult queryFeatureExtension(
      String sourceId,
      Feature feature,
      String extension,
      String extensionField,
      JsonValue arguments) {
    NativeLibrary.ensureLoaded();
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeFeature =
            new QueryStructs.FeatureScope(Objects.requireNonNull(feature, "feature"));
        var nativeExtension =
            JavaCppValues.stringView(Objects.requireNonNull(extension, "extension"));
        var nativeExtensionField =
            JavaCppValues.stringView(Objects.requireNonNull(extensionField, "extensionField"));
        var nativeArguments = arguments == null ? null : JavaCppValues.json(arguments)) {
      var outResult = JavaCppSupport.outPointer(MaplibreNativeC.mln_feature_extension_result.class);
      Status.check(
          MaplibreNativeC.mln_render_session_query_feature_extensions(
              JavaCppSupport.renderSession(state.requireLiveAddress()),
              source.view(),
              nativeFeature.feature(),
              nativeExtension.view(),
              nativeExtensionField.view(),
              nativeArguments == null ? null : nativeArguments.value(),
              outResult));
      return QueryStructs.featureExtensionResult(outResult);
    }
  }

  public TextureImageInfo textureImageInfo() {
    NativeLibrary.ensureLoaded();
    var outInfo = MaplibreNativeC.mln_texture_image_info_default();
    var status =
        MaplibreNativeC.mln_texture_read_premultiplied_rgba8(
            JavaCppSupport.renderSession(state.requireLiveAddress()),
            (org.bytedeco.javacpp.BytePointer) null,
            0,
            outInfo);
    var info = RenderStructs.textureImageInfo(outInfo);
    if (status == MaplibreStatus.OK.nativeCode()
        || (status == MaplibreStatus.INVALID_ARGUMENT.nativeCode() && info.byteLength() > 0)) {
      return info;
    }
    Status.check(status);
    throw new AssertionError("unreachable");
  }

  /**
   * Reads into a caller-owned buffer.
   *
   * <p>Use {@link #textureImageInfo()} first when sizing reusable buffers; undersized buffers throw
   * {@link org.maplibre.nativejni.error.InvalidArgumentException} after native reports the required
   * layout.
   */
  public TextureImageInfo readPremultipliedRgba8(NativeBuffer buffer) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(buffer, "buffer");
    synchronized (buffer) {
      var capacity = buffer.byteLength();
      var outInfo = MaplibreNativeC.mln_texture_image_info_default();
      Status.check(
          MaplibreNativeC.mln_texture_read_premultiplied_rgba8(
              JavaCppSupport.renderSession(state.requireLiveAddress()),
              buffer.borrowBuffer(),
              capacity,
              outInfo));
      return RenderStructs.textureImageInfo(outInfo);
    }
  }

  public PremultipliedRgba8Image readPremultipliedRgba8() {
    var info = textureImageInfo();
    try (var buffer = NativeBuffer.allocate(info.byteLength())) {
      var readInfo = readPremultipliedRgba8(buffer);
      return new PremultipliedRgba8Image(
          readInfo.width(),
          readInfo.height(),
          readInfo.stride(),
          Arrays.copyOf(buffer.toByteArray(), Math.toIntExact(readInfo.byteLength())));
    }
  }

  /**
   * Acquires an explicit Metal session-owned texture frame handle.
   *
   * <p>This advanced API is intended for integrations that submit GPU work using the returned
   * texture and need to release it after that work completes. The returned handle must be closed on
   * the render session owner thread after GPU work using {@link MetalOwnedTextureFrame#texture()}
   * has completed. While the handle is open, the native session rejects resize, render, detach,
   * destroy, and second-acquire operations.
   */
  public MetalOwnedTextureFrameHandle acquireMetalOwnedTextureFrame() {
    NativeLibrary.ensureLoaded();
    var nativeFrame = new MaplibreNativeC.mln_metal_owned_texture_frame();
    nativeFrame.size(nativeFrame.sizeof());
    Status.check(
        MaplibreNativeC.mln_metal_owned_texture_acquire_frame(
            JavaCppSupport.renderSession(state.requireLiveAddress()), nativeFrame));
    var scope = new FrameScope();
    return new MetalOwnedTextureFrameHandle(
        this,
        nativeFrame,
        scope,
        new MetalOwnedTextureFrame(
            scope,
            nativeFrame.generation(),
            nativeFrame.width(),
            nativeFrame.height(),
            nativeFrame.scale_factor(),
            nativeFrame.frame_id(),
            NativePointer.scoped(address(nativeFrame.texture()), scope),
            NativePointer.scoped(address(nativeFrame.device()), scope),
            nativeFrame.pixel_format()));
  }

  /**
   * Acquires an explicit Vulkan session-owned texture frame handle.
   *
   * <p>This advanced API is intended for integrations that submit GPU work using the returned image
   * and need to release it after an external fence signals. The returned handle must be closed on
   * the render session owner thread after GPU work using {@link VulkanOwnedTextureFrame#image()} or
   * {@link VulkanOwnedTextureFrame#imageView()} has completed. While the handle is open, the native
   * session rejects resize, render, detach, destroy, and second-acquire operations.
   */
  public VulkanOwnedTextureFrameHandle acquireVulkanOwnedTextureFrame() {
    NativeLibrary.ensureLoaded();
    var nativeFrame = new MaplibreNativeC.mln_vulkan_owned_texture_frame();
    nativeFrame.size(nativeFrame.sizeof());
    Status.check(
        MaplibreNativeC.mln_vulkan_owned_texture_acquire_frame(
            JavaCppSupport.renderSession(state.requireLiveAddress()), nativeFrame));
    var scope = new FrameScope();
    return new VulkanOwnedTextureFrameHandle(
        this,
        nativeFrame,
        scope,
        new VulkanOwnedTextureFrame(
            scope,
            nativeFrame.generation(),
            nativeFrame.width(),
            nativeFrame.height(),
            nativeFrame.scale_factor(),
            nativeFrame.frame_id(),
            NativePointer.scoped(address(nativeFrame.image()), scope),
            NativePointer.scoped(address(nativeFrame.image_view()), scope),
            NativePointer.scoped(address(nativeFrame.device()), scope),
            nativeFrame.format(),
            nativeFrame.layout()));
  }

  /**
   * Acquires an explicit OpenGL session-owned texture frame handle.
   *
   * <p>This advanced API is intended for integrations that submit GPU work using the returned
   * texture and need to release it after that work completes. The returned handle must be closed on
   * the render session owner thread after GPU work using {@link OpenGLOwnedTextureFrame#texture()}
   * has completed. While the handle is open, the native session rejects resize, render, detach,
   * destroy, and second-acquire operations.
   */
  public OpenGLOwnedTextureFrameHandle acquireOpenGLOwnedTextureFrame() {
    NativeLibrary.ensureLoaded();
    var nativeFrame = new MaplibreNativeC.mln_opengl_owned_texture_frame();
    nativeFrame.size(nativeFrame.sizeof());
    Status.check(
        MaplibreNativeC.mln_opengl_owned_texture_acquire_frame(
            JavaCppSupport.renderSession(state.requireLiveAddress()), nativeFrame));
    var scope = new FrameScope();
    return new OpenGLOwnedTextureFrameHandle(
        this,
        nativeFrame,
        scope,
        new OpenGLOwnedTextureFrame(
            scope,
            nativeFrame.generation(),
            nativeFrame.width(),
            nativeFrame.height(),
            nativeFrame.scale_factor(),
            nativeFrame.frame_id(),
            nativeFrame.texture(),
            nativeFrame.target(),
            nativeFrame.internal_format(),
            nativeFrame.format(),
            nativeFrame.type()));
  }

  private static long sessionAddress(
      PointerPointer<MaplibreNativeC.mln_render_session> outSession) {
    return JavaCppSupport.outAddress(outSession, MaplibreNativeC.mln_render_session.class);
  }

  private static long address(org.bytedeco.javacpp.Pointer pointer) {
    return pointer == null || pointer.isNull() ? 0 : pointer.address();
  }

  private List<QueriedFeature> queryRenderedFeaturesInternal(
      RenderedQueryGeometry geometry, RenderedFeatureQueryOptions options) {
    NativeLibrary.ensureLoaded();
    try (var nativeGeometry =
            new QueryStructs.RenderedGeometryScope(Objects.requireNonNull(geometry, "geometry"));
        var nativeOptions = new QueryStructs.RenderedOptionsScope(options)) {
      var outResult = JavaCppSupport.outPointer(MaplibreNativeC.mln_feature_query_result.class);
      Status.check(
          MaplibreNativeC.mln_render_session_query_rendered_features(
              JavaCppSupport.renderSession(state.requireLiveAddress()),
              nativeGeometry.geometry(),
              nativeOptions.options(),
              outResult));
      return QueryStructs.featureQueryResult(outResult);
    }
  }

  private List<QueriedFeature> querySourceFeaturesInternal(
      String sourceId, SourceFeatureQueryOptions options) {
    NativeLibrary.ensureLoaded();
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeOptions = new QueryStructs.SourceOptionsScope(options)) {
      var outResult = JavaCppSupport.outPointer(MaplibreNativeC.mln_feature_query_result.class);
      Status.check(
          MaplibreNativeC.mln_render_session_query_source_features(
              JavaCppSupport.renderSession(state.requireLiveAddress()),
              source.view(),
              nativeOptions.options(),
              outResult));
      return QueryStructs.featureQueryResult(outResult);
    }
  }

  public void close() {
    NativeLibrary.ensureLoaded();
    state.closeOnce(
        address ->
            MaplibreNativeC.mln_render_session_destroy(JavaCppSupport.renderSession(address)));
  }

  public boolean isClosed() {
    return state.isReleased();
  }

  public MapHandle map() {
    return map;
  }

  long nativeAddress() {
    return state.requireLiveAddress();
  }

  void releaseMetalFrame(
      MaplibreNativeC.mln_metal_owned_texture_frame frame, Throwable callbackFailure) {
    try {
      Status.check(
          MaplibreNativeC.mln_metal_owned_texture_release_frame(
              JavaCppSupport.renderSession(state.requireLiveAddress()), frame));
    } catch (Throwable releaseFailure) {
      if (callbackFailure != null) {
        callbackFailure.addSuppressed(releaseFailure);
      } else {
        throw releaseFailure;
      }
    }
  }

  void releaseVulkanFrame(
      MaplibreNativeC.mln_vulkan_owned_texture_frame frame, Throwable callbackFailure) {
    try {
      Status.check(
          MaplibreNativeC.mln_vulkan_owned_texture_release_frame(
              JavaCppSupport.renderSession(state.requireLiveAddress()), frame));
    } catch (Throwable releaseFailure) {
      if (callbackFailure != null) {
        callbackFailure.addSuppressed(releaseFailure);
      } else {
        throw releaseFailure;
      }
    }
  }

  void releaseOpenGLFrame(
      MaplibreNativeC.mln_opengl_owned_texture_frame frame, Throwable callbackFailure) {
    try {
      Status.check(
          MaplibreNativeC.mln_opengl_owned_texture_release_frame(
              JavaCppSupport.renderSession(state.requireLiveAddress()), frame));
    } catch (Throwable releaseFailure) {
      if (callbackFailure != null) {
        callbackFailure.addSuppressed(releaseFailure);
      } else {
        throw releaseFailure;
      }
    }
  }
}
