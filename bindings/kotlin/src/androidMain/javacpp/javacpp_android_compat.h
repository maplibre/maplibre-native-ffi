#pragma once

#include <cstddef>
#include <string>

namespace std {

// JavaCPP's generated Android JNI glue uses std::basic_string<unsigned short>
// for UTF-16 string handling, but Android NDK libc++ does not provide this
// specialization. Force-including this header keeps the generated source
// buildable without carrying a patched JavaCPP runtime.
template <>
struct char_traits<unsigned short> {
  using char_type = unsigned short;
  using int_type = unsigned int;
  using off_type = streamoff;
  using pos_type = streampos;
  using state_type = mbstate_t;

  static size_t length(const char_type* value) {
    size_t size = 0;
    while (value[size] != 0) {
      ++size;
    }
    return size;
  }
};

}  // namespace std
