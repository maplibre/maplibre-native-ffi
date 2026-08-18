package org.maplibre.nativeffi.render

import java.util.concurrent.ConcurrentHashMap
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeRenderSession
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

/** Owned JVM FFM render-session control handle. */
public actual class RenderSessionHandle
internal constructor(private val ownerMap: MapHandle, private val handle: NativeRenderSession) :
  AutoCloseable {
  private val core = HandleStateCore("RenderSessionHandle", handle.raw, ownerMap)
  private val runtime = ownerMap.runtime()
  private val acquiredFrameScopes = ConcurrentHashMap.newKeySet<FrameScope>()

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun map(): MapHandle = ownerMap

  public actual fun capabilities(): RenderSessionCapabilities {
    NativeAccess.ensureLoaded()
    return NativeAccess.renderSessionCapabilities(requireLiveHandle())
  }

  public actual fun snapshot(): RenderSessionSnapshot {
    NativeAccess.ensureLoaded()
    return NativeAccess.renderSessionSnapshot(requireLiveHandle())
  }

  public actual fun requestFrame(demand: FrameDemand) {
    NativeAccess.ensureLoaded()
    NativeAccess.requestRenderFrame(requireLiveHandle(), demand)
  }

  public actual fun drainFrameResults(): List<RenderFrameResult> {
    NativeAccess.ensureLoaded()
    return NativeAccess.drainRenderFrameResults(requireLiveHandle())
  }

  public actual fun serviceDriverWork(maxWork: Int): Int {
    NativeAccess.ensureLoaded()
    Status.requireArgument(maxWork >= 0) { "maxWork must be non-negative" }
    return NativeAccess.serviceRenderDriverWork(requireLiveHandle(), maxWork)
  }

  public actual fun acquireFrame(): AcquiredFrameHandle? {
    NativeAccess.ensureLoaded()
    val nativeFrame = NativeAccess.acquireRenderFrame(requireLiveHandle()) ?: return null
    val scope = FrameScope()
    acquiredFrameScopes.add(scope)
    return AcquiredFrameHandle(this, nativeFrame, scope)
  }

  public actual fun resize(extent: RenderTargetExtent): Deferred<Unit> =
    NativeAccess.resizeRenderSession(requireLiveHandle(), extent)

  public actual fun setMetalSurfaceTarget(descriptor: MetalSurfaceDescriptor): Deferred<Unit> =
    NativeAccess.setMetalSurfaceTarget(requireLiveHandle(), descriptor)

  public actual fun setVulkanSurfaceTarget(descriptor: VulkanSurfaceDescriptor): Deferred<Unit> =
    NativeAccess.setVulkanSurfaceTarget(requireLiveHandle(), descriptor)

  public actual fun setOpenGLSurfaceTarget(descriptor: OpenGLSurfaceDescriptor): Deferred<Unit> =
    NativeAccess.setOpenGLSurfaceTarget(requireLiveHandle(), descriptor)

  public actual fun setMetalBorrowedTextureTarget(
    descriptor: MetalBorrowedTextureDescriptor
  ): Deferred<Unit> = NativeAccess.setMetalBorrowedTextureTarget(requireLiveHandle(), descriptor)

  public actual fun setVulkanBorrowedTextureTarget(
    descriptor: VulkanBorrowedTextureDescriptor
  ): Deferred<Unit> = NativeAccess.setVulkanBorrowedTextureTarget(requireLiveHandle(), descriptor)

  public actual fun setOpenGLBorrowedTextureTarget(
    descriptor: OpenGLBorrowedTextureDescriptor
  ): Deferred<Unit> = NativeAccess.setOpenGLBorrowedTextureTarget(requireLiveHandle(), descriptor)

  public actual fun reduceMemoryUse(): Deferred<Unit> =
    NativeAccess.renderControl(requireLiveHandle(), "mln_render_session_reduce_memory_use")

  public actual fun clearData(): Deferred<Unit> =
    NativeAccess.renderControl(requireLiveHandle(), "mln_render_session_clear_data")

  public actual fun dumpDebugLogs(): Deferred<Unit> =
    NativeAccess.renderControl(requireLiveHandle(), "mln_render_session_dump_debug_logs")

  public actual fun barrier(): Deferred<Unit> = NativeAccess.renderBarrier(requireLiveHandle())

  public actual fun detach(): Deferred<Unit> =
    NativeAccess.renderControl(requireLiveHandle(), "mln_render_session_detach")

  public actual fun setFeatureState(
    selector: FeatureStateSelector,
    value: ByteArray,
  ): Deferred<Unit> = NativeAccess.setFeatureState(requireLiveHandle(), selector, value)

  public actual fun getFeatureState(selector: FeatureStateSelector): Deferred<ByteArray> =
    NativeAccess.getFeatureState(requireLiveHandle(), selector)

  public actual fun removeFeatureState(selector: FeatureStateSelector): Deferred<Unit> =
    NativeAccess.removeFeatureState(requireLiveHandle(), selector)

  public actual fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): Deferred<List<QueriedFeature>> =
    NativeAccess.queryRenderedFeatures(requireLiveHandle(), geometry, options)

  public actual fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): Deferred<List<QueriedFeature>> =
    NativeAccess.querySourceFeatures(requireLiveHandle(), sourceId, options)

  public actual fun queryFeatureExtension(
    sourceId: String,
    feature: ByteArray,
    extension: String,
    extensionField: String,
    arguments: ByteArray?,
  ): Deferred<ByteArray> =
    NativeAccess.queryFeatureExtension(
      requireLiveHandle(),
      sourceId,
      feature,
      extension,
      extensionField,
      arguments,
    )

  public actual fun readPremultipliedRgba8(): Deferred<TextureReadback> =
    NativeAccess.textureReadback(requireLiveHandle())

  public actual fun abandon(): RenderAbandonResult {
    NativeAccess.ensureLoaded()
    val result = NativeAccess.abandonRenderSession(requireLiveHandle())
    acquiredFrameScopes.forEach(FrameScope::close)
    return result
  }

  public actual override fun close() {
    NativeAccess.ensureLoaded()
    core.closeOnce(destroy = { NativeAccess.destroyRenderSession(handle) })
  }

  internal fun frameReleased(scope: FrameScope) {
    acquiredFrameScopes.remove(scope)
  }

  private fun requireLiveHandle(): NativeRenderSession {
    core.requireLive()
    return handle
  }

  internal companion object {
    fun attachMetalOwnedTexture(
      map: MapHandle,
      descriptor: MetalOwnedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attachment(map, NativeAccess.attachMetalOwnedTexture(map.nativeHandle(), descriptor, options))

    fun attachMetalBorrowedTexture(
      map: MapHandle,
      descriptor: MetalBorrowedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attachment(
        map,
        NativeAccess.attachMetalBorrowedTexture(map.nativeHandle(), descriptor, options),
      )

    fun attachVulkanOwnedTexture(
      map: MapHandle,
      descriptor: VulkanOwnedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attachment(
        map,
        NativeAccess.attachVulkanOwnedTexture(map.nativeHandle(), descriptor, options),
      )

    fun attachVulkanBorrowedTexture(
      map: MapHandle,
      descriptor: VulkanBorrowedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attachment(
        map,
        NativeAccess.attachVulkanBorrowedTexture(map.nativeHandle(), descriptor, options),
      )

    fun attachOpenGLOwnedTexture(
      map: MapHandle,
      descriptor: OpenGLOwnedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attachment(
        map,
        NativeAccess.attachOpenGLOwnedTexture(map.nativeHandle(), descriptor, options),
      )

    fun attachOpenGLBorrowedTexture(
      map: MapHandle,
      descriptor: OpenGLBorrowedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attachment(
        map,
        NativeAccess.attachOpenGLBorrowedTexture(map.nativeHandle(), descriptor, options),
      )

    fun attachMetalSurface(
      map: MapHandle,
      descriptor: MetalSurfaceDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attachment(map, NativeAccess.attachMetalSurface(map.nativeHandle(), descriptor, options))

    fun attachVulkanSurface(
      map: MapHandle,
      descriptor: VulkanSurfaceDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attachment(map, NativeAccess.attachVulkanSurface(map.nativeHandle(), descriptor, options))

    fun attachOpenGLSurface(
      map: MapHandle,
      descriptor: OpenGLSurfaceDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attachment(map, NativeAccess.attachOpenGLSurface(map.nativeHandle(), descriptor, options))

    private fun attachment(
      map: MapHandle,
      native: Pair<NativeRenderSession, Deferred<Unit>>,
    ): RenderSessionAttachment {
      NativeAccess.ensureLoaded()
      val session = RenderSessionHandle(map, native.first)
      return try {
        RenderSessionAttachment(session, retainSessionUntilComplete(session, native.second))
      } catch (error: Throwable) {
        runCatching { session.abandon() }
        runCatching { session.close() }
        throw error
      }
    }
  }
}

/** JVM FFM acquired-frame lease. */
public actual class AcquiredFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  frame: Long,
  private val scope: FrameScope,
) {
  @Volatile private var nativeFrame: Long = frame

  public actual val isReleased: Boolean
    get() = nativeFrame == 0L

  public actual fun result(): RenderFrameResult =
    NativeAccess.acquiredFrameResult(requireAccessibleFrame())

  public actual fun producerSync(): GpuSync =
    NativeAccess.acquiredFrameProducerSync(requireAccessibleFrame())

  public actual fun metalTexture(): MetalOwnedTextureFrame =
    NativeAccess.acquiredMetalTexture(requireAccessibleFrame(), scope)

  public actual fun vulkanTexture(): VulkanOwnedTextureFrame =
    NativeAccess.acquiredVulkanTexture(requireAccessibleFrame(), scope)

  public actual fun openGLTexture(): OpenGLOwnedTextureFrame =
    NativeAccess.acquiredOpenGLTexture(requireAccessibleFrame(), scope)

  @Synchronized
  public actual fun release(consumerCompletion: GpuSync) {
    val frame = requireFrame()
    val remainingFrame = NativeAccess.releaseAcquiredFrame(frame, consumerCompletion)
    check(remainingFrame == 0L) { "native frame release did not consume the handle" }
    nativeFrame = 0L
    scope.close()
    session.frameReleased(scope)
  }

  private fun requireAccessibleFrame(): Long {
    scope.ensureActive()
    return requireFrame()
  }

  private fun requireFrame(): Long {
    val frame = nativeFrame
    if (frame == 0L) throw Status.invalidState("AcquiredFrameHandle is released")
    return frame
  }
}
