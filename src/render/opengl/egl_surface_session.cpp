#include <cmath>
#include <cstdint>
#include <memory>
#include <utility>

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gfx/renderable.hpp>
#include <mbgl/gfx/renderer_backend.hpp>
#include <mbgl/gl/renderable_resource.hpp>
#include <mbgl/gl/renderer_backend.hpp>
#include <mbgl/renderer/renderer.hpp>
#include <mbgl/util/event.hpp>
#include <mbgl/util/logging.hpp>
#include <mbgl/util/size.hpp>

#include <EGL/egl.h>
#include <maplibre_native_c/base.h>
#include <maplibre_native_c/surface.h>

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "render/render_session_common.hpp"
#include "render/surface_session.hpp"

namespace {

auto validate_descriptor(const mln_egl_surface_descriptor* descriptor)
  -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("EGL surface descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_egl_surface_descriptor)) {
    mln::core::set_thread_error("mln_egl_surface_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    descriptor->width == 0 || descriptor->height == 0 ||
    !std::isfinite(descriptor->scale_factor) || descriptor->scale_factor <= 0.0
  ) {
    mln::core::set_thread_error(
      "EGL surface dimensions and scale_factor must be positive"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    descriptor->display == nullptr || descriptor->context == nullptr ||
    descriptor->surface == nullptr
  ) {
    mln::core::set_thread_error(
      "EGL display, context, and surface must not be null"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

class EGLRenderableResource final : public mbgl::gl::RenderableResource {
 public:
  explicit EGLRenderableResource(class EGLSurfaceBackendImpl& backend_)
      : backend(backend_) {}
  void bind() override;

 private:
  class EGLSurfaceBackendImpl& backend;
};

class EGLSurfaceBackendImpl final : public mbgl::gl::RendererBackend,
                                    public mbgl::gfx::Renderable {
 public:
  EGLSurfaceBackendImpl(
    EGLDisplay display_, EGLContext context_, EGLSurface surface_,
    mbgl::Size size_
  )
      : mbgl::gl::RendererBackend(mbgl::gfx::ContextMode::Unique),
        mbgl::gfx::Renderable(
          size_, std::make_unique<EGLRenderableResource>(*this)
        ),
        display(display_),
        context(context_),
        surface(surface_) {}

  EGLSurfaceBackendImpl(const EGLSurfaceBackendImpl&) = delete;
  auto operator=(const EGLSurfaceBackendImpl&)
    -> EGLSurfaceBackendImpl& = delete;
  EGLSurfaceBackendImpl(EGLSurfaceBackendImpl&&) = delete;
  auto operator=(EGLSurfaceBackendImpl&&) -> EGLSurfaceBackendImpl& = delete;

  ~EGLSurfaceBackendImpl() override {
    auto guard = mbgl::gfx::BackendScope{
      *this, mbgl::gfx::BackendScope::ScopeType::Implicit
    };
    (void)guard;
  }

  auto getDefaultRenderable() -> mbgl::gfx::Renderable& override {
    return *this;
  }

  void setSize(mbgl::Size size_) { size = size_; }

  auto swapBuffers() -> bool {
    if (eglSwapBuffers(display, surface) == EGL_FALSE) {
      mbgl::Log::Error(mbgl::Event::Render, "eglSwapBuffers failed");
      return false;
    }
    return true;
  }

 protected:
  void activate() override {
    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
      mbgl::Log::Error(mbgl::Event::Render, "eglMakeCurrent failed");
    }
  }

  void deactivate() override {
    if (
      eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) ==
      EGL_FALSE
    ) {
      mbgl::Log::Error(
        mbgl::Event::Render, "eglMakeCurrent (deactivate) failed"
      );
    }
  }

  auto getExtensionFunctionPointer(const char* name)
    -> mbgl::gl::ProcAddress override {
    return eglGetProcAddress(name);
  }

  // Re-sync mbgl's assumed GL state after each context switch; the host may
  // have mutated framebuffer binding or viewport between frames.
  void updateAssumedState() override {
    assumeFramebufferBinding(ImplicitFramebufferBinding);
    assumeViewport(0, 0, size);
  }

 private:
  EGLDisplay display;
  EGLContext context;
  EGLSurface surface;
};

void EGLRenderableResource::bind() {
  backend.setFramebufferBinding(0);
  backend.setViewport(0, 0, backend.getSize());
}

class EGLSurfaceSessionBackend final : public mln::core::SurfaceSessionBackend {
 public:
  EGLSurfaceSessionBackend(
    EGLDisplay display, EGLContext context, EGLSurface surface, mbgl::Size size
  )
      : backend_(display, context, surface, size) {}

  auto renderer_backend() -> mbgl::gfx::RendererBackend& override {
    return backend_;
  }

  void resize(uint32_t physical_width, uint32_t physical_height) override {
    backend_.setSize(mbgl::Size{physical_width, physical_height});
  }

  auto swap_buffers() -> mln_status override {
    if (!backend_.swapBuffers()) {
      mln::core::set_thread_error(
        "eglSwapBuffers failed: surface may have been lost"
      );
      return MLN_STATUS_INVALID_STATE;
    }
    return MLN_STATUS_OK;
  }
  [[nodiscard]] auto scope_type() const
    -> mbgl::gfx::BackendScope::ScopeType override {
    return mbgl::gfx::BackendScope::ScopeType::Explicit;
  }

 private:
  EGLSurfaceBackendImpl backend_;
};

}  // namespace

namespace mln::core {

auto egl_surface_attach(
  mln_map* map, const mln_egl_surface_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_descriptor(descriptor);
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
    descriptor->width, descriptor->height, descriptor->scale_factor,
    "scaled EGL surface dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  auto session = std::make_unique<mln_render_session>();
  session->map = map;
  session->owner_thread = map_owner_thread(map);
  session->width = descriptor->width;
  session->height = descriptor->height;
  session->scale_factor = descriptor->scale_factor;
  session->physical_width =
    physical_dimension(descriptor->width, descriptor->scale_factor);
  session->physical_height =
    physical_dimension(descriptor->height, descriptor->scale_factor);
  session->surface.backend = std::make_unique<EGLSurfaceSessionBackend>(
    static_cast<EGLDisplay>(descriptor->display),
    static_cast<EGLContext>(descriptor->context),
    static_cast<EGLSurface>(descriptor->surface),
    mbgl::Size{session->physical_width, session->physical_height}
  );

  return attach_render_session(
    std::move(session), out_session, RenderSessionKind::Surface,
    RenderSessionAttachMessages{
      .null_session = "EGL surface session must not be null",
      .null_output = "out_session must not be null",
      .non_null_output = "out_session must point to a null handle"
    }
  );
}

}  // namespace mln::core
