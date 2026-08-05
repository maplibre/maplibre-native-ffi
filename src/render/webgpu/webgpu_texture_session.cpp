// clang-format off
#include <atomic>
#include <cstdint>
#include <cstring>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>

// The WebGPU backend is a browser backend: it is built only against the
// emdawnwebgpu port, which is what supplies webgpu.h. See
// cmake/render/webgpu.cmake.
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
#include "render/texture_session.hpp"
// clang-format on

namespace {

constexpr auto webgpu_owned_color_format = WGPUTextureFormat_RGBA8Unorm;

// WebGPU pads every row of a texture-to-buffer copy to this many bytes, so a
// readback stages the padded rows and unpacks them.
constexpr uint32_t readback_row_alignment = 256;
constexpr uint32_t readback_bytes_per_pixel = 4;
// Five seconds of yielding, which a map that is going to complete beats by
// orders of magnitude.
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
    const mln_webgpu_owned_texture_descriptor& descriptor, mbgl::Size size
  )
      : mbgl::webgpu::RendererBackend(mbgl::gfx::ContextMode::Shared),
        mbgl::gfx::HeadlessBackend(size),
        owns_color_texture_(true),
        color_format_(webgpu_owned_color_format) {
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
      : mbgl::webgpu::RendererBackend(mbgl::gfx::ContextMode::Shared),
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

  // Copies the rendered texture into a mappable buffer and waits for the map,
  // which is how WebGPU reads a texture back.
  //
  // Only a session-owned texture arrives here: the C API offers readback for
  // owned sessions alone, and a caller's texture need not carry CopySrc usage
  // at all. ensureColorTexture() gives the owned one that usage.
  //
  // Upstream blocks the same map on wgpuInstanceWaitAny
  // (mbgl/webgpu/offscreen_texture.cpp), which a session cannot: a non-zero
  // WaitAny timeout is legal only on an instance created with
  // timedWaitAnyEnable, and this instance belongs to the host and is optional
  // besides. A spontaneous callback needs no instance, and yielding to the
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

  // Whether a replacement target names the context this session attached with.
  //
  // The device is what the renderer allocated its resources on, so it has to be
  // the same one. The queue matters because a replacement is taken without
  // re-reading it, so one naming another queue would be silently ignored. A
  // null queue names the device's default queue here exactly as it does at
  // attach, which makes the two spellings of that queue the same context. The
  // instance is left out: a texture session never uses it, which is why the
  // descriptor documents it as optional.
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
    // The replacement is referenced before the outgoing pair is let go, so a
    // descriptor that hands back the texture this session already holds cannot
    // drop its last reference partway through.
    wgpuTextureAddRef(texture);
    wgpuTextureViewAddRef(view);
    releaseColorTexture();
    texture_ = texture;
    color_view_ = view;
    // Drops the renderable resource, which this backend rebuilds lazily, and
    // leaves the depth and stencil attachments to follow the new size the next
    // time the renderable binds.
    setSize(mbgl::Size{descriptor.physical_width, descriptor.physical_height});
  }

 protected:
  void activate() override {
    ensureColorTexture();
    ensureDepthStencilTexture();
  }

  void deactivate() override {}

 private:
  // Submits the texture-to-buffer copy on the session's queue. Ordered behind
  // the render commands already submitted there, so it reads the drawn frame
  // without a fence of its own.
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

  // The map callback's own state, held by shared_ptr because a wait that gives
  // up outlives neither. WebGPU still delivers the callback afterwards -- with
  // Aborted, once releasing the buffer cancels the map -- so a state on this
  // stack would be written through after the frame is gone.
  struct MapState {
    std::atomic<WGPUMapAsyncStatus> status{WGPUMapAsyncStatus_Error};
    std::atomic_bool completed{false};
  };

  // Maps the staged copy and unpacks its padded rows into an image.
  //
  // The map resolves on the browser's job queue, so this thread yields rather
  // than spins: emscripten_sleep suspends through JSPI and lets that queue run,
  // which is also what delivers the spontaneous callback. Bounded, so a device
  // that never answers fails the read instead of hanging the caller.
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
      // Takes back the reference handed to the callback, so the state lives
      // exactly until the one delivery WebGPU promises.
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

  auto acquire_webgpu_owned_frame(
    const mln_render_session_object& texture,
    mln_webgpu_owned_texture_frame& out_frame
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
  mln_map map, const mln_webgpu_owned_texture_descriptor* descriptor,
  mln_render_session* out_session
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
    auto session = std::make_shared<mln_render_session_object>();
    session->map = map;
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
  mln_map map, const mln_webgpu_borrowed_texture_descriptor* descriptor,
  mln_render_session* out_session
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
  const auto output_status = validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto physical_status = validate_borrowed_physical_size(
    descriptor->physical_width, descriptor->physical_height
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  const auto physical_size =
    mbgl::Size{descriptor->physical_width, descriptor->physical_height};
  const auto texture_status =
    validate_webgpu_texture(*descriptor, physical_size);
  if (texture_status != MLN_STATUS_OK) {
    return texture_status;
  }

  try {
    auto session = std::make_shared<mln_render_session_object>();
    session->map = map;
    set_borrowed_session_extent(
      *session, descriptor->extent, descriptor->physical_width,
      descriptor->physical_height
    );
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

auto webgpu_borrowed_texture_set_target(
  mln_render_session session,
  const mln_webgpu_borrowed_texture_descriptor* descriptor
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto session_status = validate_render_session_retarget(
    session, RetargetTargetKind::BorrowedTexture, live
  );
  if (session_status != MLN_STATUS_OK) {
    return session_status;
  }
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
  // The same probe attach makes. Without it a texture of the wrong shape, size,
  // format, or usage is taken here and only fails on the next render, with the
  // outgoing target already let go.
  const auto texture_status = validate_webgpu_texture(
    *descriptor,
    mbgl::Size{descriptor->physical_width, descriptor->physical_height}
  );
  if (texture_status != MLN_STATUS_OK) {
    return texture_status;
  }
  return render_session_set_target(
    session, RetargetTargetKind::BorrowedTexture, descriptor->extent,
    descriptor->physical_width, descriptor->physical_height,
    [descriptor](mln_render_session_object& target_session) -> mln_status {
      return target_session.texture.backend->set_webgpu_borrowed_target(
        *descriptor
      );
    }
  );
}

auto webgpu_owned_texture_acquire_frame(
  mln_render_session texture, mln_webgpu_owned_texture_frame* out_frame
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_texture(texture, live);
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
  if (live->texture.acquired) {
    set_thread_error("a texture frame is already acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (live->rendered_generation != live->generation) {
    set_thread_error("no rendered frame is available for this generation");
    return MLN_STATUS_INVALID_STATE;
  }
  if (
    live->texture.mode != TextureSessionMode::Owned ||
    live->texture.api_kind != TextureSessionApi::WebGPU
  ) {
    set_thread_error("texture session cannot expose a WebGPU texture frame");
    return MLN_STATUS_UNSUPPORTED;
  }

  const auto acquire_status =
    live->texture.backend->acquire_webgpu_owned_frame(*live, *out_frame);
  if (acquire_status != MLN_STATUS_OK) {
    return acquire_status;
  }
  live->texture.acquired_native_texture = out_frame->texture;
  live->texture.acquired = true;
  live->texture.acquired_frame_id = out_frame->frame_id;
  live->texture.acquired_frame_kind = TextureSessionFrameKind::WebGPUOwned;
  ++live->texture.next_frame_id;
  return MLN_STATUS_OK;
}

auto webgpu_owned_texture_release_frame(
  mln_render_session texture, const mln_webgpu_owned_texture_frame* frame
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_texture(texture, live);
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
    !live->texture.acquired ||
    live->texture.acquired_frame_kind != TextureSessionFrameKind::WebGPUOwned
  ) {
    set_thread_error("no texture frame is currently acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (frame->generation != live->generation) {
    set_thread_error("frame generation does not match acquired frame");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (frame->frame_id != live->texture.acquired_frame_id) {
    set_thread_error("frame identity does not match acquired frame");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  live->texture.acquired = false;
  live->texture.acquired_frame_id = 0;
  live->texture.acquired_frame_kind = TextureSessionFrameKind::None;
  live->texture.acquired_native_texture = nullptr;
  return MLN_STATUS_OK;
}

}  // namespace mln::core
