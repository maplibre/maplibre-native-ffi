// Frame demands: result batches and the frame wake, the texture ring's
// backpressure, barriers and resizes ordered with demands, demands past the
// ring depth, and the keep-alive demands a still image needs.

#include "support/frames.h"
#include "support/test_support.h"

static mln_render_session_snapshot read_snapshot(mln_render_session session) {
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(mln_render_session_get_snapshot(session, &snapshot, NULL));
  return snapshot;
}

static void attach(
  mln_runtime* runtime, mln_map* map, mln_test_render_fixture* fixture
) {
  *runtime = mln_test_create_runtime();
  *map = mln_test_create_map(*runtime);
  mln_test_render_prepare_map(*runtime, *map);
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(*map, fixture));
}

static void detach(
  mln_runtime runtime, mln_map map, mln_test_render_fixture* fixture
) {
  mln_test_render_fixture_destroy(fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Runs a driver command to completion, so everything the driver was given
// before it has run.
static void fence_driver(const mln_test_render_fixture* fixture) {
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, fixture,
    mln_render_session_reduce_memory_use(
      fixture->session, &completion.descriptor, NULL
    )
  );
}

// The result of the one demand a batch holds, which this releases.
static mln_render_frame_result take_one_result(
  const mln_test_render_fixture* fixture
) {
  mln_render_frame_batch batch = mln_test_render_wait_for_results(fixture, 1);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  return result;
}

typedef struct frame_wake_wait {
  const mln_test_render_fixture* fixture;
  unsigned int before;
} frame_wake_wait;

static bool frame_woke(void* context) {
  const frame_wake_wait* wait = context;
  return atomic_load(&wait->fixture->frame_wakes) != wait->before;
}

// A drain of an empty result queue reports not ready and leaves its output
// null. The frame wake runs when the queue goes from empty to nonempty, and
// not again for a drain. A drained batch is an owned handle: releasing it
// twice is a no-op, releasing the null handle is a no-op, and a released
// handle names no batch.
static void frame_results_wake_the_host_and_drain_into_an_owned_batch(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture);
  mln_render_frame_batch empty = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_NOT_READY,
    mln_render_session_drain_frame_results(fixture.session, &empty, NULL)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, empty);
  frame_wake_wait wake = {
    .fixture = &fixture, .before = atomic_load(&fixture.frame_wakes)
  };
  mln_frame_demand demand = mln_frame_demand_default();
  demand.token = 201;
  MLN_TEST_OK(mln_render_session_request_frame(fixture.session, &demand, NULL));
  MLN_TEST_OK(mln_test_render_step_until(
    &fixture, frame_woke, &wake, mln_test_deadline_default(), "a frame wake"
  ));
  const unsigned int woke = atomic_load(&fixture.frame_wakes);
  mln_render_frame_batch batch = mln_test_render_wait_for_results(&fixture, 1);
  TEST_ASSERT_EQUAL_UINT32(woke, atomic_load(&fixture.frame_wakes));

  size_t count = 99;
  MLN_TEST_OK(mln_render_frame_batch_count(batch, &count, NULL));
  TEST_ASSERT_EQUAL_size_t(1, count);
  mln_render_frame_batch_release(batch);
  mln_render_frame_batch_release(batch);
  mln_render_frame_batch_release(MLN_HANDLE_NULL);
  count = 99;
  MLN_TEST_INVALID_STATE(mln_render_frame_batch_count(batch, &count, NULL));
  TEST_ASSERT_EQUAL_size_t(99, count);
  mln_render_frame_result result = {
    .size = sizeof(mln_render_frame_result), .token = 99
  };
  MLN_TEST_INVALID_STATE(mln_render_frame_batch_get(batch, 0, &result, NULL));
  MLN_TEST_INVALID(
    mln_render_frame_batch_get(MLN_HANDLE_NULL, 0, &result, NULL)
  );
  TEST_ASSERT_EQUAL_UINT64(99, result.token);
  detach(runtime, map, &fixture);
}

// With every ring slot acquired, a demand waits for a CPU release instead of
// rendering over an acquired frame, and detach refuses while frames are out.
// The parked demand gives up its driver work item, so a barrier accepted
// after it waits for its terminal result, and detach, the last place that can
// give it one, does.
static void a_full_ring_parks_demands_until_a_release_or_detach(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture);
  mln_acquired_frame first = mln_test_render_and_acquire(&fixture, 201);
  mln_acquired_frame second = mln_test_render_and_acquire(&fixture, 202);
  mln_gpu_sync producer = mln_gpu_sync_default();
  MLN_TEST_OK(mln_acquired_frame_get_producer_sync(first, &producer, NULL));
  TEST_ASSERT_EQUAL_UINT32(MLN_GPU_SYNC_CPU_COMPLETE, producer.kind);

  mln_test_render_request_forced(&fixture, 203);
  mln_test_completion barrier = mln_test_completion_default(0);
  MLN_TEST_OK(
    mln_render_session_barrier(fixture.session, &barrier.descriptor, NULL)
  );
  // The fence runs after the driver parked the demand and checked the
  // barrier, so the barrier stays pending until a frame is released.
  fence_driver(&fixture);
  TEST_ASSERT_FALSE(mln_test_completion_poll(&barrier));
  mln_render_session_snapshot snapshot = read_snapshot(fixture.session);
  TEST_ASSERT_EQUAL_UINT32(2, snapshot.acquired_frame_count);
  TEST_ASSERT_EQUAL_UINT32(1, snapshot.pending_demand_count);
  mln_completion rejected_detach = mln_test_discard_completion();
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_detach(fixture.session, &rejected_detach, NULL)
  );

  mln_test_render_release_frame(&first);
  TEST_ASSERT_EQUAL_UINT64(203, take_one_result(&fixture).token);
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &barrier));
  mln_test_completion_destroy(&barrier);

  // Park another demand behind a full ring, then release both frames, which
  // leaves the demand with no work item, and detach.
  first = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_render_session_acquire_frame(fixture.session, &first, NULL));
  mln_test_render_request_forced(&fixture, 503);
  fence_driver(&fixture);
  mln_test_render_release_frame(&first);
  mln_test_render_release_frame(&second);
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_detach(fixture.session, &completion.descriptor, NULL)
  );
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_OK(
    mln_render_session_drain_frame_results(fixture.session, &batch, NULL)
  );
  size_t count = 0;
  MLN_TEST_OK(mln_render_frame_batch_count(batch, &count, NULL));
  bool reported = false;
  for (size_t index = 0; index < count; index += 1) {
    reported |= mln_test_render_batch_result(batch, index).token == 503;
  }
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_TRUE(reported);
  snapshot = read_snapshot(fixture.session);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_DETACHED, snapshot.state);
  TEST_ASSERT_EQUAL_UINT32(0, snapshot.pending_demand_count);
  detach(runtime, map, &fixture);
}

// Demands keep rendering past the ring depth while the host acquires nothing:
// each frame takes the slot of the oldest unacquired one, so the ring ends up
// holding the newest frames, oldest first.
static void sustained_demands_past_the_ring_depth_keep_the_newest_frames(void) {
  enum { demand_count = 5 };
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture);
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
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, extra);
  mln_test_render_release_frame(&frames[0]);
  mln_test_render_release_frame(&frames[1]);
  detach(runtime, map, &fixture);
}

// A resize changes the extent generation from the next frame on, and keeps
// the scale factor. Two resizes accepted before the driver reaches either
// both commit: the first is superseded rather than parked forever waiting for
// an extent the map has already moved past, so the queue behind it keeps
// draining.
static void resizes_order_extent_generations_and_supersede_each_other(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture);
  mln_test_render_request_forced(&fixture, 301);
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_barrier(fixture.session, &completion.descriptor, NULL)
  );
  const mln_render_frame_result old_frame = take_one_result(&fixture);

  mln_logical_extent extent = {
    .width = 96,
    .height = 48,
    .scale_factor = 2.0,
  };
  mln_completion rejected_scale = mln_test_discard_completion();
  MLN_TEST_INVALID(
    mln_render_session_resize(fixture.session, extent, &rejected_scale, NULL)
  );
  extent.scale_factor = 1.0;
  MLN_TEST_INVALID(mln_render_session_resize(
    fixture.session,
    (mln_logical_extent){.width = 0, .height = 48, .scale_factor = 1.0},
    &rejected_scale, NULL
  ));
  mln_test_render_request_forced(&fixture, 302);
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_resize(
      fixture.session, extent, &completion.descriptor, NULL
    )
  );
  TEST_ASSERT_EQUAL_UINT64(
    old_frame.extent_generation, take_one_result(&fixture).extent_generation
  );
  mln_test_render_request_forced(&fixture, 303);
  const mln_render_frame_result new_frame = take_one_result(&fixture);
  TEST_ASSERT_GREATER_THAN_UINT64(
    old_frame.extent_generation, new_frame.extent_generation
  );
  TEST_ASSERT_GREATER_THAN_UINT64(
    old_frame.frame_generation, new_frame.frame_generation
  );
  mln_render_session_snapshot snapshot = read_snapshot(fixture.session);
  TEST_ASSERT_EQUAL_UINT32(96, snapshot.extent.width);
  TEST_ASSERT_EQUAL_UINT32(48, snapshot.extent.height);
  TEST_ASSERT_DOUBLE_WITHIN(0.0, 1.0, snapshot.extent.scale_factor);
  TEST_ASSERT_EQUAL_UINT64(
    new_frame.extent_generation, snapshot.extent_generation
  );

  mln_logical_extent second = extent;
  second.width = 128;
  second.height = 72;
  mln_test_completion first_resize = mln_test_completion_default(0);
  mln_test_completion second_resize = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_resize(
    fixture.session, extent, &first_resize.descriptor, NULL
  ));
  MLN_TEST_OK(mln_render_session_resize(
    fixture.session, second, &second_resize.descriptor, NULL
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
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_barrier(fixture.session, &completion.descriptor, NULL)
  );
  snapshot = read_snapshot(fixture.session);
  TEST_ASSERT_EQUAL_UINT32(128, snapshot.extent.width);
  TEST_ASSERT_EQUAL_UINT32(72, snapshot.extent.height);
  detach(runtime, map, &fixture);
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
// results and the observer delivery that completes the still run on the
// session scheduler, so a demand that resolves without rendering still drains
// that scheduler.
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
  detach(runtime, map, &fixture);
}

MLN_TEST_GROUP {
  RUN_TEST(frame_results_wake_the_host_and_drain_into_an_owned_batch);
  RUN_TEST(a_full_ring_parks_demands_until_a_release_or_detach);
  RUN_TEST(sustained_demands_past_the_ring_depth_keep_the_newest_frames);
  RUN_TEST(resizes_order_extent_generations_and_supersede_each_other);
  RUN_TEST(still_image_completes_under_if_needed_keepalive_demands);
}
