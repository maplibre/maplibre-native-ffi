#pragma once

// The context mode every OpenGL session runs its renderer in, whether it draws
// into a surface or a texture.

#include <mbgl/gfx/renderer_backend.hpp>

namespace mln::core::opengl {

// A browser session renders into the host's own context, so MapLibre must treat
// its cached GL state as stale each frame. Every other provider gets a session
// context of its own inside the host's share group, and nothing else makes that
// context current, so MapLibre keeps its cached state across frames and clears
// each frame to the style's background color.
#if defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
constexpr auto session_context_mode = mbgl::gfx::ContextMode::Shared;
#else
constexpr auto session_context_mode = mbgl::gfx::ContextMode::Unique;
#endif

}  // namespace mln::core::opengl
