#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>

#include <emscripten.h>
#include <emscripten/heap.h>

#include "maplibre_native_c.h"

extern "C" void mln_emscripten_http_trace_set(int enabled);
extern "C" void mln_emscripten_run_loop_trace_set(int enabled);
extern "C" auto mln_emscripten_run_loop_last_ready_count() -> std::size_t;
extern "C" auto mln_emscripten_run_loop_last_runnable_count() -> std::size_t;
extern "C" auto mln_emscripten_run_loop_last_runnables_ms() -> double;

namespace {

struct MallInfo {
  size_t arena;
  size_t ordblks;
  size_t smblks;
  size_t hblks;
  size_t hblkhd;
  size_t usmblks;
  size_t fsmblks;
  size_t uordblks;
  size_t fordblks;
  size_t keepcost;
};

extern "C" auto mallinfo() -> MallInfo;

constexpr auto keyboardAnimationMs = 160.0;
constexpr auto benchmarkFlyAnimationMs = 2500.0;
constexpr auto styleUrl = "https://tiles.openfreemap.org/styles/bright";
constexpr auto slowFrameTraceThresholdMs = 30.0;

std::atomic_bool traceEnabled = false;
std::atomic_bool terminateHookInstalled = false;
std::atomic<const char*> lastOperation = "startup";
thread_local const char* currentOperation = "idle";
std::terminate_handler previousTerminateHandler = nullptr;

auto tracing() -> bool { return traceEnabled.load(std::memory_order_relaxed); }

void traceLog(const char* format, ...) {
  if (!tracing()) {
    return;
  }
  std::fprintf(stderr, "browser trace: ");
  va_list args;
  va_start(args, format);
  std::vfprintf(stderr, format, args);
  va_end(args);
  std::fprintf(stderr, "\n");
}

void terminateHandler() {
  auto* exceptionMessage = "no active exception";
  auto exception = std::current_exception();
  if (exception) {
    try {
      std::rethrow_exception(exception);
    } catch (const std::exception& caught) {
      exceptionMessage = caught.what();
    } catch (...) {
      exceptionMessage = "unknown active exception";
    }
  }
  std::fprintf(
    stderr,
    "browser trace: std::terminate while current_operation=%s "
    "last_operation=%s exception=%s\n",
    currentOperation, lastOperation.load(std::memory_order_relaxed),
    exceptionMessage
  );
  if (
    previousTerminateHandler != nullptr &&
    previousTerminateHandler != terminateHandler
  ) {
    previousTerminateHandler();
  }
  std::abort();
}

void installTerminateHook() {
  auto expected = false;
  if (terminateHookInstalled.compare_exchange_strong(expected, true)) {
    previousTerminateHandler = std::set_terminate(terminateHandler);
  }
}

class OperationTrace {
 public:
  explicit OperationTrace(const char* operation) : previous_(currentOperation) {
    currentOperation = operation;
    lastOperation.store(operation, std::memory_order_relaxed);
  }

  ~OperationTrace() { currentOperation = previous_; }

 private:
  const char* previous_;
};

struct Viewport {
  uint32_t width;
  uint32_t height;
  double scaleFactor;
};

class App {
  using Clock = std::chrono::steady_clock;

  struct FrameTimings {
    double runLoopMs = 0.0;
    double runnableMs = 0.0;
    double eventDrainMs = 0.0;
    double renderUpdateMs = 0.0;
    std::size_t readyRunnableCount = 0;
    std::size_t runnableCount = 0;
  };

 public:
  App(
    Viewport viewport, void* webgpuDevice, void* webgpuQueue,
    mln_status& outStatus
  )
      : viewport_(viewport) {
    outStatus = initialize(webgpuDevice, webgpuQueue);
  }

  ~App() {
    releaseOwnedTextureFrame();
    if (session_ != nullptr) {
      report("destroy render session", mln_render_session_destroy(session_));
    }
    if (map_ != nullptr) {
      report("destroy map", mln_map_destroy(map_));
    }
    if (runtime_ != nullptr) {
      report("destroy runtime", mln_runtime_destroy(runtime_));
    }
  }

  auto renderFrame() -> bool {
    const auto frameStart = Clock::now();
    auto frameRendered = false;
    const auto runLoopStart = Clock::now();
    const auto runLoopStatus = mln_runtime_run_once(runtime_);
    lastFrameTimings_.runLoopMs = elapsedMs(runLoopStart);
    lastFrameTimings_.runnableMs = mln_emscripten_run_loop_last_runnables_ms();
    lastFrameTimings_.readyRunnableCount =
      mln_emscripten_run_loop_last_ready_count();
    lastFrameTimings_.runnableCount =
      mln_emscripten_run_loop_last_runnable_count();
    lastFrameTimings_.eventDrainMs = 0.0;
    lastFrameTimings_.renderUpdateMs = 0.0;
    if (!check("run runtime", runLoopStatus)) {
      return false;
    }
    const auto eventDrainStart = Clock::now();
    const auto eventRequestedRender = drainEvents();
    lastFrameTimings_.eventDrainMs = elapsedMs(eventDrainStart);
    if (eventRequestedRender) {
      renderPending_ = true;
    }
    if (!renderPending_) {
      return false;
    }

    const auto renderUpdateStart = Clock::now();
    const auto status = mln_render_session_render_update(session_);
    lastFrameTimings_.renderUpdateMs = elapsedMs(renderUpdateStart);
    if (status == MLN_STATUS_INVALID_STATE) {
      return false;
    }
    if (!check("render update", status)) {
      return false;
    }

    renderPending_ = false;
    frameRendered = true;
    const auto frameMs = elapsedMs(frameStart);
    if (frameMs >= slowFrameTraceThresholdMs) {
      traceLog(
        "slow frame total=%.3fms run_loop=%.3fms event_drain=%.3fms "
        "render_update=%.3fms runnables=%.3fms ready=%zu total=%zu",
        frameMs, lastFrameTimings_.runLoopMs, lastFrameTimings_.eventDrainMs,
        lastFrameTimings_.renderUpdateMs, lastFrameTimings_.runnableMs,
        lastFrameTimings_.readyRunnableCount, lastFrameTimings_.runnableCount
      );
    }
    return frameRendered;
  }

  auto resize(Viewport viewport) -> bool {
    OperationTrace operation("resize");
    traceLog(
      "resize %ux%u scale %.3f", viewport.width, viewport.height,
      viewport.scaleFactor
    );
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

  auto acquireOwnedTexture() -> uintptr_t {
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
  }

  auto releaseOwnedTextureFrame() -> bool {
    if (!hasOwnedFrame_) {
      return true;
    }
    const auto status =
      mln_webgpu_owned_texture_release_frame(session_, &ownedFrame_);
    ownedFrame_ = {};
    hasOwnedFrame_ = false;
    return check("release owned texture frame", status);
  }

  auto moveBy(double deltaX, double deltaY) -> bool {
    OperationTrace operation("move_by");
    if (!check("move map", mln_map_move_by(map_, deltaX, deltaY))) {
      return false;
    }
    requestRender();
    return true;
  }

  auto moveByAnimated(double deltaX, double deltaY) -> bool {
    OperationTrace operation("move_by_animated");
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
    OperationTrace operation("scale_by");
    traceLog("scale_by scale=%.9f anchor=(%.3f, %.3f)", scale, x, y);
    const auto anchor = mln_screen_point{.x = x, .y = y};
    if (!check("scale map", mln_map_scale_by(map_, scale, &anchor))) {
      return false;
    }
    requestRender();
    return true;
  }

  auto scaleByAnimated(double scale, double x, double y) -> bool {
    OperationTrace operation("scale_by_animated");
    traceLog("scale_by_animated scale=%.9f anchor=(%.3f, %.3f)", scale, x, y);
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
    OperationTrace operation("rotate_pitch_by");
    traceLog(
      "rotate_pitch_by bearing_delta=%.3f pitch_delta=%.3f", bearingDelta,
      pitchDelta
    );
    auto camera = currentCamera();
    return setOrientation(
      camera.bearing + bearingDelta, camera.pitch + pitchDelta, false
    );
  }

  auto rotateBy(double bearingDelta) -> bool {
    OperationTrace operation("rotate_by");
    auto camera = currentCamera();
    return setOrientation(camera.bearing + bearingDelta, camera.pitch, true);
  }

  auto pitchBy(double pitchDelta) -> bool {
    OperationTrace operation("pitch_by");
    auto camera = currentCamera();
    return setOrientation(camera.bearing, camera.pitch + pitchDelta, true);
  }

  auto resetOrientation() -> bool { return setOrientation(0.0, 0.0, true); }

  auto jumpTo(
    double longitude, double latitude, double zoom, double bearing, double pitch
  ) -> bool {
    OperationTrace operation("jump_to");
    traceLog(
      "jump_to lon=%.6f lat=%.6f zoom=%.3f bearing=%.3f pitch=%.3f", longitude,
      latitude, zoom, bearing, pitch
    );
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

  auto flyTo(
    double longitude, double latitude, double zoom, double bearing, double pitch
  ) -> bool {
    OperationTrace operation("fly_to");
    traceLog(
      "fly_to lon=%.6f lat=%.6f zoom=%.3f bearing=%.3f pitch=%.3f", longitude,
      latitude, zoom, bearing, pitch
    );
    auto camera = mln_camera_options_default();
    camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                    MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
    camera.longitude = longitude;
    camera.latitude = latitude;
    camera.zoom = zoom;
    camera.bearing = bearing;
    camera.pitch = clampPitch(pitch);
    auto animation = mln_animation_options_default();
    animation.fields = MLN_ANIMATION_OPTION_DURATION;
    animation.duration_ms = benchmarkFlyAnimationMs;
    if (!check("fly camera", mln_map_fly_to(map_, &camera, &animation))) {
      return false;
    }
    requestRender();
    return true;
  }

  auto isFullyLoaded() -> bool {
    auto loaded = false;
    if (!check("check fully loaded", mln_map_is_fully_loaded(map_, &loaded))) {
      return false;
    }
    return loaded;
  }

  auto lastRunLoopMs() const -> double { return lastFrameTimings_.runLoopMs; }

  auto lastRunnableMs() const -> double { return lastFrameTimings_.runnableMs; }

  auto lastEventDrainMs() const -> double {
    return lastFrameTimings_.eventDrainMs;
  }

  auto lastRenderUpdateMs() const -> double {
    return lastFrameTimings_.renderUpdateMs;
  }

  auto lastReadyRunnableCount() const -> std::size_t {
    return lastFrameTimings_.readyRunnableCount;
  }

  auto lastRunnableCount() const -> std::size_t {
    return lastFrameTimings_.runnableCount;
  }

 private:
  struct CameraSnapshot {
    double bearing;
    double pitch;
  };

  static auto elapsedMs(Clock::time_point start) -> double {
    return std::chrono::duration<double, std::milli>(Clock::now() - start)
      .count();
  }

  auto initialize(void* webgpuDevice, void* webgpuQueue) -> mln_status {
    if (!validViewport(viewport_)) {
      std::fprintf(stderr, "browser map init failed: invalid viewport\n");
      return MLN_STATUS_INVALID_ARGUMENT;
    }

    const auto backends = mln_supported_render_backend_mask();
    std::fprintf(stderr, "supported render backends: 0x%x\n", backends);
    logViewport();
    if ((backends & MLN_RENDER_BACKEND_FLAG_WEBGPU) == 0) {
      std::fprintf(
        stderr, "browser map init failed: WebGPU backend is unavailable\n"
      );
      return MLN_STATUS_UNSUPPORTED;
    }

    auto status = mln_runtime_create(nullptr, &runtime_);
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

    auto descriptor = mln_webgpu_owned_texture_descriptor_default();
    descriptor.extent.width = viewport_.width;
    descriptor.extent.height = viewport_.height;
    descriptor.extent.scale_factor = viewport_.scaleFactor;
    descriptor.context.device = webgpuDevice;
    descriptor.context.queue = webgpuQueue;
    status = mln_webgpu_owned_texture_attach(map_, &descriptor, &session_);
    if (status != MLN_STATUS_OK) {
      logError("attach WebGPU owned texture", status);
      return status;
    }

    renderPending_ = true;
    return MLN_STATUS_OK;
  }

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
    std::fprintf(
      stderr, "browser viewport: %ux%u scale %.3f\n", viewport_.width,
      viewport_.height, viewport_.scaleFactor
    );
  }

  static auto validViewport(Viewport viewport) -> bool {
    return viewport.width > 0 && viewport.height > 0 &&
           std::isfinite(viewport.scaleFactor) && viewport.scaleFactor > 0.0;
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
  mln_runtime* runtime_ = nullptr;
  mln_map* map_ = nullptr;
  mln_render_session* session_ = nullptr;
  mln_webgpu_owned_texture_frame ownedFrame_ = {};
  FrameTimings lastFrameTimings_;
  bool hasOwnedFrame_ = false;
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

EMSCRIPTEN_KEEPALIVE void mln_browser_map_set_trace(int enabled) {
  const auto shouldTrace = enabled != 0;
  traceEnabled.store(shouldTrace, std::memory_order_relaxed);
  if (shouldTrace) {
    installTerminateHook();
  }
  mln_emscripten_http_trace_set(enabled);
  mln_emscripten_run_loop_trace_set(enabled);
  traceLog("trace enabled");
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_init(
  uint32_t logicalWidth, uint32_t logicalHeight, double scaleFactor,
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

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_fly_to(
  double longitude, double latitude, double zoom, double bearing, double pitch
) -> int {
  return withApp(
           false,
           [&](App& current) {
             return current.flyTo(longitude, latitude, zoom, bearing, pitch);
           }
         )
           ? 0
           : 1;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_is_fully_loaded() -> int {
  return app && app->isFullyLoaded() ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_last_run_loop_ms() -> double {
  return app ? app->lastRunLoopMs() : 0.0;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_last_runnable_ms() -> double {
  return app ? app->lastRunnableMs() : 0.0;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_last_event_drain_ms() -> double {
  return app ? app->lastEventDrainMs() : 0.0;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_last_render_update_ms() -> double {
  return app ? app->lastRenderUpdateMs() : 0.0;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_last_ready_runnable_count()
  -> std::size_t {
  return app ? app->lastReadyRunnableCount() : 0;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_last_runnable_count() -> std::size_t {
  return app ? app->lastRunnableCount() : 0;
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_heap_size() -> double {
  return static_cast<double>(emscripten_get_heap_size());
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_heap_max() -> double {
  return static_cast<double>(emscripten_get_heap_max());
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_malloc_arena() -> double {
  return static_cast<double>(mallinfo().arena);
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_malloc_allocated() -> double {
  return static_cast<double>(mallinfo().uordblks);
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_malloc_free() -> double {
  return static_cast<double>(mallinfo().fordblks);
}

EMSCRIPTEN_KEEPALIVE auto mln_browser_map_malloc_keepcost() -> double {
  return static_cast<double>(mallinfo().keepcost);
}

}  // extern "C"

auto main() -> int { return 0; }
