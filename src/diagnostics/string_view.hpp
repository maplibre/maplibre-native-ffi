#pragma once

#include <cstring>
#include <string>
#include <string_view>

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"

namespace mln::core {

inline auto string_from_view(mln_buffer_view value) -> std::string {
  return value.size == 0
           ? std::string{}
           : std::string{static_cast<const char*>(value.data), value.size};
}

inline auto string_view_from_view(mln_buffer_view value) -> std::string_view {
  return value.size == 0
           ? std::string_view{}
           : std::string_view{static_cast<const char*>(value.data), value.size};
}

// Strings that reach a C-string consumer must preserve their full length.
inline auto validate_c_string_view(mln_buffer_view value, const char* name)
  -> bool {
  if (value.data == nullptr && value.size != 0) {
    set_thread_error(
      (std::string{name} + " has null data with a nonzero size").c_str()
    );
    return false;
  }
  if (value.size != 0 && std::memchr(value.data, '\0', value.size) != nullptr) {
    set_thread_error((std::string{name} + " contains embedded NUL").c_str());
    return false;
  }
  return true;
}

}  // namespace mln::core
