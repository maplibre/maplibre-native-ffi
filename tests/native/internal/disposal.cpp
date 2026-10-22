// Disposal retires the graph only once nothing still runs against it.

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>

#include "completion/completion.hpp"
#include "internal/support/allocation_faults.hpp"
#include "internal/support/checks.hpp"
#include "internal/support/driver_blocker.hpp"
#include "internal/support/sync_points.hpp"
#include "map/map.hpp"
#include "map/map_internal.hpp"
#include "maplibre_native_c.h"
#include "maplibre_native_c/binding_support.h"
#include "operation/operation.hpp"
#include "render/render_session_common.hpp"
#include "runtime/runtime.hpp"
#include "support/camera.h"
#include "support/frames.h"
#include "support/harness.h"
#include "support/status.h"
#include "support/test_support.h"
#include "support/wait.h"
#include "unity.h"

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
#include "support/host_graphics.h"
#endif

namespace {

using mln::native_tests::AllocationFaults;
using mln::native_tests::await;
using mln::native_tests::BackgroundChecks;
using mln::native_tests::DriverBlocker;
using mln::native_tests::SyncPoint;
using mln::native_tests::SyncPointScope;

// The completions that the fake surface attachments and the blocking driver
// work below stand in for. Any surface attachment and any maintenance request
// serves, because each one delivers no value.
constexpr auto surface_attach_completion =
  mln::core::valueless_completion<&mln_map_attach_metal_surface>();
constexpr auto driver_work_completion =
  mln::core::valueless_completion<&mln_render_session_reduce_memory_use>();

struct Result {
  std::atomic_int status{MLN_STATUS_INVALID_STATE};
  std::atomic_uint releases{0};
  std::atomic_uint64_t handle{MLN_HANDLE_NULL};
};

auto descriptor(Result& result) -> mln_completion {
  return {
    .size = sizeof(mln_completion),
    .callback =
      [](void* context, const mln_completion_result* completion) {
        auto& result = *static_cast<Result*>(context);
        result.status = completion->status;
        if (completion->value != nullptr)
          result.handle = *static_cast<const mln_map*>(completion->value);
      },
    .user_data = &result,
    .release_user_data =
      [](void* context) {
        ++static_cast<Result*>(context)->releases;
        mln_test_pulse();
      },
  };
}

auto released(const Result& result, unsigned count = 1) -> bool {
  return await(
    [&] { return result.releases.load() >= count; }, "a completion's release"
  );
}

auto expired(const auto& weak) -> bool {
  return await([&] { return weak.expired(); }, "an object to retire");
}

auto create_runtime() -> mln_runtime {
  auto options = mln_runtime_options_default();
  auto runtime = mln_runtime{MLN_HANDLE_NULL};
  MLN_TEST_OK(mln_runtime_create(&options, &runtime, nullptr));
  return runtime;
}

auto create_map(mln_runtime runtime) -> mln_map {
  auto result = Result{};
  const auto completion = descriptor(result);
  auto options = mln_map_options_default();
  options.map_mode = MLN_MAP_MODE_STATIC;
  MLN_TEST_OK(mln_runtime_create_map(runtime, &options, &completion, nullptr));
  TEST_ASSERT_TRUE(released(result));
  MLN_TEST_OK(result.status.load());
  return result.handle;
}

// Parks the runtime worker in a task of its own until released.
struct WorkerGate {
  std::atomic_bool entered = false;
  std::atomic_bool release = false;
};

void park_worker(mln::core::RuntimeObject& runtime, WorkerGate& gate) {
  runtime.executor.invoke([&gate] {
    gate.entered = true;
    mln_test_pulse();
    static_cast<void>(await([&] { return gate.release.load(); }, "release"));
  });
  TEST_ASSERT_TRUE(await([&] { return gate.entered.load(); }, "the worker"));
}

void discarded_commands_keep_the_graph_alive_until_the_worker_drains() {
  const auto runtime = create_runtime();
  const auto map = create_map(runtime);
  auto live = mln::core::lease_runtime(runtime);
  auto weak = std::weak_ptr{live};
  auto worker = WorkerGate{};
  park_worker(*live, worker);
  const auto submitted = mln_map_set_debug_options(
    map, MLN_MAP_DEBUG_TILE_BORDERS, nullptr, nullptr
  );
  const auto map_disposed = mln_map_dispose(map, nullptr);
  const auto runtime_disposed = mln_runtime_dispose(runtime, nullptr);
  worker.release = true;
  mln_test_pulse();
  live.reset();
  MLN_TEST_OK(submitted);
  MLN_TEST_OK(map_disposed);
  MLN_TEST_OK(runtime_disposed);
  TEST_ASSERT_TRUE(expired(weak));
}

// Disposal keeps queued commands and map cleanup running to completion: the
// runtime retires only after the map's pool shuts down.
void disposal_drains_queued_work_and_map_cleanup() {
  const auto runtime = create_runtime();
  const auto map = create_map(runtime);
  auto live = mln::core::lease_runtime(runtime);
  auto runtime_weak = std::weak_ptr{live};
  auto rejected = Result{};
  const auto rejected_completion = descriptor(rejected);
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_runtime_release(runtime, &rejected_completion, nullptr)
  );
  auto sync_points = SyncPointScope{};
  sync_points.hold(SyncPoint::MapPoolShutdown);
  auto worker = WorkerGate{};
  park_worker(*live, worker);
  auto resize = Result{};
  auto still = Result{};
  const auto resize_completion = descriptor(resize);
  const auto still_completion = descriptor(still);
  MLN_TEST_OK(
    mln_map_resize(map, {300, 200, 1.0}, &resize_completion, nullptr)
  );
  MLN_TEST_OK(mln_map_request_still_image(map, &still_completion, nullptr));
  MLN_TEST_OK(mln_runtime_dispose(runtime, nullptr));
  TEST_ASSERT_NULL_MESSAGE(
    mln::core::lease_runtime(runtime).get(),
    "a disposed parent stayed publicly live"
  );
  MLN_TEST_OK(mln_map_dispose(map, nullptr));
  worker.release = true;
  mln_test_pulse();
  live.reset();
  TEST_ASSERT_TRUE(sync_points.wait_for_hits(SyncPoint::MapPoolShutdown, 1));
  TEST_ASSERT_FALSE_MESSAGE(
    runtime_weak.expired(), "the runtime retired before its map's cleanup"
  );
  sync_points.release(SyncPoint::MapPoolShutdown);
  TEST_ASSERT_TRUE(expired(runtime_weak));
  TEST_ASSERT_EQUAL_UINT(1, resize.releases.load());
  MLN_TEST_OK(resize.status.load());
  TEST_ASSERT_EQUAL_UINT(1, still.releases.load());
  MLN_TEST_STATUS(MLN_STATUS_CANCELLED, still.status.load());
}

void failed_creation_releases_parent_reservation() {
  const auto runtime = create_runtime();
  auto weak = std::weak_ptr{mln::core::lease_runtime(runtime)};
  auto result = Result{};
  const auto completion = descriptor(result);
  auto status = MLN_STATUS_OK;
  {
    const auto faults = AllocationFaults{};
    status = mln_runtime_create_map(runtime, nullptr, &completion, nullptr);
  }
  MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, status);
  TEST_ASSERT_EQUAL_UINT(0, result.releases.load());
  MLN_TEST_OK(mln_runtime_dispose(runtime, nullptr));
  TEST_ASSERT_TRUE(expired(weak));
}

void disposal_waits_for_pending_child_creation() {
  const auto runtime = create_runtime();
  auto live = mln::core::lease_runtime(runtime);
  auto weak = std::weak_ptr{live};
  auto worker = WorkerGate{};
  park_worker(*live, worker);
  auto result = Result{};
  const auto completion = descriptor(result);
  MLN_TEST_OK(mln_runtime_create_map(runtime, nullptr, &completion, nullptr));
  MLN_TEST_OK(mln_runtime_dispose(runtime, nullptr));
  live.reset();
  // The worker is parked, so the accepted creation cannot have run yet.
  TEST_ASSERT_FALSE_MESSAGE(
    weak.expired(), "the parent retired before an accepted child creation"
  );
  worker.release = true;
  mln_test_pulse();
  TEST_ASSERT_TRUE(expired(weak));
  TEST_ASSERT_TRUE(released(result));
  MLN_TEST_INVALID_STATE(result.status.load());
}

// An operation that stays pending after its run-loop task returns, the way an
// asynchronous database operation does, until the case completes it.
auto submit_pending_operation(
  mln_runtime runtime, const mln_completion& completion,
  std::atomic_bool& entered,
  std::shared_ptr<mln::core::OperationObject>& out_operation
) -> mln_status {
  auto live = mln::core::lease_runtime(runtime);
  auto pending = mln::core::CompletionOperation{};
  const auto created = mln::core::create_completion_operation(
    &completion,
    mln::core::valueless_completion<&mln_runtime_clear_resource_transform>(),
    pending
  );
  if (created != MLN_STATUS_OK) return created;
  const auto status = mln::core::submit_runtime_operation(
    live, pending.operation,
    [&entered] {
      entered = true;
      mln_test_pulse();
    },
    pending.completion
  );
  if (status == MLN_STATUS_OK) out_operation = pending.operation;
  return status;
}

void a_pending_operation_keeps_only_its_runtime_alive() {
  const auto pending_runtime = create_runtime();
  const auto ready_runtime = create_runtime();
  auto pending_weak = std::weak_ptr{mln::core::lease_runtime(pending_runtime)};
  auto ready_weak = std::weak_ptr{mln::core::lease_runtime(ready_runtime)};
  auto result = Result{};
  const auto completion = descriptor(result);
  auto operation = std::shared_ptr<mln::core::OperationObject>{};
  auto entered = std::atomic_bool{false};
  MLN_TEST_OK(
    submit_pending_operation(pending_runtime, completion, entered, operation)
  );
  TEST_ASSERT_TRUE(await([&] { return entered.load(); }, "the operation"));
  MLN_TEST_OK(mln_runtime_dispose(pending_runtime, nullptr));
  MLN_TEST_OK(mln_runtime_dispose(ready_runtime, nullptr));
  TEST_ASSERT_TRUE(expired(ready_weak));
  TEST_ASSERT_FALSE_MESSAGE(
    pending_weak.expired(), "a runtime retired before its pending operation"
  );
  operation->complete(MLN_STATUS_OK, {}, {});
  operation.reset();
  TEST_ASSERT_TRUE(expired(pending_weak));
  TEST_ASSERT_EQUAL_UINT(1, result.releases.load());
  MLN_TEST_OK(result.status.load());
}

void dropping_a_pending_operation_cancels_and_retires_its_runtime() {
  const auto runtime = create_runtime();
  auto weak = std::weak_ptr{mln::core::lease_runtime(runtime)};
  auto result = Result{};
  const auto completion = descriptor(result);
  auto operation = std::shared_ptr<mln::core::OperationObject>{};
  auto entered = std::atomic_bool{false};
  MLN_TEST_OK(
    submit_pending_operation(runtime, completion, entered, operation)
  );
  TEST_ASSERT_TRUE(await([&] { return entered.load(); }, "the operation"));
  auto closed = mln_test_completion_default(0);
  MLN_TEST_OK(mln_runtime_release(runtime, &closed.descriptor, nullptr));
  operation.reset();
  MLN_TEST_OK(mln_test_completion_settle(&closed));
  TEST_ASSERT_TRUE(expired(weak));
  TEST_ASSERT_TRUE(released(result));
  MLN_TEST_STATUS(MLN_STATUS_CANCELLED, result.status.load());
}

void disposal_retires_an_attached_graph_after_driver_quiescence() {
  const auto runtime = create_runtime();
  const auto map = create_map(runtime);
  auto runtime_weak = std::weak_ptr{mln::core::lease_runtime(runtime)};
  auto map_weak =
    std::weak_ptr{mln::core::handle_table<mln::core::MapObject>().lease(map)};
  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  auto options = mln_render_session_attach_options_default();
  options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
  auto capabilities = mln_render_session_capabilities{};
  capabilities.size = sizeof(capabilities);
  auto attach = Result{};
  const auto completion = descriptor(attach);
  auto handle = mln_render_session{MLN_HANDLE_NULL};
  MLN_TEST_OK(
    mln::core::start_attach_render_session(
      session, mln::core::RenderSessionKind::Surface, &options, capabilities,
      &handle, &completion, surface_attach_completion
    )
  );
  TEST_ASSERT_TRUE(released(attach));
  MLN_TEST_OK(attach.status.load());
  auto weak = std::weak_ptr{session};
  auto driver = WorkerGate{};
  auto blocked = Result{};
  const auto blocked_completion = descriptor(blocked);
  MLN_TEST_OK(
    mln::core::enqueue_driver_operation(
      handle,
      [&driver](mln_render_session_object&) {
        driver.entered = true;
        mln_test_pulse();
        static_cast<void>(
          await([&] { return driver.release.load(); }, "release")
        );
        return MLN_STATUS_OK;
      },
      &blocked_completion, driver_work_completion
    )
  );
  TEST_ASSERT_TRUE(await([&] { return driver.entered.load(); }, "the driver"));
  auto* runtime_token = mln_binding_owner_token_create(runtime);
  auto* map_token = mln_binding_owner_token_create(map);
  auto* session_token = mln_binding_owner_token_create(handle);
  TEST_ASSERT_NOT_NULL(runtime_token);
  TEST_ASSERT_NOT_NULL(map_token);
  TEST_ASSERT_NOT_NULL(session_token);
  mln_binding_owner_finalize(runtime_token);
  mln_binding_owner_finalize(map_token);
  mln_binding_owner_finalize(session_token);
  TEST_ASSERT_NULL_MESSAGE(
    mln::core::lease_render_session(handle).get(),
    "a disposed session stayed callable"
  );
  // The driver is parked, so nothing below it may retire.
  TEST_ASSERT_FALSE_MESSAGE(
    map_weak.expired(), "the map retired while its driver was active"
  );
  TEST_ASSERT_FALSE_MESSAGE(
    runtime_weak.expired(), "the runtime retired while its driver was active"
  );
  session.reset();
  driver.release = true;
  mln_test_pulse();
  TEST_ASSERT_TRUE(await(
    [&] {
      return weak.expired() && map_weak.expired() && runtime_weak.expired();
    },
    "the graph to retire"
  ));
  TEST_ASSERT_EQUAL_UINT(1, blocked.releases.load());
  MLN_TEST_OK(blocked.status.load());
}

void flag_wake_release(void* context) {
  static_cast<std::atomic_bool*>(context)->store(true);
  mln_test_pulse();
}

// Attaches the preset's render fixture with a frame wake whose release sets
// `released`, which happens once disposal has retired the session.
void attach_observed(
  mln_map map, mln_test_render_fixture& fixture, std::atomic_bool& released
) {
  auto options = mln_render_session_attach_options_default();
  options.requested_texture_ring_depth = 2;
  options.frame_wake = mln_wake{
    .callback = [](void*) {},
    .user_data = &released,
    .release_user_data = flag_wake_release,
  };
  auto attach = mln_test_completion_default(0);
  MLN_TEST_OK(mln_test_render_fixture_start_attach(
    map, &options, &attach.descriptor, &fixture
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &attach));
  mln_test_completion_destroy(&attach);
}

auto wake_released(const std::atomic_bool& released) -> bool {
  return await([&] { return released.load(); }, "the wake release");
}

// Disposing an attached core-worker session detaches it on its worker after
// the call in flight, which frees its renderer rather than quarantining it.
// Work queued behind that call completes as abandoned. A caller driver never
// runs again after disposal, so its session is abandoned instead.
void disposal_detaches_an_attached_core_worker_session() {
  auto sync_points = SyncPointScope{};
  const auto runtime = mln_test_create_runtime();
  const auto map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(map);
  auto released = std::atomic_bool{false};
  auto fixture = mln_test_render_fixture{};
  attach_observed(map, fixture, released);
  mln_test_render_request_forced(&fixture, 1);
  mln_render_frame_batch_release(mln_test_render_wait_for_results(&fixture));
  const auto core_worker = fixture.driver == MLN_RENDER_DRIVER_CORE_WORKER;
  auto blocker = DriverBlocker{};
  MLN_TEST_OK(blocker.submit(fixture.session));
  if (core_worker)
    TEST_ASSERT_TRUE(mln_test_gate_wait_entered(blocker.gate.get()));
  auto queued = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_reduce_memory_use(
    fixture.session, &queued.descriptor, nullptr
  ));
  MLN_TEST_OK(mln_render_session_dispose(fixture.session, nullptr));
  mln_test_gate_release(blocker.gate.get());
  MLN_TEST_STATUS(
    core_worker ? MLN_STATUS_OK : MLN_STATUS_TARGET_LOST,
    mln_test_completion_settle(&blocker.completion)
  );
  blocker.submitted = false;
  MLN_TEST_STATUS(MLN_STATUS_TARGET_LOST, mln_test_completion_settle(&queued));
  TEST_ASSERT_TRUE(wake_released(released));
  TEST_ASSERT_EQUAL_INT(
    core_worker ? 1 : 0, sync_points.hits(SyncPoint::RenderDisposalDetached)
  );
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A destroy that leased its session before a dispose released the handle finds
// the handle released, without waiting for the session's driver. Disposal
// starts no abandonment when the session's detach finishes first, so a destroy
// that waited for one could wait forever.
void a_destroy_that_races_dispose_finds_the_handle_released() {
  auto sync_points = SyncPointScope{};
  const auto runtime = mln_test_create_runtime();
  const auto map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(map);
  auto released = std::atomic_bool{false};
  auto fixture = mln_test_render_fixture{};
  attach_observed(map, fixture, released);
  const auto core_worker = fixture.driver == MLN_RENDER_DRIVER_CORE_WORKER;
  // The detach queues behind the blocker, so the session stays detaching.
  auto blocker = DriverBlocker{};
  MLN_TEST_OK(blocker.submit(fixture.session));
  if (core_worker)
    TEST_ASSERT_TRUE(mln_test_gate_wait_entered(blocker.gate.get()));
  auto detach = mln_test_completion_default(0);
  MLN_TEST_OK(
    mln_render_session_detach(fixture.session, &detach.descriptor, nullptr)
  );
  sync_points.hold(SyncPoint::RenderDestroyLeased);
  auto destroyed = std::atomic_int{MLN_STATUS_OK};
  auto destroyer = std::thread{[&] {
    destroyed = mln_render_session_release(fixture.session, nullptr);
  }};
  TEST_ASSERT_TRUE(
    sync_points.wait_for_hits(SyncPoint::RenderDestroyLeased, 1)
  );
  MLN_TEST_OK(mln_render_session_dispose(fixture.session, nullptr));
  sync_points.release(SyncPoint::RenderDestroyLeased);
  destroyer.join();
  MLN_TEST_STATUS(MLN_STATUS_INVALID_STATE, destroyed.load());
  mln_test_gate_release(blocker.gate.get());
  static_cast<void>(mln_test_completion_settle(&blocker.completion));
  blocker.submitted = false;
  static_cast<void>(mln_test_completion_settle(&detach));
  TEST_ASSERT_TRUE(wake_released(released));
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A session whose frame the host still holds keeps its target in the host's
// hands, so disposal abandons it on every driver. The abandon keeps the ring,
// and a kept Vulkan ring is a child of the host's device.
void disposal_abandons_a_session_with_an_acquired_frame() {
  auto sync_points = SyncPointScope{};
  const auto runtime = mln_test_create_runtime();
  const auto map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(map);
  auto released = std::atomic_bool{false};
  auto fixture = mln_test_render_fixture{};
  attach_observed(map, fixture, released);
  auto frame = mln_test_render_and_acquire(&fixture, 1);
  MLN_TEST_OK(mln_render_session_dispose(fixture.session, nullptr));
  MLN_TEST_OK(mln_acquired_frame_dispose(frame, nullptr));
  TEST_ASSERT_TRUE(wake_released(released));
  TEST_ASSERT_EQUAL_INT(0, sync_points.hits(SyncPoint::RenderDisposalDetached));
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  mln_test_render_fixture_keep_graphics_until_exit(&fixture);
#endif
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A finalizer can run while the process exits, when nothing waits for the
// wake releases, so a finalized core-worker session keeps its graphics
// resources rather than detaching through graphics calls. Kept Vulkan objects
// are children of the host's device.
void finalization_abandons_an_attached_session() {
  auto sync_points = SyncPointScope{};
  const auto runtime = mln_test_create_runtime();
  const auto map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(map);
  auto released = std::atomic_bool{false};
  auto fixture = mln_test_render_fixture{};
  attach_observed(map, fixture, released);
  mln_test_render_request_forced(&fixture, 1);
  mln_render_frame_batch_release(mln_test_render_wait_for_results(&fixture));
  auto* const token = mln_binding_owner_token_create(fixture.session);
  TEST_ASSERT_NOT_NULL(token);
  mln_binding_owner_finalize(token);
  TEST_ASSERT_TRUE(wake_released(released));
  TEST_ASSERT_EQUAL_INT(0, sync_points.hits(SyncPoint::RenderDisposalDetached));
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  mln_test_render_fixture_keep_graphics_until_exit(&fixture);
#endif
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// An attached session with one acquired frame, standing in for a backend that
// rendered into a texture ring.
struct FakeTextureSession {
  std::shared_ptr<mln_render_session_object> session;
  mln_acquired_frame frame = MLN_HANDLE_NULL;
};

auto attach_fake_texture_session(mln_map map) -> FakeTextureSession {
  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  session->self = mln::core::register_render_session(session);
  session->state = MLN_RENDER_SESSION_STATE_ATTACHED;
  session->attached = true;
  session->acquired_frame_count = 1;
  MLN_TEST_OK(
    mln::core::map_attach_render_target_session(
      map, session.get(), session->map_object
    )
  );
  auto frame = std::make_shared<mln_acquired_frame_object>();
  frame->session = session;
  const auto frame_id =
    mln::core::handle_table<mln_acquired_frame_object>().insert(frame);
  return {.session = std::move(session), .frame = frame_id};
}

auto session_state(const mln_render_session_object& session) -> uint32_t {
  const auto lock = std::scoped_lock{session.control_mutex};
  return session.state;
}

// A view keeps its session busy: abandon reports busy until the view ends,
// and a release from work that the abandon delivers is busy rather than
// waiting for its own abandonment.
void a_view_and_its_own_abandonment_keep_a_session_busy() {
  const auto runtime = create_runtime();
  const auto map = create_map(runtime);
  auto fake = attach_fake_texture_session(map);
  const auto session = fake.session->self;
  auto reentrant_destroy_status = std::atomic_int{MLN_STATUS_OK};
  fake.session->driver_work.push_back(
    mln::core::RenderDriverWork{
      .execute = {},
      .abandon = [session, &reentrant_destroy_status] {
        reentrant_destroy_status = mln_render_session_release(session, nullptr);
      },
    }
  );
  void* scope = nullptr;
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_acquired_frame_view_begin(fake.frame, &scope, nullptr);
  });
  auto result =
    mln_render_abandon_result{sizeof(mln_render_abandon_result), 0, 0, 0};
  MLN_TEST_STATUS(
    MLN_STATUS_BUSY, mln_render_session_abandon(session, &result, nullptr)
  );
  mln_acquired_frame_view_end(scope);
  auto sync = mln_gpu_sync_default();
  MLN_TEST_OK(mln_acquired_frame_release(fake.frame, &sync, nullptr));
  MLN_TEST_OK(mln_render_session_abandon(session, &result, nullptr));
  MLN_TEST_OK(mln_render_session_release(session, nullptr));
  TEST_ASSERT_EQUAL_INT_MESSAGE(
    MLN_STATUS_BUSY, reentrant_destroy_status.load(),
    "an abandonment callback waited for its own retirement"
  );
  MLN_TEST_OK(mln_map_dispose(map, nullptr));
  MLN_TEST_OK(mln_runtime_dispose(runtime, nullptr));
}

// Every session retires on one lane, so a held view must park its session's
// retirements rather than block the lane.
void a_held_view_parks_its_retirements_without_stalling_others() {
  const auto runtime = create_runtime();
  const auto held_map = create_map(runtime);
  const auto other_map = create_map(runtime);
  auto held = attach_fake_texture_session(held_map);
  auto other = attach_fake_texture_session(other_map);
  auto held_weak = std::weak_ptr{held.session};
  auto other_weak = std::weak_ptr{other.session};
  void* scope = nullptr;
  MLN_TEST_OK(mln_acquired_frame_view_begin(held.frame, &scope, nullptr));
  MLN_TEST_OK(mln_acquired_frame_dispose(held.frame, nullptr));
  MLN_TEST_OK(mln_render_session_dispose(held.session->self, nullptr));
  MLN_TEST_OK(mln_acquired_frame_dispose(other.frame, nullptr));
  MLN_TEST_OK(mln_render_session_dispose(other.session->self, nullptr));
  other.session.reset();
  // The lane runs in submission order, so the other session retiring means
  // the held session's retirement already ran.
  TEST_ASSERT_TRUE(expired(other_weak));
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(
    MLN_RENDER_SESSION_STATE_ATTACHED, session_state(*held.session),
    "disposal abandoned a target under a borrowed view"
  );
  held.session.reset();
  mln_acquired_frame_view_end(scope);
  TEST_ASSERT_TRUE(expired(held_weak));
  MLN_TEST_OK(mln_map_dispose(held_map, nullptr));
  MLN_TEST_OK(mln_map_dispose(other_map, nullptr));
  MLN_TEST_OK(mln_runtime_dispose(runtime, nullptr));
}

// A core worker's driver call parks its session's retirement the same way.
void a_running_driver_call_parks_its_retirement_without_stalling_others() {
  const auto runtime = create_runtime();
  const auto busy_map = create_map(runtime);
  const auto other_map = create_map(runtime);
  auto busy = std::make_shared<mln_render_session_object>();
  busy->map = busy_map;
  auto options = mln_render_session_attach_options_default();
  options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
  auto capabilities = mln_render_session_capabilities{};
  capabilities.size = sizeof(capabilities);
  auto attach = Result{};
  const auto attach_completion = descriptor(attach);
  auto busy_id = mln_render_session{MLN_HANDLE_NULL};
  MLN_TEST_OK(
    mln::core::start_attach_render_session(
      busy, mln::core::RenderSessionKind::Surface, &options, capabilities,
      &busy_id, &attach_completion, surface_attach_completion
    )
  );
  TEST_ASSERT_TRUE(released(attach));
  MLN_TEST_OK(attach.status.load());
  auto busy_weak = std::weak_ptr{busy};
  busy.reset();
  auto driver = WorkerGate{};
  auto blocked = Result{};
  const auto blocked_completion = descriptor(blocked);
  MLN_TEST_OK(
    mln::core::enqueue_driver_operation(
      busy_id,
      [&driver](mln_render_session_object&) {
        driver.entered = true;
        mln_test_pulse();
        static_cast<void>(
          await([&] { return driver.release.load(); }, "release")
        );
        return MLN_STATUS_OK;
      },
      &blocked_completion, driver_work_completion
    )
  );
  TEST_ASSERT_TRUE(await([&] { return driver.entered.load(); }, "the driver"));
  auto other = attach_fake_texture_session(other_map);
  auto other_weak = std::weak_ptr{other.session};
  MLN_TEST_OK(mln_render_session_dispose(busy_id, nullptr));
  MLN_TEST_OK(mln_acquired_frame_dispose(other.frame, nullptr));
  MLN_TEST_OK(mln_render_session_dispose(other.session->self, nullptr));
  other.session.reset();
  TEST_ASSERT_TRUE(expired(other_weak));
  TEST_ASSERT_FALSE_MESSAGE(
    busy_weak.expired(), "a session retired under its driver call"
  );
  driver.release = true;
  mln_test_pulse();
  TEST_ASSERT_TRUE(expired(busy_weak));
  TEST_ASSERT_TRUE(released(blocked));
  MLN_TEST_OK(blocked.status.load());
  MLN_TEST_OK(mln_map_dispose(busy_map, nullptr));
  MLN_TEST_OK(mln_map_dispose(other_map, nullptr));
  MLN_TEST_OK(mln_runtime_dispose(runtime, nullptr));
}

// So does a call that services a caller-driven session's driver work.
void a_serviced_driver_call_parks_its_retirement_without_stalling_others() {
  const auto runtime = create_runtime();
  const auto busy_map = create_map(runtime);
  const auto other_map = create_map(runtime);
  auto busy = std::make_shared<mln_render_session_object>();
  busy->map = busy_map;
  auto options = mln_render_session_attach_options_default();
  options.driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
  auto capabilities = mln_render_session_capabilities{};
  capabilities.size = sizeof(capabilities);
  auto attach = Result{};
  const auto attach_completion = descriptor(attach);
  auto busy_id = mln_render_session{MLN_HANDLE_NULL};
  MLN_TEST_OK(
    mln::core::start_attach_render_session(
      busy, mln::core::RenderSessionKind::Surface, &options, capabilities,
      &busy_id, &attach_completion, surface_attach_completion
    )
  );
  auto busy_weak = std::weak_ptr{busy};
  busy.reset();
  // Driver work belongs to the thread that first services it, so one helper
  // thread services both the attachment and the blocking operation.
  auto checks = BackgroundChecks{};
  auto serve = std::atomic_bool{false};
  auto attachment_serviced = std::atomic_bool{false};
  auto graphics = std::thread{[&] {
    checks.check(
      mln_render_session_service_driver_work(busy_id, nullptr) == MLN_STATUS_OK,
      "servicing the attachment failed"
    );
    attachment_serviced = true;
    mln_test_pulse();
    checks.check(
      await([&] { return serve.load(); }, "the operation to service"),
      "the operation was never enqueued"
    );
    checks.check(
      mln_render_session_service_driver_work(busy_id, nullptr) == MLN_STATUS_OK,
      "servicing the operation failed"
    );
  }};
  const auto attached = await(
                          [&] { return attachment_serviced.load(); },
                          "the attachment service to return"
                        ) &&
                        released(attach);
  auto driver = WorkerGate{};
  auto blocked = Result{};
  const auto blocked_completion = descriptor(blocked);
  const auto enqueued = mln::core::enqueue_driver_operation(
    busy_id,
    [&driver](mln_render_session_object&) {
      driver.entered = true;
      mln_test_pulse();
      static_cast<void>(
        await([&] { return driver.release.load(); }, "release")
      );
      return MLN_STATUS_OK;
    },
    &blocked_completion, driver_work_completion
  );
  serve = true;
  mln_test_pulse();
  const auto entered =
    await([&] { return driver.entered.load(); }, "the driver call");
  auto other = attach_fake_texture_session(other_map);
  auto other_weak = std::weak_ptr{other.session};
  const auto busy_disposed = mln_render_session_dispose(busy_id, nullptr);
  const auto frame_disposed = mln_acquired_frame_dispose(other.frame, nullptr);
  const auto other_disposed =
    mln_render_session_dispose(other.session->self, nullptr);
  other.session.reset();
  const auto other_retired = expired(other_weak);
  const auto busy_retired_early = busy_weak.expired();
  // Assertions wait until the helper thread joins, so a failure never leaves
  // it parked on this frame's state.
  driver.release = true;
  mln_test_pulse();
  graphics.join();
  TEST_ASSERT_TRUE(attached);
  MLN_TEST_OK(attach.status.load());
  MLN_TEST_OK(enqueued);
  TEST_ASSERT_TRUE(entered);
  MLN_TEST_OK(busy_disposed);
  MLN_TEST_OK(frame_disposed);
  MLN_TEST_OK(other_disposed);
  TEST_ASSERT_NULL_MESSAGE(checks.failure(), checks.failure());
  TEST_ASSERT_TRUE(other_retired);
  TEST_ASSERT_FALSE_MESSAGE(
    busy_retired_early, "a session retired under its driver call"
  );
  TEST_ASSERT_TRUE(expired(busy_weak));
  TEST_ASSERT_TRUE(released(blocked));
  MLN_TEST_OK(blocked.status.load());
  MLN_TEST_OK(mln_map_dispose(busy_map, nullptr));
  MLN_TEST_OK(mln_map_dispose(other_map, nullptr));
  MLN_TEST_OK(mln_runtime_dispose(runtime, nullptr));
}

// A query that fails admission returns its map lease, so the map's release
// still completes.
void map_query_admission_failure_releases_its_map_lease() {
  for (const auto query : {mln_map_get_camera, mln_map_get_style_url}) {
    const auto runtime = create_runtime();
    auto creation = mln_test_completion_default(sizeof(mln_map));
    MLN_TEST_OK(
      mln_runtime_create_map(runtime, nullptr, &creation.descriptor, nullptr)
    );
    auto map = mln_map{MLN_HANDLE_NULL};
    MLN_TEST_OK(mln_test_completion_finish_value(&creation, &map, sizeof(map)));
    auto transition = mln_test_transition_end{};
    mln_test_transition_end_init(&transition);
    auto camera = mln_camera_update_default();
    camera.mode = MLN_CAMERA_UPDATE_MODE_EASE;
    camera.camera.fields = MLN_CAMERA_OPTION_ZOOM;
    camera.camera.zoom = 6;
    camera.animation.fields = MLN_ANIMATION_OPTION_DURATION;
    camera.animation.duration_ms = 60000;
    camera.animation.end_handler = mln_test_transition_end_handler(&transition);
    MLN_TEST_AWAIT_OK(
      mln_map_update_camera(map, &camera, &completion.descriptor, nullptr)
    );
    const auto discard = mln_test_discard_completion();
    auto status = MLN_STATUS_OK;
    {
      const auto fault = AllocationFaults{};
      status = query(map, &discard, nullptr);
    }
    auto release = mln_test_completion_default(0);
    MLN_TEST_OK(mln_map_release(map, &release.descriptor, nullptr));
    TEST_ASSERT_TRUE(mln_test_wait_transition_end(&transition));
    MLN_TEST_OK_MESSAGE(
      mln_test_completion_settle(&release),
      "a rejected map query leaked its submission lease"
    );
    MLN_TEST_OK(mln_test_runtime_close(runtime));
    MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, status);
  }
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(map_query_admission_failure_releases_its_map_lease);

  RUN_TEST(discarded_commands_keep_the_graph_alive_until_the_worker_drains);
  RUN_TEST(a_view_and_its_own_abandonment_keep_a_session_busy);
  RUN_TEST(a_held_view_parks_its_retirements_without_stalling_others);
  RUN_TEST(a_running_driver_call_parks_its_retirement_without_stalling_others);
  RUN_TEST(a_serviced_driver_call_parks_its_retirement_without_stalling_others);
  RUN_TEST(disposal_retires_an_attached_graph_after_driver_quiescence);
  RUN_TEST(disposal_detaches_an_attached_core_worker_session);
  RUN_TEST(disposal_abandons_a_session_with_an_acquired_frame);
  RUN_TEST(a_destroy_that_races_dispose_finds_the_handle_released);
  RUN_TEST(finalization_abandons_an_attached_session);
  RUN_TEST(disposal_drains_queued_work_and_map_cleanup);
  RUN_TEST(failed_creation_releases_parent_reservation);
  RUN_TEST(disposal_waits_for_pending_child_creation);
  RUN_TEST(a_pending_operation_keeps_only_its_runtime_alive);
  RUN_TEST(dropping_a_pending_operation_cancels_and_retires_its_runtime);
}
