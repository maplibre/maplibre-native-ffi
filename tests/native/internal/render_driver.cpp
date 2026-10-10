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
#include "support/test_support.h"
#include "testing/render_clock.hpp"

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
  MLN_TEST_OK(blocker.submit(fixture.render.session));
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
  MLN_TEST_OK(request_frame(fixture, first));
  MLN_TEST_OK(request_frame(fixture, newest));
  MLN_TEST_OK(request_frame(fixture, separate));
  MLN_TEST_OK(blocker.finish(fixture.render));

  auto unknown = mln_frame_demand_default();
  unknown.flags = 1U << 8U;
  MLN_TEST_INVALID(request_frame(fixture, unknown));

  const auto batch = wait_for_results(fixture, 3);
  auto later = separate;
  later.token = 104;
  MLN_TEST_OK(request_frame(fixture, later));
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
  MLN_TEST_OK(request_frame(fixture, missed));
  mln::testing::advance_render_clock(std::chrono::hours{2});
  auto in_time = missed;
  in_time.token = 106;
  in_time.coalescing_boundary = 2;
  MLN_TEST_OK(request_frame(fixture, in_time));
  MLN_TEST_OK(blocker.finish(fixture.render));

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
  MLN_TEST_OK(request_frame(fixture, parked));
  // The fence runs after the driver parked the demand.
  auto fence = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_reduce_memory_use(
    fixture.render.session, &fence.descriptor, nullptr
  ));
  MLN_TEST_OK(
    mln_test_render_fixture_finish_operation(&fixture.render, &fence)
  );
  mln_test_completion_destroy(&fence);
  mln::testing::advance_render_clock(std::chrono::hours{2});

  const auto cpu_complete = mln_gpu_sync_default();
  MLN_TEST_OK(mln_acquired_frame_release(&first, &cpu_complete, nullptr));
  const auto batch = wait_for_results(fixture, 1);
  const auto result = batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT64(113, result.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_DEADLINE_MISSED, result.disposition
  );
  mln_render_frame_batch_release(batch);
  MLN_TEST_OK(mln_acquired_frame_release(&second, &cpu_complete, nullptr));
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
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED, result.disposition
    );
  } else {
    // Servicing the attach already passed the exit point, so the held call is
    // the next exit.
    release.exits_before_the_call = points.hits(SyncPoint::RenderDriverExited);
    auto* thread = mln_test_thread_start(abandon_inside_the_call, &release);
    auto serviced = std::size_t{0};
    MLN_TEST_OK(mln_render_session_service_driver_work(
      fixture.render.session, 0, &serviced, nullptr
    ));
    mln_test_thread_join(thread);
    MLN_TEST_STATUS(MLN_STATUS_BUSY, release.abandon_status);
    TEST_ASSERT_EQUAL_INT(0, points.hits(SyncPoint::RenderAbandonWaits));
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
// then, so the abandon completes the detach as abandoned instead of leaving it
// queued for a driver that has stopped. The blocked driver keeps the detach
// queued until the abandon takes it.
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
  demand.flags = 0;
  demand.token = 122;
  demand.coalescing_boundary = 9;
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
  first.flags = 0;
  first.token = 121;
  first.coalescing_boundary = 9;
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

struct ParkedDisposal {
  SyncPointScope* points;
  mln_acquired_frame first;
  mln_acquired_frame second;
  bool parked = false;
  mln_status first_status = MLN_STATUS_NATIVE_ERROR;
  mln_status second_status = MLN_STATUS_NATIVE_ERROR;
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
  if (fixture.render.driver != MLN_RENDER_DRIVER_CORE_WORKER) {
    // The case's own thread runs the demand, so it parks inside this call
    // until the disposal lets it go.
    auto serviced = std::size_t{0};
    MLN_TEST_OK(mln_render_session_service_driver_work(
      fixture.render.session, 0, &serviced, nullptr
    ));
  }
  mln_test_thread_join(disposing);
  TEST_ASSERT_TRUE(disposal.parked);
  MLN_TEST_OK(disposal.first_status);
  MLN_TEST_OK(disposal.second_status);

  const auto batch = wait_for_results(fixture, 1);
  const auto result = batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(133, result.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_TARGET_NOT_READY, result.disposition
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
  RUN_TEST(an_abandon_during_a_detach_submission_completes_the_detach);
  RUN_TEST(an_abandon_wakes_for_results_a_racing_request_has_yet_to_wake);
  RUN_TEST(a_demand_parked_as_disposal_quarantines_the_ring_gets_a_result);
}
