#include <algorithm>
// clang-format off
#include <atomic>
#include <cstdint>
#include <cstring>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>

// The WebGPU backend builds only against the emdawnwebgpu port, which supplies
// webgpu.h.
#include <emscripten/emscripten.h>

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
#include "render/surface_session.hpp"
#include "render/texture_session.hpp"
// clang-format on

namespace {

constexpr auto webgpu_owned_color_format = WGPUTextureFormat_RGBA8Unorm;

// WebGPU pads every row of a texture-to-buffer copy to this many bytes, so a
// readback stages the padded rows and unpacks them.
constexpr uint32_t readback_row_alignment = 256;
constexpr uint32_t readback_bytes_per_pixel = 4;
// Five seconds of yielding, orders of magnitude more than a map that is going
// to complete needs.
constexpr uint32_t readback_yield_milliseconds = 1;
constexpr uint32_t readback_yield_attempts = 5000;

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
      "the descriptor physical dimensions"
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
    const mln_webgpu_owned_texture_descriptor& descriptor, mbgl::Size size,
    std::size_t ring_depth
  )
      : mbgl::webgpu::RendererBackend(mbgl::gfx::ContextMode::Unique),
        mbgl::gfx::HeadlessBackend(size),
        owns_color_texture_(true),
        color_format_(webgpu_owned_color_format),
        slots_(ring_depth) {
    try {
      initializeContext(descriptor.context);
      setColorFormat(static_cast<wgpu::TextureFormat>(color_format_));
      setDepthStencilFormat(wgpu::TextureFormat::Depth24PlusStencil8);
      ensureColorTexture();
    } catch (...) {
      shutdown();
      throw;
    }
  }

  WebGPUTextureBackend(
    const mln_webgpu_borrowed_texture_descriptor& descriptor, mbgl::Size size
  )
      : mbgl::webgpu::RendererBackend(mbgl::gfx::ContextMode::Unique),
        mbgl::gfx::HeadlessBackend(size),
        owns_color_texture_(false),
        texture_(static_cast<WGPUTexture>(descriptor.texture)),
        color_view_(static_cast<WGPUTextureView>(descriptor.texture_view)),
        color_format_(static_cast<WGPUTextureFormat>(descriptor.format)) {
    wgpuTextureAddRef(texture_);
    wgpuTextureViewAddRef(color_view_);
    try {
      initializeContext(descriptor.context);
      setColorFormat(static_cast<wgpu::TextureFormat>(color_format_));
      setDepthStencilFormat(wgpu::TextureFormat::Depth24PlusStencil8);
    } catch (...) {
      shutdown();
      throw;
    }
  }

  ~WebGPUTextureBackend() override { shutdown(); }

  mbgl::gfx::Renderable& getDefaultRenderable() override {
    if (!hasResource()) {
      setResource(std::make_unique<RenderableResource>(*this));
    }
    return *this;
  }

  // Copies the rendered texture into a mappable buffer and waits for the map.
  // Only a session-owned texture reaches here; a caller's texture need not
  // carry CopySrc usage.
  //
  // wgpuInstanceWaitAny is unusable: a non-zero timeout is legal only on an
  // instance created with timedWaitAnyEnable, and the host's instance is
  // optional. A spontaneous callback needs no instance, and yielding to the
  // browser's job queue is what lets it arrive.
  mbgl::PremultipliedImage readStillImage() override {
    const auto size = getSize();
    if (
      device_ == nullptr || queue_ == nullptr || texture_ == nullptr ||
      size.width == 0 || size.height == 0
    ) {
      return {};
    }

    const auto row_stride = size.width * readback_bytes_per_pixel;
    const auto aligned_row_stride =
      ((row_stride + readback_row_alignment - 1) / readback_row_alignment) *
      readback_row_alignment;
    const auto mapped_size =
      static_cast<uint64_t>(aligned_row_stride) * size.height;

    WGPUBufferDescriptor buffer_desc{};
    buffer_desc.size = mapped_size;
    buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    auto* const staging = wgpuDeviceCreateBuffer(device_, &buffer_desc);
    if (staging == nullptr) {
      return {};
    }

    if (!copy_texture_into(staging, aligned_row_stride, size)) {
      wgpuBufferRelease(staging);
      return {};
    }

    auto image = map_buffer_into_image(
      staging, mapped_size, size, row_stride, aligned_row_stride
    );
    wgpuBufferRelease(staging);
    return image;
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

  auto select_slot(std::size_t slot) -> bool {
    if (slot >= slots_.size()) return false;
    if (slot == selected_slot_) {
      if (texture_ != nullptr && color_texture_size_ != getSize()) {
        releaseColorTexture();
      }
      return true;
    }
    slots_[selected_slot_] =
      ColorSlot{texture_, color_view_, color_texture_size_};
    texture_ = nullptr;
    color_view_ = nullptr;
    color_texture_size_ = {};
    releaseDepthStencilTexture();
    selected_slot_ = slot;
    texture_ = slots_[slot].texture;
    color_view_ = slots_[slot].view;
    color_texture_size_ = slots_[slot].size;
    slots_[slot] = {};
    if (texture_ != nullptr && color_texture_size_ != getSize()) {
      releaseColorTexture();
    }
    return true;
  }

  auto rendered_texture_at(std::size_t slot) const -> void* {
    return slot == selected_slot_ ? texture_ : slots_[slot].texture;
  }
  auto rendered_texture_view_at(std::size_t slot) const -> void* {
    return slot == selected_slot_ ? color_view_ : slots_[slot].view;
  }
  void set_ring_size(mbgl::Size new_size) { size = new_size; }

  // Whether a replacement target names the context this session attached with.
  // A null queue names the device's default queue, as it does at attach. The
  // instance is excluded: a texture session never uses it.
  auto matches_context(const mln_webgpu_context_descriptor& context) const
    -> bool {
    if (static_cast<WGPUDevice>(context.device) != device_) {
      return false;
    }
    if (context.queue != nullptr) {
      return static_cast<WGPUQueue>(context.queue) == queue_;
    }
    auto* const default_queue = wgpuDeviceGetQueue(device_);
    const auto matches = default_queue == queue_;
    wgpuQueueRelease(default_queue);
    return matches;
  }

  // Whether a replacement texture can be drawn by the pipelines already built.
  // mbgl reads the color format off the target when it builds a render pipeline
  // but does not key its cache on it, so a texture in another format would be
  // drawn with pipelines built for this one.
  auto matches_borrowed_target(
    const mln_webgpu_borrowed_texture_descriptor& descriptor
  ) const -> bool {
    return static_cast<WGPUTextureFormat>(descriptor.format) == color_format_;
  }

  // Renders into a different caller-owned texture from here on. The caller has
  // already established that it matches this session's context and format.
  void set_borrowed_target(
    const mln_webgpu_borrowed_texture_descriptor& descriptor
  ) {
    auto* const texture = static_cast<WGPUTexture>(descriptor.texture);
    auto* const view = static_cast<WGPUTextureView>(descriptor.texture_view);
    // Reference the replacement before releasing the outgoing pair, so a
    // descriptor naming the texture this session already holds cannot drop its
    // last reference partway through.
    wgpuTextureAddRef(texture);
    wgpuTextureViewAddRef(view);
    releaseColorTexture();
    texture_ = texture;
    color_view_ = view;
    setSize(mbgl::Size{descriptor.physical_width, descriptor.physical_height});
  }

 protected:
  void activate() override {
    ensureColorTexture();
    ensureDepthStencilTexture();
  }

  void deactivate() override {}

 private:
  // Submits the texture-to-buffer copy on the session's queue, ordered behind
  // the render commands already there, so it needs no fence of its own.
  auto copy_texture_into(
    WGPUBuffer destination, uint32_t aligned_row_stride, mbgl::Size size
  ) -> bool {
    auto* const encoder = wgpuDeviceCreateCommandEncoder(device_, nullptr);
    if (encoder == nullptr) {
      return false;
    }

    WGPUTexelCopyTextureInfo source{};
    source.texture = texture_;
    source.mipLevel = 0;
    source.origin = {0, 0, 0};
    source.aspect = WGPUTextureAspect_All;

    WGPUTexelCopyBufferInfo target{};
    target.buffer = destination;
    target.layout.offset = 0;
    target.layout.bytesPerRow = aligned_row_stride;
    target.layout.rowsPerImage = size.height;

    const WGPUExtent3D copy_size{size.width, size.height, 1};
    wgpuCommandEncoderCopyTextureToBuffer(
      encoder, &source, &target, &copy_size
    );

    auto* const commands = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);
    if (commands == nullptr) {
      return false;
    }
    wgpuQueueSubmit(queue_, 1, &commands);
    wgpuCommandBufferRelease(commands);
    return true;
  }

  // Shared because WebGPU still delivers the callback after a wait gives up, so
  // state on the waiting frame's stack would be written after it is gone.
  struct MapState {
    std::atomic<WGPUMapAsyncStatus> status{WGPUMapAsyncStatus_Error};
    std::atomic_bool completed{false};
  };

  // Maps the staged copy and unpacks its padded rows into an image. The map
  // resolves on the browser's job queue, so this thread yields rather than
  // spins: emscripten_sleep suspends through JSPI and lets that queue run. The
  // wait is bounded so a device that never answers fails the read.
  auto map_buffer_into_image(
    WGPUBuffer staging, uint64_t mapped_size, mbgl::Size size,
    uint32_t row_stride, uint32_t aligned_row_stride
  ) -> mbgl::PremultipliedImage {
    auto const state = std::make_shared<MapState>();
    WGPUBufferMapCallbackInfo callback_info{};
    callback_info.mode = WGPUCallbackMode_AllowSpontaneous;
    callback_info.callback = [](
                               WGPUMapAsyncStatus status,
                               WGPUStringView /*message*/, void* user_data,
                               void* /*reserved*/
                             ) {
      // Takes back the reference handed to the callback; WebGPU promises
      // exactly one delivery.
      const std::unique_ptr<std::shared_ptr<MapState>> owner(
        static_cast<std::shared_ptr<MapState>*>(user_data)
      );
      (*owner)->status.store(status, std::memory_order_relaxed);
      (*owner)->completed.store(true, std::memory_order_release);
    };
    callback_info.userdata1 = new std::shared_ptr<MapState>(state);
    wgpuBufferMapAsync(
      staging, WGPUMapMode_Read, 0, mapped_size, callback_info
    );

    for (uint32_t attempt = 0; attempt < readback_yield_attempts; ++attempt) {
      if (state->completed.load(std::memory_order_acquire)) {
        break;
      }
      emscripten_sleep(readback_yield_milliseconds);
    }
    if (
      !state->completed.load(std::memory_order_acquire) ||
      state->status.load(std::memory_order_relaxed) !=
        WGPUMapAsyncStatus_Success
    ) {
      return {};
    }

    const auto* const mapped = static_cast<const uint8_t*>(
      wgpuBufferGetConstMappedRange(staging, 0, mapped_size)
    );
    if (mapped == nullptr) {
      wgpuBufferUnmap(staging);
      return {};
    }

    auto data = std::make_unique<uint8_t[]>(
      static_cast<size_t>(row_stride) * size.height
    );
    for (uint32_t row = 0; row < size.height; ++row) {
      std::memcpy(
        data.get() + static_cast<size_t>(row) * row_stride,
        mapped + static_cast<size_t>(row) * aligned_row_stride, row_stride
      );
    }
    wgpuBufferUnmap(staging);
    return {size, std::move(data)};
  }

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
      wgpuTextureDestroy(depth_stencil_texture_);
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
      if (owns_color_texture_) {
        wgpuTextureDestroy(texture_);
      }
      wgpuTextureRelease(texture_);
      texture_ = nullptr;
    }
    color_texture_size_ = {0, 0};
  }

  void shutdown() {
    for (auto& slot : slots_) {
      if (slot.view != nullptr) wgpuTextureViewRelease(slot.view);
      if (slot.texture != nullptr) {
        wgpuTextureDestroy(slot.texture);
        wgpuTextureRelease(slot.texture);
      }
    }
    slots_.clear();
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

  struct ColorSlot {
    WGPUTexture texture = nullptr;
    WGPUTextureView view = nullptr;
    mbgl::Size size{};
  };
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
  std::vector<ColorSlot> slots_;
  std::size_t selected_slot_ = 0;
};

// Renders into a surface the host presents, which in a browser is a canvas. A
// surface hands out one texture per frame and takes it back at present, so this
// acquires at frame start and releases at swap rather than holding one.
class WebGPUSurfaceBackend final : public mbgl::webgpu::RendererBackend,
                                   public mbgl::gfx::Renderable {
 public:
  class RenderableResource final : public mbgl::webgpu::RenderableResource {
   public:
    explicit RenderableResource(WebGPUSurfaceBackend& backend_)
        : backend(backend_) {}

    void bind() override { backend.ensureDepthStencilTexture(); }

    void swap() override { backend.present(); }

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
      return backend.color_view();
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
    WebGPUSurfaceBackend& backend;
  };

  WebGPUSurfaceBackend(
    const mln_webgpu_surface_descriptor& descriptor, mbgl::Size size
  )
      : mbgl::webgpu::RendererBackend(mbgl::gfx::ContextMode::Unique),
        mbgl::gfx::Renderable(
          size, std::make_unique<RenderableResource>(*this)
        ),
        surface_(static_cast<WGPUSurface>(descriptor.surface)),
        color_format_(static_cast<WGPUTextureFormat>(descriptor.format)) {
    wgpuSurfaceAddRef(surface_);
    try {
      initializeContext(descriptor.context);
      setColorFormat(static_cast<wgpu::TextureFormat>(color_format_));
      setDepthStencilFormat(wgpu::TextureFormat::Depth24PlusStencil8);
      configureSurface();
    } catch (...) {
      shutdown();
      throw;
    }
  }

  WebGPUSurfaceBackend(const WebGPUSurfaceBackend&) = delete;
  auto operator=(const WebGPUSurfaceBackend&) -> WebGPUSurfaceBackend& = delete;
  WebGPUSurfaceBackend(WebGPUSurfaceBackend&&) = delete;
  auto operator=(WebGPUSurfaceBackend&&) -> WebGPUSurfaceBackend& = delete;

  ~WebGPUSurfaceBackend() override { shutdown(); }

  mbgl::gfx::Renderable& getDefaultRenderable() override { return *this; }

  void* getCurrentTextureView() override { return color_view(); }

  void* getDepthStencilView() override {
    ensureDepthStencilTexture();
    return depth_stencil_view_;
  }

  mbgl::Size getFramebufferSize() const override { return getSize(); }

  auto color_format() const -> wgpu::TextureFormat {
    return static_cast<wgpu::TextureFormat>(color_format_);
  }

  // Takes the frame's texture from the surface. A surface with nothing to give
  // reports not-ready rather than failing: a canvas the browser is not
  // compositing is a frame to skip.
  auto acquire_frame(bool& out_ready) -> bool {
    out_ready = false;
    if (color_view_ != nullptr) {
      out_ready = true;
      return true;
    }
    WGPUSurfaceTexture frame{};
    wgpuSurfaceGetCurrentTexture(surface_, &frame);
    if (
      frame.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
      frame.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal
    ) {
      if (frame.texture != nullptr) {
        wgpuTextureRelease(frame.texture);
      }
      // Outdated means the surface configuration no longer matches what it
      // presents to, which happens when the canvas changes size. Nothing else
      // reconfigures until the host resizes or retargets, so do it here and
      // take the next frame.
      if (frame.status == WGPUSurfaceGetCurrentTextureStatus_Outdated) {
        configureSurface();
        return true;
      }
      // Timeout is this frame arriving late, so the next render tries again.
      // Lost and Error are the surface itself, which only the host can replace.
      return frame.status == WGPUSurfaceGetCurrentTextureStatus_Timeout;
    }

    WGPUTextureViewDescriptor view_desc{};
    view_desc.format = color_format_;
    view_desc.dimension = WGPUTextureViewDimension_2D;
    view_desc.baseMipLevel = 0;
    view_desc.mipLevelCount = 1;
    view_desc.baseArrayLayer = 0;
    view_desc.arrayLayerCount = 1;
    view_desc.aspect = WGPUTextureAspect_All;
    auto* const view = wgpuTextureCreateView(frame.texture, &view_desc);
    if (view == nullptr) {
      wgpuTextureRelease(frame.texture);
      return false;
    }
    color_texture_ = frame.texture;
    color_view_ = view;
    out_ready = true;
    return true;
  }

  auto color_view() -> WGPUTextureView {
    bool ready = false;
    if (!acquire_frame(ready) || !ready) {
      return nullptr;
    }
    return color_view_;
  }

  // Ends the frame by releasing its texture. The browser composites the canvas
  // itself, and wgpuSurfacePresent() aborts here with "unsupported (use
  // requestAnimationFrame via html5.h instead)".
  void present() {
    if (color_view_ == nullptr) {
      return;
    }
    releaseFrame();
  }

  void resize(mbgl::Size size_) {
    if (size_ == getSize()) {
      return;
    }
    size = size_;
    releaseFrame();
    configureSurface();
  }

  auto matches_context(const mln_webgpu_context_descriptor& context) const
    -> bool {
    return static_cast<WGPUDevice>(context.device) == device_;
  }

  auto matches_format(const mln_webgpu_surface_descriptor& descriptor) const
    -> bool {
    return static_cast<WGPUTextureFormat>(descriptor.format) == color_format_;
  }

  // Presents through a different host surface from here on. The caller has
  // already established that it matches this session's device and format.
  void set_surface(const mln_webgpu_surface_descriptor& descriptor) {
    auto* const replacement = static_cast<WGPUSurface>(descriptor.surface);
    // Reference the replacement before releasing the outgoing one, so a
    // descriptor naming the surface this session already holds cannot drop its
    // last reference partway through.
    wgpuSurfaceAddRef(replacement);
    releaseFrame();
    if (surface_ != nullptr) {
      wgpuSurfaceUnconfigure(surface_);
      wgpuSurfaceRelease(surface_);
    }
    surface_ = replacement;
    size = mbgl::Size{
      mln::core::physical_dimension(
        descriptor.extent.width, descriptor.extent.scale_factor
      ),
      mln::core::physical_dimension(
        descriptor.extent.height, descriptor.extent.scale_factor
      )
    };
    configureSurface();
  }

 protected:
  void activate() override { ensureDepthStencilTexture(); }

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

  void configureSurface() {
    const auto size = getSize();
    if (surface_ == nullptr || size.width == 0 || size.height == 0) {
      return;
    }
    WGPUSurfaceConfiguration configuration{};
    configuration.device = device_;
    configuration.format = color_format_;
    configuration.usage = WGPUTextureUsage_RenderAttachment;
    configuration.width = size.width;
    configuration.height = size.height;
    configuration.alphaMode = WGPUCompositeAlphaMode_Auto;
    configuration.presentMode = WGPUPresentMode_Fifo;
    wgpuSurfaceConfigure(surface_, &configuration);
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
      wgpuTextureDestroy(depth_stencil_texture_);
      wgpuTextureRelease(depth_stencil_texture_);
      depth_stencil_texture_ = nullptr;
    }
    depth_stencil_size_ = {0, 0};
  }

  // Releases the frame's texture without presenting it.
  void releaseFrame() {
    if (color_view_ != nullptr) {
      wgpuTextureViewRelease(color_view_);
      color_view_ = nullptr;
    }
    if (color_texture_ != nullptr) {
      wgpuTextureRelease(color_texture_);
      color_texture_ = nullptr;
    }
  }

  void shutdown() {
    releaseDepthStencilTexture();
    releaseFrame();
    if (surface_ != nullptr) {
      wgpuSurfaceUnconfigure(surface_);
      wgpuSurfaceRelease(surface_);
      surface_ = nullptr;
    }
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

  WGPUSurface surface_ = nullptr;
  WGPUTexture color_texture_ = nullptr;
  WGPUTextureView color_view_ = nullptr;
  WGPUTextureFormat color_format_ = WGPUTextureFormat_Undefined;
  WGPUInstance instance_ = nullptr;
  WGPUDevice device_ = nullptr;
  WGPUQueue queue_ = nullptr;
  WGPUTexture depth_stencil_texture_ = nullptr;
  WGPUTextureView depth_stencil_view_ = nullptr;
  mbgl::Size depth_stencil_size_{0, 0};
};

class WebGPUSurfaceSessionBackend final
    : public mln::core::SurfaceSessionBackend {
 public:
  WebGPUSurfaceSessionBackend(
    const mln_webgpu_surface_descriptor& descriptor, mbgl::Size size
  )
      : backend_(descriptor, size) {}

  auto renderer_backend() -> mbgl::gfx::RendererBackend& override {
    return backend_;
  }

  void resize(uint32_t physical_width, uint32_t physical_height) override {
    backend_.resize(mbgl::Size{physical_width, physical_height});
  }

  auto prepare_frame(bool& out_ready) -> mln_status override {
    if (!backend_.acquire_frame(out_ready)) {
      mln::core::set_thread_error("WebGPU surface produced no frame texture");
      return MLN_STATUS_NATIVE_ERROR;
    }
    return MLN_STATUS_OK;
  }

  auto set_webgpu_target(const mln_webgpu_surface_descriptor& descriptor)
    -> mln_status override {
    if (!backend_.matches_context(descriptor.context)) {
      mln::core::set_thread_error(
        "WebGPU surface target must name the device this session attached with"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (!backend_.matches_format(descriptor)) {
      return mln::core::unsupported_retarget(
        "WebGPU surface target must have the format this session's render "
        "pipelines were built for; destroy the session and attach again to "
        "change it"
      );
    }
    backend_.set_surface(descriptor);
    return MLN_STATUS_OK;
  }

 private:
  WebGPUSurfaceBackend backend_;
};

class WebGPUTextureSessionBackend final
    : public mln::core::TextureSessionBackend {
 public:
  WebGPUTextureSessionBackend(
    const mln_webgpu_owned_texture_descriptor& descriptor, mbgl::Size size,
    std::size_t ring_depth
  )
      : backend_(descriptor, size, ring_depth) {}

  WebGPUTextureSessionBackend(
    const mln_webgpu_borrowed_texture_descriptor& descriptor, mbgl::Size size
  )
      : backend_(descriptor, size) {}

  auto headless_backend() -> mbgl::gfx::HeadlessBackend& override {
    return backend_;
  }
  void resize(mbgl::Size size) override { backend_.set_ring_size(size); }

  auto set_webgpu_borrowed_target(
    const mln_webgpu_borrowed_texture_descriptor& descriptor
  ) -> mln_status override {
    if (!backend_.matches_context(descriptor.context)) {
      mln::core::set_thread_error(
        "WebGPU texture target must name the device and queue this session "
        "attached with"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (!backend_.matches_borrowed_target(descriptor)) {
      return mln::core::unsupported_retarget(
        "WebGPU texture target must have the format this session's render "
        "pipelines were built for; destroy the session and attach again to "
        "change it"
      );
    }
    backend_.set_borrowed_target(descriptor);
    return MLN_STATUS_OK;
  }

  auto after_render(mln_render_session_object& session, bool& out_rendered)
    -> mln_status override {
    session.texture.rendered_native_texture = backend_.rendered_texture();
    out_rendered = true;
    return MLN_STATUS_OK;
  }

  auto select_render_slot(std::size_t slot) -> mln_status override {
    return backend_.select_slot(slot) ? MLN_STATUS_OK
                                      : MLN_STATUS_INVALID_ARGUMENT;
  }

  auto copy_slot_metadata(
    const mln_render_session_object& texture, std::size_t slot,
    std::any& out_metadata
  ) -> mln_status override {
    out_metadata = mln_webgpu_owned_texture_frame{
      .size = sizeof(mln_webgpu_owned_texture_frame),
      .generation = texture.generation,
      .width = texture.physical_width,
      .height = texture.physical_height,
      .scale_factor = texture.scale_factor,
      .frame_id = texture.frame_generation,
      .texture = backend_.rendered_texture_at(slot),
      .texture_view = backend_.rendered_texture_view_at(slot),
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

auto webgpu_owned_texture_attach_start(
  mln_map map, const mln_webgpu_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
) -> mln_status {
#if !defined(MLN_RENDER_BACKEND_WEBGPU)
  set_thread_error("WebGPU texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
#else
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_webgpu_owned_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  if (
    options != nullptr &&
    options->driver != MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
  ) {
    set_thread_error(
      "WebGPU targets require the caller graphics thread driver"
    );
    return MLN_STATUS_UNSUPPORTED;
  }
  const auto physical_status = validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled texture dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_session_extent(*session, descriptor->extent);
  session->texture.api_kind = TextureSessionApi::WebGPU;
  session->texture.mode = TextureSessionMode::Owned;
  const auto copied = *descriptor;
  const auto ring_depth = std::clamp(
    options == nullptr ? 1u : options->requested_texture_ring_depth, 1u, 3u
  );
  session->initialize_backend =
    [copied, ring_depth](mln_render_session_object& target) {
      target.texture.backend = std::make_unique<WebGPUTextureSessionBackend>(
        copied, mbgl::Size{target.physical_width, target.physical_height},
        ring_depth
      );
      return MLN_STATUS_OK;
    };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD,
    .texture_ring_depth = ring_depth,
    .flags = MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION |
             MLN_RENDER_SESSION_CAPABILITY_READBACK |
             MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, out_operation
  );
#endif
}

auto webgpu_borrowed_texture_attach_start(
  mln_map map, const mln_webgpu_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
) -> mln_status {
#if !defined(MLN_RENDER_BACKEND_WEBGPU)
  set_thread_error(
    "WebGPU borrowed texture sessions are not supported by this build"
  );
  return MLN_STATUS_UNSUPPORTED;
#else
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_webgpu_borrowed_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  if (
    options != nullptr &&
    options->driver != MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
  ) {
    set_thread_error(
      "WebGPU targets require the caller graphics thread driver"
    );
    return MLN_STATUS_UNSUPPORTED;
  }
  const auto physical_status = validate_borrowed_physical_size(
    descriptor->physical_width, descriptor->physical_height
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_borrowed_session_extent(
    *session, descriptor->extent, descriptor->physical_width,
    descriptor->physical_height
  );
  session->texture.api_kind = TextureSessionApi::WebGPU;
  session->texture.mode = TextureSessionMode::Borrowed;
  const auto copied = *descriptor;
  session->initialize_backend = [copied](mln_render_session_object& target) {
    const auto physical_size =
      mbgl::Size{target.physical_width, target.physical_height};
    const auto texture_status = validate_webgpu_texture(copied, physical_size);
    if (texture_status != MLN_STATUS_OK) {
      return texture_status;
    }
    target.texture.backend =
      std::make_unique<WebGPUTextureSessionBackend>(copied, physical_size);
    return MLN_STATUS_OK;
  };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD,
    .texture_ring_depth = 0,
    .flags = MLN_RENDER_SESSION_CAPABILITY_READBACK
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, out_operation
  );
#endif
}

auto webgpu_borrowed_texture_set_target_start(
  mln_render_session session,
  const mln_webgpu_borrowed_texture_descriptor* descriptor,
  mln_operation* out_operation
) -> mln_status {
  const auto descriptor_status =
    validate_webgpu_borrowed_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto physical_status = validate_borrowed_physical_size(
    descriptor->physical_width, descriptor->physical_height
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  const auto copied = *descriptor;
  return enqueue_driver_operation(
    session, RENDER_OPERATION_MAINTENANCE,
    [copied](mln_render_session_object& target) {
      const auto texture_status = validate_webgpu_texture(
        copied, mbgl::Size{copied.physical_width, copied.physical_height}
      );
      if (texture_status != MLN_STATUS_OK) {
        return texture_status;
      }
      return render_session_set_target(
        target.self, RetargetTargetKind::BorrowedTexture, copied.extent,
        copied.physical_width, copied.physical_height,
        [&copied](mln_render_session_object& live) {
          return live.texture.backend->set_webgpu_borrowed_target(copied);
        }
      );
    },
    out_operation
  );
}

auto webgpu_surface_attach_start(
  mln_map map, const mln_webgpu_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
) -> mln_status {
#if !defined(MLN_RENDER_BACKEND_WEBGPU)
  set_thread_error("WebGPU surface sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
#else
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_webgpu_surface_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  if (
    options != nullptr &&
    options->driver != MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
  ) {
    set_thread_error(
      "WebGPU targets require the caller graphics thread driver"
    );
    return MLN_STATUS_UNSUPPORTED;
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
  const auto copied = *descriptor;
  session->initialize_backend = [copied](mln_render_session_object& target) {
    target.surface.backend = std::make_unique<WebGPUSurfaceSessionBackend>(
      copied, mbgl::Size{target.physical_width, target.physical_height}
    );
    return MLN_STATUS_OK;
  };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD,
    .texture_ring_depth = 0,
    .flags = MLN_RENDER_SESSION_CAPABILITY_PRESENTATION
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Surface, options, capabilities,
    out_session, out_operation
  );
#endif
}

auto webgpu_surface_set_target_start(
  mln_render_session session, const mln_webgpu_surface_descriptor* descriptor,
  mln_operation* out_operation
) -> mln_status {
  const auto descriptor_status = validate_webgpu_surface_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto copied = *descriptor;
  return enqueue_driver_operation(
    session, RENDER_OPERATION_MAINTENANCE,
    [copied](mln_render_session_object& target) {
      return surface_session_set_target(
        target.self, copied.extent, [&copied](mln_render_session_object& live) {
          return live.surface.backend->set_webgpu_target(copied);
        }
      );
    },
    out_operation
  );
}

}  // namespace mln::core
