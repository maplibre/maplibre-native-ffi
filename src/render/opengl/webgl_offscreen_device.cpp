#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include <mln/util/logging.hpp>

#include <emscripten/em_js.h>
#include <emscripten/html5.h>

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/offscreen_device.hpp"
#include "render/opengl/opengl_texture_session.hpp"
#include "render/render_session_common.hpp"

// Creates a WebGL 2 context on a new OffscreenCanvas for the calling thread,
// and returns its handle, or 0 when the browser cannot. Emscripten's own
// context creation finds its canvas by a selector, which a canvas that no page
// or worker registered has none of, so this hands the canvas to its GL
// library directly.
// clang-format off
EM_JS_DEPS(mln_webgl_offscreen_device, "$GL");
EM_JS(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE, mln_create_offscreen_webgl_context, (), {
  if (typeof OffscreenCanvas === "undefined") return 0;
  // A session renders into textures of its own and never into the canvas, so
  // the canvas stays as small as it can be.
  return GL.createContext(new OffscreenCanvas(1, 1), {
    majorVersion: 2,
    minorVersion: 0,
    alpha: true,
    depth: false,
    stencil: false,
    antialias: false,
    premultipliedAlpha: true,
    preserveDrawingBuffer: false,
    enableExtensionsByDefault: true,
    explicitSwapControl: false,
  });
});

// Loses the context before Emscripten forgets it. The browser caps how many
// contexts a page keeps, and counts one that nothing reaches until garbage
// collection finds it, so losing it frees its place at once.
EM_JS(void, mln_lose_offscreen_webgl_context, (EMSCRIPTEN_WEBGL_CONTEXT_HANDLE handle), {
  GL.getContext(handle)?.GLctx.getExtension("WEBGL_lose_context")?.loseContext();
});
// clang-format on

namespace mln::core {
namespace {

// A WebGL context belongs to the thread that created it, which is the only
// one that may destroy it.
class OffscreenWebGLContext {
 public:
  explicit OffscreenWebGLContext(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE handle)
      : handle_(handle) {}
  OffscreenWebGLContext(const OffscreenWebGLContext&) = delete;
  auto operator=(const OffscreenWebGLContext&)
    -> OffscreenWebGLContext& = delete;
  OffscreenWebGLContext(OffscreenWebGLContext&&) = delete;
  auto operator=(OffscreenWebGLContext&&) -> OffscreenWebGLContext& = delete;
  ~OffscreenWebGLContext() {
    mln_lose_offscreen_webgl_context(handle_);
    emscripten_webgl_destroy_context(handle_);
  }

 private:
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE handle_;
};

// Holds nothing: a WebGL context belongs to one thread, so each session
// creates its own on its core worker.
class WebGLOffscreenDevice final : public OffscreenDevice {
 public:
  auto create_texture_backend(
    mln::Size size, std::unique_ptr<TextureSessionBackend>& out_backend
  ) -> mln_status override {
    const auto handle = mln_create_offscreen_webgl_context();
    if (handle == 0) {
      set_thread_error(
        "creating a WebGL 2 context on an OffscreenCanvas failed"
      );
      return MLN_STATUS_NATIVE_ERROR;
    }
    auto context = std::make_shared<OffscreenWebGLContext>(handle);
    auto descriptor = mln_open_gl_context_descriptor{};
    descriptor.platform = MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL;
    descriptor.ownership = MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED;
    descriptor.data.web_gl.kind = MLN_WEB_GL_CONTEXT_EXISTING;
    descriptor.data.web_gl.context = static_cast<int32_t>(handle);
    const auto status =
      create_opengl_offscreen_texture_backend(descriptor, size, out_backend);
    if (out_backend != nullptr) {
      out_backend->retain(std::move(context));
    }
    return status;
  }
};

}  // namespace

auto create_backend_offscreen_device(
  const mln_offscreen_device_options& options,
  std::shared_ptr<OffscreenDevice>& out_device
) -> mln_status {
  if (options.client_api == MLN_OPEN_GL_CLIENT_API_GL) {
    set_thread_error("WebGL 2 implements OpenGL ES 3, not desktop OpenGL");
    return MLN_STATUS_UNSUPPORTED;
  }
  if (
    (options.fields & MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX) != 0U &&
    options.adapter_index != 0
  ) {
    const auto message = "adapter_index " +
                         std::to_string(options.adapter_index) +
                         " is past the one WebGL adapter";
    set_thread_error(message.c_str());
    return MLN_STATUS_NATIVE_ERROR;
  }
  mln::Log::Info(mln::Event::Render, "offscreen device uses WebGL 2");
  out_device = std::make_shared<WebGLOffscreenDevice>();
  return MLN_STATUS_OK;
}

}  // namespace mln::core
