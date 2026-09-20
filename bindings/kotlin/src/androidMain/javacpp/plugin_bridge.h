#pragma once

#include <maplibre_native_c/plugin.h>

// JavaCPP borrows each struct argument by pointer; forward the views by value.
inline mln_status mln_android_plugin_load_library(
  const mln_buffer_view* path, const mln_buffer_view* entry_point
) {
  return mln_plugin_load_library(*path, *entry_point);
}
