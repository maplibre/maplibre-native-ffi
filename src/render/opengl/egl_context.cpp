#include "render/opengl/egl_context.hpp"

#if defined(MLN_FFI_OPENGL_PROVIDER_EGL)

#include <atomic>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

#include <mln/platform/gl_functions.hpp>

#include "render/opengl/egl_common.hpp"

namespace mln::core::opengl {

// The turns of the pooled sessions that render in one host context. Each
// session has its own user number, so the next one to take a turn knows
// whether the cached GL state it left behind survived.
struct PooledContext {
  std::mutex mutex;
  std::uint64_t last_user = 0;
};

namespace {

// An entry lives while a session holds it, so a host context destroyed after
// its last session leaves nothing behind.
auto pooled_context(EGLContext context) -> std::shared_ptr<PooledContext> {
  static auto registry_mutex = std::mutex{};
  static auto registry =
    std::unordered_map<EGLContext, std::weak_ptr<PooledContext>>{};
  const auto lock = std::scoped_lock{registry_mutex};
  std::erase_if(registry, [](const auto& entry) {
    return entry.second.expired();
  });
  auto& entry = registry[context];
  auto pool = entry.lock();
  if (pool == nullptr) {
    pool = std::make_shared<PooledContext>();
    entry = pool;
  }
  return pool;
}

auto next_pool_user() -> std::uint64_t {
  static auto users = std::atomic<std::uint64_t>{0};
  return users.fetch_add(1, std::memory_order_relaxed) + 1;
}

}  // namespace

EglSharedContext::EglSharedContext(
  mln_egl_context_descriptor descriptor, mln_open_gl_context_ownership ownership
)
    : descriptor_(descriptor), ownership_(ownership) {
  if (pooled()) {
    pool_ = pooled_context(share_context());
    pool_user_ = next_pool_user();
  }
}

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
  // Releasing the context flushes it, and hands the turn to the next pooled
  // session.
  if (pooled()) {
    release_current_context();
    release_pool();
    return;
  }
  release_current_context();
  restore_previous_api();
  restore_previous_context();
}

auto EglSharedContext::dedicated() const -> bool {
  return ownership_ == MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED;
}

auto EglSharedContext::pooled() const -> bool {
  return ownership_ == MLN_OPEN_GL_CONTEXT_OWNERSHIP_POOLED;
}

auto EglSharedContext::take_state_lost() -> bool {
  const auto lost = state_lost_;
  state_lost_ = false;
  return lost;
}

void EglSharedContext::release_pool() {
  if (holds_pool_) {
    holds_pool_ = false;
    pool_->mutex.unlock();
  }
}

// Another pooled session may hold the host context on its own core worker, so
// this waits for its turn, then makes the context current with this session's
// pbuffer until deactivate() hands it back.
void EglSharedContext::activate_pooled(
  EGLSurface draw_surface, EGLSurface read_surface
) {
  if (!holds_pool_) {
    pool_->mutex.lock();
    holds_pool_ = true;
  }
  if (pool_->last_user != pool_user_) {
    pool_->last_user = pool_user_;
    state_lost_ = true;
  }
  try {
    const auto api = requested_api();
    if (eglBindAPI(api) == EGL_FALSE) {
      throw std::runtime_error("Binding EGL OpenGL API failed");
    }
    active_api_ = api;
    if (
      eglMakeCurrent(display(), draw_surface, read_surface, share_context()) ==
      EGL_FALSE
    ) {
      throw std::runtime_error("Switching OpenGL EGL context failed");
    }
  } catch (...) {
    release_current_context();
    release_pool();
    throw;
  }
}

// The API to bind before creating a context and making it current. A dedicated
// context has no share context to ask, and a pooled one renders in the host
// context, so the descriptor names it for both.
auto EglSharedContext::requested_api() const -> EGLenum {
  if (dedicated() || pooled()) {
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
  if (pooled()) {
    activate_pooled(draw_surface, read_surface);
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
  // The host owns a pooled context, so teardown destroys only the pbuffer.
  if (pooled() && holds_pool_) {
    release_current_context();
    release_pool();
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
