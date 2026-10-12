// Canvas registration for the browser render fixtures.
//
// A fixture renders into a texture and never presents, so it needs a context
// but no on-page canvas. Each one gets a private OffscreenCanvas on whichever
// thread asked for it, because an OffscreenCanvas belongs to a single thread.
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
// lives. tests/native/support/render_webgl.c does the same through EM_JS.
addToLibrary({
  mln_test_register_offscreen_canvas__deps: ["$GL", "$UTF8ToString"],
  mln_test_register_offscreen_canvas: (name, width, height) => {
    const id = UTF8ToString(name);
    const canvas = new OffscreenCanvas(width, height);
    GL.offscreenCanvases[id] = { canvas, offscreenCanvas: canvas, id };
  },

  mln_test_unregister_offscreen_canvas__deps: ["$GL", "$UTF8ToString"],
  mln_test_unregister_offscreen_canvas: (name) => {
    delete GL.offscreenCanvases[UTF8ToString(name)];
  },
});
