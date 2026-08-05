#pragma once

#include <cstddef>
#include <string>

namespace std {

// JavaCPP's generated Android JNI glue uses std::basic_string<unsigned short>,
// a specialization Android NDK libc++ does not provide. Force-including this
// header keeps the generated source buildable.
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
