package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeRenderSession
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.Dispatcher
import org.maplibre.nativeffi.internal.wasm.GeoJsonMarshal
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.HeapArena
import org.maplibre.nativeffi.internal.wasm.HeapPointer
import org.maplibre.nativeffi.internal.wasm.JsonMarshal
import org.maplibre.nativeffi.internal.wasm.NativeCall
import org.maplibre.nativeffi.internal.wasm.RenderMarshal
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.query.FeatureExtensionResult
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

/**
 * A render session, owned by the thread the module runs its maps on.
 *
 * Session work is placed on that thread rather than run on the page: the C API reports an
 * owner-thread status for a session call from anywhere else, and MapLibre blocks, which a browser
 * page may not. The dispatcher is what lets this keep the ordinary synchronous shape the other
 * platforms have.
 *
 * Not every call here is owner-affine. A query hands back a result handle whose contents are a
 * plain snapshot, and the C API places no thread rule on reading or destroying one, so those calls
 * run on the page. That matters more here than it would elsewhere: each dispatched call is an
 * event-loop round trip, and a query that dispatched its count, every getter, and its destroy would
 * cost one per feature.
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
  // Held so the map reports a live session rather than letting native refuse the destroy. Passing
  // the map to HandleStateCore only names the parent; this is what makes closing the map while a
  // session is attached the binding's own INVALID_STATE, which is what the common KDoc promises.
  private val mapRetention = map.retainChild("RenderSessionHandle")
  private val core = HandleStateCore("RenderSessionHandle", handle.raw, map)
  private val activeFrame = ActiveFrameState()

  /**
   * Checks this handle is live and then runs [body], without holding a use count across it.
   *
   * `withLive` would hold one, and a dispatched call parks the Kotlin stack while the owner thread
   * works. A close arriving during that park would drain a count that cannot be released until the
   * park ends, which is the invariant `yieldWhileClosing` refuses to spin on. The window this
   * leaves is the one the C API already closes: a handle destroyed between the check and the call
   * is a stale handle, and native reports invalid argument for it.
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
    live {
      Dispatcher.call(
        "mln_render_session_resize",
        4,
        { slots ->
          slots.setLong(0, handle.raw)
          slots.setInt(1, width)
          slots.setInt(2, height)
          slots.setDouble(3, scaleFactor)
        },
        { Status.check(Heap.loadInt(it)) },
      )
    }
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
   * bound to, and `descriptor.surface` must be [NativePointer.NULL] as it was at attach. Resizing
   * the canvas's drawing buffer is the host's to do, on the thread that owns the context, and is
   * not something this binding can reach.
   */
  public actual fun setOpenGLSurfaceTarget(descriptor: OpenGLSurfaceDescriptor) {
    activeFrame.ensureInactive("set target")
    live {
      Heap.withScratch(RenderMarshal.OPENGL_SURFACE_SIZEOF) { block ->
        RenderMarshal.writeOpenGLSurface(block, descriptor)
        callWithDescriptor("mln_opengl_surface_set_target", block)
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
    live {
      Heap.withScratch(RenderMarshal.OPENGL_BORROWED_TEXTURE_SIZEOF) { block ->
        RenderMarshal.writeOpenGLBorrowedTexture(block, descriptor)
        callWithDescriptor("mln_opengl_borrowed_texture_set_target", block)
      }
    }
  }

  public actual fun renderUpdate(): Boolean {
    activeFrame.ensureInactive("render")
    return live {
      Heap.withScratch(BOOL_BYTES) { out ->
        Dispatcher.call(
          "mln_render_session_render_update",
          2,
          { slots ->
            slots.setLong(0, handle.raw)
            slots.setPointer(1, out)
          },
          { Status.check(Heap.loadInt(it)) },
        )
        Heap.loadByte(out) != 0.toByte()
      }
    }
  }

  public actual fun detach() {
    activeFrame.ensureInactive("detach")
    live { callWithSession("mln_render_session_detach") }
    // A detached session no longer holds the map, so the map becomes closeable again even though
    // this handle is still live and can be attached to a new target.
    mapRetention.close()
    // The backend released its GL objects during the detach, so nothing here names the WebGL
    // context any more and the host may close it.
    contextRetention?.close()
  }

  public actual fun reduceMemoryUse() {
    activeFrame.ensureInactive("reduce memory use")
    live { callWithSession("mln_render_session_reduce_memory_use") }
  }

  public actual fun clearData() {
    activeFrame.ensureInactive("clear data")
    live { callWithSession("mln_render_session_clear_data") }
  }

  public actual fun dumpDebugLogs() {
    activeFrame.ensureInactive("dump debug logs")
    live { callWithSession("mln_render_session_dump_debug_logs") }
  }

  public actual fun setFeatureState(selector: FeatureStateSelector, value: JsonValue) {
    activeFrame.ensureInactive("set feature state")
    live {
      // The selector and the state share one block, so this costs one scratch acquisition rather
      // than two, and both are measured before either is written.
      val bytes =
        RenderMarshal.measureFeatureStateSelector(selector) + JsonMarshal.measureValue(value, 0)
      withArena(bytes) { arena ->
        val selectorBlock = RenderMarshal.writeFeatureStateSelector(arena, selector)
        val stateBlock = JsonMarshal.write(arena, value)
        Dispatcher.call(
          "mln_render_session_set_feature_state",
          3,
          { slots ->
            slots.setLong(0, handle.raw)
            slots.setPointer(1, selectorBlock)
            slots.setPointer(2, stateBlock)
          },
          { Status.check(Heap.loadInt(it)) },
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
        Dispatcher.call(
          "mln_render_session_get_feature_state",
          3,
          { slots ->
            slots.setLong(0, handle.raw)
            slots.setPointer(1, selectorBlock)
            slots.setPointer(2, out)
          },
          { Status.check(Heap.loadInt(it)) },
        )
        Heap.loadLong(out)
      }
    }
    // A snapshot is a copied tree with no owner thread, so borrowing its root and destroying it run
    // on the page rather than costing two more event-loop round trips.
    return readJsonSnapshot(snapshot) ?: JsonValue.ObjectValue(emptyList())
  }

  public actual fun removeFeatureState(selector: FeatureStateSelector) {
    activeFrame.ensureInactive("remove feature state")
    live {
      withArena(RenderMarshal.measureFeatureStateSelector(selector)) { arena ->
        callWithDescriptor(
          "mln_render_session_remove_feature_state",
          RenderMarshal.writeFeatureStateSelector(arena, selector),
        )
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
        Dispatcher.call(
          "mln_render_session_query_rendered_features",
          4,
          { slots ->
            slots.setLong(0, handle.raw)
            slots.setPointer(1, geometryBlock)
            slots.setPointer(2, optionsBlock)
            slots.setPointer(3, out)
          },
          { Status.check(Heap.loadInt(it)) },
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
        val sourceBlock = RenderMarshal.writeStringViewRoot(arena, sourceId)
        val optionsBlock = RenderMarshal.writeSourceFeatureQueryOptions(arena, options)
        val out = arena.allocate(RenderMarshal.OUT_SLOT_BYTES, RenderMarshal.OUT_SLOT_BYTES)
        Dispatcher.call(
          "mln_render_session_query_source_features",
          4,
          { slots ->
            slots.setLong(0, handle.raw)
            // A source ID is a string view by value in C, which this target passes indirectly,
            // so the argument is a pointer to the view rather than to its bytes.
            slots.setPointer(1, sourceBlock)
            slots.setPointer(2, optionsBlock)
            slots.setPointer(3, out)
          },
          { Status.check(Heap.loadInt(it)) },
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
        Dispatcher.call(
          "mln_render_session_query_feature_extensions",
          7,
          { slots ->
            slots.setLong(0, handle.raw)
            slots.setPointer(1, sourceBlock)
            slots.setPointer(2, featureBlock)
            slots.setPointer(3, extensionBlock)
            slots.setPointer(4, fieldBlock)
            slots.setPointer(5, argumentBlock)
            slots.setPointer(6, out)
          },
          { Status.check(Heap.loadInt(it)) },
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
        val status =
          Dispatcher.call(
            "mln_texture_read_premultiplied_rgba8",
            4,
            { slots ->
              slots.setLong(0, handle.raw)
              slots.setPointer(1, HeapPointer(0))
              slots.setInt(2, 0)
              slots.setPointer(3, out)
            },
            { Heap.loadInt(it) },
          )
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
        // The buffer already lives in the module's heap, so native writes the pixels straight into
        // it and nothing here copies a frame through Kotlin.
        buffer.borrow { pixels, length ->
          Dispatcher.call(
            "mln_texture_read_premultiplied_rgba8",
            4,
            { slots ->
              slots.setLong(0, handle.raw)
              slots.setPointer(1, pixels)
              slots.setInt(2, length.toInt())
              slots.setPointer(3, out)
            },
            { Status.check(Heap.loadInt(it)) },
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

  public actual fun acquireOpenGLOwnedTextureFrame(): OpenGLOwnedTextureFrameHandle {
    activeFrame.beginAcquire()
    try {
      val scope = FrameScope()
      val frame = live {
        Heap.withScratch(RenderMarshal.OPENGL_OWNED_TEXTURE_FRAME_SIZEOF) { out ->
          RenderMarshal.writeOpenGLFrameHeader(out)
          Dispatcher.call(
            "mln_opengl_owned_texture_acquire_frame",
            2,
            { slots ->
              slots.setLong(0, handle.raw)
              slots.setPointer(1, out)
            },
            { Status.check(Heap.loadInt(it)) },
          )
          // Copied out here rather than kept: the descriptor lives in scratch this call frees,
          // and release matches a frame by generation and frame id rather than by its address.
          RenderMarshal.readOpenGLFrame(out, scope)
        }
      }
      // Nothing between the acquire and this line can fail, so a native frame is never left
      // acquired with no handle to release it.
      return OpenGLOwnedTextureFrameHandle(this, scope, frame)
    } catch (error: Throwable) {
      activeFrame.endBorrow()
      throw error
    }
  }

  public actual override fun close() {
    activeFrame.ensureInactive("destroy")
    core.closeOnce(
      destroy = {
        Dispatcher.call(
          "mln_render_session_destroy",
          1,
          { slots -> slots.setLong(0, handle.raw) },
          { Heap.loadInt(it) },
        )
      },
      // Closing twice is what a detach followed by a close would otherwise do; both retentions are
      // idempotent for that reason, so this needs no flag of its own. Released only after native
      // has destroyed the session, because destroying is itself GL work in the context this holds.
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
        callWithDescriptor("mln_opengl_owned_texture_release_frame", descriptor)
      }
    }
  }

  internal fun finishFrameBorrow() {
    activeFrame.endBorrow()
  }

  /** One session argument, which is the shape most of the maintenance calls take. */
  private fun callWithSession(name: String) {
    Dispatcher.call(
      name,
      1,
      { slots -> slots.setLong(0, handle.raw) },
      { Status.check(Heap.loadInt(it)) },
    )
  }

  /** One session argument and one descriptor. */
  private fun callWithDescriptor(name: String, descriptor: HeapPointer) {
    Dispatcher.call(
      name,
      2,
      { slots ->
        slots.setLong(0, handle.raw)
        slots.setPointer(1, descriptor)
      },
      { Status.check(Heap.loadInt(it)) },
    )
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

  /**
   * Copies a query result and destroys it, without dispatching any of it.
   *
   * A result is a snapshot the C API places no thread rule on, so the count, every getter, and the
   * destroy run on the page. Dispatching them would cost an event-loop round trip per feature, and
   * the C API has no composite entry point that would collapse them into one.
   */
  private fun readFeatureQueryResult(result: Long): List<QueriedFeature> {
    try {
      val count =
        Heap.withScratch(SIZE_BYTES) { out ->
          callResult("mln_feature_query_result_count", result, out)
          Heap.loadInt(out)
        }
      // A count is `size_t`, which is thirty-two bits here, so one past Int.MAX_VALUE arrives
      // negative. No result that large could exist, so a negative one means the handle read is not
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
          NativeCall.call(
            "mln_feature_query_result_get",
            3,
            { slots ->
              slots.setLong(0, result)
              slots.setInt(1, index)
              slots.setPointer(2, out)
            },
            { Status.check(Heap.loadInt(it)) },
          )
          // Copied before the result is destroyed below: every string and JSON value in it is a
          // view into storage the destroy frees.
          RenderMarshal.readQueriedFeature(out)
        }
      }
    } finally {
      destroyResult("mln_feature_query_result_destroy", result)
    }
  }

  private fun readFeatureExtensionResult(result: Long): FeatureExtensionResult {
    try {
      return Heap.withScratch(RenderMarshal.FEATURE_EXTENSION_RESULT_INFO_SIZEOF) { out ->
        RenderMarshal.writeFeatureExtensionResultInfoHeader(out)
        callResult("mln_feature_extension_result_get", result, out)
        RenderMarshal.readFeatureExtensionResultInfo(out)
      }
    } finally {
      destroyResult("mln_feature_extension_result_destroy", result)
    }
  }

  private fun readJsonSnapshot(snapshot: Long): JsonValue? {
    // The C API reports an absent value as the null snapshot rather than as a failure.
    if (snapshot == 0L) return null
    try {
      return Heap.withScratch(POINTER_BYTES) { out ->
        callResult("mln_json_snapshot_get", snapshot, out)
        RenderMarshal.readJsonPointer(HeapPointer(Heap.loadInt(out)))
      }
    } finally {
      destroyResult("mln_json_snapshot_destroy", snapshot)
    }
  }

  /** One result handle and one output pointer, run on the page rather than dispatched. */
  private fun callResult(name: String, result: Long, out: HeapPointer) {
    NativeCall.call(
      name,
      2,
      { slots ->
        slots.setLong(0, result)
        slots.setPointer(1, out)
      },
      { Status.check(Heap.loadInt(it)) },
    )
  }

  /** Destroys a result handle. These return nothing, so there is no status to check. */
  private fun destroyResult(name: String, result: Long) {
    NativeCall.call(name, 1, { slots -> slots.setLong(0, result) }, {})
  }

  private fun unsupportedBackend(backend: String): UnsupportedFeatureException =
    UnsupportedFeatureException(
      MaplibreStatus.UNSUPPORTED.nativeCode,
      "$backend render targets are not supported by the browser build of MapLibre Native, " +
        "which compiles OpenGL against WebGL",
    )

  internal companion object {
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
