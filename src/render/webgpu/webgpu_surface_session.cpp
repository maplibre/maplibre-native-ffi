#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gfx/renderable.hpp>
#include <mbgl/gfx/renderer_backend.hpp>
#include <mbgl/util/logging.hpp>
#include <mbgl/util/size.hpp>
#include <mbgl/webgpu/context.hpp>
#include <mbgl/webgpu/renderable_resource.hpp>
#include <mbgl/webgpu/renderer_backend.hpp>

#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "maplibre_native_c/surface.h"
#include "render/render_session_common.hpp"
#include "render/surface_session.hpp"

namespace {

#if defined(__EMSCRIPTEN__)

void wait_webgpu_future(const wgpu::Instance& instance, wgpu::Future future) {
  if (instance.WaitAny(future, UINT64_MAX) != wgpu::WaitStatus::Success) {
    throw std::runtime_error("WebGPU future wait failed");
  }
}

auto request_adapter_blocking(
  const wgpu::Instance& instance, const wgpu::RequestAdapterOptions& options
) -> wgpu::Adapter {
  wgpu::Adapter adapter;
  auto status = wgpu::RequestAdapterStatus::Error;
  const auto future = instance.RequestAdapter(
    &options,
    wgpu::CallbackMode::WaitAnyOnly,
    [&](wgpu::RequestAdapterStatus callback_status, wgpu::Adapter result,
        wgpu::StringView) {
      status = callback_status;
      adapter = std::move(result);
    }
  );
  wait_webgpu_future(instance, future);
  if (status != wgpu::RequestAdapterStatus::Success || !adapter) {
    throw std::runtime_error("No WebGPU adapter found");
  }
  return adapter;
}

auto request_device_blocking(
  const wgpu::Instance& instance, const wgpu::Adapter& adapter,
  const wgpu::DeviceDescriptor& descriptor
) -> wgpu::Device {
  wgpu::Device device;
  auto status = wgpu::RequestDeviceStatus::Error;
  const auto future = adapter.RequestDevice(
    &descriptor,
    wgpu::CallbackMode::WaitAnyOnly,
    [&](wgpu::RequestDeviceStatus callback_status, wgpu::Device result,
        wgpu::StringView) {
      status = callback_status;
      device = std::move(result);
    }
  );
  wait_webgpu_future(instance, future);
  if (status != wgpu::RequestDeviceStatus::Success || !device) {
    throw std::runtime_error("Failed to create WebGPU device");
  }
  return device;
}

#endif

auto validate_webgpu_descriptor(const mln_webgpu_surface_descriptor* descriptor)
  -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("surface descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_webgpu_surface_descriptor)) {
    mln::core::set_thread_error(
      "mln_webgpu_surface_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = mln::core::validate_render_target_extent(
    descriptor->extent, "surface dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  if (descriptor->context.size < sizeof(mln_webgpu_context_descriptor)) {
    mln::core::set_thread_error(
      "mln_webgpu_context_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
#if defined(__EMSCRIPTEN__)
  if (
    descriptor->canvas_selector == nullptr ||
    descriptor->canvas_selector[0] == '\0'
  ) {
    mln::core::set_thread_error(
      "WebGPU surface sessions require a canvas_selector on Emscripten"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
#endif
  return MLN_STATUS_OK;
}

class WebGPUSurfaceBackend final : public mbgl::webgpu::RendererBackend,
                                   public mbgl::gfx::Renderable {
 public:
  class RenderableResource final : public mbgl::webgpu::RenderableResource {
   public:
    explicit RenderableResource(WebGPUSurfaceBackend& backend_) : backend(backend_) {}

    void bind() override {}

    void swap() override { backend.present(); }

    const mbgl::webgpu::RendererBackend& getBackend() const override {
      return backend;
    }

    const WGPUCommandEncoder& getCommandEncoder() const override {
      static WGPUCommandEncoder dummy = nullptr;
      return dummy;
    }

    WGPURenderPassEncoder getRenderPassEncoder() const override { return nullptr; }

    WGPUTextureView getColorTextureView() override {
      return static_cast<WGPUTextureView>(backend.getCurrentTextureView());
    }

    WGPUTextureView getDepthStencilTextureView() override {
      return static_cast<WGPUTextureView>(backend.getDepthStencilView());
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
        descriptor_(descriptor) {
    initialize();
  }

  ~WebGPUSurfaceBackend() override { shutdown(); }

  auto renderer_backend() -> mbgl::gfx::RendererBackend& { return *this; }

  mbgl::gfx::Renderable& getDefaultRenderable() override { return *this; }

  void resize(mbgl::Size new_size) {
    size = new_size;
    if (surface_ != nullptr && device_ != nullptr) {
      configureSurface();
    }
  }

  void present() {
    if (surface_ == nullptr || !surface_configured_) {
      return;
    }
    if (current_texture_view_ != nullptr && !frame_presented_) {
      wgpuSurfacePresent(surface_);
      frame_presented_ = true;
      wgpuTextureViewRelease(current_texture_view_);
      current_texture_view_ = nullptr;
      if (current_texture_ != nullptr) {
        wgpuTextureRelease(current_texture_);
        current_texture_ = nullptr;
      }
    }
  }

  void* getCurrentTextureView() override {
    if (surface_ == nullptr || !surface_configured_) {
      return nullptr;
    }

    if (current_texture_view_ != nullptr && !frame_presented_) {
      return current_texture_view_;
    }

    releaseCurrentFrame();

    WGPUSurfaceTexture surface_texture{};
    wgpuSurfaceGetCurrentTexture(surface_, &surface_texture);

    const auto texture_status = static_cast<WGPUSurfaceGetCurrentTextureStatus>(
      surface_texture.status
    );
    if (
      texture_status == WGPUSurfaceGetCurrentTextureStatus_Outdated ||
      texture_status == WGPUSurfaceGetCurrentTextureStatus_Lost
    ) {
      if (surface_texture.texture != nullptr) {
        wgpuTextureRelease(surface_texture.texture);
      }
      configureSurface();
      return nullptr;
    }
    if (
      texture_status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
      texture_status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal
    ) {
      if (surface_texture.texture != nullptr) {
        wgpuTextureRelease(surface_texture.texture);
      }
      return nullptr;
    }

    current_texture_ = surface_texture.texture;
    if (current_texture_ == nullptr) {
      return nullptr;
    }

    WGPUTextureViewDescriptor view_desc{};
    view_desc.format = surface_format_;
    view_desc.dimension = WGPUTextureViewDimension_2D;
    view_desc.baseMipLevel = 0;
    view_desc.mipLevelCount = 1;
    view_desc.baseArrayLayer = 0;
    view_desc.arrayLayerCount = 1;
    view_desc.aspect = WGPUTextureAspect_All;
    current_texture_view_ = wgpuTextureCreateView(current_texture_, &view_desc);
    frame_presented_ = false;
    return current_texture_view_;
  }

  void* getDepthStencilView() override {
    ensureDepthStencilTexture();
    return depth_stencil_view_;
  }

  mbgl::Size getFramebufferSize() const override { return size; }

 protected:
  void activate() override {
    if (surface_ != nullptr && !surface_configured_) {
      configureSurface();
    }
  }

  void deactivate() override {}

 private:
  void initialize() {
    if (descriptor_.context.instance != nullptr) {
      instance_ = static_cast<WGPUInstance>(descriptor_.context.instance);
      wgpuInstanceAddRef(instance_);
#if defined(__EMSCRIPTEN__)
      wgpu_instance_ = wgpu::Instance::Acquire(instance_);
#endif
    } else {
#if defined(__EMSCRIPTEN__)
      wgpu::InstanceDescriptor instance_desc{};
      instance_desc.capabilities.timedWaitAnyEnable = true;
      instance_desc.capabilities.timedWaitAnyMaxCount = 64;
      wgpu_instance_ = wgpu::CreateInstance(&instance_desc);
      if (!wgpu_instance_) {
        throw std::runtime_error("Failed to create WebGPU instance");
      }
      instance_ = wgpu_instance_.Get();
      wgpuInstanceAddRef(instance_);
#else
      WGPUInstanceDescriptor instance_desc{};
      instance_ = wgpuCreateInstance(&instance_desc);
      if (instance_ == nullptr) {
        throw std::runtime_error("Failed to create WebGPU instance");
      }
#endif
    }

#if defined(__EMSCRIPTEN__)
    const char* selector = descriptor_.canvas_selector;
    wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvas_source{};
    canvas_source.selector = wgpu::StringView{
      selector, selector != nullptr ? std::strlen(selector) : 0};

    wgpu::SurfaceDescriptor surface_desc{};
    surface_desc.nextInChain = &canvas_source;
    wgpu_surface_ = wgpu_instance_.CreateSurface(&surface_desc);
    if (!wgpu_surface_) {
      throw std::runtime_error("Failed to create WebGPU canvas surface");
    }
    surface_ = wgpu_surface_.Get();
    wgpuSurfaceAddRef(surface_);
#endif

    if (descriptor_.context.device != nullptr) {
      device_ = static_cast<WGPUDevice>(descriptor_.context.device);
      wgpuDeviceAddRef(device_);
      if (descriptor_.context.queue != nullptr) {
        queue_ = static_cast<WGPUQueue>(descriptor_.context.queue);
        wgpuQueueAddRef(queue_);
      } else {
        queue_ = wgpuDeviceGetQueue(device_);
      }
    } else {
      createDevice();
    }

    setInstance(instance_);
    setDevice(device_);
    setQueue(queue_);

#if defined(__EMSCRIPTEN__)
    if (!wgpu_adapter_) {
      wgpu::RequestAdapterOptions adapter_opts{};
      adapter_opts.powerPreference = wgpu::PowerPreference::HighPerformance;
      adapter_opts.compatibleSurface = wgpu_surface_;
      wgpu_adapter_ = request_adapter_blocking(wgpu_instance_, adapter_opts);
    }

    wgpu::SurfaceCapabilities capabilities{};
    if (
      wgpu_surface_.GetCapabilities(wgpu_adapter_, &capabilities) ==
      wgpu::Status::Success
    ) {
      pickSurfaceFormat(capabilities);
    }
    if (surface_format_ == WGPUTextureFormat_Undefined) {
      surface_format_ = WGPUTextureFormat_BGRA8Unorm;
      setColorFormat(wgpu::TextureFormat::BGRA8Unorm);
    }
    configureSurface();
#endif
  }

  void createDevice() {
#if defined(__EMSCRIPTEN__)
    wgpu::RequestAdapterOptions adapter_opts{};
    adapter_opts.powerPreference = wgpu::PowerPreference::HighPerformance;
    if (wgpu_surface_) {
      adapter_opts.compatibleSurface = wgpu_surface_;
    }
    if (!wgpu_adapter_) {
      wgpu_adapter_ = request_adapter_blocking(wgpu_instance_, adapter_opts);
    }

    wgpu::SupportedFeatures features{};
    wgpu_adapter_.GetFeatures(&features);
    std::vector<wgpu::FeatureName> required_features;
    wgpu::TextureFormat depth_format = wgpu::TextureFormat::Depth24PlusStencil8;
    if (features.featureCount > 0 && features.features != nullptr) {
      for (size_t i = 0; i < features.featureCount; ++i) {
        if (features.features[i] == wgpu::FeatureName::Depth32FloatStencil8) {
          required_features.push_back(wgpu::FeatureName::Depth32FloatStencil8);
          depth_format = wgpu::TextureFormat::Depth32FloatStencil8;
          break;
        }
      }
    }
    wgpuSupportedFeaturesFreeMembers(features);

    wgpu::DeviceDescriptor device_desc{};
    device_desc.requiredFeatureCount = required_features.size();
    device_desc.requiredFeatures = required_features.data();
    wgpu_device_ = request_device_blocking(
      wgpu_instance_, wgpu_adapter_, device_desc
    );
    device_ = wgpu_device_.Get();
    wgpuDeviceAddRef(device_);
    wgpu_queue_ = wgpu_device_.GetQueue();
    queue_ = wgpu_queue_.Get();
    wgpuQueueAddRef(queue_);
    setDepthStencilFormat(depth_format);
#else
    WGPURequestAdapterOptions adapter_opts{};
    adapter_opts.powerPreference = WGPUPowerPreference_HighPerformance;
    WGPUAdapter adapter = wgpuInstanceRequestAdapter(instance_, &adapter_opts);
    if (adapter == nullptr) {
      throw std::runtime_error("No WebGPU adapter found");
    }

    WGPUSupportedFeatures features{};
    wgpuAdapterGetFeatures(adapter, &features);
    std::vector<WGPUFeatureName> required_features;
    wgpu::TextureFormat depth_format = wgpu::TextureFormat::Depth24PlusStencil8;
    if (features.featureCount > 0 && features.features != nullptr) {
      for (size_t i = 0; i < features.featureCount; ++i) {
        if (
          features.features[i] ==
          static_cast<WGPUFeatureName>(wgpu::FeatureName::Depth32FloatStencil8)
        ) {
          required_features.push_back(features.features[i]);
          depth_format = wgpu::TextureFormat::Depth32FloatStencil8;
          break;
        }
      }
    }
    wgpuSupportedFeaturesFreeMembers(features);

    WGPUDeviceDescriptor device_desc{};
    device_desc.requiredFeatureCount = required_features.size();
    device_desc.requiredFeatures = required_features.data();
    device_ = wgpuAdapterRequestDevice(adapter, &device_desc);
    wgpuAdapterRelease(adapter);
    if (device_ == nullptr) {
      throw std::runtime_error("Failed to create WebGPU device");
    }
    queue_ = wgpuDeviceGetQueue(device_);
    setDepthStencilFormat(depth_format);
#endif
  }

  void pickSurfaceFormat(const wgpu::SurfaceCapabilities& capabilities) {
    const wgpu::TextureFormat preferred_formats[] = {
      wgpu::TextureFormat::BGRA8Unorm,
      wgpu::TextureFormat::BGRA8UnormSrgb,
      wgpu::TextureFormat::RGBA8Unorm,
      wgpu::TextureFormat::RGBA8UnormSrgb,
    };
    for (const auto format : preferred_formats) {
      for (size_t i = 0; i < capabilities.formatCount; ++i) {
        if (capabilities.formats[i] == format) {
          surface_format_ = static_cast<WGPUTextureFormat>(format);
          setColorFormat(format);
          return;
        }
      }
    }
    if (capabilities.formatCount > 0) {
      surface_format_ = static_cast<WGPUTextureFormat>(capabilities.formats[0]);
      setColorFormat(capabilities.formats[0]);
    }
  }

  void pickSurfaceFormat(const WGPUSurfaceCapabilities& capabilities) {
    const wgpu::TextureFormat preferred_formats[] = {
      wgpu::TextureFormat::BGRA8Unorm,
      wgpu::TextureFormat::BGRA8UnormSrgb,
      wgpu::TextureFormat::RGBA8Unorm,
      wgpu::TextureFormat::RGBA8UnormSrgb,
    };
    for (const auto format : preferred_formats) {
      for (size_t i = 0; i < capabilities.formatCount; ++i) {
        if (capabilities.formats[i] == static_cast<WGPUTextureFormat>(format)) {
          surface_format_ = capabilities.formats[i];
          setColorFormat(format);
          return;
        }
      }
    }
    if (capabilities.formatCount > 0) {
      surface_format_ = capabilities.formats[0];
      setColorFormat(static_cast<wgpu::TextureFormat>(surface_format_));
    }
  }

  void configureSurface() {
    if (surface_ == nullptr || device_ == nullptr) {
      return;
    }

    WGPUSurfaceConfiguration config = WGPU_SURFACE_CONFIGURATION_INIT;
    config.device = device_;
    config.format = surface_format_;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.width = size.width;
    config.height = size.height;
    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Opaque;
    wgpuSurfaceConfigure(surface_, &config);
    surface_configured_ = true;
    ensureDepthStencilTexture();
  }

  void ensureDepthStencilTexture() {
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

    const auto depth_format = static_cast<WGPUTextureFormat>(getDepthStencilFormat());
    WGPUTextureDescriptor depth_desc{};
    depth_desc.usage = WGPUTextureUsage_RenderAttachment;
    depth_desc.dimension = WGPUTextureDimension_2D;
    depth_desc.size = {size.width, size.height, 1};
    depth_desc.format = depth_format;
    depth_desc.mipLevelCount = 1;
    depth_desc.sampleCount = 1;
    depth_stencil_texture_ = wgpuDeviceCreateTexture(device_, &depth_desc);

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
    depth_stencil_size_ = size;
  }

  void releaseCurrentFrame() {
    if (current_texture_view_ != nullptr) {
      wgpuTextureViewRelease(current_texture_view_);
      current_texture_view_ = nullptr;
    }
    if (current_texture_ != nullptr) {
      wgpuTextureRelease(current_texture_);
      current_texture_ = nullptr;
    }
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
  }

  void shutdown() {
    releaseCurrentFrame();
    releaseDepthStencilTexture();
    if (surface_ != nullptr && surface_configured_) {
      wgpuSurfaceUnconfigure(surface_);
      surface_configured_ = false;
    }
    if (surface_ != nullptr) {
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

  mln_webgpu_surface_descriptor descriptor_;
  WGPUInstance instance_ = nullptr;
  WGPUDevice device_ = nullptr;
  WGPUQueue queue_ = nullptr;
  WGPUSurface surface_ = nullptr;
  WGPUTextureFormat surface_format_ = WGPUTextureFormat_Undefined;
  bool surface_configured_ = false;
  WGPUTexture current_texture_ = nullptr;
  WGPUTextureView current_texture_view_ = nullptr;
  bool frame_presented_ = true;
  WGPUTexture depth_stencil_texture_ = nullptr;
  WGPUTextureView depth_stencil_view_ = nullptr;
  mbgl::Size depth_stencil_size_{0, 0};
#if defined(__EMSCRIPTEN__)
  wgpu::Instance wgpu_instance_;
  wgpu::Adapter wgpu_adapter_;
  wgpu::Device wgpu_device_;
  wgpu::Queue wgpu_queue_;
  wgpu::Surface wgpu_surface_;
#endif
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

 private:
  WebGPUSurfaceBackend backend_;
};

}  // namespace

namespace mln::core {

auto webgpu_surface_descriptor_default() noexcept
  -> mln_webgpu_surface_descriptor {
  return mln_webgpu_surface_descriptor{
    .size = sizeof(mln_webgpu_surface_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 800,
        .height = 600,
        .scale_factor = 1.0,
      },
    .context =
      mln_webgpu_context_descriptor{
        .size = sizeof(mln_webgpu_context_descriptor),
        .instance = nullptr,
        .device = nullptr,
        .queue = nullptr,
      },
    .canvas_selector = "#canvas",
  };
}

auto webgpu_surface_attach(
  mln_map* map, const mln_webgpu_surface_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
#if !defined(MLN_RENDER_BACKEND_WEBGPU)
  set_thread_error("WebGPU surface sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
#else
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_webgpu_descriptor(descriptor);
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

  try {
    auto session = std::make_unique<mln_render_session>();
    session->map = map;
    session->owner_thread = map_owner_thread(map);
    set_session_extent(*session, descriptor->extent);
    session->surface.backend = std::make_unique<WebGPUSurfaceSessionBackend>(
      *descriptor, mbgl::Size{session->physical_width, session->physical_height}
    );
    return attach_render_session(
      std::move(session), out_session, RenderSessionKind::Surface,
      RenderSessionAttachMessages{
        .null_session = "surface session must not be null",
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

}  // namespace mln::core
