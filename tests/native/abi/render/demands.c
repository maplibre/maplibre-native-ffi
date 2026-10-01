// Frame demands: result batches and the frame wake, the texture ring's
// backpressure, barriers and resizes ordered with demands, demands past the
// ring depth, and the keep-alive demands a still image needs.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "support/frames.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static mln_render_session_snapshot read_snapshot(mln_render_session session) {
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(mln_render_session_get_snapshot(session, &snapshot, NULL));
  return snapshot;
}

static void release_frame(mln_acquired_frame* frame) {
  const mln_gpu_sync cpu_complete = mln_gpu_sync_default();
  MLN_TEST_OK(mln_acquired_frame_release(frame, &cpu_complete, NULL));
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, *frame);
}

// Runs a driver command to completion, so everything the driver was given
// before it has run.
static void fence_driver(const mln_test_render_fixture* fixture) {
  mln_test_completion fence = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_reduce_memory_use(
    fixture->session, &fence.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, &fence));
  mln_test_completion_destroy(&fence);
}

// A drained frame-result batch is an owned handle: releasing it twice is a
// no-op, releasing the null handle is a no-op, and a released handle names no
// batch.
static void a_released_frame_batch_names_no_batch(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_frame_demand demand = mln_frame_demand_default();
  demand.token = 201;
  MLN_TEST_OK(mln_render_session_request_frame(fixture.session, &demand, NULL));
  mln_render_frame_batch batch = mln_test_render_wait_for_results(&fixture, 1);

  size_t count = 99;
  MLN_TEST_OK(mln_render_frame_batch_count(batch, &count, NULL));
  TEST_ASSERT_EQUAL_size_t(1, count);
  mln_render_frame_batch_release(batch);
  mln_render_frame_batch_release(batch);
  mln_render_frame_batch_release(MLN_HANDLE_NULL);

  count = 99;
  MLN_TEST_INVALID(mln_render_frame_batch_count(batch, &count, NULL));
  TEST_ASSERT_EQUAL_size_t(99, count);
  mln_render_frame_result result = {
    .size = sizeof(mln_render_frame_result), .token = 99
  };
  MLN_TEST_INVALID(mln_render_frame_batch_get(batch, 0, &result, NULL));
  TEST_ASSERT_EQUAL_UINT64(99, result.token);
  MLN_TEST_INVALID(
    mln_render_frame_batch_get(MLN_HANDLE_NULL, 0, &result, NULL)
  );
  TEST_ASSERT_EQUAL_UINT64(99, result.token);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct frame_wake_wait {
  const mln_test_render_fixture* fixture;
  unsigned int before;
} frame_wake_wait;

static bool frame_woke(void* context) {
  const frame_wake_wait* wait = context;
  return atomic_load(&wait->fixture->frame_wakes) != wait->before;
}

// The frame wake runs when the result queue goes from empty to nonempty, and
// not again for a drain.
static void frame_wake_runs_when_the_result_queue_becomes_nonempty(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  frame_wake_wait wake = {
    .fixture = &fixture, .before = atomic_load(&fixture.frame_wakes)
  };
  mln_test_render_request_forced(&fixture, 1);
  MLN_TEST_OK(mln_test_render_step_until(
    &fixture, frame_woke, &wake, mln_test_deadline_default(), "a frame wake"
  ));
  const unsigned int woke = atomic_load(&fixture.frame_wakes);
  mln_render_frame_batch results = MLN_HANDLE_NULL;
  MLN_TEST_OK(
    mln_render_session_drain_frame_results(fixture.session, &results, NULL)
  );
  mln_render_frame_batch_release(results);
  TEST_ASSERT_EQUAL_UINT32(woke, atomic_load(&fixture.frame_wakes));
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// With every ring slot acquired, a demand waits for a CPU release instead of
// rendering over an acquired frame, and detach refuses while frames are out.
static void texture_ring_leases_apply_backpressure_until_cpu_release(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_acquired_frame first = mln_test_render_and_acquire(&fixture, 201);
  mln_acquired_frame second = mln_test_render_and_acquire(&fixture, 202);
  mln_gpu_sync producer = mln_gpu_sync_default();
  MLN_TEST_OK(mln_acquired_frame_get_producer_sync(first, &producer, NULL));
  TEST_ASSERT_EQUAL_UINT32(MLN_GPU_SYNC_CPU_COMPLETE, producer.kind);

  mln_test_render_request_forced(&fixture, 203);
  fence_driver(&fixture);
  const mln_render_session_snapshot snapshot = read_snapshot(fixture.session);
  TEST_ASSERT_EQUAL_UINT32(2, snapshot.acquired_frame_count);
  TEST_ASSERT_EQUAL_UINT32(1, snapshot.pending_demand_count);
  mln_completion rejected_detach = mln_test_discard_completion();
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_detach(fixture.session, &rejected_detach, NULL)
  );

  release_frame(&first);
  mln_render_frame_batch batch = mln_test_render_wait_for_results(&fixture, 1);
  TEST_ASSERT_EQUAL_UINT64(203, mln_test_render_batch_result(batch, 0).token);
  mln_render_frame_batch_release(batch);
  release_frame(&second);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A demand the full texture ring parks gives up its driver work item. A
// barrier accepted after it still has to wait for its terminal result.
static void barrier_waits_for_a_demand_parked_by_a_full_ring(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_acquired_frame first = mln_test_render_and_acquire(&fixture, 401);
  mln_acquired_frame second = mln_test_render_and_acquire(&fixture, 402);
  mln_test_render_request_forced(&fixture, 403);
  mln_test_completion barrier = mln_test_completion_default(0);
  MLN_TEST_OK(
    mln_render_session_barrier(fixture.session, &barrier.descriptor, NULL)
  );
  // The fence runs after the driver parked the demand and checked the
  // barrier, so the barrier stays pending until a frame is released.
  fence_driver(&fixture);
  TEST_ASSERT_FALSE(mln_test_completion_poll(&barrier));

  release_frame(&first);
  mln_render_frame_batch batch = mln_test_render_wait_for_results(&fixture, 1);
  TEST_ASSERT_EQUAL_UINT64(403, mln_test_render_batch_result(batch, 0).token);
  mln_render_frame_batch_release(batch);
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &barrier));
  mln_test_completion_destroy(&barrier);

  release_frame(&second);
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A demand parked by a full ring has no work item left to complete it, so
// detach is the last place that can give it a terminal result.
static void detach_gives_a_parked_demand_its_terminal_result(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_acquired_frame first = mln_test_render_and_acquire(&fixture, 501);
  mln_acquired_frame second = mln_test_render_and_acquire(&fixture, 502);
  mln_test_render_request_forced(&fixture, 503);
  fence_driver(&fixture);
  release_frame(&first);
  release_frame(&second);

  mln_test_completion detach = mln_test_completion_default(0);
  MLN_TEST_OK(
    mln_render_session_detach(fixture.session, &detach.descriptor, NULL)
  );
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &detach));
  mln_test_completion_destroy(&detach);

  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_OK(
    mln_render_session_drain_frame_results(fixture.session, &batch, NULL)
  );
  size_t count = 0;
  MLN_TEST_OK(mln_render_frame_batch_count(batch, &count, NULL));
  bool reported = false;
  for (size_t index = 0; index < count; index += 1) {
    if (mln_test_render_batch_result(batch, index).token == 503) {
      reported = true;
    }
  }
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_TRUE(reported);

  const mln_render_session_snapshot snapshot = read_snapshot(fixture.session);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_DETACHED, snapshot.state);
  TEST_ASSERT_EQUAL_UINT32(0, snapshot.pending_demand_count);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Demands keep rendering past the ring depth while the host acquires nothing:
// each frame takes the slot of the oldest unacquired one, so the ring ends up
// holding the newest frames, oldest first.
static void sustained_demands_past_the_ring_depth_keep_the_newest_frames(void) {
  enum { demand_count = 5 };
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  for (uint64_t token = 1; token <= demand_count; token += 1) {
    mln_test_render_request_forced(&fixture, token);
  }
  mln_render_frame_batch batch =
    mln_test_render_wait_for_results(&fixture, demand_count);
  uint64_t previous_generation = 0;
  for (size_t index = 0; index < demand_count; index += 1) {
    const mln_render_frame_result result =
      mln_test_render_batch_result(batch, index);
    TEST_ASSERT_EQUAL_UINT64(index + 1, result.token);
    TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
    TEST_ASSERT_GREATER_THAN_UINT64(
      previous_generation, result.frame_generation
    );
    previous_generation = result.frame_generation;
  }
  mln_render_frame_batch_release(batch);

  mln_acquired_frame frames[2] = {MLN_HANDLE_NULL, MLN_HANDLE_NULL};
  for (size_t index = 0; index < 2; index += 1) {
    MLN_TEST_OK(
      mln_render_session_acquire_frame(fixture.session, &frames[index], NULL)
    );
    mln_render_frame_result result = {.size = sizeof(mln_render_frame_result)};
    MLN_TEST_OK(mln_acquired_frame_get_result(frames[index], &result, NULL));
    TEST_ASSERT_EQUAL_UINT64(demand_count - 1 + index, result.token);
  }
  mln_acquired_frame extra = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_NOT_READY,
    mln_render_session_acquire_frame(fixture.session, &extra, NULL)
  );
  release_frame(&frames[0]);
  release_frame(&frames[1]);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void resize_and_barrier_order_frame_and_extent_generations(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_test_render_request_forced(&fixture, 301);
  mln_test_completion barrier = mln_test_completion_default(0);
  MLN_TEST_OK(
    mln_render_session_barrier(fixture.session, &barrier.descriptor, NULL)
  );
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &barrier));
  mln_test_completion_destroy(&barrier);
  mln_render_frame_batch batch = mln_test_render_wait_for_results(&fixture, 1);
  const mln_render_frame_result old_frame =
    mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);

  const mln_render_target_extent extent = {
    .size = sizeof(mln_render_target_extent),
    .width = 96,
    .height = 48,
    .scale_factor = 1.0,
  };
  mln_render_target_extent rescaled = extent;
  rescaled.scale_factor = 2.0;
  mln_completion rejected_scale = mln_test_discard_completion();
  MLN_TEST_INVALID(
    mln_render_session_resize(fixture.session, &rescaled, &rejected_scale, NULL)
  );
  mln_test_render_request_forced(&fixture, 302);
  mln_test_completion resize = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_resize(
    fixture.session, &extent, &resize.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &resize));
  mln_test_completion_destroy(&resize);
  batch = mln_test_render_wait_for_results(&fixture, 1);
  const mln_render_frame_result resizing_frame =
    mln_test_render_batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT64(
    old_frame.extent_generation, resizing_frame.extent_generation
  );
  mln_render_frame_batch_release(batch);

  mln_test_render_request_forced(&fixture, 303);
  batch = mln_test_render_wait_for_results(&fixture, 1);
  const mln_render_frame_result new_frame =
    mln_test_render_batch_result(batch, 0);
  TEST_ASSERT_GREATER_THAN_UINT64(
    old_frame.extent_generation, new_frame.extent_generation
  );
  TEST_ASSERT_GREATER_THAN_UINT64(
    old_frame.frame_generation, new_frame.frame_generation
  );
  mln_render_frame_batch_release(batch);

  const mln_render_session_snapshot snapshot = read_snapshot(fixture.session);
  TEST_ASSERT_EQUAL_UINT32(96, snapshot.extent.width);
  TEST_ASSERT_EQUAL_UINT32(48, snapshot.extent.height);
  TEST_ASSERT_DOUBLE_WITHIN(0.0, 1.0, snapshot.extent.scale_factor);
  TEST_ASSERT_EQUAL_UINT64(
    new_frame.extent_generation, snapshot.extent_generation
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Two resizes accepted before the driver reaches either: the first is
// superseded rather than parked forever waiting for an extent the map has
// already moved past, so the queue behind it keeps draining.
static void back_to_back_resizes_supersede_and_release_the_queue(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_render_target_extent first = {
    .size = sizeof(mln_render_target_extent),
    .width = 96,
    .height = 48,
    .scale_factor = 1.0,
  };
  mln_render_target_extent second = first;
  second.width = 128;
  second.height = 72;

  mln_test_completion first_resize = mln_test_completion_default(0);
  mln_test_completion second_resize = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_resize(
    fixture.session, &first, &first_resize.descriptor, NULL
  ));
  MLN_TEST_OK(mln_render_session_resize(
    fixture.session, &second, &second_resize.descriptor, NULL
  ));
  MLN_TEST_OK(
    mln_test_render_fixture_finish_operation(&fixture, &first_resize)
  );
  MLN_TEST_OK(
    mln_test_render_fixture_finish_operation(&fixture, &second_resize)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_COMMITTED,
    mln_test_completion_disposition(&second_resize)
  );
  mln_test_completion_destroy(&first_resize);
  mln_test_completion_destroy(&second_resize);

  mln_test_completion barrier = mln_test_completion_default(0);
  MLN_TEST_OK(
    mln_render_session_barrier(fixture.session, &barrier.descriptor, NULL)
  );
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &barrier));
  mln_test_completion_destroy(&barrier);

  const mln_render_session_snapshot snapshot = read_snapshot(fixture.session);
  TEST_ASSERT_EQUAL_UINT32(128, snapshot.extent.width);
  TEST_ASSERT_EQUAL_UINT32(72, snapshot.extent.height);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct keepalive_wait {
  const mln_test_render_fixture* fixture;
  mln_test_completion* still;
  bool demand_pending;
  bool failed;
} keepalive_wait;

// Keeps one render-if-needed demand in flight until the still image
// completes, releasing any frame a demand rendered.
static bool still_completed_under_keepalive(void* context) {
  keepalive_wait* wait = context;
  if (wait->demand_pending) {
    mln_render_frame_batch batch = MLN_HANDLE_NULL;
    const mln_status drained = mln_render_session_drain_frame_results(
      wait->fixture->session, &batch, NULL
    );
    if (drained == MLN_STATUS_NOT_READY) {
      return false;
    }
    mln_render_frame_batch_release(batch);
    wait->failed = drained != MLN_STATUS_OK;
    wait->demand_pending = false;
    mln_acquired_frame frame = MLN_HANDLE_NULL;
    while (
      mln_render_session_acquire_frame(wait->fixture->session, &frame, NULL) ==
      MLN_STATUS_OK) {
      wait->failed |=
        mln_acquired_frame_release(&frame, NULL, NULL) != MLN_STATUS_OK;
      frame = MLN_HANDLE_NULL;
    }
  }
  if (wait->failed || mln_test_completion_poll(wait->still)) {
    return true;
  }
  const mln_frame_demand demand = mln_frame_demand_default();
  wait->failed =
    mln_render_session_request_frame(wait->fixture->session, &demand, NULL) !=
    MLN_STATUS_OK;
  wait->demand_pending = !wait->failed;
  return wait->failed;
}

// A pending still image on a static map must complete while the host's
// keep-alive demands all resolve on the render-if-needed fast path. Worker
// results and the observer delivery that completes the still ride the session
// scheduler; only a rendering demand used to drain it, which stranded this
// exact await shape.
static void still_image_completes_under_if_needed_keepalive_demands(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.map_mode = MLN_MAP_MODE_STATIC;
  options.initial_extent.width = 64;
  options.initial_extent.height = 64;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_test_completion still = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_request_still_image(map, &still.descriptor, NULL));
  keepalive_wait wait = {.fixture = &fixture, .still = &still};
  MLN_TEST_OK(mln_test_render_step_until(
    &fixture, still_completed_under_keepalive, &wait,
    mln_test_deadline_default(), "a still image"
  ));
  TEST_ASSERT_FALSE(wait.failed);
  MLN_TEST_OK(mln_test_completion_status(&still));
  mln_test_completion_destroy(&still);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_released_frame_batch_names_no_batch);
  RUN_TEST(frame_wake_runs_when_the_result_queue_becomes_nonempty);
  RUN_TEST(texture_ring_leases_apply_backpressure_until_cpu_release);
  RUN_TEST(barrier_waits_for_a_demand_parked_by_a_full_ring);
  RUN_TEST(detach_gives_a_parked_demand_its_terminal_result);
  RUN_TEST(sustained_demands_past_the_ring_depth_keep_the_newest_frames);
  RUN_TEST(resize_and_barrier_order_frame_and_extent_generations);
  RUN_TEST(back_to_back_resizes_supersede_and_release_the_queue);
  RUN_TEST(still_image_completes_under_if_needed_keepalive_demands);
}
