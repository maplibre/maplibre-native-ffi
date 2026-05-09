// opengl_support.cpp — OpenGL backend support utilities for
// maplibre-native-ffi.
//
// Provides functions that the C API layer calls into but that have no
// backend-specific implementation (e.g. the render-backend capability mask).
// Compiled only when MLN_FFI_RENDER_BACKEND=opengl.

#include "maplibre_native_c.h"
#include "render/texture_session.hpp"

namespace mln::core {

auto supported_render_backend_mask() noexcept -> uint32_t {
  return MLN_RENDER_BACKEND_FLAG_OPENGL;
}

}  // namespace mln::core
