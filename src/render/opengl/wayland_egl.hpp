#pragma once

#if defined(MLN_FFI_WAYLAND) && defined(MLN_FFI_OPENGL_PROVIDER_EGL)

#include <cstdint>

#include "maplibre_native_c/base.h"
#include "render/opengl/egl_context.hpp"

struct wl_display;
struct wl_egl_window;
struct wl_surface;

namespace mln::core::opengl {

// The EGL display of a Wayland connection, with its config from
// choose_window_egl_config(). The library's sessions on one connection share
// it. When the library initialized it, the last one terminates it, since EGL
// would hand the stale display to a later connection at the same address; a
// display that the host initialized stays the host's.
class WaylandEglDisplay final {
 public:
  WaylandEglDisplay() = default;
  WaylandEglDisplay(const WaylandEglDisplay&) = delete;
  auto operator=(const WaylandEglDisplay&) -> WaylandEglDisplay& = delete;
  WaylandEglDisplay(WaylandEglDisplay&& other) noexcept;
  auto operator=(WaylandEglDisplay&& other) noexcept -> WaylandEglDisplay&;
  ~WaylandEglDisplay();

  [[nodiscard]] auto config() const noexcept -> const WindowEglConfig& {
    return config_;
  }

  friend auto acquire_wayland_egl_display(
    wl_display* connection, WaylandEglDisplay& out_display
  ) -> mln_status;

 private:
  void reset() noexcept;

  wl_display* connection_ = nullptr;
  WindowEglConfig config_{};
};

auto acquire_wayland_egl_display(
  wl_display* connection, WaylandEglDisplay& out_display
) -> mln_status;

// A wl_egl_window that the library made for a Wayland surface, from
// libwayland-egl, which it loads on first use.
class WaylandEglWindow final {
 public:
  WaylandEglWindow() = default;
  WaylandEglWindow(const WaylandEglWindow&) = delete;
  auto operator=(const WaylandEglWindow&) -> WaylandEglWindow& = delete;
  WaylandEglWindow(WaylandEglWindow&& other) noexcept;
  auto operator=(WaylandEglWindow&& other) noexcept -> WaylandEglWindow&;
  ~WaylandEglWindow();

  void reset() noexcept;
  void resize(std::uint32_t width, std::uint32_t height) noexcept;
  [[nodiscard]] auto get() const noexcept -> wl_egl_window* { return window_; }

  friend auto create_wayland_egl_window(
    wl_surface* surface, std::uint32_t width, std::uint32_t height,
    WaylandEglWindow& out_window
  ) -> mln_status;

 private:
  wl_egl_window* window_ = nullptr;
};

auto create_wayland_egl_window(
  wl_surface* surface, std::uint32_t width, std::uint32_t height,
  WaylandEglWindow& out_window
) -> mln_status;

}  // namespace mln::core::opengl

#endif
