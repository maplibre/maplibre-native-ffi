#pragma once

#include <mln/gfx/headless_backend.hpp>
#include <mln/mtl/headless_backend.hpp>

#include <Metal/MTLDevice.hpp>
#include <Metal/MTLTexture.hpp>

namespace mln::core {

class MetalTextureBackend final : public mln::mtl::RendererBackend,
                                  public mln::gfx::HeadlessBackend {
 private:
  class MetalTextureRenderableResource;

 public:
  MetalTextureBackend(MTL::Device* host_device, mln::Size size);
  MetalTextureBackend(MTL::Texture* borrowed_texture, mln::Size size);
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
  void updateAssumedState() override;

  auto metal_texture() -> MTL::Texture*;

  [[nodiscard]] auto has_device(const MTL::Device* other) const -> bool;
  // Whether a replacement texture has the pixel format this session's cached
  // render pipeline states were built against.
  [[nodiscard]] auto has_borrowed_pixel_format(MTL::PixelFormat format) const
    -> bool;
  // Renders into a different caller-owned texture from here on. The caller has
  // already established that the format matches.
  void set_borrowed_texture(MTL::Texture* texture, mln::Size new_size);

 private:
  MTL::Texture* borrowed_texture_ = nullptr;
  // Recorded when this session took borrowed_texture_, which it does not
  // retain and the host may already have released.
  MTL::PixelFormat borrowed_pixel_format_ = MTL::PixelFormatInvalid;
};

}  // namespace mln::core
