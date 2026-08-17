package org.maplibre.nativeffi.render

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlinx.cinterop.*
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
      value.flags and MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION.toUInt() != 0u,
      value.flags and MLN_RENDER_SESSION_CAPABILITY_READBACK.toUInt() != 0u,
      value.flags and MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC.toUInt() != 0u,
      value.flags and MLN_RENDER_SESSION_CAPABILITY_PRESENTATION.toUInt() != 0u,
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
      (if (demand.ifNeeded) MLN_FRAME_DEMAND_IF_NEEDED.toUInt() else 0u) or
        (if (demand.present) MLN_FRAME_DEMAND_PRESENT.toUInt() else 0u)
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

  public actual fun startResize(extent: RenderTargetExtent): OperationHandle<Unit> = memScoped {
    val native = extent(extent, this)
    unitOperation { mln_render_session_resize_start(id(), native, it) }
  }

  public actual fun startSetMetalSurfaceTarget(descriptor: MetalSurfaceDescriptor) = memScoped {
    unitOperation {
      mln_metal_surface_set_target_start(
        id(),
        RenderStructs.metalSurfaceDescriptor(descriptor, this),
        it,
      )
    }
  }

  public actual fun startSetVulkanSurfaceTarget(descriptor: VulkanSurfaceDescriptor) = memScoped {
    unitOperation {
      mln_vulkan_surface_set_target_start(
        id(),
        RenderStructs.vulkanSurfaceDescriptor(descriptor, this),
        it,
      )
    }
  }

  public actual fun startSetOpenGLSurfaceTarget(descriptor: OpenGLSurfaceDescriptor) = memScoped {
    unitOperation {
      mln_opengl_surface_set_target_start(
        id(),
        RenderStructs.openglSurfaceDescriptor(descriptor, this),
        it,
      )
    }
  }

  public actual fun startSetMetalBorrowedTextureTarget(descriptor: MetalBorrowedTextureDescriptor) =
    memScoped {
      unitOperation {
        mln_metal_borrowed_texture_set_target_start(
          id(),
          RenderStructs.metalBorrowedTextureDescriptor(descriptor, this),
          it,
        )
      }
    }

  public actual fun startSetVulkanBorrowedTextureTarget(
    descriptor: VulkanBorrowedTextureDescriptor
  ) = memScoped {
    unitOperation {
      mln_vulkan_borrowed_texture_set_target_start(
        id(),
        RenderStructs.vulkanBorrowedTextureDescriptor(descriptor, this),
        it,
      )
    }
  }

  public actual fun startSetOpenGLBorrowedTextureTarget(
    descriptor: OpenGLBorrowedTextureDescriptor
  ) = memScoped {
    unitOperation {
      mln_opengl_borrowed_texture_set_target_start(
        id(),
        RenderStructs.openglBorrowedTextureDescriptor(descriptor, this),
        it,
      )
    }
  }

  public actual fun startReduceMemoryUse() = unitOperation {
    mln_render_session_reduce_memory_use_start(id(), it)
  }

  public actual fun startClearData() = unitOperation {
    mln_render_session_clear_data_start(id(), it)
  }

  public actual fun startDumpDebugLogs() = unitOperation {
    mln_render_session_dump_debug_logs_start(id(), it)
  }

  public actual fun startBarrier() = unitOperation { mln_render_session_barrier_start(id(), it) }

  public actual fun startDetach() = unitOperation { mln_render_session_detach_start(id(), it) }

  public actual fun startSetFeatureState(selector: FeatureStateSelector, value: ByteArray) =
    memScoped {
      val views = selectorViews(selector, this)
      unitOperation {
        mln_render_session_set_feature_state_start(
          id(),
          views[0],
          views[1],
          views[2],
          ByteStructs.bufferView(value, this),
          it,
        )
      }
    }

  public actual fun startGetFeatureState(
    selector: FeatureStateSelector
  ): OperationHandle<ByteArray> = memScoped {
    val views = selectorViews(selector, this)
    val operation = startOperation {
      mln_render_session_get_feature_state_start(id(), views[0], views[1], views[2], it)
    }
    this@RenderSessionHandle.operation(
      operation,
      OperationKind.RENDER_FEATURE_STATE_GET,
      OperationResultKind.BUFFER,
    )
  }

  public actual fun takeFeatureStateResult(operation: OperationHandle<ByteArray>): ByteArray =
    takeBuffer(
      operation,
      OperationKind.RENDER_FEATURE_STATE_GET,
      ::mln_render_session_get_feature_state_take_result,
    )

  public actual fun startRemoveFeatureState(selector: FeatureStateSelector) = memScoped {
    val views = selectorViews(selector, this)
    unitOperation {
      mln_render_session_remove_feature_state_start(
        id(),
        views[0],
        views[1],
        views[2],
        ByteStructs.bufferView(selector.stateKey?.encodeToByteArray() ?: byteArrayOf(), this),
        it,
      )
    }
  }

  public actual fun startQueryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ) = memScoped {
    queryFeaturesOperation {
      mln_render_session_query_rendered_features_start(
        id(),
        QueryStructs.renderedQueryGeometry(geometry, this),
        QueryStructs.renderedFeatureQueryOptions(options, this),
        it,
      )
    }
  }

  public actual fun startQuerySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ) = memScoped {
    queryFeaturesOperation {
      mln_render_session_query_source_features_start(
        id(),
        ByteStructs.bufferView(sourceId.encodeToByteArray(), this),
        QueryStructs.sourceFeatureQueryOptions(options, this),
        it,
      )
    }
  }

  public actual fun startQueryFeatureExtension(
    sourceId: String,
    feature: ByteArray,
    extension: String,
    extensionField: String,
    arguments: ByteArray?,
  ) = memScoped {
    val argument = arguments?.let { ByteStructs.bufferView(it, this) }
    bufferOperation {
      mln_render_session_query_feature_extensions_start(
        id(),
        ByteStructs.bufferView(sourceId.encodeToByteArray(), this),
        ByteStructs.bufferView(feature, this),
        ByteStructs.bufferView(extension.encodeToByteArray(), this),
        ByteStructs.bufferView(extensionField.encodeToByteArray(), this),
        argument?.getPointer(this),
        it,
      )
    }
  }

  public actual fun takeQueryResult(operation: OperationHandle<ByteArray>): ByteArray =
    takeBuffer(operation, OperationKind.RENDER_QUERY, ::mln_render_query_take_result)

  public actual fun takeQueryFeaturesResult(
    operation: OperationHandle<List<QueriedFeature>>
  ): List<QueriedFeature> =
    operation.withResultUse(OperationKind.RENDER_QUERY, OperationResultKind.QUERIED_FEATURE_LIST) {
      op ->
      memScoped {
        val out = alloc<ULongVar>()
        out.value = 0u
        Status.check(mln_render_query_features_take_result(op, out.ptr))
        operation.markResultConsumed()
        QueryStructs.queriedFeatureList(
          out.value.asHandle("mln_queried_feature_list", ::queriedFeatureListHandle)
        )
      }
    }

  public actual fun startReadPremultipliedRgba8(): OperationHandle<TextureReadback> {
    val operation = startOperation { mln_texture_read_premultiplied_rgba8_start(id(), it) }
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
      op ->
      memScoped {
        val data = alloc<ULongVar>()
        data.value = 0u
        val info = alloc<mln_texture_image_info>()
        info.size = sizeOf<mln_texture_image_info>().toUInt()
        Status.check(mln_texture_read_premultiplied_rgba8_take_result(op, data.ptr, info.ptr))
        operation.markResultConsumed()
        TextureReadback(
          ByteStructs.ownedBuffer(data.value.asHandle("mln_buffer", ::ownedBufferHandle)),
          RenderStructs.textureImageInfo(info),
        )
      }
    }

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

  private fun unitOperation(start: (CPointer<ULongVar>) -> Int): OperationHandle<Unit> =
    operation(startOperation(start), OperationKind.RENDER_CONTROL, OperationResultKind.NONE)

  private fun bufferOperation(start: (CPointer<ULongVar>) -> Int): OperationHandle<ByteArray> =
    operation(startOperation(start), OperationKind.RENDER_QUERY, OperationResultKind.BUFFER)

  private fun queryFeaturesOperation(
    start: (CPointer<ULongVar>) -> Int
  ): OperationHandle<List<QueriedFeature>> =
    operation(
      startOperation(start),
      OperationKind.RENDER_QUERY,
      OperationResultKind.QUERIED_FEATURE_LIST,
    )

  internal fun <T> operation(
    id: ULong,
    kind: OperationKind,
    resultKind: OperationResultKind,
  ): OperationHandle<T> = OperationHandle(map.runtime(), id, kind, resultKind)

  private fun takeBuffer(
    operation: OperationHandle<ByteArray>,
    kind: OperationKind,
    take: (ULong, CPointer<ULongVar>) -> Int,
  ): ByteArray =
    operation.withResultUse(kind, OperationResultKind.BUFFER) { op ->
      memScoped {
        val out = alloc<ULongVar>()
        out.value = 0u
        Status.check(take(op, out.ptr))
        operation.markResultConsumed()
        ByteStructs.ownedBuffer(out.value.asHandle("mln_buffer", ::ownedBufferHandle))
      }
    }

  internal companion object {
    private fun attach(
      map: MapHandle,
      options: RenderSessionAttachOptions,
      call:
        (CPointer<mln_render_session_attach_options>, CPointer<ULongVar>, CPointer<ULongVar>) -> Int,
    ): RenderSessionAttachment = memScoped {
      val nativeOptions = alloc<mln_render_session_attach_options>()
      mln_render_session_attach_options_default().place(nativeOptions.ptr)
      nativeOptions.driver = options.driver.nativeValue.toUInt()
      nativeOptions.requested_texture_ring_depth = options.requestedTextureRingDepth.toUInt()
      val session = alloc<ULongVar>()
      session.value = 0u
      val operation = alloc<ULongVar>()
      operation.value = 0u
      Status.check(call(nativeOptions.ptr, session.ptr, operation.ptr))
      val handle =
        RenderSessionHandle(
          map,
          session.value.asHandle("mln_render_session", ::renderSessionHandle),
        )
      try {
        RenderSessionAttachment(
          handle,
          OperationHandle(
            map.runtime(),
            operation.value,
            OperationKind.RENDER_ATTACH,
            OperationResultKind.NONE,
          ),
        )
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
        mln_metal_owned_texture_attach_start(
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
        mln_metal_borrowed_texture_attach_start(
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
        mln_vulkan_owned_texture_attach_start(
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
        mln_vulkan_borrowed_texture_attach_start(
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
        mln_opengl_owned_texture_attach_start(
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
        mln_opengl_borrowed_texture_attach_start(
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
        mln_metal_surface_attach_start(
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
        mln_vulkan_surface_attach_start(
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
        mln_opengl_surface_attach_start(
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
