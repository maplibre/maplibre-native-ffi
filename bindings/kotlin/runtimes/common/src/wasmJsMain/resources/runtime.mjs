// The module object that the Kotlin/Wasm binding calls, over the browser
// package's wasm-bindgen bindings (`maplibre_native_c.js`).
//
// The binding reaches the object through a `Symbol.for` global and calls the
// module's raw C exports on it, `mln_*`, as on native. The object adds what
// Kotlin can't do from its own memory: views of the module's memory, which a
// growth replaces, allocation that remembers each block's size for
// `mln_web_free`, the table of JavaScript objects, and the dispatch of the
// module's callback trampolines.
//
// The module runs on the thread that loads it and never blocks it, so load
// the binding on a dedicated Web Worker to keep the map's work off the page.

/** Instantiates `bindings`, the package's module, and returns the module object. */
export async function instantiateFrom(bindings, options = {}) {
  const exports = await bindings.default(
    options.moduleOrPath === undefined
      ? undefined
      : { module_or_path: options.moduleOrPath },
  );
  const sizes = new Map();
  let view = null;
  const module = Object.create(null);
  for (const [name, value] of Object.entries(exports)) {
    if (name.startsWith("mln_") && typeof value === "function")
      module[name] = value;
  }
  module.mlnHeap = {
    bytes: () => new Uint8Array(exports.memory.buffer),
    data: () => {
      if (view?.buffer !== exports.memory.buffer)
        view = new DataView(exports.memory.buffer);
      return view;
    },
  };
  // Every block is aligned for any C type.
  module.mlnAllocate = (size) => {
    const bytes = Math.max(size, 1);
    const address = exports.mln_web_alloc(bytes, 8) >>> 0;
    if (address !== 0) sizes.set(address, bytes);
    return address;
  };
  module.mlnFree = (address) => {
    const bytes = sizes.get(address >>> 0);
    if (bytes === undefined) return;
    sizes.delete(address >>> 0);
    exports.mln_web_free(address, bytes, 8);
  };
  module.mlnWebObjectInsert = bindings.mlnWebObjectInsert;
  module.mlnWebObjectGet = bindings.mlnWebObjectGet;
  module.mlnWebUpcallPointer = bindings.mlnWebUpcallPointer;
  // Kotlin contains each callback's failure, and the page reports anything
  // else that escapes, which would otherwise turn the trampoline's result
  // into zero without a trace.
  module.mlnSetUpcallDispatcher = (dispatch) =>
    bindings.mlnWebSetUpcallDispatcher((name, ...args) => {
      try {
        return dispatch(name, args);
      } catch (error) {
        globalThis.reportError(error);
        return undefined;
      }
    });
  return module;
}
