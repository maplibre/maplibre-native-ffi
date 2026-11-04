#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <mln/gfx/backend_scope.hpp>
#include <mln/mtl/context.hpp>
#include <mln/mtl/renderable_resource.hpp>
#include <mln/mtl/texture2d.hpp>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/NSError.hpp>
#include <Foundation/NSString.hpp>
#include <IOSurface/IOSurfaceRef.h>
#include <Metal/MTLBlitCommandEncoder.hpp>
#include <Metal/MTLBlitPass.hpp>
#include <Metal/MTLCommandBuffer.hpp>
#include <Metal/MTLCommandQueue.hpp>
#include <Metal/MTLEvent.hpp>
#include <Metal/MTLRenderPass.hpp>
#include <Metal/MTLTexture.hpp>
#include <TargetConditionals.h>

#include "render/metal/metal_texture_backend.inc"

namespace mln::core {
namespace {

struct CFReleaser {
  void operator()(CFTypeRef object) const { CFRelease(object); }
};
using IOSurfacePtr =
  std::unique_ptr<std::remove_pointer_t<IOSurfaceRef>, CFReleaser>;

void set_number(
  CFMutableDictionaryRef properties, CFStringRef key, int64_t value
) {
  auto number = std::unique_ptr<const void, CFReleaser>(
    CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &value)
  );
  CFDictionarySetValue(properties, key, number.get());
}

// A surface that Metal renders into as MTLPixelFormatBGRA8Unorm, and that
// Core Video wraps as kCVPixelFormatType_32BGRA.
auto create_bgra_io_surface(std::size_t width, std::size_t height)
  -> IOSurfacePtr {
  constexpr int64_t bytes_per_element = 4;
  // kCVPixelFormatType_32BGRA, the four characters 'BGRA'.
  constexpr int64_t pixel_format = 0x42475241;
  auto properties =
    std::unique_ptr<__CFDictionary, CFReleaser>(CFDictionaryCreateMutable(
      kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks
    ));
  if (!properties) return {};
  set_number(properties.get(), kIOSurfaceWidth, static_cast<int64_t>(width));
  set_number(properties.get(), kIOSurfaceHeight, static_cast<int64_t>(height));
  set_number(properties.get(), kIOSurfaceBytesPerElement, bytes_per_element);
  set_number(
    properties.get(), kIOSurfaceBytesPerRow,
    static_cast<int64_t>(IOSurfaceAlignProperty(
      kIOSurfaceBytesPerRow, width * static_cast<std::size_t>(bytes_per_element)
    ))
  );
  set_number(properties.get(), kIOSurfacePixelFormat, pixel_format);
  return IOSurfacePtr{IOSurfaceCreate(properties.get())};
}

}  // namespace

class MetalTextureBackend::MetalTextureRenderableResource final
    : public mln::mtl::RenderableResource {
 public:
  MetalTextureRenderableResource(
    MetalTextureBackend& backend_, mln::mtl::Context& context_, mln::Size size_,
    MTL::Texture* borrowed_texture_, MTL::PixelFormat owned_pixel_format_,
    bool io_surface_
  )
      : backend(backend_),
        context(context_),
        size(size_),
        borrowedTexture(borrowed_texture_),
        ownedPixelFormat(owned_pixel_format_),
        ioSurfaceBacked(io_surface_) {
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
    // mbgl's offscreen texture is RGBA8 only, so an offscreen session makes
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
      if (ioSurfaceBacked) {
        ioSurface = create_bgra_io_surface(size.width, size.height);
        if (!ioSurface) throw std::bad_alloc();
        ownedTexture =
          NS::TransferPtr(context.getBackend().getDevice()->newTexture(
            descriptor.get(), ioSurface.get(), 0
          ));
      } else {
        ownedTexture = context.createMetalTexture(std::move(descriptor));
      }
      if (!ownedTexture) {
        throw std::bad_alloc();
      }
    }

    backend.finish_frame();
    assert(context.getBackend().getCommandQueue());
    commandBuffer =
      NS::RetainPtr(context.getBackend().getCommandQueue()->commandBuffer());
    // A wait applies to the passes encoded after it in the command buffer.
    // Each pass binds a new buffer, and only the last one commits, so every
    // buffer gets the waits until the frame commits.
    for (const auto& wait : backend.event_waits_) {
      commandBuffer->encodeWait(wait.event.get(), wait.value);
    }
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
#if TARGET_OS_OSX || TARGET_OS_MACCATALYST
    // A macOS texture's storage is managed by default, and on a Mac with a
    // discrete GPU, its CPU copy lags what the GPU wrote. The host may read the
    // IOSurface on the CPU, after IOSurfaceLock, so the frame brings that copy
    // up to date.
    if (
      ioSurfaceBacked && ownedTexture &&
      ownedTexture->storageMode() == MTL::StorageModeManaged
    ) {
      auto* const blit = commandBuffer->blitCommandEncoder();
      blit->synchronizeResource(ownedTexture.get());
      blit->endEncoding();
    }
#endif
    backend.commit(std::exchange(commandBuffer, {}));
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
  bool ioSurfaceBacked;
  IOSurfacePtr ioSurface;
  mln::mtl::MTLTexturePtr ownedTexture;
  mln::gfx::Texture2DPtr depthTexture;
  mln::gfx::Texture2DPtr stencilTexture;
  mln::mtl::MTLCommandBufferPtr commandBuffer;
  mln::mtl::MTLRenderPassDescriptorPtr renderPassDescriptor;
};

MetalTextureBackend::MetalTextureBackend(
  MTL::Device* host_device, MTL::CommandQueue* host_queue, mln::Size size,
  std::size_t ring_depth, MTL::PixelFormat pixel_format, bool io_surfaces
)
    : mln::mtl::RendererBackend(mln::gfx::ContextMode::Unique),
      mln::gfx::HeadlessBackend(size),
      owned_pixel_format_(pixel_format),
      io_surfaces_(io_surfaces),
      ring_(ring_depth) {
  device = NS::RetainPtr(host_device);
  commandQueue = host_queue != nullptr
                   ? NS::RetainPtr(host_queue)
                   : NS::TransferPtr(device->newCommandQueue());
}

MetalTextureBackend::MetalTextureBackend(
  std::vector<MTL::Texture*> borrowed_textures, MTL::CommandQueue* host_queue,
  mln::Size size
)
    : mln::mtl::RendererBackend(mln::gfx::ContextMode::Unique),
      mln::gfx::HeadlessBackend(size),
      borrowed_textures_(std::move(borrowed_textures)),
      borrowed_pixel_format_(borrowed_textures_.front()->pixelFormat()),
      ring_(borrowed_textures_.size()) {
  device = NS::RetainPtr(borrowed_textures_.front()->device());
  commandQueue = host_queue != nullptr
                   ? NS::RetainPtr(host_queue)
                   : NS::TransferPtr(device->newCommandQueue());
}

MetalTextureBackend::~MetalTextureBackend() {
  finish_frame();
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
      owned_pixel_format_, io_surfaces_
    );
    // Recorded with the resource it describes, so a slot that keeps an older
    // resource keeps the size that resource was built for.
    ring_.record_size(getSize());
  }
  return *this;
}

auto MetalTextureBackend::readStillImage() -> mln::PremultipliedImage {
  finish_frame();
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

void MetalTextureBackend::commit(NS::SharedPtr<MTL::CommandBuffer> buffer) {
  if (producer_sync_kind_ == MLN_GPU_SYNC_METAL_SHARED_EVENT) {
    if (!event_) event_ = NS::TransferPtr(device->newSharedEvent());
    buffer->encodeSignalEvent(event_.get(), ++event_value_);
  }
  buffer->commit();
  event_waits_.clear();
  if (producer_sync_kind_ != MLN_GPU_SYNC_CPU_COMPLETE) {
    in_flight_ = std::move(buffer);
    return;
  }
  buffer->waitUntilCompleted();
  record_command_buffer_result(*buffer.get());
}

auto MetalTextureBackend::producer_sync() const -> mln_gpu_sync {
  return mln_gpu_sync{
    .size = sizeof(mln_gpu_sync),
    .kind = producer_sync_kind_,
    .handle = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(event_.get())),
    .value = event_value_,
  };
}

void MetalTextureBackend::wait_event(MTL::SharedEvent* event, uint64_t value) {
  event_waits_.push_back(
    EventWait{.event = NS::RetainPtr(event), .value = value}
  );
}

void MetalTextureBackend::finish_frame() {
  if (!in_flight_) return;
  in_flight_->waitUntilCompleted();
  record_command_buffer_result(*in_flight_.get());
  in_flight_.reset();
}

void MetalTextureBackend::record_command_buffer_result(
  const MTL::CommandBuffer& buffer
) {
  if (buffer.status() != MTL::CommandBufferStatusError) {
    command_buffer_failure_.reset();
    return;
  }
  auto failure = CommandBufferFailure{};
  if (const auto* error = buffer.error(); error != nullptr) {
    failure.code = error->code();
    if (const auto* description = error->localizedDescription()) {
      failure.description = description->utf8String();
    }
  }
  command_buffer_failure_ = std::move(failure);
}

auto MetalTextureBackend::take_command_buffer_failure()
  -> std::optional<CommandBufferFailure> {
  return std::exchange(command_buffer_failure_, std::nullopt);
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

void MetalTextureBackend::release_borrowed_textures() {
  finish_frame();
  // The session never retained the textures, so it keeps no pointer to one
  // that the host may destroy.
  std::fill(borrowed_textures_.begin(), borrowed_textures_.end(), nullptr);
  auto guard = mln::gfx::BackendScope{*this};
  resource.reset();
  ring_.reset(borrowed_textures_.size());
}

void MetalTextureBackend::set_borrowed_textures(
  std::vector<MTL::Texture*> textures, mln::Size new_size
) {
  // The session no longer touches a texture that it replaces.
  finish_frame();
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
