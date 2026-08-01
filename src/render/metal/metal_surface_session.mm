#include <memory>
#include <stdexcept>

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/mtl/context.hpp>
#include <mbgl/mtl/renderable_resource.hpp>
#include <mbgl/mtl/renderer_backend.hpp>
#include <mbgl/mtl/texture2d.hpp>
#include <mbgl/util/size.hpp>

#include <Foundation/NSSharedPtr.hpp>
#include <Metal/MTLBlitPass.hpp>
#include <Metal/MTLCommandBuffer.hpp>
#include <Metal/MTLCommandQueue.hpp>
#include <Metal/MTLDevice.hpp>
#include <Metal/MTLPixelFormat.hpp>
#include <Metal/MTLRenderPass.hpp>
#include <QuartzCore/CAMetalDrawable.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "render/render_session_common.hpp"
#include "render/surface_session.hpp"

namespace {

class MetalSurfaceBackend final : public mbgl::mtl::RendererBackend,
                                  public mbgl::gfx::Renderable {
 private:
  class MetalSurfaceRenderableResource final
      : public mbgl::mtl::RenderableResource {
   public:
    MetalSurfaceRenderableResource(
      MetalSurfaceBackend& backend_, CA::MetalLayer* layer_, mbgl::Size size_
    )
        : backend(backend_), layer(NS::RetainPtr(layer_)) {
      layer->setDevice(backend.getDevice().get());
      layer->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
      layer->setFramebufferOnly(false);
      setSize(size_);
    }
    MetalSurfaceRenderableResource(const MetalSurfaceRenderableResource&) =
      delete;
    auto operator=(const MetalSurfaceRenderableResource&)
      -> MetalSurfaceRenderableResource& = delete;
    MetalSurfaceRenderableResource(MetalSurfaceRenderableResource&&) = delete;
    auto operator=(MetalSurfaceRenderableResource&&)
      -> MetalSurfaceRenderableResource& = delete;
    ~MetalSurfaceRenderableResource() override = default;

    void setSize(mbgl::Size size_) {
      size = size_;
      layer->setDrawableSize(CGSizeMake(
        static_cast<CGFloat>(size.width), static_cast<CGFloat>(size.height)
      ));
      depthStencilDirty = true;
    }

    // Presents through a different layer from here on. Every layer this session
    // takes is configured for the same device and pixel format, so the render
    // pipeline states mbgl caches against that format stay usable and the
    // renderer keeps its resources.
    void set_layer(CA::MetalLayer* layer_, mbgl::Size size_) {
      // Release what is still bound to the outgoing layer. A drawable outlives
      // its layer badly, and bind() acquires one from the new layer anyway.
      commandBuffer.reset();
      renderPassDescriptor.reset();
      drawable.reset();

      layer = NS::RetainPtr(layer_);
      layer->setDevice(backend.getDevice().get());
      layer->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
      layer->setFramebufferOnly(false);
      setSize(size_);
    }

    void bind() override {
      if (drawable && commandBuffer && renderPassDescriptor) {
        return;
      }

      auto* next_drawable = layer->nextDrawable();
      if (next_drawable == nullptr) {
        throw std::runtime_error("Metal surface did not provide a drawable");
      }
      drawable = NS::RetainPtr(next_drawable);

      commandBuffer = NS::RetainPtr(backend.getCommandQueue()->commandBuffer());
      renderPassDescriptor =
        NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());
      renderPassDescriptor->colorAttachments()->object(0)->setTexture(
        drawable->texture()
      );

      if (depthStencilDirty || !depthTexture || !stencilTexture) {
        depthStencilDirty = false;
        // Metal renderables are always attached to a Metal backend/context.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        auto& context = static_cast<mbgl::mtl::Context&>(backend.getContext());
        depthTexture = context.createTexture2D();
        depthTexture->setSize(size);
        depthTexture->setFormat(
          mbgl::gfx::TexturePixelType::Depth,
          mbgl::gfx::TextureChannelDataType::Float
        );
        depthTexture->setSamplerConfiguration(
          {.filter = mbgl::gfx::TextureFilterType::Linear,
           .wrapU = mbgl::gfx::TextureWrapType::Clamp,
           .wrapV = mbgl::gfx::TextureWrapType::Clamp}
        );
        // The texture was created by mbgl::mtl::Context above.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        static_cast<mbgl::mtl::Texture2D*>(depthTexture.get())
          ->setUsage(
            MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite |
            MTL::TextureUsageRenderTarget
          );

        stencilTexture = context.createTexture2D();
        stencilTexture->setSize(size);
        stencilTexture->setFormat(
          mbgl::gfx::TexturePixelType::Stencil,
          mbgl::gfx::TextureChannelDataType::UnsignedByte
        );
        stencilTexture->setSamplerConfiguration(
          {.filter = mbgl::gfx::TextureFilterType::Linear,
           .wrapU = mbgl::gfx::TextureWrapType::Clamp,
           .wrapV = mbgl::gfx::TextureWrapType::Clamp}
        );
        // The texture was created by mbgl::mtl::Context above.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        static_cast<mbgl::mtl::Texture2D*>(stencilTexture.get())
          ->setUsage(
            MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite |
            MTL::TextureUsageRenderTarget
          );
      }

      depthTexture->create();
      if (auto* depthTarget = renderPassDescriptor->depthAttachment()) {
        depthTarget->setTexture(
          // The texture was created by mbgl::mtl::Context above.
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
          static_cast<mbgl::mtl::Texture2D*>(depthTexture.get())
            ->getMetalTexture()
        );
      }
      stencilTexture->create();
      if (auto* stencilTarget = renderPassDescriptor->stencilAttachment()) {
        stencilTarget->setTexture(
          // The texture was created by mbgl::mtl::Context above.
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
          static_cast<mbgl::mtl::Texture2D*>(stencilTexture.get())
            ->getMetalTexture()
        );
      }
    }

    void swap() override {
      commandBuffer->presentDrawable(drawable.get());
      commandBuffer->commit();
      commandBuffer.reset();
      drawable.reset();
      renderPassDescriptor.reset();
    }

    [[nodiscard]] auto getBackend() const
      -> const mbgl::mtl::RendererBackend& override {
      return backend;
    }

    [[nodiscard]] auto getCommandBuffer() const
      -> const mbgl::mtl::MTLCommandBufferPtr& override {
      return commandBuffer;
    }

    [[nodiscard]] auto getUploadPassDescriptor() const
      -> mbgl::mtl::MTLBlitPassDescriptorPtr override {
      return NS::TransferPtr(MTL::BlitPassDescriptor::alloc()->init());
    }

    [[nodiscard]] auto getRenderPassDescriptor() const
      -> const mbgl::mtl::MTLRenderPassDescriptorPtr& override {
      return renderPassDescriptor;
    }

   private:
    MetalSurfaceBackend& backend;
    NS::SharedPtr<CA::MetalLayer> layer;
    mbgl::Size size{0, 0};
    mbgl::mtl::MTLCommandBufferPtr commandBuffer;
    mbgl::mtl::MTLRenderPassDescriptorPtr renderPassDescriptor;
    mbgl::mtl::CAMetalDrawablePtr drawable;
    mbgl::gfx::Texture2DPtr depthTexture;
    mbgl::gfx::Texture2DPtr stencilTexture;
    bool depthStencilDirty = true;
  };

 public:
  MetalSurfaceBackend(
    CA::MetalLayer* layer, MTL::Device* host_device, mbgl::Size size_
  )
      : mbgl::mtl::RendererBackend(mbgl::gfx::ContextMode::Unique),
        mbgl::gfx::Renderable(size_, nullptr) {
    if (host_device != nullptr) {
      device = NS::RetainPtr(host_device);
      commandQueue = NS::TransferPtr(device->newCommandQueue());
    }
    setResource(
      std::make_unique<MetalSurfaceRenderableResource>(*this, layer, size_)
    );
  }
  MetalSurfaceBackend(const MetalSurfaceBackend&) = delete;
  auto operator=(const MetalSurfaceBackend&) -> MetalSurfaceBackend& = delete;
  MetalSurfaceBackend(MetalSurfaceBackend&&) = delete;
  auto operator=(MetalSurfaceBackend&&) -> MetalSurfaceBackend& = delete;

  ~MetalSurfaceBackend() override {
    auto guard = mbgl::gfx::BackendScope{*this};
    resource.reset();
    context.reset();
  }

  auto getDefaultRenderable() -> mbgl::gfx::Renderable& override {
    return *this;
  }

  void setSize(mbgl::Size size_) {
    size = size_;
    getResource<MetalSurfaceRenderableResource>().setSize(size_);
  }

  void set_layer(CA::MetalLayer* layer_, mbgl::Size size_) {
    size = size_;
    getResource<MetalSurfaceRenderableResource>().set_layer(layer_, size_);
  }

  // A null device in a descriptor names no device at all, which every session
  // satisfies; the session keeps the one it attached with either way.
  [[nodiscard]] auto has_device(MTL::Device* other) const -> bool {
    return other == nullptr || other == device.get();
  }

  void activate() override {}
  void deactivate() override {}
  void updateAssumedState() override {}
};

class MetalSurfaceSessionBackend final
    : public mln::core::SurfaceSessionBackend {
 public:
  MetalSurfaceSessionBackend(
    CA::MetalLayer* layer, MTL::Device* host_device, mbgl::Size size
  )
      : backend_(layer, host_device, size) {}

  auto renderer_backend() -> mbgl::gfx::RendererBackend& override {
    return backend_;
  }

  void resize(uint32_t physical_width, uint32_t physical_height) override {
    backend_.setSize(mbgl::Size{physical_width, physical_height});
  }

  auto set_metal_target(const mln_metal_surface_descriptor& descriptor)
    -> mln_status override {
    if (!backend_.has_device(
          static_cast<MTL::Device*>(descriptor.context.device)
        )) {
      mln::core::set_thread_error(
        "Metal surface target must name the device this session attached with"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    backend_.set_layer(
      static_cast<CA::MetalLayer*>(descriptor.layer),
      mbgl::Size{
        mln::core::physical_dimension(
          descriptor.extent.width, descriptor.extent.scale_factor
        ),
        mln::core::physical_dimension(
          descriptor.extent.height, descriptor.extent.scale_factor
        )
      }
    );
    return MLN_STATUS_OK;
  }

 private:
  MetalSurfaceBackend backend_;
};

}  // namespace

namespace mln::core {

auto metal_surface_attach(
  mln_map map, const mln_metal_surface_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_metal_surface_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto physical_status = validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled surface dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_session_extent(*session, descriptor->extent);
  session->surface.backend = std::make_unique<MetalSurfaceSessionBackend>(
    static_cast<CA::MetalLayer*>(descriptor->layer),
    static_cast<MTL::Device*>(descriptor->context.device),
    mbgl::Size{session->physical_width, session->physical_height}
  );
  return attach_render_session(
    std::move(session), out_session, RenderSessionKind::Surface,
    RenderSessionAttachMessages{
      .null_session = "surface session must not be null",
      .null_output = "out_session must not be null",
      .non_null_output = "out_session must point to a null handle"
    }
  );
}

auto metal_surface_set_target(
  mln_render_session session, const mln_metal_surface_descriptor* descriptor
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto session_status = validate_render_session_retarget(
    session, RetargetTargetKind::Surface, live
  );
  if (session_status != MLN_STATUS_OK) {
    return session_status;
  }
  const auto descriptor_status = validate_metal_surface_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  return surface_session_set_target(
    session, descriptor->extent,
    [descriptor](mln_render_session_object& target_session) -> mln_status {
      return target_session.surface.backend->set_metal_target(*descriptor);
    }
  );
}

}  // namespace mln::core
