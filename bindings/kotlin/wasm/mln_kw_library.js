// The JavaScript imports of the Kotlin/Wasm binding's page shim. See
// mln_kw_runtime.c.
//
// Each runs on the page and calls a function that Maplibre.loadModule
// installs before it makes any call that wakes. Kotlin contains each
// callback's failure, and the page reports anything else that escapes. An
// exception that unwound a page task would leave the page queue marked as
// running, which stops every later wake, and one that unwound a dispatch would
// leave the delivery queue busy.
addToLibrary({
  // The heap, for Kotlin, which reads it from outside the module as
  // Module.mlnKwHeap. A growth of the heap replaces its buffer, which leaves a
  // view of the old buffer short. The module refreshes its own views as its
  // JavaScript reads them, so each call here returns a view of the current
  // buffer.
  $mlnKwHeap: {
    view: null,
    bytes: () => HEAPU8,
    data: () => {
      const buffer = HEAPU8.buffer;
      if (mlnKwHeap.view?.buffer !== buffer) {
        mlnKwHeap.view = new DataView(buffer);
      }
      return mlnKwHeap.view;
    },
  },

  // Once for each wake of the delivery queue, from a task of the page queue.
  mln_kw_js_dispatch: () => {
    try {
      Module["mlnKwDispatch"]();
    } catch (error) {
      globalThis.reportError(error);
    }
  },

  // Once for each call that ran on a thread of its own, from a task of the
  // page queue.
  mln_kw_js_thread_end: (token, status) => {
    try {
      Module["mlnKwThreadEnd"](token, status);
    } catch (error) {
      globalThis.reportError(error);
    }
  },

  // Once for each call that the delivery queue's dispatch runs.
  mln_kw_js_upcall: (site, args) => {
    try {
      Module["mlnKwUpcall"](site, args);
    } catch (error) {
      globalThis.reportError(error);
    }
  },
});
