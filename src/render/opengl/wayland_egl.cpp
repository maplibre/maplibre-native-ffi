#include "render/opengl/wayland_egl.hpp"

#if defined(MLN_FFI_WAYLAND) && defined(MLN_FFI_OPENGL_PROVIDER_EGL)

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <EGL/egl.h>
#include <dlfcn.h>

#include "diagnostics/diagnostics.hpp"

namespace mln::core::opengl {
namespace {

// EGL_PLATFORM_WAYLAND_KHR, from EGL_KHR_platform_wayland.
constexpr auto egl_platform_wayland = EGLenum{0x31D8};

using GetPlatformDisplay = EGLDisplay (*)(
  EGLenum platform, void* native_display, const EGLint* attributes
);

auto egl_failure(const char* what) -> mln_status {
  const auto message =
    std::string{what} + " (EGL error " + std::to_string(eglGetError()) + ")";
  set_thread_error(message.c_str());
  return MLN_STATUS_NATIVE_ERROR;
}

struct DisplayEntry {
  wl_display* connection = nullptr;
  WindowEglConfig config{};
  // Whether the library initialized the display, and so terminates it.
  bool initialized_here = false;
  std::size_t users = 0;
};

// Counted by hand rather than by weak pointers, so that the last user
// terminates the display before another can find it still initialized and
// take it for the host's.
struct DisplayRegistry {
  std::mutex mutex;
  std::vector<DisplayEntry> entries;
};

auto display_registry() -> DisplayRegistry& {
  // Leaked, so a session that lets go of its display while the process exits
  // never finds this destroyed.
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  static auto* const registry = new DisplayRegistry();
  return *registry;
}

auto open_display(wl_display* connection, DisplayEntry& out_entry)
  -> mln_status {
  // EGL returns extension functions as untyped procedure pointers.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto get_platform_display = reinterpret_cast<GetPlatformDisplay>(
    eglGetProcAddress("eglGetPlatformDisplayEXT")
  );
  if (get_platform_display == nullptr) {
    set_thread_error("EGL can't open displays by platform");
    return MLN_STATUS_NATIVE_ERROR;
  }
  auto* const display =
    get_platform_display(egl_platform_wayland, connection, nullptr);
  if (display == EGL_NO_DISPLAY) {
    return egl_failure("EGL can't open the Wayland display");
  }
  // EGL answers a query only on an initialized display.
  const auto initialized_here = eglQueryString(display, EGL_VENDOR) == nullptr;
  if (
    initialized_here && eglInitialize(display, nullptr, nullptr) == EGL_FALSE
  ) {
    return egl_failure("initializing the Wayland EGL display failed");
  }
  auto config = EGLConfig{};
  const auto config_status =
    choose_window_egl_config(display, "Wayland EGL display", config);
  if (config_status != MLN_STATUS_OK) {
    if (initialized_here) eglTerminate(display);
    return config_status;
  }
  out_entry = DisplayEntry{
    .connection = connection,
    .config = {.display = display, .config = config},
    .initialized_here = initialized_here,
    .users = 1,
  };
  return MLN_STATUS_OK;
}

struct WaylandEglFunctions {
  wl_egl_window* (*create)(wl_surface* surface, int width, int height);
  void (*destroy)(wl_egl_window* window);
  void (*resize)(wl_egl_window* window, int width, int height, int dx, int dy);
};

// Loaded rather than linked, so a host without libwayland-egl can still load
// the library.
auto wayland_egl_functions() -> const WaylandEglFunctions* {
  static const auto functions = []() -> WaylandEglFunctions {
    auto* const library = dlopen("libwayland-egl.so.1", RTLD_LAZY | RTLD_LOCAL);
    if (library == nullptr) return {};
    // dlsym returns untyped pointers.
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    return WaylandEglFunctions{
      .create = reinterpret_cast<wl_egl_window* (*)(wl_surface*, int, int)>(
        dlsym(library, "wl_egl_window_create")
      ),
      .destroy = reinterpret_cast<void (*)(wl_egl_window*)>(
        dlsym(library, "wl_egl_window_destroy")
      ),
      .resize = reinterpret_cast<void (*)(wl_egl_window*, int, int, int, int)>(
        dlsym(library, "wl_egl_window_resize")
      ),
    };
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
  }();
  if (
    functions.create == nullptr || functions.destroy == nullptr ||
    functions.resize == nullptr
  ) {
    return nullptr;
  }
  return &functions;
}

}  // namespace

WaylandEglDisplay::WaylandEglDisplay(WaylandEglDisplay&& other) noexcept
    : connection_(std::exchange(other.connection_, nullptr)),
      config_(other.config_) {}

auto WaylandEglDisplay::operator=(WaylandEglDisplay&& other) noexcept
  -> WaylandEglDisplay& {
  if (this != &other) {
    reset();
    connection_ = std::exchange(other.connection_, nullptr);
    config_ = other.config_;
  }
  return *this;
}

WaylandEglDisplay::~WaylandEglDisplay() { reset(); }

void WaylandEglDisplay::reset() noexcept {
  if (connection_ == nullptr) return;
  auto& registry = display_registry();
  const auto lock = std::scoped_lock{registry.mutex};
  const auto found =
    std::ranges::find(registry.entries, connection_, &DisplayEntry::connection);
  if (found != registry.entries.end() && --found->users == 0) {
    if (found->initialized_here) eglTerminate(found->config.display);
    registry.entries.erase(found);
  }
  connection_ = nullptr;
}

auto acquire_wayland_egl_display(
  wl_display* connection, WaylandEglDisplay& out_display
) -> mln_status {
  // Releasing a held display takes the registry's lock.
  out_display.reset();
  auto& registry = display_registry();
  const auto lock = std::scoped_lock{registry.mutex};
  auto found =
    std::ranges::find(registry.entries, connection, &DisplayEntry::connection);
  if (found != registry.entries.end()) {
    ++found->users;
  } else {
    auto entry = DisplayEntry{};
    const auto status = open_display(connection, entry);
    if (status != MLN_STATUS_OK) return status;
    registry.entries.push_back(entry);
    found = std::prev(registry.entries.end());
  }
  out_display.connection_ = connection;
  out_display.config_ = found->config;
  return MLN_STATUS_OK;
}

WaylandEglWindow::WaylandEglWindow(WaylandEglWindow&& other) noexcept
    : window_(std::exchange(other.window_, nullptr)) {}

auto WaylandEglWindow::operator=(WaylandEglWindow&& other) noexcept
  -> WaylandEglWindow& {
  if (this != &other) {
    reset();
    window_ = std::exchange(other.window_, nullptr);
  }
  return *this;
}

WaylandEglWindow::~WaylandEglWindow() { reset(); }

void WaylandEglWindow::reset() noexcept {
  // A window exists only once the functions loaded.
  if (window_ != nullptr) wayland_egl_functions()->destroy(window_);
  window_ = nullptr;
}

void WaylandEglWindow::resize(
  std::uint32_t width, std::uint32_t height
) noexcept {
  if (window_ == nullptr) return;
  wayland_egl_functions()->resize(
    window_, static_cast<int>(width), static_cast<int>(height), 0, 0
  );
}

auto create_wayland_egl_window(
  wl_surface* surface, std::uint32_t width, std::uint32_t height,
  WaylandEglWindow& out_window
) -> mln_status {
  const auto* const functions = wayland_egl_functions();
  if (functions == nullptr) {
    set_thread_error("libwayland-egl.so.1 is missing");
    return MLN_STATUS_NATIVE_ERROR;
  }
  auto* const window = functions->create(
    surface, static_cast<int>(width), static_cast<int>(height)
  );
  if (window == nullptr) {
    set_thread_error("creating the wl_egl_window failed");
    return MLN_STATUS_NATIVE_ERROR;
  }
  out_window.reset();
  out_window.window_ = window;
  return MLN_STATUS_OK;
}

}  // namespace mln::core::opengl

#endif
