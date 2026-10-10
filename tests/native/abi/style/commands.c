// Style command semantics shared by every style edit: copied inputs, ordered
// reads, terminal statuses for duplicate and missing IDs, and the style-wide
// values (global state, transitions, and light) that belong to no layer.

#include <math.h>

#include "support/style.h"
#include "support/test_support.h"

static bool source_exists(mln_map map, const char* id) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_source_info));
  MLN_TEST_OK(mln_map_get_style_source(
    map, mln_test_view_of(id), &completion.descriptor, NULL
  ));
  mln_style_source_info result;
  bool found = false;
  MLN_TEST_OK(mln_test_completion_finish_optional(
    &completion, &result, sizeof(result), &found
  ));
  return found;
}

static bool image_exists(mln_map map, const char* id) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_image_info));
  MLN_TEST_OK(mln_map_get_style_image(
    map, mln_test_view_of(id), &completion.descriptor, NULL
  ));
  mln_style_image_info result;
  bool found = false;
  MLN_TEST_OK(mln_test_completion_finish_optional(
    &completion, &result, sizeof(result), &found
  ));
  return found;
}

// A command copies its inputs before it returns, adding an ID twice fails the
// second command, and removing an ID twice reports the second removal missing.
static void style_commands_copy_inputs_and_fail_on_duplicate_or_missing_ids(
  void
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  char id[] = "owned-source";
  char json[sizeof(MLN_TEST_EMPTY_GEOJSON_SOURCE)];
  memcpy(json, MLN_TEST_EMPTY_GEOJSON_SOURCE, sizeof(json));
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, mln_test_view_of(id), mln_test_view_of(json), &completion.descriptor,
    NULL
  ));
  memset(id, 'x', strlen(id));
  memset(json, ' ', strlen(json));
  TEST_ASSERT_TRUE(source_exists(map, "owned-source"));

  const mln_buffer_view source = MLN_BUFFER_LITERAL("owned-source");
  const mln_buffer_view empty_source =
    MLN_BUFFER_LITERAL(MLN_TEST_EMPTY_GEOJSON_SOURCE);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "",
    mln_map_add_style_source_json(
      map, source, empty_source, &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_OK(
    mln_map_remove_style_source(map, source, &completion.descriptor, NULL)
  );
  TEST_ASSERT_FALSE(source_exists(map, "owned-source"));
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "owned-source",
    mln_map_remove_style_source(map, source, &completion.descriptor, NULL)
  );

  const uint8_t pixel[4] = {0, 0, 0, 0};
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 1;
  image.height = 1;
  image.stride = 4;
  image.pixels = pixel;
  image.byte_length = sizeof(pixel);
  const mln_buffer_view doomed = MLN_BUFFER_LITERAL("doomed-image");
  MLN_TEST_AWAIT_OK(mln_map_set_style_image(
    map, doomed, &image, NULL, &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(
    mln_map_remove_style_image(map, doomed, &completion.descriptor, NULL)
  );
  TEST_ASSERT_FALSE(image_exists(map, "doomed-image"));
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "doomed-image",
    mln_map_remove_style_image(map, doomed, &completion.descriptor, NULL)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Every command and query that names a style ID reports a missing one the same
// way, whether it removes, mutates, or reads: NOT_FOUND through the
// completion. A command also fails its disposition, with a diagnostic naming
// what is missing.
static void missing_style_ids_report_not_found(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_buffer_view layer = MLN_BUFFER_LITERAL("absent-layer");
  const mln_buffer_view source = MLN_BUFFER_LITERAL("absent-source");
  const mln_buffer_view none = MLN_BUFFER_LITERAL("");
  const char* missing_layer = "layer does not exist";
  const char* missing_source = "source does not exist";

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "absent-layer",
    mln_map_remove_style_layer(map, layer, &completion.descriptor, NULL)
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_layer,
    mln_map_move_style_layer(map, layer, none, &completion.descriptor, NULL)
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_layer,
    mln_map_set_layer_property(
      map, layer, MLN_BUFFER_LITERAL("circle-radius"), MLN_BUFFER_LITERAL("4"),
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_layer,
    mln_map_set_layer_filter(map, layer, NULL, &completion.descriptor, NULL)
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_layer,
    mln_map_set_layer_visibility(
      map, layer, MLN_STYLE_LAYER_VISIBILITY_NONE, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_layer,
    mln_map_set_layer_min_zoom(map, layer, 2.0, &completion.descriptor, NULL)
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_layer,
    mln_map_set_layer_source_id(
      map, layer, MLN_BUFFER_LITERAL("points"), &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_layer,
    mln_map_set_layer_source_layer(
      map, layer, MLN_BUFFER_LITERAL("roads"), &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_layer,
    mln_map_set_location_indicator_bearing(
      map, layer, 90.0, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "before_layer_id does not exist",
    mln_map_add_location_indicator_layer(
      map, MLN_BUFFER_LITERAL("indicator"), layer, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_source,
    mln_map_add_hillshade_layer(
      map, MLN_BUFFER_LITERAL("hillshade"), source, none,
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "absent-source",
    mln_map_remove_style_source(map, source, &completion.descriptor, NULL)
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_source,
    mln_map_set_image_source_url(
      map, source, MLN_BUFFER_LITERAL("fixture://image.png"),
      &completion.descriptor, NULL
    )
  );
  const mln_lat_lng corners[4] = {{1, 0}, {1, 1}, {0, 1}, {0, 0}};
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_source,
    mln_map_set_image_source_coordinates(
      map, source, corners, 4, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_source,
    mln_map_set_geojson_source_url(
      map, source, MLN_BUFFER_LITERAL("fixture://points.geojson"),
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_source,
    mln_map_set_geojson_source_synchronous_tiling(
      map, source, true, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "absent-source",
    mln_map_set_style_source_volatile(
      map, source, true, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_source,
    mln_map_invalidate_custom_geometry_source_region(
      map, source, (mln_lat_lng_bounds){{-1, -1}, {1, 1}},
      &completion.descriptor, NULL
    )
  );

  // A query that reads one attribute of a missing layer has no disposition.
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_NOT_FOUND, mln_map_get_layer_property(
                            map, layer, MLN_BUFFER_LITERAL("circle-radius"),
                            &completion.descriptor, NULL
                          )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_NOT_FOUND,
    mln_map_get_layer_filter(map, layer, &completion.descriptor, NULL)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Reads the style's global state as JSON.
static void read_global_state(mln_map map, char* out, size_t capacity) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  MLN_TEST_OK(mln_map_get_global_state(map, &completion.descriptor, NULL));
  bool found = false;
  MLN_TEST_OK(mln_test_style_finish_text(&completion, out, capacity, &found));
  TEST_ASSERT_TRUE(found);
}

static void set_global_state(mln_map map, const char* value) {
  MLN_TEST_AWAIT_OK(mln_map_set_global_state_property(
    map, MLN_BUFFER_LITERAL("theme"), mln_test_view_of(value),
    &completion.descriptor, NULL
  ));
}

#define EXPECT_GLOBAL_STATE(expected, map)          \
  do {                                              \
    char state[128];                                \
    read_global_state((map), state, sizeof(state)); \
    TEST_ASSERT_EQUAL_STRING((expected), state);    \
  } while (false)

// Global state needs a loaded style. It starts from the defaults the style
// declares. A set property shadows its default until it is set to null, a
// value that is not JSON fails the command and leaves the state alone, and
// loading a style again discards what the host set. A style without state
// declarations holds only what the host sets.
static void global_state_reads_back_defaults_and_set_properties(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_completion discard = mln_test_discard_completion();
  MLN_TEST_INVALID(mln_map_get_global_state(map, NULL, NULL));
  MLN_TEST_INVALID(mln_map_set_global_state_property(
    map, (mln_buffer_view){.data = NULL, .size = 1}, MLN_BUFFER_LITERAL("true"),
    &discard, NULL
  ));
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_STATE, "style JSON has not loaded",
    mln_map_set_global_state_property(
      map, MLN_BUFFER_LITERAL("theme"), MLN_BUFFER_LITERAL("true"),
      &completion.descriptor, NULL
    )
  );

  static const char stateful_style[] =
    "{\"version\":8,\"sources\":{},\"layers\":[],"
    "\"state\":{\"theme\":{\"default\":\"light\"}}}";
  mln_test_load_style_and_wait(
    runtime, map, MLN_BUFFER_LITERAL(stateful_style)
  );
  EXPECT_GLOBAL_STATE("{\"theme\":\"light\"}", map);
  set_global_state(map, "[\"dark\",{\"enabled\":true}]");
  EXPECT_GLOBAL_STATE("{\"theme\":[\"dark\",{\"enabled\":true}]}", map);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "",
    mln_map_set_global_state_property(
      map, MLN_BUFFER_LITERAL("theme"), MLN_BUFFER_LITERAL("{not json"),
      &completion.descriptor, NULL
    )
  );
  EXPECT_GLOBAL_STATE("{\"theme\":[\"dark\",{\"enabled\":true}]}", map);
  set_global_state(map, "null");
  EXPECT_GLOBAL_STATE("{\"theme\":\"light\"}", map);

  set_global_state(map, "\"dark\"");
  mln_test_load_style_and_wait(
    runtime, map, MLN_BUFFER_LITERAL(stateful_style)
  );
  EXPECT_GLOBAL_STATE("{\"theme\":\"light\"}", map);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  EXPECT_GLOBAL_STATE("{}", map);
  set_global_state(map, "true");
  EXPECT_GLOBAL_STATE("{\"theme\":true}", map);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void read_loaded_style_json(mln_map map, char* out, size_t capacity) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  MLN_TEST_OK(mln_map_loaded_style_json(map, &completion.descriptor, NULL));
  bool found = false;
  MLN_TEST_OK(mln_test_style_finish_text(&completion, out, capacity, &found));
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
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, MLN_BUFFER_LITERAL("added-later"),
    MLN_BUFFER_LITERAL(MLN_TEST_EMPTY_GEOJSON_SOURCE), &completion.descriptor,
    NULL
  ));
  read_loaded_style_json(map, document, sizeof(document));
  TEST_ASSERT_EQUAL_STRING(named_style, document);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Reads the style's transition configuration back through the ordered query.
static mln_style_transition_options read_transition_options(mln_map map) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_transition_options));
  MLN_TEST_OK(
    mln_map_get_style_transition_options(map, &completion.descriptor, NULL)
  );
  mln_style_transition_options value = {0};
  MLN_TEST_OK(
    mln_test_completion_finish_value(&completion, &value, sizeof(value))
  );
  return value;
}

static void set_transition_options(
  mln_map map, const mln_style_transition_options* options
) {
  MLN_TEST_AWAIT_OK(mln_map_set_style_transition_options(
    map, options, &completion.descriptor, NULL
  ));
}

static const uint32_t timing =
  MLN_STYLE_TRANSITION_OPTION_DURATION | MLN_STYLE_TRANSITION_OPTION_DELAY;

// A map with no style reports no duration, a style without a transition
// member reports MapLibre Native's 300 ms default, and a style whose
// transition names only a delay reports only that delay.
static void a_loaded_style_reports_the_300_ms_transition_default(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_UINT32(0, read_transition_options(map).fields & timing);

  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  mln_style_transition_options read = read_transition_options(map);
  TEST_ASSERT_BITS_HIGH(MLN_STYLE_TRANSITION_OPTION_DURATION, read.fields);
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
    MLN_STYLE_TRANSITION_OPTION_DELAY, read.fields & timing
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
  applied.fields = timing;
  applied.duration_ms = 250.0;
  applied.delay_ms = 75.0;
  set_transition_options(map, &applied);
  mln_style_transition_options read = read_transition_options(map);
  TEST_ASSERT_EQUAL_DOUBLE(250.0, read.duration_ms);
  TEST_ASSERT_EQUAL_DOUBLE(75.0, read.delay_ms);
  TEST_ASSERT_TRUE(read.enable_placement_transitions);

  // Placement transitions stay on unless a set names them.
  mln_style_transition_options placement =
    mln_style_transition_options_default();
  placement.fields = MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
  placement.enable_placement_transitions = false;
  set_transition_options(map, &placement);
  read = read_transition_options(map);
  TEST_ASSERT_BITS_HIGH(placement.fields, read.fields);
  TEST_ASSERT_FALSE(read.enable_placement_transitions);
  set_transition_options(map, &applied);

  // A null or undersized struct never reaches the map worker.
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "options must not be null",
    mln_map_set_style_transition_options(
      map, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  mln_style_transition_options rejected = applied;
  rejected.size = sizeof(mln_style_transition_options) - 1;
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "size is too small",
    mln_map_set_style_transition_options(
      map, &rejected, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );

  // Unknown bits and unusable durations are the command's rejections.
  rejected = applied;
  rejected.fields |= UINT32_C(1) << 31;
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "unknown bits",
    mln_map_set_style_transition_options(
      map, &rejected, &completion.descriptor, NULL
    )
  );
  rejected = applied;
  rejected.duration_ms = -1.0;
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "duration_ms",
    mln_map_set_style_transition_options(
      map, &rejected, &completion.descriptor, NULL
    )
  );
  rejected = applied;
  rejected.delay_ms = INFINITY;
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "delay_ms",
    mln_map_set_style_transition_options(
      map, &rejected, &completion.descriptor, NULL
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

// Expects one light property's JSON, or "" when it is undefined.
#define EXPECT_LIGHT_PROPERTY(expected, map, name)                    \
  do {                                                                \
    mln_test_completion light = mln_test_completion_buffer_view();    \
    MLN_TEST_OK(mln_map_get_style_light_property(                     \
      (map), MLN_BUFFER_LITERAL(name), &light.descriptor, NULL        \
    ));                                                               \
    char value[64];                                                   \
    MLN_TEST_OK(mln_test_style_finish_text(&light, value, 64, NULL)); \
    TEST_ASSERT_EQUAL_STRING((expected), value);                      \
  } while (false)

// The light has no ID to miss, so every bad input is an INVALID_ARGUMENT, and
// a rejected write leaves the light as it was.
static void the_style_light_round_trips_through_json(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  const mln_buffer_view intensity = MLN_BUFFER_LITERAL("intensity");

  MLN_TEST_AWAIT_OK(mln_map_set_style_light_json(
    map, MLN_BUFFER_LITERAL("{\"anchor\":\"viewport\",\"intensity\":0.5}"),
    &completion.descriptor, NULL
  ));
  EXPECT_LIGHT_PROPERTY("\"viewport\"", map, "anchor");
  EXPECT_LIGHT_PROPERTY("0.5", map, "intensity");

  MLN_TEST_AWAIT_OK(mln_map_set_style_light_property(
    map, intensity, MLN_BUFFER_LITERAL("0.75"), &completion.descriptor, NULL
  ));
  EXPECT_LIGHT_PROPERTY("0.75", map, "intensity");

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
      map, intensity, MLN_BUFFER_LITERAL("\"bright\""), &completion.descriptor,
      NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "style light",
    mln_map_set_style_light_json(
      map, MLN_BUFFER_LITERAL("{\"anchor\":7}"), &completion.descriptor, NULL
    )
  );
  EXPECT_LIGHT_PROPERTY("0.75", map, "intensity");
  EXPECT_LIGHT_PROPERTY("\"viewport\"", map, "anchor");
  // An unknown name reads as undefined rather than failing.
  EXPECT_LIGHT_PROPERTY("", map, "brightness");

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(style_commands_copy_inputs_and_fail_on_duplicate_or_missing_ids);
  RUN_TEST(missing_style_ids_report_not_found);
  RUN_TEST(global_state_reads_back_defaults_and_set_properties);
  RUN_TEST(the_loaded_style_document_reads_back_as_loaded);
  RUN_TEST(a_loaded_style_reports_the_300_ms_transition_default);
  RUN_TEST(style_transition_options_reject_unsafe_raw_input);
  RUN_TEST(the_style_light_round_trips_through_json);
}
