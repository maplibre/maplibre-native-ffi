package org.maplibre.nativeffi.render

import java.nio.charset.StandardCharsets
import java.util.concurrent.ConcurrentHashMap
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.LongPointer
import org.bytedeco.javacpp.Pointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.javacpp.ByteArrayViewScope
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.javacpp.ownedBuffer
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions
import org.maplibre.nativeffi.runtime.OperationHandle
import org.maplibre.nativeffi.runtime.OperationKind
import org.maplibre.nativeffi.runtime.OperationResultKind
import org.maplibre.nativeffi.runtime.startOperation

/** Owned Android JNI render session handle. */
public actual class RenderSessionHandle
private constructor(private val map: MapHandle, private val handleId: Long) : AutoCloseable {
  private val mapRetention = map.retainChild("RenderSessionHandle")
  private val core = HandleStateCore("RenderSessionHandle", handleId, map)
  private val acquiredFrameScopes = ConcurrentHashMap.newKeySet<FrameScope>()

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun map(): MapHandle = map

  public actual fun capabilities(): RenderSessionCapabilities {
    NativeAccess.ensureLoaded()
    MaplibreNativeC.mln_render_session_capabilities().use { value ->
      value.size(value.sizeof())
      Status.check(MaplibreNativeC.mln_render_session_get_capabilities(requireLiveHandle(), value))
      val flags = value.flags()
      return RenderSessionCapabilities(
        RenderDriver.fromNative(value.driver()),
        value.texture_ring_depth(),
        flags and MaplibreNativeC.MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION != 0,
        flags and MaplibreNativeC.MLN_RENDER_SESSION_CAPABILITY_READBACK != 0,
        flags and MaplibreNativeC.MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC != 0,
        flags and MaplibreNativeC.MLN_RENDER_SESSION_CAPABILITY_PRESENTATION != 0,
      )
    }
  }

  public actual fun snapshot(): RenderSessionSnapshot {
    NativeAccess.ensureLoaded()
    MaplibreNativeC.mln_render_session_snapshot().use { value ->
      value.size(value.sizeof())
      Status.check(MaplibreNativeC.mln_render_session_get_snapshot(requireLiveHandle(), value))
      val nativeExtent = value.extent()
      return RenderSessionSnapshot(
        RenderSessionState.fromNative(value.state()),
        RenderDriver.fromNative(value.driver()),
        RenderResult.fromNative(value.latest_result()),
        RenderTargetExtent(
          nativeExtent.width(),
          nativeExtent.height(),
          nativeExtent.scale_factor(),
        ),
        value.generation().toULong(),
        value.map_update_generation().toULong(),
        value.rendered_update_generation().toULong(),
        value.extent_generation().toULong(),
        value.frame_generation().toULong(),
        value.latest_demand_token().toULong(),
        value.pending_demand_count(),
        value.acquired_frame_count(),
        value.target_ready(),
        value.pending_changes(),
      )
    }
  }

  public actual fun requestFrame(demand: FrameDemand) {
    NativeAccess.ensureLoaded()
    MaplibreNativeC.mln_frame_demand_default().use { value ->
      var flags = 0
      if (demand.ifNeeded) flags = flags or MaplibreNativeC.MLN_FRAME_DEMAND_IF_NEEDED
      if (demand.present) flags = flags or MaplibreNativeC.MLN_FRAME_DEMAND_PRESENT
      value
        .flags(flags)
        .token(demand.token.toLong())
        .coalescing_boundary(demand.coalescingBoundary.toLong())
        .presentation_time_ns(demand.presentationTimeNanoseconds)
        .deadline_ns(demand.deadlineNanoseconds)
      Status.check(MaplibreNativeC.mln_render_session_request_frame(requireLiveHandle(), value))
    }
  }

  public actual fun drainFrameResults(maxResults: Int): List<RenderFrameResult> {
    NativeAccess.ensureLoaded()
    Status.requireArgument(maxResults >= 0) { "maxResults must be non-negative" }
    LongPointer(1).use { outBatch ->
      outBatch.put(0, 0L)
      Status.check(
        MaplibreNativeC.mln_render_session_drain_frame_results(
          requireLiveHandle(),
          maxResults.toLong(),
          outBatch,
        )
      )
      val batch = outBatch.get()
      try {
        org.bytedeco.javacpp.SizeTPointer(1).use { outCount ->
          Status.check(MaplibreNativeC.mln_render_frame_batch_count(batch, outCount))
          return List(Math.toIntExact(outCount.get())) { index ->
            MaplibreNativeC.mln_render_frame_result().use { value ->
              value.size(value.sizeof())
              Status.check(MaplibreNativeC.mln_render_frame_batch_get(batch, index.toLong(), value))
              frameResult(value)
            }
          }
        }
      } finally {
        if (batch != 0L) MaplibreNativeC.mln_render_frame_batch_release(batch)
      }
    }
  }

  public actual fun serviceDriverWork(maxWork: Int): Int {
    NativeAccess.ensureLoaded()
    Status.requireArgument(maxWork >= 0) { "maxWork must be non-negative" }
    org.bytedeco.javacpp.SizeTPointer(1).use { outServiced ->
      Status.check(
        MaplibreNativeC.mln_render_session_service_driver_work(
          requireLiveHandle(),
          maxWork.toLong(),
          outServiced,
        )
      )
      return Math.toIntExact(outServiced.get())
    }
  }

  public actual fun acquireFrame(): AcquiredFrameHandle? {
    NativeAccess.ensureLoaded()
    LongPointer(1).use { outFrame ->
      outFrame.put(0, 0L)
      val status = MaplibreNativeC.mln_render_session_acquire_frame(requireLiveHandle(), outFrame)
      if (status == MaplibreStatus.NOT_READY.nativeCode) return null
      Status.check(status)
      val scope = FrameScope()
      acquiredFrameScopes.add(scope)
      return AcquiredFrameHandle(this, outFrame.get(), scope)
    }
  }

  public actual fun startResize(extent: RenderTargetExtent): OperationHandle<Unit> =
    unitOperation { outOperation ->
      MaplibreNativeC.mln_render_target_extent().use { nativeExtent ->
        nativeExtent.size(nativeExtent.sizeof())
        setExtent(nativeExtent, extent)
        MaplibreNativeC.mln_render_session_resize_start(
          requireLiveHandle(),
          nativeExtent,
          outOperation,
        )
      }
    }

  public actual fun startSetMetalSurfaceTarget(
    descriptor: MetalSurfaceDescriptor
  ): OperationHandle<Unit> = unitOperation {
    MaplibreNativeC.mln_metal_surface_set_target_start(
      requireLiveHandle(),
      metalSurfaceDescriptor(descriptor),
      it,
    )
  }

  public actual fun startSetVulkanSurfaceTarget(
    descriptor: VulkanSurfaceDescriptor
  ): OperationHandle<Unit> = unitOperation {
    MaplibreNativeC.mln_vulkan_surface_set_target_start(
      requireLiveHandle(),
      vulkanSurfaceDescriptor(descriptor),
      it,
    )
  }

  public actual fun startSetOpenGLSurfaceTarget(
    descriptor: OpenGLSurfaceDescriptor
  ): OperationHandle<Unit> = unitOperation {
    MaplibreNativeC.mln_opengl_surface_set_target_start(
      requireLiveHandle(),
      openglSurfaceDescriptor(descriptor),
      it,
    )
  }

  public actual fun startSetMetalBorrowedTextureTarget(
    descriptor: MetalBorrowedTextureDescriptor
  ): OperationHandle<Unit> = unitOperation {
    MaplibreNativeC.mln_metal_borrowed_texture_set_target_start(
      requireLiveHandle(),
      metalBorrowedTextureDescriptor(descriptor),
      it,
    )
  }

  public actual fun startSetVulkanBorrowedTextureTarget(
    descriptor: VulkanBorrowedTextureDescriptor
  ): OperationHandle<Unit> = unitOperation {
    MaplibreNativeC.mln_vulkan_borrowed_texture_set_target_start(
      requireLiveHandle(),
      vulkanBorrowedTextureDescriptor(descriptor),
      it,
    )
  }

  public actual fun startSetOpenGLBorrowedTextureTarget(
    descriptor: OpenGLBorrowedTextureDescriptor
  ): OperationHandle<Unit> = unitOperation {
    MaplibreNativeC.mln_opengl_borrowed_texture_set_target_start(
      requireLiveHandle(),
      openglBorrowedTextureDescriptor(descriptor),
      it,
    )
  }

  public actual fun startReduceMemoryUse(): OperationHandle<Unit> = unitOperation {
    MaplibreNativeC.mln_render_session_reduce_memory_use_start(requireLiveHandle(), it)
  }

  public actual fun startClearData(): OperationHandle<Unit> = unitOperation {
    MaplibreNativeC.mln_render_session_clear_data_start(requireLiveHandle(), it)
  }

  public actual fun startDumpDebugLogs(): OperationHandle<Unit> = unitOperation {
    MaplibreNativeC.mln_render_session_dump_debug_logs_start(requireLiveHandle(), it)
  }

  public actual fun startBarrier(minimumUpdateGeneration: ULong): OperationHandle<Unit> =
    unitOperation {
      MaplibreNativeC.mln_render_session_barrier_start(
        requireLiveHandle(),
        minimumUpdateGeneration.toLong(),
        it,
      )
    }

  public actual fun startDetach(): OperationHandle<Unit> = unitOperation {
    MaplibreNativeC.mln_render_session_detach_start(requireLiveHandle(), it)
  }

  public actual fun startSetFeatureState(
    selector: FeatureStateSelector,
    value: ByteArray,
  ): OperationHandle<Unit> =
    withSelectorViews(selector) { sourceId, sourceLayerId, featureId ->
      ByteArrayViewScope(value).use { nativeValue ->
        unitOperation {
          MaplibreNativeC.mln_render_session_set_feature_state_start(
            requireLiveHandle(),
            sourceId,
            sourceLayerId,
            featureId,
            nativeValue.view,
            it,
          )
        }
      }
    }

  public actual fun startGetFeatureState(
    selector: FeatureStateSelector
  ): OperationHandle<ByteArray> =
    withSelectorViews(selector) { sourceId, sourceLayerId, featureId ->
      bufferOperation(OperationKind.RENDER_FEATURE_STATE_GET) {
        MaplibreNativeC.mln_render_session_get_feature_state_start(
          requireLiveHandle(),
          sourceId,
          sourceLayerId,
          featureId,
          it,
        )
      }
    }

  public actual fun takeFeatureStateResult(operation: OperationHandle<ByteArray>): ByteArray =
    takeBuffer(
      operation,
      OperationKind.RENDER_FEATURE_STATE_GET,
      MaplibreNativeC::mln_render_session_get_feature_state_take_result,
    )

  public actual fun startRemoveFeatureState(selector: FeatureStateSelector): OperationHandle<Unit> =
    withSelectorViews(selector) { sourceId, sourceLayerId, featureId ->
      StringViewScope(selector.stateKey ?: "").use { stateKey ->
        unitOperation {
          MaplibreNativeC.mln_render_session_remove_feature_state_start(
            requireLiveHandle(),
            sourceId,
            sourceLayerId,
            featureId,
            stateKey.view,
            it,
          )
        }
      }
    }

  public actual fun startQueryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): OperationHandle<ByteArray> =
    RenderedQueryGeometryScope(geometry).use { nativeGeometry ->
      RenderedFeatureQueryOptionsScope(options).use { nativeOptions ->
        bufferOperation {
          MaplibreNativeC.mln_render_session_query_rendered_features_start(
            requireLiveHandle(),
            nativeGeometry.geometry,
            nativeOptions.options,
            it,
          )
        }
      }
    }

  public actual fun startQuerySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): OperationHandle<ByteArray> =
    StringViewScope(sourceId).use { nativeSourceId ->
      SourceFeatureQueryOptionsScope(options).use { nativeOptions ->
        bufferOperation {
          MaplibreNativeC.mln_render_session_query_source_features_start(
            requireLiveHandle(),
            nativeSourceId.view,
            nativeOptions.options,
            it,
          )
        }
      }
    }

  public actual fun startQueryFeatureExtension(
    sourceId: String,
    feature: ByteArray,
    extension: String,
    extensionField: String,
    arguments: ByteArray?,
  ): OperationHandle<ByteArray> =
    StringViewScope(sourceId).use { nativeSourceId ->
      ByteArrayViewScope(feature).use { nativeFeature ->
        StringViewScope(extension).use { nativeExtension ->
          StringViewScope(extensionField).use { nativeExtensionField ->
            ByteArrayViewScope(arguments ?: byteArrayOf()).use { nativeArguments ->
              bufferOperation {
                MaplibreNativeC.mln_render_session_query_feature_extensions_start(
                  requireLiveHandle(),
                  nativeSourceId.view,
                  nativeFeature.view,
                  nativeExtension.view,
                  nativeExtensionField.view,
                  if (arguments == null) null else nativeArguments.view,
                  it,
                )
              }
            }
          }
        }
      }
    }

  public actual fun takeQueryResult(operation: OperationHandle<ByteArray>): ByteArray =
    takeBuffer(operation, OperationKind.RENDER_QUERY, MaplibreNativeC::mln_render_query_take_result)

  public actual fun startReadPremultipliedRgba8(): OperationHandle<TextureReadback> {
    val operation = startOperation {
      MaplibreNativeC.mln_texture_read_premultiplied_rgba8_start(requireLiveHandle(), it)
    }
    return this.operation(
      operation,
      OperationKind.RENDER_READBACK,
      OperationResultKind.TEXTURE_READBACK,
    )
  }

  public actual fun takeReadPremultipliedRgba8Result(
    operation: OperationHandle<TextureReadback>
  ): TextureReadback =
    operation.withResultUse(OperationKind.RENDER_READBACK, OperationResultKind.TEXTURE_READBACK) {
      operationId ->
      LongPointer(1).use { outBuffer ->
        outBuffer.put(0, 0L)
        MaplibreNativeC.mln_texture_image_info().use { outInfo ->
          outInfo.size(outInfo.sizeof())
          Status.check(
            MaplibreNativeC.mln_texture_read_premultiplied_rgba8_take_result(
              operationId,
              outBuffer,
              outInfo,
            )
          )
          operation.markResultConsumed()
          TextureReadback(ownedBuffer(outBuffer.get()), textureImageInfo(outInfo))
        }
      }
    }

  public actual fun abandon(): RenderAbandonResult {
    NativeAccess.ensureLoaded()
    MaplibreNativeC.mln_render_abandon_result().use { result ->
      result.size(result.sizeof())
      Status.check(MaplibreNativeC.mln_render_session_abandon(requireLiveHandle(), result))
      acquiredFrameScopes.forEach(FrameScope::close)
      return RenderAbandonResult(
        RenderAbandonDisposition.fromNative(result.disposition()),
        result.quarantined_resource_count(),
      )
    }
  }

  public actual override fun close() {
    NativeAccess.ensureLoaded()
    core.closeOnce(
      destroy = { MaplibreNativeC.mln_render_session_destroy(handleId) },
      afterSuccess = { mapRetention.close() },
    )
  }

  internal fun retainAttachOperation() = core.retainChild("RenderAttachOperation")

  internal fun frameReleased(scope: FrameScope) {
    acquiredFrameScopes.remove(scope)
  }

  private fun requireLiveHandle(): Long {
    core.requireLive()
    return handleId
  }

  private fun unitOperation(start: (LongPointer) -> Int): OperationHandle<Unit> =
    operation(startOperation(start), OperationKind.RENDER_CONTROL, OperationResultKind.NONE)

  private fun bufferOperation(
    kind: OperationKind = OperationKind.RENDER_QUERY,
    start: (LongPointer) -> Int,
  ): OperationHandle<ByteArray> = operation(startOperation(start), kind, OperationResultKind.BUFFER)

  internal fun <T> operation(
    id: Long,
    kind: OperationKind,
    resultKind: OperationResultKind,
  ): OperationHandle<T> {
    val retention = core.retainChild("RenderOperation")
    return try {
      OperationHandle(map.runtime(), id, kind, resultKind, retention)
    } catch (error: Throwable) {
      retention.close()
      throw error
    }
  }

  private fun takeBuffer(
    operation: OperationHandle<ByteArray>,
    kind: OperationKind,
    take: (Long, LongPointer) -> Int,
  ): ByteArray =
    operation.withResultUse(kind, OperationResultKind.BUFFER) { operationId ->
      LongPointer(1).use { outBuffer ->
        outBuffer.put(0, 0L)
        Status.check(take(operationId, outBuffer))
        operation.markResultConsumed()
        ownedBuffer(outBuffer.get())
      }
    }

  private inline fun <T> withSelectorViews(
    selector: FeatureStateSelector,
    block:
      (
        MaplibreNativeC.mln_buffer_view,
        MaplibreNativeC.mln_buffer_view,
        MaplibreNativeC.mln_buffer_view,
      ) -> T,
  ): T =
    StringViewScope(selector.sourceId).use { sourceId ->
      StringViewScope(selector.sourceLayerId ?: "").use { sourceLayerId ->
        StringViewScope(selector.featureId ?: "").use { featureId ->
          block(sourceId.view, sourceLayerId.view, featureId.view)
        }
      }
    }

  internal companion object {
    private fun attach(
      map: MapHandle,
      options: RenderSessionAttachOptions,
      call: (MaplibreNativeC.mln_render_session_attach_options, LongPointer, LongPointer) -> Int,
    ): RenderSessionAttachment {
      NativeAccess.ensureLoaded()
      Status.requireArgument(options.requestedTextureRingDepth >= 0) {
        "requestedTextureRingDepth must be non-negative"
      }
      MaplibreNativeC.mln_render_session_attach_options_default().use { nativeOptions ->
        nativeOptions
          .driver(options.driver.nativeValue)
          .requested_texture_ring_depth(options.requestedTextureRingDepth)
        LongPointer(1).use { outSession ->
          LongPointer(1).use { outOperation ->
            outSession.put(0, 0L)
            outOperation.put(0, 0L)
            Status.check(call(nativeOptions, outSession, outOperation))
            val sessionId = outSession.get()
            require(sessionId != 0L) { "render session attach returned a null session" }
            val session = RenderSessionHandle(map, sessionId)
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
                  outOperation.get(),
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
    }

    internal fun attachMetalOwnedTexture(
      map: MapHandle,
      descriptor: MetalOwnedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, outOperation ->
        MaplibreNativeC.mln_metal_owned_texture_attach_start(
          map.nativeHandleId(),
          metalOwnedTextureDescriptor(descriptor),
          nativeOptions,
          outSession,
          outOperation,
        )
      }

    internal fun attachMetalBorrowedTexture(
      map: MapHandle,
      descriptor: MetalBorrowedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, outOperation ->
        MaplibreNativeC.mln_metal_borrowed_texture_attach_start(
          map.nativeHandleId(),
          metalBorrowedTextureDescriptor(descriptor),
          nativeOptions,
          outSession,
          outOperation,
        )
      }

    internal fun attachVulkanOwnedTexture(
      map: MapHandle,
      descriptor: VulkanOwnedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, outOperation ->
        MaplibreNativeC.mln_vulkan_owned_texture_attach_start(
          map.nativeHandleId(),
          vulkanOwnedTextureDescriptor(descriptor),
          nativeOptions,
          outSession,
          outOperation,
        )
      }

    internal fun attachVulkanBorrowedTexture(
      map: MapHandle,
      descriptor: VulkanBorrowedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, outOperation ->
        MaplibreNativeC.mln_vulkan_borrowed_texture_attach_start(
          map.nativeHandleId(),
          vulkanBorrowedTextureDescriptor(descriptor),
          nativeOptions,
          outSession,
          outOperation,
        )
      }

    internal fun attachOpenGLOwnedTexture(
      map: MapHandle,
      descriptor: OpenGLOwnedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, outOperation ->
        MaplibreNativeC.mln_opengl_owned_texture_attach_start(
          map.nativeHandleId(),
          openglOwnedTextureDescriptor(descriptor),
          nativeOptions,
          outSession,
          outOperation,
        )
      }

    internal fun attachOpenGLBorrowedTexture(
      map: MapHandle,
      descriptor: OpenGLBorrowedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, outOperation ->
        MaplibreNativeC.mln_opengl_borrowed_texture_attach_start(
          map.nativeHandleId(),
          openglBorrowedTextureDescriptor(descriptor),
          nativeOptions,
          outSession,
          outOperation,
        )
      }

    internal fun attachMetalSurface(
      map: MapHandle,
      descriptor: MetalSurfaceDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, outOperation ->
        MaplibreNativeC.mln_metal_surface_attach_start(
          map.nativeHandleId(),
          metalSurfaceDescriptor(descriptor),
          nativeOptions,
          outSession,
          outOperation,
        )
      }

    internal fun attachVulkanSurface(
      map: MapHandle,
      descriptor: VulkanSurfaceDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, outOperation ->
        MaplibreNativeC.mln_vulkan_surface_attach_start(
          map.nativeHandleId(),
          vulkanSurfaceDescriptor(descriptor),
          nativeOptions,
          outSession,
          outOperation,
        )
      }

    internal fun attachOpenGLSurface(
      map: MapHandle,
      descriptor: OpenGLSurfaceDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, outOperation ->
        MaplibreNativeC.mln_opengl_surface_attach_start(
          map.nativeHandleId(),
          openglSurfaceDescriptor(descriptor),
          nativeOptions,
          outSession,
          outOperation,
        )
      }
  }
}

public actual class AcquiredFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private var frameId: Long,
  private val scope: FrameScope,
) {
  private val released = java.util.concurrent.atomic.AtomicBoolean(false)

  public actual val isReleased: Boolean
    get() = released.get()

  public actual fun result(): RenderFrameResult = withFrame { frame ->
    MaplibreNativeC.mln_render_frame_result().use { value ->
      value.size(value.sizeof())
      Status.check(MaplibreNativeC.mln_acquired_frame_get_result(frame, value))
      frameResult(value)
    }
  }

  public actual fun producerSync(): GpuSync = withFrame { frame ->
    MaplibreNativeC.mln_gpu_sync_default().use { value ->
      Status.check(MaplibreNativeC.mln_acquired_frame_get_producer_sync(frame, value))
      GpuSync(
        GpuSyncKind.fromNative(value.kind()),
        address(value.`object`()).toULong(),
        value.value().toULong(),
      )
    }
  }

  public actual fun metalTexture(): MetalOwnedTextureFrame = withFrame { frame ->
    MaplibreNativeC.mln_metal_owned_texture_frame().use { value ->
      value.size(value.sizeof())
      Status.check(MaplibreNativeC.mln_acquired_frame_get_metal_texture(frame, value))
      metalOwnedTextureFrame(value, scope)
    }
  }

  public actual fun vulkanTexture(): VulkanOwnedTextureFrame = withFrame { frame ->
    MaplibreNativeC.mln_vulkan_owned_texture_frame().use { value ->
      value.size(value.sizeof())
      Status.check(MaplibreNativeC.mln_acquired_frame_get_vulkan_texture(frame, value))
      vulkanOwnedTextureFrame(value, scope)
    }
  }

  public actual fun openGLTexture(): OpenGLOwnedTextureFrame = withFrame { frame ->
    MaplibreNativeC.mln_opengl_owned_texture_frame().use { value ->
      value.size(value.sizeof())
      Status.check(MaplibreNativeC.mln_acquired_frame_get_opengl_texture(frame, value))
      openglOwnedTextureFrame(value, scope)
    }
  }

  public actual fun release(consumerCompletion: GpuSync): OperationHandle<Unit> {
    check(released.compareAndSet(false, true)) { "AcquiredFrameHandle is already released" }
    MaplibreNativeC.mln_gpu_sync_default().use { nativeSync ->
      nativeSync
        .kind(consumerCompletion.kind.nativeValue)
        .`object`(pointerOrNull(consumerCompletion.objectHandle))
        .value(consumerCompletion.value.toLong())
      LongPointer(1).use { frame ->
        LongPointer(1).use { outOperation ->
          frame.put(0, frameId)
          outOperation.put(0, 0L)
          try {
            Status.check(
              MaplibreNativeC.mln_acquired_frame_release_start(frame, nativeSync, outOperation)
            )
            frameId = 0L
            scope.close()
            session.frameReleased(scope)
            return session.operation(
              outOperation.get(),
              OperationKind.FRAME_RELEASE,
              OperationResultKind.NONE,
            )
          } catch (error: Throwable) {
            released.set(false)
            throw error
          }
        }
      }
    }
  }

  private inline fun <T> withFrame(block: (Long) -> T): T {
    scope.ensureActive()
    check(!isReleased) { "AcquiredFrameHandle is already released" }
    return block(frameId)
  }
}

private fun metalOwnedTextureDescriptor(
  descriptor: MetalOwnedTextureDescriptor
): MaplibreNativeC.mln_metal_owned_texture_descriptor =
  MaplibreNativeC.mln_metal_owned_texture_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    context().device(pointerOrNull(descriptor.context.device))
  }

private fun metalBorrowedTextureDescriptor(
  descriptor: MetalBorrowedTextureDescriptor
): MaplibreNativeC.mln_metal_borrowed_texture_descriptor =
  MaplibreNativeC.mln_metal_borrowed_texture_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    physical_width(descriptor.physicalWidth)
    physical_height(descriptor.physicalHeight)
    texture(pointerOrNull(descriptor.texture))
  }

private fun metalSurfaceDescriptor(
  descriptor: MetalSurfaceDescriptor
): MaplibreNativeC.mln_metal_surface_descriptor =
  MaplibreNativeC.mln_metal_surface_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    context().device(pointerOrNull(descriptor.context.device))
    layer(pointerOrNull(descriptor.layer))
  }

private fun vulkanOwnedTextureDescriptor(
  descriptor: VulkanOwnedTextureDescriptor
): MaplibreNativeC.mln_vulkan_owned_texture_descriptor =
  MaplibreNativeC.mln_vulkan_owned_texture_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    setVulkanContext(context(), descriptor.context)
  }

private fun vulkanBorrowedTextureDescriptor(
  descriptor: VulkanBorrowedTextureDescriptor
): MaplibreNativeC.mln_vulkan_borrowed_texture_descriptor =
  MaplibreNativeC.mln_vulkan_borrowed_texture_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    physical_width(descriptor.physicalWidth)
    physical_height(descriptor.physicalHeight)
    setVulkanContext(context(), descriptor.context)
    image(pointerOrNull(descriptor.image))
    image_view(pointerOrNull(descriptor.imageView))
    format(descriptor.format)
    initial_layout(descriptor.initialLayout)
    descriptor.finalLayout?.let { final_layout(it) }
  }

private fun vulkanSurfaceDescriptor(
  descriptor: VulkanSurfaceDescriptor
): MaplibreNativeC.mln_vulkan_surface_descriptor =
  MaplibreNativeC.mln_vulkan_surface_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    setVulkanContext(context(), descriptor.context)
    surface(pointerOrNull(descriptor.surface))
  }

private fun openglOwnedTextureDescriptor(
  descriptor: OpenGLOwnedTextureDescriptor
): MaplibreNativeC.mln_opengl_owned_texture_descriptor =
  MaplibreNativeC.mln_opengl_owned_texture_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    setOpenGLContext(context(), descriptor.context)
  }

private fun openglBorrowedTextureDescriptor(
  descriptor: OpenGLBorrowedTextureDescriptor
): MaplibreNativeC.mln_opengl_borrowed_texture_descriptor =
  MaplibreNativeC.mln_opengl_borrowed_texture_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    physical_width(descriptor.physicalWidth)
    physical_height(descriptor.physicalHeight)
    setOpenGLContext(context(), descriptor.context)
    texture(descriptor.texture)
    target(descriptor.target)
  }

private fun openglSurfaceDescriptor(
  descriptor: OpenGLSurfaceDescriptor
): MaplibreNativeC.mln_opengl_surface_descriptor =
  MaplibreNativeC.mln_opengl_surface_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    setOpenGLContext(context(), descriptor.context)
    surface(pointerOrNull(descriptor.surface))
  }

private fun setExtent(out: MaplibreNativeC.mln_render_target_extent, extent: RenderTargetExtent) {
  out.width(extent.width)
  out.height(extent.height)
  out.scale_factor(extent.scaleFactor)
}

private fun setVulkanContext(
  out: MaplibreNativeC.mln_vulkan_context_descriptor,
  context: VulkanContextDescriptor,
) {
  out.instance(pointerOrNull(context.instance))
  out.physical_device(pointerOrNull(context.physicalDevice))
  out.device(pointerOrNull(context.device))
  out.graphics_queue(pointerOrNull(context.graphicsQueue))
  out.graphics_queue_family_index(context.graphicsQueueFamilyIndex)
  out.get_instance_proc_addr(pointerOrNull(context.getInstanceProcAddr))
  out.get_device_proc_addr(pointerOrNull(context.getDeviceProcAddr))
}

private fun setOpenGLContext(
  out: MaplibreNativeC.mln_opengl_context_descriptor,
  context: OpenGLContextDescriptor,
) {
  out.size(out.sizeof())
  out.ownership(context.ownership.nativeValue)
  when (context) {
    is WglContextDescriptor -> {
      out.platform(MaplibreNativeC.MLN_OPENGL_CONTEXT_PLATFORM_WGL)
      out.data_wgl().apply {
        size(sizeof())
        device_context(pointerOrNull(context.deviceContext))
        share_context(pointerOrNull(context.shareContext))
        get_proc_address(pointerOrNull(context.getProcAddress))
      }
    }
    is EglContextDescriptor -> {
      out.platform(MaplibreNativeC.MLN_OPENGL_CONTEXT_PLATFORM_EGL)
      out.data_egl().apply {
        size(sizeof())
        display(pointerOrNull(context.display))
        config(pointerOrNull(context.config))
        share_context(pointerOrNull(context.shareContext))
        client_api(context.clientApi.nativeValue)
        get_proc_address(pointerOrNull(context.getProcAddress))
      }
    }
  }
}

private fun textureImageInfo(info: MaplibreNativeC.mln_texture_image_info): TextureImageInfo =
  TextureImageInfo(
    info.width(),
    info.height(),
    info.stride(),
    checkedSizeT(info.byte_length(), "texture image byte length"),
  )

private fun checkedSizeT(value: Long, name: String): Long {
  require(value >= 0L) { "$name exceeds Long.MAX_VALUE" }
  return value
}

private fun metalOwnedTextureFrame(
  frame: MaplibreNativeC.mln_metal_owned_texture_frame,
  scope: FrameScope,
): MetalOwnedTextureFrame =
  MetalOwnedTextureFrame(
    scope,
    frame.generation(),
    frame.width(),
    frame.height(),
    frame.scale_factor(),
    frame.frame_id(),
    NativePointer.scoped(address(frame.texture()), scope),
    NativePointer.scoped(address(frame.device()), scope),
    frame.pixel_format(),
  )

private fun vulkanOwnedTextureFrame(
  frame: MaplibreNativeC.mln_vulkan_owned_texture_frame,
  scope: FrameScope,
): VulkanOwnedTextureFrame =
  VulkanOwnedTextureFrame(
    scope,
    frame.generation(),
    frame.width(),
    frame.height(),
    frame.scale_factor(),
    frame.frame_id(),
    NativePointer.scoped(address(frame.image()), scope),
    NativePointer.scoped(address(frame.image_view()), scope),
    NativePointer.scoped(address(frame.device()), scope),
    frame.format(),
    frame.layout(),
  )

private fun openglOwnedTextureFrame(
  frame: MaplibreNativeC.mln_opengl_owned_texture_frame,
  scope: FrameScope,
): OpenGLOwnedTextureFrame =
  OpenGLOwnedTextureFrame(
    scope,
    frame.generation(),
    frame.width(),
    frame.height(),
    frame.scale_factor(),
    frame.frame_id(),
    frame.texture(),
    frame.target(),
    frame.internal_format(),
    frame.format(),
    frame.type(),
  )

private fun address(pointer: Pointer?): Long =
  if (pointer == null || pointer.isNull) 0L else pointer.address()

private fun stringView(value: MaplibreNativeC.mln_buffer_view): String {
  val size = Math.toIntExact(value.size())
  if (size == 0) return ""
  val bytes = ByteArray(size)
  BytePointer(value.data()).get(bytes, 0, size)
  return String(bytes, StandardCharsets.UTF_8)
}

private fun pointerOrNull(pointer: NativePointer): Pointer? =
  if (pointer.isNull) null else AddressPointer(pointer.address)

private fun pointerOrNull(address: ULong): Pointer? =
  if (address == 0uL) null else AddressPointer(address.toLong())

private fun latLng(value: MaplibreNativeC.mln_lat_lng): LatLng =
  LatLng(value.latitude(), value.longitude())

private class RenderedQueryGeometryScope(value: RenderedQueryGeometry) : AutoCloseable {
  private val owned = mutableListOf<Pointer>()
  val geometry: MaplibreNativeC.mln_rendered_query_geometry =
    when (value) {
      is RenderedQueryGeometry.Point ->
        own(MaplibreNativeC.mln_rendered_query_geometry_point(screenPoint(value.point)))
      is RenderedQueryGeometry.Box ->
        own(MaplibreNativeC.mln_rendered_query_geometry_box(screenBox(value.box)))
      is RenderedQueryGeometry.LineString ->
        own(
          MaplibreNativeC.mln_rendered_query_geometry_line_string(
            screenPointArray(value.points),
            value.points.size.toLong(),
          )
        )
    }

  override fun close() {
    owned.asReversed().forEach(Pointer::close)
  }

  private fun <T : Pointer> own(pointer: T): T {
    owned += pointer
    return pointer
  }

  private fun screenPoint(value: ScreenPoint): MaplibreNativeC.mln_screen_point =
    own(MaplibreNativeC.mln_screen_point().x(value.x).y(value.y))

  private fun screenBox(value: ScreenBox): MaplibreNativeC.mln_screen_box =
    own(MaplibreNativeC.mln_screen_box().min(screenPoint(value.min)).max(screenPoint(value.max)))

  private fun screenPointArray(values: List<ScreenPoint>): MaplibreNativeC.mln_screen_point? {
    if (values.isEmpty()) {
      return null
    }
    val out = own(MaplibreNativeC.mln_screen_point(values.size.toLong()))
    values.forEachIndexed { index, value -> out.position(index.toLong()).x(value.x).y(value.y) }
    out.position(0)
    return out
  }
}

private class RenderedFeatureQueryOptionsScope(value: RenderedFeatureQueryOptions?) :
  AutoCloseable {
  private val strings = mutableListOf<StringViewScope>()
  private val filter = value?.filterTransit?.let(::ByteArrayViewScope)
  private val layerIds = value?.layerIds?.let { stringViewArray(it) }
  val options: MaplibreNativeC.mln_rendered_feature_query_options? = value?.let {
    MaplibreNativeC.mln_rendered_feature_query_options_default().apply {
      var fields = 0
      it.layerIds?.let { layerIdValues ->
        fields = fields or MaplibreNativeC.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS
        if (layerIds != null) {
          layer_ids(layerIds)
        }
        layer_id_count(layerIdValues.size.toLong())
      }
      filter?.let { nativeFilter -> filter(nativeFilter.view) }
      fields(fields)
    }
  }

  override fun close() {
    options?.close()
    layerIds?.close()
    filter?.close()
    strings.asReversed().forEach(StringViewScope::close)
  }

  private fun stringViewArray(values: List<String>): MaplibreNativeC.mln_buffer_view? {
    if (values.isEmpty()) {
      return null
    }
    val out = MaplibreNativeC.mln_buffer_view(values.size.toLong())
    values.forEachIndexed { index, value ->
      val scope = StringViewScope(value)
      strings += scope
      out.position(index.toLong()).put<MaplibreNativeC.mln_buffer_view>(scope.view)
    }
    out.position(0)
    return out
  }
}

private class SourceFeatureQueryOptionsScope(value: SourceFeatureQueryOptions?) : AutoCloseable {
  private val strings = mutableListOf<StringViewScope>()
  private val filter = value?.filterTransit?.let(::ByteArrayViewScope)
  private val sourceLayerIds = value?.sourceLayerIds?.let { stringViewArray(it) }
  val options: MaplibreNativeC.mln_source_feature_query_options? = value?.let {
    MaplibreNativeC.mln_source_feature_query_options_default().apply {
      var fields = 0
      it.sourceLayerIds?.let { sourceLayerIdValues ->
        fields = fields or MaplibreNativeC.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
        if (sourceLayerIds != null) {
          source_layer_ids(sourceLayerIds)
        }
        source_layer_id_count(sourceLayerIdValues.size.toLong())
      }
      filter?.let { nativeFilter -> filter(nativeFilter.view) }
      fields(fields)
    }
  }

  override fun close() {
    options?.close()
    sourceLayerIds?.close()
    filter?.close()
    strings.asReversed().forEach(StringViewScope::close)
  }

  private fun stringViewArray(values: List<String>): MaplibreNativeC.mln_buffer_view? {
    if (values.isEmpty()) {
      return null
    }
    val out = MaplibreNativeC.mln_buffer_view(values.size.toLong())
    values.forEachIndexed { index, value ->
      val scope = StringViewScope(value)
      strings += scope
      out.position(index.toLong()).put<MaplibreNativeC.mln_buffer_view>(scope.view)
    }
    out.position(0)
    return out
  }
}

private class StringViewScope(value: String) : AutoCloseable {
  private val bytes: BytePointer
  val view: MaplibreNativeC.mln_buffer_view = MaplibreNativeC.mln_buffer_view()

  init {
    val utf8 = value.toByteArray(StandardCharsets.UTF_8)
    bytes = BytePointer(Math.max(utf8.size, 1).toLong())
    if (utf8.isNotEmpty()) bytes.put(utf8, 0, utf8.size)
    view.data(if (utf8.isEmpty()) null else bytes)
    view.size(utf8.size.toLong())
  }

  override fun close() {
    view.close()
    bytes.close()
  }
}

/** A `void*` built from a raw address, for backend-native pointers and user data. */
private class AddressPointer(address: Long) : Pointer(null as Pointer?) {
  init {
    this.address = address
  }
}

private fun frameResult(value: MaplibreNativeC.mln_render_frame_result): RenderFrameResult =
  RenderFrameResult(
    RenderResult.fromNative(value.disposition()),
    value.token().toULong(),
    value.map_update_generation().toULong(),
    value.extent_generation().toULong(),
    value.frame_generation().toULong(),
    value.presentation_time_ns(),
    value.needs_repaint(),
  )

/** Direct test seam for the JavaCPP render and query adapter. */
internal object JavaCppRenderStructs {
  fun renderedQueryGeometryType(value: RenderedQueryGeometry): Int =
    RenderedQueryGeometryScope(value).use { it.geometry.type() }

  fun textureImageInfoSnapshot(
    width: Int,
    height: Int,
    stride: Int,
    byteLength: Long,
  ): TextureImageInfo =
    MaplibreNativeC.mln_texture_image_info().use {
      it.width(width).height(height).stride(stride).byte_length(byteLength)
      textureImageInfo(it)
    }

  fun metalSnapshot(value: MetalBorrowedTextureDescriptor): RenderDescriptorSnapshot =
    metalBorrowedTextureDescriptor(value).use {
      RenderDescriptorSnapshot(
        it.extent().width(),
        it.extent().height(),
        it.extent().scale_factor(),
        address(it.texture()),
        0L,
        0,
      )
    }

  fun vulkanSnapshot(value: VulkanBorrowedTextureDescriptor): RenderDescriptorSnapshot =
    vulkanBorrowedTextureDescriptor(value).use {
      RenderDescriptorSnapshot(
        it.extent().width(),
        it.extent().height(),
        it.extent().scale_factor(),
        address(it.image()),
        address(it.image_view()),
        it.final_layout(),
      )
    }

  fun openGlSnapshot(value: OpenGLBorrowedTextureDescriptor): RenderDescriptorSnapshot =
    openglBorrowedTextureDescriptor(value).use {
      RenderDescriptorSnapshot(
        it.extent().width(),
        it.extent().height(),
        it.extent().scale_factor(),
        it.texture().toLong(),
        address(it.context().data_egl().display()),
        it.target(),
      )
    }

  data class RenderDescriptorSnapshot(
    val width: Int,
    val height: Int,
    val scaleFactor: Double,
    val firstPointer: Long,
    val secondPointer: Long,
    val extra: Int,
  )
}
