// Frame demands: result batches and the frame wake, the texture ring's
// backpressure, barriers and resizes ordered with demands, demands past the
// ring depth, the keep-alive demands a still image needs, demands that wait
// for a map update, and the one frame that a burst of commands renders.

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
  mln_test_render_prepare_map(*map);
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
  mln_render_frame_batch batch = mln_test_render_wait_for_results(fixture);
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

// A drain of an empty result queue reports not ready, leaves its output null,
// and clears a diagnostic left by an earlier failure. The frame wake runs when
// the queue goes from empty to nonempty, and not again for a drain. A drained
// batch is an owned handle: releasing it twice is a no-op, releasing the null
// handle is a no-op, and a released handle names no batch.
static void frame_results_wake_the_host_and_drain_into_an_owned_batch(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture);
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  // The drain checks its view before it looks for results.
  mln_render_frame_batch unread = MLN_HANDLE_NULL;
  MLN_TEST_INVALID(mln_render_session_drain_frame_results(
    fixture.session, &unread, NULL, &diagnostic
  ));
  TEST_ASSERT_GREATER_THAN_size_t(0, strlen(diagnostic.message));
  mln_render_frame_batch_view small = {.size = sizeof(small) - 1};
  MLN_TEST_INVALID(mln_render_session_drain_frame_results(
    fixture.session, &unread, &small, NULL
  ));
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, unread);
  mln_render_frame_batch empty = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_NOT_READY,
    mln_test_drain_frame_results(fixture.session, &empty, &diagnostic)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, empty);
  TEST_ASSERT_EQUAL_size_t(0, strlen(diagnostic.message));
  // A demand with an unknown flag is rejected and queues no result.
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = 1U << 8U;
  demand.token = 200;
  MLN_TEST_INVALID(
    mln_render_session_request_frame(fixture.session, &demand, NULL)
  );
  frame_wake_wait wake = {
    .fixture = &fixture, .before = atomic_load(&fixture.frame_wakes)
  };
  demand = mln_frame_demand_default();
  demand.token = 201;
  MLN_TEST_OK(mln_render_session_request_frame(fixture.session, &demand, NULL));
  MLN_TEST_OK(mln_test_render_step_until(
    &fixture, frame_woke, &wake, mln_test_deadline_default(), "a frame wake"
  ));
  const unsigned int woke = atomic_load(&fixture.frame_wakes);
  mln_render_frame_batch batch = mln_test_render_wait_for_results(&fixture);
  TEST_ASSERT_EQUAL_UINT32(woke, atomic_load(&fixture.frame_wakes));

  const mln_render_frame_batch_view view = mln_test_render_batch_view(batch);
  TEST_ASSERT_EQUAL_UINT32(sizeof(mln_render_frame_result), view.result_size);
  TEST_ASSERT_EQUAL_size_t(1, view.result_count);
  const mln_render_frame_result* result = mln_test_render_view_result(&view, 0);
  TEST_ASSERT_EQUAL_UINT32(sizeof(mln_render_frame_result), result->size);
  TEST_ASSERT_EQUAL_UINT64(201, result->token);
  mln_render_frame_batch_release(batch);
  mln_render_frame_batch_release(batch);
  mln_render_frame_batch_release(MLN_HANDLE_NULL);
  detach(runtime, map, &fixture);
}

// A barrier ends a parked demand while the host still owns both frames.
// Detach refuses until the host releases the frames.
static void a_barrier_ends_a_demand_while_the_ring_stays_acquired(void) {
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
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &barrier));
  const mln_render_frame_result cancelled = take_one_result(&fixture);
  TEST_ASSERT_EQUAL_UINT64(203, cancelled.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_NO_UPDATE, cancelled.disposition);
  mln_test_completion_destroy(&barrier);
  mln_render_session_snapshot snapshot = read_snapshot(fixture.session);
  TEST_ASSERT_EQUAL_UINT32(2, snapshot.acquired_frame_count);
  mln_completion rejected_detach = mln_test_discard_completion();
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_detach(fixture.session, &rejected_detach, NULL)
  );

  mln_test_render_release_frame(&first);
  first = mln_test_render_and_acquire(&fixture, 204);

  // Park another demand behind a full ring, then release the frames and
  // detach. The demand gets a result whichever operation reaches it first.
  mln_test_render_request_forced(&fixture, 503);
  fence_driver(&fixture);
  mln_test_render_release_frame(&first);
  mln_test_render_release_frame(&second);
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_detach(fixture.session, &completion.descriptor, NULL)
  );
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_drain_frame_results(fixture.session, &batch, NULL));
  const mln_render_frame_batch_view view = mln_test_render_batch_view(batch);
  bool reported = false;
  for (size_t index = 0; index < view.result_count; index += 1) {
    reported |= mln_test_render_view_result(&view, index)->token == 503;
  }
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_TRUE(reported);
  snapshot = read_snapshot(fixture.session);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_DETACHED, snapshot.state);
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
  mln_render_frame_result results[demand_count];
  mln_test_render_wait_for_result_list(&fixture, demand_count, results);
  uint64_t previous_generation = 0;
  for (size_t index = 0; index < demand_count; index += 1) {
    TEST_ASSERT_EQUAL_UINT64(index + 1, results[index].token);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_RESULT_RENDERED, results[index].disposition
    );
    TEST_ASSERT_GREATER_THAN_UINT64(
      previous_generation, results[index].frame_generation
    );
    previous_generation = results[index].frame_generation;
  }

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

#if defined(__EMSCRIPTEN__) && defined(MLN_FFI_TEST_BACKEND_WEBGPU)
// A WebGPU render returns before the GPU finishes its frame, and the queue
// reports it done once this thread returns to its event loop. With as many
// frames unfinished as the ring has slots the driver runs nothing more, so
// demands past the ring depth wait rather than pile more frames onto the GPU.
static void webgpu_demands_past_the_ring_depth_wait_for_the_gpu(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture);
  mln_render_session_capabilities capabilities = {
    .size = sizeof(mln_render_session_capabilities)
  };
  MLN_TEST_OK(
    mln_render_session_get_capabilities(fixture.session, &capabilities, NULL)
  );
  const uint64_t depth = capabilities.texture_ring_depth;
  for (uint64_t token = 1; token <= depth + 1; token += 1) {
    mln_test_render_request_forced(&fixture, token);
  }

  // One service runs every queued demand without returning to the event loop.
  MLN_TEST_OK(mln_test_render_fixture_service(&fixture));
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_drain_frame_results(fixture.session, &batch, NULL));
  TEST_ASSERT_EQUAL_size_t(
    depth, mln_test_render_batch_view(batch).result_count
  );
  mln_render_frame_batch_release(batch);

  batch = mln_test_render_wait_for_results(&fixture);
  const mln_render_frame_result last = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(depth + 1, last.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, last.disposition);
  detach(runtime, map, &fixture);
}

// A demand that arrives once the ring's frames are all unfinished wakes the
// driver anyway: only a service asks the queue to report them done, so a host
// that services on the driver-work wake alone would otherwise never render it.
static void webgpu_a_demand_behind_unfinished_frames_wakes_the_driver(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture);
  mln_render_session_capabilities capabilities = {
    .size = sizeof(mln_render_session_capabilities)
  };
  MLN_TEST_OK(
    mln_render_session_get_capabilities(fixture.session, &capabilities, NULL)
  );
  const uint64_t depth = capabilities.texture_ring_depth;
  for (uint64_t token = 1; token <= depth; token += 1) {
    mln_test_render_request_forced(&fixture, token);
  }
  MLN_TEST_OK(mln_test_render_fixture_service(&fixture));
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_drain_frame_results(fixture.session, &batch, NULL));
  TEST_ASSERT_EQUAL_size_t(
    depth, mln_test_render_batch_view(batch).result_count
  );
  mln_render_frame_batch_release(batch);

  const unsigned before = atomic_load(&fixture.driver_wakes);
  mln_test_render_request_forced(&fixture, depth + 1);
  TEST_ASSERT_GREATER_THAN_UINT(before, atomic_load(&fixture.driver_wakes));
  batch = mln_test_render_wait_for_results(&fixture);
  const mln_render_frame_result last = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(depth + 1, last.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, last.disposition);
  detach(runtime, map, &fixture);
}
#endif

// Resizes apply in order, preserve the scale factor, and publish target
// generations.
static void resizes_apply_in_order_and_publish_generations(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture);
  mln_test_render_request_forced(&fixture, 301);
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
    old_frame.generation, take_one_result(&fixture).generation
  );
  mln_test_render_request_forced(&fixture, 303);
  const mln_render_frame_result new_frame = take_one_result(&fixture);
  TEST_ASSERT_GREATER_THAN_UINT64(old_frame.generation, new_frame.generation);
  TEST_ASSERT_GREATER_THAN_UINT64(
    old_frame.frame_generation, new_frame.frame_generation
  );
  mln_render_session_snapshot snapshot = read_snapshot(fixture.session);
  TEST_ASSERT_EQUAL_UINT32(96, snapshot.extent.width);
  TEST_ASSERT_EQUAL_UINT32(48, snapshot.extent.height);
  TEST_ASSERT_DOUBLE_WITHIN(0.0, 1.0, snapshot.extent.scale_factor);
  TEST_ASSERT_EQUAL_UINT64(new_frame.generation, snapshot.generation);

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
  snapshot = read_snapshot(fixture.session);
  TEST_ASSERT_EQUAL_UINT32(128, snapshot.extent.width);
  TEST_ASSERT_EQUAL_UINT32(72, snapshot.extent.height);
  detach(runtime, map, &fixture);
}

// A session attached at a scale factor other than the map's resizes at its
// own scale factor. The map keeps the one it was created with and takes only
// the new width and height.
static void a_session_resizes_at_its_own_scale_factor_over_a_different_map(
  void
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.initial_extent.scale_factor = 3.0;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_test_render_prepare_map(map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  const uint64_t old_generation = read_snapshot(fixture.session).generation;

  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_resize(
      fixture.session,
      (mln_logical_extent){.width = 96, .height = 48, .scale_factor = 1.0},
      &completion.descriptor, NULL
    )
  );
  const mln_render_session_snapshot snapshot = read_snapshot(fixture.session);
  TEST_ASSERT_EQUAL_UINT32(96, snapshot.extent.width);
  TEST_ASSERT_EQUAL_UINT32(48, snapshot.extent.height);
  TEST_ASSERT_DOUBLE_WITHIN(0.0, 1.0, snapshot.extent.scale_factor);
  TEST_ASSERT_GREATER_THAN_UINT64(old_generation, snapshot.generation);
  mln_map_snapshot map_snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &map_snapshot, NULL));
  TEST_ASSERT_EQUAL_UINT32(96, map_snapshot.view.extent.width);
  TEST_ASSERT_EQUAL_UINT32(48, map_snapshot.view.extent.height);
  TEST_ASSERT_DOUBLE_WITHIN(0.0, 3.0, map_snapshot.view.extent.scale_factor);
  detach(runtime, map, &fixture);
}

static void expect_map_size(mln_map map, uint32_t width, uint32_t height) {
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
  TEST_ASSERT_EQUAL_UINT32(width, snapshot.view.extent.width);
  TEST_ASSERT_EQUAL_UINT32(height, snapshot.view.extent.height);
}

// Attaching sizes the map to the target, so the first frame renders without
// the host resizing the map.
static void attaching_sizes_the_map_to_the_target(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.initial_extent =
    (mln_logical_extent){.width = 96, .height = 48, .scale_factor = 1.0};
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_test_render_prepare_map(map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  // A later query completes after the resize that the attach submitted.
  MLN_TEST_OK(mln_test_map_fence(map));
  expect_map_size(map, 64, 64);

  mln_test_render_request_forced(&fixture, 701);
  const mln_render_frame_result result = take_one_result(&fixture);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  detach(runtime, map, &fixture);
}

// A command group holds the new render update while resize still applies on
// both the map and the driver. Later driver work and detach can run before
// the group ends.
static void a_resize_applies_while_a_command_group_holds_the_update(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture);
  MLN_TEST_AWAIT_OK(
    mln_map_begin_command_group(map, &completion.descriptor, NULL)
  );
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_resize(
      fixture.session,
      (mln_logical_extent){.width = 96, .height = 48, .scale_factor = 1.0},
      &completion.descriptor, NULL
    )
  );
  expect_map_size(map, 96, 48);
  const mln_render_session_snapshot snapshot = read_snapshot(fixture.session);
  TEST_ASSERT_EQUAL_UINT32(96, snapshot.extent.width);
  TEST_ASSERT_EQUAL_UINT32(48, snapshot.extent.height);
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_detach(fixture.session, &completion.descriptor, NULL)
  );
  MLN_TEST_AWAIT_OK(
    mln_map_end_command_group(map, &completion.descriptor, NULL)
  );
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
    const mln_status drained =
      mln_test_drain_frame_results(wait->fixture->session, &batch, NULL);
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
        mln_acquired_frame_release(frame, NULL, NULL) != MLN_STATUS_OK;
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

// A pending still image on a static map completes while the host keeps its
// own render-if-needed demands in flight, which mostly resolve on the fast
// path. Worker results and the observer delivery that completes the still run
// on the session scheduler, so a demand that resolves without rendering still
// drains that scheduler rather than strand them.
static void still_image_completes_under_if_needed_keepalive_demands(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.map_mode = MLN_MAP_MODE_STATIC;
  options.initial_extent.width = 64;
  options.initial_extent.height = 64;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_test_render_prepare_map(map);
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

// Requests a demand that renders only if needed and waits for a map update.
static void request_waiting(
  const mln_test_render_fixture* fixture, uint64_t token
) {
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_IF_NEEDED | MLN_FRAME_DEMAND_WAIT_FOR_UPDATE;
  demand.token = token;
  MLN_TEST_OK(
    mln_render_session_request_frame(fixture->session, &demand, NULL)
  );
}

// The fence runs after the driver evaluated every earlier demand, so a
// demand that is still pending then waits for a map update. Dumping debug
// logs publishes no map update, which would end the wait.
static void expect_waiting(const mln_test_render_fixture* fixture) {
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, fixture,
    mln_render_session_dump_debug_logs(
      fixture->session, &completion.descriptor, NULL
    )
  );
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_NOT_READY,
    mln_test_drain_frame_results(fixture->session, &batch, NULL)
  );
}

static void move_camera(mln_map map, double zoom) {
  mln_camera_update update = mln_camera_update_default();
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = zoom;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

// A demand that waits for a map update finds nothing to render on an idle
// map and stays pending until the map publishes an update, which it then
// renders without another demand.
static void a_waiting_demand_renders_the_next_map_update(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture);
  (void)mln_test_render_until_idle(runtime, map, &fixture, NULL);
  const uint64_t idle_generation =
    read_snapshot(fixture.session).rendered_update_generation;

  request_waiting(&fixture, 601);
  expect_waiting(&fixture);
  move_camera(map, 2.0);
  const mln_render_frame_result result = take_one_result(&fixture);
  TEST_ASSERT_EQUAL_UINT64(601, result.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  TEST_ASSERT_GREATER_THAN_UINT64(
    idle_generation, result.map_update_generation
  );
  detach(runtime, map, &fixture);
}

static const char eight_layer_style_json[] =
  "{\"version\":8,\"transition\":{\"duration\":0,\"delay\":0},"
  "\"sources\":{},\"layers\":["
  "{\"id\":\"0\",\"type\":\"background\"},"
  "{\"id\":\"1\",\"type\":\"background\"},"
  "{\"id\":\"2\",\"type\":\"background\"},"
  "{\"id\":\"3\",\"type\":\"background\"},"
  "{\"id\":\"4\",\"type\":\"background\"},"
  "{\"id\":\"5\",\"type\":\"background\"},"
  "{\"id\":\"6\",\"type\":\"background\"},"
  "{\"id\":\"7\",\"type\":\"background\"}]}";

// Commands that the runtime worker already holds announce their render
// updates once, after the last of them, so a waiting demand renders them in
// one frame rather than a frame for the first and another for the rest.
static void a_burst_of_held_commands_renders_in_one_frame(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(
    mln_test_map_set_style_json(map, MLN_BUFFER_LITERAL(eight_layer_style_json))
  );
  MLN_TEST_OK(mln_test_map_fence(map));
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  (void)mln_test_render_until_idle(runtime, map, &fixture, NULL);
  request_waiting(&fixture, 621);
  expect_waiting(&fixture);

  // A query publishes no render update, so parking the worker in its
  // completion queues the burst behind it with nothing announced ahead.
  mln_test_gate gate;
  mln_test_gate_init(&gate);
  const mln_completion hold = mln_test_gate_completion(&gate);
  MLN_TEST_OK(mln_map_get_camera(map, &hold, NULL));
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_gate_wait_entered(&gate), "the runtime worker never parked"
  );
  static const char layer_ids[] = "01234567";
  mln_test_completion commands[sizeof(layer_ids) - 1];
  for (size_t index = 0; index < sizeof(layer_ids) - 1; index += 1) {
    commands[index] = mln_test_completion_default(0);
    MLN_TEST_OK(mln_map_set_style_layer_property(
      map, mln_test_buffer_view(&layer_ids[index], 1),
      MLN_BUFFER_LITERAL("background-color"), MLN_BUFFER_LITERAL("\"#336699\""),
      &commands[index].descriptor, NULL
    ));
  }
  mln_test_gate_release(&gate);
  for (size_t index = 0; index < sizeof(layer_ids) - 1; index += 1) {
    MLN_TEST_OK(mln_test_completion_finish(&commands[index]));
    mln_test_completion_destroy(&commands[index]);
  }

  mln_map_snapshot snapshot = {.size = sizeof(snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
  const mln_render_frame_result rendered = take_one_result(&fixture);
  TEST_ASSERT_EQUAL_UINT64(621, rendered.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, rendered.disposition);
  TEST_ASSERT_EQUAL_UINT64(
    snapshot.latest_render_update_generation, rendered.map_update_generation
  );
  request_waiting(&fixture, 622);
  expect_waiting(&fixture);
  detach(runtime, map, &fixture);
}

// A newer matching demand supersedes a waiting one, a barrier ends the wait
// with no update, and detach ends it with target not ready. A demand that
// waits must also render only if needed.
static void a_waiting_demand_ends_by_supersession_barrier_or_detach(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture);
  mln_frame_demand forced_wait = mln_frame_demand_default();
  forced_wait.flags = MLN_FRAME_DEMAND_WAIT_FOR_UPDATE;
  MLN_TEST_INVALID(
    mln_render_session_request_frame(fixture.session, &forced_wait, NULL)
  );
  (void)mln_test_render_until_idle(runtime, map, &fixture, NULL);

  request_waiting(&fixture, 611);
  expect_waiting(&fixture);
  request_waiting(&fixture, 612);
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_barrier(fixture.session, &completion.descriptor, NULL)
  );
  mln_render_frame_result results[2];
  mln_test_render_wait_for_result_list(&fixture, 2, results);
  const mln_render_frame_result superseded = results[0];
  const mln_render_frame_result ended = results[1];
  TEST_ASSERT_EQUAL_UINT64(611, superseded.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_SUPERSEDED, superseded.disposition
  );
  TEST_ASSERT_EQUAL_UINT64(612, ended.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_NO_UPDATE, ended.disposition);

  request_waiting(&fixture, 613);
  expect_waiting(&fixture);
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_detach(fixture.session, &completion.descriptor, NULL)
  );
  const mln_render_frame_result stranded = take_one_result(&fixture);
  TEST_ASSERT_EQUAL_UINT64(613, stranded.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_TARGET_NOT_READY, stranded.disposition
  );
  detach(runtime, map, &fixture);
}

MLN_TEST_GROUP {
  RUN_TEST(frame_results_wake_the_host_and_drain_into_an_owned_batch);
  RUN_TEST(a_barrier_ends_a_demand_while_the_ring_stays_acquired);
  RUN_TEST(sustained_demands_past_the_ring_depth_keep_the_newest_frames);
#if defined(__EMSCRIPTEN__) && defined(MLN_FFI_TEST_BACKEND_WEBGPU)
  RUN_TEST(webgpu_demands_past_the_ring_depth_wait_for_the_gpu);
  RUN_TEST(webgpu_a_demand_behind_unfinished_frames_wakes_the_driver);
#endif
  RUN_TEST(resizes_apply_in_order_and_publish_generations);
  RUN_TEST(a_session_resizes_at_its_own_scale_factor_over_a_different_map);
  RUN_TEST(attaching_sizes_the_map_to_the_target);
  RUN_TEST(a_resize_applies_while_a_command_group_holds_the_update);
  RUN_TEST(still_image_completes_under_if_needed_keepalive_demands);
  RUN_TEST(a_waiting_demand_renders_the_next_map_update);
  RUN_TEST(a_waiting_demand_ends_by_supersession_barrier_or_detach);
  RUN_TEST(a_burst_of_held_commands_renders_in_one_frame);
}
