// The page canvas, registered before the thread that will own it is created.
//
// Emscripten transfers a canvas to a pthread only as that thread is created,
// and the thread this binding runs on is created during instantiation, so a
// canvas the page displays has to be here before the factory resolves. The
// registry is consulted first (libpthread.js:723), ahead of the DOM lookup that
// fails pthread_create outright when a named selector matches nothing -- which
// is why an entry is registered either way. A host with no on-screen map
// transfers nothing and gets the placeholder, and its texture sessions never
// touch it.
// The name carries no "#", and both halves of the round trip need it that way:
// pthread_create looks this registry up by the transfer list entry verbatim
// (libpthread.js:723), while findCanvasEventTarget strips a leading "#" before
// looking up the same registry (libhtml5.js:357). A hash satisfies the first
// and breaks the second, which transfers the canvas and then cannot find it.
// The entry carries a shared size block because thread creation writes the
// owning thread into it for every canvas it transfers, without checking that
// one is there (libpthread.js:799-801). An entry without it takes
// `undefined + 8 >> 2`, which is address zero, so the write lands on the heap's
// first bytes instead. Emscripten allocates the same three fields when it
// builds an entry itself (libpthread.js:745-749).
Module["preRun"] ??= [];
Module["preRun"].push(() => {
  const canvas = Module["mlnPageCanvas"] ?? new OffscreenCanvas(1, 1);
  canvas.id = "maplibre";
  const shared = Module["_malloc"](12);
  HEAP32[shared >> 2] = canvas.width;
  HEAP32[(shared + 4) >> 2] = canvas.height;
  HEAPU32[(shared + 8) >> 2] = 0;
  GL.offscreenCanvases["maplibre"] = {
    canvas,
    offscreenCanvas: canvas,
    canvasSharedPtr: shared,
    id: "maplibre",
  };
});
