// Global state: the style's defaults, a property set over them, JSON null
// restoring the default, and a replaced style dropping what the old one set.

#include <stdbool.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static void expect_global_state(mln_map map, const char* expected) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_global_state(map, &completion.descriptor, NULL)
  );
  char state[128];
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_style_finish_text(&completion, state, sizeof(state), NULL)
  );
  TEST_ASSERT_EQUAL_STRING(expected, state);
}

static void set_theme(mln_map map, const char* value) {
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_global_state_property(
                     map, MLN_BUFFER_LITERAL("theme"), mln_test_view_of(value),
                     &completion.descriptor, NULL
                   )
  );
}

static void global_state_layers_properties_over_style_defaults(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map,
    MLN_BUFFER_LITERAL(
      "{\"version\":8,\"sources\":{},\"layers\":[],"
      "\"state\":{\"theme\":{\"default\":\"light\"}}}"
    )
  );
  expect_global_state(map, "{\"theme\":\"light\"}");
  set_theme(map, "[\"dark\",{\"enabled\":true}]");
  expect_global_state(map, "{\"theme\":[\"dark\",{\"enabled\":true}]}");
  set_theme(map, "null");
  expect_global_state(map, "{\"theme\":\"light\"}");

  // A style without state declarations holds only what the host sets.
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  expect_global_state(map, "{}");
  set_theme(map, "true");
  expect_global_state(map, "{\"theme\":true}");
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP { RUN_TEST(global_state_layers_properties_over_style_defaults); }
