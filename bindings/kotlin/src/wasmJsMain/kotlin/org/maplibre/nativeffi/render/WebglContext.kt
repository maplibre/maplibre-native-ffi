package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_webgl_canvas_create
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_webgl_canvas_destroy
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_webgl_canvas_resize
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_webgl_context_create
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_webgl_context_destroy
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_webgl_present_texture
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_webgl_read_pixels
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_webgl_texture_create
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_webgl_texture_destroy

/** What the release bookkeeping calls this, and what a failure blaming it names. */
private const val TYPE_NAME = "WebglContext"

/**
 * A WebGL context on the thread this binding runs on, and the GL work a host does in it.
 *
 * Every other platform hands a render target a context the host made with its own graphics API, and
 * a browser host cannot make one at all: the handle a [WebglContextDescriptor] carries indexes the
 * Emscripten module's own context table, so a context the page created with
 * `canvas.getContext("webgl2")` names nothing native can look up. A WebGL context also belongs to
 * the agent that created it, and this binding renders on the module's own thread rather than on the
 * page. So on a desktop or a phone the graphics API is EGL, Metal, or Vulkan, which the host
 * genuinely owns; in a browser it is this module, and the context is the binding's to make.
 *
 * WebGL has no share groups, so a texture belongs to the one context it was made in. A host that
 * wants a texture for a caller-owned target, or wants a rendered one on the page, issues those
 * calls here — [createTexture], [presentTexture], [readPixels].
 *
 * The canvas behind a context is either the one page canvas ([createForPageCanvas]) or a private
 * `OffscreenCanvas` that nothing displays ([createOffscreen]). Closing releases the context and,
 * with it, every texture made in it. There is no finalizer behind that: a browser host cannot
 * recover leaked GPU resources by restarting a process. A render target borrows the handle for its
 * whole life, so a context with a target attached refuses to close and names the render session
 * instead.
 */
public class WebglContext
private constructor(
  private val handle: Int,
  private val canvas: String,
  private val page: Boolean,
) : AutoCloseable {
  private val core = HandleStateCore(TYPE_NAME, handle.toLong())

  /**
   * What a descriptor carries so that it names this context rather than only its number.
   *
   * A separate object because [WebglContextOwner] is internal and a public class may not expose an
   * internal supertype.
   */
  private val identity = Identity(this)

  /** Reports whether this context has been released. */
  public val isClosed: Boolean
    get() = core.isReleased()

  /**
   * Returns a descriptor naming this context, for a render target to be attached with.
   *
   * The descriptor names this object and not merely its handle, which is what makes it safe to hold
   * on to. Emscripten frees a context handle when its context is destroyed and gives the number to
   * the next context created, so a descriptor carrying only the number would, once this context is
   * closed, start naming whichever context inherited it.
   */
  public fun descriptor(): WebglContextDescriptor {
    core.requireLive()
    return WebglContextDescriptor(handle, identity)
  }

  /**
   * Sizes the drawing buffer of the canvas this context was created against.
   *
   * A surface target renders into that canvas's default framebuffer, and the framebuffer is only as
   * large as the canvas, so changing such a target's extent means changing both: this, and then
   * [RenderSessionHandle.resize] or [RenderSessionHandle.setOpenGLSurfaceTarget] with the matching
   * extent. Neither implies the other — the session's extent is what MapLibre lays a frame out for,
   * and this is what the frame has room to land in.
   *
   * The context's contents survive: resizing reallocates the drawing buffer and nothing else, so
   * every texture, buffer, and program the session built stays as it was.
   */
  public fun resizeCanvas(width: Int, height: Int) {
    requireExtent(width, height)
    core.requireLive()
    val resized = withCanvasName { name ->
      mln_kotlin_webgl_canvas_resize(name, width, height) != 0
    }
    if (!resized) {
      throw Status.invalidState(
        "The MapLibre Native browser module could not size the canvas \"$canvas\" to ${width}x$height."
      )
    }
  }

  /**
   * Creates an RGBA8 texture in this context, for a caller-owned render target to draw into.
   *
   * The texture belongs to this context and to no other, so it is named in a descriptor that names
   * this context. It is the host's: nothing tracks it, a render target only borrows it, and
   * [destroyTexture] is what releases it — before this context is closed, or with it, since closing
   * a context releases everything made in it.
   */
  public fun createTexture(width: Int, height: Int): Int {
    requireExtent(width, height)
    core.requireLive()
    val texture = mln_kotlin_webgl_texture_create(handle, width, height)
    if (texture == 0) {
      throw Status.invalidState(
        "The MapLibre Native browser module could not create a ${width}x$height texture."
      )
    }
    return texture
  }

  /** Releases a texture from [createTexture], once no target borrows it any more. */
  public fun destroyTexture(texture: Int) {
    core.requireLive()
    mln_kotlin_webgl_texture_destroy(handle, texture)
  }

  /**
   * Puts a texture this context owns onto the canvas this context draws to.
   *
   * A texture target renders into a framebuffer of its own, so something has to move those pixels
   * onto the canvas's default framebuffer. Native blits them there, which keeps them in GPU memory:
   * they are never read back, never enter the module's heap, and never cross into JavaScript. A
   * surface target needs none of this, because it already renders into that framebuffer.
   *
   * [texture] is a name from [createTexture] for a caller-owned target, or
   * [OpenGLOwnedTextureFrame.texture] for a session-owned one, and [width] and [height] are its
   * size in device pixels.
   *
   * The frame becomes visible on the next turn of this thread's event loop rather than as this
   * returns: a browser composites a canvas when the task that drew into it ends.
   */
  public fun presentTexture(texture: Int, width: Int, height: Int) {
    Status.requireArgument(texture != 0) { "texture must name a texture" }
    requireExtent(width, height)
    core.requireLive()
    if (mln_kotlin_webgl_present_texture(handle, texture, width, height) == 0) {
      throw Status.invalidState(
        "The MapLibre Native browser module could not present texture $texture at ${width}x$height."
      )
    }
  }

  /**
   * Reads a rendered frame out of this context, as RGBA8 with row zero at the bottom.
   *
   * [texture] names a texture of this context, or is zero for the default framebuffer of the canvas
   * the context is bound to — which is what a surface target renders into and what [presentTexture]
   * blits onto.
   *
   * This is the expensive way to use a frame: it stalls this thread until the GPU is done, and it
   * copies every pixel through the module's heap. A host that only wants the frame seen presents it
   * instead. Reading back is for a host that consumes the pixels itself — encoding an image,
   * comparing two frames — and for telling a frame that was never drawn from a frame that was drawn
   * and never composited, which are the same symptom from the page's side and completely different
   * underneath.
   */
  public fun readPixels(texture: Int, width: Int, height: Int): ByteArray {
    requireExtent(width, height)
    core.requireLive()
    // Multiplied in Long rather than as an Int product of two caller-supplied extents, whose
    // product for a 20000-by-20000 read wraps to a small allocation that native is then handed the
    // real extents for.
    val pixelCount = width.toLong() * height.toLong()
    Status.requireArgument(pixelCount <= MAX_READBACK_PIXELS) {
      "a ${width}x$height frame is $pixelCount pixels, and a readback can address at most " +
        "$MAX_READBACK_PIXELS on this target"
    }
    val bytes = (pixelCount * BYTES_PER_PIXEL).toInt()
    return Heap.withScratch(bytes) { pixels ->
      if (
        mln_kotlin_webgl_read_pixels(handle, texture, width, height, pixels.address, bytes) == 0
      ) {
        throw Status.invalidState(
          "The MapLibre Native browser module could not read a ${width}x$height frame."
        )
      }
      Heap.loadBytes(pixels, bytes)
    }
  }

  override fun close() {
    core.closeOnce(
      // Destroying a context reports nothing: a handle that names no context on this thread is
      // already the state a close is asking for.
      destroy = {
        mln_kotlin_webgl_context_destroy(handle)
        MaplibreStatus.OK.nativeCode
      },
      afterSuccess = {
        if (page) {
          // The page still displays the element, and a canvas reaches this thread only as it is
          // created, so the registration outlives every context made against it.
          pageCanvas = null
        } else {
          withName(canvas, ::mln_kotlin_webgl_canvas_destroy)
        }
      },
    )
  }

  private fun <T> withCanvasName(body: (Int) -> T): T = withName(canvas, body)

  private class Identity(val context: WebglContext) : WebglContextOwner

  public companion object {
    /**
     * Creates a WebGL2 context against the canvas the page displays.
     *
     * There is one such canvas. A page transfers it to this thread as the module is instantiated,
     * by passing an `OffscreenCanvas` as the module option `mlnPageCanvas`, and a canvas can be
     * transferred to a thread only as that thread is created — so the number of on-screen maps is
     * fixed before any Kotlin runs. A host that transferred nothing gets a placeholder canvas that
     * nothing displays, and a second live context for the page canvas is refused rather than
     * silently drawing where the first one draws.
     *
     * [width] and [height] size the canvas's drawing buffer in device pixels, which for a surface
     * target is that target's physical extent and for a texture target is the size anything
     * [presentTexture] shows must fit.
     */
    public fun createForPageCanvas(width: Int, height: Int): WebglContext {
      requireExtent(width, height)
      pageCanvas?.let {
        throw Status.invalidState(
          "The page canvas already has a WebGL context. This build supports one on-screen canvas, " +
            "transferred to the render thread as the module was instantiated, so a second " +
            "on-screen map is not something the binding can create. Close the first context, or " +
            "render the second map to a texture."
        )
      }
      val context = create(PAGE_CANVAS, width, height, page = true)
      pageCanvas = context
      return context
    }

    /**
     * Creates a WebGL2 context against a private `OffscreenCanvas` on this thread.
     *
     * Nothing displays that canvas, which is what a host that reads frames back wants, and there is
     * no limit on how many there are. A texture target renders into a framebuffer of its own rather
     * than into the canvas, so for one the size bounds nothing the map draws and only has to be
     * positive, because a zero-sized canvas has no drawing buffer to create a context against.
     */
    public fun createOffscreen(width: Int, height: Int): WebglContext {
      requireExtent(width, height)
      val name = "mln-offscreen-${offscreenCanvases++}"
      if (
        withName(name) { address -> mln_kotlin_webgl_canvas_create(address, width, height) } == 0
      ) {
        throw Status.invalidState(
          "The MapLibre Native browser module could not create a ${width}x$height offscreen canvas."
        )
      }
      try {
        return create(name, width, height, page = false)
      } catch (error: Throwable) {
        // The canvas outlives a context that could not be created against it, and nothing else
        // holds a name this method invented.
        withName(name, ::mln_kotlin_webgl_canvas_destroy)
        throw error
      }
    }

    /**
     * Retains the context a render target is about to borrow, for as long as that target lives.
     *
     * A render target names its context by handle for its whole life, and the backend makes that
     * handle current on every frame and again while it tears its GL objects down. Destroying the
     * context underneath it would leave a live target naming a context that is gone, so the target
     * holds the context open the way a render session holds its map open, and closing the context
     * first reports the live child instead.
     *
     * Returns null for the WGL and EGL arms, which name a context from a graphics API this module
     * was not built against. Nothing here can retain one, and native is where a build's capability
     * is known, so those are passed down and refused there.
     */
    internal fun retainForTarget(
      context: OpenGLContextDescriptor
    ): HandleStateCore.ChildRetention? {
      val owner = requireOpenForTarget(context) ?: return null
      return owner.core.retainChild("RenderSessionHandle")
    }

    /**
     * Reports that [context] still names an open context, and returns the one it names.
     *
     * The context comes off the descriptor rather than out of a lookup by handle, and that is what
     * makes a stale descriptor safe. Emscripten's handles are allocated and freed, so the number in
     * a descriptor from a closed context is one a later context can be given; a lookup by number
     * would find that later context and hand a render target a context the host never named.
     *
     * Asked at every entry point that hands native a context descriptor, not only at attach. Native
     * compares a WebGL descriptor by its handle alone — `opengl_context_matches` in
     * `src/render/render_session_common.cpp` has nothing else to compare — so a retarget whose
     * descriptor came from a closed context whose number has since come back would be accepted
     * there, and the texture beside it would belong to whatever inherited the number.
     *
     * Returns null for the WGL and EGL arms, which this build was not compiled against and which
     * carry no object to resolve; native is where a missing provider is refused.
     */
    internal fun requireOpenForTarget(context: OpenGLContextDescriptor): WebglContext? {
      if (context !is WebglContextDescriptor) return null
      // Total, because the descriptor's constructor is internal and descriptor() is its one caller.
      val owner = (context.owner as Identity).context
      if (owner.isClosed) {
        throw Status.invalidArgument(
          "A WebGL context descriptor names a WebglContext that has been closed. A render target's " +
            "context comes from WebglContext.descriptor() and stays open until every target that " +
            "borrowed it is detached or closed; the handle ${context.context} it carries may " +
            "since have been reused by another context, which is why this is refused rather than " +
            "resolved."
        )
      }
      return owner
    }

    private fun create(name: String, width: Int, height: Int, page: Boolean): WebglContext {
      val handle =
        withName(name) { address -> mln_kotlin_webgl_context_create(address, width, height) }
      // Zero is what the module reports for a context it could not create, and it is also the value
      // the C API refuses in a descriptor, so it becomes a failure here rather than a handle that
      // fails at attach.
      if (handle == 0) {
        throw Status.invalidState(
          "The MapLibre Native browser module could not create a ${width}x$height WebGL2 context " +
            "for the canvas \"$name\". The browser may have no WebGL2 support, or too many " +
            "contexts may be live."
        )
      }
      return WebglContext(handle, name, page)
    }

    /** Runs [body] with [name] staged in the module's heap, which is how the shim takes one. */
    private fun <T> withName(name: String, body: (Int) -> T): T =
      Heap.withScratch(Heap.utf8Size(name)) { address ->
        Heap.storeUtf8(address, name)
        body(address.address)
      }

    private fun requireExtent(width: Int, height: Int) {
      Status.requireArgument(width > 0) { "width must be positive, but was $width" }
      Status.requireArgument(height > 0) { "height must be positive, but was $height" }
    }

    /**
     * The context holding the page canvas, so a second one is refused rather than made.
     *
     * A plain field because this binding runs on one thread: the module's `main()` imported Kotlin
     * into it, and nothing here is reachable from another agent.
     */
    private var pageCanvas: WebglContext? = null

    /** Names offscreen canvases apart. Never reused, so a stale name cannot find a live canvas. */
    private var offscreenCanvases = 0

    /**
     * The registry key the page canvas is transferred under.
     *
     * Fixed at link time by `-sOFFSCREENCANVASES_TO_PTHREAD` in
     * `cmake/mln_ffi_browser_module.cmake` and registered by
     * `bindings/kotlin/emscripten/mln_kotlin_pre.js`, which is why a host names no canvas here.
     */
    private const val PAGE_CANVAS = "maplibre"

    /** RGBA8, which is what a readback produces and what native writes. */
    private const val BYTES_PER_PIXEL = 4

    /** The largest frame a readback can stage, which is what a 32-bit pointer leaves room for. */
    private const val MAX_READBACK_PIXELS = Int.MAX_VALUE / BYTES_PER_PIXEL
  }
}
