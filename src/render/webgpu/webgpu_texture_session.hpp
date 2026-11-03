#pragma once

#include <memory>

#include <mln/util/size.hpp>

#include "maplibre_native_c/render_target.h"

namespace mln::core {

class TextureSessionBackend;

// Makes the backend of an offscreen session, which renders into one texture
// that it creates on the context's device. Runs on the thread whose JavaScript
// realm holds the device.
auto create_webgpu_offscreen_texture_backend(
  const mln_web_gpu_context_descriptor& context, mln::Size size,
  std::unique_ptr<TextureSessionBackend>& out_backend
) -> mln_status;

}  // namespace mln::core
