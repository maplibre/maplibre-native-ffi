#pragma once

#include <memory>

#include "maplibre_native_c.h"

// OpenHarmony's OHNativeWindow.
struct NativeWindow;
struct mln_render_session_object;

namespace mln::core {

// An OpenHarmony window that the library holds a reference to, which it
// releases with the last copy.
using OhosWindow = std::shared_ptr<NativeWindow>;

// What an OpenHarmony surface attach hands to the build's backend.
struct OhosSurfaceAttach {
  OhosWindow window;
  // The Vulkan device to present with, or null for the shared one.
  std::shared_ptr<const mln_vulkan_context_descriptor> device;
};

// Creates the session's backend for its window, on the session's driver. Each
// build's render backend defines this.
auto create_ohos_surface_backend(
  OhosSurfaceAttach attach, mln_render_session_object& target
) -> mln_status;

auto ohos_surface_attach_start(
  mln_map map, const mln_ohos_surface_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;
auto ohos_surface_set_target_start(
  mln_render_session session, const mln_ohos_surface_replacement* replacement,
  const mln_completion* completion
) -> mln_status;

}  // namespace mln::core
