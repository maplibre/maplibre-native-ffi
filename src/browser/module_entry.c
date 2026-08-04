// The browser module's anchor translation unit.
//
// The module has no main. Its entry points are the C API's own MLN_API
// functions, which the link keeps through a generated export list rather than
// through anything referenced from here. A wasm link still needs an input that
// nothing reaches from an entry point, and this is it; the dispatch table, the
// dispatcher, and the log queue are compiled in beside it.
//
// Nothing belongs here that a host could call instead: an entry point added
// here would be browser-only C API that the headers do not describe.

#include "maplibre_native_c.h"

// Referenced by nothing, and deliberately not exported. Its purpose is to give
// the link an object file and to fail the build if the umbrella header stops
// compiling for the browser target.
static uint32_t mln_browser_module_anchor(void) { return mln_c_version(); }

__attribute__((used)) static uint32_t (*const mln_browser_module_keep)(void) =
  mln_browser_module_anchor;
