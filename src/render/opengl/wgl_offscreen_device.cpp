#include <memory>
#include <string>
#include <utility>

#include <mln/util/logging.hpp>

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/offscreen_device.hpp"
#include "render/opengl/opengl_texture_session.hpp"
#include "render/opengl/wgl_window.hpp"
#include "render/render_session_common.hpp"

namespace mln::core {
namespace {

using opengl::HiddenWindow;

class WglOffscreenDevice final : public OffscreenDevice {
 public:
  auto create_texture_backend(
    mln::Size size, std::unique_ptr<TextureSessionBackend>& out_backend
  ) -> mln_status override {
    auto window = std::make_shared<HiddenWindow>();
    const auto window_status = window->create();
    if (window_status != MLN_STATUS_OK) {
      return window_status;
    }
    auto descriptor = mln_open_gl_context_descriptor{};
    descriptor.platform = MLN_OPEN_GL_CONTEXT_PLATFORM_WGL;
    descriptor.ownership = MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED;
    descriptor.data.wgl.device_context = window->device_context();
    const auto status =
      create_opengl_offscreen_texture_backend(descriptor, size, out_backend);
    if (out_backend != nullptr) {
      out_backend->retain(std::move(window));
    }
    return status;
  }
};

}  // namespace

auto create_backend_offscreen_device(
  const mln_offscreen_device_options& options,
  std::shared_ptr<OffscreenDevice>& out_device
) -> mln_status {
  if (options.client_api == MLN_OPEN_GL_CLIENT_API_GLES) {
    set_thread_error("WGL creates desktop OpenGL contexts, not OpenGL ES");
    return MLN_STATUS_UNSUPPORTED;
  }
  if (
    (options.fields & MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX) != 0U &&
    options.adapter_index != 0
  ) {
    const auto message = "adapter_index " +
                         std::to_string(options.adapter_index) +
                         " is past the one WGL adapter";
    set_thread_error(message.c_str());
    return MLN_STATUS_NATIVE_ERROR;
  }
  // A session makes its window on its own thread, so this one only checks
  // that the driver offers the pixel format.
  auto probe = HiddenWindow{};
  const auto status = probe.create();
  if (status != MLN_STATUS_OK) {
    return status;
  }
  mln::Log::Info(mln::Event::Render, "offscreen device uses WGL");
  out_device = std::make_shared<WglOffscreenDevice>();
  return MLN_STATUS_OK;
}

}  // namespace mln::core
