package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.lifecycle.BorrowedResourceCore
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.Dispatcher
import org.maplibre.nativeffi.internal.wasm.Heap

@JsFun(
  "(d, width, height, out, token) => " +
    "globalThis.__maplibreNativeC._mln_browser_webgl_context_create(d, width, height, out, token)"
)
private external fun submitCreate(
  dispatcher: Int,
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

/**
 * A WebGL context on the thread this binding's maps render on.
 *
 * Every other platform hands a render target a context the host made with its own platform API, and
 * a browser host cannot: the handle a [WebglContextDescriptor] carries is an index into the
 * Emscripten module's own table, so a context the page created with `canvas.getContext("webgl2")`
 * is not one native can look up. This is where a host gets one that native can.
 *
 * A WebGL context belongs to the thread that created it, and the thread that renders is the one the
 * module owns, so the context is created there. That is also why this exists at all rather than
 * being a field a host fills in: there is no way for page code to reach that thread's context
 * table.
 *
 * The canvas behind the context is a private `OffscreenCanvas` on that thread, never on the page.
 * This build renders into texture targets, which draw into a framebuffer of their own, so nothing
 * would be displayed by a canvas even if one had been transferred; a frame reaches the page through
 * [RenderSessionHandle.readPremultipliedRgba8] instead.
 *
 * Closing is what releases the context and its canvas, and there is no finalizer behind it: a
 * browser host cannot recover leaked resources by restarting a process. Close it only after every
 * render target that borrowed it has been detached or destroyed, because the C API borrows the
 * handle for a target's lifetime.
 */
public class WebglContext private constructor(private val handle: Int) : AutoCloseable {
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
  public fun descriptor(): WebglContextDescriptor = core.withOpenResource {
    WebglContextDescriptor(handle)
  }

  public override fun close(): Unit = core.close()

  public companion object {
    /**
     * Creates a WebGL2 context, sized to back a target of [width] by [height] device pixels.
     *
     * The size is the backing canvas's. A texture target renders into a framebuffer of its own
     * rather than into that canvas, so this bounds nothing the map draws; it only has to be
     * positive, because a zero-sized canvas has no drawing buffer to create a context against.
     *
     * Call this inside a `maplibreScope`, like every other call that reaches the owner thread.
     */
    public fun create(width: Int, height: Int): WebglContext {
      Status.requireArgument(width > 0) { "width must be positive, but was $width" }
      Status.requireArgument(height > 0) { "height must be positive, but was $height" }
      val handle =
        Heap.withScratch(CONTEXT_BYTES) { out ->
          Dispatcher.submitTask("mln_browser_webgl_context_create") { dispatcher, token ->
            submitCreate(dispatcher, width, height, out.address, token)
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
            "context. The browser may have no WebGL2 support, or too many contexts may be live."
        )
      }
      return WebglContext(handle)
    }

    private const val CONTEXT_BYTES = 4
  }
}
