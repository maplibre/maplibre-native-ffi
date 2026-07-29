#pragma once

// Looks up a GL entry point in the client library, for the generated table in
// mbgl::platform. Resolution goes through the same helpers the EGL context uses
// and needs no current context, so the table can initialize before a display
// exists.

#include <type_traits>

#include "render/opengl/egl_common.hpp"

#if !defined(__linux__)
// The resolver below only knows how to open a Linux GL client library, and it
// returns null everywhere else, which would leave the whole table null and fail
// on the first GL call. Other platforms link their loader instead.
#error "the generated GL table is Linux only"
#endif

namespace mln::core::opengl {

inline auto gl_proc_address(const char* name) -> void* {
  return get_egl_client_library_proc_address(name, EGL_OPENGL_ES_API);
}

}  // namespace mln::core::opengl
