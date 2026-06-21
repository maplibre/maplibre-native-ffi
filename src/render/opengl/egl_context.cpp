#include "render/opengl/egl_context.hpp"

#if defined(MLN_FFI_OPENGL_PROVIDER_EGL)

#include <stdexcept>

#include "render/opengl/egl_common.hpp"

namespace mln::core::opengl {

EglSharedContext::EglSharedContext(mln_egl_context_descriptor descriptor)
    : descriptor_(descriptor) {}

EglSharedContext::~EglSharedContext() { destroy(); }

void EglSharedContext::activate_surface(EGLSurface surface) {
  activate(surface, surface);
}

void EglSharedContext::activate_pbuffer() {
  auto* const surface = ensure_pbuffer_surface();
  activate(surface, surface);
}

void EglSharedContext::deactivate() {
  release_current_context();
  restore_previous_api();
  restore_previous_context();
}

auto EglSharedContext::active_api() const -> EGLenum { return active_api_; }

auto EglSharedContext::display() const -> EGLDisplay {
  return static_cast<EGLDisplay>(descriptor_.display);
}

auto EglSharedContext::config() const -> EGLConfig {
  return static_cast<EGLConfig>(descriptor_.config);
}

auto EglSharedContext::share_context() const -> EGLContext {
  return static_cast<EGLContext>(descriptor_.share_context);
}

auto EglSharedContext::share_context_api() const -> EGLenum {
  auto client_type = EGLint{};
  if (
    eglQueryContext(
      display(), share_context(), EGL_CONTEXT_CLIENT_TYPE, &client_type
    ) == EGL_FALSE
  ) {
    throw std::runtime_error("Querying OpenGL EGL context API failed");
  }
  if (client_type == EGL_OPENGL_API || client_type == EGL_OPENGL_ES_API) {
    return static_cast<EGLenum>(client_type);
  }
  throw std::runtime_error("OpenGL EGL context API is unsupported");
}

auto EglSharedContext::ensure_pbuffer_surface() -> EGLSurface {
  if (pbuffer_surface_ != EGL_NO_SURFACE) {
    return pbuffer_surface_;
  }

  const EGLint surface_attributes[] = {
    EGL_WIDTH, 8, EGL_HEIGHT, 8, EGL_LARGEST_PBUFFER, EGL_TRUE, EGL_NONE
  };
  pbuffer_surface_ =
    eglCreatePbufferSurface(display(), config(), surface_attributes);
  if (pbuffer_surface_ == EGL_NO_SURFACE) {
    throw std::runtime_error("Creating OpenGL EGL pbuffer failed");
  }
  return pbuffer_surface_;
}

void EglSharedContext::activate(
  EGLSurface draw_surface, EGLSurface read_surface
) {
  save_previous_context();
  try {
    const auto requested_api = share_context_api();
    if (eglBindAPI(requested_api) == EGL_FALSE) {
      throw std::runtime_error("Binding EGL OpenGL API failed");
    }
    active_api_ = requested_api;
    if (context_ == EGL_NO_CONTEXT) {
      create_context();
    }
    if (
      eglMakeCurrent(display(), draw_surface, read_surface, context_) ==
      EGL_FALSE
    ) {
      throw std::runtime_error("Switching OpenGL EGL context failed");
    }
  } catch (...) {
    if (active_api_ != EGL_NONE) {
      release_current_context();
    }
    restore_previous_api();
    restore_previous_context();
    throw;
  }
}

void EglSharedContext::create_context() {
  const EGLint es_context_attributes[] = {
    EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE
  };
  const EGLint opengl_context_attributes[] = {EGL_NONE};
  auto* const context_attributes = active_api_ == EGL_OPENGL_ES_API
                                     ? es_context_attributes
                                     : opengl_context_attributes;
  context_ =
    eglCreateContext(display(), config(), share_context(), context_attributes);
  if (context_ == EGL_NO_CONTEXT) {
    throw std::runtime_error("Creating OpenGL EGL context failed");
  }
}

void EglSharedContext::save_previous_context() {
  previous_display_ = eglGetCurrentDisplay();
  previous_draw_surface_ = eglGetCurrentSurface(EGL_DRAW);
  previous_read_surface_ = eglGetCurrentSurface(EGL_READ);
  previous_context_ = eglGetCurrentContext();
  previous_api_ = eglQueryAPI();
}

void EglSharedContext::release_current_context() {
  (void)eglMakeCurrent(
    display(), EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT
  );
}

void EglSharedContext::restore_previous_api() {
  if (previous_api_ != EGL_NONE) {
    eglBindAPI(previous_api_);
    previous_api_ = EGL_NONE;
  }
  active_api_ = EGL_NONE;
}

void EglSharedContext::restore_previous_context() {
  const auto had_previous_display = previous_display_ != EGL_NO_DISPLAY;
  auto* const display =
    had_previous_display ? previous_display_ : EglSharedContext::display();
  auto* const draw_surface =
    had_previous_display ? previous_draw_surface_ : EGL_NO_SURFACE;
  auto* const read_surface =
    had_previous_display ? previous_read_surface_ : EGL_NO_SURFACE;
  auto* const context =
    had_previous_display ? previous_context_ : EGL_NO_CONTEXT;
  (void)eglMakeCurrent(display, draw_surface, read_surface, context);
  previous_display_ = EGL_NO_DISPLAY;
  previous_draw_surface_ = EGL_NO_SURFACE;
  previous_read_surface_ = EGL_NO_SURFACE;
  previous_context_ = EGL_NO_CONTEXT;
}

void EglSharedContext::destroy() {
  if (pbuffer_surface_ != EGL_NO_SURFACE) {
    eglDestroySurface(display(), pbuffer_surface_);
    pbuffer_surface_ = EGL_NO_SURFACE;
  }
  if (context_ != EGL_NO_CONTEXT) {
    eglDestroyContext(display(), context_);
    context_ = EGL_NO_CONTEXT;
  }
}

auto get_egl_proc_address(
  const mln_egl_context_descriptor& descriptor, const char* name,
  EGLenum active_api
) -> void* {
  using GetProcAddressFunction = void* (*)(const char*);
  auto* loader =
    reinterpret_cast<GetProcAddressFunction>(descriptor.get_proc_address);
  if (loader != nullptr) {
    auto* proc = loader(name);
    if (proc != nullptr) {
      return proc;
    }
  }
  if (auto* proc = eglGetProcAddress(name); proc != nullptr) {
    return reinterpret_cast<void*>(proc);
  }
  return get_egl_client_library_proc_address(name, active_api);
}

}  // namespace mln::core::opengl

#endif
