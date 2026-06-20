package org.maplibre.nativeffi.render

import java.lang.foreign.MemorySegment
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.query.FeatureExtensionResult
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

/** Owned JVM FFM render session handle. Close it on the map owner thread. */
public actual class RenderSessionHandle
internal constructor(private val map: MapHandle, private val handle: MemorySegment) :
  AutoCloseable {
  private val mapRetention = map.retainChild()
  private val core = HandleStateCore("RenderSessionHandle", handle.address(), map)
  private val activeFrame = ActiveFrameState()

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun map(): MapHandle = map

  public actual fun resize(width: Int, height: Int, scaleFactor: Double) {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("resize")
    Status.requireArgument(width >= 0) { "width must be non-negative" }
    Status.requireArgument(height >= 0) { "height must be non-negative" }
    NativeAccess.resizeRenderSession(requireLiveHandle(), width, height, scaleFactor)
  }

  public actual fun renderUpdate() {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("render")
    NativeAccess.renderUpdate(requireLiveHandle())
  }

  public actual fun detach() {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("detach")
    NativeAccess.detachRenderSession(requireLiveHandle())
  }

  public actual fun reduceMemoryUse() {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("reduce memory use")
    NativeAccess.reduceRenderSessionMemoryUse(requireLiveHandle())
  }

  public actual fun clearData() {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("clear data")
    NativeAccess.clearRenderSessionData(requireLiveHandle())
  }

  public actual fun dumpDebugLogs() {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("dump debug logs")
    NativeAccess.dumpRenderSessionDebugLogs(requireLiveHandle())
  }

  public actual fun setFeatureState(selector: FeatureStateSelector, value: JsonValue) {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("set feature state")
    NativeAccess.setFeatureState(requireLiveHandle(), selector, value)
  }

  public actual fun getFeatureState(selector: FeatureStateSelector): JsonValue {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("get feature state")
    return NativeAccess.getFeatureState(requireLiveHandle(), selector)
  }

  public actual fun removeFeatureState(selector: FeatureStateSelector) {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("remove feature state")
    NativeAccess.removeFeatureState(requireLiveHandle(), selector)
  }

  public actual fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): List<QueriedFeature> {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("query rendered features")
    return NativeAccess.queryRenderedFeatures(requireLiveHandle(), geometry, options)
  }

  public actual fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): List<QueriedFeature> {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("query source features")
    return NativeAccess.querySourceFeatures(requireLiveHandle(), sourceId, options)
  }

  public actual fun queryFeatureExtension(
    sourceId: String,
    feature: Feature,
    extension: String,
    extensionField: String,
    arguments: JsonValue?,
  ): FeatureExtensionResult {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("query feature extension")
    return NativeAccess.queryFeatureExtension(
      requireLiveHandle(),
      sourceId,
      feature,
      extension,
      extensionField,
      arguments,
    )
  }

  public actual fun textureImageInfo(): TextureImageInfo {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("read texture data")
    return NativeAccess.textureImageInfo(requireLiveHandle())
  }

  public actual fun readPremultipliedRgba8(buffer: NativeBuffer): TextureImageInfo {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("read texture data")
    return NativeAccess.readPremultipliedRgba8(requireLiveHandle(), buffer)
  }

  public actual fun acquireMetalOwnedTextureFrame(): MetalOwnedTextureFrameHandle {
    NativeAccess.ensureLoaded()
    activeFrame.beginAcquire()
    var frame: NativeAccess.OwnedTextureFrameSegment? = null
    try {
      frame = NativeAccess.acquireMetalOwnedTextureFrame(requireLiveHandle())
      val scope = FrameScope()
      return MetalOwnedTextureFrameHandle(
        this,
        frame,
        scope,
        NativeAccess.metalOwnedTextureFrame(frame.segment, scope),
      )
    } catch (error: Throwable) {
      activeFrame.endBorrow()
      frame?.close()
      throw error
    }
  }

  public actual fun acquireVulkanOwnedTextureFrame(): VulkanOwnedTextureFrameHandle {
    NativeAccess.ensureLoaded()
    activeFrame.beginAcquire()
    var frame: NativeAccess.OwnedTextureFrameSegment? = null
    try {
      frame = NativeAccess.acquireVulkanOwnedTextureFrame(requireLiveHandle())
      val scope = FrameScope()
      return VulkanOwnedTextureFrameHandle(
        this,
        frame,
        scope,
        NativeAccess.vulkanOwnedTextureFrame(frame.segment, scope),
      )
    } catch (error: Throwable) {
      activeFrame.endBorrow()
      frame?.close()
      throw error
    }
  }

  public actual fun acquireOpenGLOwnedTextureFrame(): OpenGLOwnedTextureFrameHandle {
    NativeAccess.ensureLoaded()
    activeFrame.beginAcquire()
    var frame: NativeAccess.OwnedTextureFrameSegment? = null
    try {
      frame = NativeAccess.acquireOpenGLOwnedTextureFrame(requireLiveHandle())
      val scope = FrameScope()
      return OpenGLOwnedTextureFrameHandle(
        this,
        frame,
        scope,
        NativeAccess.openglOwnedTextureFrame(frame.segment, scope),
      )
    } catch (error: Throwable) {
      activeFrame.endBorrow()
      frame?.close()
      throw error
    }
  }

  public actual override fun close() {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("destroy")
    core.closeOnce(
      destroy = { NativeAccess.destroyRenderSession(handle) },
      afterSuccess = { mapRetention.close() },
    )
  }

  internal fun nativeAddress(): Long = handle.address()

  internal fun releaseMetalFrame(frame: java.lang.foreign.MemorySegment) {
    NativeAccess.releaseMetalOwnedTextureFrame(requireLiveHandle(), frame)
  }

  internal fun releaseVulkanFrame(frame: java.lang.foreign.MemorySegment) {
    NativeAccess.releaseVulkanOwnedTextureFrame(requireLiveHandle(), frame)
  }

  internal fun releaseOpenGLFrame(frame: java.lang.foreign.MemorySegment) {
    NativeAccess.releaseOpenGLOwnedTextureFrame(requireLiveHandle(), frame)
  }

  internal fun finishFrameBorrow() {
    activeFrame.endBorrow()
  }

  private fun requireLiveHandle(): MemorySegment {
    core.requireLive()
    return handle
  }
}
