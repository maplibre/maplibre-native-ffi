// The Kotlin module's boot and the canvas registry it renders through.
//
// The registry is GL.offscreenCanvases rather than specialHTMLTargets, because
// findCanvasEventTarget(), which resolves the selector under
// -sOFFSCREENCANVAS_SUPPORT, searches the former and never consults the latter.
// It is also where -sOFFSCREENCANVASES_TO_PTHREAD puts a canvas the page
// transferred, so a private canvas and a displayed one are found the same way.
//
// An entry carries its canvas under both names its consumers unwrap:
// emscripten's WebGL path looks for `offscreenCanvas`, and its own transfer
// path stores `canvas`.
addToLibrary({
  // Imports the Kotlin/Wasm module into the realm of the pthread that
  // -sPROXY_TO_PTHREAD gave main(), which is the thread Kotlin may block on.
  // The specifier resolves against this worker's module URL, so the Kotlin
  // distribution is served beside maplibre_native_c.mjs.
  //
  // Module is assigned to globalThis because a Kotlin @JsFun body compiles to
  // an arrow function in the generated import object, which can see nothing
  // else.
  mln_kotlin_boot_module: () => {
    globalThis.Module = Module;
    import("./maplibre-native-kotlin.mjs")
      .then((module) => module.mlnKotlinMain())
      .catch((error) => {
        console.error("maplibre: the Kotlin module failed to start", error);
      });
  },

  mln_kotlin_canvas_register__deps: ["$GL", "$UTF8ToString"],
  mln_kotlin_canvas_register: (name, width, height) => {
    const id = UTF8ToString(name);
    const canvas = new OffscreenCanvas(width, height);
    GL.offscreenCanvases[id] = { canvas, offscreenCanvas: canvas, id };
  },

  mln_kotlin_canvas_unregister__deps: ["$GL", "$UTF8ToString"],
  mln_kotlin_canvas_unregister: (name) => {
    delete GL.offscreenCanvases[UTF8ToString(name)];
  },

  // Written here rather than through emscripten_set_canvas_element_size(),
  // which resolves the same registry but then assigns to the entry rather than
  // to the canvas inside it.
  mln_kotlin_canvas_size__deps: ["$GL", "$UTF8ToString"],
  mln_kotlin_canvas_size: (name, width, height) => {
    const registry = GL.offscreenCanvases;
    const id = UTF8ToString(name);
    // Own properties only. The registry is a plain object, so an id of
    // `toString` or `constructor` would otherwise report a canvas that no
    // registration put there. An entry is also null while its canvas moves to
    // another thread, which is present but not usable.
    const entry = Object.hasOwn(registry, id) ? registry[id] : undefined;
    const canvas = entry && (entry.offscreenCanvas || entry.canvas);
    if (!canvas) {
      return 0;
    }
    canvas.width = width;
    canvas.height = height;
    // A transferred canvas carries its size in shared memory so that the page
    // and this thread agree on it. Left stale,
    // emscripten_get_canvas_element_size() would keep reporting whatever the
    // element measured before the transfer.
    if (entry.canvasSharedPtr) {
      HEAP32[entry.canvasSharedPtr >> 2] = width;
      HEAP32[(entry.canvasSharedPtr + 4) >> 2] = height;
    }
    return 1;
  },
});
