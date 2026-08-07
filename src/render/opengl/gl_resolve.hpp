#pragma once

// Looks up a GL entry point in the client library, for the generated table in
// mbgl::platform. Resolution needs no current context, so an entry point can
// resolve before a display exists.

#include <type_traits>

#include "render/opengl/egl_common.hpp"

#if !defined(__linux__) && !defined(__APPLE__)
// Without dlopen the resolver returns null for every entry point. Platforms
// without one link their loader instead of generating this table.
#error "the generated GL table needs a POSIX dynamic loader"
#endif

namespace mln::core::opengl {

inline auto gl_proc_address(const char* name) -> void* {
  return get_egl_client_library_proc_address(name, EGL_OPENGL_ES_API);
}

// One entry of the generated table. A shared library runs the table's
// initializers as it loads, which can precede the host loading its client
// library, so each stub resolves its own entry point on the first call.
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
