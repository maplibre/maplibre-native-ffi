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
      // Making the surface current can fail, and set_target lets a host install
      // a surface that only turns out to be unusable later, so this is
      // reachable. Run the GL teardown against whatever can be made current,
      // but release the context itself either way: skipping it would leak the
      // HGLRC or EGLContext for a session the host has already destroyed.
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
  // it presents to or, when that cannot be made current, through the drawable
  // the context itself was created from. The second try is what keeps the
  // deletes reachable: this context is created in the host's share group, so
  // the textures, buffers, and programs the renderer built there outlive it and
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
      // Nothing can be made current, so no GL call is safe from here. Give up
      // the renderable resource and the mbgl context without running their
      // teardown: destroying either issues the deletes it has queued, and after
      // destroy_native_context() those land on whatever context is current
      // instead. What they name is left to the host's share group.
      static_cast<void>(resource.release());
      static_cast<void>(context.release());
      throw;
    }
  }

  auto getDefaultRenderable() -> mbgl::gfx::Renderable& override {
    return *this;
  }

  void resize(mbgl::Size size_) { size = size_; }

  // Presents through a different host surface from here on. Nothing is touched
  // now: the session's own context stays as it is, holding every object the
  // renderer built in it, and activate() makes the new surface current on the
  // next render. That deliberately never reaches for the outgoing surface,
  // which a host may already have destroyed — the case this exists for.
  void set_surface(const mln_opengl_surface_descriptor& descriptor) {
    // The surface alone. The context descriptor stays as it was at attach: this
    // session's own GL context was created from it, WGL context creation still
    // reads its device_context, and context identity is what the caller was
    // told cannot change here.
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
