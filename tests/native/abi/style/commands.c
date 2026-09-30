// Style command semantics shared by every style edit: copied inputs, ordered
// reads, terminal statuses for duplicate and missing IDs, and the style-wide
// values (global state, transitions, and light) that belong to no layer.

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static bool source_exists(mln_map map, const char* id) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_source_result));
  const mln_buffer_view view = mln_test_view_of(id);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_source_info(map, view, &completion.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&completion));
  const bool found = mln_test_completion_value_count(&completion) == 1;
  mln_test_completion_destroy(&completion);
  return found;
}

static bool image_exists(mln_map map, const char* id) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_image_result));
  const mln_buffer_view view = mln_test_view_of(id);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_image_info(map, view, &completion.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&completion));
  const bool found = mln_test_completion_value_count(&completion) == 1;
  mln_test_completion_destroy(&completion);
  return found;
}

static void style_command_deep_copies_and_ordered_read_observes_it(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  char id[] = "owned-source";
  char json[sizeof(MLN_TEST_EMPTY_GEOJSON_SOURCE)];
  memcpy(json, MLN_TEST_EMPTY_GEOJSON_SOURCE, sizeof(json));
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_style_source_json(
                     map, mln_test_view_of(id), mln_test_view_of(json),
                     &completion.descriptor, NULL
                   )
  );
  memset(id, 'x', strlen(id));
  memset(json, ' ', strlen(json));
  TEST_ASSERT_TRUE(source_exists(map, "owned-source"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void duplicate_id_is_an_async_failed_terminal_event(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_buffer_view id = MLN_BUFFER_LITERAL("duplicate");
  const mln_buffer_view json =
    MLN_BUFFER_LITERAL(MLN_TEST_EMPTY_GEOJSON_SOURCE);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_style_source_json(map, id, json, &completion.descriptor, NULL)
  );
  mln_test_completion duplicate = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_style_source_json(map, id, json, &duplicate.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_test_completion_finish(&duplicate)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_FAILED, mln_test_completion_disposition(&duplicate)
  );
  mln_test_completion_destroy(&duplicate);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void remove_commands_commit_and_report_missing_ids(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_style_source_json(
                     map, MLN_BUFFER_LITERAL("doomed-source"),
                     MLN_BUFFER_LITERAL(MLN_TEST_EMPTY_GEOJSON_SOURCE),
                     &completion.descriptor, NULL
                   )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_remove_style_source(
      map, MLN_BUFFER_LITERAL("doomed-source"), &completion.descriptor, NULL
    )
  );
  TEST_ASSERT_FALSE(source_exists(map, "doomed-source"));
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "doomed-source",
    mln_map_remove_style_source(
      map, MLN_BUFFER_LITERAL("doomed-source"), &completion.descriptor, NULL
    )
  );

  const uint8_t pixel[4] = {0, 0, 0, 0};
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 1;
  image.height = 1;
  image.stride = 4;
  image.pixels = pixel;
  image.byte_length = sizeof(pixel);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_style_image(
                     map, MLN_BUFFER_LITERAL("doomed-image"), &image, NULL,
                     &completion.descriptor, NULL
                   )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_remove_style_image(
      map, MLN_BUFFER_LITERAL("doomed-image"), &completion.descriptor, NULL
    )
  );
  TEST_ASSERT_FALSE(image_exists(map, "doomed-image"));
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "doomed-image",
    mln_map_remove_style_image(
      map, MLN_BUFFER_LITERAL("doomed-image"), &completion.descriptor, NULL
    )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// One call that names a style ID the map does not hold.
typedef struct missing_id_case {
  const char* label;
  mln_status (*submit)(mln_map map, const mln_completion* completion);
  // Queries complete with a status; commands also fail their disposition.
  bool query;
  const char* fragment;
} missing_id_case;

static const mln_buffer_view absent_layer = MLN_BUFFER_LITERAL("absent-layer");
static const mln_buffer_view absent_source =
  MLN_BUFFER_LITERAL("absent-source");
static const mln_lat_lng image_corners[4] = {
  {.latitude = 1.0, .longitude = 0.0},
  {.latitude = 1.0, .longitude = 1.0},
  {.latitude = 0.0, .longitude = 1.0},
  {.latitude = 0.0, .longitude = 0.0},
};

static mln_status remove_layer(mln_map map, const mln_completion* completion) {
  return mln_map_remove_style_layer(map, absent_layer, completion, NULL);
}
static mln_status move_layer(mln_map map, const mln_completion* completion) {
  return mln_map_move_style_layer(
    map, absent_layer, MLN_BUFFER_LITERAL(""), completion, NULL
  );
}
static mln_status set_property(mln_map map, const mln_completion* completion) {
  return mln_map_set_layer_property(
    map, absent_layer, MLN_BUFFER_LITERAL("circle-radius"),
    MLN_BUFFER_LITERAL("4"), completion, NULL
  );
}
static mln_status get_property(mln_map map, const mln_completion* completion) {
  return mln_map_get_layer_property(
    map, absent_layer, MLN_BUFFER_LITERAL("circle-radius"), completion, NULL
  );
}
static mln_status set_filter(mln_map map, const mln_completion* completion) {
  return mln_map_set_layer_filter(map, absent_layer, NULL, completion, NULL);
}
static mln_status get_filter(mln_map map, const mln_completion* completion) {
  return mln_map_get_layer_filter(map, absent_layer, completion, NULL);
}
static mln_status set_visibility(
  mln_map map, const mln_completion* completion
) {
  return mln_map_set_layer_visibility(
    map, absent_layer, MLN_STYLE_LAYER_VISIBILITY_NONE, completion, NULL
  );
}
static mln_status set_min_zoom(mln_map map, const mln_completion* completion) {
  return mln_map_set_layer_min_zoom(map, absent_layer, 2.0, completion, NULL);
}
static mln_status set_source_id(mln_map map, const mln_completion* completion) {
  return mln_map_set_layer_source_id(
    map, absent_layer, MLN_BUFFER_LITERAL("points"), completion, NULL
  );
}
static mln_status copy_source_id(
  mln_map map, const mln_completion* completion
) {
  return mln_map_copy_layer_source_id(map, absent_layer, completion, NULL);
}
static mln_status set_source_layer(
  mln_map map, const mln_completion* completion
) {
  return mln_map_set_layer_source_layer(
    map, absent_layer, MLN_BUFFER_LITERAL("roads"), completion, NULL
  );
}
static mln_status copy_source_layer(
  mln_map map, const mln_completion* completion
) {
  return mln_map_copy_layer_source_layer(map, absent_layer, completion, NULL);
}
static mln_status set_indicator_bearing(
  mln_map map, const mln_completion* completion
) {
  return mln_map_set_location_indicator_bearing(
    map, absent_layer, 90.0, completion, NULL
  );
}
static mln_status add_indicator_before(
  mln_map map, const mln_completion* completion
) {
  return mln_map_add_location_indicator_layer(
    map, MLN_BUFFER_LITERAL("indicator"), absent_layer, completion, NULL
  );
}
static mln_status add_hillshade(mln_map map, const mln_completion* completion) {
  return mln_map_add_hillshade_layer(
    map, MLN_BUFFER_LITERAL("hillshade"), absent_source, MLN_BUFFER_LITERAL(""),
    completion, NULL
  );
}
static mln_status remove_source(mln_map map, const mln_completion* completion) {
  return mln_map_remove_style_source(map, absent_source, completion, NULL);
}
static mln_status set_image_source_url(
  mln_map map, const mln_completion* completion
) {
  return mln_map_set_image_source_url(
    map, absent_source, MLN_BUFFER_LITERAL("fixture://image.png"), completion,
    NULL
  );
}
static mln_status set_image_source_coordinates(
  mln_map map, const mln_completion* completion
) {
  return mln_map_set_image_source_coordinates(
    map, absent_source, image_corners, 4, completion, NULL
  );
}
static mln_status set_geojson_url(
  mln_map map, const mln_completion* completion
) {
  return mln_map_set_geojson_source_url(
    map, absent_source, MLN_BUFFER_LITERAL("fixture://points.geojson"),
    completion, NULL
  );
}
static mln_status set_synchronous_tiling(
  mln_map map, const mln_completion* completion
) {
  return mln_map_set_geojson_source_synchronous_tiling(
    map, absent_source, true, completion, NULL
  );
}
static mln_status set_volatile(mln_map map, const mln_completion* completion) {
  return mln_map_set_style_source_volatile(
    map, absent_source, true, completion, NULL
  );
}
static mln_status invalidate_region(
  mln_map map, const mln_completion* completion
) {
  const mln_lat_lng_bounds bounds = {
    .southwest = {.latitude = -1.0, .longitude = -1.0},
    .northeast = {.latitude = 1.0, .longitude = 1.0},
  };
  return mln_map_invalidate_custom_geometry_source_region(
    map, absent_source, bounds, completion, NULL
  );
}

// Every command and query that names a style ID reports a missing one the same
// way, whether it removes, mutates, or reads: NOT_FOUND through the
// completion, with the diagnostic naming what is missing.
static void missing_style_ids_report_not_found(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  static const missing_id_case cases[] = {
    {"remove layer", remove_layer, false, "absent-layer"},
    {"move layer", move_layer, false, "layer does not exist"},
    {"set layer property", set_property, false, "layer does not exist"},
    {"get layer property", get_property, true, NULL},
    {"set layer filter", set_filter, false, "layer does not exist"},
    {"get layer filter", get_filter, true, NULL},
    {"set layer visibility", set_visibility, false, "layer does not exist"},
    {"set layer min zoom", set_min_zoom, false, "layer does not exist"},
    {"set layer source id", set_source_id, false, "layer does not exist"},
    {"copy layer source id", copy_source_id, true, NULL},
    {"set layer source layer", set_source_layer, false, "layer does not exist"},
    {"copy layer source layer", copy_source_layer, true, NULL},
    {"set location indicator bearing", set_indicator_bearing, false,
     "layer does not exist"},
    {"add layer before a missing layer", add_indicator_before, false,
     "before_layer_id does not exist"},
    {"add hillshade on a missing source", add_hillshade, false,
     "source does not exist"},
    {"remove source", remove_source, false, "absent-source"},
    {"set image source url", set_image_source_url, false,
     "source does not exist"},
    {"set image source coordinates", set_image_source_coordinates, false,
     "source does not exist"},
    {"set geojson source url", set_geojson_url, false, "source does not exist"},
    {"set synchronous tiling", set_synchronous_tiling, false,
     "source does not exist"},
    {"set source volatility", set_volatile, false, "absent-source"},
    {"invalidate custom geometry region", invalidate_region, false,
     "source does not exist"},
  };
  for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); index += 1) {
    const missing_id_case* row = &cases[index];
    mln_test_completion completion = mln_test_completion_default(0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_OK, row->submit(map, &completion.descriptor), row->label
    );
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_NOT_FOUND, mln_test_completion_finish(&completion), row->label
    );
    if (!row->query) {
      TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        MLN_COMMAND_DISPOSITION_FAILED,
        mln_test_completion_disposition(&completion), row->label
      );
    }
    if (row->fragment != NULL) {
      TEST_ASSERT_NOT_NULL_MESSAGE(
        strstr(mln_test_completion_diagnostic(&completion), row->fragment),
        row->label
      );
    }
    mln_test_completion_destroy(&completion);
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void global_state_checks_views_and_completion(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_completion discard = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_global_state(map, NULL, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_global_state_property(
      map, (mln_buffer_view){.data = NULL, .size = 1},
      MLN_BUFFER_LITERAL("true"), &discard, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_STATE, "style JSON has not loaded",
    mln_map_set_global_state_property(
      map, MLN_BUFFER_LITERAL("theme"), MLN_BUFFER_LITERAL("true"),
      &completion.descriptor, NULL
    )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Reads the style's global state as JSON.
static void read_global_state(mln_map map, char* out, size_t capacity) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_global_state(map, &completion.descriptor, NULL)
  );
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_style_finish_text(&completion, out, capacity, &found)
  );
  TEST_ASSERT_TRUE(found);
}

static void set_global_state(mln_map map, const char* value) {
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_global_state_property(
                     map, MLN_BUFFER_LITERAL("theme"), mln_test_view_of(value),
                     &completion.descriptor, NULL
                   )
  );
}

// Global state starts from the defaults the style declares. A set property
// shadows its default until it is set to null, a value that is not JSON fails
// the command and leaves the state alone, and loading a style again discards
// what the host set. A style without state declarations holds only what the
// host sets.
static void global_state_reads_back_defaults_and_set_properties(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  static const char stateful_style[] =
    "{\"version\":8,\"sources\":{},\"layers\":[],"
    "\"state\":{\"theme\":{\"default\":\"light\"}}}";
  mln_test_load_style_and_wait(
    runtime, map, MLN_BUFFER_LITERAL(stateful_style)
  );
  char state[128];
  read_global_state(map, state, sizeof(state));
  TEST_ASSERT_EQUAL_STRING("{\"theme\":\"light\"}", state);

  set_global_state(map, "[\"dark\",{\"enabled\":true}]");
  read_global_state(map, state, sizeof(state));
  TEST_ASSERT_EQUAL_STRING("{\"theme\":[\"dark\",{\"enabled\":true}]}", state);

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "",
    mln_map_set_global_state_property(
      map, MLN_BUFFER_LITERAL("theme"), MLN_BUFFER_LITERAL("{not json"),
      &completion.descriptor, NULL
    )
  );
  read_global_state(map, state, sizeof(state));
  TEST_ASSERT_EQUAL_STRING("{\"theme\":[\"dark\",{\"enabled\":true}]}", state);

  set_global_state(map, "null");
  read_global_state(map, state, sizeof(state));
  TEST_ASSERT_EQUAL_STRING("{\"theme\":\"light\"}", state);

  set_global_state(map, "\"dark\"");
  mln_test_load_style_and_wait(
    runtime, map, MLN_BUFFER_LITERAL(stateful_style)
  );
  read_global_state(map, state, sizeof(state));
  TEST_ASSERT_EQUAL_STRING("{\"theme\":\"light\"}", state);

  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  read_global_state(map, state, sizeof(state));
  TEST_ASSERT_EQUAL_STRING("{}", state);
  set_global_state(map, "true");
  read_global_state(map, state, sizeof(state));
  TEST_ASSERT_EQUAL_STRING("{\"theme\":true}", state);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void read_loaded_style_json(mln_map map, char* out, size_t capacity) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_loaded_style_json(map, &completion.descriptor, NULL)
  );
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_style_finish_text(&completion, out, capacity, &found)
  );
  TEST_ASSERT_TRUE(found);
}

// The loaded document reads back byte for byte, so a host can reload it
// unchanged. Later edits, such as an added source, do not rewrite it, and a
// map that parsed nothing reads back empty.
static void the_loaded_style_document_reads_back_as_loaded(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  char document[512];
  read_loaded_style_json(map, document, sizeof(document));
  TEST_ASSERT_EQUAL_STRING("", document);

  static const char named_style[] =
    "{\"version\":8,\"name\":\"loaded\",\"sources\":{},\"layers\":[]}";
  mln_test_load_style_and_wait(runtime, map, MLN_BUFFER_LITERAL(named_style));
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_style_source_json(
                     map, MLN_BUFFER_LITERAL("added-later"),
                     MLN_BUFFER_LITERAL(MLN_TEST_EMPTY_GEOJSON_SOURCE),
                     &completion.descriptor, NULL
                   )
  );
  read_loaded_style_json(map, document, sizeof(document));
  TEST_ASSERT_EQUAL_STRING(named_style, document);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Reads the style's transition configuration back through the ordered query.
static mln_style_transition_options read_transition_options(mln_map map) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_transition_options));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_transition_options(map, &completion.descriptor, NULL)
  );
  mln_style_transition_options value = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&completion, &value, sizeof(value))
  );
  return value;
}

// A map with no style reports no duration, a style without a transition
// member reports MapLibre Native's 300 ms default, and a style whose
// transition names only a delay reports only that delay.
static void a_loaded_style_reports_the_300_ms_transition_default(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_style_transition_options read = read_transition_options(map);
  TEST_ASSERT_EQUAL_UINT32(
    0, read.fields & (MLN_STYLE_TRANSITION_OPTION_DURATION |
                      MLN_STYLE_TRANSITION_OPTION_DELAY)
  );

  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  read = read_transition_options(map);
  TEST_ASSERT_TRUE((read.fields & MLN_STYLE_TRANSITION_OPTION_DURATION) != 0);
  TEST_ASSERT_EQUAL_DOUBLE(300.0, read.duration_ms);

  mln_test_load_style_and_wait(
    runtime, map,
    MLN_BUFFER_LITERAL(
      "{\"version\":8,\"transition\":{\"delay\":50},\"sources\":{},"
      "\"layers\":[]}"
    )
  );
  read = read_transition_options(map);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_TRANSITION_OPTION_DELAY,
    read.fields &
      (MLN_STYLE_TRANSITION_OPTION_DURATION | MLN_STYLE_TRANSITION_OPTION_DELAY)
  );
  TEST_ASSERT_EQUAL_DOUBLE(50.0, read.delay_ms);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Raw callers can hand the transition setter a struct no binding would build.
// Undersized input is rejected at submission, unusable field values are
// rejected by the command, and neither leaves a half-applied configuration.
static void style_transition_options_reject_unsafe_raw_input(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  mln_style_transition_options applied = mln_style_transition_options_default();
  applied.fields =
    MLN_STYLE_TRANSITION_OPTION_DURATION | MLN_STYLE_TRANSITION_OPTION_DELAY;
  applied.duration_ms = 250.0;
  applied.delay_ms = 75.0;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_style_transition_options(
                     map, &applied, &completion.descriptor, NULL
                   )
  );
  mln_style_transition_options read = read_transition_options(map);
  TEST_ASSERT_EQUAL_DOUBLE(250.0, read.duration_ms);
  TEST_ASSERT_EQUAL_DOUBLE(75.0, read.delay_ms);
  TEST_ASSERT_TRUE(read.enable_placement_transitions);

  // Placement transitions stay on unless a set names them.
  mln_style_transition_options placement =
    mln_style_transition_options_default();
  placement.fields = MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
  placement.enable_placement_transitions = false;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_style_transition_options(
                     map, &placement, &completion.descriptor, NULL
                   )
  );
  const mln_style_transition_options placement_read =
    read_transition_options(map);
  TEST_ASSERT_BITS_HIGH(
    MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS,
    placement_read.fields
  );
  TEST_ASSERT_FALSE(placement_read.enable_placement_transitions);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_style_transition_options(
                     map, &applied, &completion.descriptor, NULL
                   )
  );

  // A null or undersized struct never reaches the map worker.
  mln_completion discard = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_transition_options(map, NULL, &discard, NULL)
  );
  mln_style_transition_options undersized = applied;
  undersized.size = sizeof(mln_style_transition_options) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_transition_options(map, &undersized, &discard, NULL)
  );

  // Unknown bits and unusable durations are the command's rejections.
  mln_style_transition_options unknown = applied;
  unknown.fields |= UINT32_C(1) << 31;
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "unknown bits",
    mln_map_set_style_transition_options(
      map, &unknown, &completion.descriptor, NULL
    )
  );
  mln_style_transition_options negative = applied;
  negative.duration_ms = -1.0;
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "duration_ms",
    mln_map_set_style_transition_options(
      map, &negative, &completion.descriptor, NULL
    )
  );
  mln_style_transition_options not_finite = applied;
  not_finite.delay_ms = INFINITY;
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "delay_ms",
    mln_map_set_style_transition_options(
      map, &not_finite, &completion.descriptor, NULL
    )
  );

  // A rejected set is not a partial set: the configuration is the one that
  // last committed, including the delay the last rejection tried to replace.
  read = read_transition_options(map);
  TEST_ASSERT_EQUAL_DOUBLE(250.0, read.duration_ms);
  TEST_ASSERT_EQUAL_DOUBLE(75.0, read.delay_ms);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Reads one light property as JSON, or reports it undefined with "".
static void read_light_property(mln_map map, const char* name, char* out) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_light_property(
                     map, mln_test_view_of(name), &completion.descriptor, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_style_finish_text(&completion, out, 64, NULL)
  );
}

// The light has no ID to miss, so every bad input is an INVALID_ARGUMENT, and
// a rejected write leaves the light as it was.
static void the_style_light_round_trips_through_json(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_style_light_json(
      map, MLN_BUFFER_LITERAL("{\"anchor\":\"viewport\",\"intensity\":0.5}"),
      &completion.descriptor, NULL
    )
  );
  char value[64];
  read_light_property(map, "anchor", value);
  TEST_ASSERT_EQUAL_STRING("\"viewport\"", value);
  read_light_property(map, "intensity", value);
  TEST_ASSERT_EQUAL_STRING("0.5", value);

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_style_light_property(
                     map, MLN_BUFFER_LITERAL("intensity"),
                     MLN_BUFFER_LITERAL("0.75"), &completion.descriptor, NULL
                   )
  );
  read_light_property(map, "intensity", value);
  TEST_ASSERT_EQUAL_STRING("0.75", value);

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "style light property",
    mln_map_set_style_light_property(
      map, MLN_BUFFER_LITERAL("brightness"), MLN_BUFFER_LITERAL("1"),
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "style light property",
    mln_map_set_style_light_property(
      map, MLN_BUFFER_LITERAL("intensity"), MLN_BUFFER_LITERAL("\"bright\""),
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "style light",
    mln_map_set_style_light_json(
      map, MLN_BUFFER_LITERAL("{\"anchor\":7}"), &completion.descriptor, NULL
    )
  );
  read_light_property(map, "intensity", value);
  TEST_ASSERT_EQUAL_STRING("0.75", value);
  read_light_property(map, "anchor", value);
  TEST_ASSERT_EQUAL_STRING("\"viewport\"", value);

  // An unknown name reads as undefined rather than failing.
  read_light_property(map, "brightness", value);
  TEST_ASSERT_EQUAL_STRING("", value);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(style_command_deep_copies_and_ordered_read_observes_it);
  RUN_TEST(duplicate_id_is_an_async_failed_terminal_event);
  RUN_TEST(remove_commands_commit_and_report_missing_ids);
  RUN_TEST(missing_style_ids_report_not_found);
  RUN_TEST(global_state_checks_views_and_completion);
  RUN_TEST(global_state_reads_back_defaults_and_set_properties);
  RUN_TEST(the_loaded_style_document_reads_back_as_loaded);
  RUN_TEST(a_loaded_style_reports_the_300_ms_transition_default);
  RUN_TEST(style_transition_options_reject_unsafe_raw_input);
  RUN_TEST(the_style_light_round_trips_through_json);
}
