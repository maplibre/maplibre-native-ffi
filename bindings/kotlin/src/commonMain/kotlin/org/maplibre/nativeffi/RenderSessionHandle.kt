// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlin.jvm.JvmName
import org.maplibre.nativeffi.internal.JvmSynthetic
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
private constructor(
  handle: Long,
  @Suppress("unused") private val parent: MapHandle,
  /**
   * Completes once the session is ready, and fails with a [MaplibreException] when it fails to
   * become ready. Asynchronous calls on the session before then wait for it.
   */
  public val ready: Completion<Unit>,
) : AsyncReleasable, AutoCloseable {
  @get:JvmSynthetic
  internal val binding =
    HandleState("RenderSessionHandle", handle, GeneratedOwnerDisposal::renderSession)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after release. */
  @get:JvmName("getId")
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, or discard has ended the handle. */
  public override val isClosed: Boolean
    get() = binding.isReleased

  /**
   * Calls [dispose], for the standard library's `use`, which can't wait. [useReleasing] waits for
   * [release] instead.
   */
  public override fun close() {
    dispose()
  }

  @get:JvmSynthetic internal val bindingCallbacks: CallbackOwner = CallbackOwner()

  /**
   * Stops the session right away, without running its pending graphics work.
   *
   * Use it when the target's graphics objects are going away and a detach can't finish first.
   * Pending work completes with `Status.TARGET_LOST`, and pending frame demands finish with
   * `RenderResult.TARGET_NOT_READY`. Once the call returns, the library no longer uses your
   * graphics objects, and you can attach another session to the map. Release the session afterward.
   *
   * Abandon keeps the session's OpenGL and WebGPU objects until the process exits, and reports how
   * many it kept. Detach the session instead to free them.
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
   * - `Status.TARGET_LOST` or `Status.NATIVE_ERROR`, as `RenderSessionSnapshot.failureStatus`
   *   reports, when the session has failed.
   * - `Status.INVALID_STATE` when you detached, abandoned, released, or disposed the session.
   */
  public fun acquireFrame(): AcquiredFrameHandle? =
    nativeCall(this, binding) {
      val out = allocate(8, w(8, 8, 4))
      if (published(C.mln_render_session_acquire_frame(handle, out, diagnostic), out))
        adopt(out, GeneratedOwnerDisposal::acquiredFrame) {
          AcquiredFrameHandle.wrap(it, this@RenderSessionHandle)
        }
      else null
    }

  /**
   * Ends the session's pending frame demands and waits for its earlier work.
   *
   * Frame demands that haven't started, including those that wait for a map update, finish with
   * `RenderResult.NO_UPDATE`.
   *
   * See `mln_render_session_barrier` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.TARGET_LOST` when the session loses its target before the barrier runs.
   */
  public fun barrier(): Completion<Unit> =
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
  public fun clearData(): Completion<Unit> =
    nativeUnit(this, binding) {
      check(C.mln_render_session_clear_data(handle, completion, diagnostic))
    }

  /**
   * Asks the session to render a frame.
   *
   * You can call it once the attach call returns. While the attach runs, the demand waits for it to
   * complete. Every demand finishes with one frame result that carries its token, and
   * `RenderSessionHandle.drainFrameResults()` takes it. A demand without `FrameDemandFlag.FORCE`
   * replaces each pending demand with the same flags.
   *
   * See `mln_render_session_demand_frame` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.TARGET_LOST` or `Status.NATIVE_ERROR`, as `RenderSessionSnapshot.failureStatus`
   *   reports, when the session has failed.
   * - `Status.INVALID_STATE` when you detached, abandoned, released, or disposed the session, or
   *   discarded a frame from every texture of its ring.
   */
  public fun demandFrame(demand: FrameDemand): Unit =
    nativeCall(this, binding) {
      check(C.mln_render_session_demand_frame(handle, writeFrameDemand(demand), diagnostic))
    }

  /**
   * Detaches the session from its map and frees its graphics objects.
   *
   * Release every acquired frame first. Pending frame demands finish with
   * `RenderResult.TARGET_NOT_READY`. Once the detach completes, the library no longer uses your
   * graphics objects, and you can attach another session to the map. Release the session afterward.
   *
   * See `mln_render_session_detach` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.TARGET_LOST` when the session loses its target before the detach runs.
   */
  public fun detach(): Completion<Unit> =
    nativeUnit(this, binding) { check(C.mln_render_session_detach(handle, completion, diagnostic)) }

  /**
   * Releases the session in any state, and finishes in the background.
   *
   * Use it from destructors and finalizers, and `RenderSessionHandle.release()` elsewhere. It ends
   * the session as a release does, except that it abandons a session whose frames you hold, which
   * ends those frames. Keep servicing a session driven by `RenderDriverKind.CALLER_GRAPHICS_THREAD`
   * until `RenderSessionHandle.serviceDriverWork()` no longer accepts it.
   *
   * See `mln_render_session_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun dispose(): Unit =
    nativeClose(this, binding) { check(C.mln_render_session_dispose(handle, diagnostic)) }

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
  public fun dumpDebugLogs(): Completion<Unit> =
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
   * Reads the children of a cluster in a clustered GeoJSON source.
   *
   * `clusterId` is the "cluster_id" property of a cluster feature. The result is a GeoJSON feature
   * collection of the clusters and points that the cluster splits into at the next zoom level.
   *
   * The query reads the source's data from the session's most recent render attempt, so a change to
   * the data shows once the session tries to render again.
   *
   * A cluster ID belongs to one version of the source's data. After the data changes, feature
   * queries can return IDs from the earlier version until the source's tiles reload, and such an ID
   * completes with `Status.NOT_FOUND` or names a different cluster. Query the features again for
   * current IDs.
   *
   * See `mln_render_session_get_cluster_children` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the session's most recent render attempt had no source named
   *   `sourceId`, or the source has no cluster `clusterId`.
   * - `Status.INVALID_ARGUMENT` when the source isn't a GeoJSON source with clustering turned on.
   */
  @JvmName("getClusterChildren")
  public fun getClusterChildren(sourceId: String, clusterId: UInt): Completion<String> =
    nativeSubmit(this, binding, { result -> readViewString(CompletionBridge.value(result)) }) {
      check(
        C.mln_render_session_get_cluster_children(
          handle,
          view(sourceId),
          clusterId.toInt(),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Reads the zoom level at which a cluster of a clustered GeoJSON source splits.
   *
   * `clusterId` is the "cluster_id" property of a cluster feature. At the result's zoom level, the
   * cluster splits into two or more clusters or points.
   *
   * A cluster ID can go out of date as `RenderSessionHandle.getClusterChildren()` describes.
   *
   * See `mln_render_session_get_cluster_expansion_zoom` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the session's most recent render attempt had no source named
   *   `sourceId`, or the source has no cluster `clusterId`.
   * - `Status.INVALID_ARGUMENT` when the source isn't a GeoJSON source with clustering turned on.
   */
  @JvmSynthetic
  public fun getClusterExpansionZoom(sourceId: String, clusterId: UInt): Completion<UInt> =
    nativeSubmit(this, binding, { result -> readU32(CompletionBridge.value(result)) }) {
      check(
        C.mln_render_session_get_cluster_expansion_zoom(
          handle,
          view(sourceId),
          clusterId.toInt(),
          completion,
          diagnostic,
        ),
      )
    }

  @PublishedApi
  @JvmName("getClusterExpansionZoom")
  internal fun getClusterExpansionZoomForJava(sourceId: String, clusterId: Int): Completion<Int> =
    nativeSubmit(this, binding, { result -> readU32(CompletionBridge.value(result)).toInt() }) {
      check(
        C.mln_render_session_get_cluster_expansion_zoom(
          handle,
          view(sourceId),
          clusterId.toInt(),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Reads the points in a cluster of a clustered GeoJSON source.
   *
   * `clusterId` is the "cluster_id" property of a cluster feature. The result is a GeoJSON feature
   * collection of the original point features. It skips the first `offset` points and then holds up
   * to `limit` of them, so a `limit` of 0 gives an empty collection.
   *
   * A cluster ID can go out of date as `RenderSessionHandle.getClusterChildren()` describes.
   *
   * See `mln_render_session_get_cluster_leaves` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the session's most recent render attempt had no source named
   *   `sourceId`, or the source has no cluster `clusterId`.
   * - `Status.INVALID_ARGUMENT` when the source isn't a GeoJSON source with clustering turned on.
   */
  @JvmName("getClusterLeaves")
  public fun getClusterLeaves(
    sourceId: String,
    clusterId: UInt,
    limit: UInt,
    offset: UInt,
  ): Completion<String> =
    nativeSubmit(this, binding, { result -> readViewString(CompletionBridge.value(result)) }) {
      check(
        C.mln_render_session_get_cluster_leaves(
          handle,
          view(sourceId),
          clusterId.toInt(),
          limit.toInt(),
          offset.toInt(),
          completion,
          diagnostic,
        ),
      )
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
      val out = sized(w(56, 56, 52), w(8, 8, 4))
      check(C.mln_render_session_get_snapshot(handle, out, diagnostic))
      readRenderSessionSnapshot(out)
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
  ): Completion<List<RenderedFeature>> =
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
  ): Completion<List<SourceFeature>> =
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
  public fun readTexture(): Completion<TextureReadbackResult> =
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
  public fun reduceMemoryUse(): Completion<Unit> =
    nativeUnit(this, binding) {
      check(C.mln_render_session_reduce_memory_use(handle, completion, diagnostic))
    }

  /**
   * Releases the session, detaching it first when it is attached.
   *
   * Pending frame demands finish with `RenderResult.TARGET_NOT_READY`. Once the release completes,
   * the library no longer uses your graphics objects, and you can attach another session to the
   * map.
   *
   * A session driven by `RenderDriverKind.CALLER_GRAPHICS_THREAD` detaches at your next
   * `RenderSessionHandle.serviceDriverWork()`, so keep servicing it until the release completes.
   * When the session isn't attached, such as while it attaches or after it fails, or when you
   * release or dispose its map before that service, the release abandons it as
   * `RenderSessionHandle.abandon()` does.
   *
   * See `mln_render_session_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_STATE` while you hold one of the session's acquired frames. Release or
   *   discard them first.
   */
  public override fun release(): Completion<Unit> =
    nativeRetire(this, binding) {
      check(C.mln_render_session_release(handle, completion, diagnostic))
    }

  /**
   * Lets go of the session's surface or textures, and keeps the rest of the session.
   *
   * Use it when the platform takes a surface away, such as when your app goes to the background.
   * Release every acquired frame first. Frame demands that are still pending, and later ones until
   * you give the session a new target, finish with `RenderResult.TARGET_NOT_READY`. Once the
   * release completes, you can destroy the old target.
   *
   * See `mln_render_session_release_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.UNSUPPORTED` when the session renders offscreen or into Android hardware buffers,
   *   dmabufs, DXGI shared textures, or IOSurfaces that it allocates.
   * - `Status.UNSUPPORTED` when the target is a WebGL canvas.
   * - `Status.TARGET_LOST` when the session loses its target before the release runs.
   */
  public fun releaseTarget(): Completion<Unit> =
    nativeUnit(this, binding) {
      check(C.mln_render_session_release_target(handle, completion, diagnostic))
    }

  /**
   * Resizes the session's target and its map.
   *
   * The session renders at the map's scale factor. Release every acquired frame first.
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
  public fun resize(logicalSize: LogicalSize): Completion<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_render_session_resize(handle, writeLogicalSize(logicalSize), completion, diagnostic),
      )
    }

  /**
   * Runs the session's ready graphics work on your graphics thread.
   *
   * Use it with `RenderDriverKind.CALLER_GRAPHICS_THREAD`, and call it from the same thread every
   * time. Call it each time `RenderSessionAttachOptions.driverWorkWake` is called, even while you
   * demand no frames, until a detach, release, or disposal of the session finishes. It returns once
   * no work is ready.
   *
   * It accepts a released or disposed session until that release or disposal finishes.
   *
   * See `mln_render_session_service_driver_work` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun serviceDriverWork(): Unit =
    nativeCall(this, binding) {
      check(C.mln_render_session_service_driver_work(handle, diagnostic))
    }

  /**
   * Sets the frame pacer that demands the session's frames.
   *
   * At the first display refresh after the session has something new to draw, `pacer` demands a
   * frame without flags and with token 0. One pacer can serve many sessions. A null `pacer` stops
   * pacing the session. You can call it once the attach call returns.
   *
   * See `mln_render_session_set_frame_pacer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.TARGET_LOST` or `Status.NATIVE_ERROR`, as `RenderSessionSnapshot.failureStatus`
   *   reports, when the session has failed.
   * - `Status.INVALID_STATE` when you attached the session with a redraw wake, or detached,
   *   abandoned, released, or disposed it.
   */
  public fun setFramePacer(pacer: FramePacerHandle? = null): Unit =
    nativeCall(this, binding) {
      check(
        C.mln_render_session_set_frame_pacer(
          handle,
          pacer?.let { argument(it, it.binding) } ?: 0L,
          diagnostic,
        ),
      )
    }

  /**
   * Replaces the OpenGL textures that a borrowed ring renders into.
   *
   * Use it to resize the ring, or to give the session textures again after
   * `RenderSessionHandle.releaseTarget()`. Release every acquired frame first. Once the replacement
   * completes, the session no longer uses the old textures.
   *
   * See `mln_render_session_set_open_gl_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun setOpenGlBorrowedTextureTarget(
    replacement: OpenGlBorrowedTextureReplacement,
  ): Completion<Unit> =
    nativeUnit(this, binding) {
      check(
        C.mln_render_session_set_open_gl_borrowed_texture_target(
          handle,
          writeOpenGlBorrowedTextureReplacement(replacement),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Replaces the OpenGL surface that the session presents to.
   *
   * Use it to give the session a surface again after `RenderSessionHandle.releaseTarget()`, to move
   * it to another surface, or to resize it. Once the replacement completes, the session no longer
   * uses the old surface.
   *
   * See `mln_render_session_set_open_gl_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  public fun setOpenGlSurfaceTarget(replacement: OpenGlSurfaceReplacement): Completion<Unit> =
    nativeUnit(this, binding) {
      check(
        C.mln_render_session_set_open_gl_surface_target(
          handle,
          writeOpenGlSurfaceReplacement(replacement),
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
    get() = RenderSessionDiscarding.wrap(this)

  override fun toString(): String =
    "RenderSessionHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"

  internal companion object {
    /** Creates the owner of `handle`. */
    @JvmSynthetic
    internal fun wrap(
      handle: Long,
      parent: MapHandle,
      ready: Completion<Unit> = Completion.completed(Unit),
    ): RenderSessionHandle = RenderSessionHandle(handle, parent, ready)
  }
}

/** The commands of a [RenderSessionHandle] that drop their completions. */
public class RenderSessionDiscarding
private constructor(@get:JvmSynthetic internal val owner: RenderSessionHandle) {
  @get:JvmSynthetic
  internal val binding
    get() = owner.binding

  /** [RenderSessionHandle.resize] without its completion. */
  public fun resize(logicalSize: LogicalSize): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_render_session_resize(handle, writeLogicalSize(logicalSize), completion, diagnostic),
      )
    }

  internal companion object {
    /** Creates the discarding view of `owner`. */
    @JvmSynthetic
    internal fun wrap(owner: RenderSessionHandle): RenderSessionDiscarding =
      RenderSessionDiscarding(owner)
  }
}
