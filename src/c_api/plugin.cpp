#define MLN_BUILDING_C

#include "maplibre_native_c/plugin.h"

auto mln_plugin_get_register_function_v1(void) noexcept
  -> mln_plugin_register_function_v1 {
  return &mln_plugin_register_v1;
}
