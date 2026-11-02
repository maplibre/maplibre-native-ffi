/**
 * @file maplibre_native_c/plugin.h
 * Add style layer types from a native plugin.
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
 * Returns the function that registers a plugin's style layer types.
 *
 * Pass the function to the plugin's registration entry point before loading a
 * style that uses the plugin's layer types. Plugin callbacks run on library
 * threads and can keep running while the process exits, so they must not use
 * state that the process destroys at exit.
 *
 * Lifetimes:
 * The function stays valid until the process exits.
 */
MLN_BINDING("kind=native_pointer;ownership=borrowed;lifetime=process")
MLN_API mln_plugin_register_function_v1
mln_plugin_get_register_function_v1(void) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_PLUGIN_H
