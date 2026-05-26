#pragma once

#if defined(_WIN32)
#include <cstdint>
#include <limits>
#include <stdexcept>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <Windows.h>

namespace mln::core::opengl {

using WglCreateContextAttribs = HGLRC(WINAPI*)(HDC, HGLRC, const int*);

inline constexpr auto wgl_context_major_version_arb = 0x2091;
inline constexpr auto wgl_context_minor_version_arb = 0x2092;
inline constexpr auto wgl_context_profile_mask_arb = 0x9126;
inline constexpr auto wgl_context_compatibility_profile_bit_arb = 0x00000002;

inline auto is_valid_wgl_proc_address(PROC proc) -> bool {
  const auto address = reinterpret_cast<std::uintptr_t>(proc);
  return proc != nullptr && address > 3 &&
         address != std::numeric_limits<std::uintptr_t>::max();
}

inline auto create_shared_wgl_context(
  HDC device_context, HGLRC share_context, HGLRC previous_render_context,
  WglCreateContextAttribs context_attribs
) -> HGLRC {
  auto render_context = HGLRC{};
  if (context_attribs != nullptr) {
    const int attributes[] = {
      wgl_context_major_version_arb,
      3,
      wgl_context_minor_version_arb,
      0,
      wgl_context_profile_mask_arb,
      wgl_context_compatibility_profile_bit_arb,
      0
    };
    render_context = context_attribs(device_context, share_context, attributes);
  }
  if (render_context != nullptr) {
    return render_context;
  }

  render_context = wglCreateContext(device_context);
  if (render_context == nullptr) {
    throw std::runtime_error("Creating OpenGL WGL context failed");
  }

  const auto share_context_was_current =
    previous_render_context == share_context;
  if (share_context_was_current && wglMakeCurrent(nullptr, nullptr) == 0) {
    wglDeleteContext(render_context);
    throw std::runtime_error("Releasing current WGL context failed");
  }
  if (wglShareLists(share_context, render_context) == 0) {
    if (share_context_was_current) {
      (void)wglMakeCurrent(device_context, share_context);
    }
    wglDeleteContext(render_context);
    throw std::runtime_error("Sharing OpenGL WGL context failed");
  }

  return render_context;
}

}  // namespace mln::core::opengl
#endif
