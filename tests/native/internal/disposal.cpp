// Disposal: a finalizer's release path, which must succeed without allocating
// and must retire the graph only once nothing still runs against it.

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>

#include "completion/completion.hpp"
#include "internal/support/allocation_faults.hpp"
#include "internal/support/checks.hpp"
#include "internal/support/sync_points.hpp"
#include "map/map.hpp"
#include "map/map_internal.hpp"
#include "maplibre_native_c.h"
#include "maplibre_native_c/callback_adapter.h"
#include "operation/operation.hpp"
#include "render/render_session_common.hpp"
#include "runtime/runtime.hpp"
#include "support/harness.h"
#include "support/status.h"
#include "support/wait.h"
#include "unity.h"

namespace {

using mln::native_tests::AllocationFaults;
using mln::native_tests::await;
using mln::native_tests::BackgroundChecks;
using mln::native_tests::SyncPoint;
using mln::native_tests::SyncPointScope;

// The completions that the fake surface attachments and the blocking driver
// work below stand in for. Any surface attachment and any maintenance request
// serves, because each one delivers no value.
constexpr auto surface_attach_completion =
  mln::core::valueless_completion<&mln_metal_surface_attach>();
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

auto create_runtime(std::atomic_uint* wake_releases = nullptr) -> mln_runtime {
  auto options = mln_runtime_options_default();
  if (wake_releases != nullptr) {
    options.event_wake.callback = [](void*) {};
    options.event_wake.user_data = wake_releases;
    options.event_wake.release_user_data = [](void* context) {
      ++*static_cast<std::atomic_uint*>(context);
      mln_test_pulse();
    };
  }
  auto runtime = mln_runtime{MLN_HANDLE_NULL};
  MLN_TEST_OK(mln_runtime_create(&options, &runtime, nullptr));
  return runtime;
}

auto create_map(mln_runtime runtime) -> mln_map {
  auto result = Result{};
  const auto completion = descriptor(result);
  auto options = mln_map_options_default();
  options.map_mode = MLN_MAP_MODE_STATIC;
  MLN_TEST_OK(mln_map_create(runtime, &options, &completion, nullptr));
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

void runtime_barriers_observe_retired_command_captures() {
  struct Probe {
    std::atomic_bool retired = false;
    std::atomic_bool observed_retirement = false;
    std::atomic_bool barrier_released = false;
    std::atomic_int nested_status = MLN_STATUS_NATIVE_ERROR;
  } probe;
  struct Capture {
    Probe& probe;
    ~Capture() { probe.retired.store(true); }
  };
  auto runtime = create_runtime();
  auto live = mln::core::lease_runtime(runtime);
  auto result = Result{};
  auto completion = std::make_shared<mln::core::Completion>(descriptor(result));
  auto capture = std::shared_ptr<Capture>(new Capture{probe});
  MLN_TEST_OK(
    mln::core::submit_runtime_command(
      live,
      [runtime, completion, capture = std::move(capture), &probe](uint64_t) {
        const mln_completion barrier = {
          .size = sizeof(mln_completion),
          .callback =
            [](void* context, const mln_completion_result* value) {
              auto& probe = *static_cast<Probe*>(context);
              probe.observed_retirement.store(
                value->status == MLN_STATUS_OK && probe.retired.load()
              );
            },
          .user_data = &probe,
          .release_user_data =
            [](void* context) {
              static_cast<Probe*>(context)->barrier_released.store(true);
              mln_test_pulse();
            },
        };
        probe.nested_status = mln_runtime_barrier(runtime, &barrier, nullptr);
        mln::core::complete_command(
          completion, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK
        );
      },
      completion
    )
  );
  TEST_ASSERT_TRUE(
    await([&] { return probe.barrier_released.load(); }, "the barrier")
  );
  MLN_TEST_OK(probe.nested_status.load());
  TEST_ASSERT_TRUE_MESSAGE(
    probe.observed_retirement.load(),
    "the runtime barrier completed before the command's captures retired"
  );
  TEST_ASSERT_TRUE(released(result));
  live.reset();
  auto closed = Result{};
  const auto close = descriptor(closed);
  MLN_TEST_OK(mln_runtime_release(runtime, &close, nullptr));
  TEST_ASSERT_TRUE(released(closed));
}

struct UnclaimedCreation {
  Result result;
  BackgroundChecks checks;
};

// A creation completion that discards its map disposes it on the callback
// thread, where allocation may fail.
void unclaimed_creation_disposes_on_the_callback_thread() {
  std::atomic_uint wake_releases = 0;
  const auto runtime = create_runtime(&wake_releases);
  auto runtime_weak = std::weak_ptr{mln::core::lease_runtime(runtime)};
  auto creation = UnclaimedCreation{};
  auto completion = descriptor(creation.result);
  completion.user_data = &creation;
  completion.callback = [](void* context, const mln_completion_result* value) {
    auto& creation = *static_cast<UnclaimedCreation*>(context);
    creation.result.status = value->status;
    if (value->status != MLN_STATUS_OK) return;
    const auto map = *static_cast<const mln_map*>(value->value);
    const auto discarded = mln_completion{
      .size = sizeof(mln_completion),
      .callback = [](void*, const mln_completion_result*) {},
    };
    auto release_status = MLN_STATUS_OK;
    {
      const auto faults = AllocationFaults{};
      release_status = mln_map_release(map, &discarded, nullptr);
    }
    creation.checks.check(
      release_status == MLN_STATUS_NATIVE_ERROR,
      "the native allocator bypassed fault injection"
    );
    creation.checks.check(
      mln::native_tests::ok_without_allocations([&] {
        return mln_map_dispose(map, nullptr);
      }),
      "map disposal failed with allocations disabled"
    );
  };
  completion.release_user_data = [](void* context) {
    ++static_cast<UnclaimedCreation*>(context)->result.releases;
    mln_test_pulse();
  };
  MLN_TEST_OK(mln_map_create(runtime, nullptr, &completion, nullptr));
  TEST_ASSERT_TRUE(released(creation.result));
  MLN_TEST_OK(creation.result.status.load());
  TEST_ASSERT_NULL_MESSAGE(
    creation.checks.failure(), creation.checks.failure()
  );
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_runtime_dispose(runtime, nullptr);
  });
  TEST_ASSERT_TRUE(expired(runtime_weak));
  TEST_ASSERT_TRUE(
    await([&] { return wake_releases.load() == 1; }, "the event wake release")
  );
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
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_runtime_dispose(runtime, nullptr);
  });
  TEST_ASSERT_NULL_MESSAGE(
    mln::core::lease_runtime(runtime).get(),
    "a disposed parent stayed publicly live"
  );
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_map_dispose(map, nullptr);
  });
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
    status = mln_map_create(runtime, nullptr, &completion, nullptr);
  }
  MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, status);
  TEST_ASSERT_EQUAL_UINT(0, result.releases.load());
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_runtime_dispose(runtime, nullptr);
  });
  TEST_ASSERT_TRUE(expired(weak));
}

void failed_finalizer_token_creation_disposes_the_owner() {
  const auto runtime = create_runtime();
  auto weak = std::weak_ptr{mln::core::lease_runtime(runtime)};
  void* token = nullptr;
  {
    const auto faults = AllocationFaults{};
    token = mln_adapter_owner_token_create(runtime);
  }
  TEST_ASSERT_NULL(token);
  TEST_ASSERT_TRUE(expired(weak));
}

void disposed_parent_allows_observed_child_release() {
  const auto runtime = create_runtime();
  const auto map = create_map(runtime);
  auto weak = std::weak_ptr{mln::core::lease_runtime(runtime)};
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_runtime_dispose(runtime, nullptr);
  });
  auto result = Result{};
  const auto completion = descriptor(result);
  MLN_TEST_OK(mln_map_release(map, &completion, nullptr));
  TEST_ASSERT_TRUE(expired(weak));
  TEST_ASSERT_TRUE(released(result));
  MLN_TEST_OK(result.status.load());
}

void disposal_waits_for_pending_child_creation() {
  const auto runtime = create_runtime();
  auto live = mln::core::lease_runtime(runtime);
  auto weak = std::weak_ptr{live};
  auto worker = WorkerGate{};
  park_worker(*live, worker);
  auto result = Result{};
  const auto completion = descriptor(result);
  MLN_TEST_OK(mln_map_create(runtime, nullptr, &completion, nullptr));
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_runtime_dispose(runtime, nullptr);
  });
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
    &completion, mln::core::valueless_completion<&mln_runtime_barrier>(),
    pending
  );
  if (created != MLN_STATUS_OK) return created;
  const auto status =
    mln::core::submit_runtime_operation(live, pending.operation, [&entered] {
      entered = true;
      mln_test_pulse();
    });
  if (status == MLN_STATUS_OK) {
    out_operation = pending.operation;
    pending.completion->accept();
  } else {
    pending.completion->reject();
  }
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
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_runtime_dispose(pending_runtime, nullptr);
  });
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_runtime_dispose(ready_runtime, nullptr);
  });
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
  auto* runtime_token = mln_adapter_owner_token_create(runtime);
  auto* map_token = mln_adapter_owner_token_create(map);
  auto* session_token = mln_adapter_owner_token_create(handle);
  TEST_ASSERT_NOT_NULL(runtime_token);
  TEST_ASSERT_NOT_NULL(map_token);
  TEST_ASSERT_NOT_NULL(session_token);
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    mln_adapter_owner_finalize(runtime_token);
    mln_adapter_owner_finalize(map_token);
    mln_adapter_owner_finalize(session_token);
    return MLN_STATUS_OK;
  });
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

void abandoned_frame_preserves_its_session_owner_without_synthesizing_gpu_sync() {
  const auto runtime = create_runtime();
  const auto map = create_map(runtime);
  auto runtime_weak = std::weak_ptr{mln::core::lease_runtime(runtime)};
  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  session->self = mln::core::register_render_session(session);
  session->state = MLN_RENDER_SESSION_STATE_ATTACHED;
  session->attached = true;
  session->acquired_frame_count = 1;
  MLN_TEST_OK(mln::core::map_attach_render_target_session(map, session.get()));
  auto frame = std::make_shared<mln_acquired_frame_object>();
  frame->session = session;
  const auto handle =
    mln::core::handle_table<mln_acquired_frame_object>().insert(frame);
  auto session_weak = std::weak_ptr{session};
  auto frame_weak = std::weak_ptr{frame};
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_runtime_dispose(runtime, nullptr);
  });
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_map_dispose(map, nullptr);
  });
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_acquired_frame_dispose(handle, nullptr);
  });
  TEST_ASSERT_NOT_NULL_MESSAGE(
    mln::core::lease_render_session(session->self).get(),
    "frame disposal consumed its independent session owner"
  );
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_render_session_dispose(session->self, nullptr);
  });
  frame.reset();
  session.reset();
  TEST_ASSERT_TRUE(await(
    [&] {
      return frame_weak.expired() && session_weak.expired() &&
             runtime_weak.expired();
    },
    "the graph to retire"
  ));
}

void borrowed_views_hold_the_session_through_sibling_disposal() {
  const auto runtime = create_runtime();
  const auto map = create_map(runtime);
  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  session->self = mln::core::register_render_session(session);
  session->state = MLN_RENDER_SESSION_STATE_ATTACHED;
  session->attached = true;
  session->acquired_frame_count = 2;
  MLN_TEST_OK(mln::core::map_attach_render_target_session(map, session.get()));
  auto frame = std::make_shared<mln_acquired_frame_object>();
  frame->session = session;
  auto frame_id =
    mln::core::handle_table<mln_acquired_frame_object>().insert(frame);
  auto sibling = std::make_shared<mln_acquired_frame_object>();
  sibling->session = session;
  const auto sibling_id =
    mln::core::handle_table<mln_acquired_frame_object>().insert(sibling);
  auto reentrant_destroy_status = std::atomic_int{MLN_STATUS_OK};
  session->driver_work.push_back(
    mln::core::RenderDriverWork{
      .execute = {},
      .abandon = [id = session->self, &reentrant_destroy_status] {
        reentrant_destroy_status = mln_render_session_destroy(id, nullptr);
      },
    }
  );
  void* scope = nullptr;
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_adapter_acquired_frame_view_begin(frame_id, &scope, nullptr);
  });
  auto result =
    mln_render_abandon_result{sizeof(mln_render_abandon_result), 0, 0, 0};
  MLN_TEST_STATUS(
    MLN_STATUS_BUSY, mln_render_session_abandon(session->self, &result, nullptr)
  );
  auto sync = mln_gpu_sync_default();
  MLN_TEST_STATUS(
    MLN_STATUS_BUSY, mln_acquired_frame_release(&frame_id, &sync, nullptr)
  );
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_map_dispose(map, nullptr);
  });
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_runtime_dispose(runtime, nullptr);
  });
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_acquired_frame_dispose(sibling_id, nullptr);
  });
  void* rejected = nullptr;
  MLN_TEST_STATUS(
    MLN_STATUS_TARGET_LOST,
    mln_adapter_acquired_frame_view_begin(frame_id, &rejected, nullptr)
  );
  TEST_ASSERT_NULL(rejected);
  auto state = uint32_t{};
  {
    const auto lock = std::scoped_lock{session->control_mutex};
    state = session->state;
  }
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(
    MLN_RENDER_SESSION_STATE_ATTACHED, state, "disposal retired an active view"
  );
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    mln_adapter_acquired_frame_view_end(scope);
    return MLN_STATUS_OK;
  });
  MLN_TEST_OK(mln_acquired_frame_release(&frame_id, &sync, nullptr));
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_render_session_destroy(session->self, nullptr);
  });
  TEST_ASSERT_EQUAL_INT_MESSAGE(
    MLN_STATUS_BUSY, reentrant_destroy_status.load(),
    "an abandonment callback waited for its own retirement"
  );
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
  MLN_TEST_OK(mln::core::map_attach_render_target_session(map, session.get()));
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
  MLN_TEST_OK(
    mln_adapter_acquired_frame_view_begin(held.frame, &scope, nullptr)
  );
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_acquired_frame_dispose(held.frame, nullptr);
  });
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_render_session_dispose(held.session->self, nullptr);
  });
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_acquired_frame_dispose(other.frame, nullptr);
  });
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_render_session_dispose(other.session->self, nullptr);
  });
  other.session.reset();
  // The lane runs in submission order, so the other session retiring means
  // both of the held session's retirements already ran.
  TEST_ASSERT_TRUE(expired(other_weak));
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(
    MLN_RENDER_SESSION_STATE_ATTACHED, session_state(*held.session),
    "disposal abandoned a target under a borrowed view"
  );
  held.session.reset();
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    mln_adapter_acquired_frame_view_end(scope);
    return MLN_STATUS_OK;
  });
  TEST_ASSERT_TRUE(expired(held_weak));
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_map_dispose(held_map, nullptr);
  });
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_map_dispose(other_map, nullptr);
  });
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_runtime_dispose(runtime, nullptr);
  });
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
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_render_session_dispose(busy_id, nullptr);
  });
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_acquired_frame_dispose(other.frame, nullptr);
  });
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_render_session_dispose(other.session->self, nullptr);
  });
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
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_map_dispose(busy_map, nullptr);
  });
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_map_dispose(other_map, nullptr);
  });
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_runtime_dispose(runtime, nullptr);
  });
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
  auto graphics = std::thread{[&] {
    auto serviced = std::size_t{0};
    checks.check(
      mln_render_session_service_driver_work(busy_id, 0, &serviced, nullptr) ==
        MLN_STATUS_OK,
      "servicing the attachment failed"
    );
    checks.check(
      await([&] { return serve.load(); }, "the operation to service"),
      "the operation was never enqueued"
    );
    checks.check(
      mln_render_session_service_driver_work(busy_id, 0, &serviced, nullptr) ==
        MLN_STATUS_OK,
      "servicing the operation failed"
    );
  }};
  const auto attached = released(attach);
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
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_map_dispose(busy_map, nullptr);
  });
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_map_dispose(other_map, nullptr);
  });
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_runtime_dispose(runtime, nullptr);
  });
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(runtime_barriers_observe_retired_command_captures);
  RUN_TEST(borrowed_views_hold_the_session_through_sibling_disposal);
  RUN_TEST(a_held_view_parks_its_retirements_without_stalling_others);
  RUN_TEST(a_running_driver_call_parks_its_retirement_without_stalling_others);
  RUN_TEST(a_serviced_driver_call_parks_its_retirement_without_stalling_others);
  RUN_TEST(failed_finalizer_token_creation_disposes_the_owner);
  RUN_TEST(disposal_retires_an_attached_graph_after_driver_quiescence);
  RUN_TEST(
    abandoned_frame_preserves_its_session_owner_without_synthesizing_gpu_sync
  );
  RUN_TEST(unclaimed_creation_disposes_on_the_callback_thread);
  RUN_TEST(disposal_drains_queued_work_and_map_cleanup);
  RUN_TEST(failed_creation_releases_parent_reservation);
  RUN_TEST(disposed_parent_allows_observed_child_release);
  RUN_TEST(disposal_waits_for_pending_child_creation);
  RUN_TEST(a_pending_operation_keeps_only_its_runtime_alive);
}
