#include <memory>
#include <string>
#include <utility>

#include <mln/util/logging.hpp>

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/graphics_device.hpp"
#include "render/opengl/opengl_texture_session.hpp"
#include "render/opengl/wgl_common.hpp"
#include "render/render_session_common.hpp"

namespace mln::core {
namespace {

constexpr auto window_class_name = "mln_graphics_device";

// The module that holds this code, so the window class belongs to the library
// whether it is linked as a DLL or into the host's executable.
auto library_module() -> HMODULE {
  auto module = HMODULE{};
  GetModuleHandleExA(
    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
    reinterpret_cast<LPCSTR>(&library_module), &module
  );
  return module;
}

auto set_windows_error(const char* call) -> void {
  const auto message =
    std::string{call} + " failed (" + std::to_string(GetLastError()) + ")";
  set_thread_error(message.c_str());
}

// A hidden window whose device context carries the pixel format that a
// dedicated session's context needs. Only the thread that creates a window may
// destroy it, so each session makes its own on its core worker. Windows
// destroys a thread's windows when the thread exits, which reclaims one that
// a kept backend never destroys.
class HiddenWindow {
 public:
  HiddenWindow() = default;
  HiddenWindow(const HiddenWindow&) = delete;
  auto operator=(const HiddenWindow&) -> HiddenWindow& = delete;
  HiddenWindow(HiddenWindow&&) = delete;
  auto operator=(HiddenWindow&&) -> HiddenWindow& = delete;
  ~HiddenWindow() {
    if (window_ != nullptr) {
      DestroyWindow(window_);
    }
  }

  auto create() -> mln_status {
    static const auto registered = [] {
      auto window_class = WNDCLASSEXA{};
      window_class.cbSize = sizeof(window_class);
      window_class.style = CS_OWNDC;
      window_class.lpfnWndProc = DefWindowProcA;
      window_class.hInstance = library_module();
      window_class.lpszClassName = window_class_name;
      return RegisterClassExA(&window_class) != 0 ||
             GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    }();
    if (!registered) {
      set_thread_error(
        "RegisterClassExA failed for the graphics device window"
      );
      return MLN_STATUS_NATIVE_ERROR;
    }
    window_ = CreateWindowExA(
      0, window_class_name, window_class_name, WS_POPUP, 0, 0, 8, 8, nullptr,
      nullptr, library_module(), nullptr
    );
    if (window_ == nullptr) {
      set_windows_error("CreateWindowExA");
      return MLN_STATUS_NATIVE_ERROR;
    }
    // The class owns this device context, so it needs no release and stays
    // valid until the window is destroyed.
    device_context_ = GetDC(window_);
    auto pixel_format = PIXELFORMATDESCRIPTOR{};
    pixel_format.nSize = sizeof(pixel_format);
    pixel_format.nVersion = 1;
    pixel_format.dwFlags =
      PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pixel_format.iPixelType = PFD_TYPE_RGBA;
    pixel_format.cColorBits = 32;
    pixel_format.cAlphaBits = 8;
    pixel_format.cDepthBits = 24;
    pixel_format.cStencilBits = 8;
    pixel_format.iLayerType = PFD_MAIN_PLANE;
    const auto format = ChoosePixelFormat(device_context_, &pixel_format);
    if (
      format == 0 ||
      SetPixelFormat(device_context_, format, &pixel_format) == FALSE
    ) {
      set_windows_error("setting the graphics device window's pixel format");
      return MLN_STATUS_NATIVE_ERROR;
    }
    return MLN_STATUS_OK;
  }

  [[nodiscard]] auto device_context() const -> HDC { return device_context_; }

 private:
  HWND window_ = nullptr;
  HDC device_context_ = nullptr;
};

class WglGraphicsDevice final : public GraphicsDevice {
 public:
  auto create_texture_backend(
    mln::Size size, std::unique_ptr<TextureSessionBackend>& out_backend
  ) -> mln_status override {
    auto window = std::make_shared<HiddenWindow>();
    const auto window_status = window->create();
    if (window_status != MLN_STATUS_OK) {
      return window_status;
    }
    auto descriptor = mln_open_gl_owned_texture_descriptor{};
    descriptor.size = sizeof(descriptor);
    descriptor.context.platform = MLN_OPEN_GL_CONTEXT_PLATFORM_WGL;
    descriptor.context.ownership = MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED;
    descriptor.context.data.wgl.device_context = window->device_context();
    const auto status =
      create_opengl_owned_texture_backend(descriptor, size, 1, out_backend);
    if (out_backend != nullptr) {
      out_backend->retain(std::move(window));
    }
    return status;
  }
};

}  // namespace

auto create_backend_graphics_device(
  const mln_graphics_device_options& options,
  std::shared_ptr<GraphicsDevice>& out_device
) -> mln_status {
  if (
    (options.fields & MLN_GRAPHICS_DEVICE_OPTION_ADAPTER_INDEX) != 0U &&
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
  mln::Log::Info(mln::Event::Render, "graphics device uses WGL");
  out_device = std::make_shared<WglGraphicsDevice>();
  return MLN_STATUS_OK;
}

}  // namespace mln::core
