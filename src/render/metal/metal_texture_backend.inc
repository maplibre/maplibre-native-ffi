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
  // Whether a replacement texture matches what this session's cached render
  // pipeline states were built against, which is its pixel format and its
  // sample count.
  [[nodiscard]] auto matches_borrowed_texture(const MTL::Texture* texture) const
    -> bool;
  // Renders into a different caller-owned texture from here on. The caller has
  // already established that the format matches.
  void set_borrowed_texture(MTL::Texture* texture, mbgl::Size new_size);

 private:
  MTL::Texture* borrowed_texture_ = nullptr;
  // Recorded when this session took borrowed_texture_, so a replacement is
  // measured against values rather than against a texture the host may already
  // have released.
  MTL::PixelFormat borrowed_pixel_format_ = MTL::PixelFormatInvalid;
  NS::UInteger borrowed_sample_count_ = 1;
};

}  // namespace mln::core
