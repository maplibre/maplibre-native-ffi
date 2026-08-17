// Raw C ABI coverage for core-worker and caller-graphics-thread sessions.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"
static const char red_background_style[] =
  "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"bg\","
  "\"type\":\"background\",\"paint\":{\"background-color\":\"#ff0000\"}}]}";

static void prepare_renderable_map(mln_runtime runtime, mln_map map) {
  static const char empty_style[] =
    "{\"version\":8,\"sources\":{},\"layers\":[]}";
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_map_set_style_json(map, MLN_BUFFER_LITERAL(empty_style))
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
}

static bool service_fixture(const mln_test_render_fixture* fixture) {
  if (fixture->driver != MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
    return true;
  }
  size_t serviced = 0;
  return mln_render_session_service_driver_work(
           fixture->session, SIZE_MAX, &serviced
         ) == MLN_STATUS_OK;
}

static bool wait_for_results(
  const mln_test_render_fixture* fixture, size_t minimum,
  mln_render_frame_batch* out_batch
) {
  for (unsigned int attempt = 0; attempt < 10000; attempt += 1) {
    if (!service_fixture(fixture)) {
      return false;
    }
    mln_render_session_snapshot snapshot = {
      .size = sizeof(mln_render_session_snapshot)
    };
    if (
      mln_render_session_get_snapshot(fixture->session, &snapshot) !=
        MLN_STATUS_OK ||
      snapshot.pending_demand_count != 0
    ) {
      mln_test_sleep_millisecond();
      continue;
    }
    mln_render_frame_batch batch = MLN_HANDLE_NULL;
    const mln_status status =
      mln_render_session_drain_frame_results(fixture->session, &batch);
    if (status == MLN_STATUS_OK) {
      size_t count = 0;
      if (
        mln_render_frame_batch_count(batch, &count) == MLN_STATUS_OK &&
        count >= minimum
      ) {
        *out_batch = batch;
        return true;
      }
      mln_render_frame_batch_release(batch);
    } else if (status != MLN_STATUS_NOT_READY) {
      return false;
    }
    mln_test_sleep_millisecond();
  }
  return false;
}

static bool source_reports_kind(mln_notification_source source, uint32_t kind) {
  mln_ready_batch batch = MLN_HANDLE_NULL;
  if (mln_notification_source_drain_ready(source, &batch) != MLN_STATUS_OK) {
    return false;
  }
  mln_ready_batch_view view = {.size = sizeof(mln_ready_batch_view)};
  bool found = false;
  if (mln_ready_batch_get(batch, &view) == MLN_STATUS_OK) {
    for (size_t index = 0; index < view.endpoint_count; index += 1) {
      const mln_ready_endpoint* endpoint =
        (const mln_ready_endpoint*)((const char*)view.endpoints +
                                    index * view.endpoint_size);
      found = found || endpoint->kind == kind;
    }
  }
  mln_ready_batch_release(batch);
  return found;
}

static mln_render_frame_result batch_result(
  mln_render_frame_batch batch, size_t index
) {
  mln_render_frame_result result = {.size = sizeof(mln_render_frame_result)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_frame_batch_get(batch, index, &result)
  );
  return result;
}

static void attach_reports_the_selected_native_driver(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_render_session_capabilities capabilities = {
    .size = sizeof(mln_render_session_capabilities)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_render_session_get_capabilities(fixture.session, &capabilities)
  );
  TEST_ASSERT_EQUAL_UINT32(fixture.driver, capabilities.driver);
  TEST_ASSERT_EQUAL_UINT32(2, capabilities.texture_ring_depth);
  TEST_ASSERT_BITS_HIGH(
    MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION |
      MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC,
    capabilities.flags
  );
#if defined(MLN_FFI_TEST_BACKEND_OPENGL) || defined(MLN_FFI_TEST_BACKEND_WEBGPU)
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD, capabilities.driver
  );
  TEST_ASSERT_TRUE(fixture.observed_attaching);
  TEST_ASSERT_TRUE(fixture.observed_driver_ready);
#else
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_DRIVER_CORE_WORKER, capabilities.driver);
  size_t serviced = 99;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_service_driver_work(fixture.session, 1, &serviced)
  );
#endif

  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_get_snapshot(fixture.session, &snapshot)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_ATTACHED, snapshot.state);
  TEST_ASSERT_EQUAL_UINT32(capabilities.driver, snapshot.driver);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void demand_coalescing_preserves_boundaries_and_generations(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  prepare_renderable_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  atomic_bool entered;
  atomic_bool release;
  atomic_init(&entered, false);
  atomic_init(&release, false);
  mln_operation blocker = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_session_blocking_operation_create(
                     fixture.session, &entered, &release, &blocker
                   )
  );
  if (fixture.driver == MLN_RENDER_DRIVER_CORE_WORKER) {
    for (unsigned int attempt = 0; attempt < 10000 && !atomic_load(&entered);
         attempt += 1) {
      mln_test_sleep_millisecond();
    }
    TEST_ASSERT_TRUE(atomic_load(&entered));
  }

  mln_frame_demand first = mln_frame_demand_default();
  first.flags = 0;
  first.token = 101;
  first.coalescing_boundary = 7;
  first.presentation_time_ns = 1000;
  mln_frame_demand newest = first;
  newest.token = 102;
  newest.presentation_time_ns = 2000;
  mln_frame_demand separate = newest;
  separate.token = 103;
  separate.coalescing_boundary = 8;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_request_frame(fixture.session, &first)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_request_frame(fixture.session, &newest)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_request_frame(fixture.session, &separate)
  );
  atomic_store(&release, true);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, blocker)
  );
  mln_operation_release(blocker);

  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  TEST_ASSERT_TRUE(wait_for_results(&fixture, 3, &batch));
  mln_frame_demand later = separate;
  later.token = 104;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_request_frame(fixture.session, &later)
  );
  mln_render_frame_batch second_batch = MLN_HANDLE_NULL;
  TEST_ASSERT_TRUE(wait_for_results(&fixture, 1, &second_batch));
  TEST_ASSERT_EQUAL_UINT64(104, batch_result(second_batch, 0).token);
  mln_render_frame_batch_release(second_batch);
  const mln_render_frame_result superseded = batch_result(batch, 0);
  const mln_render_frame_result rendered = batch_result(batch, 1);
  const mln_render_frame_result boundary = batch_result(batch, 2);
  TEST_ASSERT_EQUAL_UINT64(101, superseded.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_SUPERSEDED, superseded.disposition
  );
  TEST_ASSERT_EQUAL_UINT64(102, rendered.token);
  TEST_ASSERT_EQUAL_UINT64(103, boundary.token);
  TEST_ASSERT_EQUAL_INT64(2000, rendered.presentation_time_ns);
  TEST_ASSERT_GREATER_THAN_UINT64(0, rendered.extent_generation);
  TEST_ASSERT_GREATER_THAN_UINT64(0, rendered.map_update_generation);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64(
    rendered.map_update_generation, boundary.map_update_generation
  );
  TEST_ASSERT_GREATER_THAN_UINT64(0, rendered.frame_generation);
  mln_render_frame_batch_release(batch);

  mln_frame_demand expired = mln_frame_demand_default();
  expired.flags = 0;
  expired.token = 104;
  expired.deadline_ns =
    (int64_t)(mln_test_monotonic_milliseconds() * UINT64_C(1000000)) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_request_frame(fixture.session, &expired)
  );
  batch = MLN_HANDLE_NULL;
  TEST_ASSERT_TRUE(wait_for_results(&fixture, 1, &batch));
  const mln_render_frame_result missed = batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT64(104, missed.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_DEADLINE_MISSED, missed.disposition
  );
  mln_render_frame_batch_release(batch);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_acquired_frame render_and_acquire(
  const mln_test_render_fixture* fixture, uint64_t token
) {
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = 0;
  demand.token = token;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_request_frame(fixture->session, &demand)
  );
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  TEST_ASSERT_TRUE(wait_for_results(fixture, 1, &batch));
  const mln_render_frame_result result = batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  mln_render_frame_batch_release(batch);
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_acquire_frame(fixture->session, &frame)
  );
  TEST_ASSERT_NOT_EQUAL(MLN_HANDLE_NULL, frame);
  return frame;
}
static void frame_readiness_is_level_triggered_until_results_are_drained(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  prepare_renderable_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_request_frame(fixture.session, &demand)
  );
  bool ready = false;
  for (unsigned int attempt = 0; attempt < 10000 && !ready; attempt += 1) {
    TEST_ASSERT_TRUE(service_fixture(&fixture));
    ready = source_reports_kind(
      fixture.source, MLN_NOTIFICATION_ENDPOINT_RENDER_FRAMES
    );
    if (!ready) {
      mln_test_sleep_millisecond();
    }
  }
  TEST_ASSERT_TRUE(ready);
  TEST_ASSERT_TRUE(
    source_reports_kind(fixture.source, MLN_NOTIFICATION_ENDPOINT_RENDER_FRAMES)
  );
  mln_render_frame_batch results = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_render_session_drain_frame_results(fixture.session, &results)
  );
  mln_render_frame_batch_release(results);
  TEST_ASSERT_FALSE(
    source_reports_kind(fixture.source, MLN_NOTIFICATION_ENDPOINT_RENDER_FRAMES)
  );
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void texture_ring_leases_apply_backpressure_until_cpu_release(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  prepare_renderable_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_acquired_frame first = render_and_acquire(&fixture, 201);
  mln_acquired_frame second = render_and_acquire(&fixture, 202);
  mln_gpu_sync producer = mln_gpu_sync_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_acquired_frame_get_producer_sync(first, &producer)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_GPU_SYNC_CPU_COMPLETE, producer.kind);

  mln_frame_demand blocked = mln_frame_demand_default();
  blocked.flags = 0;
  blocked.token = 203;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_request_frame(fixture.session, &blocked)
  );
  TEST_ASSERT_TRUE(service_fixture(&fixture));
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_get_snapshot(fixture.session, &snapshot)
  );
  TEST_ASSERT_EQUAL_UINT32(2, snapshot.acquired_frame_count);
  mln_operation rejected_detach = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_detach_start(fixture.session, &rejected_detach)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, rejected_detach);
  TEST_ASSERT_EQUAL_UINT32(1, snapshot.pending_demand_count);

  mln_gpu_sync cpu_complete = mln_gpu_sync_default();
  mln_operation release = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_acquired_frame_release_start(&first, &cpu_complete, &release)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, first);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, release)
  );
  mln_operation_release(release);

  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  TEST_ASSERT_TRUE(wait_for_results(&fixture, 1, &batch));
  TEST_ASSERT_EQUAL_UINT64(203, batch_result(batch, 0).token);
  mln_render_frame_batch_release(batch);

  release = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_acquired_frame_release_start(&second, &cpu_complete, &release)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, release)
  );
  mln_operation_release(release);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void acquired_frame_release_after_abandon_is_cpu_only_target_loss(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  prepare_renderable_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_acquired_frame frame = render_and_acquire(&fixture, 250);

  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_abandon(fixture.session, &abandoned)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED, abandoned.disposition
  );
  TEST_ASSERT_GREATER_THAN_UINT32(0, abandoned.quarantined_resource_count);
  mln_render_frame_result invalid = {.size = sizeof(mln_render_frame_result)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_TARGET_LOST, mln_acquired_frame_get_result(frame, &invalid)
  );
  mln_gpu_sync sync = mln_gpu_sync_default();
  mln_operation release = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_acquired_frame_release_start(&frame, &sync, &release)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);
  bool completed = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_operation_wait(release, 10000, &completed)
  );
  TEST_ASSERT_TRUE(completed);
  mln_status terminal = MLN_STATUS_OK;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_operation_get_status(release, &terminal)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_TARGET_LOST, terminal);
  mln_operation_release(release);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void texture_readback_is_an_ordered_owned_operation_result(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_map_set_style_json(map, MLN_BUFFER_LITERAL(red_background_style))
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_acquired_frame frame = render_and_acquire(&fixture, 260);

  mln_operation readback = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_texture_read_premultiplied_rgba8_start(fixture.session, &readback)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, readback)
  );
  mln_buffer pixels = MLN_HANDLE_NULL;
  mln_texture_image_info info = mln_texture_image_info_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_texture_read_premultiplied_rgba8_take_result(readback, &pixels, &info)
  );
  mln_operation_release(readback);
  TEST_ASSERT_EQUAL_UINT32(64, info.width);
  TEST_ASSERT_EQUAL_UINT32(64, info.height);
  mln_buffer_view bytes = {0};
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_buffer_get(pixels, &bytes));
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(4, (uint32_t)bytes.size);
  TEST_ASSERT_EQUAL_UINT8(255, ((const uint8_t*)bytes.data)[0]);
  TEST_ASSERT_EQUAL_UINT8(0, ((const uint8_t*)bytes.data)[1]);
  TEST_ASSERT_EQUAL_UINT8(0, ((const uint8_t*)bytes.data)[2]);
  TEST_ASSERT_EQUAL_UINT8(255, ((const uint8_t*)bytes.data)[3]);
  mln_buffer_destroy(pixels);

  mln_operation release = MLN_HANDLE_NULL;
  mln_gpu_sync sync = mln_gpu_sync_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_acquired_frame_release_start(&frame, &sync, &release)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, release)
  );
  mln_operation_release(release);
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void resize_and_barrier_order_frame_and_extent_generations(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  prepare_renderable_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_frame_demand before = mln_frame_demand_default();
  before.flags = 0;
  before.token = 301;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_request_frame(fixture.session, &before)
  );
  mln_operation barrier = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_render_session_barrier_start(fixture.session, 0, &barrier)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, barrier)
  );
  mln_operation_release(barrier);
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  TEST_ASSERT_TRUE(wait_for_results(&fixture, 1, &batch));
  const mln_render_frame_result old_frame = batch_result(batch, 0);
  mln_render_frame_batch_release(batch);

  const mln_render_target_extent extent = {
    .size = sizeof(mln_render_target_extent),
    .width = 96,
    .height = 48,
    .scale_factor = 2.0,
  };
  mln_frame_demand during = mln_frame_demand_default();
  during.flags = 0;
  during.token = 302;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_request_frame(fixture.session, &during)
  );
  mln_operation resize = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_render_session_resize_start(fixture.session, &extent, &resize)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, resize)
  );
  mln_operation_release(resize);
  batch = MLN_HANDLE_NULL;
  TEST_ASSERT_TRUE(wait_for_results(&fixture, 1, &batch));
  const mln_render_frame_result resizing_frame = batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT64(
    old_frame.extent_generation, resizing_frame.extent_generation
  );
  mln_render_frame_batch_release(batch);

  mln_frame_demand after = mln_frame_demand_default();
  after.flags = 0;
  after.token = 303;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_request_frame(fixture.session, &after)
  );
  batch = MLN_HANDLE_NULL;
  TEST_ASSERT_TRUE(wait_for_results(&fixture, 1, &batch));
  const mln_render_frame_result new_frame = batch_result(batch, 0);
  TEST_ASSERT_GREATER_THAN_UINT64(
    old_frame.extent_generation, new_frame.extent_generation
  );
  TEST_ASSERT_GREATER_THAN_UINT64(
    old_frame.frame_generation, new_frame.frame_generation
  );
  mln_render_frame_batch_release(batch);

  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_get_snapshot(fixture.session, &snapshot)
  );
  TEST_ASSERT_EQUAL_UINT32(96, snapshot.extent.width);
  TEST_ASSERT_EQUAL_UINT32(48, snapshot.extent.height);
  TEST_ASSERT_DOUBLE_WITHIN(0.0, 2.0, snapshot.extent.scale_factor);
  TEST_ASSERT_EQUAL_UINT64(
    new_frame.extent_generation, snapshot.extent_generation
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct foreign_driver_probe {
  mln_render_session session;
  atomic_bool done;
  mln_status status;
} foreign_driver_probe;

static void service_from_foreign_thread(void* argument) {
  foreign_driver_probe* probe = argument;
  size_t serviced = 0;
  probe->status =
    mln_render_session_service_driver_work(probe->session, 1, &serviced);
  atomic_store(&probe->done, true);
}

static void driver_service_fixes_and_enforces_graphics_thread_identity(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  foreign_driver_probe probe = {.session = fixture.session};
  atomic_init(&probe.done, false);
  mln_test_thread* thread =
    mln_test_thread_start(service_from_foreign_thread, &probe);
  TEST_ASSERT_TRUE(mln_test_wait_until(runtime, &probe.done));
  mln_test_thread_join(thread);
  TEST_ASSERT_EQUAL_INT(
    fixture.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      ? MLN_STATUS_WRONG_THREAD
      : MLN_STATUS_INVALID_STATE,
    probe.status
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct abandon_busy_probe {
  mln_render_session session;
  atomic_bool* entered;
  atomic_bool* release;
  mln_status status;
} abandon_busy_probe;

static void abandon_when_driver_enters(void* argument) {
  abandon_busy_probe* probe = argument;
  while (!atomic_load(probe->entered)) {
    mln_test_sleep_millisecond();
  }
  mln_render_abandon_result result = {
    .size = sizeof(mln_render_abandon_result)
  };
  probe->status = mln_render_session_abandon(probe->session, &result);
  atomic_store(probe->release, true);
}

static void abandon_is_busy_during_a_driver_call_and_changes_nothing(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  atomic_bool entered;
  atomic_bool release;
  atomic_init(&entered, false);
  atomic_init(&release, false);
  mln_operation operation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_session_blocking_operation_create(
                     fixture.session, &entered, &release, &operation
                   )
  );

  mln_status abandon_status = MLN_STATUS_NATIVE_ERROR;
  if (fixture.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
    abandon_busy_probe probe = {
      .session = fixture.session, .entered = &entered, .release = &release
    };
    mln_test_thread* thread =
      mln_test_thread_start(abandon_when_driver_enters, &probe);
    size_t serviced = 0;
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_render_session_service_driver_work(
                       fixture.session, SIZE_MAX, &serviced
                     )
    );
    mln_test_thread_join(thread);
    abandon_status = probe.status;
  } else {
    for (unsigned int attempt = 0; attempt < 10000 && !atomic_load(&entered);
         attempt += 1) {
      mln_test_sleep_millisecond();
    }
    TEST_ASSERT_TRUE(atomic_load(&entered));
    mln_render_abandon_result result = {
      .size = sizeof(mln_render_abandon_result)
    };
    abandon_status = mln_render_session_abandon(fixture.session, &result);
    atomic_store(&release, true);
  }
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_BUSY, abandon_status);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, operation)
  );
  mln_operation_release(operation);
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_get_snapshot(fixture.session, &snapshot)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_ATTACHED, snapshot.state);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void receiver_loss_abandons_pending_work_and_invalidates_accessors(
  void
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_operation pending = MLN_HANDLE_NULL;
  if (fixture.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK,
      mln_render_session_reduce_memory_use_start(fixture.session, &pending)
    );
  }
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, mln_notification_source_close(fixture.source)
  );
  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_abandon(fixture.session, &abandoned)
  );
  TEST_ASSERT_TRUE(
    abandoned.disposition == MLN_RENDER_ABANDON_DISPOSITION_CLEAN ||
    abandoned.disposition == MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED
  );
  if (pending != MLN_HANDLE_NULL) {
    bool completed = false;
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_operation_wait(pending, 10000, &completed)
    );
    TEST_ASSERT_TRUE(completed);
    mln_status terminal = MLN_STATUS_OK;
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_operation_get_status(pending, &terminal)
    );
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_TARGET_LOST, terminal);
    mln_operation_release(pending);
  }

  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_get_snapshot(fixture.session, &snapshot)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_ABANDONED, snapshot.state);
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_acquire_frame(fixture.session, &frame)
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#if defined(MLN_FFI_TEST_BACKEND_OPENGL) && defined(MLN_FFI_TEST_OPENGL_WEBGL)
static void transferred_offscreen_canvas_runs_on_core_worker(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  prepare_renderable_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_transferred_webgl_surface_create(map, &fixture));

  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_get_snapshot(fixture.session, &snapshot)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_DRIVER_CORE_WORKER, snapshot.driver);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_ATTACHED, snapshot.state);

  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = 0;
  demand.token = 901;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_request_frame(fixture.session, &demand)
  );
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  TEST_ASSERT_TRUE(wait_for_results(&fixture, 1, &batch));
  const mln_render_frame_result result = batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  TEST_ASSERT_EQUAL_UINT64(901, result.token);
  mln_render_frame_batch_release(batch);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}
#endif

void run_render_thread_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(attach_reports_the_selected_native_driver);
  RUN_TEST(demand_coalescing_preserves_boundaries_and_generations);
  RUN_TEST(driver_service_fixes_and_enforces_graphics_thread_identity);
  RUN_TEST(abandon_is_busy_during_a_driver_call_and_changes_nothing);
  RUN_TEST(frame_readiness_is_level_triggered_until_results_are_drained);
  RUN_TEST(texture_ring_leases_apply_backpressure_until_cpu_release);
  RUN_TEST(texture_readback_is_an_ordered_owned_operation_result);
  RUN_TEST(resize_and_barrier_order_frame_and_extent_generations);
  RUN_TEST(receiver_loss_abandons_pending_work_and_invalidates_accessors);
  RUN_TEST(acquired_frame_release_after_abandon_is_cpu_only_target_loss);
#if defined(MLN_FFI_TEST_BACKEND_OPENGL) && defined(MLN_FFI_TEST_OPENGL_WEBGL)
  RUN_TEST(transferred_offscreen_canvas_runs_on_core_worker);
#endif
}
