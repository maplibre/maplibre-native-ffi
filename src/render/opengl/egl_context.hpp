#pragma once

#if defined(MLN_FFI_OPENGL_PROVIDER_EGL)

#include <EGL/egl.h>

#include "maplibre_native_c/render_target.h"

namespace mln::core::opengl {

class EglSharedContext final {
 public:
  explicit EglSharedContext(mln_egl_context_descriptor descriptor);
  EglSharedContext(const EglSharedContext&) = delete;
  auto operator=(const EglSharedContext&) -> EglSharedContext& = delete;
  EglSharedContext(EglSharedContext&&) = delete;
  auto operator=(EglSharedContext&&) -> EglSharedContext& = delete;
  ~EglSharedContext();

  void activate_surface(EGLSurface surface);
  void activate_pbuffer();
  void deactivate();

  [[nodiscard]] auto active_api() const -> EGLenum;

 private:
  [[nodiscard]] auto display() const -> EGLDisplay;
  [[nodiscard]] auto config() const -> EGLConfig;
  [[nodiscard]] auto share_context() const -> EGLContext;
  [[nodiscard]] auto share_context_api() const -> EGLenum;
  [[nodiscard]] auto ensure_pbuffer_surface() -> EGLSurface;

  void activate(EGLSurface draw_surface, EGLSurface read_surface);
  void create_context();
  void save_previous_context();
  void release_current_context();
  void restore_previous_api();
  void restore_previous_context();
  void destroy();

  mln_egl_context_descriptor descriptor_{};
  EGLContext context_ = EGL_NO_CONTEXT;
  EGLSurface pbuffer_surface_ = EGL_NO_SURFACE;
  EGLDisplay previous_display_ = EGL_NO_DISPLAY;
  EGLSurface previous_draw_surface_ = EGL_NO_SURFACE;
  EGLSurface previous_read_surface_ = EGL_NO_SURFACE;
  EGLContext previous_context_ = EGL_NO_CONTEXT;
  EGLenum previous_api_ = EGL_NONE;
  EGLenum active_api_ = EGL_NONE;
};

auto get_egl_proc_address(
  const mln_egl_context_descriptor& descriptor, const char* name,
  EGLenum active_api
) -> void*;

}  // namespace mln::core::opengl

#endif
