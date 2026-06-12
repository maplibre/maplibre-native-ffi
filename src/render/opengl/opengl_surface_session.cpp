#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gfx/renderable.hpp>
#include <mbgl/gl/context.hpp>
#include <mbgl/gl/renderable_resource.hpp>
#include <mbgl/gl/renderer_backend.hpp>
#include <mbgl/util/size.hpp>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <Windows.h>
#elif defined(__linux__)
#include <EGL/egl.h>
#endif

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#if defined(__linux__)
#include "render/opengl/egl_common.hpp"
#endif
#include "render/opengl/wgl_common.hpp"
#include "render/render_session_common.hpp"
#include "render/surface_session.hpp"

namespace {

auto validate_metal_surface_descriptor(
  const mln_metal_surface_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("surface descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_metal_surface_descriptor)) {
    mln::core::set_thread_error(
      "mln_metal_surface_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = mln::core::validate_render_target_extent(
    descriptor->extent, "surface dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  return mln::core::validate_metal_context(descriptor->context, false);
}

auto validate_vulkan_surface_descriptor(
  const mln_vulkan_surface_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("surface descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_vulkan_surface_descriptor)) {
    mln::core::set_thread_error(
      "mln_vulkan_surface_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = mln::core::validate_render_target_extent(
    descriptor->extent, "surface dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  return mln::core::validate_vulkan_context(
    descriptor->context, "Vulkan handles must not be null"
  );
}

auto validate_opengl_surface_descriptor(
  const mln_opengl_surface_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("surface descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_opengl_surface_descriptor)) {
    mln::core::set_thread_error(
      "mln_opengl_surface_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = mln::core::validate_render_target_extent(
    descriptor->extent, "surface dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto context_status =
    mln::core::validate_opengl_context(descriptor->context, true);
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->surface == nullptr) {
    mln::core::set_thread_error("OpenGL surface must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

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

  ~OpenGLSurfaceBackend() override {
    auto cleanup = [this] {
      resource.reset();
      context.reset();
    };
    if (render_context_ != nullptr) {
      auto guard = mbgl::gfx::BackendScope{
        *this, mbgl::gfx::BackendScope::ScopeType::Implicit
      };
      cleanup();
    } else {
      cleanup();
    }
    getThreadPool().runRenderJobs(true);
    destroy_native_context();
  }

  auto getDefaultRenderable() -> mbgl::gfx::Renderable& override {
    return *this;
  }

  void resize(mbgl::Size size_) { size = size_; }

  void updateAssumedState() override {
    assumeFramebufferBinding(0);
    setViewport(0, 0, size);
    assumeScissorTest(0, 0, 0, 0);
  }

  void swap_surface() {
#if defined(_WIN32)
    if (SwapBuffers(static_cast<HDC>(descriptor_.surface)) == 0) {
      throw std::runtime_error("Swapping OpenGL WGL surface buffers failed");
    }
#elif defined(__linux__)
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
  auto getExtensionFunctionPointer(const char* name)
    -> mbgl::gl::ProcAddress override {
#if defined(_WIN32)
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
#elif defined(__linux__)
    using GetProcAddressFunction = void* (*)(const char*);
    auto* loader = reinterpret_cast<GetProcAddressFunction>(
      descriptor_.context.data.egl.get_proc_address
    );
    if (loader != nullptr) {
      auto* proc = loader(name);
      if (proc != nullptr) {
        return reinterpret_cast<mbgl::gl::ProcAddress>(proc);
      }
    }
    if (auto* proc = eglGetProcAddress(name); proc != nullptr) {
      return reinterpret_cast<mbgl::gl::ProcAddress>(proc);
    }
    return reinterpret_cast<mbgl::gl::ProcAddress>(
      mln::core::opengl::get_egl_client_library_proc_address(name, active_api_)
    );
#else
    (void)name;
    return nullptr;
#endif
  }

  void activate() override {
#if defined(_WIN32)
    previous_device_context_ = wglGetCurrentDC();
    previous_render_context_ = wglGetCurrentContext();
    try {
      if (render_context_ == nullptr) {
        create_wgl_context();
      }
      if (
        wglMakeCurrent(
          static_cast<HDC>(descriptor_.surface),
          static_cast<HGLRC>(render_context_)
        ) == 0
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
#elif defined(__linux__)
    previous_display_ = eglGetCurrentDisplay();
    previous_draw_surface_ = eglGetCurrentSurface(EGL_DRAW);
    previous_read_surface_ = eglGetCurrentSurface(EGL_READ);
    previous_context_ = eglGetCurrentContext();
    previous_api_ = eglQueryAPI();
    try {
      const auto requested_api = share_context_api();
      if (eglBindAPI(requested_api) == EGL_FALSE) {
        throw std::runtime_error("Binding EGL OpenGL API failed");
      }
      active_api_ = requested_api;
      if (render_context_ == nullptr) {
        create_egl_context();
      }
      if (
        eglMakeCurrent(
          static_cast<EGLDisplay>(descriptor_.context.data.egl.display),
          static_cast<EGLSurface>(descriptor_.surface),
          static_cast<EGLSurface>(descriptor_.surface),
          static_cast<EGLContext>(render_context_)
        ) == EGL_FALSE
      ) {
        throw std::runtime_error("Switching OpenGL EGL context failed");
      }
    } catch (...) {
      if (active_api_ != EGL_NONE) {
        release_current_egl_context();
      }
      restore_previous_egl_api();
      restore_previous_egl_context();
      throw;
    }
#else
    throw std::runtime_error("OpenGL context provider is unsupported");
#endif
  }

  void deactivate() override {
#if defined(_WIN32)
    wglMakeCurrent(
      static_cast<HDC>(previous_device_context_),
      static_cast<HGLRC>(previous_render_context_)
    );
    previous_device_context_ = nullptr;
    previous_render_context_ = nullptr;
#elif defined(__linux__)
    release_current_egl_context();
    restore_previous_egl_api();
    restore_previous_egl_context();
#endif
  }

#if defined(_WIN32)
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
#elif defined(__linux__)
  void create_egl_context() {
    auto* const display =
      static_cast<EGLDisplay>(descriptor_.context.data.egl.display);
    auto* const config =
      static_cast<EGLConfig>(descriptor_.context.data.egl.config);
    auto* const share_context =
      static_cast<EGLContext>(descriptor_.context.data.egl.share_context);

    const EGLint es_context_attributes[] = {
      EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE
    };
    const EGLint opengl_context_attributes[] = {EGL_NONE};
    auto* const context_attributes = active_api_ == EGL_OPENGL_ES_API
                                       ? es_context_attributes
                                       : opengl_context_attributes;
    render_context_ =
      eglCreateContext(display, config, share_context, context_attributes);
    if (render_context_ == EGL_NO_CONTEXT) {
      render_context_ = nullptr;
      throw std::runtime_error("Creating OpenGL EGL context failed");
    }
  }

  auto share_context_api() -> EGLenum {
    auto* const display =
      static_cast<EGLDisplay>(descriptor_.context.data.egl.display);
    auto* const share_context =
      static_cast<EGLContext>(descriptor_.context.data.egl.share_context);
    auto client_type = EGLint{};
    if (
      eglQueryContext(
        display, share_context, EGL_CONTEXT_CLIENT_TYPE, &client_type
      ) == EGL_FALSE
    ) {
      throw std::runtime_error("Querying OpenGL EGL context API failed");
    }
    if (client_type == EGL_OPENGL_API || client_type == EGL_OPENGL_ES_API) {
      return static_cast<EGLenum>(client_type);
    }
    throw std::runtime_error("OpenGL EGL context API is unsupported");
  }

  void release_current_egl_context() {
    auto* const display =
      static_cast<EGLDisplay>(descriptor_.context.data.egl.display);
    (void)eglMakeCurrent(
      display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT
    );
  }

  void restore_previous_egl_api() {
    if (previous_api_ != EGL_NONE) {
      eglBindAPI(previous_api_);
      previous_api_ = EGL_NONE;
    }
    active_api_ = EGL_NONE;
  }

  void restore_previous_egl_context() {
    const auto had_previous_display = previous_display_ != nullptr;
    auto* const display =
      had_previous_display
        ? static_cast<EGLDisplay>(previous_display_)
        : static_cast<EGLDisplay>(descriptor_.context.data.egl.display);
    auto* const draw_surface =
      had_previous_display ? static_cast<EGLSurface>(previous_draw_surface_)
                           : EGL_NO_SURFACE;
    auto* const read_surface =
      had_previous_display ? static_cast<EGLSurface>(previous_read_surface_)
                           : EGL_NO_SURFACE;
    auto* const context = had_previous_display
                            ? static_cast<EGLContext>(previous_context_)
                            : EGL_NO_CONTEXT;
    (void)eglMakeCurrent(display, draw_surface, read_surface, context);
    previous_display_ = nullptr;
    previous_draw_surface_ = nullptr;
    previous_read_surface_ = nullptr;
    previous_context_ = nullptr;
  }

  void destroy_native_context() {
    auto* const display =
      static_cast<EGLDisplay>(descriptor_.context.data.egl.display);
    if (render_context_ != nullptr) {
      eglDestroyContext(display, static_cast<EGLContext>(render_context_));
      render_context_ = nullptr;
    }
  }
#else
  void destroy_native_context() {}
#endif

  mln_opengl_surface_descriptor descriptor_{};
  void* render_context_ = nullptr;

#if defined(_WIN32)
  void* previous_device_context_ = nullptr;
  void* previous_render_context_ = nullptr;
#elif defined(__linux__)
  void* previous_display_ = nullptr;
  void* previous_draw_surface_ = nullptr;
  void* previous_read_surface_ = nullptr;
  void* previous_context_ = nullptr;
  EGLenum previous_api_ = EGL_NONE;
  EGLenum active_api_ = EGL_NONE;
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

 private:
  OpenGLSurfaceBackend backend_;
};

}  // namespace

namespace mln::core {

auto metal_surface_attach(
  mln_map* map, const mln_metal_surface_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_metal_surface_descriptor(descriptor);
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
  set_thread_error("Metal surface sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto vulkan_surface_attach(
  mln_map* map, const mln_vulkan_surface_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_vulkan_surface_descriptor(descriptor);
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
  set_thread_error("Vulkan surface sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto opengl_surface_attach(
  mln_map* map, const mln_opengl_surface_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_opengl_surface_descriptor(descriptor);
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

  auto session = std::make_unique<mln_render_session>();
  session->map = map;
  session->owner_thread = map_owner_thread(map);
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

}  // namespace mln::core
