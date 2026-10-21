// The JavaScript halves of the Kotlin/Wasm suite's fixtures in test_support.c:
// the pthread pool's Workers and the canvases that a session's core worker
// takes over. Only the test module links them.
//
// Each function runs on the calling thread, the page, because each agent keeps
// its own GL.offscreenCanvases and an OffscreenCanvas belongs to one agent.
// Emscripten resolves a canvas selector through that registry under
// -sOFFSCREENCANVAS_SUPPORT, as tests/native/support/render_webgl.c does.
addToLibrary({
  // The Workers that wait in the pthread pool, or with [all] set, those and
  // the Workers that run a thread. The page keeps the pool as a list and the
  // running Workers by their thread.
  mln_kw_test_js_pool_workers__deps: ["$PThread"],
  mln_kw_test_js_pool_workers: (all) =>
    PThread.unusedWorkers.length +
    (all ? Object.keys(PThread.pthreads).length : 0),

  // Registers an OffscreenCanvas that pthread_create transfers to the thread
  // whose attributes name [selector]. The transfer looks the selector up with
  // its "#", and it records the owning thread in the 12 bytes at [shared].
  mln_kw_test_register_transferred_canvas__deps: ["$GL", "$UTF8ToString"],
  mln_kw_test_register_transferred_canvas: (
    selector,
    width,
    height,
    shared,
  ) => {
    const name = UTF8ToString(selector);
    const canvas = new OffscreenCanvas(width, height);
    GL.offscreenCanvases[name] = {
      canvas,
      offscreenCanvas: canvas,
      canvasSharedPtr: shared,
      id: name.slice(1),
    };
  },

  mln_kw_test_unregister_canvas__deps: ["$GL", "$UTF8ToString"],
  mln_kw_test_unregister_canvas: (name) => {
    delete GL.offscreenCanvases[UTF8ToString(name)];
  },

  // Adds a canvas element to the document, which pthread_create transfers
  // with transferControlToOffscreen. Emscripten keeps the size block at
  // [shared] on the element rather than allocating one of its own.
  mln_kw_test_add_dom_canvas__deps: ["$UTF8ToString"],
  mln_kw_test_add_dom_canvas: (selector, width, height, shared) => {
    const canvas = document.createElement("canvas");
    canvas.id = UTF8ToString(selector).slice(1);
    canvas.width = width;
    canvas.height = height;
    canvas.canvasSharedPtr = shared;
    document.body.appendChild(canvas);
  },

  mln_kw_test_remove_dom_canvas__deps: ["$UTF8ToString"],
  mln_kw_test_remove_dom_canvas: (selector) => {
    document.querySelector(UTF8ToString(selector))?.remove();
  },
});
