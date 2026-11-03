#pragma once

#include <memory>

#include <mln/util/size.hpp>

#include "maplibre_native_c/render_target.h"

namespace mln::core {

class TextureSessionBackend;

// The backend of an offscreen session, which renders into one image that it
// creates on `context`.
auto create_vulkan_offscreen_texture_backend(
  const mln_vulkan_context_descriptor& context, mln::Size size
) -> std::unique_ptr<TextureSessionBackend>;

}  // namespace mln::core
