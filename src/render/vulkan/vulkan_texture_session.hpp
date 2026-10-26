#pragma once

#include <memory>

#include <mln/util/size.hpp>

#include "maplibre_native_c/texture.h"

namespace mln::core {

class TextureSessionBackend;

// The backend of a session-owned Vulkan texture ring on the descriptor's
// context, for a session whose attach validated that context already.
auto create_vulkan_owned_texture_backend(
  const mln_vulkan_owned_texture_descriptor& descriptor, mln::Size size,
  std::size_t ring_depth
) -> std::unique_ptr<TextureSessionBackend>;

}  // namespace mln::core
