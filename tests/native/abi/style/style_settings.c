// Style-wide settings a host reads back: the loaded document, global state,
// and the transition options, plus the values their setters refuse.

#include <stdbool.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static const char layered_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"bg\","
  "\"type\":\"background\"}],\"state\":{\"theme\":{\"default\":\"light\"}}}";

static void read_text(
  mln_status (*start)(mln_map, const mln_completion*, mln_diagnostic*),
  mln_map map, char* out, size_t capacity
) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, start(map, &completion.descriptor, NULL)
  );
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_style_finish_text(&completion, out, capacity, &found)
  );
  TEST_ASSERT_TRUE(found);
}

// The loaded document reads back byte for byte, so a host can reload it.
static void the_loaded_style_reads_back_as_it_was_set(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map, MLN_BUFFER_LITERAL(layered_style_json)
  );
  char json[256];
  read_text(mln_map_loaded_style_json, map, json, sizeof(json));
  TEST_ASSERT_EQUAL_STRING(layered_style_json, json);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Global state starts from the style's defaults, and a property the host sets
// reads back in the state object.
static void global_state_reads_back_defaults_and_set_properties(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map, MLN_BUFFER_LITERAL(layered_style_json)
  );
  char state[128];
  read_text(mln_map_get_global_state, map, state, sizeof(state));
  TEST_ASSERT_EQUAL_STRING("{\"theme\":\"light\"}", state);

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_global_state_property(
      map, MLN_BUFFER_LITERAL("theme"), MLN_BUFFER_LITERAL("\"dark\""),
      &completion.descriptor, NULL
    )
  );
  read_text(mln_map_get_global_state, map, state, sizeof(state));
  TEST_ASSERT_EQUAL_STRING("{\"theme\":\"dark\"}", state);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// An override that turns placement transitions off reads back off, alongside
// the zero duration it set.
static void transition_options_carry_the_placement_flag(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  mln_style_transition_options options = mln_style_transition_options_default();
  options.fields = MLN_STYLE_TRANSITION_OPTION_DURATION |
                   MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
  options.duration_ms = 0.0;
  options.enable_placement_transitions = false;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_style_transition_options(
                     map, &options, &completion.descriptor, NULL
                   )
  );

  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_transition_options));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_transition_options(map, &completion.descriptor, NULL)
  );
  mln_style_transition_options read = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&completion, &read, sizeof(read))
  );
  TEST_ASSERT_TRUE(
    (read.fields & MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS) !=
    0
  );
  TEST_ASSERT_FALSE(read.enable_placement_transitions);
  TEST_ASSERT_EQUAL_DOUBLE(0.0, read.duration_ms);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A visibility outside the enum fails on the map thread, and an image source
// with other than four corners is refused at submission.
static void style_setters_refuse_out_of_range_values(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map, MLN_BUFFER_LITERAL(layered_style_json)
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "visibility is invalid",
    mln_map_set_layer_visibility(
      map, MLN_BUFFER_LITERAL("bg"), 900, &completion.descriptor, NULL
    )
  );

  const mln_lat_lng corner = {.latitude = 0.0, .longitude = 0.0};
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "must be 4", mln_map_add_image_source_url(
                   map, MLN_BUFFER_LITERAL("image"), &corner, 1,
                   MLN_BUFFER_LITERAL("custom://image.png"),
                   &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(the_loaded_style_reads_back_as_it_was_set);
  RUN_TEST(global_state_reads_back_defaults_and_set_properties);
  RUN_TEST(transition_options_carry_the_placement_flag);
  RUN_TEST(style_setters_refuse_out_of_range_values);
}
