package org.maplibre.nativejni.render;

import java.lang.foreign.MemorySegment;
import java.util.List;
import org.maplibre.nativejni.geo.Feature;
import org.maplibre.nativejni.json.JsonValue;
import org.maplibre.nativejni.map.MapHandle;
import org.maplibre.nativejni.query.FeatureExtensionResult;
import org.maplibre.nativejni.query.FeatureStateSelector;
import org.maplibre.nativejni.query.QueriedFeature;
import org.maplibre.nativejni.query.RenderedFeatureQueryOptions;
import org.maplibre.nativejni.query.RenderedQueryGeometry;
import org.maplibre.nativejni.query.SourceFeatureQueryOptions;

/** API-parity scaffold for the Java JNI binding. */
public final class RenderSessionHandle implements AutoCloseable {
  private static UnsupportedOperationException unsupported() {
    return new UnsupportedOperationException(
        "RenderSessionHandle is not implemented by the JNI bridge yet");
  }

  public static RenderSessionHandle attachMetalOwnedTexture(
      MapHandle map, MetalOwnedTextureDescriptor descriptor) {
    throw unsupported();
  }

  public static RenderSessionHandle attachMetalBorrowedTexture(
      MapHandle map, MetalBorrowedTextureDescriptor descriptor) {
    throw unsupported();
  }

  public static RenderSessionHandle attachVulkanOwnedTexture(
      MapHandle map, VulkanOwnedTextureDescriptor descriptor) {
    throw unsupported();
  }

  public static RenderSessionHandle attachVulkanBorrowedTexture(
      MapHandle map, VulkanBorrowedTextureDescriptor descriptor) {
    throw unsupported();
  }

  public static RenderSessionHandle attachMetalSurface(
      MapHandle map, MetalSurfaceDescriptor descriptor) {
    throw unsupported();
  }

  public static RenderSessionHandle attachVulkanSurface(
      MapHandle map, VulkanSurfaceDescriptor descriptor) {
    throw unsupported();
  }

  public void resize(int width, int height, double scaleFactor) {
    throw unsupported();
  }

  public void renderUpdate() {
    throw unsupported();
  }

  public void detach() {
    throw unsupported();
  }

  public void reduceMemoryUse() {
    throw unsupported();
  }

  public void clearData() {
    throw unsupported();
  }

  public void dumpDebugLogs() {
    throw unsupported();
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
    throw unsupported();
  }

  public boolean isClosed() {
    throw unsupported();
  }

  public MapHandle map() {
    throw unsupported();
  }

  MemorySegment nativeHandle() {
    throw unsupported();
  }

  long nativeAddress() {
    throw unsupported();
  }

  void releaseMetalFrame(MemorySegment frameSegment, Throwable callbackFailure) {
    throw unsupported();
  }

  void releaseVulkanFrame(MemorySegment frameSegment, Throwable callbackFailure) {
    throw unsupported();
  }
}
