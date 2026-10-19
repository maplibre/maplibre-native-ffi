// Runtime release against a map's physical cleanup, which runs on a teardown
// lane after the map's own release has completed.

#include "completion/completion.hpp"
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

auto accept(const mln_completion& descriptor)
  -> std::shared_ptr<mln::core::Completion> {
  auto completion = std::shared_ptr<mln::core::Completion>{};
  MLN_TEST_OK(mln::core::accept_completion(&descriptor, completion));
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
    auto completion = accept(descriptor);
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
  auto completion = accept(descriptor);
  MLN_TEST_STATUS(
    MLN_STATUS_NATIVE_ERROR,
    mln::core::submit_runtime_command(runtime, [](uint64_t) {}, completion)
  );
  MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, reentrant_status);
  TEST_ASSERT_TRUE(runtime->pending_submissions.empty());

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
  auto completion = accept(command.descriptor);
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

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(submission_failure_callbacks_can_submit_again);
  RUN_TEST(a_deferred_runtime_release_does_not_hold_other_retirements);
  RUN_TEST(map_release_waits_for_a_lease_queued_after_close_begins);
}
