#pragma once

#include <memory>

#include "maplibre_native_c.h"

struct ANativeWindow;
struct mln_render_session_object;

namespace mln::core {

// An Android window that the library holds a reference to, which it releases
// with the last copy.
using AndroidWindow = std::shared_ptr<ANativeWindow>;

// What an Android surface attach hands to the build's backend.
struct AndroidSurfaceAttach {
  AndroidWindow window;
  // The Vulkan device to present with, or null for the shared one.
  std::shared_ptr<const mln_vulkan_context_descriptor> device;
};

// Creates the session's backend for its window, on the session's driver. The
// build's render backend defines it: the Vulkan one presents through a
// VkSurfaceKHR, and the EGL one through an EGL window surface.
auto create_android_surface_backend(
  AndroidSurfaceAttach attach, mln_render_session_object& target
) -> mln_status;

auto android_surface_attach_start(
  mln_map map, const mln_android_surface_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;
auto android_surface_set_target_start(
  mln_render_session session,
  const mln_android_surface_replacement* replacement,
  const mln_completion* completion
) -> mln_status;

}  // namespace mln::core
