// clang-format off
#include <functional>
#include <memory>
#include <string>
#include <utility>

// emdawnwebgpu's header comes first, as in webgpu_texture_session.cpp.
#include <webgpu/webgpu.h>

#include <emscripten/em_js.h>

#include <mln/util/logging.hpp>

#include "maplibre_native_c.h"
#include "render/offscreen_device.hpp"
#include "render/render_session_common.hpp"
#include "render/webgpu/webgpu_texture_session.hpp"
// clang-format on

// Keeps the calling thread alive until the browser reports that `device` is
// lost. emdawnwebgpu calls into the module from that report on the thread that
// requested the device, but unlike its other requests holds no runtime
// keepalive for it. Without one, the thread can exit first, and the report
// then runs on whatever pthread the Web Worker hosts by then, or on the freed
// stack of none. emdawnwebgpu subscribed to the report when it made the
// device, so its handler runs before this one releases the keepalive. Call it
// before releasing the device, while its handle still names the object.
// clang-format off
EM_JS_DEPS(mln_webgpu_device_loss, "$WebGPU,$callUserCallback,$runtimeKeepalivePush,$runtimeKeepalivePop");
EM_JS(void, mln_keep_thread_until_device_lost, (WGPUDevice device), {
  runtimeKeepalivePush();
  WebGPU.getJsObject(device).lost.then(() => callUserCallback(runtimeKeepalivePop));
});
// clang-format on

namespace mln::core {
namespace {

auto text(WGPUStringView view) -> std::string {
  if (view.data == nullptr) return {};
  return view.length == WGPU_STRLEN ? std::string{view.data}
                                    : std::string{view.data, view.length};
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
  // Runs on the core worker. Abandon keeps a backend that only its driver
  // may destroy, so a session that never detaches never gets here, and its
  // device never reports a loss that would need the thread.
  ~WebGPUOffscreenDevice() override {
    // Destroyed rather than left to the garbage collector, so its GPU memory
    // returns as soon as its session ends. The browser reports the loss
    // later, on this thread's event loop, which the worker returns to.
    mln_keep_thread_until_device_lost(device_);
    wgpuDeviceDestroy(device_);
    wgpuDeviceRelease(device_);
    wgpuInstanceRelease(instance_);
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

using DeviceDone =
  std::function<void(std::shared_ptr<OffscreenDevice>, std::string)>;

// One request, from the instance to the device. The browser answers each step
// from this thread's event loop, and each callback owns the request until it
// hands it to the next step or finishes.
struct DeviceRequest {
  WGPUInstance instance = nullptr;
  WGPUAdapter adapter = nullptr;
  DeviceDone done;

  ~DeviceRequest() {
    if (adapter != nullptr) wgpuAdapterRelease(adapter);
    if (instance != nullptr) wgpuInstanceRelease(instance);
  }

  void fail(const char* summary, WGPUStringView message = {}) noexcept {
    auto reason = std::string{};
    try {
      reason = summary;
      if (const auto detail = text(message); !detail.empty())
        reason += ": " + detail;
    } catch (...) {
      // The attach then reports a failure without its reason.
    }
    done(nullptr, std::move(reason));
  }
};

void on_device(
  WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message,
  void* user_data, void* /*unused*/
) noexcept {
  auto request =
    std::unique_ptr<DeviceRequest>{static_cast<DeviceRequest*>(user_data)};
  if (status != WGPURequestDeviceStatus_Success || device == nullptr) {
    request->fail("requesting a WebGPU device failed", message);
    return;
  }
  auto offscreen = std::shared_ptr<OffscreenDevice>{};
  try {
    offscreen =
      std::make_shared<WebGPUOffscreenDevice>(request->instance, device);
    request->instance = nullptr;
  } catch (...) {
    mln_keep_thread_until_device_lost(device);
    wgpuDeviceDestroy(device);
    wgpuDeviceRelease(device);
    request->fail("allocating the WebGPU offscreen device failed");
    return;
  }
  mln::Log::Info(mln::Event::Render, "offscreen device uses browser WebGPU");
  request->done(std::move(offscreen), {});
}

void on_adapter(
  WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message,
  void* user_data, void* /*unused*/
) noexcept {
  auto request =
    std::unique_ptr<DeviceRequest>{static_cast<DeviceRequest*>(user_data)};
  if (status != WGPURequestAdapterStatus_Success || adapter == nullptr) {
    request->fail("the browser offered no WebGPU adapter", message);
    return;
  }
  request->adapter = adapter;
  const auto callback = WGPURequestDeviceCallbackInfo{
    .nextInChain = nullptr,
    .mode = WGPUCallbackMode_AllowSpontaneous,
    .callback = on_device,
    .userdata1 = request.get(),
    .userdata2 = nullptr,
  };
  if (wgpuAdapterRequestDevice(adapter, nullptr, callback).id == 0) {
    request->fail("the browser refused the WebGPU device request");
    return;
  }
  static_cast<void>(request.release());
}

}  // namespace

// Runs only on a session's core worker, before it takes the attach; see
// offscreen_attach_start().
auto request_backend_offscreen_device(DeviceDone done) noexcept -> void {
  auto request = std::unique_ptr<DeviceRequest>{};
  try {
    request = std::make_unique<DeviceRequest>();
  } catch (...) {
    done(nullptr, {});
    return;
  }
  request->done = std::move(done);
  request->instance = wgpuCreateInstance(nullptr);
  if (request->instance == nullptr) {
    request->fail("creating a WebGPU instance failed");
    return;
  }
  const auto callback = WGPURequestAdapterCallbackInfo{
    .nextInChain = nullptr,
    .mode = WGPUCallbackMode_AllowSpontaneous,
    .callback = on_adapter,
    .userdata1 = request.get(),
    .userdata2 = nullptr,
  };
  if (
    wgpuInstanceRequestAdapter(request->instance, nullptr, callback).id == 0
  ) {
    request->fail("the browser refused the WebGPU adapter request");
    return;
  }
  static_cast<void>(request.release());
}

}  // namespace mln::core
