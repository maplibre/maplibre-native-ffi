// What the module's package starts beside the module. wasm-bindgen copies this
// file into the package's snippets/.

/** A tile worker Web Worker, from the module script at `url`. */
export function tileWorkerSpawner(url) {
  return () => new Worker(url, { type: "module" });
}
