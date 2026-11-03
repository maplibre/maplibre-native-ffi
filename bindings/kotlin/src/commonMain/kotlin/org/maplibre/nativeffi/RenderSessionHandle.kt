// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlin.jvm.JvmInline
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A render session draws a map into a surface or a texture.
 *
 * See `mln_render_session` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class RenderSessionHandle
internal constructor(
  handle: Long,
  @Suppress("unused") private val parent: MapHandle,
) : AsyncReleasable, AutoCloseable {
  internal val binding =
    HandleState("RenderSessionHandle", handle, GeneratedOwnerDisposal::renderSession)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after close. */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, close, or discard has ended the handle. */
  public override val isClosed: Boolean
    get() = binding.isReleased

  public override fun close() {
    dispose()
  }

  internal val bindingCallbacks: CallbackOwner = CallbackOwner()

  /**
   * Stops the session right away, without running its pending graphics work.
   *
   * Use it when the target's graphics objects are going away and a detach can't finish first.
   * Pending work completes with `Status.TARGET_LOST`, and pending frame requests finish with
   * `RenderResult.TARGET_NOT_READY`. Once the call returns, the library no longer uses your
   * graphics objects, and you can attach another session to the map. Dispose the session afterward.
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
   * Takes the newest rendered frame from the session's texture ring, if one is ready.
   *
   * Older frames that you haven't acquired are dropped, and the session can render over a frame
   * before you acquire it. While you hold a frame in every texture of the ring, new frames wait
   * until you release one. When no frame is ready, it succeeds and writes the null handle.
   *
   * See `mln_render_session_acquire_frame` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.UNSUPPORTED` when the session lacks `RenderSessionCapabilityFlag.FRAME_ACQUISITION`.
   * - `Status.INVALID_STATE` when the session has failed.
   */
  public fun acquireFrame(): AcquiredFrameHandle? =
    nativeCall(this, binding) {
      val out = allocate(8, w(8, 8, 4))
      if (published(C.mln_render_session_acquire_frame(handle, out, diagnostic), out))
        adopt(out, GeneratedOwnerDisposal::acquiredFrame) {
          AcquiredFrameHandle(it, this@RenderSessionHandle)
        }
      else null
    }

  /**
   * Ends the session's pending frame requests and waits for its earlier work.
   *
   * Frame requests that haven't started, including those that wait for a map update, finish with
   * `RenderResult.NO_UPDATE`.
   *
   * See `mln_render_session_barrier` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.TARGET_LOST` when the session loses its target before the barrier runs.
   */
  public fun barrier(): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(C.mln_render_session_barrier(handle, completion, diagnostic))
    }

  /**
   * Clears the data that the session's renderer holds, such as loaded tiles, and draws the map
   * again.
   *
   * See `mln_render_session_clear_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.TARGET_LOST` when the session loses its target before the work runs.
   */
  public fun clearData(): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(C.mln_render_session_clear_data(handle, completion, diagnostic))
    }

  /**
   * Detaches the session from its map and frees its graphics objects.
   *
   * Release every acquired frame first. Pending frame requests finish with
   * `RenderResult.TARGET_NOT_READY`. Once the detach completes, the library no longer uses your
   * graphics objects, and you can attach another session to the map. Dispose the session afterward.
   *
   * See `mln_render_session_detach` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.TARGET_LOST` when the session loses its target before the detach runs.
   */
  public fun detach(): Deferred<Unit> =
    nativeUnit(this, binding) { check(C.mln_render_session_detach(handle, completion, diagnostic)) }

  /**
   * Releases the session in any state.
   *
   * An attached session driven by `RenderDriverKind.CORE_WORKER` detaches first unless you hold one
   * of its frames. Any other session is abandoned as `RenderSessionHandle.abandon()` does. To free
   * the graphics objects of a session driven by `RenderDriverKind.CALLER_GRAPHICS_THREAD`, detach
   * it before you dispose it. Once the disposal completes, the library no longer uses your graphics
   * objects.
   *
   * See `mln_render_session_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public override fun release(): Deferred<Unit> =
    nativeRetire(this, binding) {
      check(C.mln_render_session_dispose(handle, completion, diagnostic))
    }

  /**
   * Releases the session in any state.
   *
   * An attached session driven by `RenderDriverKind.CORE_WORKER` detaches first unless you hold one
   * of its frames. Any other session is abandoned as `RenderSessionHandle.abandon()` does. To free
   * the graphics objects of a session driven by `RenderDriverKind.CALLER_GRAPHICS_THREAD`, detach
   * it before you dispose it. Once the disposal completes, the library no longer uses your graphics
   * objects.
   *
   * See `mln_render_session_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun dispose(): Unit =
    nativeClose(this, binding) { check(C.mln_render_session_dispose(handle, 0L, diagnostic)) }

  /**
   * Takes the frame results that the session has queued.
   *
   * When no result is queued, it succeeds and writes the null handle and an empty view.
   *
   * See `mln_render_session_drain_frame_results` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun drainFrameResults(): List<RenderFrameResult> =
    nativeCall(this, binding) {
      val batch = allocate(8, w(8, 8, 4))
      val out = sized(w(16, 24), w(4, 8))
      if (
        published(C.mln_render_session_drain_frame_results(handle, batch, out, diagnostic), batch)
      )
        drain(batch, { C.mln_render_frame_batch_release(it) }) { readRenderFrameBatchView(out) }
      else emptyList()
    }

  /**
   * Logs debugging information about the session's renderer.
   *
   * See `mln_render_session_dump_debug_logs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.TARGET_LOST` when the session loses its target before the work runs.
   */
  public fun dumpDebugLogs(): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(C.mln_render_session_dump_debug_logs(handle, completion, diagnostic))
    }

  /**
   * Reads what the session supports.
   *
   * See `mln_render_session_get_capabilities` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun getCapabilities(): RenderSessionCapabilities =
    nativeCall(this, binding) {
      val out = sized(24, 4)
      check(C.mln_render_session_get_capabilities(handle, out, diagnostic))
      readRenderSessionCapabilities(out)
    }

  /**
   * Reads the message of the error that made the session fail.
   *
   * `RenderSessionSnapshot.failureStatus` reports the error's status. The message is empty while
   * the session hasn't failed, and doesn't change once it fails.
   *
   * See `mln_render_session_get_failure` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun getFailure(): String =
    nativeCall(this, binding) {
      val textLength = allocate(w(4, 8), w(4, 8))
      var textCapacity = 256L
      var textStorage = allocate(textCapacity, 1)
      while (true) {
        check(
          C.mln_render_session_get_failure(
            handle,
            textStorage,
            textCapacity,
            textLength,
            diagnostic,
          ),
        )
        val written = readSize(textLength).toLong()
        if (written < textCapacity) break
        textCapacity = written + 1
        textStorage = allocate(textCapacity, 1)
      }
      readBytes(textStorage, readSize(textLength)).decodeToString()
    }

  /**
   * Reads the session's current state.
   *
   * See `mln_render_session_get_snapshot` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun getSnapshot(): RenderSessionSnapshot =
    nativeCall(this, binding) {
      val out = sized(48, w(8, 8, 4))
      check(C.mln_render_session_get_snapshot(handle, out, diagnostic))
      readRenderSessionSnapshot(out)
    }

  /**
   * Reads a value that a source computes for one of its features.
   *
   * A clustered GeoJSON source answers the `supercluster` extension for a cluster feature. Its
   * `extensionField` is `children` for the cluster's child features, `leaves` for the points in the
   * cluster, or `expansion-zoom` for the zoom level where the cluster splits. For `leaves`,
   * `arguments` can set `limit` and `offset`. Without `limit`, the result has up to 10 points and
   * ignores `offset`. The result is JSON: a GeoJSON feature collection, a number, or null when the
   * source has no answer.
   *
   * See `mln_render_session_query_feature_extensions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  public fun queryFeatureExtensions(
    sourceId: String,
    feature: String,
    extension: String,
    extensionField: String,
    arguments: String,
  ): Deferred<String> =
    nativeSubmit(this, binding, { result -> readViewString(CompletionBridge.value(result)) }) {
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
   * Finds the features that the session drew in an area of the screen.
   *
   * The query reads the last frame that the session rendered. A feature that several layers draw
   * appears once for each layer.
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
   * Finds the features of the source named `sourceId`.
   *
   * The query covers only the tiles that the source has loaded for the current view. A source loads
   * tiles only while a visible layer uses it.
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
   * Reads the pixels of the last frame that the session rendered.
   *
   * See `mln_render_session_read_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.UNSUPPORTED` when the session's capabilities lack
   *   `RenderSessionCapabilityFlag.READBACK`.
   * - `Status.INVALID_STATE` when the session hasn't rendered a frame since it attached, resized,
   *   or changed its target.
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
   * Frees memory that the session's renderer can rebuild, such as caches.
   *
   * See `mln_render_session_reduce_memory_use` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.TARGET_LOST` when the session loses its target before the work runs.
   */
  public fun reduceMemoryUse(): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(C.mln_render_session_reduce_memory_use(handle, completion, diagnostic))
    }

  /**
   * Lets go of the session's surface or textures, and keeps the rest of the session.
   *
   * Use it when the platform takes a surface away, such as when your app goes to the background.
   * Release every acquired frame first. Until you give the session a new target, frame requests
   * finish with `RenderResult.TARGET_NOT_READY`. Once the release completes, you can destroy the
   * old target.
   *
   * See `mln_render_session_release_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.UNSUPPORTED` when the session renders offscreen.
   * - `Status.UNSUPPORTED` when the target is a WebGL canvas.
   * - `Status.TARGET_LOST` when the session loses its target before the release runs.
   */
  public fun releaseTarget(): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(C.mln_render_session_release_target(handle, completion, diagnostic))
    }

  /**
   * Asks the session to render a frame.
   *
   * Call it once the attach completes. Every request finishes with one frame result that carries
   * its token, and `RenderSessionHandle.drainFrameResults()` takes it. A request without
   * `FrameDemandFlag.FORCE` replaces each pending request with the same flags.
   *
   * See `mln_render_session_request_frame` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_STATE` when the session has failed.
   */
  public fun requestFrame(demand: FrameDemand): Unit =
    nativeCall(this, binding) {
      check(C.mln_render_session_request_frame(handle, writeFrameDemand(demand), diagnostic))
    }

  /**
   * Resizes the session's target and its map.
   *
   * `extent` must keep the session's scale factor. To change the scale factor, give the session a
   * new target or attach a new session. Release every acquired frame first.
   *
   * See `mln_render_session_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.UNSUPPORTED` when the session renders into a borrowed ring. Give it new textures of
   *   the new size instead.
   * - `Status.TARGET_LOST` when the session loses its target before the resize applies.
   */
  public fun resize(extent: LogicalExtent): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_render_session_resize(handle, writeLogicalExtent(extent), completion, diagnostic))
    }

  /**
   * Runs the session's ready graphics work on your graphics thread.
   *
   * Use it with `RenderDriverKind.CALLER_GRAPHICS_THREAD`, and call it from the same thread every
   * time. Call it each time `RenderSessionAttachOptions.driverWorkWake` is called, even while you
   * request no frames, and until a detach completes. It returns once no work is ready.
   *
   * See `mln_render_session_service_driver_work` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun serviceDriverWork(): Unit =
    nativeCall(this, binding) {
      check(C.mln_render_session_service_driver_work(handle, diagnostic))
    }

  /**
   * Replaces the OpenGL textures that a borrowed ring renders into.
   *
   * Use it to resize the ring, or to give the session textures again after
   * `RenderSessionHandle.releaseTarget()`. Release every acquired frame first. The new ring must
   * have as many textures as the old one, and the same context. Once the replacement completes, the
   * session no longer uses the old textures.
   *
   * See `mln_render_session_set_open_gl_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun setOpenGlBorrowedTextureTarget(
    descriptor: OpenGlBorrowedTextureDescriptor,
  ): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(
        C.mln_render_session_set_open_gl_borrowed_texture_target(
          handle,
          writeOpenGlBorrowedTextureDescriptor(descriptor),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Replaces the OpenGL surface that the session presents to.
   *
   * Use it to give the session a surface again after `RenderSessionHandle.releaseTarget()`, or to
   * move it to another surface. The new surface must use the same context. Once the replacement
   * completes, the session no longer uses the old surface.
   *
   * See `mln_render_session_set_open_gl_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  public fun setOpenGlSurfaceTarget(descriptor: OpenGlSurfaceDescriptor): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(
        C.mln_render_session_set_open_gl_surface_target(
          handle,
          writeOpenGlSurfaceDescriptor(descriptor),
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

  override fun toString(): String =
    "RenderSessionHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"
}

/** The commands of a [RenderSessionHandle] that drop their completions. */
@JvmInline
public value class RenderSessionDiscarding
internal constructor(internal val owner: RenderSessionHandle) {
  internal val binding
    get() = owner.binding

  /** [RenderSessionHandle.resize] without its completion. */
  public fun resize(extent: LogicalExtent): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_render_session_resize(handle, writeLogicalExtent(extent), completion, diagnostic))
    }
}
