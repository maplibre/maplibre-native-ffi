#pragma once

#include <memory>

#include "maplibre_native_c.h"

struct HWND__;
struct mln_render_session_object;

namespace mln::core {

// A Win32 window of the host's. This holds no reference, and the host keeps the
// window until the session lets go of it.
using Win32Window = std::shared_ptr<HWND__>;

// What a Win32 window attach hands to the build's backend.
struct Win32WindowAttach {
  Win32Window window;
  // The Vulkan device to present with, or null for the shared one.
  std::shared_ptr<const mln_vulkan_context_descriptor> device;
};

// Whether the window has a client area to present to, which a minimized window
// lacks.
auto win32_window_ready(HWND__* window) -> bool;

// Creates the session's backend for its window, on the session's driver. Each
// build's render backend defines this.
auto create_win32_window_backend(
  Win32WindowAttach attach, mln_render_session_object& target
) -> mln_status;

auto win32_window_attach_start(
  mln_map map, const mln_win32_window_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;
auto win32_window_set_target_start(
  mln_render_session session, const mln_win32_window_replacement* replacement,
  const mln_completion* completion
) -> mln_status;

}  // namespace mln::core
