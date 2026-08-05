#pragma once

// Looks up a GL entry point in the client library, for the generated table in
// mbgl::platform. Resolution needs no current context, so the table can
// initialize before a display exists.

#include <type_traits>

#include "render/opengl/egl_common.hpp"

#if !defined(__linux__)
// The resolver below only opens a Linux GL client library and returns null
// elsewhere, leaving the whole table null. Other platforms link their loader.
#error "the generated GL table is Linux only"
#endif

namespace mln::core::opengl {

inline auto gl_proc_address(const char* name) -> void* {
  return get_egl_client_library_proc_address(name, EGL_OPENGL_ES_API);
}

}  // namespace mln::core::opengl
