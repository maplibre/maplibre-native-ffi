// The JavaScript import of the Kotlin/Wasm binding's wake shim. See
// mln_kw_runtime.c.
addToLibrary({
  // Runs on the page, from a task of the shim's page queue, once for each
  // wake. Kotlin installs the function before it makes any call that wakes.
  //
  // Kotlin contains each callback's failure, and the page reports anything
  // else that escapes. An exception that unwound the task would leave the page
  // queue marked as running, which stops every later wake.
  mln_kw_js_deliver: (kind, token, status) => {
    try {
      Module["mlnKwDeliver"](kind, token, status);
    } catch (error) {
      globalThis.reportError(error);
    }
  },
});
