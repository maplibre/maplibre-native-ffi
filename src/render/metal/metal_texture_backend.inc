#pragma once

#include <vector>

#include <mln/gfx/headless_backend.hpp>
#include <mln/mtl/headless_backend.hpp>

#include <Metal/MTLDevice.hpp>
#include <Metal/MTLTexture.hpp>

#include "render/render_session_common.hpp"

namespace mln::core {

class MetalTextureBackend final : public mln::mtl::RendererBackend,
                                  public mln::gfx::HeadlessBackend {
 private:
  class MetalTextureRenderableResource;

 public:
  MetalTextureBackend(
    MTL::Device* host_device, mln::Size size, std::size_t ring_depth
  );
  // Renders slot i into borrowed_textures[i], which share one device and one
  // pixel format.
  MetalTextureBackend(
    std::vector<MTL::Texture*> borrowed_textures, mln::Size size
  );
  MetalTextureBackend(const MetalTextureBackend&) = delete;
  auto operator=(const MetalTextureBackend&) -> MetalTextureBackend& = delete;
  MetalTextureBackend(MetalTextureBackend&&) = delete;
  auto operator=(MetalTextureBackend&&) -> MetalTextureBackend& = delete;
  ~MetalTextureBackend() override;

  auto getDefaultRenderable() -> mln::gfx::Renderable& override;
  auto readStillImage() -> mln::PremultipliedImage override;
  auto getRendererBackend() -> mln::gfx::RendererBackend* override;
  void activate() override;
  void deactivate() override;

  // The texture of the slot this backend is rendering into.
  auto metal_texture() -> MTL::Texture*;
  auto select_slot(std::size_t slot) -> bool;
  void set_ring_size(mln::Size size);

  [[nodiscard]] auto has_device(const MTL::Device* other) const -> bool;
  // Whether a replacement texture has the pixel format this session's cached
  // render pipeline states were built against.
  [[nodiscard]] auto has_borrowed_pixel_format(MTL::PixelFormat format) const
    -> bool;
  // Renders slot i into textures[i] from here on, one texture per slot. The
  // caller has already established that the device and format match.
  void set_borrowed_textures(
    std::vector<MTL::Texture*> textures, mln::Size new_size
  );

 private:
  // The caller's texture for each slot, or empty for a session-owned ring.
  std::vector<MTL::Texture*> borrowed_textures_;
  // Recorded when this session took borrowed_textures_, which it does not
  // retain and the host may already have released.
  MTL::PixelFormat borrowed_pixel_format_ = MTL::PixelFormatInvalid;
  RenderableSlotRing ring_;
};

}  // namespace mln::core
