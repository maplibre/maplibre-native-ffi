#pragma once

#include <mbgl/gfx/headless_backend.hpp>
#include <mbgl/mtl/headless_backend.hpp>

#include <Metal/MTLDevice.hpp>
#include <Metal/MTLTexture.hpp>

namespace mln::core {

class MetalTextureBackend final : public mbgl::mtl::RendererBackend,
                                  public mbgl::gfx::HeadlessBackend {
 private:
  class MetalTextureRenderableResource;

 public:
  MetalTextureBackend(MTL::Device* host_device, mbgl::Size size);
  MetalTextureBackend(MTL::Texture* borrowed_texture, mbgl::Size size);
  MetalTextureBackend(const MetalTextureBackend&) = delete;
  auto operator=(const MetalTextureBackend&) -> MetalTextureBackend& = delete;
  MetalTextureBackend(MetalTextureBackend&&) = delete;
  auto operator=(MetalTextureBackend&&) -> MetalTextureBackend& = delete;
  ~MetalTextureBackend() override;

  auto getDefaultRenderable() -> mbgl::gfx::Renderable& override;
  auto readStillImage() -> mbgl::PremultipliedImage override;
  auto getRendererBackend() -> mbgl::gfx::RendererBackend* override;
  void activate() override;
  void deactivate() override;
  void updateAssumedState() override;

  auto metal_texture() -> MTL::Texture*;

  [[nodiscard]] auto has_device(const MTL::Device* other) const -> bool;
  // Whether a replacement texture matches the format this session's cached
  // render pipeline states were built against.
  [[nodiscard]] auto has_borrowed_pixel_format(MTL::PixelFormat format) const
    -> bool;
  // Renders into a different caller-owned texture from here on. The caller has
  // already established that the format matches.
  void set_borrowed_texture(MTL::Texture* texture, mbgl::Size new_size);

 private:
  MTL::Texture* borrowed_texture_ = nullptr;
  // The format of borrowed_texture_, recorded when this session took it, so a
  // replacement is measured against a value rather than against a texture the
  // host may already have released.
  MTL::PixelFormat borrowed_pixel_format_ = MTL::PixelFormatInvalid;
};

}  // namespace mln::core
