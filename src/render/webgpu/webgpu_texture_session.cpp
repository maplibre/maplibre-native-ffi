// clang-format off
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>

// Include WebGPU headers first to define WEBGPU_H_ before MapLibre WebGPU
// headers.
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

#include <mbgl/gfx/headless_backend.hpp>
#include <mbgl/util/image.hpp>
#include <mbgl/util/size.hpp>
#include <mbgl/webgpu/renderable_resource.hpp>
#include <mbgl/webgpu/renderer_backend.hpp>

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "maplibre_native_c/texture.h"
#include "render/render_session_common.hpp"
#include "render/texture_session.hpp"
// clang-format on

namespace {

constexpr auto webgpu_owned_color_format = WGPUTextureFormat_RGBA8Unorm;

auto validate_webgpu_texture(
  const mln_webgpu_borrowed_texture_descriptor& descriptor,
  mbgl::Size physical_size
) -> mln_status {
  auto* const texture = static_cast<WGPUTexture>(descriptor.texture);
  const auto format = static_cast<WGPUTextureFormat>(descriptor.format);

  if (
    wgpuTextureGetWidth(texture) != physical_size.width ||
    wgpuTextureGetHeight(texture) != physical_size.height ||
    wgpuTextureGetDepthOrArrayLayers(texture) != 1 ||
    wgpuTextureGetDimension(texture) != WGPUTextureDimension_2D ||
    wgpuTextureGetMipLevelCount(texture) != 1 ||
    wgpuTextureGetSampleCount(texture) != 1
  ) {
    mln::core::set_thread_error(
      "WebGPU texture must be 2D, single-sample, one-layer, one-mip, and match "
      "extent"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    format == WGPUTextureFormat_Undefined ||
    wgpuTextureGetFormat(texture) != format
  ) {
    mln::core::set_thread_error(
      "WebGPU texture format must be specified and match descriptor"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((wgpuTextureGetUsage(texture) & WGPUTextureUsage_RenderAttachment) == 0) {
    mln::core::set_thread_error(
      "WebGPU texture must include RenderAttachment usage"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  return MLN_STATUS_OK;
}

class WebGPUTextureBackend final : public mbgl::webgpu::RendererBackend,
                                   public mbgl::gfx::HeadlessBackend {
 public:
  class RenderableResource final : public mbgl::webgpu::RenderableResource {
   public:
    explicit RenderableResource(WebGPUTextureBackend& backend_)
        : backend(backend_) {}

    void bind() override { backend.ensureDepthStencilTexture(); }

    void swap() override {}

    const mbgl::webgpu::RendererBackend& getBackend() const override {
      return backend;
    }

    const WGPUCommandEncoder& getCommandEncoder() const override {
      static WGPUCommandEncoder dummy = nullptr;
      return dummy;
    }

    WGPURenderPassEncoder getRenderPassEncoder() const override {
      return nullptr;
    }

    WGPUTextureView getColorTextureView() override {
      return static_cast<WGPUTextureView>(backend.getCurrentTextureView());
    }

    std::optional<wgpu::TextureFormat> getColorTextureFormat() const override {
      return backend.color_format();
    }

    WGPUTextureView getDepthStencilTextureView() override {
      return static_cast<WGPUTextureView>(backend.getDepthStencilView());
    }

    std::optional<wgpu::TextureFormat>
    getDepthStencilTextureFormat() const override {
      return backend.getDepthStencilFormat();
    }

   private:
    WebGPUTextureBackend& backend;
  };

  WebGPUTextureBackend(
    const mln_webgpu_owned_texture_descriptor& descriptor, mbgl::Size size
  )
      : mbgl::webgpu::RendererBackend(mbgl::gfx::ContextMode::Shared),
        mbgl::gfx::HeadlessBackend(size),
        owns_color_texture_(true),
        color_format_(webgpu_owned_color_format) {
    initializeContext(descriptor.context);
    setColorFormat(static_cast<wgpu::TextureFormat>(color_format_));
    setDepthStencilFormat(wgpu::TextureFormat::Depth24PlusStencil8);
    ensureColorTexture();
  }

  WebGPUTextureBackend(
    const mln_webgpu_borrowed_texture_descriptor& descriptor, mbgl::Size size
  )
      : mbgl::webgpu::RendererBackend(mbgl::gfx::ContextMode::Shared),
        mbgl::gfx::HeadlessBackend(size),
        owns_color_texture_(false),
        texture_(static_cast<WGPUTexture>(descriptor.texture)),
        color_view_(static_cast<WGPUTextureView>(descriptor.texture_view)),
        color_format_(static_cast<WGPUTextureFormat>(descriptor.format)) {
    initializeContext(descriptor.context);
    wgpuTextureAddRef(texture_);
    wgpuTextureViewAddRef(color_view_);
    setColorFormat(static_cast<wgpu::TextureFormat>(color_format_));
    setDepthStencilFormat(wgpu::TextureFormat::Depth24PlusStencil8);
  }

  ~WebGPUTextureBackend() override { shutdown(); }

  mbgl::gfx::Renderable& getDefaultRenderable() override {
    if (!hasResource()) {
      setResource(std::make_unique<RenderableResource>(*this));
    }
    return *this;
  }

  mbgl::PremultipliedImage readStillImage() override {
    return mbgl::PremultipliedImage(getSize());
  }

  mbgl::gfx::RendererBackend* getRendererBackend() override { return this; }

  void* getCurrentTextureView() override {
    ensureColorTexture();
    return color_view_;
  }

  void* getDepthStencilView() override {
    ensureDepthStencilTexture();
    return depth_stencil_view_;
  }

  mbgl::Size getFramebufferSize() const override { return getSize(); }

  auto rendered_texture() const -> void* { return texture_; }
  auto rendered_texture_view() const -> void* { return color_view_; }
  auto device() const -> void* { return device_; }
  auto color_format() const -> wgpu::TextureFormat {
    return static_cast<wgpu::TextureFormat>(color_format_);
  }

 protected:
  void activate() override {
    ensureColorTexture();
    ensureDepthStencilTexture();
  }

  void deactivate() override {}

 private:
  void initializeContext(const mln_webgpu_context_descriptor& context) {
    if (context.instance != nullptr) {
      instance_ = static_cast<WGPUInstance>(context.instance);
      wgpuInstanceAddRef(instance_);
    }

    device_ = static_cast<WGPUDevice>(context.device);
    wgpuDeviceAddRef(device_);

    if (context.queue != nullptr) {
      queue_ = static_cast<WGPUQueue>(context.queue);
      wgpuQueueAddRef(queue_);
    } else {
      queue_ = wgpuDeviceGetQueue(device_);
    }

    setInstance(instance_);
    setDevice(device_);
    setQueue(queue_);
  }

  void ensureColorTexture() {
    if (!owns_color_texture_) {
      return;
    }
    const auto size = getSize();
    if (device_ == nullptr || size.width == 0 || size.height == 0) {
      return;
    }
    if (
      texture_ != nullptr && color_texture_size_.width == size.width &&
      color_texture_size_.height == size.height
    ) {
      return;
    }

    releaseColorTexture();

    WGPUTextureDescriptor texture_desc{};
    texture_desc.usage = WGPUTextureUsage_RenderAttachment |
                         WGPUTextureUsage_TextureBinding |
                         WGPUTextureUsage_CopySrc;
    texture_desc.dimension = WGPUTextureDimension_2D;
    texture_desc.size = {size.width, size.height, 1};
    texture_desc.format = color_format_;
    texture_desc.mipLevelCount = 1;
    texture_desc.sampleCount = 1;
    texture_ = wgpuDeviceCreateTexture(device_, &texture_desc);
    if (texture_ == nullptr) {
      throw std::runtime_error("Failed to create WebGPU color texture");
    }

    WGPUTextureViewDescriptor view_desc{};
    view_desc.format = color_format_;
    view_desc.dimension = WGPUTextureViewDimension_2D;
    view_desc.baseMipLevel = 0;
    view_desc.mipLevelCount = 1;
    view_desc.baseArrayLayer = 0;
    view_desc.arrayLayerCount = 1;
    view_desc.aspect = WGPUTextureAspect_All;
    color_view_ = wgpuTextureCreateView(texture_, &view_desc);
    if (color_view_ == nullptr) {
      releaseColorTexture();
      throw std::runtime_error("Failed to create WebGPU color texture view");
    }
    color_texture_size_ = size;
  }

  void ensureDepthStencilTexture() {
    const auto size = getSize();
    if (device_ == nullptr || size.width == 0 || size.height == 0) {
      return;
    }
    if (
      depth_stencil_texture_ != nullptr &&
      depth_stencil_size_.width == size.width &&
      depth_stencil_size_.height == size.height
    ) {
      return;
    }

    releaseDepthStencilTexture();

    const auto depth_format =
      static_cast<WGPUTextureFormat>(getDepthStencilFormat());
    WGPUTextureDescriptor depth_desc{};
    depth_desc.usage = WGPUTextureUsage_RenderAttachment;
    depth_desc.dimension = WGPUTextureDimension_2D;
    depth_desc.size = {size.width, size.height, 1};
    depth_desc.format = depth_format;
    depth_desc.mipLevelCount = 1;
    depth_desc.sampleCount = 1;
    depth_stencil_texture_ = wgpuDeviceCreateTexture(device_, &depth_desc);
    if (depth_stencil_texture_ == nullptr) {
      throw std::runtime_error("Failed to create WebGPU depth texture");
    }

    WGPUTextureViewDescriptor view_desc{};
    view_desc.format = depth_format;
    view_desc.dimension = WGPUTextureViewDimension_2D;
    view_desc.baseMipLevel = 0;
    view_desc.mipLevelCount = 1;
    view_desc.baseArrayLayer = 0;
    view_desc.arrayLayerCount = 1;
    view_desc.aspect = WGPUTextureAspect_All;
    depth_stencil_view_ =
      wgpuTextureCreateView(depth_stencil_texture_, &view_desc);
    if (depth_stencil_view_ == nullptr) {
      releaseDepthStencilTexture();
      throw std::runtime_error("Failed to create WebGPU depth texture view");
    }
    depth_stencil_size_ = size;
  }

  void releaseDepthStencilTexture() {
    if (depth_stencil_view_ != nullptr) {
      wgpuTextureViewRelease(depth_stencil_view_);
      depth_stencil_view_ = nullptr;
    }
    if (depth_stencil_texture_ != nullptr) {
      wgpuTextureRelease(depth_stencil_texture_);
      depth_stencil_texture_ = nullptr;
    }
    depth_stencil_size_ = {0, 0};
  }

  void releaseColorTexture() {
    if (color_view_ != nullptr) {
      wgpuTextureViewRelease(color_view_);
      color_view_ = nullptr;
    }
    if (texture_ != nullptr) {
      wgpuTextureRelease(texture_);
      texture_ = nullptr;
    }
    color_texture_size_ = {0, 0};
  }

  void shutdown() {
    releaseDepthStencilTexture();
    releaseColorTexture();
    if (queue_ != nullptr) {
      wgpuQueueRelease(queue_);
      queue_ = nullptr;
    }
    if (device_ != nullptr) {
      wgpuDeviceRelease(device_);
      device_ = nullptr;
    }
    if (instance_ != nullptr) {
      wgpuInstanceRelease(instance_);
      instance_ = nullptr;
    }
  }

  bool owns_color_texture_ = false;
  WGPUTexture texture_ = nullptr;
  WGPUTextureView color_view_ = nullptr;
  WGPUTextureFormat color_format_ = WGPUTextureFormat_Undefined;
  WGPUInstance instance_ = nullptr;
  WGPUDevice device_ = nullptr;
  WGPUQueue queue_ = nullptr;
  mbgl::Size color_texture_size_{0, 0};
  WGPUTexture depth_stencil_texture_ = nullptr;
  WGPUTextureView depth_stencil_view_ = nullptr;
  mbgl::Size depth_stencil_size_{0, 0};
};

class WebGPUTextureSessionBackend final
    : public mln::core::TextureSessionBackend {
 public:
  WebGPUTextureSessionBackend(
    const mln_webgpu_owned_texture_descriptor& descriptor, mbgl::Size size
  )
      : backend_(descriptor, size) {}

  WebGPUTextureSessionBackend(
    const mln_webgpu_borrowed_texture_descriptor& descriptor, mbgl::Size size
  )
      : backend_(descriptor, size) {}

  auto headless_backend() -> mbgl::gfx::HeadlessBackend& override {
    return backend_;
  }

  auto after_render(mln_render_session& session) -> mln_status override {
    session.texture.rendered_native_texture = backend_.rendered_texture();
    return MLN_STATUS_OK;
  }

  auto acquire_webgpu_owned_frame(
    const mln_render_session& texture, mln_webgpu_owned_texture_frame& out_frame
  ) -> mln_status override {
    out_frame = mln_webgpu_owned_texture_frame{
      .size = sizeof(mln_webgpu_owned_texture_frame),
      .generation = texture.generation,
      .width = texture.physical_width,
      .height = texture.physical_height,
      .scale_factor = texture.scale_factor,
      .frame_id = texture.texture.next_frame_id,
      .texture = backend_.rendered_texture(),
      .texture_view = backend_.rendered_texture_view(),
      .device = backend_.device(),
      .format = static_cast<uint32_t>(backend_.color_format()),
    };
    return MLN_STATUS_OK;
  }

 private:
  WebGPUTextureBackend backend_;
};

}  // namespace

namespace mln::core {

auto supported_render_backend_mask() noexcept -> uint32_t {
#if defined(MLN_RENDER_BACKEND_WEBGPU)
  return MLN_RENDER_BACKEND_FLAG_WEBGPU;
#else
  return 0;
#endif
}

auto webgpu_owned_texture_attach(
  mln_map* map, const mln_webgpu_owned_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
#if !defined(MLN_RENDER_BACKEND_WEBGPU)
  set_thread_error("WebGPU texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
#else
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_webgpu_owned_texture_descriptor(descriptor);
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
    descriptor->extent.scale_factor, "scaled texture dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  try {
    auto session = std::make_unique<mln_render_session>();
    session->map = map;
    session->owner_thread = map_owner_thread(map);
    set_session_extent(*session, descriptor->extent);
    session->texture.api_kind = TextureSessionApi::WebGPU;
    session->texture.mode = TextureSessionMode::Owned;
    session->texture.backend = std::make_unique<WebGPUTextureSessionBackend>(
      *descriptor, mbgl::Size{session->physical_width, session->physical_height}
    );
    return attach_render_session(
      std::move(session), out_session, RenderSessionKind::Texture,
      RenderSessionAttachMessages{
        .null_session = "texture session must not be null",
        .null_output = "out_session must not be null",
        .non_null_output = "out_session must point to a null handle",
      }
    );
  } catch (const std::exception& exception) {
    set_thread_error(exception.what());
    return MLN_STATUS_NATIVE_ERROR;
  }
#endif
}

auto webgpu_borrowed_texture_attach(
  mln_map* map, const mln_webgpu_borrowed_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
#if !defined(MLN_RENDER_BACKEND_WEBGPU)
  set_thread_error(
    "WebGPU borrowed texture sessions are not supported by this build"
  );
  return MLN_STATUS_UNSUPPORTED;
#else
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_webgpu_borrowed_texture_descriptor(descriptor);
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
    descriptor->extent.scale_factor, "scaled texture dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  const auto physical_size = mbgl::Size{
    physical_dimension(
      descriptor->extent.width, descriptor->extent.scale_factor
    ),
    physical_dimension(
      descriptor->extent.height, descriptor->extent.scale_factor
    ),
  };
  const auto texture_status =
    validate_webgpu_texture(*descriptor, physical_size);
  if (texture_status != MLN_STATUS_OK) {
    return texture_status;
  }

  try {
    auto session = std::make_unique<mln_render_session>();
    session->map = map;
    session->owner_thread = map_owner_thread(map);
    set_session_extent(*session, descriptor->extent);
    session->texture.api_kind = TextureSessionApi::WebGPU;
    session->texture.mode = TextureSessionMode::Borrowed;
    session->texture.backend =
      std::make_unique<WebGPUTextureSessionBackend>(*descriptor, physical_size);
    return attach_render_session(
      std::move(session), out_session, RenderSessionKind::Texture,
      RenderSessionAttachMessages{
        .null_session = "texture session must not be null",
        .null_output = "out_session must not be null",
        .non_null_output = "out_session must point to a null handle",
      }
    );
  } catch (const std::exception& exception) {
    set_thread_error(exception.what());
    return MLN_STATUS_NATIVE_ERROR;
  }
#endif
}

auto webgpu_owned_texture_acquire_frame(
  mln_render_session* texture, mln_webgpu_owned_texture_frame* out_frame
) -> mln_status {
  const auto status = validate_live_attached_texture(texture);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    out_frame == nullptr ||
    out_frame->size < sizeof(mln_webgpu_owned_texture_frame)
  ) {
    set_thread_error("out_frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (texture->texture.acquired) {
    set_thread_error("a texture frame is already acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (texture->rendered_generation != texture->generation) {
    set_thread_error("no rendered frame is available for this generation");
    return MLN_STATUS_INVALID_STATE;
  }
  if (
    texture->texture.mode != TextureSessionMode::Owned ||
    texture->texture.api_kind != TextureSessionApi::WebGPU
  ) {
    set_thread_error("texture session cannot expose a WebGPU texture frame");
    return MLN_STATUS_UNSUPPORTED;
  }

  const auto acquire_status =
    texture->texture.backend->acquire_webgpu_owned_frame(*texture, *out_frame);
  if (acquire_status != MLN_STATUS_OK) {
    return acquire_status;
  }
  texture->texture.acquired_native_texture = out_frame->texture;
  texture->texture.acquired = true;
  texture->texture.acquired_frame_id = out_frame->frame_id;
  texture->texture.acquired_frame_kind = TextureSessionFrameKind::WebGPUOwned;
  ++texture->texture.next_frame_id;
  return MLN_STATUS_OK;
}

auto webgpu_owned_texture_release_frame(
  mln_render_session* texture, const mln_webgpu_owned_texture_frame* frame
) -> mln_status {
  const auto status = validate_texture(texture);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    frame == nullptr || frame->size < sizeof(mln_webgpu_owned_texture_frame)
  ) {
    set_thread_error("frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    !texture->texture.acquired ||
    texture->texture.acquired_frame_kind != TextureSessionFrameKind::WebGPUOwned
  ) {
    set_thread_error("no texture frame is currently acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (frame->generation != texture->generation) {
    set_thread_error("frame generation does not match acquired frame");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (frame->frame_id != texture->texture.acquired_frame_id) {
    set_thread_error("frame identity does not match acquired frame");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  texture->texture.acquired = false;
  texture->texture.acquired_frame_id = 0;
  texture->texture.acquired_frame_kind = TextureSessionFrameKind::None;
  texture->texture.acquired_native_texture = nullptr;
  return MLN_STATUS_OK;
}

}  // namespace mln::core
