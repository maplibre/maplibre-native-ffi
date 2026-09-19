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

#endif  // MAPLIBRE_NATIVE_C_PLUGIN_H
