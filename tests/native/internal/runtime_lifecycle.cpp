// Runtime release against a map's physical cleanup, which runs on a teardown
// lane after the map's own release has completed.

#include <cstdio>
#include <thread>
#include <vector>

#include "completion/completion.hpp"
#include "internal/support/allocation_faults.hpp"
#include "internal/support/sync_points.hpp"
#include "map/map.hpp"
#include "map/map_internal.hpp"
#include "maplibre_native_c.h"
#include "operation/operation.hpp"
#include "runtime/runtime.hpp"
#include "support/camera.h"
#include "support/test_support.h"

using mln::native_tests::AllocationFaults;

namespace {

using mln::native_tests::SyncPoint;
using mln::native_tests::SyncPointScope;

auto created(const mln_completion& descriptor)
  -> std::shared_ptr<mln::core::Completion> {
  auto completion = std::shared_ptr<mln::core::Completion>{};
  MLN_TEST_OK(mln::core::make_completion(&descriptor, completion));
  return completion;
}

auto create_runtime() -> mln_runtime {
  const auto options = mln_runtime_options_default();
  auto runtime = mln_runtime{MLN_HANDLE_NULL};
  MLN_TEST_OK(mln_runtime_create(&options, &runtime, nullptr));
  return runtime;
}

// An unstarted executor rejects queueing deterministically, without requiring
// allocation failure in a public submission.
void submission_failure_callbacks_can_submit_again() {
  using mln::core::OperationObject;
  using mln::core::RuntimeObject;
  auto runtime = std::make_shared<RuntimeObject>();
  const auto descriptor = mln_test_discard_completion();
  auto reentrant_status = MLN_STATUS_OK;
  const auto submit_again = [&]() {
    auto completion = created(descriptor);
    reentrant_status =
      mln::core::submit_runtime_command(runtime, [](uint64_t) {}, completion);
  };
  auto barrier =
    std::make_shared<OperationObject>([&](mln_status, std::string, std::any) {
      runtime->pending_submissions.erase(2);
      runtime->next_submission_sequence = 3;
      submit_again();
    });
  runtime->pending_submissions.insert(2);
  runtime->pending_barriers.emplace(2, barrier);
  auto completion = created(descriptor);
  MLN_TEST_STATUS(
    MLN_STATUS_NATIVE_ERROR,
    mln::core::submit_runtime_command(runtime, [](uint64_t) {}, completion)
  );
  MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, reentrant_status);
  TEST_ASSERT_TRUE(runtime->pending_submissions.empty());
  TEST_ASSERT_TRUE(runtime->pending_commands.empty());

  reentrant_status = MLN_STATUS_OK;
  auto operation =
    std::make_shared<OperationObject>([&](mln_status, std::string, std::any) {
      submit_again();
    });
  MLN_TEST_STATUS(
    MLN_STATUS_NATIVE_ERROR,
    mln::core::submit_runtime_operation(runtime, operation, [] {})
  );
  MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, reentrant_status);
  TEST_ASSERT_TRUE(runtime->pending_submissions.empty());
  TEST_ASSERT_TRUE(runtime->pending_commands.empty());

  auto rejected = mln_test_completion_default(0);
  auto pair = mln::core::CompletionOperation{};
  MLN_TEST_OK(
    mln::core::create_completion_operation(
      &rejected.descriptor,
      mln::core::valueless_completion<&mln_runtime_barrier>(), pair
    )
  );
  MLN_TEST_STATUS(
    MLN_STATUS_NATIVE_ERROR, mln::core::submit_runtime_operation(
                               runtime, pair.operation, [] {}, pair.completion
                             )
  );
  pair = {};
  mln_test_completion_reject(&rejected);
  mln_test_completion_destroy(&rejected);
  TEST_ASSERT_TRUE(runtime->pending_submissions.empty());
  TEST_ASSERT_TRUE(runtime->pending_commands.empty());
}

// A released map completes before its worker pool shuts down, and the runtime
// release waits for that shutdown. The case parks the shutdown and waits for
// the first runtime's release to report that it deferred retirement, which it
// reports only when it has to defer. A second runtime retires while the first
// waits for its map's cleanup.
void a_deferred_runtime_release_does_not_hold_other_retirements() {
  auto sync_points = SyncPointScope{};
  sync_points.hold(SyncPoint::MapPoolShutdown);
  const auto waiting = create_runtime();
  const auto other = create_runtime();

  auto create_map = mln_test_completion_default(sizeof(mln_map));
  MLN_TEST_OK(
    mln_runtime_create_map(waiting, nullptr, &create_map.descriptor, nullptr)
  );
  auto map = mln_map{MLN_HANDLE_NULL};
  MLN_TEST_OK(mln_test_completion_finish_value(&create_map, &map, sizeof(map)));
  auto map_close = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_release(map, &map_close.descriptor, nullptr));
  const auto cleanup_parked =
    sync_points.wait_for_hits(SyncPoint::MapPoolShutdown, 1);
  MLN_TEST_OK(mln_test_completion_settle(&map_close));

  auto waiting_close = mln_test_completion_default(0);
  const auto waiting_accepted =
    mln_runtime_release(waiting, &waiting_close.descriptor, nullptr);
  const auto waiting_deferred =
    sync_points.wait_for_hits(SyncPoint::RuntimeRetirementDeferred, 1);
  auto other_close = mln_test_completion_default(0);
  const auto other_accepted =
    mln_runtime_release(other, &other_close.descriptor, nullptr);
  const auto other_closed = mln_test_completion_wait(&other_close, -1);
  const auto waiting_closed_early = mln_test_completion_poll(&waiting_close);
  sync_points.release(SyncPoint::MapPoolShutdown);
  const auto other_status = mln_test_completion_settle(&other_close);
  const auto waiting_status = mln_test_completion_settle(&waiting_close);

  TEST_ASSERT_TRUE(cleanup_parked);
  MLN_TEST_OK(waiting_accepted);
  TEST_ASSERT_TRUE_MESSAGE(
    waiting_deferred,
    "the runtime release never deferred for a map's parked cleanup"
  );
  MLN_TEST_OK(other_accepted);
  TEST_ASSERT_TRUE_MESSAGE(
    other_closed,
    "a runtime release waited behind another runtime's deferred retirement"
  );
  TEST_ASSERT_FALSE_MESSAGE(
    waiting_closed_early,
    "the runtime release completed while a map's cleanup was parked"
  );
  MLN_TEST_OK(other_status);
  MLN_TEST_OK(waiting_status);
}

// Models a command that acquired its map lease before close and queued after
// cancellation. The transition's CLOSED callback fences the first close stage.
void map_release_waits_for_a_lease_queued_after_close_begins() {
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
  auto live = mln::core::handle_table<mln::core::MapObject>().lease(map);
  TEST_ASSERT_TRUE(live->control.acquire());
  auto lease = mln::core::ControlLease{&live->control};
  auto closed = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_release(map, &closed.descriptor, nullptr));
  TEST_ASSERT_TRUE(mln_test_wait_transition_end(&transition));
  TEST_ASSERT_FALSE(mln_test_completion_poll(&closed));
  auto barrier = mln_test_completion_default(0);
  MLN_TEST_OK(mln_runtime_barrier(runtime, &barrier.descriptor, nullptr));
  TEST_ASSERT_FALSE(mln_test_completion_poll(&barrier));
  MLN_TEST_INVALID_STATE(mln_map_dispose(map, nullptr));
  // Runtime disposal must preserve the worker for the already-leased command.
  MLN_TEST_OK(mln_runtime_dispose(runtime, nullptr));
  auto command = mln_test_completion_default(0);
  auto completion = created(command.descriptor);
  completion->accept();
  auto runtime_weak = std::weak_ptr{live->runtime_state};
  auto& executor = live->runtime_state->executor;
  executor.invoke([live = std::move(live), lease = std::move(lease),
                   completion]() mutable {
    const auto status =
      mln::core::map_set_style_json(*live, mln_test_empty_style_json);
    mln::core::complete_command(
      completion, MLN_COMMAND_DISPOSITION_COMMITTED, status
    );
  });
  MLN_TEST_OK(mln_test_completion_settle(&command));
  MLN_TEST_OK(mln_test_completion_settle(&closed));
  MLN_TEST_OK(mln_test_completion_settle(&barrier));
  TEST_ASSERT_TRUE(mln_test_await(
    [](void* context) {
      return static_cast<std::weak_ptr<mln::core::RuntimeObject>*>(context)
        ->expired();
    },
    &runtime_weak, mln_test_deadline_default(), "the runtime to retire"
  ));
}

void barrier_admission_failure_retires_its_sequence() {
  auto failures_with_pending = unsigned{0};
  for (auto index = std::size_t{0}; index != 12; ++index) {
    const auto runtime = create_runtime();
    auto live = mln::core::lease_runtime(runtime);
    auto completion = mln_test_completion_default(0);
    auto status = MLN_STATUS_OK;
    {
      const auto fault = AllocationFaults{index};
      status = mln_runtime_barrier(runtime, &completion.descriptor, nullptr);
    }
    auto pending = std::vector<uint64_t>{};
    {
      const auto lock = std::scoped_lock{live->terminal_mutex};
      pending.assign(
        live->pending_submissions.begin(), live->pending_submissions.end()
      );
    }
    if (status != MLN_STATUS_OK && !pending.empty()) {
      ++failures_with_pending;
      std::fprintf(
        stderr, "barrier: allocation %zu failed; %zu sequence remains\n", index,
        pending.size()
      );
      // Repair only the test's orphaned admission, so cleanup can finish.
      for (const auto sequence : pending)
        mln::core::finish_runtime_submission(live, sequence);
    }
    if (status == MLN_STATUS_OK)
      MLN_TEST_OK(mln_test_completion_settle(&completion));
    else {
      mln_test_completion_reject(&completion);
      mln_test_completion_destroy(&completion);
    }
    live.reset();
    MLN_TEST_OK(mln_test_runtime_close(runtime));
  }
  TEST_ASSERT_EQUAL_UINT_MESSAGE(
    0, failures_with_pending, "a rejected barrier stranded its runtime sequence"
  );
}

struct SubmissionProbe {
  mln_test_gate release;
};

void accepted_completions_keep_runtime_submissions_pending() {
  for (auto kind = 0; kind != 3; ++kind) {
    const auto runtime = create_runtime();
    auto live = mln::core::lease_runtime(runtime);
    auto probe = SubmissionProbe{};
    mln_test_gate_init(&probe.release);
    const auto descriptor = mln_completion{
      .size = sizeof(mln_completion),
      .callback = [](void*, const mln_completion_result*) {},
      .user_data = &probe,
      .release_user_data =
        [](void* context) {
          mln_test_gate_park(&static_cast<SubmissionProbe*>(context)->release);
        },
    };
    auto resolver = std::thread{};
    if (kind == 0) {
      auto completion = created(descriptor);
      MLN_TEST_OK(
        mln::core::submit_runtime_command(
          live,
          [completion](uint64_t) {
            mln::core::complete_command(
              completion, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK
            );
          },
          completion
        )
      );
    } else if (kind == 1) {
      auto pair = mln::core::CompletionOperation{};
      MLN_TEST_OK(
        mln::core::create_completion_operation(
          &descriptor, mln::core::valueless_completion<&mln_runtime_barrier>(),
          pair
        )
      );
      MLN_TEST_OK(
        mln::core::submit_runtime_operation(
          live, pair.operation, [] {}, pair.completion
        )
      );
      auto work_retired = std::atomic_bool{false};
      live->executor.invoke([&work_retired] {
        mln_test_flag_set(&work_retired);
      });
      TEST_ASSERT_TRUE(mln_test_wait_for_flag(&work_retired));
      resolver = std::thread{[operation = pair.operation] {
        operation->complete(MLN_STATUS_OK, {}, {});
      }};
    } else {
      MLN_TEST_OK(mln_runtime_barrier(runtime, &descriptor, nullptr));
    }
    const auto release_entered = mln_test_gate_wait_entered(&probe.release);
    auto tracked = false;
    {
      const auto lock = std::scoped_lock{live->terminal_mutex};
      tracked =
        live->pending_submissions.size() + live->pending_commands.size() == 1;
    }
    auto barrier = mln_test_completion_default(0);
    const auto barrier_status =
      mln_runtime_barrier(runtime, &barrier.descriptor, nullptr);
    auto closed = mln_test_completion_default(0);
    const auto close_status =
      mln_runtime_release(runtime, &closed.descriptor, nullptr);
    const auto barrier_early = mln_test_completion_poll(&barrier);
    const auto close_early = mln_test_completion_poll(&closed);
    mln_test_gate_release(&probe.release);
    if (resolver.joinable()) resolver.join();
    MLN_TEST_OK(mln_test_completion_settle(&barrier));
    MLN_TEST_OK(mln_test_completion_settle(&closed));
    TEST_ASSERT_TRUE(release_entered);
    TEST_ASSERT_TRUE(tracked);
    MLN_TEST_OK(barrier_status);
    MLN_TEST_OK(close_status);
    TEST_ASSERT_FALSE(barrier_early);
    TEST_ASSERT_FALSE(close_early);
  }
}

void operation_submission_waits_for_native_work_to_retire() {
  const auto runtime = create_runtime();
  auto live = mln::core::lease_runtime(runtime);
  auto work = mln_test_gate{};
  mln_test_gate_init(&work);
  auto completed = mln_test_completion_default(0);
  auto pair = mln::core::CompletionOperation{};
  MLN_TEST_OK(
    mln::core::create_completion_operation(
      &completed.descriptor,
      mln::core::valueless_completion<&mln_runtime_barrier>(), pair
    )
  );
  MLN_TEST_OK(
    mln::core::submit_runtime_operation(
      live, pair.operation,
      [operation = pair.operation, &work] {
        operation->complete(MLN_STATUS_OK, {}, {});
        mln_test_gate_park(&work);
      },
      pair.completion
    )
  );
  const auto work_running = mln_test_gate_wait_entered(&work);
  auto tracked = false;
  {
    const auto lock = std::scoped_lock{live->terminal_mutex};
    tracked =
      live->pending_submissions.size() + live->pending_commands.size() == 1;
  }
  mln_test_gate_release(&work);
  MLN_TEST_OK(mln_test_completion_settle(&completed));
  MLN_TEST_OK(mln_test_runtime_close(runtime));
  TEST_ASSERT_TRUE(work_running);
  TEST_ASSERT_TRUE_MESSAGE(
    tracked, "operation retired its sequence while native work was running"
  );
}

struct ThreadExitProbe {
  std::atomic_bool destroyed = false;
  std::atomic_bool checked = false;
  std::atomic_bool destroyed_at_completion = false;
};

struct ThreadExitSentinel {
  std::shared_ptr<ThreadExitProbe> probe;
  ~ThreadExitSentinel() {
    // Windows runs TLS destructors under the loader lock. Store without
    // acquiring the test waiter's locks or blocking another thread's exit.
    if (probe != nullptr) probe->destroyed.store(true);
  }
};

void a_blocked_worker_exit_does_not_hold_other_retirements() {
  const auto runtime = create_runtime();
  auto live = mln::core::lease_runtime(runtime);
  const auto weak = std::weak_ptr{live};
  struct ExitGate {
    std::atomic_bool claimed = false;
    mln_test_gate gate;
  };
  auto exit_gate = std::make_shared<ExitGate>();
  mln_test_gate_init(&exit_gate->gate);
  // Park before TLS cleanup: Windows serializes thread startup and TLS
  // destruction under the loader lock.
  mln::testing::set_sync_point_handler(
    [](SyncPoint point, void* context) noexcept {
      if (point != SyncPoint::RuntimeWorkerExiting) return;
      const auto owned = *static_cast<std::shared_ptr<ExitGate>*>(context);
      if (!owned->claimed.exchange(true)) mln_test_gate_park(&owned->gate);
    },
    &exit_gate
  );
  auto closed = mln_test_completion_default(0);
  MLN_TEST_OK(mln_runtime_release(runtime, &closed.descriptor, nullptr));
  live.reset();
  const auto exiting = mln_test_gate_wait_entered(&exit_gate->gate);
  const auto retained = !weak.expired();
  const auto completed_early = mln_test_completion_poll(&closed);
  const auto other = create_runtime();
  const auto other_status = mln_test_runtime_close(other);
  mln_test_gate_release(&exit_gate->gate);
  const auto status = mln_test_completion_settle(&closed);
  mln::testing::set_sync_point_handler(nullptr, nullptr);
  MLN_TEST_OK(status);
  TEST_ASSERT_TRUE(exiting);
  TEST_ASSERT_TRUE(retained);
  TEST_ASSERT_FALSE(completed_early);
  MLN_TEST_OK(other_status);
}

void abandoned_completion_keeps_its_submission_pending_until_delivery() {
  for (auto is_operation : {false, true}) {
    const auto runtime = create_runtime();
    auto live = mln::core::lease_runtime(runtime);
    auto result = mln_test_completion_default(0);
    auto completion = created(result.descriptor);
    if (is_operation) {
      auto operation = std::make_shared<mln::core::OperationObject>(
        [](mln_status, std::string, std::any) { throw std::bad_alloc{}; }
      );
      MLN_TEST_OK(
        mln::core::submit_runtime_operation(
          live, operation,
          [operation] { operation->complete(MLN_STATUS_OK, {}, {}); },
          completion
        )
      );
    } else {
      MLN_TEST_OK(
        mln::core::submit_runtime_command(live, [](uint64_t) {}, completion)
      );
    }
    auto work_retired = std::atomic_bool{false};
    live->executor.invoke([&work_retired] {
      mln_test_flag_set(&work_retired);
    });
    const auto retired = mln_test_wait_for_flag(&work_retired);
    auto tracked = false;
    {
      const auto lock = std::scoped_lock{live->terminal_mutex};
      tracked =
        live->pending_submissions.size() + live->pending_commands.size() == 1;
    }
    completion.reset();
    const auto status = mln_test_completion_settle(&result);
    MLN_TEST_OK(mln_test_runtime_close(runtime));
    TEST_ASSERT_TRUE(retired);
    TEST_ASSERT_TRUE(tracked);
    MLN_TEST_STATUS(MLN_STATUS_CANCELLED, status);
  }
}

void runtime_release_waits_for_worker_thread_destruction() {
  const auto runtime = create_runtime();
  auto live = mln::core::lease_runtime(runtime);
  auto probe = std::make_shared<ThreadExitProbe>();
  live->executor.invoke([probe] {
    [[clang::always_destroy]] thread_local ThreadExitSentinel sentinel;
    sentinel.probe = probe;
  });
  auto descriptor = mln_completion{
    .size = sizeof(mln_completion),
    .callback =
      [](void* context, const mln_completion_result*) {
        auto& state = **static_cast<std::shared_ptr<ThreadExitProbe>*>(context);
        state.destroyed_at_completion = state.destroyed.load();
      },
    .user_data = new std::shared_ptr{probe},
    .release_user_data =
      [](void* context) {
        const auto owned = std::unique_ptr<std::shared_ptr<ThreadExitProbe>>{
          static_cast<std::shared_ptr<ThreadExitProbe>*>(context)
        };
        mln_test_flag_set(&(*owned)->checked);
      },
  };
  MLN_TEST_OK(mln_runtime_release(runtime, &descriptor, nullptr));
  live.reset();
  const auto completion_ran = mln_test_wait_for_flag(&probe->checked);
  const auto thread_ended = mln_test_wait_for_flag(&probe->destroyed);
  TEST_ASSERT_TRUE(completion_ran);
  TEST_ASSERT_TRUE(thread_ended);
  TEST_ASSERT_TRUE_MESSAGE(
    probe->destroyed_at_completion.load(),
    "runtime release completed before its worker's TLS destructor"
  );
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(a_blocked_worker_exit_does_not_hold_other_retirements);
  RUN_TEST(abandoned_completion_keeps_its_submission_pending_until_delivery);
  RUN_TEST(accepted_completions_keep_runtime_submissions_pending);
  RUN_TEST(operation_submission_waits_for_native_work_to_retire);
  RUN_TEST(barrier_admission_failure_retires_its_sequence);
  RUN_TEST(runtime_release_waits_for_worker_thread_destruction);

  RUN_TEST(submission_failure_callbacks_can_submit_again);
  RUN_TEST(a_deferred_runtime_release_does_not_hold_other_retirements);
  RUN_TEST(map_release_waits_for_a_lease_queued_after_close_begins);
}
