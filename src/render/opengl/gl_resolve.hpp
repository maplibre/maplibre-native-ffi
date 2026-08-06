#pragma once

// Looks up a GL entry point in the client library, for the generated table in
// mbgl::platform. Resolution needs no current context, so an entry point can
// resolve before a display exists.

#include <type_traits>

#include "render/opengl/egl_common.hpp"

#if !defined(__linux__) && !defined(__APPLE__)
// The resolver below reaches the GL client library through dlopen and returns
// null without it, leaving the whole table null. Platforms without one link
// their loader instead.
#error "the generated GL table needs a POSIX dynamic loader"
#endif

namespace mln::core::opengl {

inline auto gl_proc_address(const char* name) -> void* {
  return get_egl_client_library_proc_address(name, EGL_OPENGL_ES_API);
}

// Stands in for one entry of the generated table, which holds function pointers
// that a shared library initializes as it loads. Forwarding through a stub
// moves the lookup to the first call, so the client library that a host loads
// after this one still fills the table. Every later call reaches the driver
// through this stub, paying a call, the guard on the resolved pointer, and a
// second indirect call, against a driver entry point that costs far more.
template <typename Signature, const char* Name>
struct gl_entry_point;

template <typename Result, typename... Arguments, const char* Name>
struct gl_entry_point<Result (*)(Arguments...), Name> {
  static auto call(Arguments... arguments) -> Result {
    static const auto entry_point =
      reinterpret_cast<Result (*)(Arguments...)>(gl_proc_address(Name));
    return entry_point(arguments...);
  }
};

}  // namespace mln::core::opengl
