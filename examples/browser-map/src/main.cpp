#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>

#include <emscripten.h>

#include "maplibre_native_c.h"

namespace {

constexpr auto keyboardAnimationMs = 160.0;
constexpr auto styleUrl = "https://tiles.openfreemap.org/styles/bright";

struct Viewport {
  uint32_t width;
  uint32_t height;
  double scaleFactor;
};

struct InitialCamera {
  double longitude;
  double latitude;
  double zoom;
  double bearing;
  double pitch;
};

class App {
 public:
  App(
    Viewport viewport, InitialCamera camera, void* nativeContext,
    void* nativeTarget, mln_status& outStatus
  )
      : viewport_(viewport) {
    outStatus = initialize(camera, nativeContext, nativeTarget);
  }

  ~App() {
    releaseOwnedTextureFrame();
    if (session_ != MLN_HANDLE_NULL) {
      report("destroy render session", mln_render_session_destroy(session_));
    }
    if (map_ != MLN_HANDLE_NULL) {
      report("destroy map", mln_map_destroy(map_));
    }
    if (runtime_ != MLN_HANDLE_NULL) {
      report("destroy runtime", mln_runtime_destroy(runtime_));
    }
  }

  auto renderFrame() -> bool {
    const auto pumpStatus = mln_runtime_pump(runtime_, 0);
    if (!check("pump runtime", pumpStatus)) {
      return false;
    }
    const auto eventRequestedRender = drainEvents();
    if (eventRequestedRender) {
      renderPending_ = true;
    }
    if (!renderPending_) {
      return false;
    }

    // Consume before rendering so this follows the shared render-request
    // contract. A render update that has no frame restores the request for the
    // next browser refresh.
    renderPending_ = false;
    auto rendered = false;
    const auto status = mln_render_session_render_update(session_, &rendered);
    if (status == MLN_STATUS_INVALID_STATE) {
      renderPending_ = true;
      return false;
    }
    if (!check("render update", status)) {
      return false;
    }
    if (!rendered) {
      renderPending_ = true;
      return false;
    }

    return true;
  }

  auto resize(Viewport viewport) -> bool {
    if (!releaseOwnedTextureFrame()) {
      return false;
    }
    if (!check(
          "resize render session",
          mln_render_session_resize(
            session_, viewport.width, viewport.height, viewport.scaleFactor
          )
        )) {
      return false;
    }
    viewport_ = viewport;
    logViewport();
    requestRender();
    return true;
  }

#if defined(MLN_COMPOSE_WEBGL)
  auto resizeBorrowed(Viewport viewport, int32_t webglContext, uint32_t texture)
    -> bool {
    if (session_ != MLN_HANDLE_NULL) {
      if (!check(
            "destroy render session", mln_render_session_destroy(session_)
          )) {
        return false;
      }
      session_ = MLN_HANDLE_NULL;
    }
    viewport_ = viewport;
    if (!attachBorrowedTexture(webglContext, texture)) {
      return false;
    }
    logViewport();
    requestRender();
    return true;
  }
#endif

  auto acquireOwnedTexture() -> uintptr_t {
#if defined(MLN_COMPOSE_WEBGL)
    return 0;
#else
    if (!releaseOwnedTextureFrame()) {
      return 0;
    }
    ownedFrame_ = mln_webgpu_owned_texture_frame{
      .size = sizeof(mln_webgpu_owned_texture_frame),
    };
    if (!check(
          "acquire owned texture frame",
          mln_webgpu_owned_texture_acquire_frame(session_, &ownedFrame_)
        )) {
      ownedFrame_ = {};
      return 0;
    }
    hasOwnedFrame_ = true;
    return reinterpret_cast<uintptr_t>(ownedFrame_.texture);
#endif
  }

  auto releaseOwnedTextureFrame() -> bool {
#if defined(MLN_COMPOSE_WEBGL)
    return true;
#else
    if (!hasOwnedFrame_) {
      return true;
    }
    const auto status =
      mln_webgpu_owned_texture_release_frame(session_, &ownedFrame_);
    ownedFrame_ = {};
    hasOwnedFrame_ = false;
    return check("release owned texture frame", status);
#endif
  }

  auto moveBy(double deltaX, double deltaY) -> bool {
    if (!check("move map", mln_map_move_by(map_, deltaX, deltaY))) {
      return false;
    }
    requestRender();
    return true;
  }

  auto moveByAnimated(double deltaX, double deltaY) -> bool {
    auto animation = keyboardAnimation();
    if (!check(
          "move map animated",
          mln_map_move_by_animated(map_, deltaX, deltaY, &animation)
        )) {
      return false;
    }
    requestRender();
    return true;
  }

  auto scaleBy(double scale, double x, double y) -> bool {
    const auto anchor = mln_screen_point{.x = x, .y = y};
    if (!check("scale map", mln_map_scale_by(map_, scale, &anchor))) {
      return false;
    }
    requestRender();
    return true;
  }

  auto scaleByAnimated(double scale, double x, double y) -> bool {
    const auto anchor = mln_screen_point{.x = x, .y = y};
    auto animation = keyboardAnimation();
    if (!check(
          "scale map animated",
          mln_map_scale_by_animated(map_, scale, &anchor, &animation)
        )) {
      return false;
    }
    requestRender();
    return true;
  }

  auto rotatePitchBy(double bearingDelta, double pitchDelta) -> bool {
    auto camera = currentCamera();
    return setOrientation(
      camera.bearing + bearingDelta, camera.pitch + pitchDelta, false
    );
  }

  auto rotateBy(double bearingDelta) -> bool {
    auto camera = currentCamera();
    return setOrientation(camera.bearing + bearingDelta, camera.pitch, true);
  }

  auto pitchBy(double pitchDelta) -> bool {
    auto camera = currentCamera();
    return setOrientation(camera.bearing, camera.pitch + pitchDelta, true);
  }

  auto resetOrientation() -> bool { return setOrientation(0.0, 0.0, true); }

  auto cancelTransitions() -> bool {
    return check("cancel camera transitions", mln_map_cancel_transitions(map_));
  }

  auto jumpTo(
    double longitude, double latitude, double zoom, double bearing, double pitch
  ) -> bool {
    auto camera = mln_camera_options_default();
    camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                    MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
    camera.longitude = longitude;
    camera.latitude = latitude;
    camera.zoom = zoom;
    camera.bearing = bearing;
    camera.pitch = clampPitch(pitch);
    if (!check("jump camera", mln_map_jump_to(map_, &camera))) {
      return false;
    }
    requestRender();
    return true;
  }

 private:
  struct CameraSnapshot {
    double bearing;
    double pitch;
  };

  auto initialize(InitialCamera camera, void* nativeContext, void* nativeTarget)
    -> mln_status {
    if (!validViewport(viewport_)) {
      std::fprintf(stderr, "browser map init failed: invalid viewport\n");
      return MLN_STATUS_INVALID_ARGUMENT;
    }

    const auto backends = mln_supported_render_backend_mask();
    std::fprintf(stderr, "supported render backends: 0x%x\n", backends);
#if defined(MLN_COMPOSE_WEBGL)
    std::fprintf(stderr, "render target: borrowed-texture\n");
    std::fprintf(
      stderr,
      "render target status: zero-copy WebGL texture sampled by Compose/Skia\n"
    );
#else
    std::fprintf(stderr, "render target: owned-texture\n");
    std::fprintf(
      stderr,
      "render target status: samples MapLibre-owned texture frames into the "
      "host swapchain\n"
    );
#endif
    std::fprintf(
      stderr,
      "Controls:\n"
      "  left drag: pan\n"
      "  right drag or Ctrl+left drag: rotate with X, pitch with Y\n"
      "  scroll: zoom at cursor\n"
      "  arrows or WASD: pan\n"
      "  + / -: zoom at center\n"
      "  Q / E: rotate\n"
      "  ] / [: pitch\n"
      "  0: reset pitch and bearing\n"
    );
    logViewport();
#if defined(MLN_COMPOSE_WEBGL)
    if ((backends & MLN_RENDER_BACKEND_FLAG_OPENGL) == 0) {
      std::fprintf(
        stderr, "browser map init failed: OpenGL backend is unavailable\n"
      );
      return MLN_STATUS_UNSUPPORTED;
    }
#else
    if ((backends & MLN_RENDER_BACKEND_FLAG_WEBGPU) == 0) {
      std::fprintf(
        stderr, "browser map init failed: WebGPU backend is unavailable\n"
      );
      return MLN_STATUS_UNSUPPORTED;
    }
#endif

    auto runtimeOptions = mln_runtime_options_default();
    runtimeOptions.cache_path = ":memory:";
    auto status = mln_runtime_create(&runtimeOptions, &runtime_);
    if (status != MLN_STATUS_OK) {
      logError("create runtime", status);
      return status;
    }

    auto mapOptions = mln_map_options_default();
    mapOptions.width = viewport_.width;
    mapOptions.height = viewport_.height;
    mapOptions.scale_factor = viewport_.scaleFactor;
    mapOptions.map_mode = MLN_MAP_MODE_CONTINUOUS;
    status = mln_map_create(runtime_, &mapOptions, &map_);
    if (status != MLN_STATUS_OK) {
      logError("create map", status);
      return status;
    }

    status = mln_map_set_style_url(map_, styleUrl);
    if (status != MLN_STATUS_OK) {
      logError("set style URL", status);
      return status;
    }

    auto cameraOptions = mln_camera_options_default();
    cameraOptions.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                           MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
    cameraOptions.longitude = camera.longitude;
    cameraOptions.latitude = camera.latitude;
    cameraOptions.zoom = camera.zoom;
    cameraOptions.bearing = camera.bearing;
    cameraOptions.pitch = clampPitch(camera.pitch);
    status = mln_map_jump_to(map_, &cameraOptions);
    if (status != MLN_STATUS_OK) {
      logError("set initial camera", status);
      return status;
    }

#if defined(MLN_COMPOSE_WEBGL)
    if (!attachBorrowedTexture(
          static_cast<int32_t>(reinterpret_cast<intptr_t>(nativeContext)),
          static_cast<uint32_t>(reinterpret_cast<uintptr_t>(nativeTarget))
        )) {
      return MLN_STATUS_NATIVE_ERROR;
    }
#else
    auto descriptor = mln_webgpu_owned_texture_descriptor_default();
    descriptor.extent.width = viewport_.width;
    descriptor.extent.height = viewport_.height;
    descriptor.extent.scale_factor = viewport_.scaleFactor;
    descriptor.context.device = nativeContext;
    descriptor.context.queue = nativeTarget;
    status = mln_webgpu_owned_texture_attach(map_, &descriptor, &session_);
    if (status != MLN_STATUS_OK) {
      logError("attach WebGPU owned texture", status);
      return status;
    }
#endif

    renderPending_ = true;
    return MLN_STATUS_OK;
  }

#if defined(MLN_COMPOSE_WEBGL)
  auto attachBorrowedTexture(int32_t webglContext, uint32_t texture) -> bool {
    uint32_t physicalWidth = 0;
    uint32_t physicalHeight = 0;
    if (!physicalSize(viewport_, physicalWidth, physicalHeight)) {
      return false;
    }
    auto descriptor = mln_opengl_borrowed_texture_descriptor_default();
    descriptor.extent.width = viewport_.width;
    descriptor.extent.height = viewport_.height;
    descriptor.extent.scale_factor = viewport_.scaleFactor;
    descriptor.physical_width = physicalWidth;
    descriptor.physical_height = physicalHeight;
    descriptor.context.platform = MLN_OPENGL_CONTEXT_PLATFORM_WEBGL;
    descriptor.context.data.webgl.size = sizeof(mln_webgl_context_descriptor);
    descriptor.context.data.webgl.context = webglContext;
    descriptor.texture = texture;
    descriptor.target = 0x0DE1;  // GL_TEXTURE_2D
    const auto status =
      mln_opengl_borrowed_texture_attach(map_, &descriptor, &session_);
    return check("attach WebGL borrowed texture", status);
  }
#endif

  auto drainEvents() -> bool {
    auto renderRequested = false;
    while (true) {
      auto event = mln_runtime_event{.size = sizeof(mln_runtime_event)};
      auto hasEvent = false;
      if (!check(
            "poll runtime event",
            mln_runtime_poll_event(runtime_, &event, &hasEvent)
          )) {
        return renderRequested;
      }
      if (!hasEvent) {
        return renderRequested;
      }
      if (
        event.source_type != MLN_RUNTIME_EVENT_SOURCE_MAP ||
        event.source != map_
      ) {
        continue;
      }
      switch (event.type) {
        case MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE:
          renderRequested = true;
          break;
        case MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED:
          renderRequested = renderRequested || frameNeedsRepaint(event);
          break;
        case MLN_RUNTIME_EVENT_MAP_RENDER_ERROR:
          logRuntimeEvent("render error", event);
          break;
        default:
          break;
      }
    }
  }

  static auto frameNeedsRepaint(const mln_runtime_event& event) -> bool {
    if (
      event.payload_type != MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME ||
      event.payload == nullptr ||
      event.payload_size < sizeof(mln_runtime_event_render_frame)
    ) {
      return false;
    }
    const auto* frame =
      static_cast<const mln_runtime_event_render_frame*>(event.payload);
    return frame->needs_repaint;
  }

  auto currentCamera() -> CameraSnapshot {
    auto camera = mln_camera_options_default();
    if (!check("get camera", mln_map_get_camera(map_, &camera))) {
      return CameraSnapshot{.bearing = 0.0, .pitch = 0.0};
    }
    return CameraSnapshot{
      .bearing = hasField(camera.fields, MLN_CAMERA_OPTION_BEARING)
                   ? camera.bearing
                   : 0.0,
      .pitch =
        hasField(camera.fields, MLN_CAMERA_OPTION_PITCH) ? camera.pitch : 0.0,
    };
  }

  auto setOrientation(double bearing, double pitch, bool animated) -> bool {
    auto camera = mln_camera_options_default();
    camera.fields = MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
    camera.bearing = bearing;
    camera.pitch = clampPitch(pitch);

    auto animation = keyboardAnimation();
    const auto status = animated ? mln_map_ease_to(map_, &camera, &animation)
                                 : mln_map_jump_to(map_, &camera);
    if (!check("set orientation", status)) {
      return false;
    }
    requestRender();
    return true;
  }

  void requestRender() { renderPending_ = true; }

  void logViewport() const {
    uint32_t physicalWidth = 0;
    uint32_t physicalHeight = 0;
    if (!physicalSize(viewport_, physicalWidth, physicalHeight)) {
      std::fprintf(stderr, "browser viewport: invalid extent\n");
      return;
    }
    std::fprintf(
      stderr, "browser viewport: logical=%ux%u physical=%ux%u scale=%.3f\n",
      viewport_.width, viewport_.height, physicalWidth, physicalHeight,
      viewport_.scaleFactor
    );
  }

  static auto validViewport(Viewport viewport) -> bool {
    uint32_t physicalWidth = 0;
    uint32_t physicalHeight = 0;
    return physicalSize(viewport, physicalWidth, physicalHeight);
  }

  static auto physicalSize(Viewport viewport, uint32_t& width, uint32_t& height)
    -> bool {
    const auto extent = mln_render_target_extent{
      .size = sizeof(mln_render_target_extent),
      .width = viewport.width,
      .height = viewport.height,
      .scale_factor = viewport.scaleFactor,
    };
    return mln_render_target_extent_physical_size(&extent, &width, &height) ==
           MLN_STATUS_OK;
  }

  static auto clampPitch(double pitch) -> double {
    return std::clamp(pitch, 0.0, 60.0);
  }

  static auto keyboardAnimation() -> mln_animation_options {
    auto animation = mln_animation_options_default();
    animation.fields = MLN_ANIMATION_OPTION_DURATION;
    animation.duration_ms = keyboardAnimationMs;
    return animation;
  }

  static auto hasField(uint32_t fields, uint32_t field) -> bool {
    return (fields & field) != 0;
  }

  static auto check(const char* operation, mln_status status) -> bool {
    if (status == MLN_STATUS_OK) {
      return true;
    }
    logError(operation, status);
    return false;
  }

  static void report(const char* operation, mln_status status) {
    if (status != MLN_STATUS_OK) {
      logError(operation, status);
    }
  }

  static void logError(const char* operation, mln_status status) {
    const auto* message = mln_thread_last_error_message();
    if (message != nullptr && message[0] != '\0') {
      std::fprintf(
        stderr, "%s failed with status %d: %s\n", operation,
        static_cast<int>(status), message
      );
    } else {
      std::fprintf(
        stderr, "%s failed with status %d\n", operation,
        static_cast<int>(status)
      );
    }
  }

  static void logRuntimeEvent(
    const char* operation, const mln_runtime_event& event
  ) {
    if (event.message != nullptr && event.message_size > 0) {
      std::fprintf(
        stderr, "%s: %.*s\n", operation, static_cast<int>(event.message_size),
        event.message
      );
    } else {
      std::fprintf(stderr, "%s\n", operation);
    }
  }

  Viewport viewport_;
  mln_runtime runtime_ = MLN_HANDLE_NULL;
  mln_map map_ = MLN_HANDLE_NULL;
  mln_render_session session_ = MLN_HANDLE_NULL;
#if !defined(MLN_COMPOSE_WEBGL)
  mln_webgpu_owned_texture_frame ownedFrame_ = {};
  bool hasOwnedFrame_ = false;
#endif
  bool renderPending_ = true;
};

std::unique_ptr<App> app;

auto withApp(bool defaultValue, auto action) -> bool {
  if (!app) {
    return defaultValue;
  }
  return action(*app);
}

}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_init(
  uint32_t logicalWidth, uint32_t logicalHeight, double scaleFactor,
  double longitude, double latitude, double zoom, double bearing, double pitch,
  void* webgpuDevice, void* webgpuQueue
) -> int {
  app.reset();
  auto status = MLN_STATUS_OK;
  auto nextApp = std::make_unique<App>(
    Viewport{
      .width = logicalWidth,
      .height = logicalHeight,
      .scaleFactor = scaleFactor,
    },
    InitialCamera{
      .longitude = longitude,
      .latitude = latitude,
      .zoom = zoom,
      .bearing = bearing,
      .pitch = pitch,
    },
    webgpuDevice, webgpuQueue, status
  );
  if (status != MLN_STATUS_OK) {
    return 1;
  }
  app = std::move(nextApp);
  return 0;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_render_frame() -> int {
  return withApp(false, [](App& current) { return current.renderFrame(); }) ? 1
                                                                            : 0;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_acquire_owned_texture() -> uintptr_t {
  if (!app) {
    return 0;
  }
  return app->acquireOwnedTexture();
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_release_owned_texture_frame() -> int {
  return withApp(
           false,
           [](App& current) { return current.releaseOwnedTextureFrame(); }
         )
           ? 0
           : 1;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_resize(
  uint32_t logicalWidth, uint32_t logicalHeight, double scaleFactor
) -> int {
  return withApp(
           false,
           [&](App& current) {
             return current.resize(
               Viewport{
                 .width = logicalWidth,
                 .height = logicalHeight,
                 .scaleFactor = scaleFactor,
               }
             );
           }
         )
           ? 0
           : 1;
}

#if defined(MLN_COMPOSE_WEBGL)
EMSCRIPTEN_KEEPALIVE auto mln_browser_map_resize_borrowed(
  uint32_t logicalWidth, uint32_t logicalHeight, double scaleFactor,
  int32_t webglContext, uint32_t texture
) -> int {
  return withApp(
           false,
           [&](App& current) {
             return current.resizeBorrowed(
               Viewport{
                 .width = logicalWidth,
                 .height = logicalHeight,
                 .scaleFactor = scaleFactor,
               },
               webglContext, texture
             );
           }
         )
           ? 0
           : 1;
}
#endif

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_move_by(double deltaX, double deltaY)
  -> int {
  return withApp(
           false, [&](App& current) { return current.moveBy(deltaX, deltaY); }
         )
           ? 0
           : 1;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_move_by_animated(
  double deltaX, double deltaY
) -> int {
  return withApp(
           false,
           [&](App& current) { return current.moveByAnimated(deltaX, deltaY); }
         )
           ? 0
           : 1;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_scale_by(
  double scale, double x, double y
) -> int {
  return withApp(
           false, [&](App& current) { return current.scaleBy(scale, x, y); }
         )
           ? 0
           : 1;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_scale_by_animated(
  double scale, double x, double y
) -> int {
  return withApp(
           false,
           [&](App& current) { return current.scaleByAnimated(scale, x, y); }
         )
           ? 0
           : 1;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_rotate_pitch_by(
  double bearingDelta, double pitchDelta
) -> int {
  return withApp(
           false,
           [&](App& current) {
             return current.rotatePitchBy(bearingDelta, pitchDelta);
           }
         )
           ? 0
           : 1;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_rotate_by(double bearingDelta)
  -> int {
  return withApp(
           false, [&](App& current) { return current.rotateBy(bearingDelta); }
         )
           ? 0
           : 1;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_pitch_by(double pitchDelta) -> int {
  return withApp(
           false, [&](App& current) { return current.pitchBy(pitchDelta); }
         )
           ? 0
           : 1;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_reset_orientation() -> int {
  return withApp(false, [](App& current) { return current.resetOrientation(); })
           ? 0
           : 1;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_cancel_transitions() -> int {
  return withApp(
           false, [](App& current) { return current.cancelTransitions(); }
         )
           ? 0
           : 1;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_jump_to(
  double longitude, double latitude, double zoom, double bearing, double pitch
) -> int {
  return withApp(
           false,
           [&](App& current) {
             return current.jumpTo(longitude, latitude, zoom, bearing, pitch);
           }
         )
           ? 0
           : 1;
}

}  // extern "C"

auto main() -> int { return 0; }
