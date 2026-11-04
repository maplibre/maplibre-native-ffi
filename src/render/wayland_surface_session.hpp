#pragma once

#include <memory>

#include "maplibre_native_c.h"

struct wl_display;
struct wl_surface;
struct mln_render_session_object;

namespace mln::core {

// A Wayland surface of the host's. Wayland counts no references, so this holds
// none, and the host keeps the surface until the session lets go of it.
using WaylandWindow = std::shared_ptr<wl_surface>;

// What a Wayland surface attach hands to the build's backend.
struct WaylandSurfaceAttach {
  // The connection of every surface that the session presents to.
  wl_display* display = nullptr;
  WaylandWindow window;
  // The Vulkan device to present with, or null for the shared one.
  std::shared_ptr<const mln_vulkan_context_descriptor> device;
};

// Creates the session's backend for its surface, on the session's driver. Each
// build's render backend defines this.
auto create_wayland_surface_backend(
  WaylandSurfaceAttach attach, mln_render_session_object& target
) -> mln_status;

auto wayland_surface_attach_start(
  mln_map map, const mln_wayland_surface_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;
auto wayland_surface_set_target_start(
  mln_render_session session,
  const mln_wayland_surface_replacement* replacement,
  const mln_completion* completion
) -> mln_status;

}  // namespace mln::core
