package org.maplibre.nativeffi.render

/**
 * Mutable WebGL context descriptor for OpenGL render targets in a browser.
 *
 * The browser owns the context and a session draws into it rather than creating one, so the handle
 * is borrowed for as long as the render target exists. It is an `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`
 * rather than a pointer: the browser module keeps its contexts in a table of its own, and what
 * crosses the boundary is the entry's index. That is why this arm carries an [Int] where the other
 * two carry a [NativePointer].
 *
 * A host obtains one by creating a context on the canvas it wants drawn to, through the browser
 * module rather than through the page's own WebGL API, so the context lives in the table the module
 * renders from.
 */
public class WebglContextDescriptor(context: Int) : OpenGLContextDescriptor {
  /** Borrowed `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`. Must be positive. */
  public var context: Int = context
}
