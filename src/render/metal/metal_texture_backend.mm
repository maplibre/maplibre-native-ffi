#include <cstddef>
#include <memory>
#include <new>
#include <utility>
#include <vector>

#include <mln/gfx/backend_scope.hpp>
#include <mln/mtl/context.hpp>
#include <mln/mtl/renderable_resource.hpp>
#include <mln/mtl/texture2d.hpp>

#include <Metal/MTLBlitPass.hpp>
#include <Metal/MTLCommandBuffer.hpp>
#include <Metal/MTLCommandQueue.hpp>
#include <Metal/MTLRenderPass.hpp>
#include <Metal/MTLTexture.hpp>
#include <TargetConditionals.h>

#include "render/metal/metal_texture_backend.inc"

namespace mln::core {

class MetalTextureBackend::MetalTextureRenderableResource final
    : public mln::mtl::RenderableResource {
 public:
  MetalTextureRenderableResource(
    MetalTextureBackend& backend_, mln::mtl::Context& context_, mln::Size size_,
    MTL::Texture* borrowed_texture_, MTL::PixelFormat owned_pixel_format_
  )
      : backend(backend_),
        context(context_),
        size(size_),
        borrowedTexture(borrowed_texture_),
        ownedPixelFormat(owned_pixel_format_) {
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
    context.renderingStats().numFrameBuffers--;
  }

  void bind() override {
    // mbgl's offscreen texture is RGBA8 only, so a session-owned ring makes
    // its own color texture, on the first draw as mbgl's would.
    if (borrowedTexture == nullptr && !ownedTexture) {
      auto descriptor = NS::RetainPtr(
        MTL::TextureDescriptor::texture2DDescriptor(
          ownedPixelFormat, size.width, size.height, false
        )
      );
      // No shader writes, which would cost lossless compression on Apple GPUs.
      descriptor->setUsage(
        MTL::TextureUsageShaderRead | MTL::TextureUsageRenderTarget
      );
      ownedTexture = context.createMetalTexture(std::move(descriptor));
      if (!ownedTexture) {
        throw std::bad_alloc();
      }
    }

    assert(context.getBackend().getCommandQueue());
    commandBuffer =
      NS::RetainPtr(context.getBackend().getCommandQueue()->commandBuffer());
    renderPassDescriptor =
      NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());
    if (
      auto* colorTarget = renderPassDescriptor->colorAttachments()->object(0)
    ) {
      colorTarget->setTexture(metal_texture());
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
    assert(commandBuffer);
    commandBuffer->commit();
    commandBuffer->waitUntilCompleted();
    commandBuffer.reset();
    renderPassDescriptor.reset();
  }

  auto readStillImage() -> mln::PremultipliedImage {
    if (borrowedTexture != nullptr || !ownedTexture) {
      return {};
    }
    auto image = mln::PremultipliedImage{size};
    ownedTexture->getBytes(
      image.data.get(), static_cast<NS::UInteger>(size.width) * 4,
      MTL::Region::Make2D(0, 0, size.width, size.height), 0
    );
    // Readback is RGBA8 in either of the ring's formats.
    if (ownedPixelFormat == MTL::PixelFormatBGRA8Unorm) {
      for (std::size_t offset = 0; offset < image.bytes(); offset += 4) {
        std::swap(image.data[offset], image.data[offset + 2]);
      }
    }
    return image;
  }

  auto metal_texture() -> MTL::Texture* {
    if (borrowedTexture != nullptr) {
      return borrowedTexture;
    }
    return ownedTexture.get();
  }

  [[nodiscard]] auto getBackend() const
    -> const mln::mtl::RendererBackend& override {
    return backend;
  }

  [[nodiscard]] auto getCommandBuffer() const
    -> const mln::mtl::MTLCommandBufferPtr& override {
    return commandBuffer;
  }

  [[nodiscard]] auto getUploadPassDescriptor() const
    -> mln::mtl::MTLBlitPassDescriptorPtr override {
    return NS::TransferPtr(MTL::BlitPassDescriptor::alloc()->init());
  }

  [[nodiscard]] auto getRenderPassDescriptor() const
    -> const mln::mtl::MTLRenderPassDescriptorPtr& override {
    assert(renderPassDescriptor);
    return renderPassDescriptor;
  }

 private:
  MetalTextureBackend& backend;
  mln::mtl::Context& context;
  mln::Size size;
  MTL::Texture* borrowedTexture = nullptr;
  MTL::PixelFormat ownedPixelFormat;
  mln::mtl::MTLTexturePtr ownedTexture;
  mln::gfx::Texture2DPtr depthTexture;
  mln::gfx::Texture2DPtr stencilTexture;
  mln::mtl::MTLCommandBufferPtr commandBuffer;
  mln::mtl::MTLRenderPassDescriptorPtr renderPassDescriptor;
};

MetalTextureBackend::MetalTextureBackend(
  MTL::Device* host_device, mln::Size size, std::size_t ring_depth,
  MTL::PixelFormat pixel_format
)
    : mln::mtl::RendererBackend(mln::gfx::ContextMode::Unique),
      mln::gfx::HeadlessBackend(size),
      owned_pixel_format_(pixel_format),
      ring_(ring_depth) {
  device = NS::RetainPtr(host_device);
  commandQueue = NS::TransferPtr(device->newCommandQueue());
}

MetalTextureBackend::MetalTextureBackend(
  std::vector<MTL::Texture*> borrowed_textures, mln::Size size
)
    : mln::mtl::RendererBackend(mln::gfx::ContextMode::Unique),
      mln::gfx::HeadlessBackend(size),
      borrowed_textures_(std::move(borrowed_textures)),
      borrowed_pixel_format_(borrowed_textures_.front()->pixelFormat()),
      ring_(borrowed_textures_.size()) {
  device = NS::RetainPtr(borrowed_textures_.front()->device());
  commandQueue = NS::TransferPtr(device->newCommandQueue());
}

MetalTextureBackend::~MetalTextureBackend() {
  auto guard = mln::gfx::BackendScope{*this};
  resource.reset();
  ring_.clear();
  context.reset();
}

auto MetalTextureBackend::getDefaultRenderable() -> mln::gfx::Renderable& {
  if (!resource) {
    resource = std::make_unique<MetalTextureRenderableResource>(
      // MetalTextureBackend always creates a Metal context.
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
      *this, static_cast<mln::mtl::Context&>(getContext()), getSize(),
      borrowed_textures_.empty() ? nullptr
                                 : borrowed_textures_[ring_.selected()],
      owned_pixel_format_
    );
    // Recorded with the resource it describes, so a slot that keeps an older
    // resource keeps the size that resource was built for.
    ring_.record_size(getSize());
  }
  return *this;
}

auto MetalTextureBackend::readStillImage() -> mln::PremultipliedImage {
  // A render that aborted before it asked for the renderable leaves no
  // resource behind, so build it here rather than dereference nothing.
  getDefaultRenderable();
  return getResource<MetalTextureRenderableResource>().readStillImage();
}

auto MetalTextureBackend::getRendererBackend() -> mln::gfx::RendererBackend* {
  return this;
}

void MetalTextureBackend::activate() {}

void MetalTextureBackend::deactivate() {}

auto MetalTextureBackend::metal_texture() -> MTL::Texture* {
  getDefaultRenderable();
  return getResource<MetalTextureRenderableResource>().metal_texture();
}

auto MetalTextureBackend::select_slot(std::size_t slot) -> bool {
  return ring_.select(slot, getSize(), resource);
}

void MetalTextureBackend::set_ring_size(mln::Size new_size) {
  setRenderableSize(new_size);
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

void MetalTextureBackend::set_borrowed_textures(
  std::vector<MTL::Texture*> textures, mln::Size new_size
) {
  borrowed_textures_ = std::move(textures);
  setRenderableSize(new_size);
  // Drop every slot's renderable rather than patch it: its depth and stencil
  // textures are sized with the color attachment, and any command buffer in
  // hand was opened against the texture being replaced. Each slot rebuilds
  // over its new texture when it is next selected.
  {
    auto guard = mln::gfx::BackendScope{*this};
    resource.reset();
    ring_.reset(borrowed_textures_.size());
  }
}

}  // namespace mln::core
