// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*
import org.maplibre.nativeffi.runtime.CommandCompletion

public abstract class GeneratedRenderSessionOperations internal constructor() {
  internal abstract val binding: HandleStateCore
  internal val bindingCallbacks: CallbackOwner = CallbackOwner()

  /**
   * Irreversibly closes control and mailboxes without graphics calls.
   *
   * See `mln_render_session_abandon` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun abandon(): RenderAbandonResult =
    nativeCall(this, binding, "mln_render_session_abandon") {
      val out = sized(16, 4)
      check(C.mln_render_session_abandon(handle, out, diagnostic))
      readRenderAbandonResult(out)
    }

  /**
   * Acquires the oldest rendered frame that is not already acquired. The frame owns its slot until
   * release. The call is nonblocking.
   *
   * See `mln_render_session_acquire_frame` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun acquireFrame(): AcquiredFrameHandle? =
    nativeCall(this, binding, "mln_render_session_acquire_frame") {
      val out = allocate(8, 8)
      if (present(C.mln_render_session_acquire_frame(handle, out, diagnostic), absent = -9))
        adopt(out, GeneratedOwnerDisposal::acquiredFrame) {
          AcquiredFrameHandle(it, this@GeneratedRenderSessionOperations as RenderSessionHandle)
        }
      else null
    }

  /**
   * Starts a barrier that completes after all render work accepted before it has a terminal result.
   * A barrier does not request a frame.
   *
   * See `mln_render_session_barrier` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun barrier(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_barrier") {
      check(C.mln_render_session_barrier(handle, completion, diagnostic))
    }

  /**
   * Starts asynchronous renderer-data clearing.
   *
   * See `mln_render_session_clear_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun clearData(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_clear_data") {
      check(C.mln_render_session_clear_data(handle, completion, diagnostic))
    }

  /**
   * Copies the last completed rendered transform into an independent projection. Callable from any
   * thread. Returns invalid state before a completed render, after an extent or target change, or
   * after detachment. The caller owns the returned projection, which remains usable after the
   * session is released. out_projection must point to a null handle.
   *
   * See `mln_render_session_create_projection` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun createProjection(): MapProjectionHandle =
    nativeCall(this, binding, "mln_render_session_create_projection") {
      val out = allocate(8, 8)
      check(C.mln_render_session_create_projection(handle, out, diagnostic))
      adopt(out, GeneratedOwnerDisposal::mapProjection) { MapProjectionHandle(it) }
    }

  /**
   * Retires a detached or abandoned session handle. The call is CPU-only and may run on any native
   * thread, including from one of the session's own completions. If an abandonment is still in
   * progress on another thread, this waits for it to finish before consuming the session owner.
   *
   * See `mln_render_session_destroy` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun destroy(): Unit =
    nativeClose(this, binding, "mln_render_session_destroy") {
      check(C.mln_render_session_destroy(handle, diagnostic))
    }

  /**
   * Starts normal graphics-owner teardown and map detachment.
   *
   * See `mln_render_session_detach` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun detach(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_detach") {
      check(C.mln_render_session_detach(handle, completion, diagnostic))
    }

  /**
   * Consumes a session and schedules its retirement and destruction.
   *
   * See `mln_render_session_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun dispose(): Unit =
    nativeClose(this, binding, "mln_render_session_dispose") {
      check(C.mln_render_session_dispose(handle, diagnostic))
    }

  /**
   * Drains every currently queued terminal frame result into an independently owned batch. The
   * records remain stable until the batch is released.
   *
   * See `mln_render_session_drain_frame_results` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun drainFrameResults(): RenderFrameBatchHandle? =
    nativeCall(this, binding, "mln_render_session_drain_frame_results") {
      val out = allocate(8, 8)
      if (present(C.mln_render_session_drain_frame_results(handle, out, diagnostic), absent = -9))
        adopt(out, GeneratedOwnerDisposal::renderFrameBatch) { RenderFrameBatchHandle(it) }
      else null
    }

  /**
   * Starts asynchronous renderer diagnostic-log emission.
   *
   * See `mln_render_session_dump_debug_logs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun dumpDebugLogs(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_dump_debug_logs") {
      check(C.mln_render_session_dump_debug_logs(handle, completion, diagnostic))
    }

  /**
   * Returns the immutable capabilities fixed during attachment.
   *
   * See `mln_render_session_get_capabilities` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun getCapabilities(): RenderSessionCapabilities =
    nativeCall(this, binding, "mln_render_session_get_capabilities") {
      val out = sized(16, 4)
      check(C.mln_render_session_get_capabilities(handle, out, diagnostic))
      readRenderSessionCapabilities(out)
    }

  /**
   * Copies the latest render-session snapshot from any native thread.
   *
   * See `mln_render_session_get_snapshot` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun getSnapshot(): RenderSessionSnapshot =
    nativeCall(this, binding, "mln_render_session_get_snapshot") {
      val out = sized(104, 8)
      check(C.mln_render_session_get_snapshot(handle, out, diagnostic))
      readRenderSessionSnapshot(out)
    }

  /**
   * Starts a feature-extension query against the latest driver state. The completion borrows one
   * `mln_buffer_view` holding UTF-8 JSON (value_count 1), valid only for the callback.
   *
   * See `mln_render_session_query_feature_extensions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  public fun queryFeatureExtensions(
    sourceId: String,
    feature: ByteArray,
    extension: String,
    extensionField: String,
    arguments: ByteArray? = null,
  ): Deferred<ByteArray> =
    nativeSubmit(
      this,
      binding,
      "mln_render_session_query_feature_extensions",
      { result -> readView(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_render_session_query_feature_extensions(
          handle,
          view(sourceId),
          view(feature),
          view(extension),
          view(extensionField),
          arguments?.let { view(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts a rendered-feature query against the session's latest driver state.
   *
   * See `mln_render_session_query_rendered_features` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  public fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions? = null,
  ): Deferred<List<QueriedFeature>> =
    nativeSubmit(
      this,
      binding,
      "mln_render_session_query_rendered_features",
      { result ->
        readStrided(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          CompletionBridge.valueSize(result),
          w(36, 72),
        ) {
          readQueriedFeature(it)
        }
      },
    ) {
      check(
        C.mln_render_session_query_rendered_features(
          handle,
          writeRenderedQueryGeometry(geometry),
          options?.let { writeRenderedFeatureQueryOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts a source-feature query against the session's latest driver state. The completion borrows
   * value_count `mln_queried_feature` values, value_size bytes apart, valid only for the callback.
   *
   * See `mln_render_session_query_source_features` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  public fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions? = null,
  ): Deferred<List<QueriedFeature>> =
    nativeSubmit(
      this,
      binding,
      "mln_render_session_query_source_features",
      { result ->
        readStrided(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          CompletionBridge.valueSize(result),
          w(36, 72),
        ) {
          readQueriedFeature(it)
        }
      },
    ) {
      check(
        C.mln_render_session_query_source_features(
          handle,
          view(sourceId),
          options?.let { writeSourceFeatureQueryOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Reads back the latest frame of the session's owned texture as premultiplied RGBA8.
   *
   * See `mln_render_session_read_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun readTexture(): Deferred<TextureReadbackResult> =
    nativeSubmit(
      this,
      binding,
      "mln_render_session_read_texture",
      { result -> readTextureReadbackResult(CompletionBridge.value(result)) },
    ) {
      check(C.mln_render_session_read_texture(handle, completion, diagnostic))
    }

  /**
   * Starts best-effort release of renderer caches.
   *
   * See `mln_render_session_reduce_memory_use` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun reduceMemoryUse(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_reduce_memory_use") {
      check(C.mln_render_session_reduce_memory_use(handle, completion, diagnostic))
    }

  /**
   * Requests a frame without waiting. Every accepted demand produces one terminal result record. A
   * core worker wakes itself; a caller driver publishes its driver-work endpoint.
   *
   * See `mln_render_session_request_frame` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun requestFrame(demand: FrameDemand): Unit =
    nativeCall(this, binding, "mln_render_session_request_frame") {
      check(C.mln_render_session_request_frame(handle, writeFrameDemand(demand), diagnostic))
    }

  /**
   * Starts an ordered logical resize. The completion runs after the selected driver applies the
   * extent and updates the map viewport.
   *
   * See `mln_render_session_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun resize(extent: RenderTargetExtent): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_render_session_resize") {
      check(
        C.mln_render_session_resize(handle, writeRenderTargetExtent(extent), completion, diagnostic)
      )
    }

  /**
   * Services up to max_work items for a caller-graphics-thread driver; zero services every item
   * currently queued. The first successful service call fixes the session's graphics-thread
   * identity; later calls from another native thread return `MLN_STATUS_WRONG_THREAD`. The target
   * context must be current. Core-worker sessions return `MLN_STATUS_INVALID_STATE`.
   *
   * See `mln_render_session_service_driver_work` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun serviceDriverWork(maxWork: ULong): ULong =
    nativeCall(this, binding, "mln_render_session_service_driver_work") {
      val out = allocate(w(4, 8), w(4, 8))
      check(C.mln_render_session_service_driver_work(handle, maxWork.toLong(), out, diagnostic))
      readSize(out)
    }

  /**
   * Starts an ordered caller-owned Metal texture replacement.
   *
   * See `mln_render_session_set_metal_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun setMetalBorrowedTextureTarget(
    descriptor: MetalBorrowedTextureDescriptor
  ): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_set_metal_borrowed_texture_target") {
      check(
        C.mln_render_session_set_metal_borrowed_texture_target(
          handle,
          writeMetalBorrowedTextureDescriptor(descriptor),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts an ordered Metal surface replacement.
   *
   * See `mln_render_session_set_metal_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  public fun setMetalSurfaceTarget(descriptor: MetalSurfaceDescriptor): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_set_metal_surface_target") {
      check(
        C.mln_render_session_set_metal_surface_target(
          handle,
          writeMetalSurfaceDescriptor(descriptor),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts an ordered caller-owned OpenGL texture replacement.
   *
   * See `mln_render_session_set_opengl_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun setOpenglBorrowedTextureTarget(
    descriptor: OpenglBorrowedTextureDescriptor
  ): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_set_opengl_borrowed_texture_target") {
      check(
        C.mln_render_session_set_opengl_borrowed_texture_target(
          handle,
          writeOpenglBorrowedTextureDescriptor(descriptor),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts an ordered OpenGL surface replacement.
   *
   * See `mln_render_session_set_opengl_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  public fun setOpenglSurfaceTarget(descriptor: OpenglSurfaceDescriptor): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_set_opengl_surface_target") {
      check(
        C.mln_render_session_set_opengl_surface_target(
          handle,
          writeOpenglSurfaceDescriptor(descriptor),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts an ordered caller-owned Vulkan texture replacement.
   *
   * See `mln_render_session_set_vulkan_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun setVulkanBorrowedTextureTarget(
    descriptor: VulkanBorrowedTextureDescriptor
  ): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_set_vulkan_borrowed_texture_target") {
      check(
        C.mln_render_session_set_vulkan_borrowed_texture_target(
          handle,
          writeVulkanBorrowedTextureDescriptor(descriptor),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts an ordered Vulkan surface replacement.
   *
   * See `mln_render_session_set_vulkan_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  public fun setVulkanSurfaceTarget(descriptor: VulkanSurfaceDescriptor): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_set_vulkan_surface_target") {
      check(
        C.mln_render_session_set_vulkan_surface_target(
          handle,
          writeVulkanSurfaceDescriptor(descriptor),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts an ordered caller-owned WebGPU texture replacement.
   *
   * See `mln_render_session_set_webgpu_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun setWebgpuBorrowedTextureTarget(
    descriptor: WebgpuBorrowedTextureDescriptor
  ): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_set_webgpu_borrowed_texture_target") {
      check(
        C.mln_render_session_set_webgpu_borrowed_texture_target(
          handle,
          writeWebgpuBorrowedTextureDescriptor(descriptor),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts an ordered WebGPU surface replacement.
   *
   * See `mln_render_session_set_webgpu_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  public fun setWebgpuSurfaceTarget(descriptor: WebgpuSurfaceDescriptor): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_set_webgpu_surface_target") {
      check(
        C.mln_render_session_set_webgpu_surface_target(
          handle,
          writeWebgpuSurfaceDescriptor(descriptor),
          completion,
          diagnostic,
        )
      )
    }
}
