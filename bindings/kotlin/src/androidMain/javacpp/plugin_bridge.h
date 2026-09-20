#pragma once

#include <maplibre_native_c/plugin.h>
#include <stdint.h>

inline uintptr_t mln_android_plugin_register_function_v1() {
  return reinterpret_cast<uintptr_t>(mln_plugin_get_register_function_v1());
}
