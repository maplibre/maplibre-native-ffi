#include "render/opengl/egl_context.hpp"

#if defined(MLN_FFI_OPENGL_PROVIDER_EGL)

#include <memory>
#include <stdexcept>
#include <utility>

#include <mln/platform/gl_functions.hpp>

#include "render/opengl/egl_common.hpp"

namespace mln::core::opengl {

EglSingleContext::EglSingleContext(EGLDisplay display, EGLContext context)
    : display_(display), context_(context) {}

// Nothing makes the context current but its sessions, and each holds this, so
// the last one has released it by now.
EglSingleContext::~EglSingleContext() { eglDestroyContext(display_, context_); }

auto EglSingleContext::context() const -> EGLContext { return context_; }

auto EglSingleContext::next_user() -> std::uint64_t {
  return users_.fetch_add(1, std::memory_order_relaxed) + 1;
}

auto EglSingleContext::lock(std::uint64_t user) -> bool {
  mutex_.lock();
  const auto lost = last_user_ != user;
  last_user_ = user;
  return lost;
}

void EglSingleContext::unlock() { mutex_.unlock(); }

EglSharedContext::EglSharedContext(
  mln_egl_context_descriptor descriptor,
  mln_open_gl_context_ownership ownership,
  std::shared_ptr<EglSingleContext> single
)
    : descriptor_(descriptor),
      ownership_(ownership),
      single_(std::move(single)),
      single_user_(single_ != nullptr ? single_->next_user() : 0) {}

EglSharedContext::~EglSharedContext() { destroy(); }

void EglSharedContext::activate_surface(EGLSurface surface) {
  activate(surface, surface);
}

void EglSharedContext::activate_pbuffer() {
  auto* const surface = ensure_pbuffer_surface();
  activate(surface, surface);
}

// A dedicated context stays current between renders, so the thread is left
// exactly as this session wants to find it next time.
void EglSharedContext::deactivate() {
  // Releasing the context flushes it, and hands the turn to the next session.
  if (single_ != nullptr) {
    release_current_context();
    release_turn();
    return;
  }
  if (dedicated()) {
    // Releasing a shared context flushes it, but a dedicated one stays
    // current, so its last commands would wait for its next render. ANGLE on
    // Metal runs every context of a display on one command queue in order, so
    // such unsubmitted work would also stall every other context's waits,
    // such as a readback on another session's thread.
    if (current_on_thread()) {
      mln::platform::glFlush();
    }
    return;
  }
  release_current_context();
  restore_previous_api();
  restore_previous_context();
}

auto EglSharedContext::dedicated() const -> bool {
  return ownership_ == MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED;
}

auto EglSharedContext::take_state_lost() -> bool {
  const auto lost = state_lost_;
  state_lost_ = false;
  return lost;
}

void EglSharedContext::release_turn() {
  if (holds_turn_) {
    holds_turn_ = false;
    single_->unlock();
  }
}

// Another session may hold the single context on its own core worker, so this
// waits for its turn, then makes the context current with this session's
// pbuffer until deactivate() hands it back.
void EglSharedContext::activate_single(
  EGLSurface draw_surface, EGLSurface read_surface
) {
  if (!holds_turn_) {
    state_lost_ = single_->lock(single_user_) || state_lost_;
    holds_turn_ = true;
  }
  try {
    const auto api = requested_api();
    if (eglBindAPI(api) == EGL_FALSE) {
      throw std::runtime_error("Binding EGL OpenGL API failed");
    }
    active_api_ = api;
    if (
      eglMakeCurrent(
        display(), draw_surface, read_surface, single_->context()
      ) == EGL_FALSE
    ) {
      throw std::runtime_error("Switching OpenGL EGL context failed");
    }
  } catch (...) {
    release_current_context();
    release_turn();
    throw;
  }
}

// The API to bind before creating a context and making it current. A dedicated
// context has no share context to ask, so the descriptor names it.
auto EglSharedContext::requested_api() const -> EGLenum {
  if (dedicated()) {
    return descriptor_.client_api == MLN_OPEN_GL_CLIENT_API_GL
             ? static_cast<EGLenum>(EGL_OPENGL_API)
             : static_cast<EGLenum>(EGL_OPENGL_ES_API);
  }
  return share_context_api();
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
  if (single_ != nullptr) {
    activate_single(draw_surface, read_surface);
    return;
  }
  if (dedicated()) {
    // Another session or the host may have made its own context current on
    // this thread since the last render, so ask EGL instead of remembering.
    if (
      current_on_thread() && eglGetCurrentSurface(EGL_DRAW) == draw_surface &&
      eglGetCurrentSurface(EGL_READ) == read_surface
    ) {
      return;
    }
    const auto api = requested_api();
    if (eglBindAPI(api) == EGL_FALSE) {
      throw std::runtime_error("Binding EGL OpenGL API failed");
    }
    active_api_ = api;
    if (context_ == EGL_NO_CONTEXT) {
      create_context();
    }
    if (
      eglMakeCurrent(display(), draw_surface, read_surface, context_) ==
      EGL_FALSE
    ) {
      throw std::runtime_error("Switching OpenGL EGL context failed");
    }
    return;
  }

  save_previous_context();
  try {
    const auto api = requested_api();
    if (eglBindAPI(api) == EGL_FALSE) {
      throw std::runtime_error("Binding EGL OpenGL API failed");
    }
    active_api_ = api;
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

// EGL keeps one current context per client API, so the API this context was
// created for has to be the bound one too.
auto EglSharedContext::current_on_thread() const -> bool {
  return context_ != EGL_NO_CONTEXT && eglQueryAPI() == requested_api() &&
         eglGetCurrentContext() == context_;
}

void EglSharedContext::create_context() {
  const EGLint es_context_attributes[] = {
    EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE
  };
  const EGLint opengl_context_attributes[] = {EGL_NONE};
  auto* const context_attributes = active_api_ == EGL_OPENGL_ES_API
                                     ? es_context_attributes
                                     : opengl_context_attributes;
  context_ = eglCreateContext(
    display(), config(), dedicated() ? EGL_NO_CONTEXT : share_context(),
    context_attributes
  );
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
  // A session of a single-context device creates no context, so teardown hands
  // back its turn and destroys only its pbuffer.
  if (holds_turn_) {
    release_current_context();
    release_turn();
  }
  // Teardown runs GL deletes with a dedicated context current, and EGL only
  // retires a context once nothing holds it. Release it, but leave alone a
  // context that another session or the host made current since.
  if (dedicated() && current_on_thread()) {
    release_current_context();
  }
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
