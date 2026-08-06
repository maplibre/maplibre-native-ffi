package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeRenderSession
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.GeoJsonMarshal
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.HeapArena
import org.maplibre.nativeffi.internal.wasm.HeapPointer
import org.maplibre.nativeffi.internal.wasm.InjectedFaults
import org.maplibre.nativeffi.internal.wasm.JsonMarshal
import org.maplibre.nativeffi.internal.wasm.RenderMarshal
import org.maplibre.nativeffi.internal.wasm.generated.mln_feature_extension_result_destroy
import org.maplibre.nativeffi.internal.wasm.generated.mln_feature_extension_result_get
import org.maplibre.nativeffi.internal.wasm.generated.mln_feature_query_result_count
import org.maplibre.nativeffi.internal.wasm.generated.mln_feature_query_result_destroy
import org.maplibre.nativeffi.internal.wasm.generated.mln_feature_query_result_get
import org.maplibre.nativeffi.internal.wasm.generated.mln_json_snapshot_destroy
import org.maplibre.nativeffi.internal.wasm.generated.mln_json_snapshot_get
import org.maplibre.nativeffi.internal.wasm.generated.mln_opengl_borrowed_texture_set_target
import org.maplibre.nativeffi.internal.wasm.generated.mln_opengl_owned_texture_acquire_frame
import org.maplibre.nativeffi.internal.wasm.generated.mln_opengl_owned_texture_release_frame
import org.maplibre.nativeffi.internal.wasm.generated.mln_opengl_surface_set_target
import org.maplibre.nativeffi.internal.wasm.generated.mln_render_session_clear_data
import org.maplibre.nativeffi.internal.wasm.generated.mln_render_session_destroy
import org.maplibre.nativeffi.internal.wasm.generated.mln_render_session_detach
import org.maplibre.nativeffi.internal.wasm.generated.mln_render_session_dump_debug_logs
import org.maplibre.nativeffi.internal.wasm.generated.mln_render_session_get_feature_state
import org.maplibre.nativeffi.internal.wasm.generated.mln_render_session_query_feature_extensions
import org.maplibre.nativeffi.internal.wasm.generated.mln_render_session_query_rendered_features
import org.maplibre.nativeffi.internal.wasm.generated.mln_render_session_query_source_features
import org.maplibre.nativeffi.internal.wasm.generated.mln_render_session_reduce_memory_use
import org.maplibre.nativeffi.internal.wasm.generated.mln_render_session_remove_feature_state
import org.maplibre.nativeffi.internal.wasm.generated.mln_render_session_render_update
import org.maplibre.nativeffi.internal.wasm.generated.mln_render_session_resize
import org.maplibre.nativeffi.internal.wasm.generated.mln_render_session_set_feature_state
import org.maplibre.nativeffi.internal.wasm.generated.mln_texture_read_premultiplied_rgba8
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.query.FeatureExtensionResult
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

/**
 * A render session, owned by the thread this binding runs on.
 *
 * That thread created the runtime the session's map belongs to, so it is the session's owner thread
 * as far as the C API is concerned, and every call below is an ordinary synchronous call made on
 * it.
 *
 * The browser build compiles one render backend, OpenGL against WebGL, and every render target that
 * backend has: a native surface, which here is the canvas the context is bound to, and both the
 * owned and the borrowed texture target. See the Metal and Vulkan members below for what compiling
 * one backend leaves unreachable.
 *
 * A session holds its WebGL context open for as long as it borrows it. The backend makes that
 * context current on every frame and again while it releases its GL objects, so closing the context
 * first returns an invalid-state status naming this session rather than leaving native to work in a
 * context that is gone. [detach] and [close] release it.
 */
public actual class RenderSessionHandle
internal constructor(
  private val map: MapHandle,
  private val handle: NativeRenderSession,
  // Null only for the WGL and EGL arms of an OpenGL context descriptor, which name a graphics API
  // this module was not built against and which native refuses at attach.
  private val contextRetention: HandleStateCore.ChildRetention?,
) : AutoCloseable {
  // Held so that closing the map while a session is attached is the binding's own INVALID_STATE,
  // which is what the common KDoc promises. Naming the map to HandleStateCore does not do that.
  private val mapRetention = map.retainChild(TYPE_NAME)
  private val core = HandleStateCore(TYPE_NAME, handle.raw, map)
  private val activeFrame = ActiveFrameState()

  /**
   * Checks this handle is live and then runs [body], without holding a use count across it.
   *
   * A use count covers a host that uses a handle on one thread and closes it on another, and this
   * binding has one thread: a close can only arrive from a frame below the call it would wait for,
   * which is the wait `yieldWhileClosing` refuses to make.
   */
  private inline fun <T> live(body: () -> T): T {
    core.requireLive()
    return body()
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun map(): MapHandle = map

  public actual fun resize(width: Int, height: Int, scaleFactor: Double) {
    activeFrame.ensureInactive("resize")
    Status.requireArgument(width >= 0) { "width must be non-negative" }
    Status.requireArgument(height >= 0) { "height must be non-negative" }
    live { Status.check(mln_render_session_resize(handle.raw, width, height, scaleFactor)) }
  }

  public actual fun setMetalSurfaceTarget(descriptor: MetalSurfaceDescriptor) {
    throw unsupportedBackend("Metal")
  }

  public actual fun setVulkanSurfaceTarget(descriptor: VulkanSurfaceDescriptor) {
    throw unsupportedBackend("Vulkan")
  }

  /**
   * Takes a new extent for a surface session, which is all this can change in a browser.
   *
   * There is no surface object to replace: the session presents through the canvas its context is
   * bound to, and `descriptor.surface` must be [NativePointer.NULL] as it was at attach. Sizing
   * that canvas's drawing buffer is [WebglContext.resizeCanvas], and neither call implies the
   * other.
   */
  public actual fun setOpenGLSurfaceTarget(descriptor: OpenGLSurfaceDescriptor) {
    activeFrame.ensureInactive("set target")
    WebglContext.requireOpenForTarget(descriptor.context)
    live {
      Heap.withScratch(RenderMarshal.OPENGL_SURFACE_SIZEOF) { block ->
        RenderMarshal.writeOpenGLSurface(block, descriptor)
        Status.check(mln_opengl_surface_set_target(handle.raw, block.address))
      }
    }
  }

  public actual fun setMetalBorrowedTextureTarget(descriptor: MetalBorrowedTextureDescriptor) {
    throw unsupportedBackend("Metal")
  }

  public actual fun setVulkanBorrowedTextureTarget(descriptor: VulkanBorrowedTextureDescriptor) {
    throw unsupportedBackend("Vulkan")
  }

  public actual fun setOpenGLBorrowedTextureTarget(descriptor: OpenGLBorrowedTextureDescriptor) {
    activeFrame.ensureInactive("set target")
    WebglContext.requireOpenForTarget(descriptor.context)
    live {
      Heap.withScratch(RenderMarshal.OPENGL_BORROWED_TEXTURE_SIZEOF) { block ->
        RenderMarshal.writeOpenGLBorrowedTexture(block, descriptor)
        Status.check(mln_opengl_borrowed_texture_set_target(handle.raw, block.address))
      }
    }
  }

  public actual fun renderUpdate(): Boolean {
    activeFrame.ensureInactive("render")
    return live {
      Heap.withScratch(BOOL_BYTES) { out ->
        Status.check(mln_render_session_render_update(handle.raw, out.address))
        Heap.loadByte(out) != 0.toByte()
      }
    }
  }

  public actual fun detach() {
    activeFrame.ensureInactive("detach")
    live { Status.check(mln_render_session_detach(handle.raw)) }
    // A detached session is live for destruction and nothing else, so it holds neither the map nor
    // the WebGL context whose GL objects the detach released.
    mapRetention.close()
    contextRetention?.close()
  }

  public actual fun reduceMemoryUse() {
    activeFrame.ensureInactive("reduce memory use")
    live { Status.check(mln_render_session_reduce_memory_use(handle.raw)) }
  }

  public actual fun clearData() {
    activeFrame.ensureInactive("clear data")
    live { Status.check(mln_render_session_clear_data(handle.raw)) }
  }

  public actual fun dumpDebugLogs() {
    activeFrame.ensureInactive("dump debug logs")
    live { Status.check(mln_render_session_dump_debug_logs(handle.raw)) }
  }

  public actual fun setFeatureState(selector: FeatureStateSelector, value: JsonValue) {
    activeFrame.ensureInactive("set feature state")
    live {
      // One block for both, so both are measured before either is written.
      val bytes =
        RenderMarshal.measureFeatureStateSelector(selector) + JsonMarshal.measureValue(value, 0)
      withArena(bytes) { arena ->
        val selectorBlock = RenderMarshal.writeFeatureStateSelector(arena, selector)
        val stateBlock = JsonMarshal.write(arena, value)
        Status.check(
          mln_render_session_set_feature_state(
            handle.raw,
            selectorBlock.address,
            stateBlock.address,
          )
        )
      }
    }
  }

  public actual fun getFeatureState(selector: FeatureStateSelector): JsonValue {
    activeFrame.ensureInactive("get feature state")
    val snapshot = live {
      val bytes =
        RenderMarshal.measureFeatureStateSelector(selector) + RenderMarshal.OUT_SLOT_BYTES.toLong()
      withArena(bytes) { arena ->
        val selectorBlock = RenderMarshal.writeFeatureStateSelector(arena, selector)
        val out = arena.allocate(RenderMarshal.OUT_SLOT_BYTES, RenderMarshal.OUT_SLOT_BYTES)
        Status.check(
          mln_render_session_get_feature_state(handle.raw, selectorBlock.address, out.address)
        )
        Heap.loadLong(out)
      }
    }
    return readJsonSnapshot(snapshot) ?: JsonValue.ObjectValue(emptyList())
  }

  public actual fun removeFeatureState(selector: FeatureStateSelector) {
    activeFrame.ensureInactive("remove feature state")
    live {
      withArena(RenderMarshal.measureFeatureStateSelector(selector)) { arena ->
        val selectorBlock = RenderMarshal.writeFeatureStateSelector(arena, selector)
        Status.check(mln_render_session_remove_feature_state(handle.raw, selectorBlock.address))
      }
    }
  }

  public actual fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): List<QueriedFeature> {
    activeFrame.ensureInactive("query rendered features")
    val result = live {
      val bytes =
        RenderMarshal.measureRenderedQueryGeometry(geometry) +
          RenderMarshal.measureRenderedFeatureQueryOptions(options) +
          RenderMarshal.OUT_SLOT_BYTES.toLong()
      withArena(bytes) { arena ->
        val geometryBlock = RenderMarshal.writeRenderedQueryGeometry(arena, geometry)
        val optionsBlock = RenderMarshal.writeRenderedFeatureQueryOptions(arena, options)
        val out = arena.allocate(RenderMarshal.OUT_SLOT_BYTES, RenderMarshal.OUT_SLOT_BYTES)
        Status.check(
          mln_render_session_query_rendered_features(
            handle.raw,
            geometryBlock.address,
            optionsBlock.address,
            out.address,
          )
        )
        Heap.loadLong(out)
      }
    }
    return readFeatureQueryResult(result)
  }

  public actual fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): List<QueriedFeature> {
    activeFrame.ensureInactive("query source features")
    val result = live {
      val bytes =
        RenderMarshal.measureStringViewRoot(sourceId) +
          RenderMarshal.measureSourceFeatureQueryOptions(options) +
          RenderMarshal.OUT_SLOT_BYTES.toLong()
      withArena(bytes) { arena ->
        // A source ID is a string view by value in C, which this target passes indirectly, so the
        // argument is a pointer to the view rather than to its bytes.
        val sourceBlock = RenderMarshal.writeStringViewRoot(arena, sourceId)
        val optionsBlock = RenderMarshal.writeSourceFeatureQueryOptions(arena, options)
        val out = arena.allocate(RenderMarshal.OUT_SLOT_BYTES, RenderMarshal.OUT_SLOT_BYTES)
        Status.check(
          mln_render_session_query_source_features(
            handle.raw,
            sourceBlock.address,
            optionsBlock.address,
            out.address,
          )
        )
        Heap.loadLong(out)
      }
    }
    return readFeatureQueryResult(result)
  }

  public actual fun queryFeatureExtension(
    sourceId: String,
    feature: Feature,
    extension: String,
    extensionField: String,
    arguments: JsonValue?,
  ): FeatureExtensionResult {
    activeFrame.ensureInactive("query feature extension")
    val result = live {
      val bytes =
        RenderMarshal.measureStringViewRoot(sourceId) +
          GeoJsonMarshal.measureFeature(feature).toLong() +
          RenderMarshal.measureStringViewRoot(extension) +
          RenderMarshal.measureStringViewRoot(extensionField) +
          (arguments?.let { JsonMarshal.measureValue(it, 0) } ?: 0L) +
          RenderMarshal.OUT_SLOT_BYTES.toLong()
      withArena(bytes) { arena ->
        val sourceBlock = RenderMarshal.writeStringViewRoot(arena, sourceId)
        val featureBlock = GeoJsonMarshal.writeFeature(arena, feature)
        val extensionBlock = RenderMarshal.writeStringViewRoot(arena, extension)
        val fieldBlock = RenderMarshal.writeStringViewRoot(arena, extensionField)
        // Absent arguments reach native as the null pointer the C API documents, not as an empty
        // object, which would mean something else.
        val argumentBlock = arguments?.let { JsonMarshal.write(arena, it) } ?: HeapPointer(0)
        val out = arena.allocate(RenderMarshal.OUT_SLOT_BYTES, RenderMarshal.OUT_SLOT_BYTES)
        Status.check(
          mln_render_session_query_feature_extensions(
            handle.raw,
            sourceBlock.address,
            featureBlock.address,
            extensionBlock.address,
            fieldBlock.address,
            argumentBlock.address,
            out.address,
          )
        )
        Heap.loadLong(out)
      }
    }
    return readFeatureExtensionResult(result)
  }

  public actual fun textureImageInfo(): TextureImageInfo {
    activeFrame.ensureInactive("read texture data")
    return live {
      Heap.withScratch(RenderMarshal.TEXTURE_IMAGE_INFO_SIZEOF) { out ->
        RenderMarshal.writeTextureImageInfoHeader(out)
        // A null destination with zero capacity is the C API's size probe: it fills the metadata
        // and succeeds without copying.
        val status = mln_texture_read_premultiplied_rgba8(handle.raw, 0, 0, out.address)
        val info = RenderMarshal.readTextureImageInfo(out)
        // A backend that answered with a length still answered the question the caller asked, even
        // where it also rejected the empty destination.
        val answered =
          status == MaplibreStatus.OK.nativeCode ||
            (status == MaplibreStatus.INVALID_ARGUMENT.nativeCode && info.byteLength > 0L)
        if (!answered) Status.check(status)
        info
      }
    }
  }

  public actual fun readPremultipliedRgba8(buffer: NativeBuffer): TextureImageInfo {
    activeFrame.ensureInactive("read texture data")
    return live {
      Heap.withScratch(RenderMarshal.TEXTURE_IMAGE_INFO_SIZEOF) { out ->
        RenderMarshal.writeTextureImageInfoHeader(out)
        buffer.borrow { pixels, length ->
          Status.check(
            mln_texture_read_premultiplied_rgba8(
              handle.raw,
              pixels.address,
              length.toInt(),
              out.address,
            )
          )
        }
        val info = RenderMarshal.readTextureImageInfo(out)
        // An empty destination reaches native as the null pointer and zero capacity that mean a
        // size probe, which succeeds without copying, so recheck the capacity here.
        buffer.ensureCapacity(info.byteLength)
        info
      }
    }
  }

  public actual fun acquireMetalOwnedTextureFrame(): MetalOwnedTextureFrameHandle =
    throw unsupportedBackend("Metal")

  public actual fun acquireVulkanOwnedTextureFrame(): VulkanOwnedTextureFrameHandle =
    throw unsupportedBackend("Vulkan")

  /**
   * Takes the session's next frame, giving it back to native if this cannot hand it to the caller.
   *
   * Copying the descriptor into a Kotlin value and wrapping it is object construction, which fails
   * on an exhausted Kotlin heap, and a session holding a frame no handle can release refuses to
   * render, resize, detach, and close for the life of the host. So the release below is made from
   * the descriptor native just filled, before the scratch is freed, rather than from the value that
   * failed to be built; the C API matches a release by the frame's generation and frame id rather
   * than by the address they arrive through.
   */
  public actual fun acquireOpenGLOwnedTextureFrame(): OpenGLOwnedTextureFrameHandle {
    activeFrame.beginAcquire()
    try {
      return live {
        Heap.withScratch(RenderMarshal.OPENGL_OWNED_TEXTURE_FRAME_SIZEOF) { out ->
          RenderMarshal.writeOpenGLFrameHeader(out)
          Status.check(mln_opengl_owned_texture_acquire_frame(handle.raw, out.address))
          try {
            // The seam for the exhaustion this window cannot be put into on request; see
            // InjectedFaults. Armed or real, the recovery below is the same one.
            InjectedFaults.beginFrameWrap(RenderMarshal.OPENGL_OWNED_TEXTURE_FRAME_SIZEOF)
            val scope = FrameScope()
            OpenGLOwnedTextureFrameHandle(this, scope, RenderMarshal.readOpenGLFrame(out, scope))
          } catch (error: Throwable) {
            FrameAcquirePolicy.cleanupAfterWrapperFailure(
              acquired = true,
              releaseNative = {
                Status.check(mln_opengl_owned_texture_release_frame(handle.raw, out.address))
              },
              // The borrow is the outer catch's, which sees this failure too.
              closeLocal = {},
              failure = error,
            )
          }
        }
      }
    } catch (error: Throwable) {
      activeFrame.endBorrow()
      throw error
    }
  }

  public actual override fun close() {
    activeFrame.ensureInactive("destroy")
    core.closeOnce(
      destroy = { mln_render_session_destroy(handle.raw) },
      // Both retentions are idempotent, because a detach released them already. Released after the
      // destroy, because destroying is itself GL work in the context this holds.
      afterSuccess = {
        mapRetention.close()
        contextRetention?.close()
      },
    )
  }

  internal fun releaseOpenGLFrame(frame: OpenGLOwnedTextureFrame) {
    live {
      Heap.withScratch(RenderMarshal.OPENGL_OWNED_TEXTURE_FRAME_SIZEOF) { descriptor ->
        RenderMarshal.writeOpenGLFrame(descriptor, frame)
        // A native release refuses nothing a host can ask for, so BND-169's retry is reachable
        // only through the seam.
        InjectedFaults.beginCall("mln_opengl_owned_texture_release_frame")
        Status.check(mln_opengl_owned_texture_release_frame(handle.raw, descriptor.address))
      }
    }
  }

  internal fun finishFrameBorrow() {
    activeFrame.endBorrow()
  }

  /**
   * Takes one measured block and carves [body]'s descriptors out of it.
   *
   * The block starts zeroed, so an output slot inside it is already the null handle the C API
   * requires callers to pass.
   */
  private fun <T> withArena(bytes: Long, body: (HeapArena) -> T): T {
    Status.requireArgument(bytes in 1..Int.MAX_VALUE.toLong()) {
      "a descriptor block must be positive and addressable on this target"
    }
    val size = bytes.toInt()
    return Heap.withScratch(size) { block -> body(HeapArena(block, size)) }
  }

  /** Copies a query result and destroys it. A result is a snapshot with no owner thread. */
  private fun readFeatureQueryResult(result: Long): List<QueriedFeature> {
    try {
      InjectedFaults.beginResultCopy(result, SIZE_BYTES)
      val count =
        Heap.withScratch(SIZE_BYTES) { out ->
          Status.check(mln_feature_query_result_count(result, out.address))
          Heap.loadInt(out)
        }
      // A count is `size_t`, thirty-two bits here, so a negative one means the handle read is not
      // the result it was taken for.
      if (count < 0) {
        throw Status.invalidState(
          "The MapLibre Native browser module reported a query result count of $count"
        )
      }
      if (count == 0) return emptyList()
      return Heap.withScratch(RenderMarshal.QUERIED_FEATURE_SIZEOF) { out ->
        List(count) { index ->
          // Rewritten every iteration, not only once: the size field is what tells native which
          // fields it may fill, and the block is reused across features.
          RenderMarshal.writeQueriedFeatureHeader(out)
          Status.check(mln_feature_query_result_get(result, index, out.address))
          // Copied before the result is destroyed below: every string and JSON value in it is a
          // view into storage the destroy frees.
          RenderMarshal.readQueriedFeature(out)
        }
      }
    } finally {
      mln_feature_query_result_destroy(result)
    }
  }

  private fun readFeatureExtensionResult(result: Long): FeatureExtensionResult {
    try {
      InjectedFaults.beginResultCopy(result, RenderMarshal.FEATURE_EXTENSION_RESULT_INFO_SIZEOF)
      return Heap.withScratch(RenderMarshal.FEATURE_EXTENSION_RESULT_INFO_SIZEOF) { out ->
        RenderMarshal.writeFeatureExtensionResultInfoHeader(out)
        Status.check(mln_feature_extension_result_get(result, out.address))
        RenderMarshal.readFeatureExtensionResultInfo(out)
      }
    } finally {
      mln_feature_extension_result_destroy(result)
    }
  }

  private fun readJsonSnapshot(snapshot: Long): JsonValue? {
    // The C API reports an absent value as the null snapshot rather than as a failure.
    if (snapshot == 0L) return null
    try {
      InjectedFaults.beginResultCopy(snapshot, POINTER_BYTES)
      return Heap.withScratch(POINTER_BYTES) { out ->
        Status.check(mln_json_snapshot_get(snapshot, out.address))
        RenderMarshal.readJsonPointer(HeapPointer(Heap.loadInt(out)))
      }
    } finally {
      mln_json_snapshot_destroy(snapshot)
    }
  }

  private fun unsupportedBackend(backend: String): UnsupportedFeatureException =
    UnsupportedFeatureException(
      MaplibreStatus.UNSUPPORTED.nativeCode,
      "$backend render targets are not supported by the browser build of MapLibre Native, " +
        "which compiles OpenGL against WebGL",
    )

  internal companion object {
    private const val TYPE_NAME = "RenderSessionHandle"
    private const val BOOL_BYTES = 1
    private const val POINTER_BYTES = 4
    private const val SIZE_BYTES = 4

    fun fromNative(
      map: MapHandle,
      handle: NativeRenderSession,
      contextRetention: HandleStateCore.ChildRetention?,
    ): RenderSessionHandle = RenderSessionHandle(map, handle, contextRetention)
  }
}
