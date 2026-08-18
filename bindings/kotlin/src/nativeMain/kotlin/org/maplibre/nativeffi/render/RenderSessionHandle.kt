package org.maplibre.nativeffi.render

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlinx.cinterop.*
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.ByteStructs
import org.maplibre.nativeffi.internal.struct.QueryStructs
import org.maplibre.nativeffi.internal.struct.RenderStructs
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.query.*
import org.maplibre.nativeffi.runtime.*

@OptIn(ExperimentalAtomicApi::class, ExperimentalForeignApi::class)
public actual class RenderSessionHandle
private constructor(private val map: MapHandle, handle: NativeRenderSession) : AutoCloseable {
  private val state = HandleState("RenderSessionHandle", handle, map)
  private val acquiredFrameScopes = AtomicReference<List<FrameScope>>(emptyList())

  public actual val isClosed: Boolean
    get() = state.isReleased()

  public actual fun map(): MapHandle = map

  public actual fun capabilities(): RenderSessionCapabilities = memScoped {
    val value = alloc<mln_render_session_capabilities>()
    value.size = sizeOf<mln_render_session_capabilities>().toUInt()
    Status.check(mln_render_session_get_capabilities(id(), value.ptr))
    RenderSessionCapabilities(
      RenderDriver.fromNative(value.driver.toInt()),
      value.texture_ring_depth.toInt(),
      value.flags and MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION != 0u,
      value.flags and MLN_RENDER_SESSION_CAPABILITY_READBACK != 0u,
      value.flags and MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC != 0u,
      value.flags and MLN_RENDER_SESSION_CAPABILITY_PRESENTATION != 0u,
    )
  }

  public actual fun snapshot(): RenderSessionSnapshot = memScoped {
    val value = alloc<mln_render_session_snapshot>()
    value.size = sizeOf<mln_render_session_snapshot>().toUInt()
    Status.check(mln_render_session_get_snapshot(id(), value.ptr))
    RenderSessionSnapshot(
      RenderSessionState.fromNative(value.state.toInt()),
      RenderDriver.fromNative(value.driver.toInt()),
      RenderResult.fromNative(value.latest_result),
      RenderTargetExtent(
        value.extent.width.toInt(),
        value.extent.height.toInt(),
        value.extent.scale_factor,
      ),
      value.generation,
      value.map_update_generation,
      value.rendered_update_generation,
      value.extent_generation,
      value.frame_generation,
      value.latest_demand_token,
      value.pending_demand_count.toInt(),
      value.acquired_frame_count.toInt(),
      value.target_ready,
      value.pending_changes,
    )
  }

  public actual fun requestFrame(demand: FrameDemand) = memScoped {
    val value = alloc<mln_frame_demand>()
    mln_frame_demand_default().place(value.ptr)
    value.flags =
      (if (demand.ifNeeded) MLN_FRAME_DEMAND_IF_NEEDED else 0u) or
        (if (demand.present) MLN_FRAME_DEMAND_PRESENT else 0u)
    value.token = demand.token
    value.coalescing_boundary = demand.coalescingBoundary
    value.timeout_ns = demand.timeoutNanoseconds
    Status.check(mln_render_session_request_frame(id(), value.ptr))
  }

  public actual fun drainFrameResults(): List<RenderFrameResult> = memScoped {
    val outBatch = alloc<ULongVar>()
    outBatch.value = 0u
    Status.check(mln_render_session_drain_frame_results(id(), outBatch.ptr))
    val batch = outBatch.value
    try {
      val outCount = alloc<ULongVar>()
      Status.check(mln_render_frame_batch_count(batch, outCount.ptr))
      List(outCount.value.toInt()) { index ->
        val value = alloc<mln_render_frame_result>()
        value.size = sizeOf<mln_render_frame_result>().toUInt()
        Status.check(mln_render_frame_batch_get(batch, index.toULong(), value.ptr))
        frameResult(value)
      }
    } finally {
      if (batch != 0uL) mln_render_frame_batch_release(batch)
    }
  }

  public actual fun serviceDriverWork(maxWork: Int): Int = memScoped {
    Status.requireArgument(maxWork >= 0) { "maxWork must be non-negative" }
    val serviced = alloc<ULongVar>()
    Status.check(mln_render_session_service_driver_work(id(), maxWork.toULong(), serviced.ptr))
    serviced.value.toInt()
  }

  public actual fun acquireFrame(): AcquiredFrameHandle? = memScoped {
    val outFrame = alloc<ULongVar>()
    outFrame.value = 0u
    val status = mln_render_session_acquire_frame(id(), outFrame.ptr)
    if (status == MLN_STATUS_NOT_READY) return@memScoped null
    Status.check(status)
    val scope = FrameScope()
    retainFrameScope(scope)
    AcquiredFrameHandle(this@RenderSessionHandle, outFrame.value, scope)
  }

  public actual fun resize(extent: RenderTargetExtent): Deferred<Unit> = memScoped {
    val native = extent(extent, this)
    unit { mln_render_session_resize(id(), native, it) }
  }

  public actual fun setMetalSurfaceTarget(descriptor: MetalSurfaceDescriptor): Deferred<Unit> =
    memScoped {
      unit {
        mln_metal_surface_set_target(
          id(),
          RenderStructs.metalSurfaceDescriptor(descriptor, this),
          it,
        )
      }
    }

  public actual fun setVulkanSurfaceTarget(descriptor: VulkanSurfaceDescriptor): Deferred<Unit> =
    memScoped {
      unit {
        mln_vulkan_surface_set_target(
          id(),
          RenderStructs.vulkanSurfaceDescriptor(descriptor, this),
          it,
        )
      }
    }

  public actual fun setOpenGLSurfaceTarget(descriptor: OpenGLSurfaceDescriptor): Deferred<Unit> =
    memScoped {
      unit {
        mln_opengl_surface_set_target(
          id(),
          RenderStructs.openglSurfaceDescriptor(descriptor, this),
          it,
        )
      }
    }

  public actual fun setMetalBorrowedTextureTarget(
    descriptor: MetalBorrowedTextureDescriptor
  ): Deferred<Unit> = memScoped {
    unit {
      mln_metal_borrowed_texture_set_target(
        id(),
        RenderStructs.metalBorrowedTextureDescriptor(descriptor, this),
        it,
      )
    }
  }

  public actual fun setVulkanBorrowedTextureTarget(
    descriptor: VulkanBorrowedTextureDescriptor
  ): Deferred<Unit> = memScoped {
    unit {
      mln_vulkan_borrowed_texture_set_target(
        id(),
        RenderStructs.vulkanBorrowedTextureDescriptor(descriptor, this),
        it,
      )
    }
  }

  public actual fun setOpenGLBorrowedTextureTarget(
    descriptor: OpenGLBorrowedTextureDescriptor
  ): Deferred<Unit> = memScoped {
    unit {
      mln_opengl_borrowed_texture_set_target(
        id(),
        RenderStructs.openglBorrowedTextureDescriptor(descriptor, this),
        it,
      )
    }
  }

  public actual fun reduceMemoryUse(): Deferred<Unit> = unit {
    mln_render_session_reduce_memory_use(id(), it)
  }

  public actual fun clearData(): Deferred<Unit> = unit { mln_render_session_clear_data(id(), it) }

  public actual fun dumpDebugLogs(): Deferred<Unit> = unit {
    mln_render_session_dump_debug_logs(id(), it)
  }

  public actual fun barrier(): Deferred<Unit> = unit { mln_render_session_barrier(id(), it) }

  public actual fun detach(): Deferred<Unit> = unit { mln_render_session_detach(id(), it) }

  public actual fun setFeatureState(
    selector: FeatureStateSelector,
    value: ByteArray,
  ): Deferred<Unit> = memScoped {
    val views = selectorViews(selector, this)
    unit {
      mln_render_session_set_feature_state(
        id(),
        views[0],
        views[1],
        views[2],
        ByteStructs.bufferView(value, this),
        it,
      )
    }
  }

  public actual fun getFeatureState(selector: FeatureStateSelector): Deferred<ByteArray> =
    memScoped {
      val views = selectorViews(selector, this)
      CompletionBridge.submit(
        { result -> requiredBuffer(result) },
        { completion ->
          mln_render_session_get_feature_state(id(), views[0], views[1], views[2], completion)
        },
      )
    }

  public actual fun removeFeatureState(selector: FeatureStateSelector): Deferred<Unit> = memScoped {
    val views = selectorViews(selector, this)
    unit {
      mln_render_session_remove_feature_state(
        id(),
        views[0],
        views[1],
        views[2],
        ByteStructs.bufferView(selector.stateKey?.encodeToByteArray() ?: byteArrayOf(), this),
        it,
      )
    }
  }

  public actual fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): Deferred<List<QueriedFeature>> = memScoped {
    CompletionBridge.submit(
      { result ->
        QueryStructs.queriedFeatures(
          result.pointed.value?.reinterpret(),
          result.pointed.value_count,
        )
      },
      { completion ->
        mln_render_session_query_rendered_features(
          id(),
          QueryStructs.renderedQueryGeometry(geometry, this),
          QueryStructs.renderedFeatureQueryOptions(options, this),
          completion,
        )
      },
    )
  }

  public actual fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): Deferred<List<QueriedFeature>> = memScoped {
    CompletionBridge.submit(
      { result ->
        QueryStructs.queriedFeatures(
          result.pointed.value?.reinterpret(),
          result.pointed.value_count,
        )
      },
      { completion ->
        mln_render_session_query_source_features(
          id(),
          ByteStructs.bufferView(sourceId.encodeToByteArray(), this),
          QueryStructs.sourceFeatureQueryOptions(options, this),
          completion,
        )
      },
    )
  }

  public actual fun queryFeatureExtension(
    sourceId: String,
    feature: ByteArray,
    extension: String,
    extensionField: String,
    arguments: ByteArray?,
  ): Deferred<ByteArray> = memScoped {
    val argument = arguments?.let { ByteStructs.bufferView(it, this) }
    CompletionBridge.submit(
      { result -> requiredBuffer(result) },
      { completion ->
        mln_render_session_query_feature_extensions(
          id(),
          ByteStructs.bufferView(sourceId.encodeToByteArray(), this),
          ByteStructs.bufferView(feature, this),
          ByteStructs.bufferView(extension.encodeToByteArray(), this),
          ByteStructs.bufferView(extensionField.encodeToByteArray(), this),
          argument?.getPointer(this),
          completion,
        )
      },
    )
  }

  public actual fun readPremultipliedRgba8(): Deferred<TextureReadback> =
    CompletionBridge.submit(
      { result ->
        val raw = result.pointed.value!!.reinterpret<mln_texture_readback_result>().pointed
        TextureReadback(
          ByteStructs.copyBufferView(raw.data),
          RenderStructs.textureImageInfo(raw.info),
        )
      },
      { completion -> mln_texture_read_premultiplied_rgba8(id(), completion) },
    )

  public actual fun abandon(): RenderAbandonResult = memScoped {
    val value = alloc<mln_render_abandon_result>()
    value.size = sizeOf<mln_render_abandon_result>().toUInt()
    Status.check(mln_render_session_abandon(id(), value.ptr))
    acquiredFrameScopes.load().forEach(FrameScope::close)
    RenderAbandonResult(
      RenderAbandonDisposition.fromNative(value.disposition.toInt()),
      value.quarantined_resource_count.toInt(),
    )
  }

  public actual override fun close() {
    state.closeOnce(destroy = { mln_render_session_destroy(it.rawHandleValue) })
  }

  internal fun frameReleased(scope: FrameScope) {
    while (true) {
      val scopes = acquiredFrameScopes.load()
      if (acquiredFrameScopes.compareAndSet(scopes, scopes - scope)) return
    }
  }

  private fun retainFrameScope(scope: FrameScope) {
    while (true) {
      val scopes = acquiredFrameScopes.load()
      if (acquiredFrameScopes.compareAndSet(scopes, scopes + scope)) return
    }
  }

  private fun id(): ULong = state.requireLive().rawHandleValue

  private fun unit(start: (CPointer<mln_completion>) -> Int): Deferred<Unit> =
    CompletionBridge.unit(start)

  private fun requiredBuffer(result: CPointer<mln_completion_result>): ByteArray {
    require(result.pointed.value_count == 1uL) { "native completion omitted its byte result" }
    return ByteStructs.copyBufferView(result.pointed.value!!.reinterpret<mln_buffer_view>().pointed)
  }

  internal companion object {
    private fun attach(
      map: MapHandle,
      options: RenderSessionAttachOptions,
      call:
        (
          CPointer<mln_render_session_attach_options>, CPointer<ULongVar>, CPointer<mln_completion>,
        ) -> Int,
    ): RenderSessionAttachment = memScoped {
      val nativeOptions = alloc<mln_render_session_attach_options>()
      mln_render_session_attach_options_default().place(nativeOptions.ptr)
      nativeOptions.driver = options.driver.nativeValue.toUInt()
      nativeOptions.requested_texture_ring_depth = options.requestedTextureRingDepth.toUInt()
      val session = alloc<ULongVar>()
      session.value = 0u
      val completed = CompletionBridge.unitChecked { completion ->
        call(nativeOptions.ptr, session.ptr, completion)
      }
      val handle =
        RenderSessionHandle(
          map,
          session.value.asHandle("mln_render_session", ::renderSessionHandle),
        )
      try {
        RenderSessionAttachment(handle, retainSessionUntilComplete(handle, completed))
      } catch (error: Throwable) {
        runCatching { handle.abandon() }
        runCatching { handle.close() }
        throw error
      }
    }

    internal fun attachMetalOwnedTexture(
      map: MapHandle,
      descriptor: MetalOwnedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ) = memScoped {
      attach(map, options) { o, s, p ->
        mln_metal_owned_texture_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.metalOwnedTextureDescriptor(descriptor, this),
          o,
          s,
          p,
        )
      }
    }

    internal fun attachMetalBorrowedTexture(
      map: MapHandle,
      descriptor: MetalBorrowedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ) = memScoped {
      attach(map, options) { o, s, p ->
        mln_metal_borrowed_texture_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.metalBorrowedTextureDescriptor(descriptor, this),
          o,
          s,
          p,
        )
      }
    }

    internal fun attachVulkanOwnedTexture(
      map: MapHandle,
      descriptor: VulkanOwnedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ) = memScoped {
      attach(map, options) { o, s, p ->
        mln_vulkan_owned_texture_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.vulkanOwnedTextureDescriptor(descriptor, this),
          o,
          s,
          p,
        )
      }
    }

    internal fun attachVulkanBorrowedTexture(
      map: MapHandle,
      descriptor: VulkanBorrowedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ) = memScoped {
      attach(map, options) { o, s, p ->
        mln_vulkan_borrowed_texture_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.vulkanBorrowedTextureDescriptor(descriptor, this),
          o,
          s,
          p,
        )
      }
    }

    internal fun attachOpenGLOwnedTexture(
      map: MapHandle,
      descriptor: OpenGLOwnedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ) = memScoped {
      attach(map, options) { o, s, p ->
        mln_opengl_owned_texture_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.openglOwnedTextureDescriptor(descriptor, this),
          o,
          s,
          p,
        )
      }
    }

    internal fun attachOpenGLBorrowedTexture(
      map: MapHandle,
      descriptor: OpenGLBorrowedTextureDescriptor,
      options: RenderSessionAttachOptions,
    ) = memScoped {
      attach(map, options) { o, s, p ->
        mln_opengl_borrowed_texture_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.openglBorrowedTextureDescriptor(descriptor, this),
          o,
          s,
          p,
        )
      }
    }

    internal fun attachMetalSurface(
      map: MapHandle,
      descriptor: MetalSurfaceDescriptor,
      options: RenderSessionAttachOptions,
    ) = memScoped {
      attach(map, options) { o, s, p ->
        mln_metal_surface_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.metalSurfaceDescriptor(descriptor, this),
          o,
          s,
          p,
        )
      }
    }

    internal fun attachVulkanSurface(
      map: MapHandle,
      descriptor: VulkanSurfaceDescriptor,
      options: RenderSessionAttachOptions,
    ) = memScoped {
      attach(map, options) { o, s, p ->
        mln_vulkan_surface_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.vulkanSurfaceDescriptor(descriptor, this),
          o,
          s,
          p,
        )
      }
    }

    internal fun attachOpenGLSurface(
      map: MapHandle,
      descriptor: OpenGLSurfaceDescriptor,
      options: RenderSessionAttachOptions,
    ) = memScoped {
      attach(map, options) { o, s, p ->
        mln_opengl_surface_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.openglSurfaceDescriptor(descriptor, this),
          o,
          s,
          p,
        )
      }
    }
  }
}

@OptIn(ExperimentalAtomicApi::class, ExperimentalForeignApi::class)
public actual class AcquiredFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private var frame: ULong,
  private val scope: FrameScope,
) {
  private val released = AtomicInt(0)
  public actual val isReleased: Boolean
    get() = released.load() != 0

  public actual fun result(): RenderFrameResult = memScoped {
    val v = alloc<mln_render_frame_result>()
    v.size = sizeOf<mln_render_frame_result>().toUInt()
    Status.check(mln_acquired_frame_get_result(requireFrame(), v.ptr))
    frameResult(v)
  }

  public actual fun producerSync(): GpuSync = memScoped {
    val v = alloc<mln_gpu_sync>()
    mln_gpu_sync_default().place(v.ptr)
    Status.check(mln_acquired_frame_get_producer_sync(requireFrame(), v.ptr))
    GpuSync(
      GpuSyncKind.fromNative(v.kind.toInt()),
      v.`object`?.rawValue?.toLong()?.toULong() ?: 0u,
      v.value,
    )
  }

  public actual fun metalTexture(): MetalOwnedTextureFrame = memScoped {
    val v = alloc<mln_metal_owned_texture_frame>()
    v.size = sizeOf<mln_metal_owned_texture_frame>().toUInt()
    Status.check(mln_acquired_frame_get_metal_texture(requireFrame(), v.ptr))
    MetalOwnedTextureFrame(
      scope,
      v.generation.toLong(),
      v.width.toInt(),
      v.height.toInt(),
      v.scale_factor,
      v.frame_id.toLong(),
      scoped(v.texture),
      scoped(v.device),
      v.pixel_format.toLong(),
    )
  }

  public actual fun vulkanTexture(): VulkanOwnedTextureFrame = memScoped {
    val v = alloc<mln_vulkan_owned_texture_frame>()
    v.size = sizeOf<mln_vulkan_owned_texture_frame>().toUInt()
    Status.check(mln_acquired_frame_get_vulkan_texture(requireFrame(), v.ptr))
    VulkanOwnedTextureFrame(
      scope,
      v.generation.toLong(),
      v.width.toInt(),
      v.height.toInt(),
      v.scale_factor,
      v.frame_id.toLong(),
      scoped(v.image),
      scoped(v.image_view),
      scoped(v.device),
      v.format.toInt(),
      v.layout.toInt(),
    )
  }

  public actual fun openGLTexture(): OpenGLOwnedTextureFrame = memScoped {
    val v = alloc<mln_opengl_owned_texture_frame>()
    v.size = sizeOf<mln_opengl_owned_texture_frame>().toUInt()
    Status.check(mln_acquired_frame_get_opengl_texture(requireFrame(), v.ptr))
    OpenGLOwnedTextureFrame(
      scope,
      v.generation.toLong(),
      v.width.toInt(),
      v.height.toInt(),
      v.scale_factor,
      v.frame_id.toLong(),
      v.texture.toInt(),
      v.target.toInt(),
      v.internal_format.toInt(),
      v.format.toInt(),
      v.type.toInt(),
    )
  }

  public actual fun release(consumerCompletion: GpuSync): Unit = memScoped {
    check(released.compareAndSet(0, 1)) { "AcquiredFrameHandle is already released" }
    val native = alloc<mln_gpu_sync>()
    mln_gpu_sync_default().place(native.ptr)
    native.kind = consumerCompletion.kind.nativeValue.toUInt()
    native.`object` = consumerCompletion.objectHandle.toLong().toCPointer()
    native.value = consumerCompletion.value
    val holder = alloc<ULongVar>()
    holder.value = frame
    try {
      Status.check(mln_acquired_frame_release(holder.ptr, native.ptr))
      frame = 0u
      scope.close()
      session.frameReleased(scope)
    } catch (e: Throwable) {
      released.store(0)
      throw e
    }
  }

  private fun requireFrame(): ULong {
    scope.ensureActive()
    check(!isReleased) { "AcquiredFrameHandle is already released" }
    return frame
  }

  private fun scoped(pointer: COpaquePointer?): NativePointer =
    pointer?.rawValue?.toLong()?.let { NativePointer.scoped(it, scope) } ?: NativePointer.NULL
}

@OptIn(ExperimentalForeignApi::class)
private fun extent(value: RenderTargetExtent, scope: MemScope): CPointer<mln_render_target_extent> =
  scope
    .alloc<mln_render_target_extent>()
    .apply {
      size = sizeOf<mln_render_target_extent>().toUInt()
      width = value.width.toUInt()
      height = value.height.toUInt()
      scale_factor = value.scaleFactor
    }
    .ptr

@OptIn(ExperimentalForeignApi::class)
private fun selectorViews(value: FeatureStateSelector, scope: MemScope) =
  listOf(
    ByteStructs.bufferView(value.sourceId.encodeToByteArray(), scope),
    ByteStructs.bufferView(value.sourceLayerId?.encodeToByteArray() ?: byteArrayOf(), scope),
    ByteStructs.bufferView(value.featureId?.encodeToByteArray() ?: byteArrayOf(), scope),
  )

@OptIn(ExperimentalForeignApi::class)
private fun frameResult(value: mln_render_frame_result) =
  RenderFrameResult(
    RenderResult.fromNative(value.disposition),
    value.token,
    value.map_update_generation,
    value.extent_generation,
    value.frame_generation,
    value.needs_repaint,
  )
