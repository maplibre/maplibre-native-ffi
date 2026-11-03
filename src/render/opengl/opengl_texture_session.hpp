#pragma once

#include <memory>

#include <mln/util/size.hpp>

#include "maplibre_native_c/render_target.h"

namespace mln::core {

namespace opengl {
class EglSingleContext;
}

class TextureSessionBackend;

// The backend of an offscreen session, which renders into one texture that it
// creates in a context made from `context`. Writes the backend even when its
// context fails to initialize, so the session tears it down as it would any
// other. A session of a single-context EGL device renders in `single` instead
// of a context of its own.
auto create_opengl_offscreen_texture_backend(
  const mln_open_gl_context_descriptor& context, mln::Size size,
  std::unique_ptr<TextureSessionBackend>& out_backend,
  std::shared_ptr<opengl::EglSingleContext> single = nullptr
) -> mln_status;

}  // namespace mln::core
