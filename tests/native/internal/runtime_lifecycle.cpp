// Runtime release against a map's physical cleanup, which runs on a teardown
// lane after the map's own release has completed.

#include <cstddef>
#include <thread>

#include "completion/completion.hpp"
#include "execution/worker_thread.hpp"
#include "internal/support/sync_points.hpp"
#include "map/map.hpp"
#include "map/map_internal.hpp"
#include "maplibre_native_c.h"
#include "operation/operation.hpp"
#include "runtime/runtime.hpp"
#include "support/camera.h"
#include "support/test_support.h"

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
  auto options = mln_runtime_options{};
  options.size = sizeof(mln_runtime_options);
  auto runtime = mln_runtime{MLN_HANDLE_NULL};
  MLN_TEST_OK(mln_runtime_create(&options, &runtime, nullptr));
  return runtime;
}

// An unstarted executor rejects queueing deterministically, without requiring
// allocation failure in a public submission. A rejected submission's callbacks
// run before it returns, and they can submit again.
void submission_failure_callbacks_can_submit_again() {
  using mln::core::OperationObject;
  using mln::core::RuntimeObject;
  auto runtime = std::make_shared<RuntimeObject>();
  const auto descriptor = mln_test_discard_completion();
  auto reentrant_status = MLN_STATUS_OK;
  auto operation =
    std::make_shared<OperationObject>([&](mln_status, std::string, std::any) {
      auto completion = created(descriptor);
      reentrant_status =
        mln::core::submit_runtime_command(runtime, [] {}, completion);
    });
  MLN_TEST_STATUS(
    MLN_STATUS_NATIVE_ERROR, mln::core::submit_runtime_operation(
                               runtime, operation, [] {}, created(descriptor)
                             )
  );
  MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, reentrant_status);
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

  auto create_map = mln_test_completion_default(0);
  auto map = mln_map{MLN_HANDLE_NULL};
  const auto map_options = mln_test_map_options();
  MLN_TEST_OK(mln_runtime_create_map(
    waiting, &map_options, &map, &create_map.descriptor, nullptr
  ));
  MLN_TEST_OK(mln_test_completion_settle(&create_map));
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
  auto creation = mln_test_completion_default(0);
  auto map = mln_map{MLN_HANDLE_NULL};
  const auto map_options = mln_test_map_options();
  MLN_TEST_OK(mln_runtime_create_map(
    runtime, &map_options, &map, &creation.descriptor, nullptr
  ));
  MLN_TEST_OK(mln_test_completion_settle(&creation));
  auto transition = mln_test_transition_end{};
  mln_test_transition_end_init(&transition);
  auto camera = mln_camera_update{};
  camera.size = sizeof(mln_camera_update);
  camera.camera.size = sizeof(mln_camera_options);
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
  TEST_ASSERT_TRUE(mln_test_await(
    [](void* context) {
      return static_cast<std::weak_ptr<mln::core::RuntimeObject>*>(context)
        ->expired();
    },
    &runtime_weak, mln_test_deadline_default(), "the runtime to retire"
  ));
}

// Runtime release waits until each accepted completion has been delivered and
// has released its user data, and reports that it deferred retirement for it.
// A command and an operation each park their completion's release, and the
// operation's result arrives from another thread after its worker task
// returned, the way an offline database result does.
void accepted_completions_hold_runtime_release() {
  for (auto is_operation : {false, true}) {
    auto sync_points = SyncPointScope{};
    const auto runtime = create_runtime();
    auto live = mln::core::lease_runtime(runtime);
    auto release = mln_test_gate{};
    mln_test_gate_init(&release);
    const auto descriptor = mln_completion{
      .size = sizeof(mln_completion),
      .callback = [](void*, const mln_completion_result*) {},
      .user_data = &release,
      .release_user_data =
        [](void* context) {
          mln_test_gate_park(static_cast<mln_test_gate*>(context));
        },
    };
    auto resolver = std::thread{};
    if (is_operation) {
      auto pair = mln::core::CompletionOperation{};
      MLN_TEST_OK(
        mln::core::create_completion_operation(
          &descriptor,
          mln::core::valueless_completion<
            &mln_runtime_set_maximum_ambient_cache_size>(),
          pair
        )
      );
      MLN_TEST_OK(
        mln::core::submit_runtime_operation(
          live, pair.operation, [] {}, pair.completion
        )
      );
      auto work_ran = std::atomic_bool{false};
      live->executor.invoke([&work_ran] { mln_test_flag_set(&work_ran); });
      TEST_ASSERT_TRUE(mln_test_wait_for_flag(&work_ran));
      resolver = std::thread{[operation = pair.operation] {
        operation->complete(MLN_STATUS_OK, {}, {});
      }};
    } else {
      auto completion = created(descriptor);
      MLN_TEST_OK(
        mln::core::submit_runtime_command(
          live,
          [completion] {
            mln::core::complete_command(
              completion, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK
            );
          },
          completion
        )
      );
    }
    const auto release_entered = mln_test_gate_wait_entered(&release);
    live.reset();
    auto closed = mln_test_completion_default(0);
    const auto close_status =
      mln_runtime_release(runtime, &closed.descriptor, nullptr);
    const auto deferred =
      sync_points.wait_for_hits(SyncPoint::RuntimeRetirementDeferred, 1);
    const auto closed_early = mln_test_completion_poll(&closed);
    mln_test_gate_release(&release);
    if (resolver.joinable()) resolver.join();
    MLN_TEST_OK(mln_test_completion_settle(&closed));
    TEST_ASSERT_TRUE(release_entered);
    MLN_TEST_OK(close_status);
    TEST_ASSERT_TRUE_MESSAGE(
      deferred, "runtime release did not wait for a completion's release"
    );
    TEST_ASSERT_FALSE(closed_early);
  }
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

// A completion that native work abandons without resolving it still holds
// runtime release until it delivers its cancellation, whether the command's
// task dropped it or an operation's result delivery threw.
void an_abandoned_completion_holds_runtime_release_until_it_cancels() {
  for (auto is_operation : {false, true}) {
    auto sync_points = SyncPointScope{};
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
      MLN_TEST_OK(mln::core::submit_runtime_command(live, [] {}, completion));
    }
    auto work_ran = std::atomic_bool{false};
    live->executor.invoke([&work_ran] { mln_test_flag_set(&work_ran); });
    const auto ran = mln_test_wait_for_flag(&work_ran);
    live.reset();
    auto closed = mln_test_completion_default(0);
    const auto close_status =
      mln_runtime_release(runtime, &closed.descriptor, nullptr);
    const auto deferred =
      sync_points.wait_for_hits(SyncPoint::RuntimeRetirementDeferred, 1);
    completion.reset();
    const auto status = mln_test_completion_settle(&result);
    MLN_TEST_OK(mln_test_completion_settle(&closed));
    TEST_ASSERT_TRUE(ran);
    MLN_TEST_OK(close_status);
    TEST_ASSERT_TRUE(deferred);
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

#if defined(__linux__) && !defined(__GLIBC__) && !defined(__ANDROID__) && \
  !defined(__OHOS__)
// MapLibre starts its own threads with the process's default stack, and host
// callbacks run on them, so creating a runtime raises musl's 128 KiB default.
void a_runtime_raises_musls_default_thread_stack() {
  const auto runtime = create_runtime();
  auto attributes = pthread_attr_t{};
  TEST_ASSERT_EQUAL_INT(0, pthread_getattr_default_np(&attributes));
  auto stack_size = std::size_t{0};
  TEST_ASSERT_EQUAL_INT(0, pthread_attr_getstacksize(&attributes, &stack_size));
  pthread_attr_destroy(&attributes);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64(
    mln::core::WorkerThread::kStackSize, stack_size
  );
  auto released = mln_test_completion_default(0);
  MLN_TEST_OK(mln_runtime_release(runtime, &released.descriptor, nullptr));
  MLN_TEST_OK(mln_test_completion_settle(&released));
}
#endif

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(an_abandoned_completion_holds_runtime_release_until_it_cancels);
  RUN_TEST(accepted_completions_hold_runtime_release);
  RUN_TEST(runtime_release_waits_for_worker_thread_destruction);

  RUN_TEST(submission_failure_callbacks_can_submit_again);
  RUN_TEST(a_deferred_runtime_release_does_not_hold_other_retirements);
  RUN_TEST(map_release_waits_for_a_lease_queued_after_close_begins);
#if defined(__linux__) && !defined(__GLIBC__) && !defined(__ANDROID__) && \
  !defined(__OHOS__)
  RUN_TEST(a_runtime_raises_musls_default_thread_stack);
#endif
}
