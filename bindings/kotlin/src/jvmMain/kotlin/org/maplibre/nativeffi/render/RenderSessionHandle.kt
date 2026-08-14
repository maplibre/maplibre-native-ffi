package org.maplibre.nativeffi.render

import java.util.concurrent.ConcurrentHashMap
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeRenderSession
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions
import org.maplibre.nativeffi.runtime.OperationHandle
import org.maplibre.nativeffi.runtime.OperationKind
import org.maplibre.nativeffi.runtime.OperationResultKind

/** Owned JVM FFM render-session control handle. */
public actual class RenderSessionHandle
internal constructor(private val ownerMap: MapHandle, private val handle: NativeRenderSession) :
  AutoCloseable {
  private val mapRetention = ownerMap.retainChild("RenderSessionHandle")
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

  public actual fun drainFrameResults(maxResults: Int): List<RenderFrameResult> {
    NativeAccess.ensureLoaded()
    Status.requireArgument(maxResults >= 0) { "maxResults must be non-negative" }
    return NativeAccess.drainRenderFrameResults(requireLiveHandle(), maxResults)
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

  public actual fun startResize(extent: RenderTargetExtent): OperationHandle<Unit> =
    controlOperation(NativeAccess.startResizeRenderSession(requireLiveHandle(), extent))

  public actual fun startSetMetalSurfaceTarget(
    descriptor: MetalSurfaceDescriptor
  ): OperationHandle<Unit> =
    controlOperation(NativeAccess.startSetMetalSurfaceTarget(requireLiveHandle(), descriptor))

  public actual fun startSetVulkanSurfaceTarget(
    descriptor: VulkanSurfaceDescriptor
  ): OperationHandle<Unit> =
    controlOperation(NativeAccess.startSetVulkanSurfaceTarget(requireLiveHandle(), descriptor))

  public actual fun startSetOpenGLSurfaceTarget(
    descriptor: OpenGLSurfaceDescriptor
  ): OperationHandle<Unit> =
    controlOperation(NativeAccess.startSetOpenGLSurfaceTarget(requireLiveHandle(), descriptor))

  public actual fun startSetMetalBorrowedTextureTarget(
    descriptor: MetalBorrowedTextureDescriptor
  ): OperationHandle<Unit> =
    controlOperation(
      NativeAccess.startSetMetalBorrowedTextureTarget(requireLiveHandle(), descriptor)
    )

  public actual fun startSetVulkanBorrowedTextureTarget(
    descriptor: VulkanBorrowedTextureDescriptor
  ): OperationHandle<Unit> =
    controlOperation(
      NativeAccess.startSetVulkanBorrowedTextureTarget(requireLiveHandle(), descriptor)
    )

  public actual fun startSetOpenGLBorrowedTextureTarget(
    descriptor: OpenGLBorrowedTextureDescriptor
  ): OperationHandle<Unit> =
    controlOperation(
      NativeAccess.startSetOpenGLBorrowedTextureTarget(requireLiveHandle(), descriptor)
    )

  public actual fun startReduceMemoryUse(): OperationHandle<Unit> =
    controlOperation(
      NativeAccess.startRenderControl(
        requireLiveHandle(),
        "mln_render_session_reduce_memory_use_start",
      )
    )

  public actual fun startClearData(): OperationHandle<Unit> =
    controlOperation(
      NativeAccess.startRenderControl(requireLiveHandle(), "mln_render_session_clear_data_start")
    )

  public actual fun startDumpDebugLogs(): OperationHandle<Unit> =
    controlOperation(
      NativeAccess.startRenderControl(
        requireLiveHandle(),
        "mln_render_session_dump_debug_logs_start",
      )
    )

  public actual fun startBarrier(minimumUpdateGeneration: ULong): OperationHandle<Unit> =
    controlOperation(NativeAccess.startRenderBarrier(requireLiveHandle(), minimumUpdateGeneration))

  public actual fun startDetach(): OperationHandle<Unit> =
    controlOperation(
      NativeAccess.startRenderControl(requireLiveHandle(), "mln_render_session_detach_start")
    )

  public actual fun startSetFeatureState(
    selector: FeatureStateSelector,
    value: ByteArray,
  ): OperationHandle<Unit> =
    controlOperation(NativeAccess.startSetFeatureState(requireLiveHandle(), selector, value))

  public actual fun startGetFeatureState(
    selector: FeatureStateSelector
  ): OperationHandle<ByteArray> =
    operation(
      NativeAccess.startGetFeatureState(requireLiveHandle(), selector),
      OperationKind.RENDER_FEATURE_STATE_GET,
      OperationResultKind.BUFFER,
    )

  public actual fun takeFeatureStateResult(operation: OperationHandle<ByteArray>): ByteArray =
    operation.withResultUse(OperationKind.RENDER_FEATURE_STATE_GET, OperationResultKind.BUFFER) { id
      ->
      NativeAccess.takeFeatureStateResult(id).also { operation.markResultConsumed() }
    }

  public actual fun startRemoveFeatureState(selector: FeatureStateSelector): OperationHandle<Unit> =
    controlOperation(NativeAccess.startRemoveFeatureState(requireLiveHandle(), selector))

  public actual fun startQueryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): OperationHandle<ByteArray> =
    queryOperation(NativeAccess.startQueryRenderedFeatures(requireLiveHandle(), geometry, options))

  public actual fun startQuerySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): OperationHandle<ByteArray> =
    queryOperation(NativeAccess.startQuerySourceFeatures(requireLiveHandle(), sourceId, options))

  public actual fun startQueryFeatureExtension(
    sourceId: String,
    feature: ByteArray,
    extension: String,
    extensionField: String,
    arguments: ByteArray?,
  ): OperationHandle<ByteArray> =
    queryOperation(
      NativeAccess.startQueryFeatureExtension(
        requireLiveHandle(),
        sourceId,
        feature,
        extension,
        extensionField,
        arguments,
      )
    )

  public actual fun takeQueryResult(operation: OperationHandle<ByteArray>): ByteArray =
    operation.withResultUse(OperationKind.RENDER_QUERY, OperationResultKind.BUFFER) { id ->
      NativeAccess.takeQueryResult(id).also { operation.markResultConsumed() }
    }

  public actual fun startReadPremultipliedRgba8(): OperationHandle<TextureReadback> =
    operation(
      NativeAccess.startTextureReadback(requireLiveHandle()),
      OperationKind.RENDER_READBACK,
      OperationResultKind.TEXTURE_READBACK,
    )

  public actual fun takeReadPremultipliedRgba8Result(
    operation: OperationHandle<TextureReadback>
  ): TextureReadback =
    operation.withResultUse(OperationKind.RENDER_READBACK, OperationResultKind.TEXTURE_READBACK) {
      id ->
      NativeAccess.takeTextureReadbackResult(id).also { operation.markResultConsumed() }
    }

  public actual fun abandon(): RenderAbandonResult {
    NativeAccess.ensureLoaded()
    val result = NativeAccess.abandonRenderSession(requireLiveHandle())
    acquiredFrameScopes.forEach(FrameScope::close)
    return result
  }

  public actual override fun close() {
    NativeAccess.ensureLoaded()
    core.closeOnce(
      destroy = { NativeAccess.destroyRenderSession(handle) },
      afterSuccess = { mapRetention.close() },
    )
  }

  internal fun frameReleased(scope: FrameScope) {
    acquiredFrameScopes.remove(scope)
  }

  internal fun retainAttachOperation() = core.retainChild("RenderAttachOperation")

  internal fun releaseFrame(frame: Long, sync: GpuSync): Pair<Long, OperationHandle<Unit>> {
    val (remainingFrame, operationId) = NativeAccess.startReleaseAcquiredFrame(frame, sync)
    return remainingFrame to
      operation(operationId, OperationKind.FRAME_RELEASE, OperationResultKind.NONE)
  }

  private fun requireLiveHandle(): NativeRenderSession {
    core.requireLive()
    return handle
  }

  private fun controlOperation(id: Long): OperationHandle<Unit> =
    operation(id, OperationKind.RENDER_CONTROL, OperationResultKind.NONE)

  private fun queryOperation(id: Long): OperationHandle<ByteArray> =
    operation(id, OperationKind.RENDER_QUERY, OperationResultKind.BUFFER)

  private fun <T> operation(
    id: Long,
    kind: OperationKind,
    resultKind: OperationResultKind,
  ): OperationHandle<T> {
    val retention = core.retainChild("RenderOperation")
    return try {
      OperationHandle(runtime, id, kind, resultKind, retention)
    } catch (error: Throwable) {
      retention.close()
      throw error
    }
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
      native: Pair<NativeRenderSession, Long>,
    ): RenderSessionAttachment {
      NativeAccess.ensureLoaded()
      val session = RenderSessionHandle(map, native.first)
      val retention =
        try {
          session.retainAttachOperation()
        } catch (error: Throwable) {
          runCatching { session.abandon() }
          runCatching { session.close() }
          throw error
        }
      return try {
        RenderSessionAttachment(
          session,
          OperationHandle(
            map.runtime(),
            native.second,
            OperationKind.RENDER_ATTACH,
            OperationResultKind.NONE,
            retention,
          ),
        )
      } catch (error: Throwable) {
        retention.close()
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
  public actual fun release(consumerCompletion: GpuSync): OperationHandle<Unit> {
    val frame = requireFrame()
    val (remainingFrame, operation) = session.releaseFrame(frame, consumerCompletion)
    check(remainingFrame == 0L) { "native frame release did not consume the handle" }
    nativeFrame = 0L
    scope.close()
    session.frameReleased(scope)
    return operation
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
