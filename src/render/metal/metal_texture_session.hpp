#pragma once

#include <memory>

#include <mln/util/size.hpp>

namespace MTL {
class Device;
}

namespace mln::core {

class TextureSessionBackend;

// The backend of an offscreen session, which renders into one RGBA8 texture
// that it creates on `device`.
auto create_metal_offscreen_texture_backend(MTL::Device* device, mln::Size size)
  -> std::unique_ptr<TextureSessionBackend>;

}  // namespace mln::core
