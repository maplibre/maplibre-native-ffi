#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#if defined(__ANDROID__) || defined(__OHOS__) || defined(MLN_FFI_WAYLAND)
#include <algorithm>
#include <array>
#include <cstddef>
#include <mutex>
#include <vector>
#endif

#include <mln/gfx/backend_scope.hpp>
#include <mln/gfx/renderable.hpp>
#include <mln/gl/context.hpp>
#include <mln/gl/renderable_resource.hpp>
#include <mln/gl/renderer_backend.hpp>
#include <mln/util/size.hpp>

#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <Windows.h>
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
#include <EGL/egl.h>
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
#include <emscripten/html5.h>
#include <emscripten/threading.h>
#include <pthread.h>
#endif

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#if defined(__ANDROID__)
#include "render/android_surface_session.hpp"
#elif defined(__OHOS__)
#include "render/ohos_surface_session.hpp"
#elif defined(MLN_FFI_WAYLAND)
#include "render/wayland_surface_session.hpp"
#elif defined(_WIN32)
#include "render/win32_window_session.hpp"
#endif
#include "render/opengl/context_mode.hpp"
#if defined(MLN_FFI_OPENGL_PROVIDER_EGL)
#include "render/opengl/egl_context.hpp"
#endif
#if defined(MLN_FFI_WAYLAND) && defined(MLN_FFI_OPENGL_PROVIDER_EGL)
#include "render/opengl/wayland_egl.hpp"
#endif
#include "render/opengl/webgl_worker.hpp"
#include "render/opengl/wgl_common.hpp"
#include "render/opengl/wgl_window.hpp"
#include "render/render_session_common.hpp"
#include "render/surface_session.hpp"

namespace {

class OpenGLSurfaceBackend final : public mln::gl::RendererBackend,
                                   public mln::gfx::Renderable {
 private:
  class OpenGLSurfaceRenderableResource final
      : public mln::gl::RenderableResource {
   public:
    explicit OpenGLSurfaceRenderableResource(OpenGLSurfaceBackend& backend_)
        : backend(backend_) {}

    void bind() override {
      backend.setFramebufferBinding(0);
      backend.setViewport(0, 0, backend.getSize());
      backend.setScissorTest(0, 0, 0, 0);
    }

    void swap() override {
      if (mln::core::discard_renderable_present) {
        return;
      }
      backend.swap_surface();
    }

   private:
    OpenGLSurfaceBackend& backend;
  };

 public:
  OpenGLSurfaceBackend(
    const mln_open_gl_surface_descriptor& descriptor, mln::Size size
  )
      : mln::gl::RendererBackend(mln::core::opengl::session_context_mode),
        mln::gfx::Renderable(
          size, std::make_unique<OpenGLSurfaceRenderableResource>(*this)
        ),
        descriptor_(descriptor) {}

  OpenGLSurfaceBackend(const OpenGLSurfaceBackend&) = delete;
  auto operator=(const OpenGLSurfaceBackend&) -> OpenGLSurfaceBackend& = delete;
  OpenGLSurfaceBackend(OpenGLSurfaceBackend&&) = delete;
  auto operator=(OpenGLSurfaceBackend&&) -> OpenGLSurfaceBackend& = delete;

  ~OpenGLSurfaceBackend() noexcept override {
    try {
      destroy_backend();
    } catch (const std::exception& exception) {
      mln::core::set_thread_error(exception);
    } catch (...) {
      mln::core::set_thread_error("destroying OpenGL surface backend failed");
    }
  }

  void destroy_backend() {
    if (has_native_context()) {
      // Making the surface current can fail, so run the GL teardown against
      // whatever can be made current but release the context either way, or the
      // HGLRC or EGLContext leaks.
      try {
        cleanup_while_current();
      } catch (...) {
        getThreadPool().runRenderJobs(true);
        destroy_native_context();
        throw;
      }
    } else {
      cleanup();
    }
    getThreadPool().runRenderJobs(true);
    destroy_native_context();
  }

  void cleanup() {
    resource.reset();
    context.reset();
  }

  // Falls back to the drawable the context was created from when the surface
  // can't be made current. That keeps the deletes reachable: this context lives
  // in the host's share group, so the objects the renderer built there stay
  // allocated until something in the group deletes them.
  void cleanup_while_current() {
    try {
      auto guard = mln::gfx::BackendScope{*this};
      cleanup();
      return;
    } catch (...) {
      // Retry teardown with the context's original drawable.
    }
    fallback_drawable_ = true;
    try {
      auto guard = mln::gfx::BackendScope{*this};
      cleanup();
    } catch (...) {
      // Nothing can be made current, so no GL call is safe. Leak the renderable
      // resource and the mbgl context rather than run their teardown: their
      // queued deletes would land on whatever context is current instead.
      static_cast<void>(resource.release());
      static_cast<void>(context.release());
      throw;
    }
  }

  auto getDefaultRenderable() -> mln::gfx::Renderable& override {
    return *this;
  }

  void resize(mln::Size size_) { setRenderableSize(size_); }

  // The outgoing surface is never touched, which a host may already have
  // destroyed; activate() makes the new one current on the next render.
  void set_surface(void* surface, mln::Size size) {
    // The surface alone. The context descriptor stays as it was at attach: this
    // session's GL context was created from it, and WGL context creation still
    // reads its device_context.
    descriptor_.surface = surface;
    fallback_drawable_ = false;
#if defined(MLN_FFI_OPENGL_PROVIDER_EGL) || defined(MLN_FFI_OPENGL_PROVIDER_WGL)
    swap_interval_pending_ = swap_interval_.has_value();
#endif
    setRenderableSize(size);
  }

#if defined(MLN_FFI_OPENGL_PROVIDER_EGL) || defined(MLN_FFI_OPENGL_PROVIDER_WGL)
  // Sets the swap interval of each surface that the context renders to from
  // here on. EGL and some WGL drivers keep the interval with the surface, so
  // each one gets it once the context is first current on it.
  void set_swap_interval(int interval) {
    swap_interval_ = interval;
    swap_interval_pending_ = true;
  }
#endif

  // Stops presenting through the host surface, so the host may destroy it.
  // The context moves to the drawable it was created from, which a dedicated
  // context stays current on between renders, and set_surface() moves it back.
  // Reports false for a WebGL context, which carries its canvas.
  [[nodiscard]] auto release_surface() -> bool {
#if defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
    return false;
#else
    descriptor_.surface = nullptr;
    fallback_drawable_ = true;
    if (has_native_context()) {
      // Activating makes the context current on that drawable instead.
      const auto guard = mln::gfx::BackendScope{*this};
    }
    return true;
#endif
  }

  void updateAssumedState() override {
    assumeFramebufferBinding(0);
    setViewport(0, 0, getSize());
    assumeScissorTest(0, 0, 0, 0);
  }

  void swap_surface() {
#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
    if (SwapBuffers(static_cast<HDC>(descriptor_.surface)) == 0) {
      throw std::runtime_error("Swapping OpenGL WGL surface buffers failed");
    }
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
    if (
      eglSwapBuffers(
        static_cast<EGLDisplay>(descriptor_.context.data.egl.display),
        static_cast<EGLSurface>(descriptor_.surface)
      ) == EGL_FALSE
    ) {
      throw std::runtime_error("Swapping OpenGL EGL surface buffers failed");
    }
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
    // The browser presents a frame drawn into the context's default
    // framebuffer when the event loop of the thread that owns the canvas
    // runs, so a transferred canvas's worker yields to its event loop after
    // each piece of work. emscripten_webgl_commit_frame() is no substitute: it
    // answers INVALID_TARGET under implicit swap control and does nothing
    // under explicit.
#else
    throw std::runtime_error("OpenGL context provider is unsupported");
#endif
  }

 private:
  [[nodiscard]] auto has_native_context() const -> bool {
#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
    return render_context_ != nullptr;
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
    return egl_context_.has_value();
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
    return descriptor_.context.data.web_gl.context > 0;
#else
    return false;
#endif
  }

  auto getExtensionFunctionPointer(const char* name)
    -> mln::gl::ProcAddress override {
#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
    using GetProcAddressFunction = PROC(WINAPI*)(LPCSTR);
    auto* loader = reinterpret_cast<GetProcAddressFunction>(
      descriptor_.context.data.wgl.get_proc_address
    );
    if (loader != nullptr) {
      auto* proc = loader(name);
      if (mln::core::opengl::is_valid_wgl_proc_address(proc)) {
        return reinterpret_cast<mln::gl::ProcAddress>(proc);
      }
    }
    auto* proc = wglGetProcAddress(name);
    if (mln::core::opengl::is_valid_wgl_proc_address(proc)) {
      return reinterpret_cast<mln::gl::ProcAddress>(proc);
    }
    return reinterpret_cast<mln::gl::ProcAddress>(
      mln::core::opengl::get_opengl32_proc_address(name)
    );
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
    return reinterpret_cast<mln::gl::ProcAddress>(
      mln::core::opengl::get_egl_proc_address(
        descriptor_.context.data.egl, name,
        egl_context_ ? egl_context_->active_api() : EGL_NONE
      )
    );
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
    // Emscripten resolves GLES entry points at link time.
    (void)name;
    return nullptr;
#else
    (void)name;
    return nullptr;
#endif
  }

  void activate() override {
#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
    if (dedicated()) {
      activate_dedicated_wgl();
      return;
    }
    previous_device_context_ = wglGetCurrentDC();
    previous_render_context_ = wglGetCurrentContext();
    try {
      if (render_context_ == nullptr) {
        create_wgl_context();
      }
      auto* const draw_surface =
        fallback_drawable_
          ? static_cast<HDC>(descriptor_.context.data.wgl.device_context)
          : static_cast<HDC>(descriptor_.surface);
      if (
        wglMakeCurrent(draw_surface, static_cast<HGLRC>(render_context_)) == 0
      ) {
        throw std::runtime_error("Switching OpenGL WGL context failed");
      }
      validate_wgl_context_support();
    } catch (...) {
      (void)wglMakeCurrent(
        static_cast<HDC>(previous_device_context_),
        static_cast<HGLRC>(previous_render_context_)
      );
      previous_device_context_ = nullptr;
      previous_render_context_ = nullptr;
      throw;
    }
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
    if (!egl_context_) {
      egl_context_.emplace(
        descriptor_.context.data.egl, descriptor_.context.ownership
      );
    }
    if (fallback_drawable_) {
      egl_context_->activate_pbuffer();
    } else {
      egl_context_->activate_surface(
        static_cast<EGLSurface>(descriptor_.surface)
      );
      if (swap_interval_pending_) {
        if (
          eglSwapInterval(
            static_cast<EGLDisplay>(descriptor_.context.data.egl.display),
            *swap_interval_
          ) == EGL_FALSE
        ) {
          throw std::runtime_error("Setting the EGL swap interval failed");
        }
        swap_interval_pending_ = false;
      }
    }
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
    // The context carries the canvas it draws to, so there is no second
    // drawable to fall back to.
    previous_webgl_context_ = emscripten_webgl_get_current_context();
    if (
      emscripten_webgl_make_context_current(
        descriptor_.context.data.web_gl.context
      ) != EMSCRIPTEN_RESULT_SUCCESS
    ) {
      throw std::runtime_error("Switching WebGL context failed");
    }
#else
    throw std::runtime_error("OpenGL context provider is unsupported");
#endif
  }

  void deactivate() override {
#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
    // A dedicated context stays current between renders.
    if (dedicated()) {
      return;
    }
    wglMakeCurrent(
      static_cast<HDC>(previous_device_context_),
      static_cast<HGLRC>(previous_render_context_)
    );
    previous_device_context_ = nullptr;
    previous_render_context_ = nullptr;
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
    egl_context_->deactivate();
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
    (void)emscripten_webgl_make_context_current(previous_webgl_context_);
    previous_webgl_context_ = 0;
#endif
  }

  [[nodiscard]] auto dedicated() const -> bool {
    return descriptor_.context.ownership ==
           MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED;
  }

#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
  // Makes this session's context current and leaves it current. A render that
  // finds it still current on the surface it presents through makes no WGL
  // call. Another session or the host may have made its own context current on
  // this thread since the last render, so this asks WGL instead of remembering.
  void activate_dedicated_wgl() {
    auto* const draw_surface =
      fallback_drawable_
        ? static_cast<HDC>(descriptor_.context.data.wgl.device_context)
        : static_cast<HDC>(descriptor_.surface);
    const auto needs_swap_interval =
      swap_interval_pending_ && !fallback_drawable_;
    if (
      render_context_ != nullptr && wglGetCurrentContext() == render_context_ &&
      wglGetCurrentDC() == draw_surface && !needs_swap_interval
    ) {
      return;
    }
    if (render_context_ == nullptr) {
      create_wgl_context();
    }
    if (
      wglMakeCurrent(draw_surface, static_cast<HGLRC>(render_context_)) == 0
    ) {
      throw std::runtime_error("Switching OpenGL WGL context failed");
    }
    // A context that fails validation must not stay current, or the next
    // activation would find it current and skip validating it again.
    try {
      validate_wgl_context_support();
    } catch (...) {
      (void)wglMakeCurrent(nullptr, nullptr);
      throw;
    }
    if (needs_swap_interval) {
      using SwapInterval = BOOL(WINAPI*)(int);
      auto* const swap_interval = reinterpret_cast<SwapInterval>(
        getExtensionFunctionPointer("wglSwapIntervalEXT")
      );
      if (swap_interval != nullptr && swap_interval(*swap_interval_) == FALSE) {
        throw std::runtime_error("Setting the WGL swap interval failed");
      }
      swap_interval_pending_ = false;
    }
  }

  void create_wgl_context() {
    auto* const device_context =
      static_cast<HDC>(descriptor_.context.data.wgl.device_context);
    auto* const share_context =
      static_cast<HGLRC>(descriptor_.context.data.wgl.share_context);
    auto* context_attribs =
      reinterpret_cast<mln::core::opengl::WglCreateContextAttribs>(
        getExtensionFunctionPointer("wglCreateContextAttribsARB")
      );
    render_context_ = mln::core::opengl::create_shared_wgl_context(
      device_context, share_context,
      static_cast<HGLRC>(previous_render_context_), context_attribs
    );
  }

  void validate_wgl_context_support() {
    mln::core::opengl::validate_required_wgl_proc_addresses(
      [this](const char* name) { return getExtensionFunctionPointer(name); }
    );
  }

  void destroy_native_context() {
    if (render_context_ != nullptr) {
      // Teardown leaves a dedicated context current on this thread, and WGL
      // only retires it once nothing holds it.
      if (dedicated() && wglGetCurrentContext() == render_context_) {
        (void)wglMakeCurrent(nullptr, nullptr);
      }
      wglDeleteContext(static_cast<HGLRC>(render_context_));
      render_context_ = nullptr;
    }
  }
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
  void destroy_native_context() { egl_context_.reset(); }
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
  void destroy_native_context() {
    if (
      descriptor_.context.data.web_gl.kind ==
        MLN_WEB_GL_CONTEXT_TRANSFERRED_CANVAS &&
      descriptor_.context.data.web_gl.context > 0
    ) {
      emscripten_webgl_destroy_context(descriptor_.context.data.web_gl.context);
      descriptor_.context.data.web_gl.context = 0;
    }
  }
#else
  void destroy_native_context() {}
#endif

  mln_open_gl_surface_descriptor descriptor_{};
  // Set while the session has no surface to use, because the host released it
  // or teardown found it unusable, so activate() reaches for the drawable the
  // context was created from instead.
  bool fallback_drawable_ = false;

#if defined(MLN_FFI_OPENGL_PROVIDER_EGL) || defined(MLN_FFI_OPENGL_PROVIDER_WGL)
  std::optional<int> swap_interval_;
  // Whether the surface still needs swap_interval_.
  bool swap_interval_pending_ = false;
#endif

#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
  void* render_context_ = nullptr;
  void* previous_device_context_ = nullptr;
  void* previous_render_context_ = nullptr;
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
  std::optional<mln::core::opengl::EglSharedContext> egl_context_;
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE previous_webgl_context_ = 0;
#endif
};

class OpenGLSurfaceSessionBackend final
    : public mln::core::SurfaceSessionBackend {
 public:
  OpenGLSurfaceSessionBackend(
    const mln_open_gl_surface_descriptor& descriptor, mln::Size size
  )
      : backend_(descriptor, size) {}

  auto renderer_backend() -> mln::gfx::RendererBackend& override {
    return backend_;
  }

  void resize(uint32_t physical_width, uint32_t physical_height) override {
    backend_.resize(mln::Size{physical_width, physical_height});
  }

  auto set_opengl_target(
    const mln_open_gl_surface_replacement& replacement, mln::Size physical_size
  ) -> mln_status override {
    backend_.set_surface(replacement.surface, physical_size);
    return MLN_STATUS_OK;
  }

#if defined(MLN_FFI_OPENGL_PROVIDER_EGL) || defined(MLN_FFI_OPENGL_PROVIDER_WGL)
  void set_swap_interval(int interval) { backend_.set_swap_interval(interval); }
#endif

  auto release_target() -> mln_status override {
    if (!backend_.release_surface()) {
      return mln::core::unsupported_retarget(
        "a WebGL context carries its canvas, so its session cannot release it"
      );
    }
    return MLN_STATUS_OK;
  }

 private:
  OpenGLSurfaceBackend backend_;
};

#if (defined(__ANDROID__) || defined(__OHOS__) || defined(MLN_FFI_WAYLAND)) && \
  defined(MLN_FFI_OPENGL_PROVIDER_EGL)
#if defined(__ANDROID__)
using PlatformWindow = mln::core::AndroidWindow;
#elif defined(__OHOS__)
using PlatformWindow = mln::core::OhosWindow;
#else
using PlatformWindow = mln::core::WaylandWindow;
#endif

auto egl_failure(const char* what) -> mln_status {
  const auto message =
    std::string{what} + " (EGL error " + std::to_string(eglGetError()) + ")";
  mln::core::set_thread_error(message.c_str());
  return MLN_STATUS_NATIVE_ERROR;
}

using mln::core::opengl::WindowEglConfig;

#if defined(MLN_FFI_WAYLAND)
// The connection that each of the session's Wayland surfaces belongs to.
using PlatformDisplay = wl_display*;
// The EGL display of the session's connection, which stays initialized while
// the session holds it.
using WindowEglDisplay = mln::core::opengl::WaylandEglDisplay;

auto open_window_egl_display(PlatformDisplay display, WindowEglDisplay& out)
  -> mln_status {
  return mln::core::opengl::acquire_wayland_egl_display(display, out);
}
#else
// An Android or OpenHarmony window stands on its own.
using PlatformDisplay = std::nullptr_t;

// The default EGL display, which stays initialized for the rest of the
// process.
class WindowEglDisplay final {
 public:
  [[nodiscard]] auto config() const noexcept -> const WindowEglConfig& {
    return config_;
  }

  friend auto open_window_egl_display(PlatformDisplay, WindowEglDisplay& out)
    -> mln_status {
    return mln::core::opengl::window_egl_config(out.config_);
  }

 private:
  WindowEglConfig config_;
};
#endif

// An EGL window surface that the library made for a platform window. On
// Wayland, it also owns the wl_egl_window that EGL presents through.
class EglWindowSurface final {
 public:
  EglWindowSurface() = default;
  EglWindowSurface(EGLDisplay display, EGLSurface surface)
      : display_(display), surface_(surface) {}
  EglWindowSurface(const EglWindowSurface&) = delete;
  auto operator=(const EglWindowSurface&) -> EglWindowSurface& = delete;
  EglWindowSurface(EglWindowSurface&& other) noexcept
      : display_(other.display_),
        surface_(std::exchange(other.surface_, EGL_NO_SURFACE))
#if defined(MLN_FFI_WAYLAND)
        ,
        native_(std::move(other.native_))
#endif
  {
  }
  auto operator=(EglWindowSurface&& other) noexcept -> EglWindowSurface& {
    if (this != &other) {
      reset();
      display_ = other.display_;
      surface_ = std::exchange(other.surface_, EGL_NO_SURFACE);
#if defined(MLN_FFI_WAYLAND)
      native_ = std::move(other.native_);
#endif
    }
    return *this;
  }
  ~EglWindowSurface() { reset(); }

  void reset() noexcept {
    if (surface_ != EGL_NO_SURFACE) eglDestroySurface(display_, surface_);
    surface_ = EGL_NO_SURFACE;
#if defined(MLN_FFI_WAYLAND)
    native_.reset();
#endif
  }

  // A Wayland client sizes its own buffers, which EGL reads from the
  // wl_egl_window.
  void resize(uint32_t physical_width, uint32_t physical_height) noexcept {
#if defined(MLN_FFI_WAYLAND)
    native_.resize(physical_width, physical_height);
#else
    static_cast<void>(physical_width);
    static_cast<void>(physical_height);
#endif
  }

  [[nodiscard]] auto get() const noexcept -> EGLSurface { return surface_; }

#if defined(MLN_FFI_WAYLAND)
  friend auto create_window_surface(
    const WindowEglConfig& egl, PlatformWindow::element_type* window,
    mln::Size physical_size, EglWindowSurface& out_surface
  ) -> mln_status;
#endif

 private:
  EGLDisplay display_ = EGL_NO_DISPLAY;
  EGLSurface surface_ = EGL_NO_SURFACE;
#if defined(MLN_FFI_WAYLAND)
  mln::core::opengl::WaylandEglWindow native_;
#endif
};

auto create_window_surface(
  const WindowEglConfig& egl, PlatformWindow::element_type* window,
  mln::Size physical_size, EglWindowSurface& out_surface
) -> mln_status {
#if defined(MLN_FFI_WAYLAND)
  auto native = mln::core::opengl::WaylandEglWindow{};
  const auto native_status = mln::core::opengl::create_wayland_egl_window(
    window, physical_size.width, physical_size.height, native
  );
  if (native_status != MLN_STATUS_OK) return native_status;
  // EGL takes a wl_egl_window as its Wayland platform's native window.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto native_window =
    reinterpret_cast<EGLNativeWindowType>(native.get());
#else
  static_cast<void>(physical_size);
  auto* const native_window = window;
#endif
  auto* const surface =
    eglCreateWindowSurface(egl.display, egl.config, native_window, nullptr);
  if (surface == EGL_NO_SURFACE) {
    return egl_failure("creating the EGL window surface failed");
  }
  out_surface = EglWindowSurface{egl.display, surface};
#if defined(MLN_FFI_WAYLAND)
  out_surface.native_ = std::move(native);
#endif
  return MLN_STATUS_OK;
}

// Presents to an Android, OpenHarmony, or Wayland window through an EGL window
// surface that it creates for each window, with a dedicated context that it
// creates.
class WindowEglSurfaceSessionBackend final
    : public mln::core::SurfaceSessionBackend {
 public:
  WindowEglSurfaceSessionBackend(
    WindowEglDisplay egl, PlatformWindow window, EglWindowSurface surface,
    const mln_open_gl_surface_descriptor& descriptor, mln::Size size
  )
      : egl_(std::move(egl)),
        window_(std::move(window)),
        surface_(std::move(surface)),
        session_(descriptor, size) {
#if defined(MLN_FFI_WAYLAND)
    // A Wayland compositor answers a hidden surface's frame callbacks only
    // once it shows again, and a swap with an interval waits for them, which
    // would hold a detach queued behind a render. The host paces frames
    // itself.
    session_.set_swap_interval(0);
#endif
  }

  auto renderer_backend() -> mln::gfx::RendererBackend& override {
    return session_.renderer_backend();
  }

  void resize(uint32_t physical_width, uint32_t physical_height) override {
    surface_.resize(physical_width, physical_height);
    session_.resize(physical_width, physical_height);
  }

  auto prepare_frame(bool& out_ready) -> mln_status override {
    return session_.prepare_frame(out_ready);
  }

#if defined(__ANDROID__)
  auto set_android_target(const PlatformWindow& window, mln::Size physical_size)
    -> mln_status override {
    return set_window(window, physical_size);
  }
#elif defined(__OHOS__)
  auto set_ohos_target(const PlatformWindow& window, mln::Size physical_size)
    -> mln_status override {
    return set_window(window, physical_size);
  }
#else
  auto set_wayland_target(const PlatformWindow& window, mln::Size physical_size)
    -> mln_status override {
    return set_window(window, physical_size);
  }
#endif

  auto release_target() -> mln_status override {
    const auto status = session_.release_target();
    if (status != MLN_STATUS_OK) return status;
    surface_.reset();
    window_.reset();
    return MLN_STATUS_OK;
  }

 private:
  auto set_window(const PlatformWindow& window, mln::Size physical_size)
    -> mln_status {
    if (window == window_) {
      resize(physical_size.width, physical_size.height);
      return MLN_STATUS_OK;
    }
    auto next = EglWindowSurface{};
    const auto create_status =
      create_window_surface(egl_.config(), window.get(), physical_size, next);
    if (create_status != MLN_STATUS_OK) return create_status;
    // The context stays current on its surface between renders, so it moves
    // off the old one before that goes.
    if (window_ != nullptr) {
      const auto release_status = release_target();
      if (release_status != MLN_STATUS_OK) return release_status;
    }
    const auto replacement = mln_open_gl_surface_replacement{
      .size = sizeof(mln_open_gl_surface_replacement),
      .logical_size = {},
      .surface = next.get(),
    };
    const auto status = session_.set_opengl_target(replacement, physical_size);
    if (status != MLN_STATUS_OK) return status;
    surface_ = std::move(next);
    window_ = window;
    return MLN_STATUS_OK;
  }

  // Destroyed last, so the display outlives the context and the surfaces.
  WindowEglDisplay egl_;
  PlatformWindow window_;
  EglWindowSurface surface_;
  // Destroyed first, so the context goes before the surface and the window.
  OpenGLSurfaceSessionBackend session_;
};
#endif

#if defined(_WIN32) && defined(MLN_FFI_OPENGL_PROVIDER_WGL)
using mln::core::opengl::acquire_wgl_window_device_context;
using mln::core::opengl::HiddenWindow;
using mln::core::opengl::WglWindowDeviceContext;

// Presents to a Win32 window through its device context, with a dedicated
// context that it creates on a hidden window of the same pixel format.
class WindowWglSurfaceSessionBackend final
    : public mln::core::SurfaceSessionBackend {
 public:
  WindowWglSurfaceSessionBackend(
    std::unique_ptr<HiddenWindow> fallback, mln::core::Win32Window window,
    WglWindowDeviceContext device_context,
    const mln_open_gl_surface_descriptor& descriptor, mln::Size size
  )
      : fallback_(std::move(fallback)),
        window_(std::move(window)),
        device_context_(std::move(device_context)),
        pixel_format_(device_context_.pixel_format()),
        session_(descriptor, size) {
    session_.set_swap_interval(0);
  }

  auto renderer_backend() -> mln::gfx::RendererBackend& override {
    return session_.renderer_backend();
  }

  void resize(uint32_t physical_width, uint32_t physical_height) override {
    session_.resize(physical_width, physical_height);
  }

  auto prepare_frame(bool& out_ready) -> mln_status override {
    out_ready = mln::core::win32_window_ready(window_.get());
    return MLN_STATUS_OK;
  }

  auto set_win32_target(
    const mln::core::Win32Window& window, mln::Size physical_size
  ) -> mln_status override {
    if (window == window_) {
      session_.resize(physical_size.width, physical_size.height);
      return MLN_STATUS_OK;
    }
    auto next = WglWindowDeviceContext{};
    const auto acquire_status =
      acquire_wgl_window_device_context(window.get(), pixel_format_, next);
    if (acquire_status != MLN_STATUS_OK) return acquire_status;
    // The context stays current on its window between renders, so it moves
    // off the old one before that goes.
    if (window_ != nullptr) {
      const auto release_status = release_target();
      if (release_status != MLN_STATUS_OK) return release_status;
    }
    const auto replacement = mln_open_gl_surface_replacement{
      .size = sizeof(mln_open_gl_surface_replacement),
      .logical_size = {},
      .surface = next.get(),
    };
    const auto status = session_.set_opengl_target(replacement, physical_size);
    if (status != MLN_STATUS_OK) return status;
    device_context_ = std::move(next);
    window_ = window;
    return MLN_STATUS_OK;
  }

  auto release_target() -> mln_status override {
    const auto status = session_.release_target();
    if (status != MLN_STATUS_OK) return status;
    device_context_.reset();
    window_.reset();
    return MLN_STATUS_OK;
  }

 private:
  // Destroyed last, so the context goes before the window it was created on.
  std::unique_ptr<HiddenWindow> fallback_;
  mln::core::Win32Window window_;
  WglWindowDeviceContext device_context_;
  int pixel_format_;
  // Destroyed first, so the context goes before the device contexts.
  OpenGLSurfaceSessionBackend session_;
};
#endif

}  // namespace

namespace mln::core {

#if (defined(__ANDROID__) || defined(__OHOS__) || defined(MLN_FFI_WAYLAND)) && \
  defined(MLN_FFI_OPENGL_PROVIDER_EGL)
namespace {

auto create_window_backend(
  PlatformDisplay display, PlatformWindow window,
  mln_render_session_object& target
) -> mln_status {
  auto egl_display = WindowEglDisplay{};
  const auto display_status = open_window_egl_display(display, egl_display);
  if (display_status != MLN_STATUS_OK) return display_status;
  const auto& egl = egl_display.config();
  const auto physical_size =
    mln::Size{target.physical_width, target.physical_height};
  auto surface = EglWindowSurface{};
  const auto surface_status =
    create_window_surface(egl, window.get(), physical_size, surface);
  if (surface_status != MLN_STATUS_OK) return surface_status;
  const auto descriptor = mln_open_gl_surface_descriptor{
    .size = sizeof(mln_open_gl_surface_descriptor),
    .logical_size = {.width = target.width, .height = target.height},
    .context =
      {
        .platform = MLN_OPEN_GL_CONTEXT_PLATFORM_EGL,
        .ownership = MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED,
        .data =
          {
            .egl =
              {
                .display = egl.display,
                .config = egl.config,
                .share_context = nullptr,
                .client_api = MLN_OPEN_GL_CLIENT_API_GLES,
                .get_proc_address = nullptr,
              },
          },
      },
    .surface = surface.get(),
  };
  target.surface.backend = std::make_unique<WindowEglSurfaceSessionBackend>(
    std::move(egl_display), std::move(window), std::move(surface), descriptor,
    physical_size
  );
  return MLN_STATUS_OK;
}

}  // namespace

#if defined(__ANDROID__)
auto create_android_surface_backend(
  AndroidSurfaceAttach attach, mln_render_session_object& target
) -> mln_status {
  return create_window_backend(nullptr, std::move(attach.window), target);
}
#elif defined(__OHOS__)
auto create_ohos_surface_backend(
  OhosSurfaceAttach attach, mln_render_session_object& target
) -> mln_status {
  return create_window_backend(nullptr, std::move(attach.window), target);
}
#else
auto create_wayland_surface_backend(
  WaylandSurfaceAttach attach, mln_render_session_object& target
) -> mln_status {
  return create_window_backend(
    attach.display, std::move(attach.window), target
  );
}
#endif
#endif

#if defined(_WIN32) && defined(MLN_FFI_OPENGL_PROVIDER_WGL)
auto create_win32_window_backend(
  Win32WindowAttach attach, mln_render_session_object& target
) -> mln_status {
  auto device_context = WglWindowDeviceContext{};
  const auto acquire_status =
    acquire_wgl_window_device_context(attach.window.get(), 0, device_context);
  if (acquire_status != MLN_STATUS_OK) return acquire_status;
  auto fallback = std::make_unique<HiddenWindow>();
  const auto fallback_status = fallback->create(device_context.pixel_format());
  if (fallback_status != MLN_STATUS_OK) return fallback_status;
  auto descriptor = mln_open_gl_surface_descriptor{
    .size = sizeof(mln_open_gl_surface_descriptor),
    .logical_size = {.width = target.width, .height = target.height},
    .context = {},
    .surface = device_context.get(),
  };
  descriptor.context.platform = MLN_OPEN_GL_CONTEXT_PLATFORM_WGL;
  descriptor.context.ownership = MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED;
  descriptor.context.data.wgl.device_context = fallback->device_context();
  target.surface.backend = std::make_unique<WindowWglSurfaceSessionBackend>(
    std::move(fallback), std::move(attach.window), std::move(device_context),
    descriptor, mln::Size{target.physical_width, target.physical_height}
  );
  return MLN_STATUS_OK;
}
#endif

auto opengl_surface_attach_start(
  mln_map map, const mln_open_gl_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_opengl_surface_descriptor(descriptor, true);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  const auto size_status = size_new_session(*session, descriptor->logical_size);
  if (size_status != MLN_STATUS_OK) {
    return size_status;
  }
  session->target_identity = {render_target_handle(descriptor->surface)};
  auto copied = *descriptor;
  const auto transferred = opengl::is_transferred_webgl_canvas(copied.context);
  auto selector =
    transferred ? opengl::webgl_canvas_selector(copied.context) : std::string{};
  if (transferred) {
    opengl::configure_transferred_webgl_worker(*session, selector);
  }
  const auto driver_status = require_render_driver(
    options,
    transferred ? MLN_RENDER_DRIVER_CORE_WORKER
                : MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD,
    "OpenGL driver does not match its context placement"
  );
  if (driver_status != MLN_STATUS_OK) {
    return driver_status;
  }
  session->initialize_backend =
    [copied, selector = std::move(selector),
     transferred](mln_render_session_object& target) mutable {
      if (transferred) {
        const auto context_status =
          opengl::create_transferred_webgl_context(copied.context, selector);
        if (context_status != MLN_STATUS_OK) {
          return context_status;
        }
      }
      target.surface.backend = std::make_unique<OpenGLSurfaceSessionBackend>(
        copied, mln::Size{target.physical_width, target.physical_height}
      );
      return MLN_STATUS_OK;
    };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = {},
    .texture_ring_depth = 0,
    .flags = MLN_RENDER_SESSION_CAPABILITY_PRESENTATION,
    .producer_sync_kind = {},
    .consumer_sync_kinds = 0,
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Surface, options, capabilities,
    out_session, completion,
    valueless_completion<&mln_map_attach_open_gl_surface>()
  );
}

auto opengl_surface_set_target_start(
  mln_render_session session,
  const mln_open_gl_surface_replacement* replacement,
  const mln_completion* completion
) -> mln_status {
  const auto replacement_status =
    validate_opengl_surface_replacement(replacement);
  if (replacement_status != MLN_STATUS_OK) {
    return replacement_status;
  }
  // The session's context comes from this build's only provider. WGL and EGL
  // make it current on the surface; a WebGL context names its canvas instead.
#if defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
  if (replacement->surface != nullptr) {
    set_thread_error(
      "WebGL surface must be null; the context names its canvas"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
#else
  if (replacement->surface == nullptr) {
    set_thread_error("OpenGL surface must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
#endif
  auto checked_completion = std::shared_ptr<Completion>{};
  const auto submission_status = validate_render_session_retarget_submission(
    session, RetargetTargetKind::Surface, completion, checked_completion
  );
  if (submission_status != MLN_STATUS_OK) {
    return submission_status;
  }
  const auto copied = *replacement;
  return enqueue_surface_retarget(
    session, copied.logical_size, {render_target_handle(copied.surface)},
    [copied](mln_render_session_object& live, mln::Size physical_size) {
      return live.surface.backend->set_opengl_target(copied, physical_size);
    },
    std::move(checked_completion),
    valueless_completion<&mln_render_session_set_open_gl_surface_target>()
  );
}

}  // namespace mln::core
