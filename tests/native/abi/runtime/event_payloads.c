// The payload contract of every runtime event type, as one table: the tag, the
// source, the code, the message, the payload fields, and the zeroed bytes past
// the active payload member. Each scenario below produces a set of event types
// from local inputs only, and checks every event it drained against the rows
// for those types.

#include <assert.h>

#include "support/test_support.h"

// A binding copies an undeclared payload kind as the bytes from the payload to
// the end of the event, so a member after the payload would join that window.
static_assert(
  offsetof(mln_runtime_event, payload) + sizeof(mln_runtime_event_payload) ==
    sizeof(mln_runtime_event),
  "the payload union ends the runtime event record"
);

// A GeoJSON point under a circle and an icon whose image the style never
// provides, so rendering it to idle reports tile work and a missing image
// without a sprite, glyphs, or any request.
static const char render_style_json[] =
  "{\"version\":8,\"sources\":{\"points\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0,0]},\"properties\":{}}}},"
  "\"layers\":[{\"id\":\"bg\",\"type\":\"background\","
  "\"paint\":{\"background-color\":\"#000000\"}},"
  "{\"id\":\"dot\",\"type\":\"circle\",\"source\":\"points\"},"
  "{\"id\":\"icon\",\"type\":\"symbol\",\"source\":\"points\","
  "\"layout\":{\"icon-image\":\"missing-icon\"}}]}";

static const char missing_image_id[] = "missing-icon";
static const char geojson_source_id[] = "points";
static const char offline_style_url[] = "test://offline/style.json";
static const char denied_message[] = "the test provider serves nothing";

// A GeoJSON source whose data only the resource provider can answer.
static const char remote_source_style_json[] =
  "{\"version\":8,\"sources\":{\"remote\":{\"type\":\"geojson\",\"data\":"
  "\"test://missing.geojson\"}},\"layers\":[{\"id\":\"dots\",\"type\":"
  "\"circle\",\"source\":\"remote\"}]}";

typedef struct recorded_event {
  mln_runtime_event event;
  char message[128];
} recorded_event;

// The first events of each type a scenario drained, in queue order. Rendering
// repeats a few types once per frame, so the log keeps a bounded sample of
// each and counts the rest.
#define MLN_TEST_EVENTS_KEPT_PER_TYPE 32U

typedef struct event_log {
  recorded_event events[1024];
  size_t count;
  size_t seen_per_type[64];
  bool overflowed;
  bool drain_failed;
} event_log;

static event_log scenario_log;

static void log_reset(event_log* log) {
  log->count = 0;
  memset(log->seen_per_type, 0, sizeof(log->seen_per_type));
  log->overflowed = false;
  log->drain_failed = false;
}

// Drains without asserting, so a wait predicate may call it; the log records a
// failure for the next assertion to report.
static void log_drain_quietly(mln_runtime runtime, event_log* log) {
  while (true) {
    mln_test_event_batch batch = mln_test_event_batch_default();
    if (mln_test_drain_events(runtime, &batch) != MLN_STATUS_OK) {
      log->drain_failed = true;
      return;
    }
    if (batch.event_count == 0) {
      return;
    }
    for (size_t index = 0; index < batch.event_count; index += 1) {
      const mln_runtime_event* event =
        (const mln_runtime_event*)((const char*)batch.events +
                                   (index * batch.event_size));
      const size_t type_slot = event->type % 64U;
      log->seen_per_type[type_slot] += 1;
      if (log->seen_per_type[type_slot] > MLN_TEST_EVENTS_KEPT_PER_TYPE) {
        continue;
      }
      if (log->count == sizeof(log->events) / sizeof(*log->events)) {
        log->overflowed = true;
        return;
      }
      recorded_event* record = &log->events[log->count];
      log->count += 1;
      record->event = *event;
      const size_t length = event->message_size < sizeof(record->message) - 1
                              ? event->message_size
                              : sizeof(record->message) - 1;
      memcpy(record->message, batch.messages + event->message_offset, length);
      record->message[length] = '\0';
    }
  }
}

static void log_drain(mln_runtime runtime, event_log* log) {
  log_drain_quietly(runtime, log);
  TEST_ASSERT_FALSE_MESSAGE(log->drain_failed, "a drain failed");
  TEST_ASSERT_FALSE_MESSAGE(
    log->overflowed, "a scenario queued more events than the log holds"
  );
}

static const recorded_event* log_find(
  const event_log* log, uint32_t type, uint64_t source
) {
  for (size_t index = 0; index < log->count; index += 1) {
    const mln_runtime_event* event = &log->events[index].event;
    if (event->type == type && event->source == source) {
      return &log->events[index];
    }
  }
  return NULL;
}

static bool log_contains(const event_log* log, uint32_t type, uint64_t source) {
  return log_find(log, type, source) != NULL;
}

typedef enum scenario {
  SCENARIO_CAMERA,
  SCENARIO_RENDER,
  SCENARIO_STILL_IMAGE,
  SCENARIO_STILL_IMAGE_FAILURE,
  SCENARIO_OFFLINE,
  // No local input reaches the event; the row's reason says why.
  SCENARIO_UNREACHABLE,
} scenario;

// What a scenario created, for the rows that compare against it.
typedef struct scenario_context {
  mln_runtime runtime;
  mln_map map;
  mln_offline_region_id region_id;
} scenario_context;

typedef enum message_rule {
  MESSAGE_NONE,
  MESSAGE_TEXT,
} message_rule;

typedef enum code_rule {
  CODE_ZERO,
  CODE_CHANGE_MODE,
} code_rule;

typedef struct payload_row {
  uint32_t type;
  scenario scenario;
  uint32_t source_type;
  uint32_t payload_type;
  // Bytes of the payload union the active member covers. Every later byte
  // must be zero.
  size_t payload_size;
  code_rule code;
  message_rule message;
  // The exact message, or null to accept any nonempty one.
  const char* expected_message;
  // Checks the active member's fields, or null when it has none.
  void (*check)(const recorded_event* record, const scenario_context* context);
  const char* unreachable_reason;
} payload_row;

static void check_render_frame(
  const recorded_event* record, const scenario_context* context
) {
  (void)context;
  const mln_runtime_event_render_frame* frame =
    &record->event.payload.render_frame;
  TEST_ASSERT_TRUE(
    frame->mode == MLN_RENDER_MODE_PARTIAL ||
    frame->mode == MLN_RENDER_MODE_FULL
  );
}

static void check_render_map(
  const recorded_event* record, const scenario_context* context
) {
  (void)context;
  const uint32_t mode = record->event.payload.render_map.mode;
  TEST_ASSERT_TRUE(
    mode == MLN_RENDER_MODE_PARTIAL || mode == MLN_RENDER_MODE_FULL
  );
}

static void check_tile_action(
  const recorded_event* record, const scenario_context* context
) {
  (void)context;
  const mln_runtime_event_tile_action* action =
    &record->event.payload.tile_action;
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(MLN_TILE_OPERATION_NULL, action->operation);
  const mln_tile_id* tile = &action->tile_id;
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(tile->overscaled_z, tile->canonical_z);
  TEST_ASSERT_LESS_THAN_UINT32(32, tile->canonical_z);
  TEST_ASSERT_LESS_THAN_UINT64(
    UINT64_C(1) << tile->canonical_z, tile->canonical_x
  );
  TEST_ASSERT_LESS_THAN_UINT64(
    UINT64_C(1) << tile->canonical_z, tile->canonical_y
  );
}

static void check_offline_status(
  const recorded_event* record, const scenario_context* context
) {
  const mln_runtime_event_offline_region_status* status =
    &record->event.payload.offline_region_status;
  TEST_ASSERT_EQUAL_INT64(context->region_id, status->region_id);
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(
    MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE, status->status.download_state
  );
}

static void check_offline_response_error(
  const recorded_event* record, const scenario_context* context
) {
  const mln_runtime_event_offline_region_response_error* error =
    &record->event.payload.offline_region_response_error;
  TEST_ASSERT_EQUAL_INT64(context->region_id, error->region_id);
  TEST_ASSERT_EQUAL_UINT32(MLN_RESOURCE_ERROR_REASON_NOT_FOUND, error->reason);
}

enum { MAP = MLN_RUNTIME_EVENT_SOURCE_MAP };

// A row that names only its type, scenario, and source has no payload, code,
// or message.
static const payload_row payload_rows[] = {
  {MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE, SCENARIO_CAMERA, MAP,
   .code = CODE_CHANGE_MODE},
  {MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING, SCENARIO_RENDER, MAP},
  {MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE, SCENARIO_CAMERA, MAP,
   .code = CODE_CHANGE_MODE},
  {MLN_RUNTIME_EVENT_MAP_STYLE_LOADED, SCENARIO_RENDER, MAP},
  {MLN_RUNTIME_EVENT_MAP_LOADING_STARTED, SCENARIO_RENDER, MAP},
  {MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED, SCENARIO_RENDER, MAP},
  // A style-loading exception raised inside the call reports code 0.
  {MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, SCENARIO_STILL_IMAGE_FAILURE, MAP,
   .message = MESSAGE_TEXT},
  {MLN_RUNTIME_EVENT_MAP_IDLE, SCENARIO_RENDER, MAP},
  {MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE, SCENARIO_RENDER, MAP},
  {MLN_RUNTIME_EVENT_MAP_RENDER_ERROR, SCENARIO_UNREACHABLE, MAP,
   .message = MESSAGE_TEXT,
   .unreachable_reason =
     "MapLibre reports a render error only when a frame throws, which no "
     "valid style or target provokes"},
  {MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED, SCENARIO_STILL_IMAGE, MAP},
  {MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED, SCENARIO_STILL_IMAGE_FAILURE, MAP,
   .message = MESSAGE_TEXT},
  {MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED, SCENARIO_RENDER, MAP},
  {MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED, SCENARIO_RENDER, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME,
   sizeof(mln_runtime_event_render_frame), .check = check_render_frame},
  {MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED, SCENARIO_RENDER, MAP},
  {MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED, SCENARIO_RENDER, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP, sizeof(mln_runtime_event_render_map),
   .check = check_render_map},
  {MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING, SCENARIO_RENDER, MAP,
   .message = MESSAGE_TEXT, .expected_message = missing_image_id},
  {MLN_RUNTIME_EVENT_MAP_TILE_ACTION, SCENARIO_RENDER, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION, sizeof(mln_runtime_event_tile_action),
   .message = MESSAGE_TEXT, .expected_message = geojson_source_id,
   .check = check_tile_action},
  {MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED, SCENARIO_OFFLINE,
   MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
   MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS,
   sizeof(mln_runtime_event_offline_region_status),
   .check = check_offline_status},
  {MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR, SCENARIO_OFFLINE,
   MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
   MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR,
   sizeof(mln_runtime_event_offline_region_response_error),
   .message = MESSAGE_TEXT, .expected_message = denied_message,
   .check = check_offline_response_error},
  {MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED,
   SCENARIO_UNREACHABLE, MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
   MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT,
   sizeof(mln_runtime_event_offline_region_tile_count_limit),
   .unreachable_reason =
     "MapLibre limits only mapbox:// tile downloads, and no call sets the "
     "limit"},
  {MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED, SCENARIO_CAMERA, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED,
   sizeof(mln_runtime_event_camera_transition_finished)},
};

static const size_t payload_row_count =
  sizeof(payload_rows) / sizeof(*payload_rows);

static void check_event_against_row(
  const payload_row* row, const recorded_event* record,
  const scenario_context* context
) {
  const mln_runtime_event* event = &record->event;
  TEST_ASSERT_EQUAL_UINT32(row->source_type, event->source_type);
  TEST_ASSERT_EQUAL_UINT32(row->payload_type, event->payload_type);
  if (row->code == CODE_ZERO) {
    TEST_ASSERT_EQUAL_INT32(0, event->code);
  } else {
    TEST_ASSERT_TRUE(
      event->code == MLN_CAMERA_CHANGE_MODE_IMMEDIATE ||
      event->code == MLN_CAMERA_CHANGE_MODE_ANIMATED
    );
  }
  if (row->message == MESSAGE_NONE) {
    TEST_ASSERT_EQUAL_UINT32(0, event->message_size);
    TEST_ASSERT_EQUAL_UINT64(0, event->message_offset);
  } else {
    TEST_ASSERT_GREATER_THAN_UINT32(0, event->message_size);
    if (row->expected_message != NULL) {
      TEST_ASSERT_EQUAL_STRING(row->expected_message, record->message);
    }
  }
  const unsigned char* payload = (const unsigned char*)&event->payload;
  for (size_t offset = row->payload_size; offset < sizeof(event->payload);
       offset += 1) {
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(
      0, payload[offset], "a payload byte past the active member is not zero"
    );
  }
  if (row->check != NULL) {
    row->check(record, context);
  }
}

// Checks every drained event of each row the scenario produces, and fails a
// row whose type the scenario never queued.
static void check_scenario_rows(
  scenario which, const event_log* log, const scenario_context* context
) {
  for (size_t row_index = 0; row_index < payload_row_count; row_index += 1) {
    const payload_row* row = &payload_rows[row_index];
    if (row->scenario != which) {
      continue;
    }
    const uint64_t source = row->source_type == MLN_RUNTIME_EVENT_SOURCE_MAP
                              ? context->map
                              : context->runtime;
    size_t matched = 0;
    for (size_t index = 0; index < log->count; index += 1) {
      const recorded_event* record = &log->events[index];
      if (record->event.type != row->type || record->event.source != source) {
        continue;
      }
      check_event_against_row(row, record, context);
      matched += 1;
    }
    char label[64];
    snprintf(label, sizeof(label), "event type %u never arrived", row->type);
    TEST_ASSERT_GREATER_THAN_size_t_MESSAGE(0, matched, label);
  }
}

// Each type in MLN_RUNTIME_EVENT_MASK_ALL has one row, the table has no other
// row, and a row with no scenario says why.
static void every_event_type_has_one_payload_row(void) {
  size_t types = 0;
  for (uint32_t type = 0; type < 64; type += 1) {
    if ((MLN_RUNTIME_EVENT_MASK_ALL & (UINT64_C(1) << type)) == 0) {
      continue;
    }
    types += 1;
    size_t rows = 0;
    for (size_t index = 0; index < payload_row_count; index += 1) {
      if (payload_rows[index].type != type) {
        continue;
      }
      rows += 1;
      TEST_ASSERT_EQUAL(
        payload_rows[index].scenario == SCENARIO_UNREACHABLE,
        payload_rows[index].unreachable_reason != NULL
      );
    }
    TEST_ASSERT_EQUAL_size_t(1, rows);
  }
  TEST_ASSERT_EQUAL_size_t(types, payload_row_count);
}

static void submit_camera(
  mln_map map, uint32_t mode, double zoom, uint32_t animation_fields,
  double duration_ms, uint64_t transition_id
) {
  mln_camera_update update = mln_camera_update_default();
  update.mode = mode;
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = zoom;
  update.animation.fields = animation_fields;
  update.animation.duration_ms = duration_ms;
  update.animation.transition_id = transition_id;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

static const uint32_t with_duration_and_id =
  MLN_ANIMATION_OPTION_DURATION | MLN_ANIMATION_OPTION_TRANSITION_ID;

// Reports the index of the event of `type` from `source` after `start`, or the
// log's count when there is none.
static size_t log_next(
  const event_log* log, size_t start, uint32_t type, uint64_t source
) {
  for (size_t index = start; index < log->count; index += 1) {
    if (
      log->events[index].event.type == type &&
      log->events[index].event.source == source
    ) {
      return index;
    }
  }
  return log->count;
}

// A jump reports an immediate change and an ease an animated one. The jump
// that cancels the running ease ends it, and that transition's finished event
// comes immediately before the camera change that completed it.
static void camera_events_carry_their_change_mode_and_transition(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const scenario_context context = {.runtime = runtime, .map = map};
  mln_test_drain_all(runtime);
  event_log* log = &scenario_log;
  log_reset(log);

  submit_camera(map, MLN_CAMERA_UPDATE_MODE_JUMP, 2.0, 0, 0.0, 0);
  log_drain(runtime, log);
  const recorded_event* jump_will =
    log_find(log, MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE, map);
  TEST_ASSERT_NOT_NULL(jump_will);
  TEST_ASSERT_EQUAL_INT32(
    MLN_CAMERA_CHANGE_MODE_IMMEDIATE, jump_will->event.code
  );
  const recorded_event* jump_did =
    log_find(log, MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE, map);
  TEST_ASSERT_NOT_NULL(jump_did);
  TEST_ASSERT_EQUAL_INT32(
    MLN_CAMERA_CHANGE_MODE_IMMEDIATE, jump_did->event.code
  );

  const size_t ease_start = log->count;
  submit_camera(
    map, MLN_CAMERA_UPDATE_MODE_EASE, 6.0, with_duration_and_id, 60000.0, 41
  );
  log_drain(runtime, log);
  const size_t ease_will =
    log_next(log, ease_start, MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE, map);
  TEST_ASSERT_LESS_THAN_size_t(log->count, ease_will);
  TEST_ASSERT_EQUAL_INT32(
    MLN_CAMERA_CHANGE_MODE_ANIMATED, log->events[ease_will].event.code
  );
  TEST_ASSERT_EQUAL_size_t(
    log->count,
    log_next(
      log, ease_start, MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED, map
    )
  );

  const size_t cancel_start = log->count;
  submit_camera(map, MLN_CAMERA_UPDATE_MODE_JUMP, 8.0, 0, 0.0, 0);
  log_drain(runtime, log);
  const size_t finished = log_next(
    log, cancel_start, MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED, map
  );
  TEST_ASSERT_LESS_THAN_size_t(log->count, finished);
  TEST_ASSERT_EQUAL_UINT64(
    41,
    log->events[finished].event.payload.camera_transition_finished.transition_id
  );
  TEST_ASSERT_LESS_THAN_size_t(log->count, finished + 1);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE,
    log->events[finished + 1].event.type
  );

  check_scenario_rows(SCENARIO_CAMERA, log, &context);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct zero_duration_case {
  const char* label;
  uint32_t mode;
  uint32_t animation_fields;
} zero_duration_case;

static const zero_duration_case zero_duration_cases[] = {
  {"ease with a zero duration", MLN_CAMERA_UPDATE_MODE_EASE,
   with_duration_and_id},
  {"ease with no duration", MLN_CAMERA_UPDATE_MODE_EASE,
   MLN_ANIMATION_OPTION_TRANSITION_ID},
  {"fly with a zero duration", MLN_CAMERA_UPDATE_MODE_FLY,
   with_duration_and_id},
};

// A transition with nothing to animate finishes within its command: one
// finished event, immediately followed by an immediate camera change.
static void zero_duration_transitions_finish_within_their_command(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_drain_all(runtime);
  event_log* log = &scenario_log;

  const size_t case_count =
    sizeof(zero_duration_cases) / sizeof(*zero_duration_cases);
  for (size_t index = 0; index < case_count; index += 1) {
    const zero_duration_case* row = &zero_duration_cases[index];
    log_reset(log);
    const uint64_t transition_id = 100 + index;
    submit_camera(
      map, row->mode, 3.0 + (double)index, row->animation_fields, 0.0,
      transition_id
    );
    log_drain(runtime, log);

    size_t finished_count = 0;
    size_t finished = log->count;
    for (size_t event = 0; event < log->count; event += 1) {
      if (
        log->events[event].event.type ==
        MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED
      ) {
        finished_count += 1;
        finished = event;
      }
    }
    TEST_ASSERT_EQUAL_size_t_MESSAGE(1, finished_count, row->label);
    TEST_ASSERT_EQUAL_UINT64_MESSAGE(
      transition_id,
      log->events[finished]
        .event.payload.camera_transition_finished.transition_id,
      row->label
    );
    TEST_ASSERT_LESS_THAN_size_t_MESSAGE(log->count, finished + 1, row->label);
    const mln_runtime_event* change = &log->events[finished + 1].event;
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
      MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE, change->type, row->label
    );
    TEST_ASSERT_EQUAL_INT32_MESSAGE(
      MLN_CAMERA_CHANGE_MODE_IMMEDIATE, change->code, row->label
    );
  }

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Renders one frame demand and fences it with a render barrier, then drains
// the frame results and, when `log` is set, the runtime's events.
static void render_one_frame(
  const mln_test_render_fixture* fixture, mln_runtime runtime, event_log* log
) {
  mln_frame_demand demand = mln_frame_demand_default();
  MLN_TEST_OK(
    mln_render_session_request_frame(fixture->session, &demand, NULL)
  );
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, fixture,
    mln_render_session_barrier(fixture->session, &completion.descriptor, NULL)
  );
  mln_render_frame_batch frames = MLN_HANDLE_NULL;
  MLN_TEST_OK(
    mln_render_session_drain_frame_results(fixture->session, &frames, NULL)
  );
  mln_render_frame_batch_release(frames);
  if (log != NULL) {
    log_drain(runtime, log);
  }
}

// Renders until `type` from `source` is in the log.
static void render_until_logged(
  const mln_test_render_fixture* fixture, mln_runtime runtime, event_log* log,
  uint32_t type, uint64_t source
) {
  const mln_test_deadline deadline = mln_test_deadline_default();
  mln_test_watchdog_note("rendering until an event type arrives");
  while (!log_contains(log, type, source)) {
    TEST_ASSERT_FALSE_MESSAGE(
      mln_test_deadline_passed(deadline), "the awaited event never arrived"
    );
    render_one_frame(fixture, runtime, log);
  }
  mln_test_watchdog_note(NULL);
}

// Rendering a style to idle reports the loading and render lifecycle, tile
// work on the GeoJSON source, and the icon the style never provides. A long
// ease then reports the camera changing between frames.
static void rendering_a_style_to_idle_reports_its_lifecycle(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const scenario_context context = {.runtime = runtime, .map = map};
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_test_drain_all(runtime);
  event_log* log = &scenario_log;
  log_reset(log);

  MLN_TEST_AWAIT_OK(mln_map_set_style_json(
    map, MLN_BUFFER_LITERAL(render_style_json), &completion.descriptor, NULL
  ));
  render_until_logged(&fixture, runtime, log, MLN_RUNTIME_EVENT_MAP_IDLE, map);

  submit_camera(
    map, MLN_CAMERA_UPDATE_MODE_EASE, 4.0, MLN_ANIMATION_OPTION_DURATION,
    60000.0, 0
  );
  render_until_logged(
    &fixture, runtime, log, MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING, map
  );
  submit_camera(map, MLN_CAMERA_UPDATE_MODE_JUMP, 4.0, 0, 0.0, 0);
  log_drain(runtime, log);

  // The map rendered fully before it went idle, and the renderer's frame count
  // grows from one finished frame to the next.
  bool full_render = false;
  int64_t frame_count = -1;
  for (size_t index = 0; index < log->count; index += 1) {
    const mln_runtime_event* event = &log->events[index].event;
    if (
      event->type == MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED &&
      event->payload.render_map.mode == MLN_RENDER_MODE_FULL
    ) {
      full_render = true;
    }
    if (event->type == MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED) {
      TEST_ASSERT_GREATER_THAN_INT64(
        frame_count, event->payload.render_frame.stats.frame_count
      );
      frame_count = event->payload.render_frame.stats.frame_count;
    }
  }
  TEST_ASSERT_TRUE(full_render);
  TEST_ASSERT_GREATER_THAN_INT64(0, frame_count);
  check_scenario_rows(SCENARIO_RENDER, log, &context);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_map create_static_map(mln_runtime runtime, uint64_t event_mask) {
  mln_map_options options = mln_map_options_default();
  options.map_mode = MLN_MAP_MODE_STATIC;
  options.initial_extent.width = 64;
  options.initial_extent.height = 64;
  options.event_mask = event_mask;
  return mln_test_create_map_with_options(runtime, &options);
}

// Wraps a still image's completion to drain the scenario log from inside it,
// before forwarding the result, so that the case sees what was queued when the
// completion ran.
typedef struct still_image_probe {
  mln_completion inner;
  mln_runtime runtime;
  mln_map map;
  uint32_t event_at_completion;
  bool event_queued;
} still_image_probe;

static void drain_then_complete(
  void* user_data, const mln_completion_result* result
) {
  still_image_probe* probe = user_data;
  log_drain_quietly(probe->runtime, &scenario_log);
  probe->event_queued =
    log_contains(&scenario_log, probe->event_at_completion, probe->map);
  probe->inner.callback(probe->inner.user_data, result);
}

static void release_still_image_probe(void* user_data) {
  const still_image_probe* probe = user_data;
  if (probe->inner.release_user_data != NULL) {
    probe->inner.release_user_data(probe->inner.user_data);
  }
}

// Requests a still image and renders until its completion arrives, then
// reports its status. A nonzero event_at_completion is the event that must
// already be queued when the completion runs: a host that sees the completion
// and then drains finds it.
static mln_status render_still_image(
  const mln_test_render_fixture* fixture, mln_runtime runtime, mln_map map,
  uint32_t event_at_completion, char* out_diagnostic, size_t diagnostic_capacity
) {
  mln_test_completion still = mln_test_completion_default(0);
  still_image_probe probe = {
    .inner = still.descriptor,
    .runtime = runtime,
    .map = map,
    .event_at_completion = event_at_completion,
  };
  const mln_completion probed = {
    .size = sizeof(mln_completion),
    .callback = drain_then_complete,
    .user_data = &probe,
    .release_user_data = release_still_image_probe,
  };
  MLN_TEST_OK(mln_map_request_still_image(
    map, event_at_completion == 0 ? &still.descriptor : &probed, NULL
  ));
  const mln_test_deadline deadline = mln_test_deadline_default();
  mln_test_watchdog_note("rendering until a still image completes");
  while (!mln_test_completion_poll(&still)) {
    TEST_ASSERT_FALSE_MESSAGE(
      mln_test_deadline_passed(deadline), "the still image never completed"
    );
    render_one_frame(fixture, runtime, NULL);
  }
  mln_test_watchdog_note(NULL);
  if (event_at_completion != 0) {
    TEST_ASSERT_TRUE_MESSAGE(
      probe.event_queued,
      "the still image's event was not queued when its completion ran"
    );
  }
  const mln_status status = mln_test_completion_status(&still);
  if (out_diagnostic != NULL) {
    snprintf(
      out_diagnostic, diagnostic_capacity, "%s",
      mln_test_completion_diagnostic(&still)
    );
  }
  mln_test_completion_destroy(&still);
  return status;
}

static void a_still_image_reports_that_it_finished(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_static_map(runtime, MLN_RUNTIME_EVENT_MASK_ALL);
  const scenario_context context = {.runtime = runtime, .map = map};
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_test_drain_all(runtime);
  event_log* log = &scenario_log;
  log_reset(log);

  MLN_TEST_OK(render_still_image(
    &fixture, runtime, map, MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED, NULL, 0
  ));
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  log_drain(runtime, log);
  check_scenario_rows(SCENARIO_STILL_IMAGE, log, &context);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Answers every request with a not-found error, so every resource the library
// asks for fails without reaching the network.
static uint32_t deny_every_request(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  (void)user_data;
  (void)request;
  const mln_resource_response response = {
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_ERROR,
    .error_reason = MLN_RESOURCE_ERROR_REASON_NOT_FOUND,
    .error_message = denied_message,
  };
  (void)mln_resource_request_complete(handle, &response, NULL);
  mln_resource_request_release(handle);
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

static bool offline_error_logged(void* context) {
  scenario_context* scenario = context;
  log_drain_quietly(scenario->runtime, &scenario_log);
  return scenario_log.drain_failed || scenario_log.overflowed ||
         log_contains(
           &scenario_log, MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR,
           scenario->runtime
         );
}

static void deny_requests(mln_runtime runtime) {
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = deny_every_request,
  };
  MLN_TEST_AWAIT_OK(mln_runtime_set_resource_provider(
    runtime, &provider, &completion.descriptor, NULL
  ));
}

// A style that fails to parse reports the parser's exception with code 0. A
// source whose data fails to load then fails the still image, with the same
// text in the completion and the event.
static void failed_loads_report_their_text(void) {
  mln_runtime runtime = mln_test_create_runtime();
  deny_requests(runtime);
  mln_map map = create_static_map(runtime, MLN_RUNTIME_EVENT_MASK_ALL);
  const scenario_context context = {.runtime = runtime, .map = map};
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_test_drain_all(runtime);
  event_log* log = &scenario_log;
  log_reset(log);

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_NATIVE_ERROR,
    mln_map_set_style_json(
      map, MLN_BUFFER_LITERAL("{"), &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_OK(mln_map_set_style_json(
    map, MLN_BUFFER_LITERAL(remote_source_style_json), &completion.descriptor,
    NULL
  ));
  char still_diagnostic[MLN_DIAGNOSTIC_MESSAGE_CAPACITY] = "";
  MLN_TEST_STATUS(
    MLN_STATUS_NATIVE_ERROR,
    render_still_image(
      &fixture, runtime, map, MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED,
      still_diagnostic, sizeof(still_diagnostic)
    )
  );
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  log_drain(runtime, log);
  check_scenario_rows(SCENARIO_STILL_IMAGE_FAILURE, log, &context);
  const recorded_event* failed =
    log_find(log, MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED, map);
  TEST_ASSERT_GREATER_THAN_size_t(0, strlen(still_diagnostic));
  TEST_ASSERT_EQUAL_STRING_LEN(
    still_diagnostic, failed->message, sizeof(failed->message) - 1
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// An observed download reports its status changes and the error its style
// request met, from the runtime rather than a map.
static void an_observed_offline_download_reports_status_and_errors(void) {
  mln_runtime runtime = mln_test_create_runtime();
  deny_requests(runtime);

  const mln_offline_region_definition definition = {
    .size = sizeof(mln_offline_region_definition),
    .type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
    .data.tile_pyramid = {
      .style_url = offline_style_url,
      .bounds =
        {
          .southwest = {.latitude = 1.0, .longitude = 2.0},
          .northeast = {.latitude = 3.0, .longitude = 4.0},
        },
      .min_zoom = 0.0,
      .max_zoom = 1.0,
      .pixel_ratio = 1.0F,
    },
  };
  mln_test_completion create =
    mln_test_completion_default(sizeof(mln_offline_region_info));
  MLN_TEST_OK(mln_runtime_create_offline_region(
    runtime, &definition, NULL, 0, &create.descriptor, NULL
  ));
  mln_offline_region_info info = {0};
  MLN_TEST_OK(mln_test_completion_finish_value(&create, &info, sizeof(info)));
  scenario_context context = {.runtime = runtime, .region_id = info.id};
  mln_test_drain_all(runtime);
  log_reset(&scenario_log);

  MLN_TEST_AWAIT_OK(mln_runtime_set_offline_region_observed(
    runtime, info.id, true, &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_runtime_set_offline_region_download_state(
    runtime, info.id, MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE,
    &completion.descriptor, NULL
  ));
  TEST_ASSERT_TRUE(mln_test_await(
    offline_error_logged, &context, mln_test_deadline_default(),
    "an offline response error"
  ));
  log_drain(runtime, &scenario_log);
  MLN_TEST_AWAIT_OK(mln_runtime_set_offline_region_download_state(
    runtime, info.id, MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE,
    &completion.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  log_drain(runtime, &scenario_log);
  check_scenario_rows(SCENARIO_OFFLINE, &scenario_log, &context);

  mln_test_destroy_runtime(runtime);
}

// A map created with an empty mask queues nothing, through a camera change and
// a rendered still image, whose completion fences the check.
static void a_map_created_with_an_empty_mask_queues_nothing(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_static_map(runtime, MLN_RUNTIME_EVENT_MASK_NONE);
  MLN_TEST_AWAIT_OK(mln_map_set_style_json(
    map, mln_test_background_style_json, &completion.descriptor, NULL
  ));
  submit_camera(map, MLN_CAMERA_UPDATE_MODE_JUMP, 2.0, 0, 0.0, 0);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  MLN_TEST_OK(render_still_image(&fixture, runtime, map, 0, NULL, 0));
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));

  event_log* log = &scenario_log;
  log_reset(log);
  log_drain(runtime, log);
  TEST_ASSERT_EQUAL_size_t(0, log->count);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(every_event_type_has_one_payload_row);
  RUN_TEST(camera_events_carry_their_change_mode_and_transition);
  RUN_TEST(zero_duration_transitions_finish_within_their_command);
  RUN_TEST(rendering_a_style_to_idle_reports_its_lifecycle);
  RUN_TEST(a_still_image_reports_that_it_finished);
  RUN_TEST(failed_loads_report_their_text);
  RUN_TEST(an_observed_offline_download_reports_status_and_errors);
  RUN_TEST(a_map_created_with_an_empty_mask_queues_nothing);
}
