#include <memory>

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gfx/offscreen_texture.hpp>
#include <mbgl/mtl/context.hpp>
#include <mbgl/mtl/renderable_resource.hpp>
#include <mbgl/mtl/texture2d.hpp>

#include <Metal/MTLBlitPass.hpp>
#include <Metal/MTLCommandBuffer.hpp>
#include <Metal/MTLCommandQueue.hpp>
#include <Metal/MTLRenderPass.hpp>
#include <TargetConditionals.h>

#include "render/metal/metal_texture_backend.inc"

namespace mln::core {

class MetalTextureBackend::MetalTextureRenderableResource final
    : public mln::mtl::RenderableResource {
 public:
  MetalTextureRenderableResource(
    MetalTextureBackend& backend_, mln::mtl::Context& context_, mln::Size size_,
    MTL::Texture* borrowed_texture_
  )
      : backend(backend_),
        context(context_),
        size(size_),
        borrowedTexture(borrowed_texture_) {
    if (borrowedTexture == nullptr) {
      offscreenTexture = context.createOffscreenTexture(
        size, mln::gfx::TextureChannelDataType::UnsignedByte, true, true
      );
      return;
    }
    depthTexture = context.createTexture2D();
    depthTexture->setSize(size);
    depthTexture->setFormat(
      mln::gfx::TexturePixelType::Depth, mln::gfx::TextureChannelDataType::Float
    );
    depthTexture->setSamplerConfiguration(
      {.filter = mln::gfx::TextureFilterType::Linear,
       .wrapU = mln::gfx::TextureWrapType::Clamp,
       .wrapV = mln::gfx::TextureWrapType::Clamp}
    );
    // The texture was created by mln::mtl::Context above.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    static_cast<mln::mtl::Texture2D*>(depthTexture.get())
      ->setUsage(
        MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite |
        MTL::TextureUsageRenderTarget
      );

#if !TARGET_OS_SIMULATOR
    stencilTexture = context.createTexture2D();
    stencilTexture->setSize(size);
    stencilTexture->setFormat(
      mln::gfx::TexturePixelType::Stencil,
      mln::gfx::TextureChannelDataType::UnsignedByte
    );
    stencilTexture->setSamplerConfiguration(
      {.filter = mln::gfx::TextureFilterType::Linear,
       .wrapU = mln::gfx::TextureWrapType::Clamp,
       .wrapV = mln::gfx::TextureWrapType::Clamp}
    );
    // The texture was created by mln::mtl::Context above.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    static_cast<mln::mtl::Texture2D*>(stencilTexture.get())
      ->setUsage(
        MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite |
        MTL::TextureUsageRenderTarget
      );
#endif

    context.renderingStats().numFrameBuffers++;
  }
  MetalTextureRenderableResource(const MetalTextureRenderableResource&) =
    delete;
  auto operator=(const MetalTextureRenderableResource&)
    -> MetalTextureRenderableResource& = delete;
  MetalTextureRenderableResource(MetalTextureRenderableResource&&) = delete;
  auto operator=(MetalTextureRenderableResource&&)
    -> MetalTextureRenderableResource& = delete;

  ~MetalTextureRenderableResource() override {
    if (borrowedTexture != nullptr) {
      context.renderingStats().numFrameBuffers--;
    }
  }

  void bind() override {
    if (offscreenTexture != nullptr) {
      offscreenTexture->getResource<mln::mtl::RenderableResource>().bind();
      return;
    }

    assert(context.getBackend().getCommandQueue());
    commandBuffer =
      NS::RetainPtr(context.getBackend().getCommandQueue()->commandBuffer());
    renderPassDescriptor =
      NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());
    if (
      auto* colorTarget = renderPassDescriptor->colorAttachments()->object(0)
    ) {
      colorTarget->setTexture(borrowedTexture);
    }
    if (depthTexture != nullptr) {
      depthTexture->create();
      if (auto* depthTarget = renderPassDescriptor->depthAttachment()) {
        depthTarget->setTexture(
          // The texture was created by mln::mtl::Context above.
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
          static_cast<mln::mtl::Texture2D*>(depthTexture.get())
            ->getMetalTexture()
        );
      }
    }
    if (stencilTexture != nullptr) {
      stencilTexture->create();
      if (auto* stencilTarget = renderPassDescriptor->stencilAttachment()) {
        stencilTarget->setTexture(
          // The texture was created by mln::mtl::Context above.
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
          static_cast<mln::mtl::Texture2D*>(stencilTexture.get())
            ->getMetalTexture()
        );
      }
    }
  }

  void swap() override {
    if (offscreenTexture != nullptr) {
      offscreenTexture->getResource<mln::mtl::RenderableResource>().swap();
      return;
    }

    assert(commandBuffer);
    commandBuffer->commit();
    commandBuffer->waitUntilCompleted();
    commandBuffer.reset();
    renderPassDescriptor.reset();
  }

  auto readStillImage() -> mln::PremultipliedImage {
    if (offscreenTexture == nullptr) {
      return {};
    }
    return offscreenTexture->readStillImage();
  }

  auto metal_texture() -> MTL::Texture* {
    if (borrowedTexture != nullptr) {
      return borrowedTexture;
    }
    // Offscreen textures are created by the Metal context for this backend.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    return static_cast<mln::mtl::Texture2D*>(
             offscreenTexture->getTexture().get()
    )
      ->getMetalTexture();
  }

  [[nodiscard]] auto getBackend() const
    -> const mln::mtl::RendererBackend& override {
    return backend;
  }

  [[nodiscard]] auto getCommandBuffer() const
    -> const mln::mtl::MTLCommandBufferPtr& override {
    if (offscreenTexture != nullptr) {
      return offscreenTexture->getResource<mln::mtl::RenderableResource>()
        .getCommandBuffer();
    }
    return commandBuffer;
  }

  [[nodiscard]] auto getUploadPassDescriptor() const
    -> mln::mtl::MTLBlitPassDescriptorPtr override {
    if (offscreenTexture != nullptr) {
      return offscreenTexture->getResource<mln::mtl::RenderableResource>()
        .getUploadPassDescriptor();
    }
    return NS::TransferPtr(MTL::BlitPassDescriptor::alloc()->init());
  }

  [[nodiscard]] auto getRenderPassDescriptor() const
    -> const mln::mtl::MTLRenderPassDescriptorPtr& override {
    if (offscreenTexture != nullptr) {
      return offscreenTexture->getResource<mln::mtl::RenderableResource>()
        .getRenderPassDescriptor();
    }
    assert(renderPassDescriptor);
    return renderPassDescriptor;
  }

 private:
  MetalTextureBackend& backend;
  mln::mtl::Context& context;
  mln::Size size;
  MTL::Texture* borrowedTexture = nullptr;
  std::unique_ptr<mln::gfx::OffscreenTexture> offscreenTexture;
  mln::gfx::Texture2DPtr depthTexture;
  mln::gfx::Texture2DPtr stencilTexture;
  mln::mtl::MTLCommandBufferPtr commandBuffer;
  mln::mtl::MTLRenderPassDescriptorPtr renderPassDescriptor;
};

MetalTextureBackend::MetalTextureBackend(
  MTL::Device* host_device, mln::Size size
)
    : mln::mtl::RendererBackend(mln::gfx::ContextMode::Unique),
      mln::gfx::HeadlessBackend(size) {
  device = NS::RetainPtr(host_device);
  commandQueue = NS::TransferPtr(device->newCommandQueue());
}

MetalTextureBackend::MetalTextureBackend(
  MTL::Texture* borrowed_texture, mln::Size size
)
    : mln::mtl::RendererBackend(mln::gfx::ContextMode::Unique),
      mln::gfx::HeadlessBackend(size),
      borrowed_texture_(borrowed_texture),
      borrowed_pixel_format_(borrowed_texture->pixelFormat()) {
  device = NS::RetainPtr(borrowed_texture->device());
  commandQueue = NS::TransferPtr(device->newCommandQueue());
}

MetalTextureBackend::~MetalTextureBackend() {
  auto guard = mln::gfx::BackendScope{*this};
  resource.reset();
  context.reset();
}

auto MetalTextureBackend::getDefaultRenderable() -> mln::gfx::Renderable& {
  if (!resource) {
    resource = std::make_unique<MetalTextureRenderableResource>(
      // MetalTextureBackend always creates a Metal context.
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
      *this, static_cast<mln::mtl::Context&>(getContext()), size,
      borrowed_texture_
    );
  }
  return *this;
}

auto MetalTextureBackend::readStillImage() -> mln::PremultipliedImage {
  return getResource<MetalTextureRenderableResource>().readStillImage();
}

auto MetalTextureBackend::getRendererBackend() -> mln::gfx::RendererBackend* {
  return this;
}

void MetalTextureBackend::activate() {}

void MetalTextureBackend::deactivate() {}

void MetalTextureBackend::updateAssumedState() {}

auto MetalTextureBackend::metal_texture() -> MTL::Texture* {
  getDefaultRenderable();
  return getResource<MetalTextureRenderableResource>().metal_texture();
}

auto MetalTextureBackend::has_device(const MTL::Device* other) const -> bool {
  return other == device.get();
}

auto MetalTextureBackend::has_borrowed_pixel_format(
  MTL::PixelFormat format
) const -> bool {
  // Compared against the recorded format rather than the outgoing texture,
  // which the session never retained and the host may already have released.
  //
  // mbgl caches render pipeline states under a key that omits the color format,
  // so a replacement in another format would be drawn with a pipeline built for
  // the old one.
  return format == borrowed_pixel_format_;
}

void MetalTextureBackend::set_borrowed_texture(
  MTL::Texture* texture, mln::Size new_size
) {
  borrowed_texture_ = texture;
  size = new_size;
  // Drop the renderable rather than patch it: its depth and stencil textures
  // are sized with the color attachment, and any command buffer in hand was
  // opened against the texture being replaced.
  {
    auto guard = mln::gfx::BackendScope{*this};
    resource.reset();
  }
}

}  // namespace mln::core
