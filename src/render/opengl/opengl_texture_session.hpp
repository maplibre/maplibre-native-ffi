#pragma once

#include <cstddef>
#include <memory>

#include <mln/util/size.hpp>

#include "maplibre_native_c/texture.h"

namespace mln::core {

class TextureSessionBackend;

// The backend of a session-owned OpenGL texture ring on the descriptor's
// context, for a session whose attach validated that context already. Writes
// the backend even when its context fails to initialize, so the session tears
// it down as it would any other.
auto create_opengl_owned_texture_backend(
  const mln_open_gl_owned_texture_descriptor& descriptor, mln::Size size,
  std::size_t ring_depth, std::unique_ptr<TextureSessionBackend>& out_backend
) -> mln_status;

}  // namespace mln::core
