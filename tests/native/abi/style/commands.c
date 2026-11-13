// Style command semantics shared by every style edit, and the style-wide values
// that belong to no layer.

#include <math.h>
#include <string.h>

#include "support/style.h"
#include "support/test_support.h"

static bool source_exists(mln_map map, const char* id) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_source_info));
  MLN_TEST_OK(mln_map_get_style_source(
    map, mln_buffer_view_from_cstr(id), &completion.descriptor, NULL
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
    map, mln_buffer_view_from_cstr(id), &completion.descriptor, NULL
  ));
  mln_style_image_info result;
  bool found = false;
  MLN_TEST_OK(mln_test_completion_finish_optional(
    &completion, &result, sizeof(result), &found
  ));
  return found;
}

static void style_commands_copy_inputs_and_fail_on_duplicate_or_missing_ids(
  void
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_styled_map(runtime);
  char id[] = "owned-source";
  char json[sizeof(MLN_TEST_EMPTY_GEOJSON_SOURCE)];
  memcpy(json, MLN_TEST_EMPTY_GEOJSON_SOURCE, sizeof(json));
  mln_test_completion add = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_add_style_source_json(
    map, mln_buffer_view_from_cstr(id), mln_buffer_view_from_cstr(json),
    &add.descriptor, NULL
  ));
  // The command copied both inputs before it returned.
  memset(id, 'x', strlen(id));
  memset(json, ' ', strlen(json));
  MLN_TEST_OK(mln_test_completion_settle(&add));
  TEST_ASSERT_TRUE(source_exists(map, "owned-source"));

  const mln_buffer_view source = MLN_BUFFER_LITERAL("owned-source");
  const mln_buffer_view empty_source =
    MLN_BUFFER_LITERAL(MLN_TEST_EMPTY_GEOJSON_SOURCE);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_ALREADY_EXISTS, "source \"owned-source\" already exists",
    mln_map_add_style_source_json(
      map, source, empty_source, &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_OK(
    mln_map_remove_style_source(map, source, &completion.descriptor, NULL)
  );
  TEST_ASSERT_FALSE(source_exists(map, "owned-source"));

  const uint8_t pixel[4] = {0, 0, 0, 0};
  mln_premultiplied_rgba8_image image = {
    .size = sizeof(mln_premultiplied_rgba8_image)
  };
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
    MLN_STATUS_NOT_FOUND, "style image \"doomed-image\" does not exist",
    mln_map_remove_style_image(map, doomed, &completion.descriptor, NULL)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void missing_style_ids_report_not_found(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_styled_map(runtime);
  const mln_buffer_view layer = MLN_BUFFER_LITERAL("absent-layer");
  const mln_buffer_view source = MLN_BUFFER_LITERAL("absent-source");
  const mln_buffer_view none = MLN_BUFFER_LITERAL("");
  const char* missing_layer = "layer \"absent-layer\" does not exist";
  const char* missing_source = "source \"absent-source\" does not exist";

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_layer,
    mln_map_remove_style_layer(map, layer, &completion.descriptor, NULL)
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_layer,
    mln_map_move_style_layer(map, layer, none, &completion.descriptor, NULL)
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_layer,
    mln_map_set_style_layer_property(
      map, layer, MLN_BUFFER_LITERAL("circle-radius"), MLN_BUFFER_LITERAL("4"),
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "before_layer_id \"absent-layer\" does not exist",
    mln_map_add_style_layer_json(
      map,
      MLN_BUFFER_LITERAL(
        "{\"id\":\"backdrop\",\"type\":\"background\"}"
      ),
      layer, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND,
    "layer \"hillshade\" uses source \"absent-source\", which does not exist",
    mln_map_add_style_layer_json(
      map,
      MLN_BUFFER_LITERAL(
        "{\"id\":\"hillshade\",\"type\":\"hillshade\",\"source\":\"absent-"
        "source\"}"
      ),
      none, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_source,
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
    mln_map_set_geo_json_source_url(
      map, source, MLN_BUFFER_LITERAL("fixture://points.geojson"),
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_source,
    mln_map_set_geo_json_source_synchronous_tiling(
      map, source, true, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_source,
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
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, missing_source,
    mln_map_invalidate_custom_vector_source_region(
      map, source, (mln_lat_lng_bounds){{-1, -1}, {1, 1}},
      &completion.descriptor, NULL
    )
  );

  // A query of one attribute of a missing layer completes with
  // MLN_STATUS_NOT_FOUND too.
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_NOT_FOUND, mln_map_get_style_layer_property(
                            map, layer, MLN_BUFFER_LITERAL("circle-radius"),
                            &completion.descriptor, NULL
                          )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void read_global_state(mln_map map, char* out, size_t capacity) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  MLN_TEST_OK(mln_map_get_global_state(map, &completion.descriptor, NULL));
  bool found = false;
  MLN_TEST_OK(mln_test_style_finish_text(&completion, out, capacity, &found));
  TEST_ASSERT_TRUE(found);
}

static void set_global_state(mln_map map, const char* value) {
  MLN_TEST_AWAIT_OK(mln_map_set_global_state_property(
    map, MLN_BUFFER_LITERAL("theme"), mln_buffer_view_from_cstr(value),
    &completion.descriptor, NULL
  ));
}

#define EXPECT_GLOBAL_STATE(expected, map)          \
  do {                                              \
    char state[128];                                \
    read_global_state((map), state, sizeof(state)); \
    TEST_ASSERT_EQUAL_STRING((expected), state);    \
  } while (false)

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
    MLN_STATUS_INVALID_STATE, "style is not loaded",
    mln_map_set_global_state_property(
      map, MLN_BUFFER_LITERAL("theme"), MLN_BUFFER_LITERAL("true"),
      &completion.descriptor, NULL
    )
  );

  static const char stateful_style[] =
    "{\"version\":8,\"sources\":{},\"layers\":[],"
    "\"state\":{\"theme\":{\"default\":\"light\"}}}";
  mln_test_load_style_and_wait(map, MLN_BUFFER_LITERAL(stateful_style));
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
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void read_loaded_style_json(mln_map map, char* out, size_t capacity) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  MLN_TEST_OK(mln_map_get_loaded_style_json(map, &completion.descriptor, NULL));
  bool found = false;
  MLN_TEST_OK(mln_test_style_finish_text(&completion, out, capacity, &found));
  TEST_ASSERT_TRUE(found);
}

static void the_loaded_style_document_reads_back_as_loaded(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  char document[512];
  read_loaded_style_json(map, document, sizeof(document));
  TEST_ASSERT_EQUAL_STRING("", document);

  static const char named_style[] =
    "{\"version\":8,\"name\":\"loaded\",\"sources\":{},\"layers\":[]}";
  mln_test_load_style_and_wait(map, MLN_BUFFER_LITERAL(named_style));
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

// MapLibre picks the default duration, so the case checks only which fields
// the read reports.
static void transition_options_report_the_fields_the_style_sets(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_UINT32(0, read_transition_options(map).fields & timing);

  mln_test_load_style_and_wait(
    map, MLN_BUFFER_LITERAL(
           "{\"version\":8,\"transition\":{\"delay\":50},\"sources\":{},"
           "\"layers\":[]}"
         )
  );
  const mln_style_transition_options read = read_transition_options(map);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_TRANSITION_OPTION_DELAY, read.fields & timing
  );
  TEST_ASSERT_EQUAL_DOUBLE(50.0, read.delay_ms);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Raw callers can hand the transition setter a struct no binding would build.
static void style_transition_options_reject_unsafe_raw_input(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_styled_map(runtime);

  mln_style_transition_options applied = {
    .size = sizeof(mln_style_transition_options)
  };
  applied.fields = timing;
  applied.duration_ms = 250.0;
  applied.delay_ms = 75.0;
  set_transition_options(map, &applied);
  mln_style_transition_options read = read_transition_options(map);
  TEST_ASSERT_EQUAL_DOUBLE(250.0, read.duration_ms);
  TEST_ASSERT_EQUAL_DOUBLE(75.0, read.delay_ms);
  TEST_ASSERT_TRUE(read.enable_placement_transitions);

  mln_style_transition_options placement = {
    .size = sizeof(mln_style_transition_options)
  };
  placement.fields = MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
  placement.enable_placement_transitions = false;
  set_transition_options(map, &placement);
  read = read_transition_options(map);
  TEST_ASSERT_BITS_HIGH(placement.fields, read.fields);
  TEST_ASSERT_FALSE(read.enable_placement_transitions);
  set_transition_options(map, &applied);

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

// The light has no ID to miss, so every bad input is an INVALID_ARGUMENT.
static void the_style_light_round_trips_through_json(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_empty_style_json);
  const mln_buffer_view intensity = MLN_BUFFER_LITERAL("intensity");

  MLN_TEST_AWAIT_OK(mln_map_set_style_root_property(
    map, MLN_BUFFER_LITERAL("light"),
    MLN_BUFFER_LITERAL("{\"anchor\":\"viewport\",\"intensity\":0.5}"),
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
      map, intensity, MLN_BUFFER_LITERAL("\"bright\""), &completion.descriptor,
      NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "style root property",
    mln_map_set_style_root_property(
      map, MLN_BUFFER_LITERAL("light"), MLN_BUFFER_LITERAL("{\"anchor\":7}"),
      &completion.descriptor, NULL
    )
  );
  EXPECT_LIGHT_PROPERTY("0.75", map, "intensity");
  EXPECT_LIGHT_PROPERTY("\"viewport\"", map, "anchor");
  EXPECT_LIGHT_PROPERTY("", map, "brightness");

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Expects one root property's JSON, or "" when the style doesn't set it.
#define EXPECT_ROOT_PROPERTY(expected, map, name)                     \
  do {                                                                \
    mln_test_completion root = mln_test_completion_buffer_view();     \
    MLN_TEST_OK(mln_map_get_style_root_property(                      \
      (map), MLN_BUFFER_LITERAL(name), &root.descriptor, NULL         \
    ));                                                               \
    char value[128];                                                  \
    MLN_TEST_OK(mln_test_style_finish_text(&root, value, 128, NULL)); \
    TEST_ASSERT_EQUAL_STRING((expected), value);                      \
  } while (false)

static void root_properties_change_the_light_and_the_transition(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_empty_style_json);
  EXPECT_ROOT_PROPERTY("", map, "light");
  // MapLibre gives a style without a transition its default duration.
  EXPECT_ROOT_PROPERTY("{\"duration\":300}", map, "transition");

  MLN_TEST_AWAIT_OK(mln_map_set_style_root_property(
    map, MLN_BUFFER_LITERAL("light"),
    MLN_BUFFER_LITERAL("{\"intensity\":0.5,\"anchor\":\"viewport\"}"),
    &completion.descriptor, NULL
  ));
  EXPECT_ROOT_PROPERTY(
    "{\"anchor\":\"viewport\",\"intensity\":0.5}", map, "light"
  );
  EXPECT_LIGHT_PROPERTY("0.5", map, "intensity");

  MLN_TEST_AWAIT_OK(mln_map_set_style_root_property(
    map, MLN_BUFFER_LITERAL("transition"),
    MLN_BUFFER_LITERAL("{\"delay\":50,\"duration\":300}"),
    &completion.descriptor, NULL
  ));
  EXPECT_ROOT_PROPERTY("{\"duration\":300,\"delay\":50}", map, "transition");
  const mln_style_transition_options transition = read_transition_options(map);
  TEST_ASSERT_EQUAL_DOUBLE(300.0, transition.duration_ms);
  TEST_ASSERT_EQUAL_DOUBLE(50.0, transition.delay_ms);
  TEST_ASSERT_TRUE(transition.enable_placement_transitions);

  // A fractional millisecond from the typed setter keeps its fraction.
  mln_style_transition_options fine = {
    .size = sizeof(mln_style_transition_options)
  };
  fine.fields = MLN_STYLE_TRANSITION_OPTION_DURATION;
  fine.duration_ms = 0.5;
  set_transition_options(map, &fine);
  EXPECT_ROOT_PROPERTY("{\"duration\":0.5}", map, "transition");

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "style root property \"light\"",
    mln_map_set_style_root_property(
      map, MLN_BUFFER_LITERAL("light"), MLN_BUFFER_LITERAL("{\"anchor\":7}"),
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "not valid JSON",
    mln_map_set_style_root_property(
      map, MLN_BUFFER_LITERAL("transition"), MLN_BUFFER_LITERAL("{"),
      &completion.descriptor, NULL
    )
  );
  EXPECT_ROOT_PROPERTY(
    "{\"anchor\":\"viewport\",\"intensity\":0.5}", map, "light"
  );

  MLN_TEST_AWAIT_OK(mln_map_set_style_root_property(
    map, MLN_BUFFER_LITERAL("light"), MLN_BUFFER_LITERAL("null"),
    &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_style_root_property(
    map, MLN_BUFFER_LITERAL("transition"), MLN_BUFFER_LITERAL("null"),
    &completion.descriptor, NULL
  ));
  EXPECT_ROOT_PROPERTY("", map, "light");
  EXPECT_ROOT_PROPERTY("{\"duration\":300}", map, "transition");

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The library reads the other root properties from the style as it loaded,
// and can't change them after.
static void other_root_properties_read_back_as_loaded_and_reject_changes(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    map, MLN_BUFFER_LITERAL(
           "{\"version\":8,\"name\":\"plain\",\"sources\":{},\"layers\":[],"
           "\"projection\":{\"type\":\"globe\"}}"
         )
  );
  EXPECT_ROOT_PROPERTY("8", map, "version");
  EXPECT_ROOT_PROPERTY("\"plain\"", map, "name");
  EXPECT_ROOT_PROPERTY("{\"type\":\"globe\"}", map, "projection");
  EXPECT_ROOT_PROPERTY("", map, "sky");

  static const struct {
    const char* name;
    const char* value;
  } unsupported[] = {
    {"projection", "{\"type\":\"globe\"}"},
    {"sky", "{\"sky-color\":\"#88c6fc\"}"},
    {"terrain", "{\"source\":\"dem\"}"},
    {"sprite", "\"sprite\""},
    {"font-faces", "{}"},
  };
  for (size_t index = 0; index < sizeof(unsupported) / sizeof(unsupported[0]);
       index += 1) {
    const mln_buffer_view name = {
      .data = (const uint8_t*)unsupported[index].name,
      .size = strlen(unsupported[index].name),
    };
    const mln_buffer_view value = {
      .data = (const uint8_t*)unsupported[index].value,
      .size = strlen(unsupported[index].value),
    };
    MLN_TEST_EXPECT_COMMAND_FAILED(
      MLN_STATUS_UNSUPPORTED, unsupported[index].name,
      mln_map_set_style_root_property(
        map, name, value, &completion.descriptor, NULL
      )
    );
  }
  EXPECT_ROOT_PROPERTY("{\"type\":\"globe\"}", map, "projection");

  static const char* const rejected[] = {"layers", "sources", "Light", "fog"};
  for (size_t index = 0; index < sizeof(rejected) / sizeof(rejected[0]);
       index += 1) {
    const mln_buffer_view name = {
      .data = (const uint8_t*)rejected[index],
      .size = strlen(rejected[index]),
    };
    MLN_TEST_EXPECT_COMMAND_FAILED(
      MLN_STATUS_INVALID_ARGUMENT, "not a style root property",
      mln_map_set_style_root_property(
        map, name, MLN_BUFFER_LITERAL("{}"), &completion.descriptor, NULL
      )
    );
    mln_test_completion root = mln_test_completion_buffer_view();
    MLN_TEST_OK(
      mln_map_get_style_root_property(map, name, &root.descriptor, NULL)
    );
    char value[8];
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_INVALID_ARGUMENT,
      mln_test_style_finish_text(&root, value, sizeof(value), NULL)
    );
  }

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(style_commands_copy_inputs_and_fail_on_duplicate_or_missing_ids);
  RUN_TEST(missing_style_ids_report_not_found);
  RUN_TEST(global_state_reads_back_defaults_and_set_properties);
  RUN_TEST(the_loaded_style_document_reads_back_as_loaded);
  RUN_TEST(transition_options_report_the_fields_the_style_sets);
  RUN_TEST(style_transition_options_reject_unsafe_raw_input);
  RUN_TEST(the_style_light_round_trips_through_json);
  RUN_TEST(root_properties_change_the_light_and_the_transition);
  RUN_TEST(other_root_properties_read_back_as_loaded_and_reject_changes);
}
