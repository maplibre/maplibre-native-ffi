package org.maplibre.nativeffi.render

import java.nio.charset.StandardCharsets
import java.util.concurrent.ConcurrentHashMap
import kotlinx.coroutines.Deferred
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.LongPointer
import org.bytedeco.javacpp.Pointer
import org.bytedeco.javacpp.SizeTPointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.javacpp.ByteArrayViewScope
import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

/** Owned Android JNI render session handle. */
public actual class RenderSessionHandle
private constructor(private val map: MapHandle, private val handleId: Long) : AutoCloseable {
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
        value.generation(),
        value.map_update_generation(),
        value.rendered_update_generation(),
        value.extent_generation(),
        value.frame_generation(),
        value.latest_demand_token(),
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
        .token(demand.token)
        .coalescing_boundary(demand.coalescingBoundary)
        .timeout_ns(demand.timeoutNanoseconds)
      Status.check(MaplibreNativeC.mln_render_session_request_frame(requireLiveHandle(), value))
    }
  }

  public actual fun drainFrameResults(): List<RenderFrameResult> {
    NativeAccess.ensureLoaded()
    LongPointer(1).use { outBatch ->
      outBatch.put(0, 0L)
      val drained =
        MaplibreNativeC.mln_render_session_drain_frame_results(requireLiveHandle(), outBatch)
      if (drained == MaplibreStatus.NOT_READY.nativeCode) return emptyList()
      Status.check(drained)
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

  public actual fun resize(extent: RenderTargetExtent): Deferred<Unit> =
    MaplibreNativeC.mln_render_target_extent().use { nativeExtent ->
      nativeExtent.size(nativeExtent.sizeof())
      setExtent(nativeExtent, extent)
      CompletionBridge.unit { completion ->
        MaplibreNativeC.mln_render_session_resize(requireLiveHandle(), nativeExtent, completion)
      }
    }

  public actual fun setMetalSurfaceTarget(descriptor: MetalSurfaceDescriptor): Deferred<Unit> =
    CompletionBridge.unit { completion ->
      MaplibreNativeC.mln_metal_surface_set_target(
        requireLiveHandle(),
        metalSurfaceDescriptor(descriptor),
        completion,
      )
    }

  public actual fun setVulkanSurfaceTarget(descriptor: VulkanSurfaceDescriptor): Deferred<Unit> =
    CompletionBridge.unit { completion ->
      MaplibreNativeC.mln_vulkan_surface_set_target(
        requireLiveHandle(),
        vulkanSurfaceDescriptor(descriptor),
        completion,
      )
    }

  public actual fun setOpenGLSurfaceTarget(descriptor: OpenGLSurfaceDescriptor): Deferred<Unit> =
    CompletionBridge.unit { completion ->
      MaplibreNativeC.mln_opengl_surface_set_target(
        requireLiveHandle(),
        openglSurfaceDescriptor(descriptor),
        completion,
      )
    }

  public actual fun setMetalBorrowedTextureTarget(
    descriptor: MetalBorrowedTextureDescriptor
  ): Deferred<Unit> = CompletionBridge.unit { completion ->
    MaplibreNativeC.mln_metal_borrowed_texture_set_target(
      requireLiveHandle(),
      metalBorrowedTextureDescriptor(descriptor),
      completion,
    )
  }

  public actual fun setVulkanBorrowedTextureTarget(
    descriptor: VulkanBorrowedTextureDescriptor
  ): Deferred<Unit> = CompletionBridge.unit { completion ->
    MaplibreNativeC.mln_vulkan_borrowed_texture_set_target(
      requireLiveHandle(),
      vulkanBorrowedTextureDescriptor(descriptor),
      completion,
    )
  }

  public actual fun setOpenGLBorrowedTextureTarget(
    descriptor: OpenGLBorrowedTextureDescriptor
  ): Deferred<Unit> = CompletionBridge.unit { completion ->
    MaplibreNativeC.mln_opengl_borrowed_texture_set_target(
      requireLiveHandle(),
      openglBorrowedTextureDescriptor(descriptor),
      completion,
    )
  }

  public actual fun reduceMemoryUse(): Deferred<Unit> = CompletionBridge.unit {
    MaplibreNativeC.mln_render_session_reduce_memory_use(requireLiveHandle(), it)
  }

  public actual fun clearData(): Deferred<Unit> = CompletionBridge.unit {
    MaplibreNativeC.mln_render_session_clear_data(requireLiveHandle(), it)
  }

  public actual fun dumpDebugLogs(): Deferred<Unit> = CompletionBridge.unit {
    MaplibreNativeC.mln_render_session_dump_debug_logs(requireLiveHandle(), it)
  }

  public actual fun barrier(): Deferred<Unit> = CompletionBridge.unit {
    MaplibreNativeC.mln_render_session_barrier(requireLiveHandle(), it)
  }

  public actual fun detach(): Deferred<Unit> = CompletionBridge.unit {
    MaplibreNativeC.mln_render_session_detach(requireLiveHandle(), it)
  }

  public actual fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): Deferred<List<QueriedFeature>> =
    RenderedQueryGeometryScope(geometry).use { nativeGeometry ->
      RenderedFeatureQueryOptionsScope(options).use { nativeOptions ->
        CompletionBridge.submit(
          ::queriedFeatures,
          { completion ->
            MaplibreNativeC.mln_render_session_query_rendered_features(
              requireLiveHandle(),
              nativeGeometry.geometry,
              nativeOptions.options,
              completion,
            )
          },
        )
      }
    }

  public actual fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): Deferred<List<QueriedFeature>> =
    StringViewScope(sourceId).use { nativeSourceId ->
      SourceFeatureQueryOptionsScope(options).use { nativeOptions ->
        CompletionBridge.submit(
          ::queriedFeatures,
          { completion ->
            MaplibreNativeC.mln_render_session_query_source_features(
              requireLiveHandle(),
              nativeSourceId.view,
              nativeOptions.options,
              completion,
            )
          },
        )
      }
    }

  public actual fun queryFeatureExtension(
    sourceId: String,
    feature: ByteArray,
    extension: String,
    extensionField: String,
    arguments: ByteArray?,
  ): Deferred<ByteArray> =
    StringViewScope(sourceId).use { nativeSourceId ->
      ByteArrayViewScope(feature).use { nativeFeature ->
        StringViewScope(extension).use { nativeExtension ->
          StringViewScope(extensionField).use { nativeExtensionField ->
            ByteArrayViewScope(arguments ?: byteArrayOf()).use { nativeArguments ->
              CompletionBridge.submit(
                ::requiredBuffer,
                { completion ->
                  MaplibreNativeC.mln_render_session_query_feature_extensions(
                    requireLiveHandle(),
                    nativeSourceId.view,
                    nativeFeature.view,
                    nativeExtension.view,
                    nativeExtensionField.view,
                    if (arguments == null) null else nativeArguments.view,
                    completion,
                  )
                },
              )
            }
          }
        }
      }
    }

  public actual fun readPremultipliedRgba8(): Deferred<TextureReadback> =
    CompletionBridge.submit(
      { result ->
        val raw = MaplibreNativeC.mln_texture_readback_result(result.value())
        TextureReadback(
          JavaCppSupport.byteArray(raw.data().data(), raw.data().size()),
          textureImageInfo(raw.info()),
        )
      },
      { completion ->
        MaplibreNativeC.mln_texture_read_premultiplied_rgba8(requireLiveHandle(), completion)
      },
    )

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
    core.closeOnce(destroy = { MaplibreNativeC.mln_render_session_destroy(handleId) })
  }

  internal fun frameReleased(scope: FrameScope) {
    acquiredFrameScopes.remove(scope)
  }

  private fun requireLiveHandle(): Long {
    core.requireLive()
    return handleId
  }

  private fun requiredBuffer(result: MaplibreNativeC.mln_completion_result): ByteArray {
    require(result.value_count() == 1L) { "native completion omitted its byte result" }
    val view = MaplibreNativeC.mln_buffer_view(result.value())
    return JavaCppSupport.byteArray(view.data(), view.size())
  }

  private fun queriedFeatures(result: MaplibreNativeC.mln_completion_result): List<QueriedFeature> {
    val count = Math.toIntExact(result.value_count())
    if (count == 0) return emptyList()
    val values = MaplibreNativeC.mln_queried_feature(result.value())
    return List(count) { index -> queriedFeature(values.position(index.toLong())) }
  }

  internal companion object {
    private fun attach(
      map: MapHandle,
      options: RenderSessionAttachOptions,
      call:
        (
          MaplibreNativeC.mln_render_session_attach_options,
          LongPointer,
          MaplibreNativeC.mln_completion,
        ) -> Int,
    ): RenderSessionAttachment {
      NativeAccess.ensureLoaded()
      MaplibreNativeC.mln_render_session_attach_options_default().use { nativeOptions ->
        nativeOptions
          .driver(options.driver.nativeValue)
          .requested_texture_ring_depth(options.requestedTextureRingDepth)
        LongPointer(1).use { outSession ->
          outSession.put(0, 0L)
          val completed = CompletionBridge.unitChecked { completion ->
            call(nativeOptions, outSession, completion)
          }
          val sessionId = outSession.get()
          require(sessionId != 0L) { "render session attach returned a null session" }
          return RenderSessionAttachment(RenderSessionHandle(map, sessionId), completed)
        }
      }
    }

    internal fun attachMetalOwnedTexture(
      map: MapHandle,
      descriptor: MetalOwnedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, completion ->
        MaplibreNativeC.mln_metal_owned_texture_attach(
          map.nativeHandleId(),
          metalOwnedTextureDescriptor(descriptor),
          nativeOptions,
          outSession,
          completion,
        )
      }

    internal fun attachMetalBorrowedTexture(
      map: MapHandle,
      descriptor: MetalBorrowedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, completion ->
        MaplibreNativeC.mln_metal_borrowed_texture_attach(
          map.nativeHandleId(),
          metalBorrowedTextureDescriptor(descriptor),
          nativeOptions,
          outSession,
          completion,
        )
      }

    internal fun attachVulkanOwnedTexture(
      map: MapHandle,
      descriptor: VulkanOwnedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, completion ->
        MaplibreNativeC.mln_vulkan_owned_texture_attach(
          map.nativeHandleId(),
          vulkanOwnedTextureDescriptor(descriptor),
          nativeOptions,
          outSession,
          completion,
        )
      }

    internal fun attachVulkanBorrowedTexture(
      map: MapHandle,
      descriptor: VulkanBorrowedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, completion ->
        MaplibreNativeC.mln_vulkan_borrowed_texture_attach(
          map.nativeHandleId(),
          vulkanBorrowedTextureDescriptor(descriptor),
          nativeOptions,
          outSession,
          completion,
        )
      }

    internal fun attachOpenGLOwnedTexture(
      map: MapHandle,
      descriptor: OpenGLOwnedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, completion ->
        MaplibreNativeC.mln_opengl_owned_texture_attach(
          map.nativeHandleId(),
          openglOwnedTextureDescriptor(descriptor),
          nativeOptions,
          outSession,
          completion,
        )
      }

    internal fun attachOpenGLBorrowedTexture(
      map: MapHandle,
      descriptor: OpenGLBorrowedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, completion ->
        MaplibreNativeC.mln_opengl_borrowed_texture_attach(
          map.nativeHandleId(),
          openglBorrowedTextureDescriptor(descriptor),
          nativeOptions,
          outSession,
          completion,
        )
      }

    internal fun attachMetalSurface(
      map: MapHandle,
      descriptor: MetalSurfaceDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, completion ->
        MaplibreNativeC.mln_metal_surface_attach(
          map.nativeHandleId(),
          metalSurfaceDescriptor(descriptor),
          nativeOptions,
          outSession,
          completion,
        )
      }

    internal fun attachVulkanSurface(
      map: MapHandle,
      descriptor: VulkanSurfaceDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, completion ->
        MaplibreNativeC.mln_vulkan_surface_attach(
          map.nativeHandleId(),
          vulkanSurfaceDescriptor(descriptor),
          nativeOptions,
          outSession,
          completion,
        )
      }

    internal fun attachOpenGLSurface(
      map: MapHandle,
      descriptor: OpenGLSurfaceDescriptor,
      options: RenderSessionAttachOptions,
    ): RenderSessionAttachment =
      attach(map, options) { nativeOptions, outSession, completion ->
        MaplibreNativeC.mln_opengl_surface_attach(
          map.nativeHandleId(),
          openglSurfaceDescriptor(descriptor),
          nativeOptions,
          outSession,
          completion,
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
      GpuSync(GpuSyncKind.fromNative(value.kind()), value.`object`(), value.value())
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

  public actual fun release(consumerCompletion: GpuSync) {
    check(released.compareAndSet(false, true)) { "AcquiredFrameHandle is already released" }
    MaplibreNativeC.mln_gpu_sync_default().use { nativeSync ->
      nativeSync
        .kind(consumerCompletion.kind.nativeValue)
        .`object`(consumerCompletion.objectHandle)
        .value(consumerCompletion.value)
      LongPointer(1).use { frame ->
        frame.put(0, frameId)
        try {
          Status.check(MaplibreNativeC.mln_acquired_frame_release(frame, nativeSync))
          frameId = 0L
          scope.close()
          session.frameReleased(scope)
        } catch (error: Throwable) {
          released.set(false)
          throw error
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
    image(descriptor.image.bits)
    image_view(descriptor.imageView.bits)
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
    surface(descriptor.surface.bits)
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
    VulkanHandle.scoped(frame.image(), scope),
    VulkanHandle.scoped(frame.image_view(), scope),
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

private fun queriedFeature(value: MaplibreNativeC.mln_queried_feature): QueriedFeature {
  val fields = value.fields()
  return QueriedFeature(
    JavaCppSupport.byteArray(value.feature().data(), value.feature().size()),
    if (fields and MaplibreNativeC.MLN_QUERIED_FEATURE_SOURCE_ID != 0) stringView(value.source_id())
    else null,
    if (fields and MaplibreNativeC.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID != 0)
      stringView(value.source_layer_id())
    else null,
    if (fields and MaplibreNativeC.MLN_QUERIED_FEATURE_STATE != 0)
      JavaCppSupport.byteArray(value.state().data(), value.state().size())
    else null,
  )
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

internal class FeatureStateSelectorScope(value: FeatureStateSelector) : AutoCloseable {
  private val sourceId = StringViewScope(value.sourceId)
  private val sourceLayerId = value.sourceLayerId?.let(::StringViewScope)
  private val featureId = value.featureId?.let(::StringViewScope)
  private val stateKey = value.stateKey?.let(::StringViewScope)
  val selector: MaplibreNativeC.mln_feature_state_selector =
    MaplibreNativeC.mln_feature_state_selector()

  init {
    selector.size(selector.sizeof())
    selector.source_id(sourceId.view)
    var fields = 0
    sourceLayerId?.let {
      fields = fields or MaplibreNativeC.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
      selector.source_layer_id(it.view)
    }
    featureId?.let {
      fields = fields or MaplibreNativeC.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
      selector.feature_id(it.view)
    }
    stateKey?.let {
      fields = fields or MaplibreNativeC.MLN_FEATURE_STATE_SELECTOR_STATE_KEY
      selector.state_key(it.view)
    }
    selector.fields(fields)
  }

  override fun close() {
    selector.close()
    stateKey?.close()
    featureId?.close()
    sourceLayerId?.close()
    sourceId.close()
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
    value.token(),
    value.map_update_generation(),
    value.extent_generation(),
    value.frame_generation(),
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
        it.image(),
        it.image_view(),
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
