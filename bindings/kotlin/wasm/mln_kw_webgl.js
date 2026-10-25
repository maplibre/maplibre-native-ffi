// The WebGL registration of the Kotlin/Wasm binding's WebGraphics, which
// the module of a WebGL build links and exports as Module.mlnKwWebGl.
//
// Emscripten names a WebGL context by a handle in the GL table of the agent
// that registered it, so only the module can turn a page's
// WebGL2RenderingContext into the handle that a session descriptor takes.
// Unregistering the handle leaves the context usable.
addToLibrary({
  $mlnKwWebGl__deps: [
    "$GL",
    "emscripten_webgl_get_current_context",
    "emscripten_webgl_make_context_current",
  ],
  $mlnKwWebGl: {
    // Returns the context's handle, or 0 when [context] is not a WebGL 2
    // context in this realm. The registration records the attributes that
    // the context was created with.
    registerContext: (context) =>
      globalThis.WebGL2RenderingContext &&
      context instanceof WebGL2RenderingContext
        ? GL.registerContext(context, {
            ...context.getContextAttributes(),
            majorVersion: 2,
            minorVersion: 0,
          })
        : 0,

    // Unregisters [handle] as GL.deleteContext does, but keeps the handle's
    // memory. A pthreads build allocates each handle, so a freed handle can
    // name the next registration, and a stale descriptor would then draw into
    // another context. A kept handle names nothing, so making it current
    // fails. GL.deleteContext also removes the module's event handlers from
    // the canvas, which belongs to the page here.
    unregisterContext: (handle) => {
      if (_emscripten_webgl_get_current_context() == handle) {
        _emscripten_webgl_make_context_current(0);
      }
      const canvas = GL.contexts[handle].GLctx.canvas;
      if (canvas) canvas.GLctxObject = undefined;
      GL.contexts[handle] = null;
    },
  },
});
