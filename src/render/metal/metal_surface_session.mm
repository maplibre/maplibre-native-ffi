#include <memory>
#include <stdexcept>

#include <mln/gfx/backend_scope.hpp>
#include <mln/mtl/context.hpp>
#include <mln/mtl/renderable_resource.hpp>
#include <mln/mtl/renderer_backend.hpp>
#include <mln/mtl/texture2d.hpp>
#include <mln/util/size.hpp>

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

class MetalSurfaceBackend final : public mln::mtl::RendererBackend,
                                  public mln::gfx::Renderable {
 private:
  class MetalSurfaceRenderableResource final
      : public mln::mtl::RenderableResource {
   public:
    MetalSurfaceRenderableResource(
      MetalSurfaceBackend& backend_, CA::MetalLayer* layer_, mln::Size size_
    )
        : backend(backend_), layer(NS::RetainPtr(layer_)) {
      layer->setDevice(backend.getDevice().get());
      // BGRA8 is a supported CAMetalLayer format on every Apple target.
      // RGBA8 textures are valid offscreen but fail here on iOS 15.
      layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
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
    ~MetalSurfaceRenderableResource() override { finish_committed(); }

    void setSize(mln::Size size_) {
      size = size_;
      layer->setDrawableSize(CGSizeMake(
        static_cast<CGFloat>(size.width), static_cast<CGFloat>(size.height)
      ));
      depthStencilDirty = true;
    }

    // Every replacement layer uses the same device and pixel format, preserving
    // the cached render pipeline states.
    void set_layer(CA::MetalLayer* layer_, mln::Size size_) {
      finish_committed();
      // Release what is still bound to the outgoing layer: a drawable must not
      // outlive its layer. This frame's buffer was never committed.
      commandBuffer.reset();
      renderPassDescriptor.reset();
      drawable.reset();

      layer = NS::RetainPtr(layer_);
      layer->setDevice(backend.getDevice().get());
      layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
      layer->setFramebufferOnly(false);
      setSize(size_);
    }

    // A minimized or occluded layer may have no drawable. Check before bind(),
    // which requires one.
    auto try_bind() -> bool {
      if (drawable && commandBuffer && renderPassDescriptor) {
        return true;
      }

      auto* next_drawable = layer->nextDrawable();
      if (next_drawable == nullptr) {
        return false;
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
        auto& context = static_cast<mln::mtl::Context&>(backend.getContext());
        depthTexture = context.createTexture2D();
        depthTexture->setSize(size);
        depthTexture->setFormat(
          mln::gfx::TexturePixelType::Depth,
          mln::gfx::TextureChannelDataType::Float
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

        depthTexture->create();
        // The texture was created by mln::mtl::Context above.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        const auto depthFormat =
          static_cast<mln::mtl::Texture2D*>(depthTexture.get())
            ->getMetalTexture()
            ->pixelFormat();
        if (depthFormat == MTL::PixelFormatDepth32Float_Stencil8) {
          // Native uses a combined format on simulators and Intel Macs.
          // Metal requires both attachments to reference the same texture.
          stencilTexture = depthTexture;
        } else {
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
          stencilTexture->create();
        }
      }

      if (auto* depthTarget = renderPassDescriptor->depthAttachment()) {
        depthTarget->setTexture(
          // The texture was created by mln::mtl::Context above.
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
          static_cast<mln::mtl::Texture2D*>(depthTexture.get())
            ->getMetalTexture()
        );
      }
      if (auto* stencilTarget = renderPassDescriptor->stencilAttachment()) {
        stencilTarget->setTexture(
          // The texture was created by mln::mtl::Context above.
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
          static_cast<mln::mtl::Texture2D*>(stencilTexture.get())
            ->getMetalTexture()
        );
      }
      return true;
    }

    void bind() override {
      if (!try_bind()) {
        throw mln::core::RenderTargetNotReady(
          "Metal surface did not provide a drawable"
        );
      }
    }

    void swap() override {
      if (!mln::core::discard_renderable_present) {
        commandBuffer->presentDrawable(drawable.get());
      }
      commandBuffer->commit();
      lastCommitted = std::move(commandBuffer);
      drawable.reset();
      renderPassDescriptor.reset();
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
      return renderPassDescriptor;
    }

   private:
    // Command buffers retain the drawables they target, so letting go of a
    // layer early is valid Metal. It is not valid to the host, which may
    // destroy a layer once the session lets go of it, nor to the paravirtual
    // GPU of a macOS VM, which faults every process's GPU work when one lets
    // go of a layer with work in flight on its drawable (#1035). Buffers on
    // one queue complete in order, so waiting for the last one is enough.
    void finish_committed() {
      if (lastCommitted) {
        lastCommitted->waitUntilCompleted();
        lastCommitted.reset();
      }
    }

    MetalSurfaceBackend& backend;
    NS::SharedPtr<CA::MetalLayer> layer;
    mln::Size size{0, 0};
    mln::mtl::MTLCommandBufferPtr commandBuffer;
    mln::mtl::MTLCommandBufferPtr lastCommitted;
    mln::mtl::MTLRenderPassDescriptorPtr renderPassDescriptor;
    mln::mtl::CAMetalDrawablePtr drawable;
    mln::gfx::Texture2DPtr depthTexture;
    mln::gfx::Texture2DPtr stencilTexture;
    bool depthStencilDirty = true;
  };

 public:
  MetalSurfaceBackend(
    CA::MetalLayer* layer, MTL::Device* host_device, mln::Size size_
  )
      : mln::mtl::RendererBackend(mln::gfx::ContextMode::Unique),
        mln::gfx::Renderable(size_, nullptr) {
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
    auto guard = mln::gfx::BackendScope{*this};
    resource.reset();
    context.reset();
  }

  auto getDefaultRenderable() -> mln::gfx::Renderable& override {
    return *this;
  }

  void setSize(mln::Size size_) {
    setRenderableSize(size_);
    getResource<MetalSurfaceRenderableResource>().setSize(size_);
  }

  void set_layer(CA::MetalLayer* layer_, mln::Size size_) {
    setRenderableSize(size_);
    getResource<MetalSurfaceRenderableResource>().set_layer(layer_, size_);
  }

  auto try_bind() -> bool {
    return getResource<MetalSurfaceRenderableResource>().try_bind();
  }

  // A null device in a descriptor names no device at all, which every session
  // satisfies; the session keeps the one it attached with either way.
  [[nodiscard]] auto has_device(MTL::Device* other) const -> bool {
    return other == nullptr || other == device.get();
  }

  void activate() override {}
  void deactivate() override {}
};

class MetalSurfaceSessionBackend final
    : public mln::core::SurfaceSessionBackend {
 public:
  MetalSurfaceSessionBackend(
    CA::MetalLayer* layer, MTL::Device* host_device, mln::Size size
  )
      : backend_(layer, host_device, size) {}

  auto renderer_backend() -> mln::gfx::RendererBackend& override {
    return backend_;
  }

  // Metal objects have no thread affinity, and destroying the surface waits
  // for its last committed buffer on whichever thread does it.
  [[nodiscard]] auto allows_off_thread_teardown() const noexcept
    -> bool override {
    return true;
  }

  void resize(uint32_t physical_width, uint32_t physical_height) override {
    backend_.setSize(mln::Size{physical_width, physical_height});
  }

  auto prepare_frame(bool& out_ready) -> mln_status override {
    out_ready = backend_.try_bind();
    return MLN_STATUS_OK;
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
      mln::Size{
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

auto metal_surface_attach_start(
  mln_map map, const mln_metal_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
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
  const auto physical_status = validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled surface dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  // The retains below send messages to the descriptor's layer and device, so
  // this session validates the request before start_attach_render_session does.
  const auto request_status =
    validate_render_session_attach_request(options, out_session, completion);
  if (request_status != MLN_STATUS_OK) {
    return request_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_session_extent(*session, descriptor->extent);
  auto layer = NS::RetainPtr(static_cast<CA::MetalLayer*>(descriptor->layer));
  auto device =
    NS::RetainPtr(static_cast<MTL::Device*>(descriptor->context.device));
  session->initialize_backend =
    [layer = std::move(layer),
     device = std::move(device)](mln_render_session_object& target) mutable {
      target.surface.backend = std::make_unique<MetalSurfaceSessionBackend>(
        layer.get(), device.get(),
        mln::Size{target.physical_width, target.physical_height}
      );
      return MLN_STATUS_OK;
    };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = 0,
    .texture_ring_depth = 0,
    .flags = MLN_RENDER_SESSION_CAPABILITY_PRESENTATION
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Surface, options, capabilities,
    out_session, completion,
    valueless_completion<&mln_map_attach_metal_surface>()
  );
}

auto metal_surface_set_target_start(
  mln_render_session session, const mln_metal_surface_descriptor* descriptor,
  const mln_completion* completion
) -> mln_status {
  auto checked_completion = std::shared_ptr<Completion>{};
  const auto submission_status = validate_render_session_retarget_submission(
    session, RetargetTargetKind::Surface, completion, checked_completion
  );
  if (submission_status != MLN_STATUS_OK) {
    return submission_status;
  }
  const auto descriptor_status = validate_metal_surface_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto physical_status = validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled surface dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  const auto copied = *descriptor;
  auto layer = NS::RetainPtr(static_cast<CA::MetalLayer*>(descriptor->layer));
  auto device =
    NS::RetainPtr(static_cast<MTL::Device*>(descriptor->context.device));
  return enqueue_surface_retarget(
    session, copied.extent,
    [copied, layer = std::move(layer),
     device = std::move(device)](mln_render_session_object& live) {
      auto retained = copied;
      retained.layer = layer.get();
      retained.context.device = device.get();
      return live.surface.backend->set_metal_target(retained);
    },
    std::move(checked_completion),
    valueless_completion<&mln_render_session_set_metal_surface_target>()
  );
}

}  // namespace mln::core
