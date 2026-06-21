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

/** Owned render session handle. Platform actuals own the native render session carrier. */
public expect class RenderSessionHandle : AutoCloseable {
  public val isClosed: Boolean

  public fun map(): MapHandle

  public fun resize(width: Int, height: Int, scaleFactor: Double)

  public fun renderUpdate()

  public fun detach()

  public fun reduceMemoryUse()

  public fun clearData()

  public fun dumpDebugLogs()

  public fun setFeatureState(selector: FeatureStateSelector, value: JsonValue)

  public fun getFeatureState(selector: FeatureStateSelector): JsonValue

  public fun removeFeatureState(selector: FeatureStateSelector)

  public fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): List<QueriedFeature>

  public fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): List<QueriedFeature>

  public fun queryFeatureExtension(
    sourceId: String,
    feature: Feature,
    extension: String,
    extensionField: String,
    arguments: JsonValue?,
  ): FeatureExtensionResult

  public fun textureImageInfo(): TextureImageInfo

  public fun readPremultipliedRgba8(buffer: NativeBuffer): TextureImageInfo

  public fun acquireMetalOwnedTextureFrame(): MetalOwnedTextureFrameHandle

  public fun acquireVulkanOwnedTextureFrame(): VulkanOwnedTextureFrameHandle

  public fun acquireOpenGLOwnedTextureFrame(): OpenGLOwnedTextureFrameHandle

  override fun close()
}
