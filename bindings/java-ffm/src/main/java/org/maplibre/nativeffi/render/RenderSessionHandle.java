package org.maplibre.nativeffi.render;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.util.List;
import java.util.Objects;
import java.util.function.Consumer;
import java.util.function.Function;
import org.maplibre.nativeffi.error.InvalidArgumentException;
import org.maplibre.nativeffi.error.MaplibreStatus;
import org.maplibre.nativeffi.geo.Feature;
import org.maplibre.nativeffi.internal.access.InternalAccess;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_frame;
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_frame;
import org.maplibre.nativeffi.internal.lifecycle.HandleState;
import org.maplibre.nativeffi.internal.loader.NativeAccess;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.internal.status.Status;
import org.maplibre.nativeffi.internal.struct.CoreStructs;
import org.maplibre.nativeffi.internal.struct.QueryStructs;
import org.maplibre.nativeffi.internal.struct.RenderStructs;
import org.maplibre.nativeffi.internal.struct.ValueStructs;
import org.maplibre.nativeffi.json.JsonValue;
import org.maplibre.nativeffi.map.MapHandle;
import org.maplibre.nativeffi.query.FeatureExtensionResult;
import org.maplibre.nativeffi.query.FeatureStateSelector;
import org.maplibre.nativeffi.query.QueriedFeature;
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions;
import org.maplibre.nativeffi.query.RenderedQueryGeometry;
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions;

/** Owned native render session handle. Close it on the map owner thread. */
public final class RenderSessionHandle implements AutoCloseable {
  private final MapHandle map;
  private final HandleState state;

  private RenderSessionHandle(MapHandle map, MemorySegment handle) {
    this.map = Objects.requireNonNull(map, "map");
    this.state = new HandleState("RenderSessionHandle", handle, map);
  }

  public static RenderSessionHandle attachOwnedTexture(
      MapHandle map, OwnedTextureDescriptor descriptor) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(map, "map");
    Objects.requireNonNull(descriptor, "descriptor");
    try (var arena = Arena.ofConfined()) {
      var outSession = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_owned_texture_attach(
              map.nativeHandle(InternalAccess.INSTANCE),
              RenderStructs.ownedTextureDescriptor(descriptor, arena),
              outSession));
      return new RenderSessionHandle(map, outSession.get(ValueLayout.ADDRESS, 0));
    }
  }

  public static RenderSessionHandle attachMetalOwnedTexture(
      MapHandle map, MetalOwnedTextureDescriptor descriptor) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(map, "map");
    Objects.requireNonNull(descriptor, "descriptor");
    try (var arena = Arena.ofConfined()) {
      var outSession = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_metal_owned_texture_attach(
              map.nativeHandle(InternalAccess.INSTANCE),
              RenderStructs.metalOwnedTextureDescriptor(descriptor, arena),
              outSession));
      return new RenderSessionHandle(map, outSession.get(ValueLayout.ADDRESS, 0));
    }
  }

  public static RenderSessionHandle attachMetalBorrowedTexture(
      MapHandle map, MetalBorrowedTextureDescriptor descriptor) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(map, "map");
    Objects.requireNonNull(descriptor, "descriptor");
    try (var arena = Arena.ofConfined()) {
      var outSession = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_metal_borrowed_texture_attach(
              map.nativeHandle(InternalAccess.INSTANCE),
              RenderStructs.metalBorrowedTextureDescriptor(descriptor, arena),
              outSession));
      return new RenderSessionHandle(map, outSession.get(ValueLayout.ADDRESS, 0));
    }
  }

  public static RenderSessionHandle attachVulkanOwnedTexture(
      MapHandle map, VulkanOwnedTextureDescriptor descriptor) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(map, "map");
    Objects.requireNonNull(descriptor, "descriptor");
    try (var arena = Arena.ofConfined()) {
      var outSession = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_vulkan_owned_texture_attach(
              map.nativeHandle(InternalAccess.INSTANCE),
              RenderStructs.vulkanOwnedTextureDescriptor(descriptor, arena),
              outSession));
      return new RenderSessionHandle(map, outSession.get(ValueLayout.ADDRESS, 0));
    }
  }

  public static RenderSessionHandle attachVulkanBorrowedTexture(
      MapHandle map, VulkanBorrowedTextureDescriptor descriptor) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(map, "map");
    Objects.requireNonNull(descriptor, "descriptor");
    try (var arena = Arena.ofConfined()) {
      var outSession = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_vulkan_borrowed_texture_attach(
              map.nativeHandle(InternalAccess.INSTANCE),
              RenderStructs.vulkanBorrowedTextureDescriptor(descriptor, arena),
              outSession));
      return new RenderSessionHandle(map, outSession.get(ValueLayout.ADDRESS, 0));
    }
  }

  public static RenderSessionHandle attachMetalSurface(
      MapHandle map, MetalSurfaceDescriptor descriptor) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(map, "map");
    Objects.requireNonNull(descriptor, "descriptor");
    try (var arena = Arena.ofConfined()) {
      var outSession = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_metal_surface_attach(
              map.nativeHandle(InternalAccess.INSTANCE),
              RenderStructs.metalSurfaceDescriptor(descriptor, arena),
              outSession));
      return new RenderSessionHandle(map, outSession.get(ValueLayout.ADDRESS, 0));
    }
  }

  public static RenderSessionHandle attachVulkanSurface(
      MapHandle map, VulkanSurfaceDescriptor descriptor) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(map, "map");
    Objects.requireNonNull(descriptor, "descriptor");
    try (var arena = Arena.ofConfined()) {
      var outSession = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_vulkan_surface_attach(
              map.nativeHandle(InternalAccess.INSTANCE),
              RenderStructs.vulkanSurfaceDescriptor(descriptor, arena),
              outSession));
      return new RenderSessionHandle(map, outSession.get(ValueLayout.ADDRESS, 0));
    }
  }

  public void resize(int width, int height, double scaleFactor) {
    NativeAccess.ensureLoaded();
    Status.check(
        MapLibreNativeC.mln_render_session_resize(state.requireLive(), width, height, scaleFactor));
  }

  public void renderUpdate() {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_render_session_render_update(state.requireLive()));
  }

  public void detach() {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_render_session_detach(state.requireLive()));
  }

  public void reduceMemoryUse() {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_render_session_reduce_memory_use(state.requireLive()));
  }

  public void clearData() {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_render_session_clear_data(state.requireLive()));
  }

  public void dumpDebugLogs() {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_render_session_dump_debug_logs(state.requireLive()));
  }

  public void setFeatureState(FeatureStateSelector selector, JsonValue value) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(selector, "selector");
    Objects.requireNonNull(value, "value");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_render_session_set_feature_state(
              state.requireLive(),
              QueryStructs.featureStateSelector(selector, arena),
              ValueStructs.jsonValue(value, arena)));
    }
  }

  public JsonValue getFeatureState(FeatureStateSelector selector) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(selector, "selector");
    try (var arena = Arena.ofConfined()) {
      var outState = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_render_session_get_feature_state(
              state.requireLive(), QueryStructs.featureStateSelector(selector, arena), outState));
      return ValueStructs.jsonSnapshot(outState.get(ValueLayout.ADDRESS, 0))
          .orElseGet(() -> JsonValue.object(List.of()));
    }
  }

  public void removeFeatureState(FeatureStateSelector selector) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(selector, "selector");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_render_session_remove_feature_state(
              state.requireLive(), QueryStructs.featureStateSelector(selector, arena)));
    }
  }

  public List<QueriedFeature> queryRenderedFeatures(RenderedQueryGeometry geometry) {
    return queryRenderedFeaturesInternal(geometry, null, false);
  }

  public List<QueriedFeature> queryRenderedFeatures(
      RenderedQueryGeometry geometry, RenderedFeatureQueryOptions options) {
    return queryRenderedFeaturesInternal(
        geometry, Objects.requireNonNull(options, "options"), true);
  }

  public List<QueriedFeature> querySourceFeatures(String sourceId) {
    return querySourceFeaturesInternal(sourceId, null, false);
  }

  public List<QueriedFeature> querySourceFeatures(
      String sourceId, SourceFeatureQueryOptions options) {
    return querySourceFeaturesInternal(sourceId, Objects.requireNonNull(options, "options"), true);
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
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(feature, "feature");
    try (var arena = Arena.ofConfined()) {
      var outResult = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_render_session_query_feature_extensions(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              ValueStructs.feature(feature, arena),
              CoreStructs.stringView(Objects.requireNonNull(extension, "extension"), arena),
              CoreStructs.stringView(
                  Objects.requireNonNull(extensionField, "extensionField"), arena),
              arguments == null ? MemorySegment.NULL : ValueStructs.jsonValue(arguments, arena),
              outResult));
      return QueryStructs.featureExtensionResult(outResult.get(ValueLayout.ADDRESS, 0));
    }
  }

  public TextureImageInfo textureImageInfo() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outInfo = MapLibreNativeC.mln_texture_image_info_default(arena);
      var status =
          MapLibreNativeC.mln_texture_read_premultiplied_rgba8(
              state.requireLive(), MemorySegment.NULL, 0, outInfo);
      var info = RenderStructs.textureImageInfo(outInfo);
      if (status == MaplibreStatus.OK.nativeCode()
          || (status == MaplibreStatus.INVALID_ARGUMENT.nativeCode() && info.byteLength() > 0)) {
        return info;
      }
      Status.check(status);
      throw new AssertionError("unreachable");
    }
  }

  /**
   * Reads into a caller-owned buffer.
   *
   * <p>Use {@link #textureImageInfo()} first when sizing reusable buffers; undersized buffers throw
   * {@link InvalidArgumentException} after native reports the required layout.
   */
  public TextureImageInfo readPremultipliedRgba8(NativeBuffer buffer) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(buffer, "buffer");
    synchronized (buffer) {
      try (var arena = Arena.ofConfined()) {
        var capacity = buffer.byteLength();
        var outInfo = MapLibreNativeC.mln_texture_image_info_default(arena);
        Status.check(
            MapLibreNativeC.mln_texture_read_premultiplied_rgba8(
                state.requireLive(),
                capacity == 0 ? MemorySegment.NULL : buffer.segment(),
                capacity,
                outInfo));
        return RenderStructs.textureImageInfo(outInfo);
      }
    }
  }

  public PremultipliedRgba8Image readPremultipliedRgba8() {
    var info = textureImageInfo();
    try (var buffer = NativeBuffer.allocate(info.byteLength())) {
      var readInfo = readPremultipliedRgba8(buffer);
      return new PremultipliedRgba8Image(
          readInfo.width(), readInfo.height(), readInfo.stride(), buffer.toByteArray());
    }
  }

  public void withMetalOwnedTextureFrame(Consumer<MetalOwnedTextureFrame> callback) {
    Objects.requireNonNull(callback, "callback");
    withMetalOwnedTextureFrame(
        frame -> {
          callback.accept(frame);
          return null;
        });
  }

  public <T> T withMetalOwnedTextureFrame(Function<MetalOwnedTextureFrame, T> callback) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(callback, "callback");
    try (var arena = Arena.ofConfined()) {
      var frameSegment = RenderStructs.metalOwnedTextureFrame(arena);
      Status.check(
          MapLibreNativeC.mln_metal_owned_texture_acquire_frame(state.requireLive(), frameSegment));
      var scope = new FrameScope();
      Throwable callbackFailure = null;
      try {
        return callback.apply(metalOwnedTextureFrame(frameSegment, scope));
      } catch (Throwable throwable) {
        callbackFailure = throwable;
        throw throwable;
      } finally {
        try {
          releaseMetalFrame(frameSegment, callbackFailure);
        } finally {
          scope.close();
        }
      }
    }
  }

  public void withVulkanOwnedTextureFrame(Consumer<VulkanOwnedTextureFrame> callback) {
    Objects.requireNonNull(callback, "callback");
    withVulkanOwnedTextureFrame(
        frame -> {
          callback.accept(frame);
          return null;
        });
  }

  public <T> T withVulkanOwnedTextureFrame(Function<VulkanOwnedTextureFrame, T> callback) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(callback, "callback");
    try (var arena = Arena.ofConfined()) {
      var frameSegment = RenderStructs.vulkanOwnedTextureFrame(arena);
      Status.check(
          MapLibreNativeC.mln_vulkan_owned_texture_acquire_frame(
              state.requireLive(), frameSegment));
      var scope = new FrameScope();
      Throwable callbackFailure = null;
      try {
        return callback.apply(vulkanOwnedTextureFrame(frameSegment, scope));
      } catch (Throwable throwable) {
        callbackFailure = throwable;
        throw throwable;
      } finally {
        try {
          releaseVulkanFrame(frameSegment, callbackFailure);
        } finally {
          scope.close();
        }
      }
    }
  }

  private List<QueriedFeature> queryRenderedFeaturesInternal(
      RenderedQueryGeometry geometry, RenderedFeatureQueryOptions options, boolean hasOptions) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(geometry, "geometry");
    try (var arena = Arena.ofConfined()) {
      var outResult = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_render_session_query_rendered_features(
              state.requireLive(),
              QueryStructs.renderedQueryGeometry(geometry, arena),
              hasOptions
                  ? QueryStructs.renderedFeatureQueryOptions(options, arena)
                  : MemorySegment.NULL,
              outResult));
      return QueryStructs.featureQueryResult(outResult.get(ValueLayout.ADDRESS, 0));
    }
  }

  private List<QueriedFeature> querySourceFeaturesInternal(
      String sourceId, SourceFeatureQueryOptions options, boolean hasOptions) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outResult = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_render_session_query_source_features(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              hasOptions
                  ? QueryStructs.sourceFeatureQueryOptions(options, arena)
                  : MemorySegment.NULL,
              outResult));
      return QueryStructs.featureQueryResult(outResult.get(ValueLayout.ADDRESS, 0));
    }
  }

  @Override
  public void close() {
    NativeAccess.ensureLoaded();
    state.closeOnce(MapLibreNativeC::mln_render_session_destroy);
  }

  public boolean isClosed() {
    return state.isReleased();
  }

  public MapHandle map() {
    return map;
  }

  MemorySegment nativeHandle() {
    return state.requireLive();
  }

  long nativeAddress() {
    return state.address();
  }

  private static MetalOwnedTextureFrame metalOwnedTextureFrame(
      MemorySegment segment, FrameScope scope) {
    return new MetalOwnedTextureFrame(
        scope,
        mln_metal_owned_texture_frame.generation(segment),
        mln_metal_owned_texture_frame.width(segment),
        mln_metal_owned_texture_frame.height(segment),
        mln_metal_owned_texture_frame.scale_factor(segment),
        mln_metal_owned_texture_frame.frame_id(segment),
        pointer(mln_metal_owned_texture_frame.texture(segment), scope),
        pointer(mln_metal_owned_texture_frame.device(segment), scope),
        mln_metal_owned_texture_frame.pixel_format(segment));
  }

  private static VulkanOwnedTextureFrame vulkanOwnedTextureFrame(
      MemorySegment segment, FrameScope scope) {
    return new VulkanOwnedTextureFrame(
        scope,
        mln_vulkan_owned_texture_frame.generation(segment),
        mln_vulkan_owned_texture_frame.width(segment),
        mln_vulkan_owned_texture_frame.height(segment),
        mln_vulkan_owned_texture_frame.scale_factor(segment),
        mln_vulkan_owned_texture_frame.frame_id(segment),
        pointer(mln_vulkan_owned_texture_frame.image(segment), scope),
        pointer(mln_vulkan_owned_texture_frame.image_view(segment), scope),
        pointer(mln_vulkan_owned_texture_frame.device(segment), scope),
        mln_vulkan_owned_texture_frame.format(segment),
        mln_vulkan_owned_texture_frame.layout(segment));
  }

  private static NativePointer pointer(MemorySegment segment, FrameScope scope) {
    return MemoryUtil.isNull(segment)
        ? NativePointer.NULL
        : NativePointer.scoped(segment.address(), scope);
  }

  private void releaseMetalFrame(MemorySegment frameSegment, Throwable callbackFailure) {
    try {
      Status.check(
          MapLibreNativeC.mln_metal_owned_texture_release_frame(state.requireLive(), frameSegment));
    } catch (Throwable releaseFailure) {
      if (callbackFailure != null) {
        callbackFailure.addSuppressed(releaseFailure);
      } else {
        throw releaseFailure;
      }
    }
  }

  private void releaseVulkanFrame(MemorySegment frameSegment, Throwable callbackFailure) {
    try {
      Status.check(
          MapLibreNativeC.mln_vulkan_owned_texture_release_frame(
              state.requireLive(), frameSegment));
    } catch (Throwable releaseFailure) {
      if (callbackFailure != null) {
        callbackFailure.addSuppressed(releaseFailure);
      } else {
        throw releaseFailure;
      }
    }
  }
}
