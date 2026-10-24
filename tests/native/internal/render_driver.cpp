// A render session's driver while it is busy: queued demands, waits for map
// updates, barriers, and abandon.

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <mln/gfx/context.hpp>

#include "diagnostics/diagnostics.hpp"
#include "internal/support/checks.hpp"
#include "internal/support/driver_blocker.hpp"
#include "internal/support/sync_points.hpp"
#include "maplibre_native_c.h"
#include "support/frames.h"
#include "support/test_support.h"

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
#include "support/host_graphics.h"
#endif

namespace {

using mln::native_tests::await;
using mln::native_tests::DriverBlocker;
using mln::native_tests::SyncPoint;
using mln::native_tests::SyncPointScope;

struct Fixture {
  mln_runtime runtime = MLN_HANDLE_NULL;
  mln_map map = MLN_HANDLE_NULL;
  mln_test_render_fixture render{};
};

// A map with a loaded style, attached to the preset's render fixture. The
// render fixture's wakes point into it, so it is filled in place.
void create_fixture(Fixture& fixture) {
  fixture.runtime = mln_test_create_runtime();
  fixture.map = mln_test_create_map(fixture.runtime);
  mln_test_render_prepare_map(fixture.map);
  TEST_ASSERT_TRUE(
    mln_test_render_fixture_create(fixture.map, &fixture.render)
  );
}

void destroy_fixture(Fixture& fixture) {
  mln_test_render_fixture_destroy(&fixture.render);
  mln_test_destroy_map(fixture.map);
  mln_test_destroy_runtime(fixture.runtime);
}

// Holds a core worker inside the blocker. A caller driver runs nothing until
// the case services it, which holds its queue just the same.
void block_driver(const Fixture& fixture, DriverBlocker& blocker) {
  MLN_TEST_OK(blocker.submit(fixture.render.session));
  if (fixture.render.driver == MLN_RENDER_DRIVER_CORE_WORKER) {
    TEST_ASSERT_TRUE(mln_test_gate_wait_entered(blocker.gate.get()));
  }
}

void a_resize_queued_before_detach_completes_as_failed() {
  auto fixture = Fixture{};
  create_fixture(fixture);
  auto blocker = DriverBlocker{};
  block_driver(fixture, blocker);
  auto resize = mln_test_completion_default(0);
  const auto extent = mln_logical_extent{32, 48, 1.0};
  MLN_TEST_OK(mln_render_session_resize(
    fixture.render.session, extent, &resize.descriptor, nullptr
  ));
  auto detach = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_detach(
    fixture.render.session, &detach.descriptor, nullptr
  ));
  MLN_TEST_OK(blocker.finish(fixture.render));
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_test_render_fixture_finish_operation(&fixture.render, &resize)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_FAILED, mln_test_completion_disposition(&resize)
  );
  mln_test_completion_destroy(&resize);
  MLN_TEST_OK(
    mln_test_render_fixture_finish_operation(&fixture.render, &detach)
  );
  mln_test_completion_destroy(&detach);
  destroy_fixture(fixture);
}

auto wait_for_results(const Fixture& fixture) -> mln_render_frame_batch {
  return mln_test_render_wait_for_results(&fixture.render);
}

// The first `count` results, however many drains they arrive over.
template <std::size_t count>
auto wait_for_result_list(const Fixture& fixture)
  -> std::array<mln_render_frame_result, count> {
  auto results = std::array<mln_render_frame_result, count>{};
  mln_test_render_wait_for_result_list(&fixture.render, count, results.data());
  return results;
}

auto batch_result(mln_render_frame_batch batch, std::size_t index)
  -> mln_render_frame_result {
  return mln_test_render_batch_result(batch, index);
}

// Services a caller driver on the case's thread until another thread sets
// `done`, returning to the event loop between services, where a WebGPU frame's
// completion arrives. A held driver call runs inside one of these services.
void service_until(const Fixture& fixture, std::atomic_bool& done) {
  MLN_TEST_OK(mln_test_render_step_until(
    &fixture.render,
    [](void* flag) { return static_cast<std::atomic_bool*>(flag)->load(); },
    &done, mln_test_deadline_default(), "the thread that holds the driver"
  ));
}

auto request_frame(const Fixture& fixture, const mln_frame_demand& demand)
  -> mln_status {
  return mln_render_session_request_frame(
    fixture.render.session, &demand, nullptr
  );
}

enum class FrameFailure {
  Activation,
  Slot,
  Resources,
  Scheduler,
  Metadata,
  Drawable,
  Release
};

class FailedActivation final : public mln::gfx::RendererBackend {
 public:
  FailedActivation() : RendererBackend(mln::gfx::ContextMode::Unique) {}
  auto getDefaultRenderable() -> mln::gfx::Renderable& override {
    throw std::runtime_error("unexpected render after failed activation");
  }
  void initShaders(
    mln::gfx::ShaderRegistry&, const mln::ProgramParameters&
  ) override {}

 protected:
  auto createContext() -> std::unique_ptr<mln::gfx::Context> override {
    throw std::runtime_error("unexpected context after failed activation");
  }
  void activate() override { throw std::runtime_error("activation failed"); }
  void deactivate() override {}
};

class FailingFrameBackend final : public mln::core::TextureSessionBackend {
 public:
  FailingFrameBackend(
    std::unique_ptr<TextureSessionBackend> backend, FrameFailure failure,
    mln::core::RenderSessionScheduler& scheduler
  )
      : backend_(std::move(backend)),
        failure_(failure),
        scheduler_(scheduler) {}

  auto headless_backend() -> mln::gfx::HeadlessBackend& override {
    return backend_->headless_backend();
  }
  auto renderer_backend() -> mln::gfx::RendererBackend* override {
    return failure_ == FrameFailure::Activation ? &failed_activation_
                                                : backend_->renderer_backend();
  }
  auto select_render_slot(std::size_t slot) -> mln_status override {
    if (failure_ == FrameFailure::Slot) {
      mln::core::set_thread_error("render slot failed");
      return MLN_STATUS_NATIVE_ERROR;
    }
    if (failure_ == FrameFailure::Drawable && !retried_) {
      retried_ = true;
      throw mln::core::RenderTargetNotReady("drawable unavailable");
    }
    return backend_->select_render_slot(slot);
  }
  void prepare_render_resources() override {
    if (failure_ == FrameFailure::Resources)
      throw std::runtime_error("resource preparation failed");
    if (failure_ == FrameFailure::Scheduler)
      scheduler_.schedule([] { throw std::runtime_error("scheduler failed"); });
    backend_->prepare_render_resources();
  }
  auto after_render(mln_render_session_object& session, bool& rendered)
    -> mln_status override {
    return backend_->after_render(session, rendered);
  }
  auto record_frame_metadata(
    const mln::core::RenderFrameMetadata& metadata, std::any& result
  ) -> mln_status override {
    if (failure_ == FrameFailure::Metadata)
      throw std::runtime_error("frame metadata failed");
    return backend_->record_frame_metadata(metadata, result);
  }
  auto allows_off_thread_teardown() const noexcept -> bool override {
    return backend_->allows_off_thread_teardown();
  }
  auto prepare_off_thread_teardown() noexcept -> bool override {
    return backend_->prepare_off_thread_teardown();
  }
  void quarantine() noexcept override { backend_->quarantine(); }
  // Takes any synchronization, then fails to wait for it.
  auto supports_consumer_sync(mln_gpu_sync_kind kind) -> bool override {
    return failure_ == FrameFailure::Release ||
           backend_->supports_consumer_sync(kind);
  }
  auto release_consumer_sync(const mln_gpu_sync& sync) -> mln_status override {
    if (failure_ == FrameFailure::Release) return MLN_STATUS_NATIVE_ERROR;
    return backend_->release_consumer_sync(sync);
  }

 private:
  std::unique_ptr<TextureSessionBackend> backend_;
  FrameFailure failure_;
  mln::core::RenderSessionScheduler& scheduler_;
  FailedActivation failed_activation_;
  bool retried_ = false;
};

void install_frame_failure(const Fixture& fixture, FrameFailure failure) {
  auto install = mln_test_completion_default(0);
  MLN_TEST_OK(
    mln::core::enqueue_driver_operation(
      fixture.render.session,
      [failure](mln_render_session_object& session) {
        session.texture.backend = std::make_unique<FailingFrameBackend>(
          std::move(session.texture.backend), failure, session.scheduler
        );
        return MLN_STATUS_OK;
      },
      &install.descriptor,
      mln::core::valueless_completion<&mln_render_session_reduce_memory_use>()
    )
  );
  MLN_TEST_OK(
    mln_test_render_fixture_finish_operation(&fixture.render, &install)
  );
  mln_test_completion_destroy(&install);
}

struct LostTargetCase {
  FrameFailure failure;
  const char* message;
};

void graphics_failures_finish_demands_and_lose_the_target() {
  for (const auto [failure, message] : {
         LostTargetCase{FrameFailure::Activation, "activation failed"},
         LostTargetCase{FrameFailure::Slot, "render slot failed"},
         LostTargetCase{FrameFailure::Resources, "resource preparation failed"},
         LostTargetCase{FrameFailure::Scheduler, "scheduler failed"},
         LostTargetCase{FrameFailure::Metadata, "frame metadata failed"},
       }) {
    auto fixture = Fixture{};
    create_fixture(fixture);
    static_cast<void>(mln_test_render_until_idle(
      fixture.runtime, fixture.map, &fixture.render, nullptr
    ));
    install_frame_failure(fixture, failure);
    auto blocker = DriverBlocker{};
    block_driver(fixture, blocker);
    mln_test_render_request_forced(&fixture.render, 9100);
    mln_test_render_request_forced(&fixture.render, 9101);
    auto fence = mln_test_completion_default(0);
    MLN_TEST_OK(mln_render_session_dump_debug_logs(
      fixture.render.session, &fence.descriptor, nullptr
    ));
    MLN_TEST_OK(blocker.finish(fixture.render));
    auto results = std::vector<mln_render_frame_result>{};
    while (results.size() < 2) {
      const auto batch = wait_for_results(fixture);
      const auto view = mln_test_render_batch_view(batch);
      for (auto index = std::size_t{0}; index < view.result_count; ++index)
        results.push_back(batch_result(batch, index));
      mln_render_frame_batch_release(batch);
    }
    const auto first = results[0];
    const auto second = results[1];
    TEST_ASSERT_TRUE(mln_test_completion_wait(&fence, -1));
    const auto fence_status = mln_test_completion_status(&fence);
    mln_test_completion_destroy(&fence);
    auto snapshot =
      mln_render_session_snapshot{.size = sizeof(mln_render_session_snapshot)};
    MLN_TEST_OK(mln_render_session_get_snapshot(
      fixture.render.session, &snapshot, nullptr
    ));
    auto error = mln_runtime_event{};
    auto error_message = std::array<char, 256>{};
    const auto reported = mln_test_await_event(
      fixture.runtime, MLN_RUNTIME_EVENT_MAP_RENDER_ERROR, fixture.map, &error,
      error_message.data(), error_message.size()
    );
    auto abandoned =
      mln_render_abandon_result{.size = sizeof(mln_render_abandon_result)};
    MLN_TEST_OK(
      mln_render_session_abandon(fixture.render.session, &abandoned, nullptr)
    );
    destroy_fixture(fixture);
    TEST_ASSERT_TRUE(reported);
    TEST_ASSERT_NOT_NULL(std::strstr(error_message.data(), message));
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_RESULT_TARGET_NOT_READY, first.disposition
    );
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_RESULT_TARGET_NOT_READY, second.disposition
    );
    TEST_ASSERT_EQUAL_UINT64(0, first.frame_generation);
    TEST_ASSERT_EQUAL_UINT64(0, second.frame_generation);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_SESSION_STATE_TARGET_LOST, snapshot.state
    );
    MLN_TEST_STATUS(MLN_STATUS_TARGET_LOST, fence_status);
  }
}

void an_unavailable_drawable_releases_its_slot_for_a_retry() {
  auto fixture = Fixture{};
  create_fixture(fixture);
  static_cast<void>(mln_test_render_until_idle(
    fixture.runtime, fixture.map, &fixture.render, nullptr
  ));
  install_frame_failure(fixture, FrameFailure::Drawable);
  mln_test_render_request_forced(&fixture.render, 9102);
  auto batch = wait_for_results(fixture);
  const auto unavailable = batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  auto snapshot =
    mln_render_session_snapshot{.size = sizeof(mln_render_session_snapshot)};
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture.render.session, &snapshot, nullptr)
  );
  auto demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_IF_NEEDED;
  demand.token = 9103;
  MLN_TEST_OK(request_frame(fixture, demand));
  batch = wait_for_results(fixture);
  const auto retried = batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  destroy_fixture(fixture);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_TARGET_NOT_READY, unavailable.disposition
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_ATTACHED, snapshot.state);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, retried.disposition);
}

void an_idle_demand_reports_a_failed_backend_activation() {
  auto fixture = Fixture{};
  create_fixture(fixture);
  static_cast<void>(mln_test_render_until_idle(
    fixture.runtime, fixture.map, &fixture.render, nullptr
  ));
  install_frame_failure(fixture, FrameFailure::Activation);
  auto demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_IF_NEEDED | MLN_FRAME_DEMAND_WAIT_FOR_UPDATE;
  demand.token = 9104;
  MLN_TEST_OK(request_frame(fixture, demand));
  const auto batch = wait_for_results(fixture);
  const auto result = batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  auto snapshot =
    mln_render_session_snapshot{.size = sizeof(mln_render_session_snapshot)};
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture.render.session, &snapshot, nullptr)
  );
  auto abandoned =
    mln_render_abandon_result{.size = sizeof(mln_render_abandon_result)};
  MLN_TEST_OK(
    mln_render_session_abandon(fixture.render.session, &abandoned, nullptr)
  );
  destroy_fixture(fixture);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_TARGET_NOT_READY, result.disposition
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_SESSION_STATE_TARGET_LOST, snapshot.state
  );
}

void a_same_size_resize_redraws_after_an_earlier_queued_frame() {
  auto fixture = Fixture{};
  create_fixture(fixture);
  static_cast<void>(mln_test_render_until_idle(
    fixture.runtime, fixture.map, &fixture.render, nullptr
  ));
  auto blocker = DriverBlocker{};
  block_driver(fixture, blocker);
  mln_test_render_request_forced(&fixture.render, 9002);
  auto resize = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_resize(
    fixture.render.session, mln_logical_extent{64, 64, 1.0}, &resize.descriptor,
    nullptr
  ));
  MLN_TEST_OK(mln_test_map_fence(fixture.map));
  MLN_TEST_OK(blocker.finish(fixture.render));
  MLN_TEST_OK(
    mln_test_render_fixture_finish_operation(&fixture.render, &resize)
  );
  mln_test_completion_destroy(&resize);
  mln_render_frame_batch_release(wait_for_results(fixture));
  auto demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_IF_NEEDED;
  demand.token = 9003;
  MLN_TEST_OK(request_frame(fixture, demand));
  const auto batch = wait_for_results(fixture);
  const auto result = batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  destroy_fixture(fixture);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
}

struct OrderedBarrier {
  SyncPointScope* points;
  mln_render_session session;
  std::atomic_bool maintenance_done{false};
  std::atomic_bool barrier_done{false};
  bool demand_held = false;
  bool maintenance_first = false;
  mln_status maintenance_submission = MLN_STATUS_NATIVE_ERROR;
  mln_status barrier_submission = MLN_STATUS_NATIVE_ERROR;
  mln_status maintenance_status = MLN_STATUS_NATIVE_ERROR;
  mln_status barrier_status = MLN_STATUS_NATIVE_ERROR;
};

void record_maintenance(void* context, const mln_completion_result* result) {
  auto& order = *static_cast<OrderedBarrier*>(context);
  order.maintenance_status = static_cast<mln_status>(result->status);
  order.maintenance_done.store(true);
}

void record_barrier(void* context, const mln_completion_result* result) {
  auto& order = *static_cast<OrderedBarrier*>(context);
  order.maintenance_first = order.maintenance_done.load();
  order.barrier_status = static_cast<mln_status>(result->status);
  order.barrier_done.store(true);
  mln_test_pulse();
}

void submit_behind_the_held_demand(void* context) {
  auto& order = *static_cast<OrderedBarrier*>(context);
  order.demand_held =
    order.points->wait_for_hits(SyncPoint::RenderFrameResultPublished, 1);
  const auto maintenance = mln_completion{
    .size = sizeof(mln_completion),
    .callback = record_maintenance,
    .user_data = &order,
    .release_user_data = nullptr,
  };
  const auto barrier = mln_completion{
    .size = sizeof(mln_completion),
    .callback = record_barrier,
    .user_data = &order,
    .release_user_data = nullptr,
  };
  order.maintenance_submission =
    mln_render_session_reduce_memory_use(order.session, &maintenance, nullptr);
  order.barrier_submission =
    mln_render_session_barrier(order.session, &barrier, nullptr);
  order.points->release(SyncPoint::RenderFrameResultPublished);
}

// Publication precedes the demand's terminal result. Holding that interval
// lets maintenance and a barrier queue before the demand finishes; no public
// signal holds the driver there.
void a_barrier_waits_for_an_ordered_operation_after_a_running_demand() {
  auto points = SyncPointScope{};
  auto fixture = Fixture{};
  create_fixture(fixture);
  auto order =
    OrderedBarrier{.points = &points, .session = fixture.render.session};
  points.hold(SyncPoint::RenderFrameResultPublished);
  auto* submitting =
    mln_test_thread_start(submit_behind_the_held_demand, &order);
  mln_test_render_request_forced(&fixture.render, 160);
  MLN_TEST_OK(mln_test_render_step_until(
    &fixture.render,
    [](void* context) {
      return static_cast<OrderedBarrier*>(context)->barrier_done.load();
    },
    &order, mln_test_deadline_default(), "the ordered barrier to complete"
  ));
  mln_test_thread_join(submitting);
  TEST_ASSERT_TRUE(order.demand_held);
  MLN_TEST_OK(order.maintenance_submission);
  MLN_TEST_OK(order.barrier_submission);
  const auto batch = wait_for_results(fixture);
  TEST_ASSERT_EQUAL_UINT64(160, batch_result(batch, 0).token);
  mln_render_frame_batch_release(batch);
  // A later barrier also fences the maintenance callback before teardown.
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture.render,
    mln_render_session_barrier(
      fixture.render.session, &completion.descriptor, nullptr
    )
  );
  MLN_TEST_OK(order.maintenance_status);
  MLN_TEST_OK(order.barrier_status);
  const auto maintenance_first = order.maintenance_first;
  destroy_fixture(fixture);
  TEST_ASSERT_TRUE(maintenance_first);
}

// A newer if-needed demand supersedes the earlier one. Forced demands render
// independently while preserving the target and map generations.
void if_needed_demands_supersede_and_forced_demands_render() {
  auto fixture = Fixture{};
  create_fixture(fixture);
  auto blocker = DriverBlocker{};
  block_driver(fixture, blocker);
  auto first = mln_frame_demand_default();
  first.token = 101;
  auto forced = first;
  forced.flags = 0;
  forced.token = 102;
  auto newest = first;
  newest.token = 103;
  auto another_forced = forced;
  another_forced.token = 104;
  MLN_TEST_OK(request_frame(fixture, first));
  MLN_TEST_OK(request_frame(fixture, forced));
  MLN_TEST_OK(request_frame(fixture, newest));
  MLN_TEST_OK(request_frame(fixture, another_forced));
  MLN_TEST_OK(blocker.finish(fixture.render));
  const auto [superseded, rendered, if_needed, repeated] =
    wait_for_result_list<4>(fixture);
  TEST_ASSERT_EQUAL_UINT64(101, superseded.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_SUPERSEDED, superseded.disposition
  );
  TEST_ASSERT_EQUAL_UINT64(102, rendered.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, rendered.disposition);
  TEST_ASSERT_EQUAL_UINT64(103, if_needed.token);
  TEST_ASSERT_NOT_EQUAL_UINT32(
    MLN_RENDER_RESULT_SUPERSEDED, if_needed.disposition
  );
  TEST_ASSERT_EQUAL_UINT64(104, repeated.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, repeated.disposition);
  TEST_ASSERT_GREATER_THAN_UINT64(0, rendered.generation);
  TEST_ASSERT_EQUAL_UINT64(rendered.generation, repeated.generation);
  TEST_ASSERT_GREATER_THAN_UINT64(
    rendered.frame_generation, repeated.frame_generation
  );
  destroy_fixture(fixture);
}

// Superseding a demand before a resize leaves the later forced demand ordered
// after that resize, even when the map has already published the new extent.
void supersession_preserves_a_forced_demand_after_a_resize() {
  auto fixture = Fixture{};
  create_fixture(fixture);
  static_cast<void>(mln_test_render_until_idle(
    fixture.runtime, fixture.map, &fixture.render, nullptr
  ));
  auto before = mln_render_session_snapshot{};
  before.size = sizeof(before);
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture.render.session, &before, nullptr)
  );
  auto blocker = DriverBlocker{};
  block_driver(fixture, blocker);
  auto demand = mln_frame_demand_default();
  demand.token = 101;
  MLN_TEST_OK(request_frame(fixture, demand));
  auto resize = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_resize(
    fixture.render.session, mln_logical_extent{32, 48, 1.0}, &resize.descriptor,
    nullptr
  ));
  mln_test_render_request_forced(&fixture.render, 102);
  demand.token = 103;
  MLN_TEST_OK(request_frame(fixture, demand));
  MLN_TEST_OK(mln_test_map_fence(fixture.map));
  MLN_TEST_OK(blocker.finish(fixture.render));
  MLN_TEST_OK(
    mln_test_render_fixture_finish_operation(&fixture.render, &resize)
  );
  mln_test_completion_destroy(&resize);
  const auto [superseded, forced, later] = wait_for_result_list<3>(fixture);
  TEST_ASSERT_EQUAL_UINT64(101, superseded.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_SUPERSEDED, superseded.disposition
  );
  TEST_ASSERT_EQUAL_UINT64(102, forced.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, forced.disposition);
  TEST_ASSERT_EQUAL_UINT64(before.generation + 1, forced.generation);
  TEST_ASSERT_EQUAL_UINT64(103, later.token);
  destroy_fixture(fixture);
}

// Scheduler service may drain results, but cannot take a demand submitted
// after the work that changes its target extent.
void scheduler_service_preserves_a_forced_demand_after_a_resize() {
  auto fixture = Fixture{};
  create_fixture(fixture);
  static_cast<void>(mln_test_render_until_idle(
    fixture.runtime, fixture.map, &fixture.render, nullptr
  ));
  auto blocker = DriverBlocker{};
  block_driver(fixture, blocker);
  const auto live = mln::core::lease_render_session(fixture.render.session);
  live->scheduler.schedule([] {});
  auto resize = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_resize(
    fixture.render.session, mln_logical_extent{32, 48, 1.0}, &resize.descriptor,
    nullptr
  ));
  mln_test_render_request_forced(&fixture.render, 201);
  MLN_TEST_OK(mln_test_map_fence(fixture.map));
  MLN_TEST_OK(blocker.finish(fixture.render));
  MLN_TEST_OK(
    mln_test_render_fixture_finish_operation(&fixture.render, &resize)
  );
  mln_test_completion_destroy(&resize);
  const auto batch = wait_for_results(fixture);
  TEST_ASSERT_EQUAL_UINT64(201, batch_result(batch, 0).token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, batch_result(batch, 0).disposition
  );
  mln_render_frame_batch_release(batch);
  destroy_fixture(fixture);
}

struct ConcurrentResize {
  mln_render_session session;
  mln_logical_extent extent;
  mln_test_completion* completion;
  mln_status status = MLN_STATUS_NATIVE_ERROR;
};

void submit_concurrent_resize(void* context) {
  auto& submission = *static_cast<ConcurrentResize*>(context);
  submission.status = mln_render_session_resize(
    submission.session, submission.extent, &submission.completion->descriptor,
    nullptr
  );
}

void concurrent_resizes_publish_matching_map_and_driver_extents() {
  auto points = SyncPointScope{};
  auto fixture = Fixture{};
  create_fixture(fixture);
  auto first = mln_test_completion_default(0);
  auto second = mln_test_completion_default(0);
  auto a = ConcurrentResize{fixture.render.session, {32, 48, 1.0}, &first};
  auto b = ConcurrentResize{fixture.render.session, {48, 32, 1.0}, &second};
  points.hold(SyncPoint::RenderExtentPublished);
  auto* submitting_a = mln_test_thread_start(submit_concurrent_resize, &a);
  TEST_ASSERT_TRUE(points.wait_for_hits(SyncPoint::RenderExtentPublished, 1));
  auto* submitting_b = mln_test_thread_start(submit_concurrent_resize, &b);
  TEST_ASSERT_TRUE(
    points.wait_for_hits(SyncPoint::RenderExtentSubmissionStarted, 2)
  );
  points.release(SyncPoint::RenderExtentPublished);
  mln_test_thread_join(submitting_a);
  mln_test_thread_join(submitting_b);
  MLN_TEST_OK(a.status);
  MLN_TEST_OK(b.status);
  MLN_TEST_OK(
    mln_test_render_fixture_finish_operation(&fixture.render, &first)
  );
  MLN_TEST_OK(
    mln_test_render_fixture_finish_operation(&fixture.render, &second)
  );
  mln_test_completion_destroy(&first);
  mln_test_completion_destroy(&second);
  auto map = mln_map_snapshot{.size = sizeof(mln_map_snapshot)};
  auto session =
    mln_render_session_snapshot{.size = sizeof(mln_render_session_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(fixture.map, &map, nullptr));
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture.render.session, &session, nullptr)
  );
  TEST_ASSERT_EQUAL_UINT32(b.extent.width, map.view.extent.width);
  TEST_ASSERT_EQUAL_UINT32(b.extent.height, map.view.extent.height);
  TEST_ASSERT_EQUAL_UINT32(map.view.extent.width, session.extent.width);
  TEST_ASSERT_EQUAL_UINT32(map.view.extent.height, session.extent.height);
  mln_test_render_request_forced(&fixture.render, 202);
  const auto batch = wait_for_results(fixture);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, batch_result(batch, 0).disposition
  );
  mln_render_frame_batch_release(batch);
  destroy_fixture(fixture);
}

// Requests a forced frame for a case that holds the call that renders it
// after the call publishes its result: the window in which a host that acted
// on the result reaches abandon before the call ends, which no public fence
// can hold open.
void request_frame_and_hold_the_call(const Fixture& fixture, uint64_t token) {
  auto demand = mln_frame_demand_default();
  demand.flags = 0;
  demand.token = token;
  MLN_TEST_OK(request_frame(fixture, demand));
}

struct PublishedFrame {
  const Fixture* fixture;
  mln_render_frame_batch batch{MLN_HANDLE_NULL};
};

// Holds once a frame result is published, keeping the batch it drained.
auto frame_published(void* context) -> bool {
  auto& published = *static_cast<PublishedFrame*>(context);
  return mln_render_session_drain_frame_results(
           published.fixture->render.session, &published.batch, nullptr
         ) == MLN_STATUS_OK;
}

struct DriverRelease {
  SyncPointScope* points;
  mln_render_session session;
  // The caller driver's exits before the held call, such as the attach's.
  int exits_before_the_call = 0;
  std::atomic_bool abandon_returned{false};
  bool abandon_waited = false;
  mln_status abandon_status = MLN_STATUS_NATIVE_ERROR;
};

// Releases the held core worker once abandon waits for it, or once abandon
// returned without waiting, so a failing case does not strand the worker.
void release_when_abandon_waits(void* argument) {
  auto& release = *static_cast<DriverRelease*>(argument);
  static_cast<void>(await(
    [&] {
      return release.points->hits(SyncPoint::RenderAbandonWaits) > 0 ||
             release.abandon_returned.load();
    },
    "abandon to wait for the driver call"
  ));
  release.abandon_waited =
    release.points->hits(SyncPoint::RenderAbandonWaits) > 0;
  release.points->release(SyncPoint::RenderFrameResultPublished);
}

// Abandons from outside the caller driver's call, which the case's own thread
// holds open, then lets the call end.
void abandon_inside_the_call(void* argument) {
  auto& release = *static_cast<DriverRelease*>(argument);
  static_cast<void>(release.points->wait_for_hits(
    SyncPoint::RenderDriverExited, release.exits_before_the_call + 1
  ));
  auto result =
    mln_render_abandon_result{.size = sizeof(mln_render_abandon_result)};
  release.abandon_status =
    mln_render_session_abandon(release.session, &result, nullptr);
  release.points->release(SyncPoint::RenderDriverExited);
}

// A core worker's driver call can still be running after the host has read
// the frame result it published, and nothing public observes the call's end,
// so abandon waits it out and succeeds. A caller driver's call is the host's
// own, so abandon from another thread during it is busy and changes nothing.
void abandon_after_a_published_frame_waits_for_a_core_worker_call() {
  auto points = SyncPointScope{};
  auto fixture = Fixture{};
  create_fixture(fixture);
  // A core worker also runs calls of its own, such as ones that service its
  // scheduler, so only the frame's publication tells its call apart. A caller
  // driver runs nothing until the case services it, so its exit is the call.
  points.hold(
    fixture.render.driver == MLN_RENDER_DRIVER_CORE_WORKER
      ? SyncPoint::RenderFrameResultPublished
      : SyncPoint::RenderDriverExited
  );
  // The held hit has to be this case's frame, so nothing may have reached the
  // point before the request.
  TEST_ASSERT_EQUAL_INT(0, points.hits(SyncPoint::RenderFrameResultPublished));
  auto release =
    DriverRelease{.points = &points, .session = fixture.render.session};
  request_frame_and_hold_the_call(fixture, 107);

  auto result =
    mln_render_abandon_result{.size = sizeof(mln_render_abandon_result)};
  if (fixture.render.driver == MLN_RENDER_DRIVER_CORE_WORKER) {
    // The held call published its frame before parking. Its demand is still
    // pending, so the fixture's settled-results wait does not apply.
    auto published = PublishedFrame{.fixture = &fixture};
    TEST_ASSERT_TRUE(mln_test_await(
      frame_published, &published, mln_test_deadline_default(),
      "the held call to publish its frame"
    ));
    TEST_ASSERT_EQUAL_size_t(
      1, mln_test_render_batch_view(published.batch).result_count
    );
    TEST_ASSERT_EQUAL_UINT64(107, batch_result(published.batch, 0).token);
    mln_render_frame_batch_release(published.batch);
    auto* thread = mln_test_thread_start(release_when_abandon_waits, &release);
    release.abandon_status =
      mln_render_session_abandon(fixture.render.session, &result, nullptr);
    release.abandon_returned.store(true);
    mln_test_pulse();
    mln_test_thread_join(thread);
    TEST_ASSERT_TRUE(release.abandon_waited);
    MLN_TEST_OK(release.abandon_status);
    // The session rendered, so it holds a renderer and a backend. Abandon
    // destroys both on Vulkan and Metal, and keeps both on OpenGL and WebGPU,
    // whose objects only their graphics thread may destroy.
#if defined(MLN_FFI_TEST_BACKEND_VULKAN) || defined(MLN_FFI_TEST_BACKEND_METAL)
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_ABANDON_DISPOSITION_CLEAN, result.disposition
    );
    TEST_ASSERT_EQUAL_UINT32(0, result.quarantined_resource_count);
#else
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED, result.disposition
    );
    TEST_ASSERT_EQUAL_UINT32(2, result.quarantined_resource_count);
#endif
  } else {
    // Servicing the attach already passed the exit point, so the held call is
    // the next exit.
    release.exits_before_the_call = points.hits(SyncPoint::RenderDriverExited);
    auto* thread = mln_test_thread_start(abandon_inside_the_call, &release);

    MLN_TEST_OK(
      mln_render_session_service_driver_work(fixture.render.session, nullptr)
    );
    mln_test_thread_join(thread);
    MLN_TEST_STATUS(MLN_STATUS_BUSY, release.abandon_status);
  }
  auto snapshot =
    mln_render_session_snapshot{.size = sizeof(mln_render_session_snapshot)};
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture.render.session, &snapshot, nullptr)
  );
  TEST_ASSERT_EQUAL_UINT32(
    fixture.render.driver == MLN_RENDER_DRIVER_CORE_WORKER
      ? MLN_RENDER_SESSION_STATE_ABANDONED
      : MLN_RENDER_SESSION_STATE_ATTACHED,
    snapshot.state
  );
  destroy_fixture(fixture);
}

struct DetachSubmission {
  mln_render_session session;
  mln_test_completion completion;
  mln_status status = MLN_STATUS_NATIVE_ERROR;
};

void submit_detach(void* argument) {
  auto& detach = *static_cast<DetachSubmission*>(argument);
  detach.status = mln_render_session_detach(
    detach.session, &detach.completion.descriptor, nullptr
  );
}

struct BlockerRelease {
  SyncPointScope* points;
  mln_test_gate* gate;
  std::atomic_bool abandon_returned{false};
};

// Lets the blocked driver call end once abandon waits for it, or once abandon
// returned without waiting, as it does for a caller driver.
void release_blocker_when_abandon_waits(void* argument) {
  auto& release = *static_cast<BlockerRelease*>(argument);
  static_cast<void>(await(
    [&] {
      return release.points->hits(SyncPoint::RenderAbandonWaits) > 0 ||
             release.abandon_returned.load();
    },
    "abandon to wait for the driver call"
  ));
  mln_test_gate_release(release.gate);
}

// A host can abandon a session while another thread's detach is still
// returning, after it marked the session detaching. Its work is queued by
// then, so the abandon completes the detach as abandoned instead of leaving
// it queued for a driver that has stopped. The blocked driver keeps the
// detach queued until the abandon takes it.
void an_abandon_during_a_detach_submission_completes_the_detach() {
  auto points = SyncPointScope{};
  auto fixture = Fixture{};
  create_fixture(fixture);
  auto blocker = DriverBlocker{};
  block_driver(fixture, blocker);
  points.hold(SyncPoint::RenderDetachQueued);
  auto detach = DetachSubmission{
    .session = fixture.render.session,
    .completion = mln_test_completion_default(0),
  };
  auto* detaching = mln_test_thread_start(submit_detach, &detach);
  TEST_ASSERT_TRUE(points.wait_for_hits(SyncPoint::RenderDetachQueued, 1));

  auto release = BlockerRelease{.points = &points, .gate = blocker.gate.get()};
  auto* releasing =
    mln_test_thread_start(release_blocker_when_abandon_waits, &release);
  auto result =
    mln_render_abandon_result{.size = sizeof(mln_render_abandon_result)};
  const auto abandon_status =
    mln_render_session_abandon(fixture.render.session, &result, nullptr);
  release.abandon_returned.store(true);
  mln_test_pulse();
  mln_test_thread_join(releasing);
  points.release(SyncPoint::RenderDetachQueued);
  mln_test_thread_join(detaching);

  MLN_TEST_OK(abandon_status);
  MLN_TEST_OK(detach.status);
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_completion_wait(&detach.completion, -1),
    "the detach completion never arrived"
  );
  MLN_TEST_STATUS(
    MLN_STATUS_TARGET_LOST, mln_test_completion_status(&detach.completion)
  );
  mln_test_completion_destroy(&detach.completion);
  // A core worker finished the blocked call before the abandon took the
  // queue; a caller driver never ran it.
  TEST_ASSERT_TRUE(mln_test_completion_wait(&blocker.completion, -1));
  MLN_TEST_STATUS(
    fixture.render.driver == MLN_RENDER_DRIVER_CORE_WORKER
      ? MLN_STATUS_OK
      : MLN_STATUS_TARGET_LOST,
    mln_test_completion_status(&blocker.completion)
  );
  mln_test_completion_destroy(&blocker.completion);
  blocker.submitted = false;
  destroy_fixture(fixture);
}

struct SupersedingRequest {
  const Fixture* fixture;
  mln_status status = MLN_STATUS_NATIVE_ERROR;
};

void request_superseding_frame(void* argument) {
  auto& request = *static_cast<SupersedingRequest*>(argument);
  auto demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_IF_NEEDED;
  demand.token = 122;
  request.status = request_frame(*request.fixture, demand);
}

struct Abandon {
  mln_render_session session;
  std::atomic_bool returned{false};
  mln_status status = MLN_STATUS_NATIVE_ERROR;
};

void abandon_session(void* argument) {
  auto& abandon = *static_cast<Abandon*>(argument);
  auto result =
    mln_render_abandon_result{.size = sizeof(mln_render_abandon_result)};
  abandon.status =
    mln_render_session_abandon(abandon.session, &result, nullptr);
  abandon.returned.store(true);
  mln_test_pulse();
}

// A request_frame that supersedes a demand sets the frame wake pending and
// invokes it after releasing the session's lock. An abandon that runs in that
// window closes the wake, so the request's late wake does nothing, and the
// abandon wakes the host for the queued results itself. Holding the request
// between its lock and its wake is the window no public fence reaches.
void an_abandon_wakes_for_results_a_racing_request_has_yet_to_wake() {
  auto points = SyncPointScope{};
  auto fixture = Fixture{};
  create_fixture(fixture);
  auto blocker = DriverBlocker{};
  block_driver(fixture, blocker);
  // Results already queued would leave the frame wake pending, so the
  // superseding request would owe no wake.
  auto drained = mln_render_frame_batch{MLN_HANDLE_NULL};
  while (mln_render_session_drain_frame_results(
           fixture.render.session, &drained, nullptr
         ) == MLN_STATUS_OK) {
    mln_render_frame_batch_release(drained);
    drained = MLN_HANDLE_NULL;
  }
  auto first = mln_frame_demand_default();
  first.flags = MLN_FRAME_DEMAND_IF_NEEDED;
  first.token = 121;
  MLN_TEST_OK(request_frame(fixture, first));
  points.hold(SyncPoint::RenderFrameWakeDeferred);
  auto request = SupersedingRequest{.fixture = &fixture};
  auto* requesting = mln_test_thread_start(request_superseding_frame, &request);
  TEST_ASSERT_TRUE(points.wait_for_hits(SyncPoint::RenderFrameWakeDeferred, 1));
  const auto wakes_before = atomic_load(&fixture.render.frame_wakes);

  auto release = BlockerRelease{.points = &points, .gate = blocker.gate.get()};
  auto* releasing =
    mln_test_thread_start(release_blocker_when_abandon_waits, &release);
  auto abandon = Abandon{.session = fixture.render.session};
  auto* abandoning = mln_test_thread_start(abandon_session, &abandon);
  // An abandon that owes the wake parks at the held point before it closes
  // the wake; one that does not returns without reaching it.
  TEST_ASSERT_TRUE(await(
    [&] {
      return points.hits(SyncPoint::RenderFrameWakeDeferred) > 1 ||
             abandon.returned.load();
    },
    "abandon to reach the frame wake or return"
  ));
  points.release(SyncPoint::RenderFrameWakeDeferred);
  mln_test_thread_join(abandoning);
  release.abandon_returned.store(true);
  mln_test_pulse();
  mln_test_thread_join(releasing);
  mln_test_thread_join(requesting);

  MLN_TEST_OK(request.status);
  MLN_TEST_OK(abandon.status);
  TEST_ASSERT_GREATER_THAN_UINT(
    wakes_before, atomic_load(&fixture.render.frame_wakes)
  );
  auto batch = mln_render_frame_batch{MLN_HANDLE_NULL};
  MLN_TEST_OK(mln_render_session_drain_frame_results(
    fixture.render.session, &batch, nullptr
  ));
  TEST_ASSERT_EQUAL_size_t(2, mln_test_render_batch_view(batch).result_count);
  const auto superseded = batch_result(batch, 0);
  const auto stranded = batch_result(batch, 1);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(121, superseded.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_SUPERSEDED, superseded.disposition
  );
  TEST_ASSERT_EQUAL_UINT64(122, stranded.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_TARGET_NOT_READY, stranded.disposition
  );
  TEST_ASSERT_TRUE(mln_test_completion_wait(&blocker.completion, -1));
  mln_test_completion_destroy(&blocker.completion);
  blocker.submitted = false;
  destroy_fixture(fixture);
}

// A frame release that the host made before abandon lets go of its slot even
// while the driver has yet to run it, because CPU_COMPLETE attests that the
// host's GPU read is done. Abandon runs the queued release before it decides
// what to keep, so it destroys a Vulkan or Metal ring with the rest. Holding
// the driver keeps the release queued until the abandon, which no public
// fence reaches.
void abandon_destroys_a_ring_whose_frame_release_is_still_queued() {
  auto points = SyncPointScope{};
  auto fixture = Fixture{};
  create_fixture(fixture);
  auto frame = mln_test_render_and_acquire(&fixture.render, 141);
  auto blocker = DriverBlocker{};
  block_driver(fixture, blocker);
  MLN_TEST_OK(mln_acquired_frame_release(frame, nullptr, nullptr));

  auto release = BlockerRelease{.points = &points, .gate = blocker.gate.get()};
  auto* releasing =
    mln_test_thread_start(release_blocker_when_abandon_waits, &release);
  auto result =
    mln_render_abandon_result{.size = sizeof(mln_render_abandon_result)};
  const auto abandon_status =
    mln_render_session_abandon(fixture.render.session, &result, nullptr);
  release.abandon_returned.store(true);
  mln_test_pulse();
  mln_test_thread_join(releasing);
  MLN_TEST_OK(abandon_status);
  // OpenGL and WebGPU keep the renderer and the backend, which only their
  // graphics thread may destroy.
#if defined(MLN_FFI_TEST_BACKEND_VULKAN) || defined(MLN_FFI_TEST_BACKEND_METAL)
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_ABANDON_DISPOSITION_CLEAN, result.disposition
  );
  TEST_ASSERT_EQUAL_UINT32(0, result.quarantined_resource_count);
#else
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED, result.disposition
  );
  TEST_ASSERT_EQUAL_UINT32(2, result.quarantined_resource_count);
#endif
  TEST_ASSERT_TRUE(mln_test_completion_wait(&blocker.completion, -1));
  mln_test_completion_destroy(&blocker.completion);
  blocker.submitted = false;
  destroy_fixture(fixture);
}

// Releases the host's frame with a GPU wait from the completion of an
// operation that abandon discards, which runs after abandon took the
// session's backend and before it decides what to keep.
struct RacingRelease {
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  mln_status completion_status = MLN_STATUS_NATIVE_ERROR;
  mln_status release_status = MLN_STATUS_NATIVE_ERROR;
};

void release_with_a_gpu_wait(
  void* user_data, const mln_completion_result* result
) {
  auto& racing = *static_cast<RacingRelease*>(user_data);
  racing.completion_status = static_cast<mln_status>(result->status);
  auto sync = mln_gpu_sync_default();
  sync.kind = UINT32_MAX;
  racing.release_status =
    mln_acquired_frame_release(racing.frame, &sync, nullptr);
  if (racing.release_status == MLN_STATUS_OK) racing.frame = MLN_HANDLE_NULL;
}

// A frame release can reach a session after abandon took its backend and
// before abandon decides what to keep, when no backend checks the release's
// sync kind. Native never runs the GPU wait that such a release hands over,
// so the host's GPU may still read the slot, and abandon keeps the ring. The
// blocked driver keeps the operation queued until abandon discards it, which
// no public fence reaches.
void abandon_keeps_a_ring_released_with_a_gpu_wait_as_it_abandons() {
  auto points = SyncPointScope{};
  auto fixture = Fixture{};
  create_fixture(fixture);
  auto racing =
    RacingRelease{.frame = mln_test_render_and_acquire(&fixture.render, 142)};
  auto blocker = DriverBlocker{};
  block_driver(fixture, blocker);
  const auto completion = mln_completion{
    .size = sizeof(mln_completion),
    .callback = release_with_a_gpu_wait,
    .user_data = &racing,
    .release_user_data = nullptr,
  };
  MLN_TEST_OK(mln_render_session_reduce_memory_use(
    fixture.render.session, &completion, nullptr
  ));

  auto release = BlockerRelease{.points = &points, .gate = blocker.gate.get()};
  auto* releasing =
    mln_test_thread_start(release_blocker_when_abandon_waits, &release);
  auto result =
    mln_render_abandon_result{.size = sizeof(mln_render_abandon_result)};
  const auto abandon_status =
    mln_render_session_abandon(fixture.render.session, &result, nullptr);
  release.abandon_returned.store(true);
  mln_test_pulse();
  mln_test_thread_join(releasing);
  MLN_TEST_OK(abandon_status);
  MLN_TEST_STATUS(MLN_STATUS_TARGET_LOST, racing.completion_status);
  MLN_TEST_OK(racing.release_status);
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, racing.frame);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED, result.disposition
  );
  // OpenGL and WebGPU keep the renderer too, which only their graphics thread
  // may destroy.
#if defined(MLN_FFI_TEST_BACKEND_VULKAN) || defined(MLN_FFI_TEST_BACKEND_METAL)
  TEST_ASSERT_EQUAL_UINT32(1, result.quarantined_resource_count);
#else
  TEST_ASSERT_EQUAL_UINT32(2, result.quarantined_resource_count);
#endif
  TEST_ASSERT_TRUE(mln_test_completion_wait(&blocker.completion, -1));
  mln_test_completion_destroy(&blocker.completion);
  blocker.submitted = false;
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  mln_test_render_fixture_keep_graphics_until_exit(&fixture.render);
#endif
  destroy_fixture(fixture);
}

struct ParkedDisposal {
  SyncPointScope* points;
  mln_acquired_frame first;
  mln_acquired_frame second;
  bool parked = false;
  mln_status first_status = MLN_STATUS_NATIVE_ERROR;
  mln_status second_status = MLN_STATUS_NATIVE_ERROR;
  std::atomic_bool done{false};
};

// Disposes both frames while the driver is held just after parking a demand,
// then lets the driver go.
void dispose_when_parked(void* argument) {
  auto& disposal = *static_cast<ParkedDisposal*>(argument);
  disposal.parked =
    disposal.points->wait_for_hits(SyncPoint::RenderFrameDemandParked, 1);
  disposal.first_status = mln_acquired_frame_dispose(disposal.first, nullptr);
  disposal.second_status = mln_acquired_frame_dispose(disposal.second, nullptr);
  disposal.points->release(SyncPoint::RenderFrameDemandParked);
  disposal.done = true;
}

// A demand that finds the ring full parks in the same locked section, so a
// disposal on another thread that quarantines the last slot sees the parked
// demand and queues the work that resolves it. Holding the driver right after
// the park is the window that no public fence reaches.
void a_demand_parked_as_disposal_quarantines_the_ring_gets_a_result() {
  auto points = SyncPointScope{};
  auto fixture = Fixture{};
  create_fixture(fixture);
  auto disposal = ParkedDisposal{
    .points = &points,
    .first = mln_test_render_and_acquire(&fixture.render, 131),
    .second = mln_test_render_and_acquire(&fixture.render, 132),
  };
  points.hold(SyncPoint::RenderFrameDemandParked);
  auto* disposing = mln_test_thread_start(dispose_when_parked, &disposal);
  mln_test_render_request_forced(&fixture.render, 133);
  // A caller driver parks the demand inside a service of the case's thread
  // until the disposal lets it go.
  service_until(fixture, disposal.done);
  mln_test_thread_join(disposing);
  TEST_ASSERT_TRUE(disposal.parked);
  MLN_TEST_OK(disposal.first_status);
  MLN_TEST_OK(disposal.second_status);

  const auto batch = wait_for_results(fixture);
  const auto result = batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(133, result.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_TARGET_NOT_READY, result.disposition
  );
  // Detach keeps the quarantined ring, and a kept Vulkan ring is a child of
  // the host's device.
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  mln_test_render_fixture_keep_graphics_until_exit(&fixture.render);
#endif
  destroy_fixture(fixture);
}

auto request_waiting(const Fixture& fixture, uint64_t token) -> mln_status {
  auto demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_IF_NEEDED | MLN_FRAME_DEMAND_WAIT_FOR_UPDATE;
  demand.token = token;
  return request_frame(fixture, demand);
}

void render_until_idle(const Fixture& fixture) {
  static_cast<void>(mln_test_render_until_idle(
    fixture.runtime, fixture.map, &fixture.render, nullptr
  ));
}

// Runs after every work item that the driver already holds. Dumping debug
// logs publishes no map update, which would end a demand's wait.
void fence_driver(const Fixture& fixture) {
  auto fence = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_dump_debug_logs(
    fixture.render.session, &fence.descriptor, nullptr
  ));
  MLN_TEST_OK(
    mln_test_render_fixture_finish_operation(&fixture.render, &fence)
  );
  mln_test_completion_destroy(&fence);
}

// A release whose wait for the host's GPU fails quarantines its slot.
// Quarantining the last usable one ends the demand that waits for a map
// update, which no slot could ever render.
void a_failed_release_of_the_last_slot_ends_a_waiting_demand() {
  auto fixture = Fixture{};
  create_fixture(fixture);
  render_until_idle(fixture);
  install_frame_failure(fixture, FrameFailure::Release);
  auto first = mln_test_render_and_acquire(&fixture.render, 171);
  auto second = mln_test_render_and_acquire(&fixture.render, 172);
  MLN_TEST_OK(request_waiting(fixture, 173));
  fence_driver(fixture);
  const auto fenced = mln_gpu_sync{
    .size = sizeof(mln_gpu_sync), .kind = MLN_GPU_SYNC_OPENGL_FENCE, .object = 1
  };
  MLN_TEST_OK(mln_acquired_frame_release(first, &fenced, nullptr));
  MLN_TEST_OK(mln_acquired_frame_release(second, &fenced, nullptr));
  const auto batch = wait_for_results(fixture);
  const auto ended = batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(173, ended.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_TARGET_NOT_READY, ended.disposition
  );
  // Detach keeps the quarantined ring, and a kept Vulkan ring is a child of
  // the host's device.
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  mln_test_render_fixture_keep_graphics_until_exit(&fixture.render);
#endif
  destroy_fixture(fixture);
}

struct MaintenanceProbe {
  mln_render_session session;
  bool results_queued = false;
  std::atomic_bool done{false};
};

// Records whether any frame result was queued when maintenance completed.
void probe_results(void* context, const mln_completion_result*) {
  auto& probe = *static_cast<MaintenanceProbe*>(context);
  auto batch = mln_render_frame_batch{MLN_HANDLE_NULL};
  probe.results_queued =
    mln_render_session_drain_frame_results(probe.session, &batch, nullptr) ==
    MLN_STATUS_OK;
  if (probe.results_queued) mln_render_frame_batch_release(batch);
  probe.done = true;
  mln_test_pulse();
}

// Releases that quarantine the last slot leave a demand accepted after
// maintenance to end in its own turn on the driver, after the maintenance.
void a_quarantined_ring_ends_a_demand_in_driver_order() {
  auto fixture = Fixture{};
  create_fixture(fixture);
  render_until_idle(fixture);
  install_frame_failure(fixture, FrameFailure::Release);
  auto first = mln_test_render_and_acquire(&fixture.render, 181);
  auto second = mln_test_render_and_acquire(&fixture.render, 182);
  const auto fenced = mln_gpu_sync{
    .size = sizeof(mln_gpu_sync), .kind = MLN_GPU_SYNC_OPENGL_FENCE, .object = 1
  };
  MLN_TEST_OK(mln_acquired_frame_release(first, &fenced, nullptr));
  MLN_TEST_OK(mln_acquired_frame_release(second, &fenced, nullptr));
  auto probe = MaintenanceProbe{.session = fixture.render.session};
  const auto maintenance = mln_completion{
    .size = sizeof(mln_completion),
    .callback = probe_results,
    .user_data = &probe,
    .release_user_data = nullptr,
  };
  MLN_TEST_OK(mln_render_session_reduce_memory_use(
    fixture.render.session, &maintenance, nullptr
  ));
  mln_test_render_request_forced(&fixture.render, 183);
  service_until(fixture, probe.done);
  TEST_ASSERT_FALSE(probe.results_queued);
  const auto batch = wait_for_results(fixture);
  const auto ended = batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(183, ended.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_TARGET_NOT_READY, ended.disposition
  );
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  mln_test_render_fixture_keep_graphics_until_exit(&fixture.render);
#endif
  destroy_fixture(fixture);
}

auto latest_update_generation(mln_map map) -> uint64_t {
  auto snapshot = mln_map_snapshot{.size = sizeof(mln_map_snapshot)};
  if (mln_map_get_snapshot(map, &snapshot, nullptr) != MLN_STATUS_OK) return 0;
  return snapshot.latest_render_update_generation;
}

auto submit_camera_zoom(mln_map map, double zoom, mln_test_completion& done)
  -> mln_status {
  auto update = mln_camera_update_default();
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = zoom;
  return mln_map_update_camera(map, &update, &done.descriptor, nullptr);
}

struct HeldCameraMove {
  SyncPointScope* points;
  mln_map map;
  uint64_t generation_before = 0;
  bool held = false;
  mln_status status = MLN_STATUS_NATIVE_ERROR;
  bool published = false;
  std::atomic_bool done{false};
};

// Publishes a map update while the driver is held after a waiting demand
// found nothing to render, then lets the driver go.
void move_camera_while_the_demand_looks(void* argument) {
  auto& move = *static_cast<HeldCameraMove*>(argument);
  move.held = move.points->wait_for_hits(SyncPoint::RenderDemandWaits, 1);
  auto done = mln_test_completion_default(0);
  move.status = submit_camera_zoom(move.map, 3.0, done);
  if (move.status == MLN_STATUS_OK) {
    static_cast<void>(mln_test_completion_wait(&done, -1));
  } else {
    mln_test_completion_reject(&done);
  }
  mln_test_completion_destroy(&done);
  move.published = await(
    [&] { return latest_update_generation(move.map) > move.generation_before; },
    "the camera update to be published"
  );
  move.points->release(SyncPoint::RenderDemandWaits);
  move.done = true;
}

struct HeldBarrier {
  const Fixture* fixture;
  SyncPointScope* points;
  mln_test_completion* completion;
  bool held = false;
  mln_status status = MLN_STATUS_NATIVE_ERROR;
  mln_status next_status = MLN_STATUS_NATIVE_ERROR;
  std::atomic_bool done{false};
};

void end_the_running_demand(void* context) {
  auto& barrier = *static_cast<HeldBarrier*>(context);
  barrier.held = barrier.points->wait_for_hits(SyncPoint::RenderDemandWaits, 1);
  barrier.status = mln_render_session_barrier(
    barrier.fixture->render.session, &barrier.completion->descriptor, nullptr
  );
  barrier.next_status = request_waiting(*barrier.fixture, 158);
  barrier.points->release(SyncPoint::RenderDemandWaits);
  barrier.done = true;
}

// A barrier accepted as a running demand starts waiting ends that demand.
// A demand accepted after the barrier still waits and renders the next
// update.
void a_barrier_ends_a_running_wait_without_ending_a_later_demand() {
  auto fixture = Fixture{};
  create_fixture(fixture);
  render_until_idle(fixture);
  auto points = SyncPointScope{};
  points.hold(SyncPoint::RenderDemandWaits);
  auto done = mln_test_completion_default(0);
  auto barrier = HeldBarrier{&fixture, &points, &done};
  auto* submitting = mln_test_thread_start(end_the_running_demand, &barrier);
  MLN_TEST_OK(request_waiting(fixture, 157));
  service_until(fixture, barrier.done);
  mln_test_thread_join(submitting);
  TEST_ASSERT_TRUE(barrier.held);
  MLN_TEST_OK(barrier.status);
  MLN_TEST_OK(barrier.next_status);
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture.render, &done));
  mln_test_completion_destroy(&done);
  auto batch = wait_for_results(fixture);
  const auto ended = batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(157, ended.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_NO_UPDATE, ended.disposition);
  auto move = mln_test_completion_default(0);
  MLN_TEST_OK(submit_camera_zoom(fixture.map, 3.0, move));
  TEST_ASSERT_TRUE(mln_test_completion_wait(&move, -1));
  mln_test_completion_destroy(&move);
  batch = wait_for_results(fixture);
  const auto rendered = batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(158, rendered.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, rendered.disposition);
  destroy_fixture(fixture);
}

// A map update can land after a waiting demand found nothing to render and
// before the demand waits, when the session's update notice finds no demand
// waiting. The demand reads the map's generation again under the lock that
// the notice takes, so it runs again and renders the update instead of
// waiting for one that already came. It looks once: the update it renders
// ends it. Holding the driver between the look and the wait is the window
// that no public fence reaches.
void a_waiting_demand_renders_an_update_that_lands_as_it_looks() {
  auto points = SyncPointScope{};
  auto fixture = Fixture{};
  create_fixture(fixture);
  render_until_idle(fixture);
  auto move = HeldCameraMove{
    .points = &points,
    .map = fixture.map,
    .generation_before = latest_update_generation(fixture.map),
  };
  points.hold(SyncPoint::RenderDemandWaits);
  auto* moving =
    mln_test_thread_start(move_camera_while_the_demand_looks, &move);
  MLN_TEST_OK(request_waiting(fixture, 151));
  // A caller driver holds the demand inside a service of the case's thread
  // until the camera update lands.
  service_until(fixture, move.done);
  mln_test_thread_join(moving);
  TEST_ASSERT_TRUE(move.held);
  MLN_TEST_OK(move.status);
  TEST_ASSERT_TRUE(move.published);

  const auto batch = wait_for_results(fixture);
  const auto result = batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(151, result.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  TEST_ASSERT_GREATER_THAN_UINT64(
    move.generation_before, result.map_update_generation
  );
  destroy_fixture(fixture);
}

// The session learns of a map update from the map's notice, which trails the
// map storing it. A waiting demand that finds the map's latest update already
// rendered waits, even while the session has yet to learn of that update,
// rather than running again until the notice lands. The notice then brings
// nothing the session has not drawn, so the demand it resumes waits again and
// renders the next update instead of the same one twice. Holding the notice
// is the window that no public fence reaches.
void a_waiting_demand_waits_for_an_update_the_session_has_yet_to_learn_of() {
  auto points = SyncPointScope{};
  auto fixture = Fixture{};
  create_fixture(fixture);
  render_until_idle(fixture);
  const auto before = latest_update_generation(fixture.map);
  points.hold(SyncPoint::RenderSessionUpdateNoticed);
  // The map stores the update and advances its snapshot before the notice
  // parks, which can keep the camera command from finishing until release.
  auto held_move = mln_test_completion_default(0);
  MLN_TEST_OK(submit_camera_zoom(fixture.map, 3.0, held_move));
  TEST_ASSERT_TRUE(await(
    [&] { return latest_update_generation(fixture.map) > before; },
    "the camera update to be stored"
  ));
  const auto held = latest_update_generation(fixture.map);

  auto forced = mln_frame_demand_default();
  forced.flags = 0;
  forced.token = 155;
  MLN_TEST_OK(request_frame(fixture, forced));
  auto batch = wait_for_results(fixture);
  const auto rendered = batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(155, rendered.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, rendered.disposition);
  TEST_ASSERT_EQUAL_UINT64(held, rendered.map_update_generation);

  MLN_TEST_OK(request_waiting(fixture, 156));
  // The first fence runs after the demand looks, and the second after any
  // work that the look queued.
  fence_driver(fixture);
  fence_driver(fixture);
  auto snapshot =
    mln_render_session_snapshot{.size = sizeof(mln_render_session_snapshot)};
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture.render.session, &snapshot, nullptr)
  );
  TEST_ASSERT_LESS_THAN_UINT64(held, snapshot.map_update_generation);
  TEST_ASSERT_EQUAL_UINT64(held, snapshot.rendered_update_generation);
  TEST_ASSERT_EQUAL_INT(1, points.hits(SyncPoint::RenderDemandWaits));

  points.release(SyncPoint::RenderSessionUpdateNoticed);
  TEST_ASSERT_TRUE(mln_test_completion_wait(&held_move, -1));
  mln_test_completion_destroy(&held_move);
  auto next_move = mln_test_completion_default(0);
  MLN_TEST_OK(submit_camera_zoom(fixture.map, 4.0, next_move));
  TEST_ASSERT_TRUE(mln_test_completion_wait(&next_move, -1));
  mln_test_completion_destroy(&next_move);
  batch = wait_for_results(fixture);
  const auto waited = batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(156, waited.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, waited.disposition);
  TEST_ASSERT_GREATER_THAN_UINT64(held, waited.map_update_generation);
  destroy_fixture(fixture);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(graphics_failures_finish_demands_and_lose_the_target);
  RUN_TEST(an_unavailable_drawable_releases_its_slot_for_a_retry);
  RUN_TEST(an_idle_demand_reports_a_failed_backend_activation);
  RUN_TEST(a_same_size_resize_redraws_after_an_earlier_queued_frame);
  RUN_TEST(a_resize_queued_before_detach_completes_as_failed);
  RUN_TEST(a_barrier_waits_for_an_ordered_operation_after_a_running_demand);
  RUN_TEST(if_needed_demands_supersede_and_forced_demands_render);
  RUN_TEST(supersession_preserves_a_forced_demand_after_a_resize);
  RUN_TEST(scheduler_service_preserves_a_forced_demand_after_a_resize);
  RUN_TEST(concurrent_resizes_publish_matching_map_and_driver_extents);
  RUN_TEST(abandon_after_a_published_frame_waits_for_a_core_worker_call);
  RUN_TEST(an_abandon_during_a_detach_submission_completes_the_detach);
  RUN_TEST(an_abandon_wakes_for_results_a_racing_request_has_yet_to_wake);
  RUN_TEST(abandon_destroys_a_ring_whose_frame_release_is_still_queued);
  RUN_TEST(abandon_keeps_a_ring_released_with_a_gpu_wait_as_it_abandons);
  RUN_TEST(a_demand_parked_as_disposal_quarantines_the_ring_gets_a_result);
  RUN_TEST(a_failed_release_of_the_last_slot_ends_a_waiting_demand);
  RUN_TEST(a_quarantined_ring_ends_a_demand_in_driver_order);
  RUN_TEST(a_waiting_demand_renders_an_update_that_lands_as_it_looks);
  RUN_TEST(a_barrier_ends_a_running_wait_without_ending_a_later_demand);
  RUN_TEST(
    a_waiting_demand_waits_for_an_update_the_session_has_yet_to_learn_of
  );
}
