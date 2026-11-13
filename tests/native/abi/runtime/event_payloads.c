// The payload contract of every runtime event type, as one table: the tag, the
// source, the message, the payload fields, and the zeroed bytes past the active
// payload member. Each scenario below produces a set of event types
// from local inputs only, and checks every event it drained against the rows
// for those types.

#include <assert.h>

#include "support/frames.h"
#include "support/map.h"
#include "support/resources.h"
#include "support/style.h"
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

// A vector source described by TileJSON that only the provider serves.
static const char tilejson_source_id[] = "described";
static const char tilejson_url[] = "test://tiles.json";
static const char tilejson_style_json[] =
  "{\"version\":8,\"sources\":{\"described\":{\"type\":\"vector\","
  "\"url\":\"test://tiles.json\"}},\"layers\":[]}";
static const char tilejson[] =
  "{\"tilejson\":\"2.2.0\",\"tiles\":[\"test://tiles/{z}/{x}/{y}.pbf\"],"
  "\"attribution\":\"Served tiles\"}";

static const char unrouted_message[] =
  "the test provider has no route for this URL";

// Two sprites and two font stacks: the provider serves one of each and has no
// route for the other. Each symbol layer requests its font stack's first glyph
// range.
static const char resource_style_json[] =
  "{\"version\":8,"
  "\"sprite\":[{\"id\":\"good\",\"url\":\"test://sprites/good\"},"
  "{\"id\":\"bad\",\"url\":\"test://sprites/bad\"}],"
  "\"glyphs\":\"test://fonts/{fontstack}/{range}.pbf\","
  "\"sources\":{\"points\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0,0]},\"properties\":{}}}},"
  "\"layers\":[{\"id\":\"good\",\"type\":\"symbol\",\"source\":\"points\","
  "\"layout\":{\"text-field\":\"A\",\"text-font\":[\"GoodFont\"]}},"
  "{\"id\":\"bad\",\"type\":\"symbol\",\"source\":\"points\","
  "\"layout\":{\"text-field\":\"A\","
  "\"text-font\":[\"BadFont\",\"Fallback\"]}}]}";

static const char good_sprite_json[] = "{}";

// A transparent 1x1 RGBA PNG.
static const uint8_t good_sprite_png[] = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
  0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
  0x08, 0x06, 0x00, 0x00, 0x00, 0x1f, 0x15, 0xc4, 0x89, 0x00, 0x00, 0x00,
  0x0b, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0x60, 0x00, 0x02, 0x00,
  0x00, 0x05, 0x00, 0x01, 0x7a, 0x5e, 0xab, 0x3f, 0x00, 0x00, 0x00, 0x00,
  0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};

static const mln_test_provided_resource sprite_and_glyph_resources[] = {
  {.url = "test://sprites/good.json",
   .response =
     {
       .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
       .bytes = (const uint8_t*)good_sprite_json,
       .byte_count = sizeof(good_sprite_json) - 1,
     }},
  {.url = "test://sprites/good.png",
   .response =
     {
       .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
       .bytes = good_sprite_png,
       .byte_count = sizeof(good_sprite_png),
     }},
  {.url = "test://fonts/GoodFont/0-255.pbf",
   .response = {.status = MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT}},
};

typedef struct recorded_event {
  mln_runtime_event event;
  char message[128];
  // Copies of the payload's text, which the event borrows only until the
  // drain releases its batch: a font stack's first names, a sprite's ID and
  // URL, or an image or source ID. The payload's own views are not read after
  // the drain.
  char strings[2][64];
  size_t string_count;
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

static void copy_view(char* out, size_t capacity, mln_buffer_view view) {
  const size_t length = view.size < capacity - 1 ? view.size : capacity - 1;
  if (length != 0) memcpy(out, view.data, length);
  out[length] = '\0';
}

static void record_payload_text(
  recorded_event* record, const mln_runtime_event* event
) {
  record->string_count = 0;
  if (event->payload_type == MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS) {
    const mln_runtime_event_glyphs* glyphs = &event->payload.glyphs;
    record->string_count = glyphs->font_count;
    for (size_t index = 0; index < glyphs->font_count && index < 2;
         index += 1) {
      copy_view(
        record->strings[index], sizeof(record->strings[index]),
        glyphs->font_stack[index]
      );
    }
  } else if (event->payload_type == MLN_RUNTIME_EVENT_PAYLOAD_SPRITE) {
    record->string_count = 2;
    copy_view(
      record->strings[0], sizeof(record->strings[0]), event->payload.sprite.id
    );
    copy_view(
      record->strings[1], sizeof(record->strings[1]), event->payload.sprite.url
    );
  } else if (event->payload_type == MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE) {
    record->string_count = 1;
    copy_view(
      record->strings[0], sizeof(record->strings[0]),
      event->payload.style_image.image_id
    );
  } else if (event->payload_type == MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION) {
    record->string_count = 1;
    copy_view(
      record->strings[0], sizeof(record->strings[0]),
      event->payload.tile_action.source_id
    );
  } else if (event->payload_type == MLN_RUNTIME_EVENT_PAYLOAD_SOURCE_CHANGED) {
    record->string_count = 1;
    copy_view(
      record->strings[0], sizeof(record->strings[0]),
      event->payload.source_changed.source_id
    );
  }
}

static void log_reset(event_log* log) {
  log->count = 0;
  memset(log->seen_per_type, 0, sizeof(log->seen_per_type));
  log->overflowed = false;
  log->drain_failed = false;
}

static void log_event(const mln_runtime_event* event, void* context) {
  event_log* log = context;
  if (log->overflowed) return;
  const size_t type_slot = event->type % 64U;
  log->seen_per_type[type_slot] += 1;
  if (log->seen_per_type[type_slot] > MLN_TEST_EVENTS_KEPT_PER_TYPE) {
    return;
  }
  if (log->count == sizeof(log->events) / sizeof(*log->events)) {
    log->overflowed = true;
    return;
  }
  recorded_event* record = &log->events[log->count];
  log->count += 1;
  record->event = *event;
  copy_view(record->message, sizeof(record->message), event->message);
  record_payload_text(record, event);
}

// Drains without asserting, so a wait predicate may call it; the log records a
// failure for the next assertion to report.
static void log_drain_quietly(mln_runtime runtime, event_log* log) {
  if (mln_test_dispatch_events(runtime, log_event, log) != MLN_STATUS_OK) {
    log->drain_failed = true;
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
  SCENARIO_STILL_IMAGE_FAILURE,
  SCENARIO_OFFLINE,
  SCENARIO_SOURCE_DESCRIPTION,
  SCENARIO_SPRITES_AND_GLYPHS,
  // No local input reaches the event; the row's reason says why.
  SCENARIO_UNREACHABLE,
} scenario;

typedef struct scenario_context {
  mln_runtime runtime;
  mln_map map;
  mln_offline_region_id region_id;
} scenario_context;

typedef enum message_rule {
  MESSAGE_NONE,
  MESSAGE_TEXT,
} message_rule;

typedef struct payload_row {
  uint32_t type;
  scenario scenario;
  uint32_t source_type;
  uint32_t payload_type;
  // Bytes of the payload union the active member covers. Every later byte
  // must be zero.
  size_t payload_size;
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

static void check_camera_change(
  const recorded_event* record, const scenario_context* context
) {
  (void)context;
  const uint32_t mode = record->event.payload.camera_change.mode;
  TEST_ASSERT_TRUE(
    mode == MLN_CAMERA_CHANGE_MODE_IMMEDIATE ||
    mode == MLN_CAMERA_CHANGE_MODE_ANIMATED
  );
}

// The scenario's only failed load is a style JSON that doesn't parse.
static void check_loading_failed(
  const recorded_event* record, const scenario_context* context
) {
  (void)context;
  TEST_ASSERT_EQUAL_UINT32(
    MLN_MAP_LOAD_ERROR_STYLE_PARSE, record->event.payload.loading_failed.error
  );
}

static void check_style_image(
  const recorded_event* record, const scenario_context* context
) {
  (void)context;
  TEST_ASSERT_EQUAL_size_t(1, record->string_count);
  TEST_ASSERT_EQUAL_STRING(missing_image_id, record->strings[0]);
}

static void check_source_changed(
  const recorded_event* record, const scenario_context* context
) {
  (void)context;
  TEST_ASSERT_EQUAL_size_t(1, record->string_count);
  TEST_ASSERT_EQUAL_STRING(tilejson_source_id, record->strings[0]);
}

static void check_tile_action(
  const recorded_event* record, const scenario_context* context
) {
  (void)context;
  TEST_ASSERT_EQUAL_size_t(1, record->string_count);
  TEST_ASSERT_EQUAL_STRING(geojson_source_id, record->strings[0]);
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

// GoodFont's range loads and the stack of BadFont and Fallback fails; both
// request the range that holds "A".
static void check_glyphs(
  const recorded_event* record, const scenario_context* context
) {
  (void)context;
  const mln_runtime_event_glyphs* glyphs = &record->event.payload.glyphs;
  TEST_ASSERT_EQUAL_UINT32(0, glyphs->first);
  TEST_ASSERT_EQUAL_UINT32(255, glyphs->last);
  TEST_ASSERT_FALSE(glyphs->font_face);
  const bool good = strcmp(record->strings[0], "GoodFont") == 0;
  if (record->event.type == MLN_RUNTIME_EVENT_MAP_GLYPHS_LOADED) {
    TEST_ASSERT_TRUE(good);
  } else if (record->event.type == MLN_RUNTIME_EVENT_MAP_GLYPHS_ERROR) {
    TEST_ASSERT_FALSE(good);
  }
  if (good) {
    TEST_ASSERT_EQUAL_size_t(1, record->string_count);
  } else {
    TEST_ASSERT_EQUAL_size_t(2, record->string_count);
    TEST_ASSERT_EQUAL_STRING("BadFont", record->strings[0]);
    TEST_ASSERT_EQUAL_STRING("Fallback", record->strings[1]);
  }
}

static void check_sprite(
  const recorded_event* record, const scenario_context* context
) {
  (void)context;
  const bool good = strcmp(record->strings[0], "good") == 0;
  if (record->event.type == MLN_RUNTIME_EVENT_MAP_SPRITE_LOADED) {
    TEST_ASSERT_TRUE(good);
  } else if (record->event.type == MLN_RUNTIME_EVENT_MAP_SPRITE_ERROR) {
    TEST_ASSERT_FALSE(good);
  }
  if (!good) {
    TEST_ASSERT_EQUAL_STRING("bad", record->strings[0]);
  }
  TEST_ASSERT_EQUAL_STRING(
    good ? "test://sprites/good" : "test://sprites/bad", record->strings[1]
  );
}

enum { MAP = MLN_RUNTIME_EVENT_SOURCE_MAP };

// A row that names only its type, scenario, and source has no payload or
// message.
static const payload_row payload_rows[] = {
  {MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE, SCENARIO_CAMERA, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_CHANGE,
   sizeof(mln_runtime_event_camera_change), .check = check_camera_change},
  {MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING, SCENARIO_RENDER, MAP},
  {MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE, SCENARIO_CAMERA, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_CHANGE,
   sizeof(mln_runtime_event_camera_change), .check = check_camera_change},
  {MLN_RUNTIME_EVENT_MAP_STYLE_LOADED, SCENARIO_RENDER, MAP},
  {MLN_RUNTIME_EVENT_MAP_LOADING_STARTED, SCENARIO_RENDER, MAP},
  {MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED, SCENARIO_RENDER, MAP},
  {MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, SCENARIO_STILL_IMAGE_FAILURE, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_LOADING_FAILED,
   sizeof(mln_runtime_event_loading_failed), .message = MESSAGE_TEXT,
   .check = check_loading_failed},
  {MLN_RUNTIME_EVENT_MAP_IDLE, SCENARIO_RENDER, MAP},
  {MLN_RUNTIME_EVENT_MAP_RENDER_ERROR, SCENARIO_UNREACHABLE, MAP,
   .message = MESSAGE_TEXT,
   .unreachable_reason =
     "MapLibre reports a render error only when a frame throws, and a render "
     "session only when it loses its target, which no valid style or target "
     "provokes"},
  {MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED, SCENARIO_RENDER, MAP},
  {MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED, SCENARIO_RENDER, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME,
   sizeof(mln_runtime_event_render_frame), .check = check_render_frame},
  {MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED, SCENARIO_RENDER, MAP},
  // MapLibre always reports the full mode here, so the event carries none.
  {MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED, SCENARIO_RENDER, MAP},
  {MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING, SCENARIO_RENDER, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE, sizeof(mln_runtime_event_style_image),
   .check = check_style_image},
  {MLN_RUNTIME_EVENT_MAP_TILE_ACTION, SCENARIO_RENDER, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION, sizeof(mln_runtime_event_tile_action),
   .check = check_tile_action},
  {MLN_RUNTIME_EVENT_MAP_SOURCE_CHANGED, SCENARIO_SOURCE_DESCRIPTION, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_SOURCE_CHANGED,
   sizeof(mln_runtime_event_source_changed), .check = check_source_changed},
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
  {MLN_RUNTIME_EVENT_MAP_GLYPHS_REQUESTED, SCENARIO_SPRITES_AND_GLYPHS, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS, sizeof(mln_runtime_event_glyphs),
   .check = check_glyphs},
  {MLN_RUNTIME_EVENT_MAP_GLYPHS_LOADED, SCENARIO_SPRITES_AND_GLYPHS, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS, sizeof(mln_runtime_event_glyphs),
   .check = check_glyphs},
  {MLN_RUNTIME_EVENT_MAP_GLYPHS_ERROR, SCENARIO_SPRITES_AND_GLYPHS, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS, sizeof(mln_runtime_event_glyphs),
   .message = MESSAGE_TEXT, .expected_message = unrouted_message,
   .check = check_glyphs},
  {MLN_RUNTIME_EVENT_MAP_SPRITE_REQUESTED, SCENARIO_SPRITES_AND_GLYPHS, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_SPRITE, sizeof(mln_runtime_event_sprite),
   .check = check_sprite},
  {MLN_RUNTIME_EVENT_MAP_SPRITE_LOADED, SCENARIO_SPRITES_AND_GLYPHS, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_SPRITE, sizeof(mln_runtime_event_sprite),
   .check = check_sprite},
  {MLN_RUNTIME_EVENT_MAP_SPRITE_ERROR, SCENARIO_SPRITES_AND_GLYPHS, MAP,
   MLN_RUNTIME_EVENT_PAYLOAD_SPRITE, sizeof(mln_runtime_event_sprite),
   .message = MESSAGE_TEXT, .expected_message = unrouted_message,
   .check = check_sprite},
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
  if (row->message == MESSAGE_NONE) {
    TEST_ASSERT_EQUAL_UINT32(0, event->message.size);
  } else {
    TEST_ASSERT_GREATER_THAN_UINT32(0, event->message.size);
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

static void every_event_type_has_one_payload_row(void) {
  size_t types = 0;
  for (uint32_t type = 0; type < 64; type += 1) {
    if ((MLN_RUNTIME_EVENT_FLAG_ALL & (UINT64_C(1) << type)) == 0) {
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
  double duration_ms
) {
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.mode = mode;
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = zoom;
  update.animation.fields = animation_fields;
  update.animation.duration_ms = duration_ms;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

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

static void camera_events_carry_their_change_mode(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const scenario_context context = {.runtime = runtime, .map = map};
  mln_test_drain_all(runtime);
  event_log* log = &scenario_log;
  log_reset(log);

  submit_camera(map, MLN_CAMERA_UPDATE_MODE_JUMP, 2.0, 0, 0.0);
  log_drain(runtime, log);
  const recorded_event* jump_will =
    log_find(log, MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE, map);
  TEST_ASSERT_NOT_NULL(jump_will);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_CAMERA_CHANGE_MODE_IMMEDIATE,
    jump_will->event.payload.camera_change.mode
  );
  const recorded_event* jump_did =
    log_find(log, MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE, map);
  TEST_ASSERT_NOT_NULL(jump_did);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_CAMERA_CHANGE_MODE_IMMEDIATE, jump_did->event.payload.camera_change.mode
  );

  const size_t ease_start = log->count;
  submit_camera(
    map, MLN_CAMERA_UPDATE_MODE_EASE, 6.0, MLN_ANIMATION_OPTION_DURATION,
    60000.0
  );
  log_drain(runtime, log);
  const size_t ease_will =
    log_next(log, ease_start, MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE, map);
  TEST_ASSERT_LESS_THAN_size_t(log->count, ease_will);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_CAMERA_CHANGE_MODE_ANIMATED,
    log->events[ease_will].event.payload.camera_change.mode
  );
  // Stopping the animation ends it with an animated change.
  MLN_TEST_AWAIT_OK(
    mln_map_cancel_camera_transitions(map, &completion.descriptor, NULL)
  );
  log_drain(runtime, log);
  const size_t ease_did =
    log_next(log, ease_will, MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE, map);
  TEST_ASSERT_LESS_THAN_size_t(log->count, ease_did);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_CAMERA_CHANGE_MODE_ANIMATED,
    log->events[ease_did].event.payload.camera_change.mode
  );

  check_scenario_rows(SCENARIO_CAMERA, log, &context);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Drains the runtime's events too when `log` is set.
static void render_one_frame(
  const mln_test_render_fixture* fixture, mln_runtime runtime, event_log* log
) {
  mln_frame_demand demand = {.size = sizeof(mln_frame_demand)};
  MLN_TEST_OK(mln_render_session_demand_frame(fixture->session, &demand, NULL));
  const mln_render_frame_batch frames =
    mln_test_render_wait_for_results(fixture);
  mln_render_frame_batch_release(frames);
  if (log != NULL) {
    log_drain(runtime, log);
  }
}

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

static void rendering_a_style_to_idle_reports_its_lifecycle(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const scenario_context context = {.runtime = runtime, .map = map};
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  mln_test_drain_all(runtime);
  event_log* log = &scenario_log;
  log_reset(log);

  MLN_TEST_AWAIT_OK(mln_map_set_style_json(
    map, MLN_BUFFER_LITERAL(render_style_json), &completion.descriptor, NULL
  ));
  render_until_logged(&fixture, runtime, log, MLN_RUNTIME_EVENT_MAP_IDLE, map);

  submit_camera(
    map, MLN_CAMERA_UPDATE_MODE_EASE, 4.0, MLN_ANIMATION_OPTION_DURATION,
    60000.0
  );
  render_until_logged(
    &fixture, runtime, log, MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING, map
  );
  submit_camera(map, MLN_CAMERA_UPDATE_MODE_JUMP, 4.0, 0, 0.0);
  log_drain(runtime, log);

  int64_t frame_count = -1;
  for (size_t index = 0; index < log->count; index += 1) {
    const mln_runtime_event* event = &log->events[index].event;
    if (event->type == MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED) {
      TEST_ASSERT_GREATER_THAN_INT64(
        frame_count, event->payload.render_frame.stats.frame_count
      );
      frame_count = event->payload.render_frame.stats.frame_count;
    }
  }
  TEST_ASSERT_GREATER_THAN_INT64(0, frame_count);
  check_scenario_rows(SCENARIO_RENDER, log, &context);

  // The style names no sprite, which MapLibre reports as one empty sprite.
  const recorded_event* sprite =
    log_find(log, MLN_RUNTIME_EVENT_MAP_SPRITE_LOADED, map);
  TEST_ASSERT_NOT_NULL(sprite);
  TEST_ASSERT_EQUAL_STRING("", sprite->strings[0]);
  TEST_ASSERT_EQUAL_STRING("", sprite->strings[1]);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_map create_static_map(mln_runtime runtime, uint64_t event_mask) {
  mln_map_options options = mln_test_map_options();
  options.map_mode = MLN_MAP_MODE_STATIC;
  options.initial_extent.width = 64;
  options.initial_extent.height = 64;
  options.fields |= MLN_MAP_OPTION_EVENT_MASK;
  options.event_mask = event_mask;
  return mln_test_create_map_with_options(runtime, &options);
}

static mln_status render_still_image(
  mln_test_render_fixture* fixture, mln_map map, char* out_diagnostic,
  size_t diagnostic_capacity
) {
  mln_test_completion still = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_request_still_image(map, &still.descriptor, NULL));
  const mln_status status =
    mln_test_render_pending_still_image(fixture, &still);
  if (out_diagnostic != NULL) {
    snprintf(
      out_diagnostic, diagnostic_capacity, "%s",
      mln_test_completion_diagnostic(&still)
    );
  }
  mln_test_completion_destroy(&still);
  return status;
}

// Answers every request with a not-found error, so every resource the library
// asks for fails without reaching the network.
static void deny_every_request(
  void* user_data, const mln_resource_request_info* request,
  mln_resource_request handle
) {
  (void)user_data;
  (void)request;
  const mln_resource_response response = {
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_ERROR,
    .error_reason = MLN_RESOURCE_ERROR_REASON_NOT_FOUND,
    .error_message = mln_buffer_view_from_cstr(denied_message),
  };
  mln_test_request_answer(handle, &response);
}

static bool offline_events_logged(void* context) {
  scenario_context* scenario = context;
  log_drain_quietly(scenario->runtime, &scenario_log);
  return scenario_log.drain_failed || scenario_log.overflowed ||
         (log_contains(
            &scenario_log, MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR,
            scenario->runtime
          ) &&
          log_contains(
            &scenario_log, MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED,
            scenario->runtime
          ));
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

static void failed_loads_report_their_text(void) {
  mln_runtime runtime = mln_test_create_runtime();
  deny_requests(runtime);
  mln_map map = create_static_map(runtime, MLN_RUNTIME_EVENT_FLAG_ALL);
  const scenario_context context = {.runtime = runtime, .map = map};
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  mln_test_drain_all(runtime);
  event_log* log = &scenario_log;
  log_reset(log);

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
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
      &fixture, map, still_diagnostic, sizeof(still_diagnostic)
    )
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(still_diagnostic, denied_message), still_diagnostic
  );
  // The failed style command queued its loading-failed event before completing.
  log_drain(runtime, log);
  check_scenario_rows(SCENARIO_STILL_IMAGE_FAILURE, log, &context);
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static const char unparsable_style_url[] = "test://styles/unparsable.json";
static const char empty_style_url[] = "test://styles/empty.json";
static const char missing_style_url[] = "test://styles/missing.json";

static const mln_test_provided_resource failing_style_resources[] = {
  {.url = unparsable_style_url,
   .response =
     {
       .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
       .bytes = (const uint8_t*)"{",
       .byte_count = 1,
     }},
  {.url = empty_style_url,
   .response = {.status = MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT}},
};

// Loads `url`, which fails, and checks the kind that its loading-failed event
// names.
static void expect_style_url_failure(
  mln_runtime runtime, mln_map map, const char* url, uint32_t expected
) {
  log_reset(&scenario_log);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_NATIVE_ERROR,
    mln_map_set_style_url(
      map, mln_buffer_view_from_cstr(url), &completion.descriptor, NULL
    )
  );
  log_drain(runtime, &scenario_log);
  const recorded_event* failed =
    log_find(&scenario_log, MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, map);
  TEST_ASSERT_NOT_NULL_MESSAGE(failed, url);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RUNTIME_EVENT_PAYLOAD_LOADING_FAILED, failed->event.payload_type
  );
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(
    expected, failed->event.payload.loading_failed.error, url
  );
  TEST_ASSERT_GREATER_THAN_UINT32(0, failed->event.message.size);
}

// A style that doesn't parse reports a parse error whether it came as JSON or
// from a URL. A style request that fails, or whose first response has no
// content, reports a load error.
static void a_failed_style_load_names_its_kind(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider* provider = mln_test_provider_create(
    failing_style_resources,
    sizeof(failing_style_resources) / sizeof(*failing_style_resources)
  );
  mln_test_provider_install(runtime, provider);
  mln_map map = mln_test_create_map(runtime);
  mln_test_drain_all(runtime);

  expect_style_url_failure(
    runtime, map, unparsable_style_url, MLN_MAP_LOAD_ERROR_STYLE_PARSE
  );
  expect_style_url_failure(
    runtime, map, empty_style_url, MLN_MAP_LOAD_ERROR_STYLE_LOAD
  );
  expect_style_url_failure(
    runtime, map, missing_style_url, MLN_MAP_LOAD_ERROR_STYLE_LOAD
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

// The render style, plus the source that only a TileJSON describes, so one
// render reports a missing image, tile actions, and a source change.
static const char id_style_json[] =
  "{\"version\":8,\"sources\":{\"points\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0,0]},\"properties\":{}}},"
  "\"described\":{\"type\":\"vector\",\"url\":\"test://tiles.json\"}},"
  "\"layers\":[{\"id\":\"dot\",\"type\":\"circle\",\"source\":\"points\"},"
  "{\"id\":\"icon\",\"type\":\"symbol\",\"source\":\"points\","
  "\"layout\":{\"icon-image\":\"missing-icon\"}}]}";

// What a callback-mode handler found, while the payload's views were lent.
typedef struct lent_ids {
  atomic_bool image_id;
  atomic_bool tile_source_id;
  atomic_bool changed_source_id;
} lent_ids;

static bool view_equals(mln_buffer_view view, const char* text) {
  return view.size == strlen(text) &&
         (view.size == 0 || memcmp(view.data, text, view.size) == 0);
}

static void find_lent_ids(void* user_data, const mln_event_batch_view* events) {
  lent_ids* found = user_data;
  for (size_t index = 0; index < events->event_count; index += 1) {
    const mln_runtime_event* event = MLN_STRIDED_AT(
      mln_runtime_event, events->events, events->event_size, index
    );
    switch (event->payload_type) {
      case MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE:
        if (view_equals(event->payload.style_image.image_id, missing_image_id))
          mln_test_flag_set(&found->image_id);
        break;
      case MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION:
        if (
          view_equals(event->payload.tile_action.source_id, geojson_source_id)
        )
          mln_test_flag_set(&found->tile_source_id);
        break;
      case MLN_RUNTIME_EVENT_PAYLOAD_SOURCE_CHANGED:
        if (
          view_equals(
            event->payload.source_changed.source_id, tilejson_source_id
          )
        )
          mln_test_flag_set(&found->changed_source_id);
        break;
      default:
        break;
    }
  }
}

static bool every_lent_id_found(const lent_ids* found) {
  return atomic_load(&found->image_id) && atomic_load(&found->tile_source_id) &&
         atomic_load(&found->changed_source_id);
}

// A handler without a delivery queue reads the IDs while native lends them.
static void a_callback_handler_reads_the_ids_in_its_payloads(void) {
  mln_runtime runtime = mln_test_create_runtime();
  static const mln_test_provided_resource resources[] = {
    {.url = tilejson_url,
     .response = {
       .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
       .bytes = (const uint8_t*)tilejson,
       .byte_count = sizeof(tilejson) - 1,
     }},
  };
  mln_test_provider* provider = mln_test_provider_create(resources, 1);
  mln_test_provider_install(runtime, provider);
  static lent_ids found;
  atomic_store(&found.image_id, false);
  atomic_store(&found.tile_source_id, false);
  atomic_store(&found.changed_source_id, false);
  const mln_runtime_event_handler handler = {
    .size = sizeof(handler),
    .callback = find_lent_ids,
    .user_data = &found,
  };
  MLN_TEST_AWAIT_OK(mln_runtime_set_event_handler(
    runtime, &handler, &completion.descriptor, NULL
  ));
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));

  MLN_TEST_AWAIT_OK(mln_map_set_style_json(
    map, MLN_BUFFER_LITERAL(id_style_json), &completion.descriptor, NULL
  ));
  const mln_test_deadline deadline = mln_test_deadline_default();
  mln_test_watchdog_note("rendering until every payload ID arrives");
  while (!every_lent_id_found(&found)) {
    TEST_ASSERT_FALSE_MESSAGE(
      mln_test_deadline_passed(deadline), "a payload ID never arrived"
    );
    render_one_frame(&fixture, runtime, NULL);
  }
  mln_test_watchdog_note(NULL);
  MLN_TEST_AWAIT_OK(
    mln_runtime_clear_event_handler(runtime, &completion.descriptor, NULL)
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

static void an_observed_offline_download_reports_status_and_errors(void) {
  mln_runtime runtime = mln_test_create_runtime();
  deny_requests(runtime);

  const mln_offline_region_definition definition = {
    .size = sizeof(mln_offline_region_definition),
    .type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
    .data.tile_pyramid = {
      .style_url = mln_buffer_view_from_cstr(offline_style_url),
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
    runtime, info.id, MLN_RUNTIME_EVENT_FLAG_ALL_RUNTIME_EVENTS,
    &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_runtime_set_offline_region_download_state(
    runtime, info.id, MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE,
    &completion.descriptor, NULL
  ));
  TEST_ASSERT_TRUE(mln_test_await(
    offline_events_logged, &context, mln_test_deadline_default(),
    "offline status and response error events"
  ));
  log_drain(runtime, &scenario_log);
  MLN_TEST_AWAIT_OK(mln_runtime_set_offline_region_download_state(
    runtime, info.id, MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE,
    &completion.descriptor, NULL
  ));
  log_drain(runtime, &scenario_log);
  check_scenario_rows(SCENARIO_OFFLINE, &scenario_log, &context);

  mln_test_destroy_runtime(runtime);
}

static bool source_change_logged(void* context) {
  scenario_context* scenario = context;
  log_drain_quietly(scenario->runtime, &scenario_log);
  return scenario_log.drain_failed || scenario_log.overflowed ||
         log_contains(
           &scenario_log, MLN_RUNTIME_EVENT_MAP_SOURCE_CHANGED, scenario->map
         );
}

typedef struct attribution_probe {
  atomic_bool done;
  mln_status status;
  char attribution[32];
} attribution_probe;

static void copy_attribution(
  void* user_data, const mln_completion_result* result
) {
  attribution_probe* probe = user_data;
  probe->status = result->status;
  if (result->status == MLN_STATUS_OK && result->value_count == 1) {
    const mln_style_source_info* info = result->value;
    snprintf(
      probe->attribution, sizeof(probe->attribution), "%.*s",
      (int)info->attribution.size, (const char*)info->attribution.data
    );
  }
  mln_test_flag_set(&probe->done);
}

static void a_source_reports_the_description_its_tilejson_brings(void) {
  mln_runtime runtime = mln_test_create_runtime();
  static const mln_test_provided_resource resources[] = {
    {.url = tilejson_url,
     .response = {
       .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
       .bytes = (const uint8_t*)tilejson,
       .byte_count = sizeof(tilejson) - 1,
     }},
  };
  mln_test_provider* provider = mln_test_provider_create(resources, 1);
  mln_test_provider_install(runtime, provider);
  mln_map map = mln_test_create_map(runtime);
  scenario_context context = {.runtime = runtime, .map = map};
  mln_test_drain_all(runtime);
  log_reset(&scenario_log);

  MLN_TEST_AWAIT_OK(mln_map_set_style_json(
    map, MLN_BUFFER_LITERAL(tilejson_style_json), &completion.descriptor, NULL
  ));
  TEST_ASSERT_TRUE(mln_test_await(
    source_change_logged, &context, mln_test_deadline_default(),
    "a source change"
  ));
  check_scenario_rows(SCENARIO_SOURCE_DESCRIPTION, &scenario_log, &context);

  attribution_probe probe = {.status = MLN_STATUS_INVALID_STATE};
  atomic_init(&probe.done, false);
  const mln_completion query = {
    .size = sizeof(mln_completion),
    .callback = copy_attribution,
    .user_data = &probe,
  };
  MLN_TEST_OK(mln_map_get_style_source(
    map, mln_buffer_view_from_cstr(tilejson_source_id), &query, NULL
  ));
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe.done));
  MLN_TEST_OK(probe.status);
  TEST_ASSERT_EQUAL_STRING("Served tiles", probe.attribution);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

static void sprites_and_glyphs_name_the_resource_they_report(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider* provider = mln_test_provider_create(
    sprite_and_glyph_resources,
    sizeof(sprite_and_glyph_resources) / sizeof(*sprite_and_glyph_resources)
  );
  mln_test_provider_install(runtime, provider);
  mln_map map = mln_test_create_map(runtime);
  const scenario_context context = {.runtime = runtime, .map = map};
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  mln_test_drain_all(runtime);
  event_log* log = &scenario_log;
  log_reset(log);

  MLN_TEST_AWAIT_OK(mln_map_set_style_json(
    map, MLN_BUFFER_LITERAL(resource_style_json), &completion.descriptor, NULL
  ));
  static const uint32_t awaited[] = {
    MLN_RUNTIME_EVENT_MAP_SPRITE_REQUESTED,
    MLN_RUNTIME_EVENT_MAP_SPRITE_LOADED,
    MLN_RUNTIME_EVENT_MAP_SPRITE_ERROR,
    MLN_RUNTIME_EVENT_MAP_GLYPHS_REQUESTED,
    MLN_RUNTIME_EVENT_MAP_GLYPHS_LOADED,
    MLN_RUNTIME_EVENT_MAP_GLYPHS_ERROR,
  };
  for (size_t index = 0; index < sizeof(awaited) / sizeof(*awaited);
       index += 1) {
    render_until_logged(&fixture, runtime, log, awaited[index], map);
  }
  check_scenario_rows(SCENARIO_SPRITES_AND_GLYPHS, log, &context);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

static bool sprite_error_logged(void* context) {
  scenario_context* scenario = context;
  log_drain_quietly(scenario->runtime, &scenario_log);
  return scenario_log.drain_failed || scenario_log.overflowed ||
         log_contains(
           &scenario_log, MLN_RUNTIME_EVENT_MAP_SPRITE_ERROR, scenario->map
         );
}

static void a_failed_sprite_names_itself_to_a_failed_still_image(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider* provider = mln_test_provider_create(NULL, 0);
  mln_test_provider_install(runtime, provider);
  mln_map map =
    create_static_map(runtime, MLN_RUNTIME_EVENT_FLAG_MAP_SPRITE_ERROR);
  scenario_context context = {.runtime = runtime, .map = map};
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  log_reset(&scenario_log);

  MLN_TEST_AWAIT_OK(mln_map_set_style_json(
    map,
    MLN_BUFFER_LITERAL(
      "{\"version\":8,\"sprite\":\"test://sprites/bad\",\"sources\":{},"
      "\"layers\":[{\"id\":\"bg\",\"type\":\"background\"}]}"
    ),
    &completion.descriptor, NULL
  ));
  char still_diagnostic[MLN_DIAGNOSTIC_MESSAGE_CAPACITY] = "";
  MLN_TEST_STATUS(
    MLN_STATUS_NATIVE_ERROR,
    render_still_image(
      &fixture, map, still_diagnostic, sizeof(still_diagnostic)
    )
  );
  TEST_ASSERT_EQUAL_STRING(unrouted_message, still_diagnostic);
  // MapLibre fails the image before it reports the sprite's error.
  TEST_ASSERT_TRUE(mln_test_await(
    sprite_error_logged, &context, mln_test_deadline_default(), "a sprite error"
  ));
  log_drain(runtime, &scenario_log);
  const recorded_event* error =
    log_find(&scenario_log, MLN_RUNTIME_EVENT_MAP_SPRITE_ERROR, map);
  TEST_ASSERT_NOT_NULL(error);
  TEST_ASSERT_EQUAL_STRING(still_diagnostic, error->message);
  TEST_ASSERT_EQUAL_STRING("default", error->strings[0]);
  TEST_ASSERT_EQUAL_STRING("test://sprites/bad", error->strings[1]);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

// The still image's completion fences the check.
static void a_map_created_with_an_empty_mask_queues_nothing(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_static_map(runtime, MLN_RUNTIME_EVENT_FLAG_NONE);
  MLN_TEST_AWAIT_OK(mln_map_set_style_json(
    map, mln_test_background_style_json, &completion.descriptor, NULL
  ));
  submit_camera(map, MLN_CAMERA_UPDATE_MODE_JUMP, 2.0, 0, 0.0);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  MLN_TEST_OK(render_still_image(&fixture, map, NULL, 0));

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
  RUN_TEST(camera_events_carry_their_change_mode);
  RUN_TEST(rendering_a_style_to_idle_reports_its_lifecycle);
  RUN_TEST(failed_loads_report_their_text);
  RUN_TEST(a_failed_style_load_names_its_kind);
  RUN_TEST(a_callback_handler_reads_the_ids_in_its_payloads);
  RUN_TEST(an_observed_offline_download_reports_status_and_errors);
  RUN_TEST(a_source_reports_the_description_its_tilejson_brings);
  RUN_TEST(sprites_and_glyphs_name_the_resource_they_report);
  RUN_TEST(a_failed_sprite_names_itself_to_a_failed_still_image);
  RUN_TEST(a_map_created_with_an_empty_mask_queues_nothing);
}
