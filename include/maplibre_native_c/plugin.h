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
 * the rest of the process, and plugin authoring uses native code. Language
 * bindings expose the registration function for plugin integrations.
 *
 * A plugin integration calls its own registration entry point with the
 * function returned by mln_plugin_get_register_function_v1(). The plugin
 * registers its descriptors through that pointer, into this library's copy
 * of MapLibre Native. The integration owns loading the plugin and keeping
 * its code loaded for the process lifetime.
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

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_PLUGIN_H
