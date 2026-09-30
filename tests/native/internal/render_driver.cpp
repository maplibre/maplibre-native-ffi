// A render session's driver while it is busy: demands that arrive during a
// driver call, deadlines measured on the render clock, and abandon.

#include <chrono>
#include <cstddef>
#include <cstdint>

#include "internal/support/driver_blocker.hpp"
#include "maplibre_native_c.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "testing/render_clock.hpp"
#include "unity.h"

namespace {

using mln::native_tests::DriverBlocker;

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
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_map_set_style_json(fixture.map, mln_test_empty_style_json)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_runtime_barrier(fixture.runtime)
  );
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

struct ResultsWait {
  const mln_test_render_fixture* fixture;
  std::size_t minimum;
  mln_render_frame_batch batch;
  bool failed;
};

// Holds once every pending demand has settled and one drain yields at least
// `minimum` results. A drain with fewer is released and the wait goes on.
auto results_ready(void* context) -> bool {
  auto& wait = *static_cast<ResultsWait*>(context);
  auto snapshot =
    mln_render_session_snapshot{.size = sizeof(mln_render_session_snapshot)};
  if (
    mln_render_session_get_snapshot(
      wait.fixture->session, &snapshot, nullptr
    ) != MLN_STATUS_OK ||
    snapshot.pending_demand_count != 0
  ) {
    return false;
  }
  auto batch = mln_render_frame_batch{MLN_HANDLE_NULL};
  const auto status = mln_render_session_drain_frame_results(
    wait.fixture->session, &batch, nullptr
  );
  if (status == MLN_STATUS_OK) {
    auto count = std::size_t{0};
    if (
      mln_render_frame_batch_count(batch, &count, nullptr) == MLN_STATUS_OK &&
      count >= wait.minimum
    ) {
      wait.batch = batch;
      return true;
    }
    mln_render_frame_batch_release(batch);
  } else if (status != MLN_STATUS_NOT_READY) {
    wait.failed = true;
    return true;
  }
  return false;
}

auto wait_for_results(const Fixture& fixture, std::size_t minimum)
  -> mln_render_frame_batch {
  auto wait = ResultsWait{&fixture.render, minimum, MLN_HANDLE_NULL, false};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_step_until(
                     &fixture.render, results_ready, &wait,
                     mln_test_deadline_default(), "frame results"
                   )
  );
  TEST_ASSERT_FALSE(wait.failed);
  return wait.batch;
}

auto batch_result(mln_render_frame_batch batch, std::size_t index)
  -> mln_render_frame_result {
  auto result =
    mln_render_frame_result{.size = sizeof(mln_render_frame_result)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_frame_batch_get(batch, index, &result, nullptr)
  );
  return result;
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
// left renders.
void a_demand_misses_a_deadline_that_passes_while_the_driver_is_busy() {
  auto fixture = Fixture{};
  create_fixture(fixture);
  auto blocker = DriverBlocker{};
  block_driver(fixture, blocker);

  auto missed = mln_frame_demand_default();
  missed.flags = 0;
  missed.token = 105;
  missed.coalescing_boundary = 1;
  missed.timeout_ns = std::chrono::nanoseconds{std::chrono::seconds{1}}.count();
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, request_frame(fixture, missed));
  mln::testing::advance_render_clock(std::chrono::seconds{2});
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

struct AbandonProbe {
  mln_render_session session;
  mln_test_gate* gate;
  mln_status status;
};

void abandon_when_driver_enters(void* argument) {
  auto& probe = *static_cast<AbandonProbe*>(argument);
  static_cast<void>(mln_test_gate_wait_entered(probe.gate));
  auto result =
    mln_render_abandon_result{.size = sizeof(mln_render_abandon_result)};
  probe.status = mln_render_session_abandon(probe.session, &result, nullptr);
  mln_test_gate_release(probe.gate);
}

// Abandon during a driver call reports busy and leaves the session attached.
void abandon_is_busy_during_a_driver_call_and_changes_nothing() {
  auto fixture = Fixture{};
  create_fixture(fixture);
  auto blocker = DriverBlocker{};
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, blocker.submit(fixture.render.session));

  auto abandon_status = MLN_STATUS_NATIVE_ERROR;
  if (fixture.render.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
    // The case's own thread parks inside the driver call it services, so
    // abandon comes from another thread.
    auto probe = AbandonProbe{
      fixture.render.session, blocker.gate.get(), MLN_STATUS_NATIVE_ERROR
    };
    auto* thread = mln_test_thread_start(abandon_when_driver_enters, &probe);
    auto serviced = std::size_t{0};
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_render_session_service_driver_work(
                       fixture.render.session, SIZE_MAX, &serviced, nullptr
                     )
    );
    mln_test_thread_join(thread);
    abandon_status = probe.status;
  } else {
    TEST_ASSERT_TRUE(mln_test_gate_wait_entered(blocker.gate.get()));
    auto result =
      mln_render_abandon_result{.size = sizeof(mln_render_abandon_result)};
    abandon_status =
      mln_render_session_abandon(fixture.render.session, &result, nullptr);
  }
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, blocker.finish(fixture.render));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_BUSY, abandon_status);
  auto snapshot =
    mln_render_session_snapshot{.size = sizeof(mln_render_session_snapshot)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_render_session_get_snapshot(fixture.render.session, &snapshot, nullptr)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_ATTACHED, snapshot.state);
  destroy_fixture(fixture);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(demand_coalescing_preserves_boundaries_and_generations);
  RUN_TEST(a_demand_misses_a_deadline_that_passes_while_the_driver_is_busy);
  RUN_TEST(abandon_is_busy_during_a_driver_call_and_changes_nothing);
}
