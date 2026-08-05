// Canvas registration for the browser render fixtures.
//
// A fixture renders into its own texture and never presents, so it needs a GL
// context but no on-page canvas. Each one gets a private OffscreenCanvas on
// whichever thread asked for it, because an OffscreenCanvas belongs to a single
// thread and the suite attaches sessions from more than one.
//
// The registry is GL.offscreenCanvases rather than specialHTMLTargets:
// findCanvasEventTarget(), which is what resolves the selector under
// -sOFFSCREENCANVAS_SUPPORT, searches the former and never consults the latter.
//
// An entry carries the OffscreenCanvas under both names its consumers unwrap:
// emscripten's WebGL path and emdawnwebgpu's surface creation each look for
// `offscreenCanvas`, while emscripten's own transfer path stores `canvas`.
//
// These are JavaScript library functions rather than a compiled shim so that a
// Rust test binary needs no C toolchain, and they carry no __proxy annotation so
// each one runs on the calling thread, which is where that thread's registry
// lives. src/c_api/tests/test_support.c does the same through EM_JS.
addToLibrary({
  mln_test_register_offscreen_canvas__deps: ["$GL", "$UTF8ToString"],
  mln_test_register_offscreen_canvas: (name, width, height) => {
    const id = UTF8ToString(name);
    const canvas = new OffscreenCanvas(width, height);
    GL.offscreenCanvases[id] = { canvas, offscreenCanvas: canvas, id };
  },

  // Reads a registered canvas back, which is how a surface test tells a
  // presented frame from a call that merely returned. A canvas holding a WebGPU
  // context cannot also give out a 2D one, so this draws it into a second
  // canvas and reads that.
  mln_test_read_canvas_rgba__deps: ["$GL", "$UTF8ToString"],
  mln_test_read_canvas_rgba: (name, out, capacity) => {
    const entry = GL.offscreenCanvases[UTF8ToString(name)];
    if (!entry) return 0;
    const source = entry.offscreenCanvas;
    const length = source.width * source.height * 4;
    if (length > capacity) return 0;
    const copy = new OffscreenCanvas(source.width, source.height);
    const context = copy.getContext("2d", { willReadFrequently: true });
    context.drawImage(source, 0, 0);
    const pixels = context.getImageData(0, 0, source.width, source.height).data;
    HEAPU8.set(pixels, out);
    return length;
  },

  // The canvas format this device prefers, which is what a browser host passes
  // in its surface descriptor. Reported as a code the fixture maps, so the
  // WebGPU enum values stay in the generated bindings.
  mln_test_preferred_canvas_format: () =>
    navigator.gpu.getPreferredCanvasFormat() === "bgra8unorm" ? 1 : 0,

  mln_test_unregister_offscreen_canvas__deps: ["$GL", "$UTF8ToString"],
  mln_test_unregister_offscreen_canvas: (name) => {
    delete GL.offscreenCanvases[UTF8ToString(name)];
  },
});
