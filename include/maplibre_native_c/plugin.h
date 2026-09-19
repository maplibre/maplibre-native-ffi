/**
 * @file plugin.h
 * Layer plugin registration.
 *
 * MapLibre Native's plugin API lets native code register style layer types
 * that the renderer draws itself: a plugin declares paint properties and
 * per-backend shaders, lays out tile features into vertex data on tile
 * workers, and fills uniform blocks on the render thread. This library builds
 * that support into its core and exports the registration entry point,
 * mln_plugin_register_v1(). The contract is upstream's plugin header, which
 * installs next to this library's headers as mln/plugin/plugin_api.h.
 *
 * This header stays outside maplibre_native_c.h on purpose. Plugin authoring
 * is a separate contract from the map API: upstream versions its structs
 * independently, its callbacks run on tile workers and the render thread for
 * the rest of the process, and it is for native code that links this library
 * directly. Language bindings expose it through their raw C layer only.
 *
 * Loading a plugin is a separate, narrower contract that any consumer can
 * drive: a plugin built as a shared library exports an entry point that takes
 * mln_plugin_register_function_v1, so the plugin binary never links this
 * library. mln_plugin_load_library() covers the whole pattern for
 * managed-language consumers that bind only this library: it opens the plugin
 * library, resolves its entry point, and registers through the register
 * function. Native consumers that already link this library may instead call
 * the entry point themselves with mln_plugin_get_register_function_v1(), or
 * pass mln_plugin_register_v1 directly when they can take its address.
 *
 * A plugin registers once per process, before any style that uses its layer
 * types loads, into the copy of MapLibre Native that this library carries.
 * The plugin must be built against the same MapLibre Native revision as the
 * library; upstream does not yet promise ABI stability for this API.
 *
 * The plugin API declares shaders for the OpenGL, Vulkan, and Metal backends.
 * A WebGPU build registers plugins but has no shader path for their layers.
 */

#ifndef MAPLIBRE_NATIVE_C_PLUGIN_H
#define MAPLIBRE_NATIVE_C_PLUGIN_H

#include <mln/plugin/plugin_api.h>  // IWYU pragma: export

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the process-wide mln_plugin_register_v1 entry point; never null.
 *
 * The accessor exists for consumers that cannot take a C function's address,
 * such as managed-language bindings over this library. Such a consumer obtains
 * the register function here and passes it to the entry point of a plugin
 * shared library, which registers its layer types through the pointer. See
 * the file-top comment for the loading pattern.
 */
MLN_API mln_plugin_register_function_v1
mln_plugin_get_register_function_v1(void) MLN_NOEXCEPT;

/**
 * Loads a layer plugin shared library and registers its layer types.
 *
 * path and entry_point are UTF-8 text, borrowed for the call. The function
 * opens the shared library at path with platform dynamic loading, resolves
 * entry_point, and calls it as
 * mln_plugin_status (*)(mln_plugin_register_function_v1, char*, size_t),
 * passing the register function from mln_plugin_get_register_function_v1() and
 * a buffer for the plugin's registration diagnostic. The library is never
 * unloaded, because registration retains the plugin's callbacks for the
 * process lifetime.
 *
 * Call on any thread, before any style that uses the plugin's layer types
 * loads. A repeated load of an identical plugin succeeds: the entry point's
 * already-registered status maps to MLN_STATUS_OK.
 *
 * Returns:
 * - MLN_STATUS_OK when the plugin registered or was already registered.
 * - MLN_STATUS_INVALID_ARGUMENT when path or entry_point is empty.
 * - MLN_STATUS_NATIVE_ERROR when the operating system cannot load the library
 *   or resolve the entry point, or when the entry point reports a registration
 *   failure; the thread diagnostic carries the OS or plugin message.
 */
MLN_API mln_status mln_plugin_load_library(
  mln_buffer_view path, mln_buffer_view entry_point
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_PLUGIN_H
