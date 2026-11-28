/**
 * @file maplibre_native_c/web.h
 * JavaScript objects and linear memory for a host in a browser.
 *
 * In a browser, the library is a WebAssembly module (wasm32-unknown-unknown)
 * that runs on the thread that loaded it: a Web Worker, or the page. Each
 * instance has its own memory and one thread.
 *
 * Beside the functions here, the module's JavaScript bindings export two
 * functions that tell the library where the files it loads beside the module
 * are. Call them before you create the first runtime. A package that ships
 * those files usually calls both when it loads the module.
 *
 * - `mlnWebConfigureTileWorkers(count, spawn)` sets how the instance starts
 *   the Web Workers that load its maps' tiles: `count` of them, or a default
 *   number when `count` is undefined, each the return value of `spawn()`. A
 *   new call replaces both settings. The script of each Web Worker that
 *   `spawn()` returns loads the module's bindings, and gets a message
 *   `{module, port}` for each map: on the first, it initializes the bindings
 *   with `module`, and for each it calls their `mlnWebServeTileWorker(port)`.
 *   Without a `spawn`, maps load their tiles on the instance's thread, and
 *   the library logs a warning when a map starts to render.
 * - `mlnWebSetShaperLoader(loader)` sets the function that loads the font-face
 *   shaper module: `loader()` returns a promise of the module's bindings,
 *   initialized. The library calls it the first time a style declares font
 *   faces. Without a loader, those font faces draw no text, and the library
 *   logs an error when a style declares them.
 */

#ifndef MAPLIBRE_NATIVE_C_WEB_H
#define MAPLIBRE_NATIVE_C_WEB_H

#include <stddef.h>
#include <stdint.h>

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A JavaScript object that you lent the library, as a handle into the object
 * table of the module instance that you loaded. 0 names no object.
 *
 * Make one with the module's JavaScript export `mlnWebObjectInsert(value)`. A
 * handle is valid only in the instance that made it.
 *
 * A call that takes a handle takes its own reference to the object before it
 * returns, so you may release the handle right after the call. A handle that
 * the library gives you is valid until the release of the object that it came
 * from, such as an acquired frame. Don't release those.
 */
typedef uint32_t mln_web_object;

/**
 * Releases a handle that you made. Releasing 0, or a handle that names no
 * object, does nothing.
 */
MLN_BINDING("platforms=web")
MLN_API void mln_web_object_release(mln_web_object object) MLN_NOEXCEPT;

/**
 * Allocates `size` bytes of the instance's memory, aligned to `align`, a power
 * of two, for the descriptors and buffers that you pass the library.
 *
 * Returns null when `size` is 0, `align` isn't a power of two, or the memory
 * can't grow. The memory's `buffer` detaches when it grows, so make new views
 * of it after each call.
 */
MLN_BINDING("platforms=web")
MLN_API void* mln_web_alloc(size_t size, size_t align) MLN_NOEXCEPT;

/**
 * Frees what mln_web_alloc() allocated, with the same `size` and `align`.
 * Freeing null does nothing.
 */
MLN_BINDING("platforms=web")
MLN_API void mln_web_free(
  void* pointer, size_t size, size_t align
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_WEB_H
