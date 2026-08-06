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
// The key is the selector as written in the transfer list, not the element id
// it names: pthread_create looks the registry up before it parses the selector
// (libpthread.js:723), so the two only agree when the hash is kept.
Module["preRun"] ??= [];
Module["preRun"].push(() => {
  const canvas = Module["mlnPageCanvas"] ?? new OffscreenCanvas(1, 1);
  canvas.id = "maplibre";
  GL.offscreenCanvases["#maplibre"] = {
    canvas,
    offscreenCanvas: canvas,
    id: "#maplibre",
  };
});
