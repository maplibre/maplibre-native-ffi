#include <string>
#include <utility>

#include "render/opengl/wgl_window.hpp"

#include "diagnostics/diagnostics.hpp"

namespace mln::core::opengl {
namespace {

constexpr auto window_class_name = "mln_offscreen_device";

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

auto pixel_format_descriptor() -> PIXELFORMATDESCRIPTOR {
  auto descriptor = PIXELFORMATDESCRIPTOR{};
  descriptor.nSize = sizeof(descriptor);
  descriptor.nVersion = 1;
  descriptor.dwFlags =
    PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  descriptor.iPixelType = PFD_TYPE_RGBA;
  descriptor.cColorBits = 32;
  descriptor.cAlphaBits = 8;
  descriptor.cDepthBits = 24;
  descriptor.cStencilBits = 8;
  descriptor.iLayerType = PFD_MAIN_PLANE;
  return descriptor;
}

auto other_pixel_format_error() -> mln_status {
  set_thread_error(
    "the window already has a pixel format other than the library's, which "
    "Windows doesn't let it change"
  );
  return MLN_STATUS_INVALID_ARGUMENT;
}

}  // namespace

void set_windows_error(const char* what) {
  const auto message =
    std::string{what} + " failed (" + std::to_string(GetLastError()) + ")";
  set_thread_error(message.c_str());
}

HiddenWindow::~HiddenWindow() {
  if (window_ != nullptr) {
    DestroyWindow(window_);
  }
}

auto HiddenWindow::create(int pixel_format) -> mln_status {
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
    set_thread_error("RegisterClassExA failed for the offscreen device window");
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
  const auto descriptor = pixel_format_descriptor();
  const auto format = pixel_format != 0
                        ? pixel_format
                        : ChoosePixelFormat(device_context_, &descriptor);
  if (
    format == 0 || SetPixelFormat(device_context_, format, &descriptor) == FALSE
  ) {
    set_windows_error("setting the offscreen device window's pixel format");
    return MLN_STATUS_NATIVE_ERROR;
  }
  return MLN_STATUS_OK;
}

WglWindowDeviceContext::WglWindowDeviceContext(
  WglWindowDeviceContext&& other
) noexcept
    : window_(std::exchange(other.window_, nullptr)),
      device_context_(std::exchange(other.device_context_, nullptr)),
      pixel_format_(std::exchange(other.pixel_format_, 0)) {}

auto WglWindowDeviceContext::operator=(WglWindowDeviceContext&& other) noexcept
  -> WglWindowDeviceContext& {
  if (this != &other) {
    reset();
    window_ = std::exchange(other.window_, nullptr);
    device_context_ = std::exchange(other.device_context_, nullptr);
    pixel_format_ = std::exchange(other.pixel_format_, 0);
  }
  return *this;
}

void WglWindowDeviceContext::reset() noexcept {
  if (device_context_ != nullptr) ReleaseDC(window_, device_context_);
  window_ = nullptr;
  device_context_ = nullptr;
  pixel_format_ = 0;
}

auto acquire_wgl_window_device_context(
  HWND window, int pixel_format, WglWindowDeviceContext& out_context
) -> mln_status {
  auto acquired = WglWindowDeviceContext{};
  acquired.window_ = window;
  acquired.device_context_ = GetDC(window);
  if (acquired.device_context_ == nullptr) {
    set_windows_error("GetDC on the window");
    return MLN_STATUS_NATIVE_ERROR;
  }
  const auto descriptor = pixel_format_descriptor();
  const auto format =
    pixel_format != 0
      ? pixel_format
      : ChoosePixelFormat(acquired.device_context_, &descriptor);
  if (format == 0) {
    set_windows_error("ChoosePixelFormat on the window");
    return MLN_STATUS_NATIVE_ERROR;
  }
  const auto current = GetPixelFormat(acquired.device_context_);
  if (current != 0 && current != format) return other_pixel_format_error();
  if (
    current == 0 &&
    SetPixelFormat(acquired.device_context_, format, &descriptor) == FALSE
  ) {
    set_windows_error("setting the window's pixel format");
    return MLN_STATUS_NATIVE_ERROR;
  }
  acquired.pixel_format_ = format;
  out_context = std::move(acquired);
  return MLN_STATUS_OK;
}

auto validate_wgl_window_pixel_format(HWND window) -> mln_status {
  auto* const device_context = GetDC(window);
  if (device_context == nullptr) {
    set_windows_error("GetDC on the window");
    return MLN_STATUS_NATIVE_ERROR;
  }
  const auto current = GetPixelFormat(device_context);
  const auto descriptor = pixel_format_descriptor();
  const auto chosen =
    current == 0 ? 0 : ChoosePixelFormat(device_context, &descriptor);
  ReleaseDC(window, device_context);
  return current != 0 && current != chosen ? other_pixel_format_error()
                                           : MLN_STATUS_OK;
}

}  // namespace mln::core::opengl
