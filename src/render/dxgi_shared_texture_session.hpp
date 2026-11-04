#pragma once

#include <cstdint>

#include "maplibre_native_c.h"

struct mln_render_session_object;

namespace mln::core {

// The DXGI_FORMAT of every texture in a DXGI shared texture ring.
inline constexpr uint32_t dxgi_format_b8g8r8a8_unorm = 87;

// The ring that a DXGI shared texture attach asks for.
struct DxgiSharedTextureRing {
  uint32_t texture_count = 0;
};

// Prepares a new session to render into `ring`, on the attaching thread. Each
// build's render backend defines this. The Vulkan build renders with `device`,
// or a device of the library's when it is null. The WGL build renders into no
// DXGI shared textures.
auto configure_dxgi_shared_texture_session(
  const DxgiSharedTextureRing& ring, mln_vulkan_device device,
  mln_render_session_object& session
) -> mln_status;

auto dxgi_shared_texture_attach_start(
  mln_map map, const mln_dxgi_shared_texture_ring_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;

}  // namespace mln::core
