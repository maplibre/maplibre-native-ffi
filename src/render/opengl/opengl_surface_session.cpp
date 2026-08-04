#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gfx/renderable.hpp>
#include <mbgl/gl/context.hpp>
#include <mbgl/gl/renderable_resource.hpp>
#include <mbgl/gl/renderer_backend.hpp>
#include <mbgl/util/size.hpp>

#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <Windows.h>
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
#include <EGL/egl.h>
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
#include <emscripten/html5.h>
#endif

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#if defined(MLN_FFI_OPENGL_PROVIDER_EGL)
#include "render/opengl/egl_context.hpp"
#endif
#include "render/opengl/wgl_common.hpp"
#include "render/render_session_common.hpp"
#include "render/surface_session.hpp"

namespace {

class OpenGLSurfaceBackend final : public mbgl::gl::RendererBackend,
                                   public mbgl::gfx::Renderable {
 private:
  class OpenGLSurfaceRenderableResource final
      : public mbgl::gl::RenderableResource {
   public:
    explicit OpenGLSurfaceRenderableResource(OpenGLSurfaceBackend& backend_)
        : backend(backend_) {}

    void bind() override {
      backend.setFramebufferBinding(0);
      backend.setViewport(0, 0, backend.getSize());
      backend.setScissorTest(0, 0, 0, 0);
    }

    void swap() override { backend.swap_surface(); }

   private:
    OpenGLSurfaceBackend& backend;
  };

 public:
  OpenGLSurfaceBackend(
    const mln_opengl_surface_descriptor& descriptor, mbgl::Size size
  )
      : mbgl::gl::RendererBackend(mbgl::gfx::ContextMode::Shared),
        mbgl::gfx::Renderable(
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

  // Runs GL teardown with this session's context current, through the surface
  // it presents to or, failing that, through the drawable the context was
  // created from. The second try keeps the deletes reachable: this context
  // lives in the host's share group, so the objects the renderer built there
  // stay allocated until something in the group deletes them.
  void cleanup_while_current() {
    try {
      auto guard = mbgl::gfx::BackendScope{*this};
      cleanup();
      return;
    } catch (...) {  // NOLINT(bugprone-empty-catch)
      // Fall through to the drawable the context was created from.
    }
    fallback_drawable_ = true;
    try {
      auto guard = mbgl::gfx::BackendScope{*this};
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

  auto getDefaultRenderable() -> mbgl::gfx::Renderable& override {
    return *this;
  }

  void resize(mbgl::Size size_) { size = size_; }

  // Presents through a different host surface from here on. The outgoing
  // surface is never touched, which a host may already have destroyed;
  // activate() makes the new one current on the next render.
  void set_surface(const mln_opengl_surface_descriptor& descriptor) {
    // The surface alone. The context descriptor stays as it was at attach: this
    // session's GL context was created from it, and WGL context creation still
    // reads its device_context.
    descriptor_.surface = descriptor.surface;
    size = mbgl::Size{
      mln::core::physical_dimension(
        descriptor.extent.width, descriptor.extent.scale_factor
      ),
      mln::core::physical_dimension(
        descriptor.extent.height, descriptor.extent.scale_factor
      )
    };
  }

  [[nodiscard]] auto context_descriptor() const
    -> const mln_opengl_context_descriptor& {
    return descriptor_.context;
  }

  void updateAssumedState() override {
    assumeFramebufferBinding(0);
    setViewport(0, 0, size);
    assumeScissorTest(0, 0, 0, 0);
  }

  void swap_surface() {
#if defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
    // Nothing to swap. A WebGL drawing buffer is presented by the browser, not
    // by the program that drew into it: the canvas this context is bound to is
    // composited once the task that rendered returns to the event loop, which
    // is what makes the frame above visible. Emscripten's
    // emscripten_webgl_commit_frame() exists for the same moment, but it wants
    // a context created with explicitSwapControl and is a no-op even then,
    // because the .commit() it was written against was removed from browsers.
    //
    // The consequence belongs to whoever owns the render thread rather than to
    // this file: a thread that renders and then parks without ending its task
    // has drawn a frame the browser never composites. See src/browser/
    // dispatcher.c, which is why the module's owner thread runs one task per
    // call instead of looping inside a single one.
#elif defined(MLN_FFI_OPENGL_PROVIDER_WGL)
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
    // The browser composites the canvas itself, so a frame drawn into the
    // context's default framebuffer is presented with no call here.
    // emscripten_webgl_commit_frame() is not that call: it answers
    // INVALID_TARGET under implicit swap control and does nothing under
    // explicit.
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
    return descriptor_.context.data.webgl.context > 0;
#else
    return false;
#endif
  }

  auto getExtensionFunctionPointer(const char* name)
    -> mbgl::gl::ProcAddress override {
#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
    using GetProcAddressFunction = PROC(WINAPI*)(LPCSTR);
    auto* loader = reinterpret_cast<GetProcAddressFunction>(
      descriptor_.context.data.wgl.get_proc_address
    );
    if (loader != nullptr) {
      auto* proc = loader(name);
      if (mln::core::opengl::is_valid_wgl_proc_address(proc)) {
        return reinterpret_cast<mbgl::gl::ProcAddress>(proc);
      }
    }
    auto* proc = wglGetProcAddress(name);
    if (mln::core::opengl::is_valid_wgl_proc_address(proc)) {
      return reinterpret_cast<mbgl::gl::ProcAddress>(proc);
    }
    return reinterpret_cast<mbgl::gl::ProcAddress>(
      mln::core::opengl::get_opengl32_proc_address(name)
    );
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
    return reinterpret_cast<mbgl::gl::ProcAddress>(
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
      egl_context_.emplace(descriptor_.context.data.egl);
    }
    if (fallback_drawable_) {
      egl_context_->activate_pbuffer();
    } else {
      egl_context_->activate_surface(
        static_cast<EGLSurface>(descriptor_.surface)
      );
    }
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
    // The context carries the canvas it draws to, so there is no second
    // drawable to fall back to.
    previous_webgl_context_ = emscripten_webgl_get_current_context();
    if (
      emscripten_webgl_make_context_current(
        descriptor_.context.data.webgl.context
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

#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
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
      wglDeleteContext(static_cast<HGLRC>(render_context_));
      render_context_ = nullptr;
    }
  }
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
  void destroy_native_context() { egl_context_.reset(); }
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
  // The host owns the context this session borrowed.
  void destroy_native_context() {}
#else
  void destroy_native_context() {}
#endif

  mln_opengl_surface_descriptor descriptor_{};
  // Set once teardown has found the session's surface unusable, so activate()
  // reaches for the drawable the context was created from instead.
  bool fallback_drawable_ = false;

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
    const mln_opengl_surface_descriptor& descriptor, mbgl::Size size
  )
      : backend_(descriptor, size) {}

  auto renderer_backend() -> mbgl::gfx::RendererBackend& override {
    return backend_;
  }

  void resize(uint32_t physical_width, uint32_t physical_height) override {
    backend_.resize(mbgl::Size{physical_width, physical_height});
  }

  auto set_opengl_target(const mln_opengl_surface_descriptor& descriptor)
    -> mln_status override {
    if (!mln::core::opengl_context_matches(
          backend_.context_descriptor(), descriptor.context,
          mln::core::OpenGLContextMatch::ShareGroup
        )) {
      mln::core::set_thread_error(
        "OpenGL surface target must name the context this session attached with"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    backend_.set_surface(descriptor);
    return MLN_STATUS_OK;
  }

 private:
  OpenGLSurfaceBackend backend_;
};

}  // namespace

namespace mln::core {

auto opengl_surface_attach(
  mln_map map, const mln_opengl_surface_descriptor* descriptor,
  mln_render_session* out_session
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
  const auto output_status = validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto physical_status = validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled surface dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_session_extent(*session, descriptor->extent);
  session->surface.backend = std::make_unique<OpenGLSurfaceSessionBackend>(
    *descriptor, mbgl::Size{session->physical_width, session->physical_height}
  );
  return attach_render_session(
    std::move(session), out_session, RenderSessionKind::Surface,
    RenderSessionAttachMessages{
      .null_session = "surface session must not be null",
      .null_output = "out_session must not be null",
      .non_null_output = "out_session must point to a null handle"
    }
  );
}

auto opengl_surface_set_target(
  mln_render_session session, const mln_opengl_surface_descriptor* descriptor
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto session_status = validate_render_session_retarget(
    session, RetargetTargetKind::Surface, live
  );
  if (session_status != MLN_STATUS_OK) {
    return session_status;
  }
  const auto descriptor_status =
    validate_opengl_surface_descriptor(descriptor, true);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  return surface_session_set_target(
    session, descriptor->extent,
    [descriptor](mln_render_session_object& target_session) -> mln_status {
      return target_session.surface.backend->set_opengl_target(*descriptor);
    }
  );
}

}  // namespace mln::core
