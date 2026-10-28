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
