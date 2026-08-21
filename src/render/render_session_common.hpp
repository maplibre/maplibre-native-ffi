#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <mbgl/actor/scheduler.hpp>
#include <mbgl/gfx/headless_backend.hpp>
#include <mbgl/gfx/renderer_backend.hpp>
#include <mbgl/renderer/renderer.hpp>
#include <mbgl/renderer/renderer_observer.hpp>
#include <mbgl/util/feature.hpp>
#include <mbgl/util/size.hpp>

#include "diagnostics/diagnostics.hpp"
#include "map/feature_state.hpp"
#include "maplibre_native_c.h"

struct mln_render_session_object;

namespace mln {
class UpdateParameters;
}

namespace mln::core {

// Backend swap() skips host present while an internal render is in progress.
inline thread_local bool discard_renderable_present = false;

enum class RenderSessionKind : uint8_t { Surface, Texture };
enum class TextureSessionApi : uint8_t {
  Generic,
  Metal,
  OpenGL,
  Vulkan,
  WebGPU
};
enum class TextureSessionFrameKind : uint8_t {
  None,
  MetalOwned,
  OpenGLOwned,
  VulkanOwned,
  WebGPUOwned
};
enum class TextureSessionMode : uint8_t { Owned, Borrowed };

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
  virtual auto acquire_vulkan_owned_frame(
    const mln_render_session_object& session,
    mln_vulkan_owned_texture_frame& out_frame
  ) -> mln_status {
    (void)session;
    (void)out_frame;
    return MLN_STATUS_UNSUPPORTED;
  }
  virtual auto acquire_opengl_owned_frame(
    const mln_render_session_object& session,
    mln_opengl_owned_texture_frame& out_frame
  ) -> mln_status {
    (void)session;
    (void)out_frame;
    return MLN_STATUS_UNSUPPORTED;
  }
  virtual auto acquire_webgpu_owned_frame(
    const mln_render_session_object& session,
    mln_webgpu_owned_texture_frame& out_frame
  ) -> mln_status {
    (void)session;
    (void)out_frame;
    return MLN_STATUS_UNSUPPORTED;
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
  // Only the owner thread may run this queue, so a caller on any other thread
  // gets a no-op rather than tasks running off the owner thread.
  void waitForEmpty(
    const mln::util::SimpleIdentity = mln::util::SimpleIdentity::Empty
  ) override {
    if (mln::Scheduler::GetCurrent(/*init=*/false) == this) {
      drain();
    }
  }

  // Runs queued work on the calling thread, which must be the session's owner
  // thread. Loops until the queue is empty, because a task may enqueue more.
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
// session call, but only when it does not already have one. A session sharing
// its owner thread with the runtime keeps the runtime's run loop, which the
// host pumps more often than it renders.
//
// GetCurrent(false) is required: the default would create a thread_local
// RunLoop on a bare render thread, which permanently disqualifies that thread
// from mln_runtime_create().
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

// Records the frame status that mln::Renderer reports synchronously out of
// render(), so render_session_render_update() can return its repaint flag,
// and forwards every callback to the map's observer so runtime event delivery
// is unchanged. Runs entirely on the session owner thread.
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
  uint64_t next_frame_id = 1;
  uint64_t acquired_frame_id = 0;
  bool acquired = false;
  TextureSessionFrameKind acquired_frame_kind = TextureSessionFrameKind::None;
  TextureSessionApi api_kind = TextureSessionApi::Generic;
  TextureSessionMode mode = TextureSessionMode::Owned;
  void* rendered_native_texture = nullptr;
  void* acquired_native_texture = nullptr;
};

}  // namespace mln::core

struct mln_render_session_object {
  mln::core::RenderSessionKind kind = mln::core::RenderSessionKind::Surface;
  mln_map map = MLN_HANDLE_NULL;
  // The thread that attached the session, fixed for its lifetime. Set before
  // the session is registered.
  std::thread::id owner_thread;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t physical_width = 0;
  uint32_t physical_height = 0;
  double scale_factor = 1.0;
  uint64_t generation = 1;
  uint64_t rendered_generation = 0;
  bool attached = true;

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

namespace mln::core {

struct RenderSessionAttachMessages {
  const char* null_session;
  const char* null_output;
  const char* non_null_output;
};

auto register_render_session(std::shared_ptr<mln_render_session_object> session)
  -> mln_render_session;
auto validate_render_session(
  mln_render_session session, mln_render_session_object*& out_session
) -> mln_status;
auto validate_live_attached_render_session(
  mln_render_session session, mln_render_session_object*& out_session
) -> mln_status;
auto erase_render_session(mln_render_session session)
  -> std::shared_ptr<mln_render_session_object>;

inline auto validate_attach_output(
  mln_render_session* out_session, const char* null_message,
  const char* not_null_message
) -> mln_status {
  if (out_session == nullptr) {
    set_thread_error(null_message);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (*out_session != MLN_HANDLE_NULL) {
    set_thread_error(not_null_message);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

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

auto attach_render_session(
  std::shared_ptr<mln_render_session_object> session,
  mln_render_session* out_session, RenderSessionKind kind,
  RenderSessionAttachMessages messages
) -> mln_status;

auto render_session_resize(
  mln_render_session session, uint32_t width, uint32_t height,
  double scale_factor
) -> mln_status;

enum class RetargetTargetKind : uint8_t { Surface, BorrowedTexture };

// Checks that a session can take a replacement target of this kind, before any
// descriptor is read. Entry points call this first, including in builds without
// that backend, so the reported status does not depend on which failure the
// build notices first.
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
auto render_session_render_update(
  mln_render_session session, mln_render_result* out_result,
  bool* out_needs_repaint
) -> mln_status;
auto render_session_detach(mln_render_session session) -> mln_status;
auto render_session_destroy(mln_render_session session) -> mln_status;
auto render_session_reduce_memory_use(mln_render_session session) -> mln_status;
auto render_session_clear_data(mln_render_session session) -> mln_status;
auto render_session_dump_debug_logs(mln_render_session session) -> mln_status;
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

}  // namespace mln::core
