package org.maplibre.nativeffi.render

/**
 * The `WebglContext` that a [WebglContextDescriptor] came from.
 *
 * Only the browser binding's context class implements this, and common code needs nothing from it
 * beyond identity. It exists so that a descriptor names an object rather than only a recyclable
 * handle; see [WebglContextDescriptor.owner].
 */
internal interface WebglContextOwner

/**
 * WebGL context descriptor for OpenGL render targets in a browser.
 *
 * The browser owns the context and a session draws into it rather than creating one, so the handle
 * is borrowed for as long as the render target exists. It is an `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`
 * rather than a pointer: the browser module keeps its contexts in a table of its own, and what
 * crosses the boundary is the entry's index. That is why this arm carries an [Int] where the other
 * two carry a [NativePointer].
 *
 * A host obtains one from the context it wants drawn to rather than by building one, which is the
 * whole difference between this arm and the other two. Everywhere else the host owns the graphics
 * API and fills a descriptor in from what it made; here the binding made the context, so the
 * binding fills this in and none of it is the host's to change.
 *
 * That is also what makes the handle safe to attach with. The module allocates a context handle and
 * frees it again when the context is destroyed, so the number names one context only until the next
 * context reuses it, and a descriptor kept past its context's close would otherwise name whichever
 * context inherited that number. So a descriptor carries the context it came from beside the
 * handle, and attaching resolves that rather than the number. A descriptor from a closed context is
 * refused however many contexts have been created since.
 */
public class WebglContextDescriptor
internal constructor(
  /** Borrowed `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`. Always positive. */
  public val context: Int,
  /** The `WebglContext` this names, which is what a render target is really attached to. */
  internal val owner: WebglContextOwner,
) : OpenGLContextDescriptor
