// A render session's driver while it is busy: demands that arrive during a
// driver call, deadlines measured on the render clock, and abandon.

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>

#include "internal/support/checks.hpp"
#include "internal/support/driver_blocker.hpp"
#include "internal/support/sync_points.hpp"
#include "maplibre_native_c.h"
#include "support/frames.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "testing/render_clock.hpp"
#include "unity.h"

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
  mln_test_render_prepare_map(fixture.runtime, fixture.map);
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
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, blocker.submit(fixture.render.session));
  if (fixture.render.driver == MLN_RENDER_DRIVER_CORE_WORKER) {
    TEST_ASSERT_TRUE(mln_test_gate_wait_entered(blocker.gate.get()));
  }
}

auto wait_for_results(const Fixture& fixture, std::size_t minimum)
  -> mln_render_frame_batch {
  return mln_test_render_wait_for_results(&fixture.render, minimum);
}

auto batch_result(mln_render_frame_batch batch, std::size_t index)
  -> mln_render_frame_result {
  return mln_test_render_batch_result(batch, index);
}

auto request_frame(const Fixture& fixture, const mln_frame_demand& demand)
  -> mln_status {
  return mln_render_session_request_frame(
    fixture.render.session, &demand, nullptr
  );
}

// Demands that queue behind a busy driver coalesce within a boundary: the
// older one is superseded, and a demand with a new boundary stays separate.
void demand_coalescing_preserves_boundaries_and_generations() {
  auto fixture = Fixture{};
  create_fixture(fixture);
  auto blocker = DriverBlocker{};
  block_driver(fixture, blocker);

  auto first = mln_frame_demand_default();
  first.flags = 0;
  first.token = 101;
  first.coalescing_boundary = 7;
  auto newest = first;
  newest.token = 102;
  auto separate = newest;
  separate.token = 103;
  separate.coalescing_boundary = 8;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, request_frame(fixture, first));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, request_frame(fixture, newest));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, request_frame(fixture, separate));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, blocker.finish(fixture.render));

  auto unknown = mln_frame_demand_default();
  unknown.flags = 1U << 8U;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, request_frame(fixture, unknown)
  );

  const auto batch = wait_for_results(fixture, 3);
  auto later = separate;
  later.token = 104;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, request_frame(fixture, later));
  const auto second_batch = wait_for_results(fixture, 1);
  TEST_ASSERT_EQUAL_UINT64(104, batch_result(second_batch, 0).token);
  mln_render_frame_batch_release(second_batch);
  const auto superseded = batch_result(batch, 0);
  const auto rendered = batch_result(batch, 1);
  const auto boundary = batch_result(batch, 2);
  TEST_ASSERT_EQUAL_UINT64(101, superseded.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_SUPERSEDED, superseded.disposition
  );
  TEST_ASSERT_EQUAL_UINT64(102, rendered.token);
  TEST_ASSERT_EQUAL_UINT64(103, boundary.token);
  TEST_ASSERT_GREATER_THAN_UINT64(0, rendered.extent_generation);
  TEST_ASSERT_GREATER_THAN_UINT64(0, rendered.map_update_generation);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64(
    rendered.map_update_generation, boundary.map_update_generation
  );
  TEST_ASSERT_GREATER_THAN_UINT64(0, rendered.frame_generation);
  mln_render_frame_batch_release(batch);
  destroy_fixture(fixture);
}

// A demand's timeout runs from its acceptance on the render clock, so one
// whose deadline passes while the driver is busy misses it, and one with time
// left renders. The timeouts are far longer than any run, so only the clock
// advance decides which demand misses.
void a_demand_misses_a_deadline_that_passes_while_the_driver_is_busy() {
  auto fixture = Fixture{};
  create_fixture(fixture);
  auto blocker = DriverBlocker{};
  block_driver(fixture, blocker);

  auto missed = mln_frame_demand_default();
  missed.flags = 0;
  missed.token = 105;
  missed.coalescing_boundary = 1;
  missed.timeout_ns = std::chrono::nanoseconds{std::chrono::hours{1}}.count();
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, request_frame(fixture, missed));
  mln::testing::advance_render_clock(std::chrono::hours{2});
  auto in_time = missed;
  in_time.token = 106;
  in_time.coalescing_boundary = 2;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, request_frame(fixture, in_time));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, blocker.finish(fixture.render));

  const auto batch = wait_for_results(fixture, 2);
  const auto first = batch_result(batch, 0);
  const auto second = batch_result(batch, 1);
  TEST_ASSERT_EQUAL_UINT64(105, first.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_DEADLINE_MISSED, first.disposition
  );
  TEST_ASSERT_EQUAL_UINT64(106, second.token);
  TEST_ASSERT_NOT_EQUAL_UINT32(
    MLN_RENDER_RESULT_DEADLINE_MISSED, second.disposition
  );
  mln_render_frame_batch_release(batch);
  destroy_fixture(fixture);
}

// A demand the full texture ring parks keeps the time it was accepted at, so
// when a released frame lets it run after its deadline passed on the render
// clock, it misses the deadline rather than rendering late.
void a_parked_demand_misses_a_deadline_that_passes_while_the_ring_is_full() {
  auto fixture = Fixture{};
  create_fixture(fixture);
  auto first = mln_test_render_and_acquire(&fixture.render, 111);
  auto second = mln_test_render_and_acquire(&fixture.render, 112);

  auto parked = mln_frame_demand_default();
  parked.flags = 0;
  parked.token = 113;
  parked.coalescing_boundary = 113;
  parked.timeout_ns = std::chrono::nanoseconds{std::chrono::hours{1}}.count();
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, request_frame(fixture, parked));
  // The fence runs after the driver parked the demand.
  auto fence = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_reduce_memory_use(
                     fixture.render.session, &fence.descriptor, nullptr
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_render_fixture_finish_operation(&fixture.render, &fence)
  );
  mln_test_completion_destroy(&fence);
  mln::testing::advance_render_clock(std::chrono::hours{2});

  const auto cpu_complete = mln_gpu_sync_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_acquired_frame_release(&first, &cpu_complete, nullptr)
  );
  const auto batch = wait_for_results(fixture, 1);
  const auto result = batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT64(113, result.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_DEADLINE_MISSED, result.disposition
  );
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_acquired_frame_release(&second, &cpu_complete, nullptr)
  );
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
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, request_frame(fixture, demand));
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

// A core worker's driver call can still be running after the host has read the
// frame result it published, and nothing public observes the call's end, so
// abandon waits it out and succeeds. A caller driver's call is the host's own,
// so abandon from another thread during it is busy and changes nothing.
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
    auto published_count = std::size_t{0};
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK,
      mln_render_frame_batch_count(published.batch, &published_count, nullptr)
    );
    TEST_ASSERT_EQUAL_size_t(1, published_count);
    TEST_ASSERT_EQUAL_UINT64(107, batch_result(published.batch, 0).token);
    mln_render_frame_batch_release(published.batch);
    auto* thread = mln_test_thread_start(release_when_abandon_waits, &release);
    release.abandon_status =
      mln_render_session_abandon(fixture.render.session, &result, nullptr);
    release.abandon_returned.store(true);
    mln_test_pulse();
    mln_test_thread_join(thread);
    TEST_ASSERT_TRUE(release.abandon_waited);
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, release.abandon_status);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED, result.disposition
    );
  } else {
    // Servicing the attach already passed the exit point, so the held call is
    // the next exit.
    release.exits_before_the_call = points.hits(SyncPoint::RenderDriverExited);
    auto* thread = mln_test_thread_start(abandon_inside_the_call, &release);
    auto serviced = std::size_t{0};
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_render_session_service_driver_work(
                       fixture.render.session, 0, &serviced, nullptr
                     )
    );
    mln_test_thread_join(thread);
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_BUSY, release.abandon_status);
    TEST_ASSERT_EQUAL_INT(0, points.hits(SyncPoint::RenderAbandonWaits));
  }
  auto snapshot =
    mln_render_session_snapshot{.size = sizeof(mln_render_session_snapshot)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
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

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(demand_coalescing_preserves_boundaries_and_generations);
  RUN_TEST(a_demand_misses_a_deadline_that_passes_while_the_driver_is_busy);
  RUN_TEST(
    a_parked_demand_misses_a_deadline_that_passes_while_the_ring_is_full
  );
  RUN_TEST(abandon_after_a_published_frame_waits_for_a_core_worker_call);
}
