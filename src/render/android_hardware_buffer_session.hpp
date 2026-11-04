#pragma once

#include <cstdint>

#include "maplibre_native_c.h"

struct mln_render_session_object;

namespace mln::core {

// The ring that an Android hardware buffer attach asks for.
struct AndroidHardwareBufferRing {
  uint32_t buffer_count = 0;
  // The usage bits that the host adds to those that rendering needs.
  uint64_t usage = 0;
};

// Prepares a new session to render into `ring`, on the attaching thread. Each
// build's render backend defines this. The Vulkan build renders with `device`,
// or the shared device when it is null. The EGL build takes only a null
// `device`.
auto configure_android_hardware_buffer_session(
  const AndroidHardwareBufferRing& ring, mln_vulkan_device device,
  mln_render_session_object& session
) -> mln_status;

auto android_hardware_buffer_attach_start(
  mln_map map, const mln_android_hardware_buffer_ring_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;

}  // namespace mln::core
