package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
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
 * What this wrapper is called where the module counts what its owner thread still owns.
 *
 * The same name the release bookkeeping uses, because both name it to a host: one in the failure
 * for closing a context a render target still holds, the other in the failure for shutting the
 * module down while this is open.
 */
private const val TYPE_NAME = "WebglContext"

/**
 * A WebGL context on the thread this binding's maps render on, and the GL work a host does there.
 *
 * Every other platform hands a render target a context the host made with its own graphics API, and
 * a browser host cannot make one at all. The handle a [WebglContextDescriptor] carries is an index
 * into the Emscripten module's own context table, so a context the page created with
 * `canvas.getContext("webgl2")` names nothing native can look up. A WebGL context also belongs to
 * the agent that created it, and the agent that renders is the thread this binding places
 * owner-affine work on rather than the page. The entry points that satisfy both conditions are
 * inside the module this binding distributes, so this is where a host reaches them.
 *
 * That is why a context is the binding's to make here and the host's everywhere else. On a desktop
 * or a phone the graphics API is EGL, Metal, or Vulkan, which the host genuinely owns; in a browser
 * it is this module and this module's thread.
 *
 * WebGL has no share groups, so a texture belongs to the one context it was made in. A host that
 * wants a texture for a caller-owned target, or wants a rendered one on the page, issues those GL
 * calls in this context on that thread, which is what [createTexture] and [presentTexture] are.
 *
 * The canvas behind the context is either a page canvas reserved with [reserveCanvas] — whose
 * `<canvas>` element keeps displaying whatever this draws, with no copy — or a private
 * `OffscreenCanvas` on the owner thread, which nothing displays and which a host reads back from.
 *
 * Every call here reaches the owner thread, so every call belongs inside a
 * [maplibreScope][org.maplibre.nativeffi.maplibreScope], as on every other path into this binding.
 * [reserveCanvas] is the exception, because it runs before that thread exists.
 *
 * Closing is what releases the context, and there is no finalizer behind it: a browser host cannot
 * recover leaked resources by restarting a process. A render target borrows the handle for its
 * whole life, so this context stays open while one is attached: closing it then returns an
 * invalid-state status naming the render session that holds it, and the host detaches or closes
 * that session first. A close that native refuses leaves this context open for a retry.
 */
public class WebglContext private constructor(private val handle: Int) : AutoCloseable {
  // The same release bookkeeping every owned handle in this binding uses, for the two properties it
  // brings. A render target retains this as a child, so the context outlives every target naming
  // it; and a destroy the module refuses restores the open state, so the wrapper is retryable
  // rather than spent while the native context is still there.
  private val core = HandleStateCore(TYPE_NAME, handle.toLong())

  init {
    // Counted so that shutting the module down while this is open is refused and names it, rather
    // than reaching the terminal failure a shutdown reports for anything it could not see. Every
    // other owner-affine handle is covered by the runtime it retains; a context has no runtime
    // behind it, so it is the one that has to say so itself.
    Dispatcher.retainHandle(TYPE_NAME)
  }

  /** Reports whether this context has been released. */
  public val isClosed: Boolean
    get() = core.isReleased()

  /**
   * Returns a descriptor naming this context, for a render target to be attached with.
   *
   * The descriptor names this object and not merely its handle, which is what makes it safe to hold
   * on to. Emscripten allocates a context handle and frees it when the context is destroyed, so the
   * number is reused by the next context created afterwards; a descriptor that carried only the
   * number would, once this context is closed, start naming whichever context inherited it and
   * would attach a render target to that one instead. Carrying the object means a descriptor from a
   * closed context stays a descriptor from a closed context.
   */
  public fun descriptor(): WebglContextDescriptor {
    requireOpen()
    return WebglContextDescriptor(handle, this)
  }

  /**
   * Reports that this context is still open, without holding a use count while native works.
   *
   * Every call below parks its stack on the owner thread, and a use count held across that park
   * would be a count held for as long as native takes. Closing is the caller's own next statement,
   * on the same single-threaded page, so the check is what is wanted here and the count is not.
   */
  private fun requireOpen() {
    core.requireLive()
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
  public fun resizeCanvas(width: Int, height: Int) {
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
  public fun presentTexture(texture: Int, width: Int, height: Int) {
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
  public fun createTexture(width: Int, height: Int): Int {
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
   * it copies every pixel through the module's heap. A host that only wants the frame seen presents
   * it instead. Reading back is for a host that consumes the pixels itself — encoding an image,
   * comparing two frames — and for telling a frame that was never drawn from a frame that was drawn
   * and never composited, which are the same symptom from the page's side and completely different
   * underneath.
   */
  public fun readPixels(texture: Int, width: Int, height: Int): ByteArray {
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
  public fun destroyTexture(texture: Int) {
    requireOpen()
    Dispatcher.submitTask("mln_browser_webgl_texture_destroy") { dispatcher, token ->
      submitTextureDestroy(dispatcher, handle, texture, token)
    }
  }

  override fun close() {
    core.closeOnce(
      destroy = {
        // The module reports a refused submission by throwing rather than by a status, and that
        // throw is what closeOnce turns back into an open context. A task that was accepted always
        // completes, so there is no destroy status to check here.
        Dispatcher.submitTask("mln_browser_webgl_context_destroy") { dispatcher, token ->
          submitDestroy(dispatcher, handle, token)
        }
        MaplibreStatus.OK.nativeCode
      },
      // Only a destroy that happened stops the count. A refused submission leaves the native
      // context there and this wrapper open for a retry, and releasing here would let a shutdown be
      // accepted while it still exists.
      afterSuccess = { Dispatcher.releaseHandle(TYPE_NAME) },
    )
  }

  public companion object {
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
     *
     * Called from the page rather than from inside a
     * [maplibreScope][org.maplibre.nativeffi.maplibreScope]: only the agent holding a canvas can
     * give it away, and the thread that receives it does not exist yet.
     *
     * [id] may be any `id` an HTML document accepts, including one no CSS identifier can spell,
     * within three limits the module imposes: no comma, because the ids cross to native as one
     * comma-separated list; no ASCII whitespace, which that list trims off each entry and which
     * HTML forbids in an `id` anyway; and at most 63 bytes of UTF-8, which is what the module's
     * fixed-size canvas record holds.
     *
     * Those are checked here as well as at [createForCanvas], because transferring a canvas cannot
     * be undone. A host that learned about a limit from the context failing would learn about it
     * with the `<canvas>` element already given away for the page's whole life and nothing left
     * able to draw into it.
     */
    public fun reserveCanvas(id: String) {
      requireCanvasId(id)
      Dispatcher.reserveCanvas(id)
    }

    /**
     * Creates a WebGL2 context against a canvas reserved with [reserveCanvas].
     *
     * [width] and [height] size that canvas's drawing buffer in device pixels, which for a surface
     * target is the target's physical extent and for a texture target is the size anything
     * [presentTexture] shows must fit.
     *
     * [id] follows [reserveCanvas]'s rules, and names a canvas reserved there: the two spell the id
     * differently on the way to the browser — a selector there, a registry key here — so an id both
     * accept is what makes the pair name one element.
     */
    public fun createForCanvas(id: String, width: Int, height: Int): WebglContext {
      requireCanvasId(id)
      return create(id, width, height)
    }

    /**
     * Reports that [id] is a canvas id both halves of the module can name.
     *
     * A `<canvas>` reaches the owner thread through [reserveCanvas] and is found again here, and
     * the two spell an id differently — a selector there, a registry key here — so an id has to
     * survive both. What that rules out is narrower than what an HTML document accepts, and each
     * exclusion is one of the two spellings:
     * - A comma separates the ids in the list that crosses to native, so one inside an id would
     *   split it into two that name nothing.
     * - ASCII whitespace is trimmed off each entry of that list, so a reserved `" map "` transfers
     *   the element `map` while a context asks the registry for `" map "` and finds nothing. HTML
     *   forbids whitespace in an id anyway, so refusing is the honest answer rather than trimming
     *   one and not the other.
     * - The module carries the id in a record of [MAX_CANVAS_ID_BYTES] bytes with the terminator
     *   inside it, so an id may encode to one byte fewer than that.
     *
     * Checked here rather than left to native because of *when* native checks. Reserving a canvas
     * transfers it, transferring cannot be undone, and the module refuses the id only afterwards —
     * so a host that learned about the limit from the failure would learn about it with the element
     * already gone.
     */
    private fun requireCanvasId(id: String) {
      Status.requireArgument(id.isNotEmpty()) { "a canvas id must not be empty" }
      Status.requireArgument(',' !in id) { "a canvas id must not contain a comma, but was \"$id\"" }
      Status.requireArgument(id.none(::isAsciiWhitespace)) {
        "a canvas id must not contain whitespace, but was \"$id\""
      }
      Heap.requireCString(id, "canvas id")
      // Encoded here rather than measured with Heap.utf8Size, which asks the module for the length
      // and so needs the module loaded. Reserving happens before it is, which is the whole point of
      // reserving, so this check has to be one Kotlin can make on its own.
      val bytes = id.encodeToByteArray().size
      Status.requireArgument(bytes < MAX_CANVAS_ID_BYTES) {
        "a canvas id must be at most ${MAX_CANVAS_ID_BYTES - 1} bytes of UTF-8, but \"$id\" is " +
          "$bytes"
      }
    }

    /**
     * The whitespace HTML forbids in an `id`, which is also what the module's id list trims.
     *
     * Narrower than [Char.isWhitespace], deliberately: that one also matches a non-breaking space
     * and the other Unicode separators, which an HTML id may hold and which the module carries
     * through untouched. Refusing those would refuse ids that work.
     */
    private fun isAsciiWhitespace(character: Char): Boolean =
      character == ' ' ||
        character == '\t' ||
        character == '\n' ||
        character == '\u000C' ||
        character == '\r'

    /**
     * Creates a WebGL2 context against a private `OffscreenCanvas` the owner thread makes.
     *
     * Nothing displays that canvas, which is what a host that reads frames back wants. A texture
     * target renders into a framebuffer of its own rather than into it, so for one the size bounds
     * nothing the map draws and only has to be positive, because a zero-sized canvas has no drawing
     * buffer to create a context against.
     */
    public fun createOffscreen(width: Int, height: Int): WebglContext = create("", width, height)

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

    /**
     * Retains the context a render target is about to borrow, for as long as that target lives.
     *
     * A render target names its context by handle for its whole life, and the backend makes that
     * handle current on every frame and again while it tears its GL objects down. Destroying the
     * context underneath it would leave a live target naming a context that is gone, so the target
     * holds the context open the way a render session holds its map open, and closing the context
     * first reports the live child instead.
     *
     * The context comes off the descriptor rather than out of a lookup by handle, and that is what
     * makes a stale descriptor safe. Emscripten's handles are allocated and freed, so the number in
     * a descriptor from a closed context is one a later context can be given; a lookup by number
     * would find that later context, retain it, and attach a target to a context the host never
     * named. Resolving the object cannot confuse the two, however many contexts have come and gone.
     *
     * Returns null for the WGL and EGL arms, which name a context from a graphics API this module
     * was not built against. Nothing here can retain one, and native is where a build's capability
     * is known, so those are passed down and refused there.
     */
    internal fun retainForTarget(
      context: OpenGLContextDescriptor
    ): HandleStateCore.ChildRetention? {
      if (context !is WebglContextDescriptor) return null
      // Total, because the descriptor's constructor is internal and descriptor() is its one caller.
      val owner = context.owner as WebglContext
      if (owner.isClosed) {
        throw Status.invalidArgument(
          "A WebGL context descriptor names a WebglContext that has been closed. A render target's " +
            "context comes from WebglContext.descriptor() and stays open until every target that " +
            "borrowed it is detached or closed; the handle ${context.context} it carries may " +
            "since have been reused by another context, which is why this is refused rather than " +
            "resolved."
        )
      }
      return owner.core.retainChild("RenderSessionHandle")
    }

    /** One four-byte output: a context handle, a texture name, or a success flag. */
    private const val FLAG_BYTES = 4

    /**
     * The bytes the module's fixed-size canvas record holds, terminator included.
     *
     * `MLN_BROWSER_WEBGL_CANVAS_ID_BYTES` in `src/browser/webgl_context.c`, and a real limit rather
     * than a defensive one: the record is what lets the module copy a canvas key out from under its
     * own lock into a stack buffer, and lets a create request carry the id by value to the owner
     * thread, neither of which allocates. Sixty-three bytes is past what an element id needs, so
     * the limit is stated and enforced rather than lifted.
     */
    private const val MAX_CANVAS_ID_BYTES = 64
  }
}
