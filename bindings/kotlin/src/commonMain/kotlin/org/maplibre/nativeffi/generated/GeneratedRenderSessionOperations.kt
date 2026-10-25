// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlin.jvm.JvmInline
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*
import org.maplibre.nativeffi.runtime.CommandCompletion

public abstract class GeneratedRenderSessionOperations internal constructor() {
  internal abstract val binding: HandleState
  internal val bindingCallbacks: CallbackOwner = CallbackOwner()

  /**
   * Irreversibly ends session work and destroys the session's graphics objects where that is safe.
   *
   * See `mln_render_session_abandon` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun abandon(): RenderAbandonResult =
    nativeCall(this, binding) {
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
    nativeCall(this, binding) {
      val out = allocate(8, 8)
      if (present(C.mln_render_session_acquire_frame(handle, out, diagnostic), absent = -9))
        adopt(out, GeneratedOwnerDisposal::acquiredFrame) {
          AcquiredFrameHandle(it, this@GeneratedRenderSessionOperations as RenderSessionHandle)
        }
      else null
    }

  /**
   * Ends every frame demand that has not started with `MLN_RENDER_RESULT_NO_UPDATE` and completes
   * after earlier driver work has finished. A running demand finishes its attempt and ends any wait
   * for a map update or a free slot. After completion, a new frame demand is required to render
   * again.
   *
   * See `mln_render_session_barrier` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun barrier(): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(C.mln_render_session_barrier(handle, completion, diagnostic))
    }

  /**
   * Starts asynchronous renderer-data clearing.
   *
   * See `mln_render_session_clear_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun clearData(): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(C.mln_render_session_clear_data(handle, completion, diagnostic))
    }

  /**
   * Starts normal graphics-owner teardown and map detachment.
   *
   * See `mln_render_session_detach` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun detach(): Deferred<Unit> =
    nativeUnit(this, binding) { check(C.mln_render_session_detach(handle, completion, diagnostic)) }

  /**
   * Consumes a session in any state and retires it, then runs completion, if given, once the host
   * may destroy the target's graphics objects.
   *
   * See `mln_render_session_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun release(): Deferred<Unit> =
    nativeRetire(this, binding) {
      check(C.mln_render_session_dispose(handle, completion, diagnostic))
    }

  /**
   * Consumes a session in any state and retires it, then runs completion, if given, once the host
   * may destroy the target's graphics objects.
   *
   * See `mln_render_session_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun dispose(): Unit =
    nativeClose(this, binding) { check(C.mln_render_session_dispose(handle, 0L, diagnostic)) }

  /**
   * Drains every currently queued terminal frame result into an independently owned batch and
   * borrows its view. The records remain stable until the batch is released.
   *
   * See `mln_render_session_drain_frame_results` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun drainFrameResults(): List<RenderFrameResult> =
    nativeCall(this, binding) {
      val batch = allocate(8, 8)
      val out = sized(w(16, 24), w(4, 8))
      if (
        present(
          C.mln_render_session_drain_frame_results(handle, batch, out, diagnostic),
          absent = -9,
        )
      )
        drain(batch, { C.mln_render_frame_batch_release(it) }) { readRenderFrameBatchView(out) }
      else emptyList()
    }

  /**
   * Starts asynchronous renderer diagnostic-log emission.
   *
   * See `mln_render_session_dump_debug_logs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun dumpDebugLogs(): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(C.mln_render_session_dump_debug_logs(handle, completion, diagnostic))
    }

  /**
   * Returns the immutable capabilities fixed during attachment.
   *
   * See `mln_render_session_get_capabilities` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun getCapabilities(): RenderSessionCapabilities =
    nativeCall(this, binding) {
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
    nativeCall(this, binding) {
      val out = sized(64, 8)
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
    arguments: ByteArray,
  ): Deferred<ByteArray> =
    nativeSubmit(this, binding, { result -> readView(CompletionBridge.value(result)) }) {
      check(
        C.mln_render_session_query_feature_extensions(
          handle,
          view(sourceId),
          view(feature),
          view(extension),
          view(extensionField),
          view(arguments),
          completion,
          diagnostic,
        ),
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
  ): Deferred<List<RenderedFeature>> =
    nativeSubmit(
      this,
      binding,
      { result ->
        readStrided(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          CompletionBridge.valueSize(result),
          w(40, 80),
        ) {
          readRenderedFeature(it)
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
        ),
      )
    }

  /**
   * Starts a source-feature query against the session's latest driver state.
   *
   * See `mln_render_session_query_source_features` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  public fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions? = null,
  ): Deferred<List<SourceFeature>> =
    nativeSubmit(
      this,
      binding,
      { result ->
        readStrided(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          CompletionBridge.valueSize(result),
          w(16, 32),
        ) {
          readSourceFeature(it)
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
        ),
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
    nativeUnit(this, binding) {
      check(C.mln_render_session_reduce_memory_use(handle, completion, diagnostic))
    }

  /**
   * Requests a frame without waiting. Every accepted demand produces one terminal result record.
   * Caller-driver sessions require service on the graphics thread. Core-worker sessions need no
   * host service.
   *
   * See `mln_render_session_request_frame` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun requestFrame(demand: FrameDemand): Unit =
    nativeCall(this, binding) {
      check(C.mln_render_session_request_frame(handle, writeFrameDemand(demand), diagnostic))
    }

  /**
   * Starts an ordered logical resize of the driver and the map. The driver applies the extent in
   * order with its other work, and later driver work does not wait for the map. The completion runs
   * after both have applied it and the map has published its snapshot. Frames render once a map
   * update matches the applied extent; static and tile maps publish that update for their next
   * still image.
   *
   * See `mln_render_session_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun resize(extent: LogicalExtent): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_render_session_resize(handle, writeLogicalExtent(extent), completion, diagnostic))
    }

  /**
   * Services every queued item for a caller-graphics-thread driver. The first successful service
   * call fixes the session's graphics-thread identity; later calls from another native thread
   * return `MLN_STATUS_WRONG_THREAD`. The target context must be current. Core-worker sessions
   * return `MLN_STATUS_INVALID_STATE`.
   *
   * See `mln_render_session_service_driver_work` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun serviceDriverWork(): Unit =
    nativeCall(this, binding) {
      check(C.mln_render_session_service_driver_work(handle, diagnostic))
    }

  /**
   * Starts an ordered replacement of every texture of a caller-owned OpenGL ring.
   *
   * See `mln_render_session_set_opengl_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun setOpenglBorrowedTextureTarget(
    descriptor: OpenglBorrowedTextureDescriptor,
  ): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(
        C.mln_render_session_set_opengl_borrowed_texture_target(
          handle,
          writeOpenglBorrowedTextureDescriptor(descriptor),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Starts an ordered OpenGL surface replacement.
   *
   * See `mln_render_session_set_opengl_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  public fun setOpenglSurfaceTarget(descriptor: OpenglSurfaceDescriptor): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(
        C.mln_render_session_set_opengl_surface_target(
          handle,
          writeOpenglSurfaceDescriptor(descriptor),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * This handle's commands without their completions. A discarding call returns once native accepts
   * the command, and native logs a command that fails as a warning.
   */
  public val discarding: RenderSessionDiscarding
    get() = RenderSessionDiscarding(this)
}

/** The commands of a [RenderSessionHandle] that drop their completions. */
@JvmInline
public value class RenderSessionDiscarding
internal constructor(internal val owner: GeneratedRenderSessionOperations) {
  internal val binding
    get() = owner.binding

  /** [RenderSessionHandle.resize] without its completion. */
  public fun resize(extent: LogicalExtent): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_render_session_resize(handle, writeLogicalExtent(extent), completion, diagnostic))
    }
}
