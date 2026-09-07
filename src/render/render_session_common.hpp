#pragma once

#include <algorithm>
#include <any>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <mln/actor/scheduler.hpp>
#include <mln/gfx/headless_backend.hpp>
#include <mln/gfx/renderable.hpp>
#include <mln/gfx/renderer_backend.hpp>
#include <mln/renderer/renderer.hpp>
#include <mln/renderer/renderer_observer.hpp>
#include <mln/util/feature.hpp>
#include <mln/util/size.hpp>

#include "diagnostics/diagnostics.hpp"
#include "execution/worker_thread.hpp"
#include "handles/handle_table.hpp"
#include "map/feature_state.hpp"
#include "maplibre_native_c.h"
#include "operation/operation.hpp"
#include "render/discard_present.hpp"
#include "wake/wake.hpp"

struct mln_render_session_object;

namespace mln {
class UpdateParameters;
}

namespace mln::core {

enum class RenderSessionKind : uint8_t { Surface, Texture };
enum class TextureSessionMode : uint8_t { Owned, Borrowed };

// Which renderer cache a maintenance submission releases.
enum class RenderSessionMaintenance : uint8_t {
  ReduceMemoryUse,
  ClearData,
  DumpDebugLogs,
};

// Reports a target replacement the backend does not implement. Compiled GPU
// state lives partly outside the renderer, so a target that state cannot serve
// is refused instead of rebuilt; the host destroys the session and attaches
// again. Backends check before mutating anything, so a refusal leaves the
// session rendering into the target it already had.
auto unsupported_retarget(const char* message) -> mln_status;

class SurfaceSessionBackend {
 public:
  SurfaceSessionBackend() = default;
  SurfaceSessionBackend(const SurfaceSessionBackend&) = delete;
  auto operator=(const SurfaceSessionBackend&)
    -> SurfaceSessionBackend& = delete;
  SurfaceSessionBackend(SurfaceSessionBackend&&) = delete;
  auto operator=(SurfaceSessionBackend&&) -> SurfaceSessionBackend& = delete;
  virtual ~SurfaceSessionBackend() = default;

  virtual auto renderer_backend() -> mln::gfx::RendererBackend& = 0;
  virtual void resize(uint32_t physical_width, uint32_t physical_height) = 0;

  // Whether the surface can take a frame right now. Not ready skips the frame
  // and reports nothing rendered, so a minimized or occluded window is a retry
  // rather than a failure.
  virtual auto prepare_frame(bool& out_ready) -> mln_status {
    out_ready = true;
    return MLN_STATUS_OK;
  }

  // Presents through a new host surface, keeping the graphics context and every
  // resource the renderer holds against it. The descriptor must name the
  // context this session attached with; a backend rejects anything else.
  virtual auto set_metal_target(const mln_metal_surface_descriptor& descriptor)
    -> mln_status {
    (void)descriptor;
    return unsupported_retarget(
      "session does not render through a Metal surface"
    );
  }
  virtual auto set_vulkan_target(
    const mln_vulkan_surface_descriptor& descriptor
  ) -> mln_status {
    (void)descriptor;
    return unsupported_retarget(
      "session does not render through a Vulkan surface"
    );
  }
  virtual auto set_opengl_target(
    const mln_opengl_surface_descriptor& descriptor
  ) -> mln_status {
    (void)descriptor;
    return unsupported_retarget(
      "session does not render through an OpenGL surface"
    );
  }
  virtual auto set_webgpu_target(
    const mln_webgpu_surface_descriptor& descriptor
  ) -> mln_status {
    (void)descriptor;
    return unsupported_retarget(
      "session does not render through a WebGPU surface"
    );
  }
};

// What an acquirable frame's metadata takes from the session rather than from
// the graphics resource. Captured under the session lock and handed to the
// driver thread, so a backend never reads session state without it.
struct RenderFrameMetadata {
  uint64_t generation = 0;
  uint64_t frame_id = 0;
  uint32_t physical_width = 0;
  uint32_t physical_height = 0;
  double scale_factor = 1.0;
};

class TextureSessionBackend {
 public:
  TextureSessionBackend() = default;
  TextureSessionBackend(const TextureSessionBackend&) = delete;
  auto operator=(const TextureSessionBackend&)
    -> TextureSessionBackend& = delete;
  TextureSessionBackend(TextureSessionBackend&&) = delete;
  auto operator=(TextureSessionBackend&&) -> TextureSessionBackend& = delete;
  virtual ~TextureSessionBackend() = default;

  virtual auto headless_backend() -> mln::gfx::HeadlessBackend& = 0;
  virtual auto renderer_backend() -> mln::gfx::RendererBackend* {
    return headless_backend().getRendererBackend();
  }
  // Follows a new physical size. The default drops the renderable resource and
  // rebuilds it lazily; a backend whose renderer keys cached GPU state on that
  // resource overrides this to rebuild only what the size changed.
  virtual void resize(mln::Size size) { headless_backend().setSize(size); }

  // Renders into a new caller-owned texture, keeping the graphics context and
  // every resource the renderer holds against it. The descriptor must name the
  // context this session attached with; a backend rejects anything else.
  virtual auto set_metal_borrowed_target(
    const mln_metal_borrowed_texture_descriptor& descriptor
  ) -> mln_status {
    (void)descriptor;
    return unsupported_retarget(
      "session does not render into a caller-owned Metal texture"
    );
  }
  virtual auto set_vulkan_borrowed_target(
    const mln_vulkan_borrowed_texture_descriptor& descriptor
  ) -> mln_status {
    (void)descriptor;
    return unsupported_retarget(
      "session does not render into a caller-owned Vulkan image"
    );
  }
  virtual auto set_opengl_borrowed_target(
    const mln_opengl_borrowed_texture_descriptor& descriptor
  ) -> mln_status {
    (void)descriptor;
    return unsupported_retarget(
      "session does not render into a caller-owned OpenGL texture"
    );
  }
  virtual auto set_webgpu_borrowed_target(
    const mln_webgpu_borrowed_texture_descriptor& descriptor
  ) -> mln_status {
    (void)descriptor;
    return unsupported_retarget(
      "session does not render into a caller-owned WebGPU texture"
    );
  }

  // Whether headless_backend().readStillImage() produces an image. False makes
  // the C API answer UNSUPPORTED instead of reporting an empty image.
  [[nodiscard]] virtual auto supports_readback() const -> bool { return true; }

  virtual void prepare_render_resources() {}
  virtual auto after_render(
    mln_render_session_object& session, bool& out_rendered
  ) -> mln_status {
    (void)session;
    out_rendered = true;
    return MLN_STATUS_OK;
  }
  // Rejecting a sync kind here keeps the release synchronous and all-or-
  // nothing: the caller keeps frame ownership instead of losing the handle to
  // a release whose wait can never run.
  virtual auto supports_consumer_sync(mln_gpu_sync_kind kind) -> bool {
    return kind == MLN_GPU_SYNC_CPU_COMPLETE;
  }
  virtual auto release_consumer_sync(const mln_gpu_sync& sync) -> mln_status {
    return sync.kind == MLN_GPU_SYNC_CPU_COMPLETE ? MLN_STATUS_OK
                                                  : MLN_STATUS_UNSUPPORTED;
  }
  // Records the backend metadata a host reads back from an acquired frame.
  // Runs on the driver thread, right after the render, for the slot the
  // backend still has selected, so it may touch graphics state; the acquiring
  // thread only copies what this stored. NOT_READY reports a slot with no
  // publishable texture yet.
  virtual auto record_frame_metadata(const RenderFrameMetadata&, std::any&)
    -> mln_status {
    return MLN_STATUS_UNSUPPORTED;
  }
  virtual auto select_render_slot(std::size_t) -> mln_status {
    return MLN_STATUS_OK;
  }
};

// The scheduler mbgl sees as current while a render session renders. Work
// created during a render — tile mailboxes, file-source requests, the
// style-image-missing continuation — is delivered here, and must run on the
// thread that owns the renderer: tile mailboxes mutate state that
// RenderOrchestrator reads every frame.
//
// Lifetime is the session's. Mailboxes created during a render hold a WeakPtr
// to it, so it outlives every message in flight.
class RenderSessionScheduler final : public mln::Scheduler {
 public:
  RenderSessionScheduler() = default;
  RenderSessionScheduler(const RenderSessionScheduler&) = delete;
  auto operator=(const RenderSessionScheduler&)
    -> RenderSessionScheduler& = delete;
  RenderSessionScheduler(RenderSessionScheduler&&) = delete;
  auto operator=(RenderSessionScheduler&&) -> RenderSessionScheduler& = delete;
  ~RenderSessionScheduler() override { weak_factory_.invalidateWeakPtrs(); }

  void schedule(std::function<void()>&& task) override;
  void schedule(
    const mln::util::SimpleIdentity, std::function<void()>&& task
  ) override;
  auto makeWeakPtr() -> mapbox::base::WeakPtr<mln::Scheduler> override {
    return weak_factory_.makeWeakPtr();
  }
  // Only the graphics thread may run this queue, so another caller gets a
  // no-op rather than tasks running off the graphics thread.
  void waitForEmpty(
    const mln::util::SimpleIdentity = mln::util::SimpleIdentity::Empty
  ) override {
    if (mln::Scheduler::GetCurrent(/*init=*/false) == this) {
      drain();
    }
  }

  // Runs queued work on the calling graphics thread. Loops until the queue is
  // empty, because a task may enqueue more.
  auto drain() -> void;

  // Drops queued work without running it, for detach.
  auto discard() -> void;

  // Requests a host frame when work makes an idle queue nonempty. Cleared
  // before detach so late worker results are discarded.
  auto set_repaint_request(std::function<void()> repaint_request) -> void;

 private:
  // Reopens the queue and wakes pending work if drain() exits through an
  // exception.
  class DrainGuard {
   public:
    explicit DrainGuard(RenderSessionScheduler& scheduler)
        : scheduler_(scheduler) {}
    DrainGuard(const DrainGuard&) = delete;
    auto operator=(const DrainGuard&) -> DrainGuard& = delete;
    DrainGuard(DrainGuard&&) = delete;
    auto operator=(DrainGuard&&) -> DrainGuard& = delete;
    ~DrainGuard();

   private:
    RenderSessionScheduler& scheduler_;
  };

  std::mutex mutex_;
  std::vector<std::function<void()>> queue_;
  std::function<void()> repaint_request_;
  bool draining_ = false;
  mapbox::base::WeakPtrFactory<mln::Scheduler> weak_factory_{this};
  // Do not add members here, see `WeakPtrFactory`
};

// Gives the calling thread a current mbgl scheduler for the duration of a
// session call, but only when it does not already have one.
//
// GetCurrent(false) is required: the default would create a thread-local
// RunLoop whose lifetime and task queue are not owned by the render session.
class ScopedCurrentScheduler {
 public:
  explicit ScopedCurrentScheduler(mln::Scheduler& scheduler)
      : previous_(mln::Scheduler::GetCurrent(/*init=*/false)) {
    if (previous_ == nullptr) {
      mln::Scheduler::SetCurrent(&scheduler);
    }
  }
  ScopedCurrentScheduler(const ScopedCurrentScheduler&) = delete;
  auto operator=(const ScopedCurrentScheduler&)
    -> ScopedCurrentScheduler& = delete;
  ScopedCurrentScheduler(ScopedCurrentScheduler&&) = delete;
  auto operator=(ScopedCurrentScheduler&&) -> ScopedCurrentScheduler& = delete;
  ~ScopedCurrentScheduler() { mln::Scheduler::SetCurrent(previous_); }

 private:
  mln::Scheduler* previous_;
};

struct RenderSurfaceState {
  std::unique_ptr<SurfaceSessionBackend> backend = nullptr;
};

struct RenderTextureSlot {
  mln_render_frame_result result{};
  mln_gpu_sync producer_sync{
    .size = sizeof(mln_gpu_sync),
    .kind = MLN_GPU_SYNC_CPU_COMPLETE,
    .object = 0,
    .value = 0
  };
  // Recorded by the driver thread when the slot's frame is published. Empty
  // until then, which makes the slot unacquirable.
  std::any backend_metadata;
  bool available = false;
  bool acquired = false;
  bool rendering = false;
};

// The renderable resources a session-owned texture backend cycles through. The
// selected slot's resource lives in the backend's own `resource` member, which
// mbgl reads every frame; this parks the others and drops any whose recorded
// size no longer matches the backend's. A borrowed target has no ring, so an
// empty one accepts size records and refuses every selection.
class RenderableSlotRing {
 public:
  explicit RenderableSlotRing(std::size_t depth)
      : resources_(depth), sizes_(depth) {}

  [[nodiscard]] auto selected_size() const -> mln::Size {
    return sizes_.empty() ? mln::Size{} : sizes_[selected_];
  }

  auto record_size(mln::Size size) -> void {
    if (!sizes_.empty()) sizes_[selected_] = size;
  }

  auto select(
    std::size_t slot, mln::Size size,
    std::unique_ptr<mln::gfx::RenderableResource>& resource
  ) -> bool {
    if (slot >= resources_.size()) return false;
    if (slot == selected_) {
      if (resource != nullptr && sizes_[slot] != size) resource.reset();
      return true;
    }
    resources_[selected_] = std::move(resource);
    if (resources_[slot] != nullptr && sizes_[slot] != size) {
      resources_[slot].reset();
    }
    resource = std::move(resources_[slot]);
    selected_ = slot;
    return true;
  }

  auto clear() -> void {
    resources_.clear();
    sizes_.clear();
    selected_ = 0;
  }

 private:
  std::vector<std::unique_ptr<mln::gfx::RenderableResource>> resources_;
  std::vector<mln::Size> sizes_;
  std::size_t selected_ = 0;
};

// Records the frame status that mln::Renderer reports synchronously out of
// render(), so render_session_render_update_on_driver() can return its repaint
// flag with the terminal frame result, and forwards every callback to the
// map's observer so runtime event delivery is unchanged. Runs entirely on the
// session's driver thread.
class SessionFrameObserver final : public mln::RendererObserver {
 public:
  auto set_delegate(mln::RendererObserver* delegate) -> void {
    delegate_ = delegate;
  }

  [[nodiscard]] auto needs_repaint() const -> bool { return needs_repaint_; }

  auto suppress_frame_callbacks(bool suppress) -> void {
    suppress_frame_callbacks_ = suppress;
  }

  void onInvalidate() override {
    if (delegate_ != nullptr) {
      delegate_->onInvalidate();
    }
  }

  void onResourceError(std::exception_ptr error) override {
    if (delegate_ != nullptr) {
      delegate_->onResourceError(error);
    }
  }

  void onWillStartRenderingMap() override {
    if (suppress_frame_callbacks_ || delegate_ == nullptr) {
      return;
    }
    delegate_->onWillStartRenderingMap();
  }

  void onWillStartRenderingFrame() override {
    if (suppress_frame_callbacks_ || delegate_ == nullptr) {
      return;
    }
    delegate_->onWillStartRenderingFrame();
  }

  void onDidFinishRenderingFrame(
    RenderMode mode, bool repaint, bool placement_changed,
    const mln::gfx::RenderingStats& stats
  ) override {
    if (suppress_frame_callbacks_) {
      return;
    }
    needs_repaint_ = repaint;
    if (delegate_ != nullptr) {
      delegate_->onDidFinishRenderingFrame(
        mode, repaint, placement_changed, stats
      );
    }
  }

  void onDidFinishRenderingMap() override {
    if (suppress_frame_callbacks_ || delegate_ == nullptr) {
      return;
    }
    delegate_->onDidFinishRenderingMap();
  }

  void onStyleImageMissing(
    const std::string& id, const StyleImageMissingCallback& done
  ) override {
    if (delegate_ != nullptr) {
      delegate_->onStyleImageMissing(id, done);
    }
  }

  void onRemoveUnusedStyleImages(const std::vector<std::string>& ids) override {
    if (delegate_ != nullptr) {
      delegate_->onRemoveUnusedStyleImages(ids);
    }
  }

  void onPreCompileShader(
    mln::shaders::BuiltIn id, mln::gfx::Backend::Type type,
    const std::string& defines
  ) override {
    if (delegate_ != nullptr) {
      delegate_->onPreCompileShader(id, type, defines);
    }
  }

  void onPostCompileShader(
    mln::shaders::BuiltIn id, mln::gfx::Backend::Type type,
    const std::string& defines
  ) override {
    if (delegate_ != nullptr) {
      delegate_->onPostCompileShader(id, type, defines);
    }
  }

  void onShaderCompileFailed(
    mln::shaders::BuiltIn id, mln::gfx::Backend::Type type,
    const std::string& defines
  ) override {
    if (delegate_ != nullptr) {
      delegate_->onShaderCompileFailed(id, type, defines);
    }
  }

  void onGlyphsLoaded(
    const mln::FontStack& stack, const mln::GlyphRange& range
  ) override {
    if (delegate_ != nullptr) {
      delegate_->onGlyphsLoaded(stack, range);
    }
  }

  void onGlyphsError(
    const mln::FontStack& stack, const mln::GlyphRange& range,
    std::exception_ptr error
  ) override {
    if (delegate_ != nullptr) {
      delegate_->onGlyphsError(stack, range, error);
    }
  }

  void onGlyphsRequested(
    const mln::FontStack& stack, const mln::GlyphRange& range
  ) override {
    if (delegate_ != nullptr) {
      delegate_->onGlyphsRequested(stack, range);
    }
  }

  void onTileAction(
    mln::TileOperation operation, const mln::OverscaledTileID& id,
    const std::string& source_id
  ) override {
    if (delegate_ != nullptr) {
      delegate_->onTileAction(operation, id, source_id);
    }
  }

  void onRenderError(std::exception_ptr error) override {
    if (delegate_ != nullptr) {
      delegate_->onRenderError(error);
    }
  }

 private:
  mln::RendererObserver* delegate_ = nullptr;
  bool needs_repaint_ = false;
  bool suppress_frame_callbacks_ = false;
};

struct RenderTextureState {
  std::unique_ptr<TextureSessionBackend> backend = nullptr;
  TextureSessionMode mode = TextureSessionMode::Owned;
  std::vector<RenderTextureSlot> slots;
};

struct RenderDriverWork {
  std::function<void()> execute;
  std::function<void()> abandon;
};

struct PendingFrameDemand {
  mln_frame_demand demand;
  std::chrono::steady_clock::time_point accepted_at;
  std::uint64_t barrier_epoch;
};

// An accepted barrier waiting for the demands that preceded it. Demands carry
// the barrier epoch current when they were accepted, so a barrier completes
// once no demand with a lower epoch is still outstanding.
struct PendingBarrier {
  std::shared_ptr<OperationObject> operation;
  std::uint64_t epoch;
};
}  // namespace mln::core

struct mln_render_session_object
    : public std::enable_shared_from_this<mln_render_session_object> {
  mln::core::RenderSessionKind kind = mln::core::RenderSessionKind::Surface;
  mln_render_session self = MLN_HANDLE_NULL;
  mln_map map = MLN_HANDLE_NULL;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t physical_width = 0;
  uint32_t physical_height = 0;
  uint64_t barrier_epoch = 0;
  double scale_factor = 1.0;

  mutable std::mutex control_mutex;
  mln_render_session_state state = MLN_RENDER_SESSION_STATE_ATTACHING;
  mln_render_session_capabilities capabilities{};
  uint64_t generation = 1;
  uint64_t map_update_generation = 0;
  uint64_t rendered_generation = 0;
  uint64_t rendered_target_generation = 0;
  uint64_t extent_generation = 1;
  uint64_t frame_generation = 0;
  uint64_t latest_demand_token = 0;
  mln_render_result latest_result = MLN_RENDER_RESULT_NO_UPDATE;
  bool target_ready = true;
  bool pending_changes = true;
  std::optional<mln_render_target_extent> pending_extent;
  std::optional<std::thread::id> graphics_thread;
  uint32_t acquired_frame_count = 0;
  bool driver_call_in_flight = false;
  bool stop_worker = false;
  bool attached = false;
  // Ticket of the newest accepted resize. An older ticket reaching the driver
  // was replaced and completes as superseded instead of waiting for an extent
  // the map will never publish.
  uint64_t resize_submission = 0;

  std::deque<mln_render_frame_result> frame_results;
  std::deque<mln::core::PendingFrameDemand> demands;
  // Barrier epochs of the demands currently running on the driver.
  std::vector<uint64_t> active_demand_epochs;
  std::deque<mln::core::PendingBarrier> barriers;
  std::deque<mln::core::RenderDriverWork> waiting_update_work;
  std::deque<mln::core::RenderDriverWork> driver_work;
  std::condition_variable worker_condition;
  mln::core::WorkerThread worker;
  // Backends with transfer-time thread attributes may replace the default
  // worker thread before attachment.
  std::function<mln_status(std::function<void()>)> start_worker;
  std::function<void()> join_worker;
  // Attachment descriptors are copied into this closure. It creates every
  // graphics object on the selected driver.
  std::function<mln_status(mln_render_session_object&)> initialize_backend;
  std::shared_ptr<mln::core::Wake> frame_wake;
  std::shared_ptr<mln::core::Wake> driver_wake;
  bool frame_wake_pending = false;
  bool driver_wake_pending = false;

  // Declared before `renderer` so reverse-order destruction tears the renderer
  // down while the scheduler its mailboxes point at is still alive.
  mln::core::RenderSessionScheduler scheduler;
  // Declared before `renderer`, which holds a raw pointer to it.
  mln::core::SessionFrameObserver frame_observer;
  std::unique_ptr<mln::Renderer> renderer = nullptr;
  std::unordered_set<std::string> rendered_source_ids;
  mln::core::FeatureStateSnapshot applied_feature_state;
  std::shared_ptr<const mln::core::FeatureStateSnapshot> pushed_feature_state;
  mln::core::RenderSurfaceState surface;
  mln::core::RenderTextureState texture;
};

struct mln_render_frame_batch_object {
  std::deque<mln_render_frame_result> results;
};

struct mln_acquired_frame_object {
  std::shared_ptr<mln_render_session_object> session;
  std::any backend_metadata;
  std::size_t slot = 0;
  mln_render_frame_result result{};
  mln_gpu_sync producer_sync{};
  std::atomic_bool valid{true};
};

namespace mln::core {
template <>
struct HandleTraits<mln_render_session_object> {
  static constexpr auto kind = HandleKind::RenderSession;
  static constexpr auto leasable = true;
};

template <>
struct HandleTraits<mln_render_frame_batch_object> {
  static constexpr auto kind = HandleKind::RenderFrameBatch;
  static constexpr auto leasable = true;
};

template <>
struct HandleTraits<mln_acquired_frame_object> {
  static constexpr auto kind = HandleKind::AcquiredFrame;
  static constexpr auto leasable = true;
};

using RenderDriverCallable =
  std::function<mln_status(mln_render_session_object&)>;
using RenderDriverResultCallable =
  std::function<mln_status(mln_render_session_object&, std::any&)>;

[[nodiscard]] auto lease_render_session(mln_render_session session)
  -> std::shared_ptr<mln_render_session_object>;
// Occupies the session's driver until *release is set, publishing *entered
// once it runs. Reachable from outside the library through the test hook the
// C ABI suite links; see src/c_api/test_hooks.hpp.
auto enqueue_blocking_test_render_operation(
  mln_render_session session, std::atomic_bool* entered,
  const std::atomic_bool* release, const mln_completion* completion
) -> mln_status;

auto enqueue_driver_operation(
  mln_render_session session, RenderDriverCallable work,
  const mln_completion* completion
) -> mln_status;
using RenderCompletionTransfer = std::function<
  void(const std::shared_ptr<Completion>&, mln_status, std::string, std::any)>;
auto enqueue_driver_result_operation(
  mln_render_session session, RenderDriverResultCallable work,
  const mln_completion* completion, RenderCompletionTransfer transfer
) -> mln_status;
auto validate_render_session_attach_request(
  const mln_render_session_attach_options* options,
  const mln_render_session* out_session, const mln_completion* completion
) -> mln_status;

auto start_attach_render_session(
  std::shared_ptr<mln_render_session_object> session, RenderSessionKind kind,
  const mln_render_session_attach_options* options,
  mln_render_session_capabilities capabilities, mln_render_session* out_session,
  const mln_completion* completion
) -> mln_status;
auto notify_render_session_map_update(
  mln_render_session_object* session
) noexcept -> void;

auto register_render_session(std::shared_ptr<mln_render_session_object> session)
  -> mln_render_session;
auto validate_render_session(
  mln_render_session session, mln_render_session_object*& out_session
) -> mln_status;
auto validate_live_attached_render_session(
  mln_render_session session, mln_render_session_object*& out_session
) -> mln_status;

inline auto physical_dimension(uint32_t logical, double scale_factor)
  -> uint32_t {
  return static_cast<uint32_t>(std::ceil(logical * scale_factor));
}

inline auto validate_render_target_extent(
  const mln_render_target_extent& extent, const char* dimension_message
) -> mln_status {
  if (extent.size < sizeof(mln_render_target_extent)) {
    set_thread_error("mln_render_target_extent.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    extent.width == 0 || extent.height == 0 ||
    !std::isfinite(extent.scale_factor) || extent.scale_factor <= 0.0
  ) {
    set_thread_error(dimension_message);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

inline auto validate_metal_context(
  const mln_metal_context_descriptor& context, bool require_device
) -> mln_status {
  if (context.size < sizeof(mln_metal_context_descriptor)) {
    set_thread_error("mln_metal_context_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (require_device && context.device == nullptr) {
    set_thread_error("Metal device must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

inline auto validate_vulkan_context(
  const mln_vulkan_context_descriptor& context, const char* null_handles_message
) -> mln_status {
  if (context.size < sizeof(mln_vulkan_context_descriptor)) {
    set_thread_error("mln_vulkan_context_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    context.instance == nullptr || context.physical_device == nullptr ||
    context.device == nullptr || context.graphics_queue == nullptr
  ) {
    set_thread_error(null_handles_message);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto render_target_extent_physical_size(
  const mln_render_target_extent* extent, uint32_t* out_width,
  uint32_t* out_height
) -> mln_status;

auto opengl_supported_context_provider_mask() noexcept -> uint32_t;
auto opengl_context_descriptor_default() noexcept
  -> mln_opengl_context_descriptor;
auto opengl_owned_texture_descriptor_default() noexcept
  -> mln_opengl_owned_texture_descriptor;
auto opengl_borrowed_texture_descriptor_default() noexcept
  -> mln_opengl_borrowed_texture_descriptor;
auto validate_opengl_context(
  const mln_opengl_context_descriptor& context, bool require_supported_provider
) -> mln_status;
auto validate_webgpu_context(const mln_webgpu_context_descriptor& context)
  -> mln_status;

// Whether two Vulkan context descriptors name the same device and queue.
auto vulkan_context_matches(
  const mln_vulkan_context_descriptor& lhs,
  const mln_vulkan_context_descriptor& rhs
) -> bool;

// How strictly two OpenGL context descriptors have to agree across a target
// replacement. A surface target carries its own drawable, so only the share
// group has to match; a texture target keeps making the session's context
// current against the handles it attached with, so those must be identical.
enum class OpenGLContextMatch : uint8_t { ShareGroup, Exact };

// Whether two OpenGL context descriptors name the same host context. The
// proc-address loader is excluded: it is a way to reach a context, not part of
// its identity.
auto opengl_context_matches(
  const mln_opengl_context_descriptor& lhs,
  const mln_opengl_context_descriptor& rhs, OpenGLContextMatch strictness
) -> bool;

inline auto set_session_extent(
  mln_render_session_object& session, const mln_render_target_extent& extent
) -> void {
  session.width = extent.width;
  session.height = extent.height;
  session.scale_factor = extent.scale_factor;
  session.physical_width =
    physical_dimension(extent.width, extent.scale_factor);
  session.physical_height =
    physical_dimension(extent.height, extent.scale_factor);
}

// Borrowed targets are sized by their owner, so the caller states the physical
// size instead of deriving it from the logical extent and scale factor.
inline auto set_borrowed_session_extent(
  mln_render_session_object& session, const mln_render_target_extent& extent,
  uint32_t physical_width, uint32_t physical_height
) -> void {
  session.width = extent.width;
  session.height = extent.height;
  session.scale_factor = extent.scale_factor;
  session.physical_width = physical_width;
  session.physical_height = physical_height;
}

inline auto validate_borrowed_physical_size(
  uint32_t physical_width, uint32_t physical_height
) -> mln_status {
  if (physical_width == 0 || physical_height == 0) {
    set_thread_error("physical texture dimensions must be positive");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

inline auto validate_physical_size(
  uint32_t width, uint32_t height, double scale_factor,
  const char* too_large_message
) -> mln_status {
  constexpr auto max_dimension =
    static_cast<double>(std::numeric_limits<uint32_t>::max());
  if (
    std::ceil(width * scale_factor) > max_dimension ||
    std::ceil(height * scale_factor) > max_dimension
  ) {
    set_thread_error(too_large_message);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

enum class RetargetTargetKind : uint8_t { Surface, BorrowedTexture };

// Checks the completion and session before a backend reads opaque target
// objects from the descriptor.
auto validate_render_session_retarget_submission(
  mln_render_session session, RetargetTargetKind kind,
  const mln_completion* completion
) -> mln_status;

// Rechecks on the driver thread that the session can take the replacement.
// Submission entry points perform the descriptor-safe check above first; this
// second check closes state-change races before the backend swaps resources.
auto validate_render_session_retarget(
  mln_render_session session, RetargetTargetKind kind,
  mln_render_session_object*& out_session
) -> mln_status;

// Hands a validated descriptor to the session's backend.
using RenderTargetReplacer =
  std::function<mln_status(mln_render_session_object&)>;

// Shared body behind every set-target entry point. Checks that the session is
// live, attached, owned by this thread, and of the kind the descriptor targets,
// then calls `replace`. On success the session takes the new extent and the
// renderer is kept unless the scale factor changed. The per-backend entry point
// validates its own descriptor first.
auto render_session_set_target(
  mln_render_session session, RetargetTargetKind kind,
  const mln_render_target_extent& extent, uint32_t physical_width,
  uint32_t physical_height, const RenderTargetReplacer& replace
) -> mln_status;

// render_session_set_target() for a surface, whose physical size follows from
// its logical extent rather than being stated by the caller.
auto surface_session_set_target(
  mln_render_session session, const mln_render_target_extent& extent,
  const RenderTargetReplacer& replace
) -> mln_status;
auto render_session_destroy(mln_render_session session) -> mln_status;
auto queried_feature_list_count(
  mln_queried_feature_list list, size_t* out_count
) -> mln_status;
auto queried_feature_list_get(
  mln_queried_feature_list list, size_t index, mln_queried_feature* out_feature
) -> mln_status;
auto queried_feature_list_destroy(mln_queried_feature_list list) -> void;
auto render_session_query_rendered_features(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options,
  mln_queried_feature_list* out_result
) -> mln_status;
auto render_session_query_source_features(
  mln_render_session session, mln_buffer_view source_id,
  const mln_source_feature_query_options* options,
  mln_queried_feature_list* out_result
) -> mln_status;
auto render_session_query_feature_extensions(
  mln_render_session session, mln_buffer_view source_id,
  mln_buffer_view feature, mln_buffer_view extension,
  mln_buffer_view extension_field, const mln_buffer_view* arguments,
  mln_buffer* out_result
) -> mln_status;

auto render_session_query_rendered_features_start(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options,
  const mln_completion* completion
) -> mln_status;
auto render_session_query_source_features_start(
  mln_render_session session, mln_buffer_view source_id,
  const mln_source_feature_query_options* options,
  const mln_completion* completion
) -> mln_status;
auto render_session_query_feature_extensions_start(
  mln_render_session session, mln_buffer_view source_id,
  mln_buffer_view feature, mln_buffer_view extension,
  mln_buffer_view extension_field, const mln_buffer_view* arguments,
  const mln_completion* completion
) -> mln_status;

auto render_session_get_capabilities(
  mln_render_session session, mln_render_session_capabilities* out_capabilities
) -> mln_status;
auto render_session_get_snapshot(
  mln_render_session session, mln_render_session_snapshot* out_snapshot
) -> mln_status;
auto render_session_request_frame(
  mln_render_session session, const mln_frame_demand* demand
) -> mln_status;
auto render_session_service_driver_work(
  mln_render_session session, std::size_t max_work, std::size_t* out_serviced
) -> mln_status;
auto render_session_drain_frame_results(
  mln_render_session session, mln_render_frame_batch* out_batch
) -> mln_status;
auto render_frame_batch_count(
  mln_render_frame_batch batch, std::size_t* out_count
) -> mln_status;
auto render_frame_batch_get(
  mln_render_frame_batch batch, std::size_t index,
  mln_render_frame_result* out_result
) -> mln_status;
auto render_frame_batch_release(mln_render_frame_batch batch) noexcept -> void;
auto render_session_acquire_frame(
  mln_render_session session, mln_acquired_frame* out_frame
) -> mln_status;
auto acquired_frame_get_result(
  mln_acquired_frame frame, mln_render_frame_result* out_result
) -> mln_status;
auto acquired_frame_get_producer_sync(
  mln_acquired_frame frame, mln_gpu_sync* out_sync
) -> mln_status;
auto acquired_frame_release(
  mln_acquired_frame* frame, const mln_gpu_sync* consumer_completion
) -> mln_status;
auto render_session_resize_start(
  mln_render_session session, const mln_render_target_extent* extent,
  const mln_completion* completion
) -> mln_status;
auto render_session_barrier_start(
  mln_render_session session, const mln_completion* completion
) -> mln_status;
auto render_session_maintenance_start(
  mln_render_session session, RenderSessionMaintenance maintenance,
  const mln_completion* completion
) -> mln_status;
auto render_session_detach_start(
  mln_render_session session, const mln_completion* completion
) -> mln_status;
auto render_session_abandon(
  mln_render_session session, mln_render_abandon_result* out_result
) -> mln_status;

// Leases an acquired frame and checks that its session can still describe it.
// A session that lost or abandoned its target no longer owns the texture the
// frame names, so both states report MLN_STATUS_TARGET_LOST.
auto lease_valid_acquired_frame(
  mln_acquired_frame frame,
  std::shared_ptr<mln_acquired_frame_object>& out_frame
) -> mln_status;

// Slot count a session-owned texture ring is granted. Attachment rejects null
// options; the clamp keeps a ring sane for the backends that size themselves
// before reaching that check.
inline auto attach_ring_depth(const mln_render_session_attach_options* options)
  -> uint32_t {
  constexpr auto max_ring_depth = 3U;
  return std::clamp(
    options == nullptr ? 1U : options->requested_texture_ring_depth, 1U,
    max_ring_depth
  );
}

// Rejects an attachment whose requested driver is not the one this target can
// run on.
inline auto require_render_driver(
  const mln_render_session_attach_options* options,
  mln_render_driver_kind expected, const char* message
) -> mln_status {
  if (options != nullptr && options->driver != expected) {
    set_thread_error(message);
    return MLN_STATUS_UNSUPPORTED;
  }
  return MLN_STATUS_OK;
}
}  // namespace mln::core
