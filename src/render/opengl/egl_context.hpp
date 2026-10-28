#pragma once

#if defined(MLN_FFI_OPENGL_PROVIDER_EGL)

#include <cstdint>
#include <memory>

#include <EGL/egl.h>

#include "maplibre_native_c/render_target.h"

namespace mln::core::opengl {

struct PooledContext;

class EglSharedContext final {
 public:
  EglSharedContext(
    mln_egl_context_descriptor descriptor,
    mln_open_gl_context_ownership ownership
  );
  EglSharedContext(const EglSharedContext&) = delete;
  auto operator=(const EglSharedContext&) -> EglSharedContext& = delete;
  EglSharedContext(EglSharedContext&&) = delete;
  auto operator=(EglSharedContext&&) -> EglSharedContext& = delete;
  ~EglSharedContext();

  void activate_surface(EGLSurface surface);
  void activate_pbuffer();
  void deactivate();

  [[nodiscard]] auto active_api() const -> EGLenum;

  // Whether another session rendered in this pooled context since this one
  // last did. Reading it clears it.
  [[nodiscard]] auto take_state_lost() -> bool;

 private:
  [[nodiscard]] auto dedicated() const -> bool;
  [[nodiscard]] auto pooled() const -> bool;
  [[nodiscard]] auto requested_api() const -> EGLenum;
  [[nodiscard]] auto display() const -> EGLDisplay;
  [[nodiscard]] auto config() const -> EGLConfig;
  [[nodiscard]] auto share_context() const -> EGLContext;
  [[nodiscard]] auto share_context_api() const -> EGLenum;
  [[nodiscard]] auto ensure_pbuffer_surface() -> EGLSurface;
  [[nodiscard]] auto current_on_thread() const -> bool;

  void activate(EGLSurface draw_surface, EGLSurface read_surface);
  void activate_pooled(EGLSurface draw_surface, EGLSurface read_surface);
  void release_pool();
  void create_context();
  void save_previous_context();
  void release_current_context();
  void restore_previous_api();
  void restore_previous_context();
  void destroy();

  mln_egl_context_descriptor descriptor_{};
  mln_open_gl_context_ownership ownership_ =
    MLN_OPEN_GL_CONTEXT_OWNERSHIP_SHARED;
  EGLContext context_ = EGL_NO_CONTEXT;
  EGLSurface pbuffer_surface_ = EGL_NO_SURFACE;
  EGLDisplay previous_display_ = EGL_NO_DISPLAY;
  EGLSurface previous_draw_surface_ = EGL_NO_SURFACE;
  EGLSurface previous_read_surface_ = EGL_NO_SURFACE;
  EGLContext previous_context_ = EGL_NO_CONTEXT;
  EGLenum previous_api_ = EGL_NONE;
  EGLenum active_api_ = EGL_NONE;
  std::shared_ptr<PooledContext> pool_;
  std::uint64_t pool_user_ = 0;
  bool holds_pool_ = false;
  bool state_lost_ = false;
};

auto get_egl_proc_address(
  const mln_egl_context_descriptor& descriptor, const char* name,
  EGLenum active_api
) -> void*;

}  // namespace mln::core::opengl

#endif
