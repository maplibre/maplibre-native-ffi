#pragma once

// The transferred-canvas WebGL path shared by the OpenGL surface and texture
// sessions. A transferred canvas belongs to the worker its pthread claimed, so
// the session both creates that worker and creates the WebGL context on it.
// The non-browser build compiles the same call sites with a canvas that is
// never transferred.

#include <string>

#if defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
#include <functional>
#include <memory>

#include <emscripten/html5.h>
#include <emscripten/threading.h>
#include <pthread.h>
#endif

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"

namespace mln::core::opengl {

// Whether this descriptor names a canvas the session transfers to its worker.
inline auto is_transferred_webgl_canvas(
  const mln_opengl_context_descriptor& context
) -> bool {
#if defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
  return context.platform == MLN_OPENGL_CONTEXT_PLATFORM_WEBGL &&
         context.data.webgl.kind == MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS;
#else
  static_cast<void>(context);
  return false;
#endif
}

inline auto webgl_canvas_selector(const mln_opengl_context_descriptor& context)
  -> std::string {
  const auto view = context.data.webgl.canvas_selector;
  if (view.size == 0) return {};
  return std::string{static_cast<const char*>(view.data), view.size};
}

#if defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)

struct WebGLWorkerCall {
  std::function<void()> function;
};

inline auto run_webgl_worker(void* opaque) -> void* {
  auto call =
    std::unique_ptr<WebGLWorkerCall>{static_cast<WebGLWorkerCall*>(opaque)};
  call->function();
  return nullptr;
}

#endif

// Runs the session's core worker on a pthread that claims `selector`, which is
// what makes the canvas reachable from native code.
inline auto configure_transferred_webgl_worker(
  mln_render_session_object& session, std::string selector
) -> void {
#if defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
  auto thread = std::make_shared<pthread_t>();
  session.start_worker =
    [thread, selector = std::move(selector)](std::function<void()> function) {
      auto attributes = pthread_attr_t{};
      if (pthread_attr_init(&attributes) != 0) {
        set_thread_error("creating WebGL worker attributes failed");
        return MLN_STATUS_NATIVE_ERROR;
      }
      if (
        emscripten_pthread_attr_settransferredcanvases(
          &attributes, selector.c_str()
        ) != 0
      ) {
        pthread_attr_destroy(&attributes);
        set_thread_error("transferring the WebGL canvas failed");
        return MLN_STATUS_NATIVE_ERROR;
      }
      auto call =
        std::make_unique<WebGLWorkerCall>(WebGLWorkerCall{std::move(function)});
      const auto result =
        pthread_create(thread.get(), &attributes, run_webgl_worker, call.get());
      pthread_attr_destroy(&attributes);
      if (result != 0) {
        set_thread_error("creating the WebGL worker failed");
        return MLN_STATUS_NATIVE_ERROR;
      }
      static_cast<void>(call.release());
      return MLN_STATUS_OK;
    };
  // A session destroyed from its own completion would otherwise join itself.
  session.join_worker = [thread]() {
    if (pthread_equal(pthread_self(), *thread) != 0) {
      static_cast<void>(pthread_detach(*thread));
      return;
    }
    static_cast<void>(pthread_join(*thread, nullptr));
  };
#else
  static_cast<void>(session);
  static_cast<void>(selector);
#endif
}

// Creates the WebGL 2 context for a canvas this worker already owns, and
// records its handle in the descriptor the backend is built from.
inline auto create_transferred_webgl_context(
  mln_opengl_context_descriptor& context, const std::string& selector
) -> mln_status {
#if defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
  auto attributes = EmscriptenWebGLContextAttributes{};
  emscripten_webgl_init_context_attributes(&attributes);
  attributes.majorVersion = 2;
  attributes.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW;
  const auto created =
    emscripten_webgl_create_context(selector.c_str(), &attributes);
  if (created <= 0) {
    set_thread_error("creating the transferred WebGL 2 context failed");
    return MLN_STATUS_NATIVE_ERROR;
  }
  context.data.webgl.context = created;
  return MLN_STATUS_OK;
#else
  static_cast<void>(context);
  static_cast<void>(selector);
  set_thread_error(
    "transferred WebGL canvases are not supported by this build"
  );
  return MLN_STATUS_UNSUPPORTED;
#endif
}

}  // namespace mln::core::opengl
