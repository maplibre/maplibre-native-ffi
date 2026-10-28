#pragma once

#if defined(MLN_FFI_OPENGL_PROVIDER_EGL)

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

#include <EGL/egl.h>

#include "maplibre_native_c/render_target.h"

namespace mln::core::opengl {

// The one context that the texture sessions of a single-context graphics
// device take turns in. Each session is a user with its own number, so the next
// one to take a turn knows whether the cached GL state it left behind survived.
class EglSingleContext final {
 public:
  EglSingleContext(EGLDisplay display, EGLContext context);
  EglSingleContext(const EglSingleContext&) = delete;
  auto operator=(const EglSingleContext&) -> EglSingleContext& = delete;
  EglSingleContext(EglSingleContext&&) = delete;
  auto operator=(EglSingleContext&&) -> EglSingleContext& = delete;
  ~EglSingleContext();

  [[nodiscard]] auto context() const -> EGLContext;
  [[nodiscard]] auto next_user() -> std::uint64_t;
  // Waits for the turn of `user`, and reports whether another user rendered
  // since its last turn.
  [[nodiscard]] auto lock(std::uint64_t user) -> bool;
  void unlock();

 private:
  EGLDisplay display_;
  EGLContext context_;
  std::mutex mutex_;
  std::uint64_t last_user_ = 0;
  std::atomic<std::uint64_t> users_ = 0;
};

class EglSharedContext final {
 public:
  // A session of a single-context device passes that context, and renders in
  // it instead of creating its own.
  EglSharedContext(
    mln_egl_context_descriptor descriptor,
    mln_open_gl_context_ownership ownership,
    std::shared_ptr<EglSingleContext> single = nullptr
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

  // Whether another session rendered in the single context since this one last
  // did. Reading it clears it.
  [[nodiscard]] auto take_state_lost() -> bool;

 private:
  [[nodiscard]] auto dedicated() const -> bool;
  [[nodiscard]] auto requested_api() const -> EGLenum;
  [[nodiscard]] auto display() const -> EGLDisplay;
  [[nodiscard]] auto config() const -> EGLConfig;
  [[nodiscard]] auto share_context() const -> EGLContext;
  [[nodiscard]] auto share_context_api() const -> EGLenum;
  [[nodiscard]] auto ensure_pbuffer_surface() -> EGLSurface;
  [[nodiscard]] auto current_on_thread() const -> bool;

  void activate(EGLSurface draw_surface, EGLSurface read_surface);
  void activate_single(EGLSurface draw_surface, EGLSurface read_surface);
  void release_turn();
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
  std::shared_ptr<EglSingleContext> single_;
  std::uint64_t single_user_ = 0;
  bool holds_turn_ = false;
  bool state_lost_ = false;
};

auto get_egl_proc_address(
  const mln_egl_context_descriptor& descriptor, const char* name,
  EGLenum active_api
) -> void*;

}  // namespace mln::core::opengl

#endif
