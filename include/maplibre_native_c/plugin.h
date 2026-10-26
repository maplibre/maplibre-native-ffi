/**
 * @file maplibre_native_c/plugin.h
 * Layer plugin registration.
 *
 * Native plugins register style layer types through mln_plugin_register_v1().
 * The plugin contract is declared in mln/plugin/plugin_api.h, installed next
 * to this library's headers and versioned independently of the map API.
 * Include this header separately from maplibre_native_c.h.
 *
 * A plugin integration calls its own registration entry point with the
 * function returned by mln_plugin_get_register_function_v1(), once per process
 * and before any style that uses its layer types loads. The integration owns
 * loading the plugin and keeping its code loaded for the process lifetime.
 *
 * The plugin must be built against the same MapLibre Native revision as the
 * library; upstream does not yet promise ABI stability for this API.
 *
 * Plugin callbacks run on native tile workers and the render thread and may
 * run during process exit, after host exit handlers and static destructors.
 * A plugin must not use state that those destroy.
 *
 * The plugin API declares shaders for the OpenGL, Vulkan, and Metal backends.
 * A WebGPU build registers plugins but has no shader path for their layers.
 */

#ifndef MAPLIBRE_NATIVE_C_PLUGIN_H
#define MAPLIBRE_NATIVE_C_PLUGIN_H

// Installs and Apple frameworks carry upstream's header beside this one.
#if __has_include("../mln/plugin/plugin_api.h")
#include "../mln/plugin/plugin_api.h"  // IWYU pragma: export
#else
#include <mln/plugin/plugin_api.h>  // IWYU pragma: export
#endif

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the process-wide plugin registration entry point; never null.
 *
 * The entry point is an mln_plugin_register_v1 function.
 */
MLN_BINDING("kind=native_pointer;ownership=borrowed;lifetime=process")
MLN_API mln_plugin_register_function_v1
mln_plugin_get_register_function_v1(void) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_PLUGIN_H
