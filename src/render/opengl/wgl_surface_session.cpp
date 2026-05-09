/**
 * wgl_surface_session.cpp — WGL surface session for maplibre-native-ffi.
 *
 * Provides mln_wgl_surface_attach(), enabling OpenGL rendering on Windows
 * through a caller-owned WGL device context (HDC) and rendering context
 * (HGLRC).
 *
 * Design mirrors vulkan_surface_session.cpp:
 *   - HDC and HGLRC are borrowed from the host and must remain valid until
 *     the session is detached or destroyed.
 *   - WGLSurfaceSessionBackend implements SurfaceSessionBackend, giving the
 *     shared render-session machinery a renderer_backend() and resize().
 *   - WGLBackend extends mbgl::gl::RendererBackend and mbgl::gfx::Renderable,
 *     following the pattern used by the MapLibre Native Qt backend and by
 *     the WGL frontends in tdcosta100/MaplibreNative.NET (and the
 *     acalcutt/MaplibreNative.NET fork) and bjtrounson/maplibre-maui.
 *   - activate() / deactivate() call wglMakeCurrent.
 *   - updateAssumedState() re-syncs mbgl's cached GL state, preventing stale
 *     framebuffer/viewport assumptions after the host draws between frames.
 *   - After each render the session calls SwapBuffers(hdc) to present.
 *
 * Threading: all methods must be called on the session owner thread (the map
 * owner thread). The WGL context is not shared across threads.
 *
 * Windows header note: WIN32_LEAN_AND_MEAN is defined before <windows.h> to
 * avoid pulling in Winsock 1.x and other rarely-needed Win32 APIs.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cmath>
#include <memory>
#include <stdexcept>

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gfx/renderable.hpp>
#include <mbgl/gl/renderable_resource.hpp>
#include <mbgl/gl/renderer_backend.hpp>
#include <mbgl/renderer/renderer.hpp>
#include <mbgl/util/size.hpp>

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "render/render_session_common.hpp"
#include "render/surface_session.hpp"

namespace {

auto validate_descriptor(const mln_wgl_surface_descriptor* descriptor)
  -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("WGL surface descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_wgl_surface_descriptor)) {
    mln::core::set_thread_error("mln_wgl_surface_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    descriptor->width == 0 || descriptor->height == 0 ||
    !std::isfinite(descriptor->scale_factor) || descriptor->scale_factor <= 0.0
  ) {
    mln::core::set_thread_error(
      "WGL surface dimensions and scale_factor must be positive"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->hdc == nullptr || descriptor->hglrc == nullptr) {
    mln::core::set_thread_error("WGL hdc and hglrc must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

// ── WGL renderable resource ──────────────────────────────────────────────────

class WGLRenderableResource final : public mbgl::gl::RenderableResource {
 public:
  explicit WGLRenderableResource(class WGLSurfaceBackendImpl& backend_)
      : backend(backend_) {}
  void bind() override;

 private:
  class WGLSurfaceBackendImpl& backend;
};

// ── WGL renderer backend ─────────────────────────────────────────────────────

class WGLSurfaceBackendImpl final : public mbgl::gl::RendererBackend,
                                    public mbgl::gfx::Renderable {
 public:
  WGLSurfaceBackendImpl(HDC hdc_, HGLRC hglrc_, mbgl::Size size_)
      : mbgl::gl::RendererBackend(mbgl::gfx::ContextMode::Unique),
        mbgl::gfx::Renderable(
          size_, std::make_unique<WGLRenderableResource>(*this)
        ),
        hdc(hdc_),
        hglrc(hglrc_) {}

  WGLSurfaceBackendImpl(const WGLSurfaceBackendImpl&) = delete;
  auto operator=(const WGLSurfaceBackendImpl&) -> WGLSurfaceBackendImpl& =
    delete;
  WGLSurfaceBackendImpl(WGLSurfaceBackendImpl&&) = delete;
  auto operator=(WGLSurfaceBackendImpl&&) -> WGLSurfaceBackendImpl& = delete;

  ~WGLSurfaceBackendImpl() override {
    auto guard = mbgl::gfx::BackendScope{
      *this, mbgl::gfx::BackendScope::ScopeType::Implicit
    };
    (void)guard;
  }

  auto getDefaultRenderable() -> mbgl::gfx::Renderable& override {
    return *this;
  }

  void setSize(mbgl::Size size_) { size = size_; }

  void swapBuffers() { ::SwapBuffers(hdc); }

 protected:
  void activate() override { ::wglMakeCurrent(hdc, hglrc); }

  void deactivate() override { ::wglMakeCurrent(nullptr, nullptr); }

  auto getExtensionFunctionPointer(const char* name)
    -> mbgl::gl::ProcAddress override {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<mbgl::gl::ProcAddress>(::wglGetProcAddress(name));
  }

  // Re-sync mbgl's cached GL state to match what is actually current on the
  // WGL context. The host application may clear or bind framebuffers between
  // frames. Without re-syncing, mbgl's state cache skips re-binding and
  // produces missing fills, labels, or draw calls.
  // Mirrors the pattern used in tdcosta100/MaplibreNative.NET (and the
  // acalcutt/MaplibreNative.NET fork that this is most directly derived
  // from), bjtrounson/maplibre-maui's Windows frontend, and the MapLibre
  // Native Qt backend (updateAssumedState).
  void updateAssumedState() override {
    assumeFramebufferBinding(ImplicitFramebufferBinding);
    assumeViewport(0, 0, size);
  }

 private:
  HDC   hdc;
  HGLRC hglrc;
};

void WGLRenderableResource::bind() {
  backend.setFramebufferBinding(0);
  backend.setViewport(0, 0, backend.getSize());
}

// ── SurfaceSessionBackend adapter ────────────────────────────────────────────

class WGLSurfaceSessionBackend final
    : public mln::core::SurfaceSessionBackend {
 public:
  WGLSurfaceSessionBackend(HDC hdc, HGLRC hglrc, mbgl::Size size)
      : backend_(hdc, hglrc, size) {}

  auto renderer_backend() -> mbgl::gfx::RendererBackend& override {
    return backend_;
  }

  void resize(uint32_t physical_width, uint32_t physical_height) override {
    backend_.setSize(mbgl::Size{physical_width, physical_height});
  }

  void swap_buffers() { backend_.swapBuffers(); }

 private:
  WGLSurfaceBackendImpl backend_;
};

}  // namespace

namespace mln::core {

auto wgl_surface_attach(
  mln_map* map, const mln_wgl_surface_descriptor* descriptor,
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
    out_session,
    "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto physical_status = validate_physical_size(
    descriptor->width, descriptor->height, descriptor->scale_factor,
    "scaled WGL surface dimensions are too large"
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
  session->surface.backend = std::make_unique<WGLSurfaceSessionBackend>(
    static_cast<HDC>(descriptor->hdc),
    static_cast<HGLRC>(descriptor->hglrc),
    mbgl::Size{session->physical_width, session->physical_height}
  );

  return attach_render_session(
    std::move(session), out_session, RenderSessionKind::Surface,
    RenderSessionAttachMessages{
      .null_session = "WGL surface session must not be null",
      .null_output = "out_session must not be null",
      .non_null_output = "out_session must point to a null handle"
    }
  );
}

}  // namespace mln::core
