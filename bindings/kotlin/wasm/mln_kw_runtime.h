// The page shim of the Kotlin/Wasm binding, linked into the browser module.
//
// Kotlin runs on the module's main runtime thread, the page, and MapLibre's
// threads cannot call it. Every registration and completion names the module's
// one delivery queue, which native code fills, so the shim only wakes the page:
// the queue's wake, and the end of a call that runs on a thread of its own.
// Each wake is a task on a private proxying queue, which the page runs only
// from its event loop, never inside a futex wait of a C call, so Kotlin never
// runs inside a native call that the page made. The page then dispatches the
// queue, which calls each site's trampoline in mln_kw_generated.c.
//
// This header includes no Emscripten header, so the generated wrappers that
// include it compile for any wasm32 target.
#ifndef MLN_KW_RUNTIME_H
#define MLN_KW_RUNTIME_H

#include <maplibre_native_c.h>
#include <stddef.h>
#include <stdint.h>

/** The bytes of arguments that a thread start copies. */
#define MLN_KW_THREAD_START_ARGUMENTS_SIZE 64

/** One call that runs on a thread of its own, and its status once it returns.
 */
typedef struct mln_kw_thread_start mln_kw_thread_start;

/**
 * Starts the call [run] with a copy of the [size] bytes at [arguments] on a
 * detached pthread, and returns at once. When the call returns, the page
 * receives its status for the token that [start] carries.
 *
 * Returns MLN_STATUS_OK when the thread started, and MLN_STATUS_NATIVE_ERROR
 * otherwise, when the caller still owns [start].
 */
int32_t mln_kw_start_thread(
  mln_kw_thread_start* start, int32_t (*run)(const void* arguments),
  const void* arguments, size_t size
);

/**
 * Runs the Kotlin upcall site numbered [site] with the arguments that
 * [arguments] holds, one in each 8-byte slot. Defined in mln_kw_library.js;
 * only the page's dispatch of the delivery queue calls it.
 */
void mln_kw_js_upcall(uint32_t site, const uint64_t* arguments);

#endif  // MLN_KW_RUNTIME_H
