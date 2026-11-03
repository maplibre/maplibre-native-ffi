// clang-format off
#include <cstdint>
#include <memory>
#include <string>

// emdawnwebgpu's header comes first, as in webgpu_texture_session.cpp.
#include <webgpu/webgpu.h>

#include <emscripten/em_js.h>
#include <emscripten/eventloop.h>

#include <mln/util/logging.hpp>

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/offscreen_device.hpp"
#include "render/render_session_common.hpp"
#include "render/webgpu/webgpu_texture_session.hpp"
// clang-format on

// Suspends the calling thread through JSPI until its event loop has run one
// task.
// clang-format off
EM_ASYNC_JS(void, mln_yield_to_event_loop, (), {
  await new Promise((resolve) => setTimeout(resolve, 0));
});
// clang-format on

namespace mln::core {
namespace {

auto text(WGPUStringView view) -> std::string {
  if (view.data == nullptr) return {};
  return view.length == WGPU_STRLEN ? std::string{view.data}
                                    : std::string{view.data, view.length};
}

template <typename Object>
struct Requested {
  Object object = nullptr;
  std::string message;
};

void on_adapter(
  WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message,
  void* user_data, void* /*unused*/
) noexcept {
  auto& requested = *static_cast<Requested<WGPUAdapter>*>(user_data);
  if (status == WGPURequestAdapterStatus_Success) {
    requested.object = adapter;
  } else {
    requested.message = text(message);
  }
}

void on_device(
  WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message,
  void* user_data, void* /*unused*/
) noexcept {
  auto& requested = *static_cast<Requested<WGPUDevice>*>(user_data);
  if (status == WGPURequestDeviceStatus_Success) {
    requested.object = device;
  } else {
    requested.message = text(message);
  }
}

// Suspends the calling thread until `future` completes, and runs its
// callback. Only a thread that started inside the library can suspend.
auto wait_for(WGPUInstance instance, WGPUFuture future) -> bool {
  auto wait = WGPUFutureWaitInfo{.future = future, .completed = false};
  return future.id != 0 &&
         wgpuInstanceWaitAny(instance, 1, &wait, UINT64_MAX) ==
           WGPUWaitStatus_Success &&
         wait.completed;
}

auto fail(const char* summary, const std::string& message) -> mln_status {
  const auto diagnostic = message.empty()
                            ? std::string{summary}
                            : std::string{summary} + ": " + message;
  set_thread_error(diagnostic.c_str());
  return MLN_STATUS_NATIVE_ERROR;
}

// A device that the session's core worker requested, which only that thread
// may use: the browser keeps WebGPU objects in the JavaScript realm of the
// worker that made them.
class WebGPUOffscreenDevice final : public OffscreenDevice {
 public:
  WebGPUOffscreenDevice(WGPUInstance instance, WGPUDevice device)
      : instance_(instance), device_(device) {}
  WebGPUOffscreenDevice(const WebGPUOffscreenDevice&) = delete;
  auto operator=(const WebGPUOffscreenDevice&)
    -> WebGPUOffscreenDevice& = delete;
  WebGPUOffscreenDevice(WebGPUOffscreenDevice&&) = delete;
  auto operator=(WebGPUOffscreenDevice&&) -> WebGPUOffscreenDevice& = delete;
  ~WebGPUOffscreenDevice() override {
    // Destroyed rather than left to the garbage collector, so its GPU memory
    // returns as soon as its session ends.
    const auto lost = wgpuDeviceGetLostFuture(device_);
    wgpuDeviceDestroy(device_);
    // The browser reports the loss later, on this thread's event loop, and
    // emdawnwebgpu then calls into the module on whatever pthread that Web
    // Worker hosts by then, or on the freed stack of none. Unlike its other
    // requests, the report holds no runtime keepalive, so the worker waits
    // for it here.
    auto wait = WGPUFutureWaitInfo{.future = lost, .completed = false};
    static_cast<void>(wgpuInstanceWaitAny(instance_, 1, &wait, UINT64_MAX));
    wgpuDeviceRelease(device_);
    wgpuInstanceRelease(instance_);
    // emdawnwebgpu holds a runtime keepalive for each request still pending,
    // such as a readback's buffer map, until this thread's event loop
    // settles it. A pthread that returns while one is held exits only later,
    // from inside that callback, after which the page's next runtime was seen
    // to hang. So the worker settles them here, before it can return.
    while (emscripten_runtime_keepalive_check()) mln_yield_to_event_loop();
  }

  auto create_texture_backend(
    mln::Size size, std::unique_ptr<TextureSessionBackend>& out_backend
  ) -> mln_status override {
    const auto context = mln_web_gpu_context_descriptor{
      .instance = instance_, .device = device_, .queue = nullptr
    };
    return create_webgpu_offscreen_texture_backend(context, size, out_backend);
  }

 private:
  WGPUInstance instance_;
  WGPUDevice device_;
};

}  // namespace

// Runs only on a session's core worker, during its attach; see
// offscreen_device_create(). The browser answers both requests
// asynchronously, so the worker suspends until it does.
auto create_backend_offscreen_device(
  const mln_offscreen_device_options& options,
  std::shared_ptr<OffscreenDevice>& out_device
) -> mln_status {
  (void)options;
  // Without this feature, emdawnwebgpu rejects every wait that may block.
  const auto timed_wait_any = WGPUInstanceFeatureName_TimedWaitAny;
  const auto instance_descriptor = WGPUInstanceDescriptor{
    .nextInChain = nullptr,
    .requiredFeatureCount = 1,
    .requiredFeatures = &timed_wait_any,
    .requiredLimits = nullptr,
  };
  auto* const instance = wgpuCreateInstance(&instance_descriptor);
  if (instance == nullptr) {
    return fail("creating a WebGPU instance failed", {});
  }

  auto adapter = Requested<WGPUAdapter>{};
  const auto adapter_callback = WGPURequestAdapterCallbackInfo{
    .nextInChain = nullptr,
    .mode = WGPUCallbackMode_WaitAnyOnly,
    .callback = on_adapter,
    .userdata1 = &adapter,
    .userdata2 = nullptr,
  };
  if (
    !wait_for(
      instance, wgpuInstanceRequestAdapter(instance, nullptr, adapter_callback)
    ) ||
    adapter.object == nullptr
  ) {
    wgpuInstanceRelease(instance);
    return fail("the browser offered no WebGPU adapter", adapter.message);
  }

  auto device = Requested<WGPUDevice>{};
  const auto device_callback = WGPURequestDeviceCallbackInfo{
    .nextInChain = nullptr,
    .mode = WGPUCallbackMode_WaitAnyOnly,
    .callback = on_device,
    .userdata1 = &device,
    .userdata2 = nullptr,
  };
  const auto device_ready = wait_for(
    instance, wgpuAdapterRequestDevice(adapter.object, nullptr, device_callback)
  );
  wgpuAdapterRelease(adapter.object);
  if (!device_ready || device.object == nullptr) {
    wgpuInstanceRelease(instance);
    return fail("requesting a WebGPU device failed", device.message);
  }

  mln::Log::Info(mln::Event::Render, "offscreen device uses browser WebGPU");
  out_device = std::make_shared<WebGPUOffscreenDevice>(instance, device.object);
  return MLN_STATUS_OK;
}

}  // namespace mln::core
