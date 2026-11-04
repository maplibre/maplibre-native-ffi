#pragma once

#include <memory>

#include <mln/util/size.hpp>

#include "maplibre_native_c.h"

namespace MTL {
class Device;
}

namespace mln::core {

class TextureSessionBackend;

// The backend of an offscreen session, which renders into one RGBA8 texture
// that it creates on `device`.
auto create_metal_offscreen_texture_backend(MTL::Device* device, mln::Size size)
  -> std::unique_ptr<TextureSessionBackend>;

auto io_surface_ring_attach_start(
  mln_map map, const mln_io_surface_ring_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;

}  // namespace mln::core
