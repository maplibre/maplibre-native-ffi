#pragma once

#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)

#include "maplibre_native_c/base.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <Windows.h>

namespace mln::core::opengl {

// Sets the thread's error to `what` and GetLastError().
void set_windows_error(const char* what);

// A hidden window whose device context carries the pixel format that a
// dedicated session's context needs. Only the thread that creates a window may
// destroy it, so each session makes its own on its core worker. Windows
// destroys a thread's windows when the thread exits, which reclaims one that
// a kept backend never destroys.
class HiddenWindow final {
 public:
  HiddenWindow() = default;
  HiddenWindow(const HiddenWindow&) = delete;
  auto operator=(const HiddenWindow&) -> HiddenWindow& = delete;
  HiddenWindow(HiddenWindow&&) = delete;
  auto operator=(HiddenWindow&&) -> HiddenWindow& = delete;
  ~HiddenWindow();

  // Creates the window with `pixel_format`, or with the format that the
  // library chooses when it is 0.
  auto create(int pixel_format = 0) -> mln_status;

  [[nodiscard]] auto device_context() const -> HDC { return device_context_; }

 private:
  HWND window_ = nullptr;
  HDC device_context_ = nullptr;
};

// A device context of a host's window, whose pixel format the library set.
// Only the thread that got it may release it.
class WglWindowDeviceContext final {
 public:
  WglWindowDeviceContext() = default;
  WglWindowDeviceContext(const WglWindowDeviceContext&) = delete;
  auto operator=(const WglWindowDeviceContext&)
    -> WglWindowDeviceContext& = delete;
  WglWindowDeviceContext(WglWindowDeviceContext&& other) noexcept;
  auto operator=(WglWindowDeviceContext&& other) noexcept
    -> WglWindowDeviceContext&;
  ~WglWindowDeviceContext() { reset(); }

  void reset() noexcept;

  [[nodiscard]] auto get() const noexcept -> HDC { return device_context_; }
  [[nodiscard]] auto pixel_format() const noexcept -> int {
    return pixel_format_;
  }

  friend auto acquire_wgl_window_device_context(
    HWND window, int pixel_format, WglWindowDeviceContext& out_context
  ) -> mln_status;

 private:
  HWND window_ = nullptr;
  HDC device_context_ = nullptr;
  int pixel_format_ = 0;
};

// Gets `window`'s device context with `pixel_format`, or with the format that
// the library chooses when it is 0, and sets that format unless the window
// already has it. Fails when the window has another one, which Windows doesn't
// let it change.
auto acquire_wgl_window_device_context(
  HWND window, int pixel_format, WglWindowDeviceContext& out_context
) -> mln_status;

// Checks that `window` has no pixel format yet, or the library's.
auto validate_wgl_window_pixel_format(HWND window) -> mln_status;

}  // namespace mln::core::opengl

#endif
