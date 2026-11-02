// The JavaScript halves of the Kotlin/Wasm suite's fixtures in test_support.c:
// the pthread pool's Workers and the canvases that a session takes over. Only
// the test module links them. Each function runs on the calling thread, the
// page.
addToLibrary({
  // The Workers that wait in the pthread pool, or with [all] set, those and
  // the Workers that run a thread. The page keeps the pool as a list and the
  // running Workers by their thread.
  mln_kw_test_js_pool_workers__deps: ["$PThread"],
  mln_kw_test_js_pool_workers: (all) =>
    PThread.unusedWorkers.length +
    (all ? Object.keys(PThread.pthreads).length : 0),

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
