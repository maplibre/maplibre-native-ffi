// Raw C ABI coverage for the batch event drain and the two subscription masks:
// batch layout and lifetime, bounded drains, mask validation, and the
// suppression a cleared mask bit causes at push time.

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

static const char background_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"background\",\"type\":"
  "\"background\",\"paint\":{\"background-color\":\"#102030\"}}]}";
static const uint64_t unknown_mask_bit = UINT64_C(1) << 63U;
static const size_t style_pump_attempts = 200;
static const int64_t park_timeout_milliseconds = 200;
static const uint64_t park_floor_milliseconds = 100;
static const char missing_database_path[] = "does-not-exist.db";
static const size_t take_result_attempts = 5000;

// The record layout every binding probes. A host reads a batch as two byte
// ranges at these offsets, so a change here is an ABI break.
static_assert(
  sizeof(mln_rendering_stats) == 40, "mln_rendering_stats is 40 bytes wide"
);
static_assert(
  sizeof(mln_runtime_event_render_frame) == 48,
  "mln_runtime_event_render_frame is 48 bytes wide"
);
static_assert(
  sizeof(mln_runtime_event_render_map) == 4,
  "mln_runtime_event_render_map is 4 bytes wide"
);
static_assert(sizeof(mln_tile_id) == 20, "mln_tile_id is 20 bytes wide");
static_assert(
  sizeof(mln_runtime_event_tile_action) == 24,
  "mln_runtime_event_tile_action is 24 bytes wide"
);
static_assert(
  sizeof(mln_runtime_event_camera_transition_finished) == 8,
  "mln_runtime_event_camera_transition_finished is 8 bytes wide"
);
static_assert(
  sizeof(mln_offline_region_status) == 64,
  "mln_offline_region_status is 64 bytes wide"
);
static_assert(
  sizeof(mln_runtime_event_offline_region_status) == 72,
  "mln_runtime_event_offline_region_status is 72 bytes wide"
);
static_assert(
  sizeof(mln_runtime_event_offline_region_response_error) == 16,
  "mln_runtime_event_offline_region_response_error is 16 bytes wide"
);
static_assert(
  sizeof(mln_runtime_event_offline_region_tile_count_limit) == 16,
  "mln_runtime_event_offline_region_tile_count_limit is 16 bytes wide"
);
static_assert(
  sizeof(mln_runtime_event_offline_operation_completed) == 24,
  "mln_runtime_event_offline_operation_completed is 24 bytes wide"
);
static_assert(
  sizeof(mln_runtime_event_payload) == 72,
  "mln_runtime_event_payload is 72 bytes wide"
);
static_assert(
  _Alignof(mln_runtime_event_payload) == 8,
  "mln_runtime_event_payload is 8-byte aligned"
);
static_assert(
  sizeof(mln_runtime_event) == 104, "mln_runtime_event is 104 bytes wide"
);
static_assert(
  _Alignof(mln_runtime_event) == 8, "mln_runtime_event is 8-byte aligned"
);
static_assert(
  offsetof(mln_runtime_event, type) == 0,
  "mln_runtime_event.type sits at offset 0"
);
static_assert(
  offsetof(mln_runtime_event, source_type) == 4,
  "mln_runtime_event.source_type sits at offset 4"
);
static_assert(
  offsetof(mln_runtime_event, source) == 8,
  "mln_runtime_event.source sits at offset 8"
);
static_assert(
  offsetof(mln_runtime_event, code) == 16,
  "mln_runtime_event.code sits at offset 16"
);
static_assert(
  offsetof(mln_runtime_event, payload_type) == 20,
  "mln_runtime_event.payload_type sits at offset 20"
);
static_assert(
  offsetof(mln_runtime_event, message_offset) == 24,
  "mln_runtime_event.message_offset sits at offset 24"
);
static_assert(
  offsetof(mln_runtime_event, message_size) == 28,
  "mln_runtime_event.message_size sits at offset 28"
);
static_assert(
  offsetof(mln_runtime_event, payload) == 32,
  "mln_runtime_event.payload sits at offset 32"
);
static_assert(
  sizeof(mln_map_options) == 40, "mln_map_options is 40 bytes wide"
);
static_assert(
  offsetof(mln_map_options, event_mask) == 32,
  "mln_map_options.event_mask sits at offset 32"
);
static_assert(
  offsetof(mln_runtime_options, flags) == 4,
  "mln_runtime_options.flags sits at offset 4"
);
static_assert(
  MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS == UINT64_C(0x87FFFE),
  "MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS is 0x87FFFE"
);
static_assert(
  MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS == UINT64_C(0x780000),
  "MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS is 0x780000"
);
static_assert(
  MLN_RUNTIME_EVENT_MASK_ALL == UINT64_C(0xFFFFFE),
  "MLN_RUNTIME_EVENT_MASK_ALL is 0xFFFFFE"
);

// These three carry pointers or size_t, so their extents follow the target's
// pointer width. Every field above is fixed width, so one probed offset table
// serves every ABI, wasm32 included.
#if UINTPTR_MAX == UINT64_MAX
static_assert(
  sizeof(mln_runtime_event_batch) == 48,
  "mln_runtime_event_batch is 48 bytes wide"
);
static_assert(
  sizeof(mln_runtime_options) == 32, "mln_runtime_options is 32 bytes wide"
);
static_assert(
  offsetof(mln_runtime_options, event_mask) == 24,
  "mln_runtime_options.event_mask sits at offset 24"
);
#endif

static const mln_runtime_event* batch_event(
  const mln_runtime_event_batch* batch, size_t index
) {
  return (const mln_runtime_event*)((const char*)batch->events +
                                    (index * batch->event_size));
}

// Loads an inline style and pumps until its events are queued, so a test that
// needs a populated queue does not depend on the network.
static void load_style_and_pump(mln_runtime runtime, mln_map map) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_style_json(map, MLN_BUFFER_LITERAL(background_style_json))
  );
  for (size_t attempt = 0; attempt < style_pump_attempts; attempt += 1) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 2, -1));
  }
}

static mln_map_options map_options_with_event_mask(uint64_t mask) {
  mln_map_options options = mln_map_options_default();
  options.event_mask = mask;
  return options;
}

// A batch header carries its own size, so a host built against an older header
// passes a shorter record than this one. Rejecting it keeps the drain from
// writing fields the caller does not own.
static void a_drain_rejects_an_undersized_batch_or_a_stale_runtime(void) {
  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_drain_events(runtime, 0, NULL)
  );
  mln_runtime_event_batch small = mln_runtime_event_batch_default();
  small.size = sizeof(mln_runtime_event_batch) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_drain_events(runtime, 0, &small)
  );

  mln_test_destroy_runtime(runtime);
  mln_runtime_event_batch batch = mln_runtime_event_batch_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_drain_events(runtime, 0, &batch)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_set_event_mask(runtime, MLN_RUNTIME_EVENT_MASK_ALL)
  );
}

typedef struct foreign_thread_probe {
  mln_runtime runtime;
  mln_map map;
  mln_status drain_status;
  mln_status runtime_mask_status;
  mln_status map_mask_status;
  atomic_bool finished;
} foreign_thread_probe;

static void call_event_api_from_a_foreign_thread(void* argument) {
  foreign_thread_probe* probe = argument;
  mln_runtime_event_batch batch = mln_runtime_event_batch_default();
  probe->drain_status = mln_runtime_drain_events(probe->runtime, 0, &batch);
  probe->runtime_mask_status =
    mln_runtime_set_event_mask(probe->runtime, MLN_RUNTIME_EVENT_MASK_ALL);
  probe->map_mask_status =
    mln_map_set_event_mask(probe->map, MLN_RUNTIME_EVENT_MASK_ALL);
  atomic_store(&probe->finished, true);
}

static void event_entry_points_reject_a_foreign_thread(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  foreign_thread_probe probe = {.runtime = runtime, .map = map};
  atomic_init(&probe.finished, false);
  mln_test_thread* thread =
    mln_test_thread_start(call_event_api_from_a_foreign_thread, &probe);
  TEST_ASSERT_TRUE(mln_test_pump_until(runtime, &probe.finished));
  mln_test_thread_join(thread);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_WRONG_THREAD, probe.drain_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_WRONG_THREAD, probe.runtime_mask_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_WRONG_THREAD, probe.map_mask_status);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Each setter reads only its own group's bits but stores the whole value, so a
// host that reads a mask, sets one bit, and writes it back keeps every other.
static void both_mask_setters_reject_unknown_bits_and_keep_foreign_ones(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_set_event_mask(runtime, unknown_mask_bit)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_event_mask(map, unknown_mask_bit)
  );

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_runtime_set_event_mask(runtime, MLN_RUNTIME_EVENT_MASK_ALL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_event_mask(map, MLN_RUNTIME_EVENT_MASK_ALL)
  );

  uint64_t runtime_mask = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_get_event_mask(runtime, &runtime_mask)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_RUNTIME_EVENT_MASK_ALL, runtime_mask);
  uint64_t map_mask = 0;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_get_event_mask(map, &map_mask));
  TEST_ASSERT_EQUAL_UINT64(MLN_RUNTIME_EVENT_MASK_ALL, map_mask);

  // One in-group bit for the other source kind, which each setter accepts and
  // reports back unchanged.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_runtime_set_event_mask(runtime, MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_get_event_mask(runtime, &runtime_mask)
  );
  TEST_ASSERT_EQUAL_UINT64(
    MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED, runtime_mask
  );

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_get_event_mask(runtime, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_event_mask(map, NULL)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_fresh_map_and_runtime_select_every_event_type(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  uint64_t runtime_mask = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_get_event_mask(runtime, &runtime_mask)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_RUNTIME_EVENT_MASK_ALL, runtime_mask);
  uint64_t map_mask = 0;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_get_event_mask(map, &map_mask));
  TEST_ASSERT_EQUAL_UINT64(MLN_RUNTIME_EVENT_MASK_ALL, map_mask);

  load_style_and_pump(runtime, map);
  TEST_ASSERT_GREATER_THAN_size_t(
    0, mln_test_drain_counting(runtime, MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void runtime_options_reject_unknown_flags(void) {
  mln_runtime_options runtime_options = mln_runtime_options_default();
  runtime_options.flags = UINT32_C(1) << 31U;
  mln_runtime bad_runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_create(&runtime_options, &bad_runtime)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, bad_runtime);
}

// Creation rejects a mask the setters reject, so one value is not accepted at
// creation and then refused on the way back through a read-modify-write.
static void options_reject_unknown_event_mask_bits(void) {
  const uint64_t unknown = UINT64_C(1) << 40U;

  mln_runtime_options runtime_options = mln_runtime_options_default();
  runtime_options.event_mask = MLN_RUNTIME_EVENT_MASK_ALL | unknown;
  mln_runtime bad_runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_create(&runtime_options, &bad_runtime)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, bad_runtime);

  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options map_options = mln_map_options_default();
  map_options.event_mask = MLN_RUNTIME_EVENT_MASK_ALL | unknown;
  mln_map bad_map = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_create(runtime, &map_options, &bad_map)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, bad_map);

  // The same value the setter rejects, so the two agree.
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_event_mask(map, MLN_RUNTIME_EVENT_MASK_ALL | unknown)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A creation mask applies before MapLibre reports constructor-time events.
static void a_creation_mask_applies_during_construction(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const mln_map_options options =
    map_options_with_event_mask(MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED);
  mln_map map = mln_test_create_map_with_options(runtime, &options);

  uint64_t map_mask = MLN_RUNTIME_EVENT_MASK_ALL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_get_event_mask(map, &map_mask));
  TEST_ASSERT_EQUAL_UINT64(MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED, map_mask);

  mln_runtime_event_batch batch = mln_runtime_event_batch_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_drain_events(runtime, 0, &batch)
  );
  TEST_ASSERT_EQUAL_size_t(0, batch.event_count);

  load_style_and_pump(runtime, map);
  TEST_ASSERT_EQUAL_size_t(
    0, mln_test_drain_counting(
         runtime, MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE
       )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Clearing one bit leaves every other type arriving, which separates
// suppression from a broken producer.
static void clearing_one_type_leaves_the_others_arriving(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_event_mask(
      map, MLN_RUNTIME_EVENT_MASK_ALL &
             ~(uint64_t)MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE
    )
  );
  mln_test_drain_all(runtime);

  load_style_and_pump(runtime, map);
  TEST_ASSERT_EQUAL_size_t(
    0, mln_test_drain_counting(
         runtime, MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE
       )
  );

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_event_mask(map, MLN_RUNTIME_EVENT_MASK_ALL)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_request_repaint(map));
  TEST_ASSERT_GREATER_THAN_size_t(
    0, mln_test_drain_counting(
         runtime, MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE
       )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Suppression happens at push time, so a cleared type raises no wake flag and a
// parking pump waits out its timeout where it previously returned immediately.
static void a_suppressed_producer_leaves_a_pump_parked(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const mln_map_options options =
    map_options_with_event_mask(MLN_RUNTIME_EVENT_MASK_NONE);
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_runtime_set_event_mask(runtime, MLN_RUNTIME_EVENT_MASK_NONE)
  );
  // Two zero pumps and a drain leave the wake flag clear and the queue empty.
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 0, -1));
  TEST_ASSERT_EQUAL_size_t(0, mln_test_drain_all(runtime));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 0, -1));
  TEST_ASSERT_EQUAL_size_t(0, mln_test_drain_all(runtime));

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_request_repaint(map));
  const uint64_t started = mln_test_monotonic_milliseconds();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_pump(runtime, park_timeout_milliseconds, -1)
  );
  const uint64_t elapsed = mln_test_monotonic_milliseconds() - started;
  TEST_ASSERT_TRUE_MESSAGE(
    elapsed >= park_floor_milliseconds,
    "A repaint whose event type is cleared still released a parked pump."
  );
  TEST_ASSERT_EQUAL_size_t(0, mln_test_drain_all(runtime));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_batch_reports_this_headers_stride_and_ends_the_previous(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  load_style_and_pump(runtime, map);

  mln_runtime_event_batch batch = mln_runtime_event_batch_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_drain_events(runtime, 0, &batch)
  );
  TEST_ASSERT_EQUAL_UINT32(sizeof(mln_runtime_event), batch.event_size);
  TEST_ASSERT_GREATER_THAN_size_t(1, batch.event_count);
  TEST_ASSERT_EQUAL_size_t(0, batch.remaining_count);
  TEST_ASSERT_NOT_NULL(batch.events);

  // A drain that finds nothing reports an empty batch and ends the previous
  // batch's readable window.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_drain_events(runtime, 0, &batch)
  );
  TEST_ASSERT_EQUAL_size_t(0, batch.event_count);
  TEST_ASSERT_NULL(batch.events);
  TEST_ASSERT_NULL(batch.messages);
  TEST_ASSERT_EQUAL_size_t(0, batch.messages_size);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_bounded_drain_reports_what_stayed_queued(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  load_style_and_pump(runtime, map);

  mln_runtime_event_batch batch = mln_runtime_event_batch_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_drain_events(runtime, 1, &batch)
  );
  TEST_ASSERT_EQUAL_size_t(1, batch.event_count);
  TEST_ASSERT_GREATER_THAN_size_t(0, batch.remaining_count);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_drain_events(runtime, 0, &batch)
  );
  TEST_ASSERT_GREATER_THAN_size_t(0, batch.event_count);
  TEST_ASSERT_EQUAL_size_t(0, batch.remaining_count);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A batch holds copies on the runtime, so destroying the map whose events it
// carries leaves it readable.
static void a_batch_outlives_the_map_that_produced_it(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  load_style_and_pump(runtime, map);

  mln_runtime_event_batch batch = mln_runtime_event_batch_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_drain_events(runtime, 0, &batch)
  );
  TEST_ASSERT_GREATER_THAN_size_t(0, batch.event_count);
  const size_t event_count = batch.event_count;
  const size_t messages_size = batch.messages_size;

  mln_test_destroy_map(map);

  size_t map_sourced = 0;
  for (size_t index = 0; index < event_count; index += 1) {
    const mln_runtime_event* event = batch_event(&batch, index);
    if (event->source_type == MLN_RUNTIME_EVENT_SOURCE_MAP) {
      TEST_ASSERT_EQUAL_UINT64(map, event->source);
      map_sourced += 1;
    }
    TEST_ASSERT_TRUE(
      (size_t)event->message_offset + event->message_size <= messages_size
    );
  }
  TEST_ASSERT_GREATER_THAN_size_t(0, map_sourced);

  mln_test_destroy_runtime(runtime);
}

// A transition reports its identity immediately before the camera change that
// completed it, and one batch preserves that order.
static void one_batch_reports_events_in_queue_order(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_drain_all(runtime);

  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_ZOOM;
  camera.zoom = 4.0;
  mln_animation_options animation = mln_animation_options_default();
  animation.fields = MLN_ANIMATION_OPTION_TRANSITION_ID;
  animation.transition_id = 77;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_ease_to(map, &camera, &animation)
  );

  mln_runtime_event_batch batch = mln_runtime_event_batch_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_drain_events(runtime, 0, &batch)
  );
  bool saw_finish = false;
  bool did_change_followed_finish = false;
  for (size_t index = 0; index < batch.event_count; index += 1) {
    const mln_runtime_event* event = batch_event(&batch, index);
    if (event->type == MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED) {
      TEST_ASSERT_EQUAL_UINT64(
        77, event->payload.camera_transition_finished.transition_id
      );
      saw_finish = true;
    } else if (
      event->type == MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE && saw_finish
    ) {
      did_change_followed_finish = true;
    }
  }
  TEST_ASSERT_TRUE(saw_finish);
  TEST_ASSERT_TRUE(did_change_followed_finish);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Two text-bearing events in one batch point at distinct arena ranges, and each
// range holds that event's own bytes.
static void the_message_arena_carries_one_range_per_event(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map first = mln_test_create_map(runtime);
  mln_map second = mln_test_create_map(runtime);
  mln_test_drain_all(runtime);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NATIVE_ERROR,
    mln_map_set_style_json(first, MLN_BUFFER_LITERAL("{\"version\":"))
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NATIVE_ERROR,
    mln_map_set_style_json(second, MLN_BUFFER_LITERAL("not json at all"))
  );

  mln_runtime_event_batch batch = mln_runtime_event_batch_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_drain_events(runtime, 0, &batch)
  );

  size_t failures = 0;
  uint32_t first_offset = 0;
  uint32_t second_offset = 0;
  for (size_t index = 0; index < batch.event_count; index += 1) {
    const mln_runtime_event* event = batch_event(&batch, index);
    if (event->type != MLN_RUNTIME_EVENT_MAP_LOADING_FAILED) {
      continue;
    }
    TEST_ASSERT_GREATER_THAN_UINT32(0, event->message_size);
    TEST_ASSERT_TRUE(
      (size_t)event->message_offset + event->message_size <= batch.messages_size
    );
    // The arena null-terminates each message, so a host reads a range as a C
    // string without copying it first.
    TEST_ASSERT_EQUAL_CHAR(
      '\0', batch.messages[event->message_offset + event->message_size]
    );
    TEST_ASSERT_EQUAL_size_t(
      event->message_size, strlen(batch.messages + event->message_offset)
    );
    if (failures == 0) {
      first_offset = event->message_offset;
    } else {
      second_offset = event->message_offset;
    }
    failures += 1;
  }
  TEST_ASSERT_EQUAL_size_t(2, failures);
  TEST_ASSERT_NOT_EQUAL_UINT32(first_offset, second_offset);

  mln_test_destroy_map(second);
  mln_test_destroy_map(first);
  mln_test_destroy_runtime(runtime);
}

// A failed offline operation records its text before the mask is consulted, so
// the take-result diagnostic carries the same text as the completion event.
static void a_take_result_reports_the_operations_failure_text(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_offline_operation_id operation_id = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_offline_regions_merge_database_start(
                     runtime, missing_database_path, &operation_id
                   )
  );
  TEST_ASSERT_NOT_EQUAL(0, operation_id);

  char event_message[512] = {0};
  mln_runtime_event event = {0};
  bool completed = false;
  for (size_t attempt = 0; attempt < take_result_attempts && !completed;
       attempt += 1) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 2, -1));
    completed = mln_test_drain_find(
      runtime, MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED, MLN_HANDLE_NULL,
      &event, event_message, sizeof(event_message)
    );
  }
  TEST_ASSERT_TRUE_MESSAGE(
    completed, "Merging a missing database should report completion."
  );
  TEST_ASSERT_EQUAL_UINT64(
    operation_id, event.payload.offline_operation_completed.operation_id
  );
  TEST_ASSERT_NOT_EQUAL_INT(
    MLN_STATUS_OK, event.payload.offline_operation_completed.result_status
  );
  TEST_ASSERT_GREATER_THAN_size_t(0, strlen(event_message));

  mln_offline_region_list regions = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_runtime_offline_regions_merge_database_take_result(
      runtime, operation_id, &regions
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, regions);
  TEST_ASSERT_EQUAL_STRING(event_message, mln_thread_last_error_message());
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_offline_operation_discard(runtime, operation_id)
  );

  // The same failure with the completion event unselected reports the same
  // text, so leaving that event type out loses nothing.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_runtime_set_event_mask(runtime, MLN_RUNTIME_EVENT_MASK_NONE)
  );
  operation_id = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_offline_regions_merge_database_start(
                     runtime, missing_database_path, &operation_id
                   )
  );
  bool reported = false;
  for (size_t attempt = 0; attempt < take_result_attempts && !reported;
       attempt += 1) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 2, -1));
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_INVALID_STATE,
      mln_runtime_offline_regions_merge_database_take_result(
        runtime, operation_id, &regions
      )
    );
    reported = strcmp(mln_thread_last_error_message(), event_message) == 0;
  }
  TEST_ASSERT_TRUE_MESSAGE(
    reported,
    "An unselected completion event left the take-result diagnostic generic."
  );
  TEST_ASSERT_EQUAL_size_t(
    0, mln_test_drain_counting(
         runtime, MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED
       )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_offline_operation_discard(runtime, operation_id)
  );
  mln_test_destroy_runtime(runtime);
}

void run_runtime_events_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(a_drain_rejects_an_undersized_batch_or_a_stale_runtime);
  RUN_TEST(event_entry_points_reject_a_foreign_thread);
  RUN_TEST(both_mask_setters_reject_unknown_bits_and_keep_foreign_ones);
  RUN_TEST(a_fresh_map_and_runtime_select_every_event_type);
  RUN_TEST(runtime_options_reject_unknown_flags);
  RUN_TEST(options_reject_unknown_event_mask_bits);
  RUN_TEST(a_creation_mask_applies_during_construction);
  RUN_TEST(clearing_one_type_leaves_the_others_arriving);
  RUN_TEST(a_suppressed_producer_leaves_a_pump_parked);
  RUN_TEST(a_batch_reports_this_headers_stride_and_ends_the_previous);
  RUN_TEST(a_bounded_drain_reports_what_stayed_queued);
  RUN_TEST(a_batch_outlives_the_map_that_produced_it);
  RUN_TEST(one_batch_reports_events_in_queue_order);
  RUN_TEST(the_message_arena_carries_one_range_per_event);
  RUN_TEST(a_take_result_reports_the_operations_failure_text);
}
