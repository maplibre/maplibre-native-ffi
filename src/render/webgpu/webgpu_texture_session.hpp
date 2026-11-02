#pragma once

#include <cstddef>
#include <memory>

#include <mln/util/size.hpp>

#include "maplibre_native_c/texture.h"

namespace mln::core {

class TextureSessionBackend;

// Makes the backend of a texture session that renders into textures it owns
// on the descriptor's device. Runs on the thread whose JavaScript realm holds
// the device.
auto create_webgpu_owned_texture_backend(
  const mln_web_gpu_owned_texture_descriptor& descriptor, mln::Size size,
  std::size_t ring_depth, std::unique_ptr<TextureSessionBackend>& out_backend
) -> mln_status;

}  // namespace mln::core
