#pragma once

#include <cstddef>
#include <memory>

#include <mln/util/size.hpp>

namespace MTL {
class Device;
}

namespace mln::core {

class TextureSessionBackend;

// The backend of a session-owned RGBA8 Metal texture ring on `device`.
auto create_metal_owned_texture_backend(
  MTL::Device* device, mln::Size size, std::size_t ring_depth
) -> std::unique_ptr<TextureSessionBackend>;

}  // namespace mln::core
