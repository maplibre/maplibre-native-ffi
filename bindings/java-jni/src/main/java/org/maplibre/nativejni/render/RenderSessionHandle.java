package org.maplibre.nativejni.render;

import java.lang.foreign.MemorySegment;
import java.util.List;
import java.util.Objects;
import org.maplibre.nativejni.geo.Feature;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.internal.bridge.RenderSessionNative;
import org.maplibre.nativejni.internal.bridge.SurfaceNative;
import org.maplibre.nativejni.internal.bridge.TextureNative;
import org.maplibre.nativejni.internal.lifecycle.HandleState;
import org.maplibre.nativejni.internal.loader.NativeLibrary;
import org.maplibre.nativejni.internal.status.Status;
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

  private static UnsupportedOperationException unsupported() {
    return new UnsupportedOperationException(
        "RenderSessionHandle is not implemented by the JNI bridge yet");
  }

  public static RenderSessionHandle attachMetalOwnedTexture(
      MapHandle map, MetalOwnedTextureDescriptor descriptor) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(map, "map");
    var value = RenderStructs.metalOwnedTextureDescriptor(descriptor);
    var outSession = new long[1];
    Status.check(
        TextureNative.mln_metal_owned_texture_attach(
            map.nativeAddress(InternalAccess.INSTANCE),
            value.extent().width(),
            value.extent().height(),
            value.extent().scaleFactor(),
            value.context().device(),
            outSession));
    return new RenderSessionHandle(map, outSession[0]);
  }

  public static RenderSessionHandle attachMetalBorrowedTexture(
      MapHandle map, MetalBorrowedTextureDescriptor descriptor) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(map, "map");
    var value = RenderStructs.metalBorrowedTextureDescriptor(descriptor);
    var outSession = new long[1];
    Status.check(
        TextureNative.mln_metal_borrowed_texture_attach(
            map.nativeAddress(InternalAccess.INSTANCE),
            value.extent().width(),
            value.extent().height(),
            value.extent().scaleFactor(),
            value.texture(),
            outSession));
    return new RenderSessionHandle(map, outSession[0]);
  }

  public static RenderSessionHandle attachVulkanOwnedTexture(
      MapHandle map, VulkanOwnedTextureDescriptor descriptor) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(map, "map");
    var value = RenderStructs.vulkanOwnedTextureDescriptor(descriptor);
    var context = value.context();
    var outSession = new long[1];
    Status.check(
        TextureNative.mln_vulkan_owned_texture_attach(
            map.nativeAddress(InternalAccess.INSTANCE),
            value.extent().width(),
            value.extent().height(),
            value.extent().scaleFactor(),
            context.instance(),
            context.physicalDevice(),
            context.device(),
            context.graphicsQueue(),
            context.graphicsQueueFamilyIndex(),
            outSession));
    return new RenderSessionHandle(map, outSession[0]);
  }

  public static RenderSessionHandle attachVulkanBorrowedTexture(
      MapHandle map, VulkanBorrowedTextureDescriptor descriptor) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(map, "map");
    var value = RenderStructs.vulkanBorrowedTextureDescriptor(descriptor);
    var context = value.context();
    var outSession = new long[1];
    Status.check(
        TextureNative.mln_vulkan_borrowed_texture_attach(
            map.nativeAddress(InternalAccess.INSTANCE),
            value.extent().width(),
            value.extent().height(),
            value.extent().scaleFactor(),
            context.instance(),
            context.physicalDevice(),
            context.device(),
            context.graphicsQueue(),
            context.graphicsQueueFamilyIndex(),
            value.image(),
            value.imageView(),
            value.format(),
            value.initialLayout(),
            value.finalLayout(),
            outSession));
    return new RenderSessionHandle(map, outSession[0]);
  }

  public static RenderSessionHandle attachMetalSurface(
      MapHandle map, MetalSurfaceDescriptor descriptor) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(map, "map");
    var value = RenderStructs.metalSurfaceDescriptor(descriptor);
    var outSession = new long[1];
    Status.check(
        SurfaceNative.mln_metal_surface_attach(
            map.nativeAddress(InternalAccess.INSTANCE),
            value.extent().width(),
            value.extent().height(),
            value.extent().scaleFactor(),
            value.context().device(),
            value.layer(),
            outSession));
    return new RenderSessionHandle(map, outSession[0]);
  }

  public static RenderSessionHandle attachVulkanSurface(
      MapHandle map, VulkanSurfaceDescriptor descriptor) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(map, "map");
    var value = RenderStructs.vulkanSurfaceDescriptor(descriptor);
    var context = value.context();
    var outSession = new long[1];
    Status.check(
        SurfaceNative.mln_vulkan_surface_attach(
            map.nativeAddress(InternalAccess.INSTANCE),
            value.extent().width(),
            value.extent().height(),
            value.extent().scaleFactor(),
            context.instance(),
            context.physicalDevice(),
            context.device(),
            context.graphicsQueue(),
            context.graphicsQueueFamilyIndex(),
            value.surface(),
            outSession));
    return new RenderSessionHandle(map, outSession[0]);
  }

  public void resize(int width, int height, double scaleFactor) {
    NativeLibrary.ensureLoaded();
    Status.check(
        RenderSessionNative.mln_render_session_resize(
            state.requireLiveAddress(), width, height, scaleFactor));
  }

  public void renderUpdate() {
    NativeLibrary.ensureLoaded();
    Status.check(RenderSessionNative.mln_render_session_render_update(state.requireLiveAddress()));
  }

  public void detach() {
    NativeLibrary.ensureLoaded();
    Status.check(RenderSessionNative.mln_render_session_detach(state.requireLiveAddress()));
  }

  public void reduceMemoryUse() {
    NativeLibrary.ensureLoaded();
    Status.check(
        RenderSessionNative.mln_render_session_reduce_memory_use(state.requireLiveAddress()));
  }

  public void clearData() {
    NativeLibrary.ensureLoaded();
    Status.check(RenderSessionNative.mln_render_session_clear_data(state.requireLiveAddress()));
  }

  public void dumpDebugLogs() {
    NativeLibrary.ensureLoaded();
    Status.check(
        RenderSessionNative.mln_render_session_dump_debug_logs(state.requireLiveAddress()));
  }

  public void setFeatureState(FeatureStateSelector selector, JsonValue value) {
    throw unsupported();
  }

  public JsonValue getFeatureState(FeatureStateSelector selector) {
    throw unsupported();
  }

  public void removeFeatureState(FeatureStateSelector selector) {
    throw unsupported();
  }

  public List<QueriedFeature> queryRenderedFeatures(RenderedQueryGeometry geometry) {
    throw unsupported();
  }

  public List<QueriedFeature> queryRenderedFeatures(
      RenderedQueryGeometry geometry, RenderedFeatureQueryOptions options) {
    throw unsupported();
  }

  public List<QueriedFeature> querySourceFeatures(String sourceId) {
    throw unsupported();
  }

  public List<QueriedFeature> querySourceFeatures(
      String sourceId, SourceFeatureQueryOptions options) {
    throw unsupported();
  }

  public FeatureExtensionResult queryFeatureExtension(
      String sourceId, Feature feature, String extension, String extensionField) {
    throw unsupported();
  }

  public FeatureExtensionResult queryFeatureExtension(
      String sourceId,
      Feature feature,
      String extension,
      String extensionField,
      JsonValue arguments) {
    throw unsupported();
  }

  public TextureImageInfo textureImageInfo() {
    throw unsupported();
  }

  public TextureImageInfo readPremultipliedRgba8(NativeBuffer buffer) {
    throw unsupported();
  }

  public PremultipliedRgba8Image readPremultipliedRgba8() {
    throw unsupported();
  }

  public MetalOwnedTextureFrameHandle acquireMetalOwnedTextureFrame() {
    throw unsupported();
  }

  public VulkanOwnedTextureFrameHandle acquireVulkanOwnedTextureFrame() {
    throw unsupported();
  }

  public void close() {
    NativeLibrary.ensureLoaded();
    state.closeOnce(RenderSessionNative::mln_render_session_destroy);
  }

  public boolean isClosed() {
    return state.isReleased();
  }

  public MapHandle map() {
    return map;
  }

  public MemorySegment nativeHandle(InternalAccess access) {
    Objects.requireNonNull(access, "access");
    return state.requireLiveSegment();
  }

  public long nativeAddress(InternalAccess access) {
    Objects.requireNonNull(access, "access");
    return state.requireLiveAddress();
  }

  MemorySegment nativeHandle() {
    return state.requireLiveSegment();
  }

  long nativeAddress() {
    return state.requireLiveAddress();
  }

  void releaseMetalFrame(MemorySegment frameSegment, Throwable callbackFailure) {
    throw unsupported();
  }

  void releaseVulkanFrame(MemorySegment frameSegment, Throwable callbackFailure) {
    throw unsupported();
  }
}
