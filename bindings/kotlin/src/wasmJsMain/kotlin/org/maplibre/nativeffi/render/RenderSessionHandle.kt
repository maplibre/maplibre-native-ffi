package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.query.FeatureExtensionResult
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

/**
 * Scaffold for the browser render session handle.
 *
 * Every member throws. The actual exists so the `wasmJs` source set compiles while the browser
 * binding is filled in one file at a time; nothing here is finished work.
 */
public actual class RenderSessionHandle private constructor() : AutoCloseable {
  public actual val isClosed: Boolean
    get() = throw NotImplementedError("wasmJs RenderSessionHandle.isClosed is not implemented yet")

  public actual fun map(): MapHandle =
    throw NotImplementedError("wasmJs RenderSessionHandle.map is not implemented yet")

  public actual fun resize(width: Int, height: Int, scaleFactor: Double) {
    throw NotImplementedError("wasmJs RenderSessionHandle.resize is not implemented yet")
  }

  public actual fun setMetalSurfaceTarget(descriptor: MetalSurfaceDescriptor) {
    throw NotImplementedError(
      "wasmJs RenderSessionHandle.setMetalSurfaceTarget is not implemented yet"
    )
  }

  public actual fun setVulkanSurfaceTarget(descriptor: VulkanSurfaceDescriptor) {
    throw NotImplementedError(
      "wasmJs RenderSessionHandle.setVulkanSurfaceTarget is not implemented yet"
    )
  }

  public actual fun setOpenGLSurfaceTarget(descriptor: OpenGLSurfaceDescriptor) {
    throw NotImplementedError(
      "wasmJs RenderSessionHandle.setOpenGLSurfaceTarget is not implemented yet"
    )
  }

  public actual fun setMetalBorrowedTextureTarget(descriptor: MetalBorrowedTextureDescriptor) {
    throw NotImplementedError(
      "wasmJs RenderSessionHandle.setMetalBorrowedTextureTarget is not implemented yet"
    )
  }

  public actual fun setVulkanBorrowedTextureTarget(descriptor: VulkanBorrowedTextureDescriptor) {
    throw NotImplementedError(
      "wasmJs RenderSessionHandle.setVulkanBorrowedTextureTarget is not implemented yet"
    )
  }

  public actual fun setOpenGLBorrowedTextureTarget(descriptor: OpenGLBorrowedTextureDescriptor) {
    throw NotImplementedError(
      "wasmJs RenderSessionHandle.setOpenGLBorrowedTextureTarget is not implemented yet"
    )
  }

  public actual fun renderUpdate(): Boolean =
    throw NotImplementedError("wasmJs RenderSessionHandle.renderUpdate is not implemented yet")

  public actual fun detach() {
    throw NotImplementedError("wasmJs RenderSessionHandle.detach is not implemented yet")
  }

  public actual fun reduceMemoryUse() {
    throw NotImplementedError("wasmJs RenderSessionHandle.reduceMemoryUse is not implemented yet")
  }

  public actual fun clearData() {
    throw NotImplementedError("wasmJs RenderSessionHandle.clearData is not implemented yet")
  }

  public actual fun dumpDebugLogs() {
    throw NotImplementedError("wasmJs RenderSessionHandle.dumpDebugLogs is not implemented yet")
  }

  public actual fun setFeatureState(selector: FeatureStateSelector, value: JsonValue) {
    throw NotImplementedError("wasmJs RenderSessionHandle.setFeatureState is not implemented yet")
  }

  public actual fun getFeatureState(selector: FeatureStateSelector): JsonValue =
    throw NotImplementedError("wasmJs RenderSessionHandle.getFeatureState is not implemented yet")

  public actual fun removeFeatureState(selector: FeatureStateSelector) {
    throw NotImplementedError(
      "wasmJs RenderSessionHandle.removeFeatureState is not implemented yet"
    )
  }

  public actual fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): List<QueriedFeature> =
    throw NotImplementedError(
      "wasmJs RenderSessionHandle.queryRenderedFeatures is not implemented yet"
    )

  public actual fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): List<QueriedFeature> =
    throw NotImplementedError(
      "wasmJs RenderSessionHandle.querySourceFeatures is not implemented yet"
    )

  public actual fun queryFeatureExtension(
    sourceId: String,
    feature: Feature,
    extension: String,
    extensionField: String,
    arguments: JsonValue?,
  ): FeatureExtensionResult =
    throw NotImplementedError(
      "wasmJs RenderSessionHandle.queryFeatureExtension is not implemented yet"
    )

  public actual fun textureImageInfo(): TextureImageInfo =
    throw NotImplementedError("wasmJs RenderSessionHandle.textureImageInfo is not implemented yet")

  public actual fun readPremultipliedRgba8(buffer: NativeBuffer): TextureImageInfo =
    throw NotImplementedError(
      "wasmJs RenderSessionHandle.readPremultipliedRgba8 is not implemented yet"
    )

  public actual fun acquireMetalOwnedTextureFrame(): MetalOwnedTextureFrameHandle =
    throw NotImplementedError(
      "wasmJs RenderSessionHandle.acquireMetalOwnedTextureFrame is not implemented yet"
    )

  public actual fun acquireVulkanOwnedTextureFrame(): VulkanOwnedTextureFrameHandle =
    throw NotImplementedError(
      "wasmJs RenderSessionHandle.acquireVulkanOwnedTextureFrame is not implemented yet"
    )

  public actual fun acquireOpenGLOwnedTextureFrame(): OpenGLOwnedTextureFrameHandle =
    throw NotImplementedError(
      "wasmJs RenderSessionHandle.acquireOpenGLOwnedTextureFrame is not implemented yet"
    )

  public actual override fun close() {
    throw NotImplementedError("wasmJs RenderSessionHandle.close is not implemented yet")
  }
}
