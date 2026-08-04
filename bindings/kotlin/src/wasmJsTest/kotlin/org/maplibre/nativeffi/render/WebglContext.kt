package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.lifecycle.BorrowedResourceCore
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.Dispatcher
import org.maplibre.nativeffi.internal.wasm.Heap

@JsFun(
  "(d, canvas, width, height, out, token) => " +
    "globalThis.__maplibreNativeC._mln_browser_webgl_context_create(" +
    "d, canvas, width, height, out, token)"
)
private external fun submitCreate(
  dispatcher: Int,
  canvasId: Int,
  width: Int,
  height: Int,
  out: Int,
  token: Int,
): Boolean

@JsFun(
  "(d, context, token) => " +
    "globalThis.__maplibreNativeC._mln_browser_webgl_context_destroy(d, context, token)"
)
private external fun submitDestroy(dispatcher: Int, context: Int, token: Int): Boolean

@JsFun(
  "(d, context, width, height, out, token) => " +
    "globalThis.__maplibreNativeC._mln_browser_webgl_canvas_resize(" +
    "d, context, width, height, out, token)"
)
private external fun submitCanvasResize(
  dispatcher: Int,
  context: Int,
  width: Int,
  height: Int,
  out: Int,
  token: Int,
): Boolean

@JsFun(
  "(d, context, texture, width, height, out, token) => " +
    "globalThis.__maplibreNativeC._mln_browser_webgl_present_texture(" +
    "d, context, texture, width, height, out, token)"
)
private external fun submitPresent(
  dispatcher: Int,
  context: Int,
  texture: Int,
  width: Int,
  height: Int,
  out: Int,
  token: Int,
): Boolean

@JsFun(
  "(d, context, width, height, out, token) => " +
    "globalThis.__maplibreNativeC._mln_browser_webgl_texture_create(" +
    "d, context, width, height, out, token)"
)
private external fun submitTextureCreate(
  dispatcher: Int,
  context: Int,
  width: Int,
  height: Int,
  out: Int,
  token: Int,
): Boolean

@JsFun(
  "(d, context, texture, token) => " +
    "globalThis.__maplibreNativeC._mln_browser_webgl_texture_destroy(d, context, texture, token)"
)
private external fun submitTextureDestroy(
  dispatcher: Int,
  context: Int,
  texture: Int,
  token: Int,
): Boolean

@JsFun(
  "(d, context, texture, width, height, pixels, capacity, out, token) => " +
    "globalThis.__maplibreNativeC._mln_browser_webgl_read_pixels(" +
    "d, context, texture, width, height, pixels, capacity, out, token)"
)
private external fun submitReadPixels(
  dispatcher: Int,
  context: Int,
  texture: Int,
  width: Int,
  height: Int,
  pixels: Int,
  capacity: Int,
  out: Int,
  token: Int,
): Boolean

/**
 * A WebGL context on the thread this binding's maps render on, and the GL work a host does there.
 *
 * Every other platform hands a render target a context the host made with its own platform API, and
 * a browser host cannot: the handle a [WebglContextDescriptor] carries is an index into the
 * Emscripten module's own table, so a context the page created with `canvas.getContext("webgl2")`
 * is not one native can look up. This is where a host gets one that native can.
 *
 * A WebGL context belongs to the thread that created it, and the thread that renders is the one the
 * module owns, so the context is created there — and so is everything else here. WebGL has no share
 * groups, so a texture belongs to the one context it was made in; a host that wants to create a
 * texture for a caller-owned target, or to put a rendered one on the page, issues those GL calls in
 * this context, on that thread. That is what [createTexture] and [presentTexture] are.
 *
 * This lives in the test source set rather than in the binding, because creating a context is the
 * host's job. A host owns its canvases and decides which of them a map draws onto; a binding that
 * created its own could never share one with the host's other rendering.
 *
 * The canvas behind the context is either a page canvas reserved with [reserveCanvas] — whose
 * `<canvas>` element keeps displaying whatever this draws, with no copy — or a private
 * `OffscreenCanvas` on the owner thread, which nothing displays and which a host reads back from.
 *
 * Closing is what releases the context, and there is no finalizer behind it: a browser host cannot
 * recover leaked resources by restarting a process. Close it only after every render target that
 * borrowed it has been detached or destroyed, because the C API borrows the handle for a target's
 * lifetime.
 */
internal class WebglContext private constructor(private val handle: Int) : AutoCloseable {
  private val core =
    BorrowedResourceCore("WebglContext") {
      Dispatcher.submitTask("mln_browser_webgl_context_destroy") { dispatcher, token ->
        submitDestroy(dispatcher, handle, token)
      }
    }

  /**
   * Returns a descriptor naming this context, for a render target to be attached with.
   *
   * A fresh descriptor each call, because a descriptor is mutable: a shared one that a caller
   * changed would point every later target at whatever it was changed to.
   */
  fun descriptor(): WebglContextDescriptor = core.withOpenResource {
    WebglContextDescriptor(handle)
  }

  /**
   * Reports that this context is still open, without holding a borrow while native works.
   *
   * Every call below parks its stack on the owner thread, and a borrow held across that park would
   * be a borrow held for as long as native takes. Closing is the caller's own next statement, on
   * the same single-threaded page, so the check is what is wanted here and the borrow is not.
   */
  private fun requireOpen() {
    core.withOpenResource {}
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
   * The context's contents survive. Resizing a canvas reallocates its drawing buffer and nothing
   * else, so every texture, buffer, and program the session built stays as it was.
   */
  fun resizeCanvas(width: Int, height: Int) {
    Status.requireArgument(width > 0) { "width must be positive, but was $width" }
    Status.requireArgument(height > 0) { "height must be positive, but was $height" }
    requireOpen()
    val resized =
      Heap.withScratch(FLAG_BYTES) { out ->
        Dispatcher.submitTask("mln_browser_webgl_canvas_resize") { dispatcher, token ->
          submitCanvasResize(dispatcher, handle, width, height, out.address, token)
        }
        Heap.loadInt(out) != 0
      }
    if (!resized) {
      throw Status.invalidState(
        "The MapLibre Native browser module could not size this context's canvas to " +
          "${width}x$height."
      )
    }
  }

  /**
   * Puts a texture this context owns onto the canvas the page displays.
   *
   * A texture target renders into a framebuffer of its own, so something has to move those pixels
   * onto the canvas's default framebuffer, and it has to happen in this context on the thread that
   * owns it. Native blits them there, which keeps them in GPU memory: they are never read back,
   * never enter the module's heap, and never cross into JavaScript.
   *
   * [texture] is a name from [createTexture] for a caller-owned target, or
   * [OpenGLOwnedTextureFrame.texture] for a session-owned one, and [width] and [height] are its
   * size in device pixels.
   *
   * The frame becomes visible on the next turn of the page's event loop rather than as this
   * returns: a browser composites a canvas when the task that drew into it ends, and that task is
   * the owner thread's, not the page's.
   */
  fun presentTexture(texture: Int, width: Int, height: Int) {
    Status.requireArgument(texture != 0) { "texture must name a texture" }
    Status.requireArgument(width > 0) { "width must be positive, but was $width" }
    Status.requireArgument(height > 0) { "height must be positive, but was $height" }
    requireOpen()
    val presented =
      Heap.withScratch(FLAG_BYTES) { out ->
        Dispatcher.submitTask("mln_browser_webgl_present_texture") { dispatcher, token ->
          submitPresent(dispatcher, handle, texture, width, height, out.address, token)
        }
        Heap.loadInt(out) != 0
      }
    if (!presented) {
      throw Status.invalidState(
        "The MapLibre Native browser module could not present texture $texture at " +
          "${width}x$height."
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
  fun createTexture(width: Int, height: Int): Int {
    Status.requireArgument(width > 0) { "width must be positive, but was $width" }
    Status.requireArgument(height > 0) { "height must be positive, but was $height" }
    requireOpen()
    val texture =
      Heap.withScratch(FLAG_BYTES) { out ->
        Dispatcher.submitTask("mln_browser_webgl_texture_create") { dispatcher, token ->
          submitTextureCreate(dispatcher, handle, width, height, out.address, token)
        }
        Heap.loadInt(out)
      }
    if (texture == 0) {
      throw Status.invalidState(
        "The MapLibre Native browser module could not create a ${width}x$height texture."
      )
    }
    return texture
  }

  /**
   * Reads a rendered frame out of this context, as RGBA8 with row zero at the bottom.
   *
   * [texture] names a texture of this context, or is zero for the default framebuffer of the canvas
   * the context is bound to — which is what a surface target renders into and what [presentTexture]
   * blits onto.
   *
   * This is the expensive way to use a frame: it stalls the owner thread until the GPU is done, and
   * it copies every pixel through the module's heap. It exists so that a test can tell a frame that
   * was never drawn from a frame that was drawn and never composited, which are the same failure
   * from the page's side and completely different underneath.
   */
  fun readPixels(texture: Int, width: Int, height: Int): ByteArray {
    Status.requireArgument(width > 0) { "width must be positive, but was $width" }
    Status.requireArgument(height > 0) { "height must be positive, but was $height" }
    requireOpen()
    val bytes = width * height * 4
    // The flag first, because it is the only member here that has an alignment to satisfy.
    return Heap.withScratch(FLAG_BYTES + bytes) { out ->
      val pixels = out + FLAG_BYTES
      Dispatcher.submitTask("mln_browser_webgl_read_pixels") { dispatcher, token ->
        submitReadPixels(
          dispatcher,
          handle,
          texture,
          width,
          height,
          pixels.address,
          bytes,
          out.address,
          token,
        )
      }
      if (Heap.loadInt(out) == 0) {
        throw Status.invalidState(
          "The MapLibre Native browser module could not read a ${width}x$height frame."
        )
      }
      Heap.loadBytes(pixels, bytes)
    }
  }

  /** Releases a texture from [createTexture], once no target borrows it any more. */
  fun destroyTexture(texture: Int) {
    requireOpen()
    Dispatcher.submitTask("mln_browser_webgl_texture_destroy") { dispatcher, token ->
      submitTextureDestroy(dispatcher, handle, texture, token)
    }
  }

  override fun close(): Unit = core.close()

  companion object {
    /**
     * Claims the `<canvas>` element with this `id` for the thread maps render on.
     *
     * The element must already be in the document, and this must run before the first call that
     * reaches native, because a browser hands a canvas to a thread only as that thread is created
     * and this binding creates its thread on first use. Afterwards the element is a placeholder
     * that displays what the owner thread draws, and page code can no longer draw into it.
     *
     * This is what makes presentation zero-copy: [createForCanvas] builds a context whose default
     * framebuffer is that canvas, so a surface target's frame — or a texture [presentTexture] blits
     * onto it — appears on the page without being copied anywhere.
     */
    fun reserveCanvas(id: String) {
      Dispatcher.reserveCanvas(id)
    }

    /**
     * Creates a WebGL2 context against a canvas reserved with [reserveCanvas].
     *
     * [width] and [height] size that canvas's drawing buffer in device pixels, which for a surface
     * target is the target's physical extent and for a texture target is the size anything
     * [presentTexture] shows must fit.
     *
     * Call this inside a `maplibreScope`, like every other call that reaches the owner thread.
     */
    fun createForCanvas(id: String, width: Int, height: Int): WebglContext {
      Status.requireArgument(id.isNotEmpty()) { "a canvas id must not be empty" }
      return create(id, width, height)
    }

    /**
     * Creates a WebGL2 context against a private `OffscreenCanvas` the owner thread makes.
     *
     * Nothing displays that canvas, which is what a test or a host that reads frames back wants. A
     * texture target renders into a framebuffer of its own rather than into it, so for one the size
     * bounds nothing the map draws and only has to be positive, because a zero-sized canvas has no
     * drawing buffer to create a context against.
     */
    fun create(width: Int, height: Int): WebglContext = create("", width, height)

    private fun create(id: String, width: Int, height: Int): WebglContext {
      Status.requireArgument(width > 0) { "width must be positive, but was $width" }
      Status.requireArgument(height > 0) { "height must be positive, but was $height" }
      Heap.requireCString(id, "canvas id")
      // One block for both, with the four-byte output first so it is the aligned member; the id is
      // bytes and has no alignment of its own.
      val handle =
        Heap.withScratch(FLAG_BYTES + Heap.utf8Size(id)) { out ->
          val canvas = out + FLAG_BYTES
          Heap.storeUtf8(canvas, id)
          Dispatcher.submitTask("mln_browser_webgl_context_create") { dispatcher, token ->
            submitCreate(dispatcher, canvas.address, width, height, out.address, token)
          }
          // Written by the owner thread while this frame was parked, which is why the scratch is
          // read only after the task's completion has come back.
          Heap.loadInt(out)
        }
      // Zero is what the module reports for a context it could not create, and it is also the value
      // the C API refuses in a descriptor, so it is turned into a failure here rather than into a
      // handle that fails at attach.
      if (handle == 0) {
        throw Status.invalidState(
          "The MapLibre Native browser module could not create a ${width}x$height WebGL2 " +
            (if (id.isEmpty()) "context." else "context for the canvas \"$id\". ") +
            "The browser may have no WebGL2 support, too many contexts may be live, or the " +
            "canvas may not have been reserved before the owner thread started."
        )
      }
      return WebglContext(handle)
    }

    /** One four-byte output: a context handle, a texture name, or a success flag. */
    private const val FLAG_BYTES = 4
  }
}
