// Standalone style expressions: an expression parses into a handle with no
// map, evaluates synchronously against the inputs a caller passes, on any
// thread, and writes its result as JSON into a buffer the caller sizes.

#include <math.h>

#include "support/test_support.h"

static mln_style_expression create_typed(
  const char* json, const mln_style_expression_options* options
) {
  mln_style_expression expression = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_style_expression_create(
    mln_buffer_view_from_cstr(json), options, &expression, MLN_TEST_DIAGNOSTIC
  ));
  return expression;
}

static mln_style_expression create(const char* json) {
  return create_typed(json, NULL);
}

static mln_status create_status(
  const char* json, const mln_style_expression_options* options
) {
  mln_style_expression expression = MLN_HANDLE_NULL;
  const mln_status status = mln_style_expression_create(
    mln_buffer_view_from_cstr(json), options, &expression, MLN_TEST_DIAGNOSTIC
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, expression);
  return status;
}

static void expect_result(
  mln_style_expression expression, const mln_style_expression_inputs* inputs,
  const char* expected
) {
  char json[512];
  size_t length = 0;
  MLN_TEST_OK(mln_style_expression_evaluate(
    expression, inputs, json, sizeof(json), &length, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_EQUAL_STRING(expected, json);
  TEST_ASSERT_EQUAL_size_t(strlen(expected), length);
}

static void expect_evaluates(
  const char* json, const mln_style_expression_inputs* inputs,
  const char* expected
) {
  const mln_style_expression expression = create(json);
  expect_result(expression, inputs, expected);
  mln_style_expression_release(expression);
}

static mln_status evaluate_status(
  mln_style_expression expression, const mln_style_expression_inputs* inputs
) {
  char json[64];
  size_t length = 0;
  return mln_style_expression_evaluate(
    expression, inputs, json, sizeof(json), &length, MLN_TEST_DIAGNOSTIC
  );
}

static mln_style_expression_inputs feature_inputs(const char* feature) {
  mln_style_expression_inputs inputs = mln_style_expression_inputs_default();
  inputs.fields = MLN_STYLE_EXPRESSION_INPUT_FEATURE;
  inputs.feature = mln_buffer_view_from_cstr(feature);
  return inputs;
}

// An image serializes as an object whose member order the C API leaves open.
static void expect_image(
  const mln_style_expression_inputs* inputs, const char* id,
  const char* availability
) {
  char json[128];
  snprintf(json, sizeof(json), "[\"image\", \"%s\"]", id);
  const mln_style_expression expression = create(json);
  size_t length = 0;
  MLN_TEST_OK(mln_style_expression_evaluate(
    expression, inputs, json, sizeof(json), &length, MLN_TEST_DIAGNOSTIC
  ));
  mln_style_expression_release(expression);
  char name[64];
  snprintf(name, sizeof(name), "\"name\":\"%s\"", id);
  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(json, name), json);
  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(json, availability), json);
  TEST_ASSERT_EQUAL_size_t(strlen(name) + strlen(availability) + 3, length);
}

static void constants_evaluate_without_inputs(void) {
  expect_evaluates("[\"+\", 1, 2]", NULL, "3.0");
  expect_evaluates("\"text\"", NULL, "\"text\"");
  expect_evaluates(
    "[\"literal\", {\"a\": [1, true]}]", NULL, "{\"a\":[1.0,true]}"
  );
  expect_evaluates("null", NULL, "null");
}

static void each_input_reaches_the_expression_that_reads_it(void) {
  mln_style_expression_inputs inputs = mln_style_expression_inputs_default();
  inputs.fields = MLN_STYLE_EXPRESSION_INPUT_ZOOM;
  inputs.zoom = 5.0;
  expect_evaluates(
    "[\"interpolate\", [\"linear\"], [\"zoom\"], 0, 0, 10, 100]", &inputs,
    "50.0"
  );

  inputs = mln_style_expression_inputs_default();
  inputs.fields = MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER;
  inputs.color_ramp_parameter = 0.25;
  expect_evaluates("[\"heatmap-density\"]", &inputs, "0.25");
  expect_evaluates("[\"line-progress\"]", &inputs, "0.25");

  inputs = mln_style_expression_inputs_default();
  inputs.fields = MLN_STYLE_EXPRESSION_INPUT_ELEVATION;
  inputs.elevation = 120.0;
  expect_evaluates("[\"elevation\"]", &inputs, "120.0");

  inputs = feature_inputs(
    "{\"type\":\"Feature\",\"id\":7,\"geometry\":{\"type\":\"Point\","
    "\"coordinates\":[1,2]},\"properties\":{\"name\":\"park\"}}"
  );
  expect_evaluates("[\"get\", \"name\"]", &inputs, "\"park\"");
  expect_evaluates("[\"id\"]", &inputs, "7.0");
  expect_evaluates("[\"geometry-type\"]", &inputs, "\"Point\"");

  static const char state[] = "{\"hover\":true}";
  inputs = mln_style_expression_inputs_default();
  inputs.fields = MLN_STYLE_EXPRESSION_INPUT_FEATURE_STATE;
  inputs.feature_state = MLN_BUFFER_LITERAL(state);
  expect_evaluates("[\"feature-state\", \"hover\"]", &inputs, "true");

  static const char global[] = "{\"theme\":\"dark\"}";
  inputs = mln_style_expression_inputs_default();
  inputs.fields = MLN_STYLE_EXPRESSION_INPUT_GLOBAL_STATE;
  inputs.global_state = MLN_BUFFER_LITERAL(global);
  expect_evaluates("[\"global-state\", \"theme\"]", &inputs, "\"dark\"");

  const mln_buffer_view images[] = {MLN_BUFFER_LITERAL("monument-15")};
  inputs = mln_style_expression_inputs_default();
  inputs.available_images = images;
  inputs.available_image_count = 1;
  expect_image(&inputs, "monument-15", "\"available\":true");
  expect_image(&inputs, "missing", "\"available\":false");
}

// The within expression places the feature in the tile that the inputs name,
// as the style-spec fixture within/point-within-polygon does.
static void within_reads_the_canonical_tile(void) {
  const mln_style_expression expression = create(
    "[\"within\", {\"type\": \"Polygon\", \"coordinates\": "
    "[[[0, 0], [0, 5], [5, 5], [5, 0], [0, 0]]]}]"
  );
  mln_style_expression_inputs inside = feature_inputs(
    "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
    "\"coordinates\":[2,2]},\"properties\":{}}"
  );
  inside.fields |= MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID;
  inside.canonical_tile_id = (mln_canonical_tile_id){.z = 3, .x = 3, .y = 3};
  expect_result(expression, &inside, "true");

  mln_style_expression_inputs outside = feature_inputs(
    "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
    "\"coordinates\":[6,6]},\"properties\":{}}"
  );
  outside.fields |= MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID;
  outside.canonical_tile_id = inside.canonical_tile_id;
  expect_result(expression, &outside, "false");
  mln_style_expression_release(expression);
}

static void values_serialize_as_style_json(void) {
  expect_evaluates(
    "[\"to-color\", \"red\"]", NULL, "[\"rgba\",255.0,0.0,0.0,1.0]"
  );
  expect_evaluates(
    "[\"format\", \"a\", {\"font-scale\": 2}]", NULL,
    "[\"format\",\"a\",{\"font-scale\":2.0}]"
  );
  // JSON has no NaN or infinity, so they become null, as JSON.stringify()
  // writes them.
  expect_evaluates("[\"/\", 0, 0]", NULL, "null");
  expect_evaluates("[\"literal\", [1, 2]]", NULL, "[1.0,2.0]");
  expect_evaluates("[\"-\", [\"ln\", 0]]", NULL, "null");
}

static void an_expected_type_coerces_strings_and_asserts_the_rest(void) {
  mln_style_expression_options options = mln_style_expression_options_default();
  options.fields = MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE;
  options.expected_type.kind = MLN_STYLE_EXPRESSION_TYPE_STRING;
  mln_style_expression expression = create_typed("[\"get\", \"n\"]", &options);
  mln_style_expression_inputs inputs = feature_inputs(
    "{\"type\":\"Feature\",\"geometry\":null,\"properties\":{\"n\":5}}"
  );
  expect_result(expression, &inputs, "\"5\"");
  mln_style_expression_release(expression);

  options.expected_type.kind = MLN_STYLE_EXPRESSION_TYPE_NUMBER;
  expression = create_typed("[\"get\", \"name\"]", &options);
  inputs = feature_inputs(
    "{\"type\":\"Feature\",\"geometry\":null,\"properties\":"
    "{\"name\":\"park\"}}"
  );
  MLN_TEST_INVALID(evaluate_status(expression, &inputs));
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "expression: "));
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "number"));
  mln_style_expression_release(expression);

  // A color type converts a string literal, as a color layer property does.
  options.expected_type.kind = MLN_STYLE_EXPRESSION_TYPE_COLOR;
  expect_result(
    expression = create_typed("\"#00ff00\"", &options), NULL,
    "[\"rgba\",0.0,255.0,0.0,1.0]"
  );
  mln_style_expression_release(expression);

  options.expected_type = (mln_style_expression_type){
    .kind = MLN_STYLE_EXPRESSION_TYPE_ARRAY,
    .item_kind = MLN_STYLE_EXPRESSION_TYPE_NUMBER,
    .item_count = 2,
  };
  expression = create_typed("[\"literal\", [1, 2]]", &options);
  expect_result(expression, NULL, "[1.0,2.0]");
  mln_style_expression_release(expression);
  MLN_TEST_INVALID(create_status("[\"literal\", [1, 2, 3]]", &options));
  MLN_TEST_INVALID(create_status("[\"literal\", [\"a\", \"b\"]]", &options));
}

static void parse_errors_name_their_location(void) {
  MLN_TEST_INVALID(create_status("[\"no-such-operator\"]", NULL));
  TEST_ASSERT_EQUAL_STRING(
    "expression: [0]: Unknown expression \"no-such-operator\". If you wanted "
    "a literal array, use [\"literal\", [...]].",
    mln_test_last_error()
  );
  MLN_TEST_INVALID(create_status("[\"+\", 1, \"a\"]", NULL));
  MLN_TEST_INVALID(create_status("{\"a\": 1}", NULL));
  MLN_TEST_INVALID(create_status("[", NULL));
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "not valid JSON"));

  // A collator has no JSON form.
  MLN_TEST_INVALID(create_status("[\"collator\", {}]", NULL));
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "collator"));
}

static void creation_rejects_invalid_arguments(void) {
  mln_style_expression expression = MLN_HANDLE_NULL;
  MLN_TEST_INVALID(mln_style_expression_create(
    MLN_BUFFER_LITERAL("1"), NULL, NULL, MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_INVALID(mln_style_expression_create(
    (mln_buffer_view){.data = NULL, .size = 0}, NULL, &expression,
    MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, expression);
  expression = create("1");
  mln_style_expression occupied = expression;
  MLN_TEST_INVALID(mln_style_expression_create(
    MLN_BUFFER_LITERAL("1"), NULL, &occupied, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_EQUAL_UINT64(expression, occupied);
  mln_style_expression_release(expression);

  mln_style_expression_options options = mln_style_expression_options_default();
  options.size = 4;
  MLN_TEST_INVALID(create_status("1", &options));
  options = mln_style_expression_options_default();
  options.fields = 1U << 7U;
  MLN_TEST_INVALID(create_status("1", &options));
  options = mln_style_expression_options_default();
  options.fields = MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE;
  options.expected_type.kind = 99;
  MLN_TEST_INVALID(create_status("1", &options));
  options.expected_type = (mln_style_expression_type){
    .kind = MLN_STYLE_EXPRESSION_TYPE_ARRAY,
    .item_kind = MLN_STYLE_EXPRESSION_TYPE_ARRAY,
  };
  MLN_TEST_INVALID(create_status("[\"literal\", [[1]]]", &options));
  options.expected_type.item_kind = 99;
  MLN_TEST_INVALID(create_status("[\"literal\", [1]]", &options));
}

static void info_reports_zoom_and_feature_dependence(void) {
  const struct {
    const char* json;
    bool feature_constant;
    bool zoom_constant;
  } cases[] = {
    {"1", true, true},
    {"[\"get\", \"x\"]", false, true},
    {"[\"feature-state\", \"x\"]", false, true},
    {"[\"interpolate\", [\"linear\"], [\"zoom\"], 0, 0, 1, 1]", true, false},
    {"[\"step\", [\"zoom\"], [\"get\", \"x\"], 5, 1]", false, false},
  };
  for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
    const mln_style_expression expression = create(cases[index].json);
    mln_style_expression_info info = {.size = sizeof(info)};
    MLN_TEST_OK(mln_style_expression_get_info(expression, &info, NULL));
    TEST_ASSERT_EQUAL_MESSAGE(
      cases[index].feature_constant, info.is_feature_constant, cases[index].json
    );
    TEST_ASSERT_EQUAL_MESSAGE(
      cases[index].zoom_constant, info.is_zoom_constant, cases[index].json
    );
    mln_style_expression_release(expression);
  }

  const mln_style_expression expression = create("1");
  MLN_TEST_INVALID(
    mln_style_expression_get_info(expression, NULL, MLN_TEST_DIAGNOSTIC)
  );
  mln_style_expression_info undersized = {.size = 4};
  MLN_TEST_INVALID(
    mln_style_expression_get_info(expression, &undersized, MLN_TEST_DIAGNOSTIC)
  );
  mln_style_expression_release(expression);
}

// A buffer too small for the result stays unchanged, and the length tells a
// caller how much to allocate.
static void a_short_buffer_reports_the_length_it_needs(void) {
  const mln_style_expression expression = create("\"abcdef\"");
  size_t length = 0;
  MLN_TEST_OK(mln_style_expression_evaluate(
    expression, NULL, NULL, 0, &length, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_EQUAL_size_t(8, length);

  char exact[8];
  memset(exact, 'x', sizeof(exact));
  length = 0;
  MLN_TEST_OK(mln_style_expression_evaluate(
    expression, NULL, exact, sizeof(exact), &length, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_EQUAL_size_t(8, length);
  TEST_ASSERT_EACH_EQUAL_CHAR('x', exact, sizeof(exact));

  char fits[9];
  MLN_TEST_OK(mln_style_expression_evaluate(
    expression, NULL, fits, sizeof(fits), &length, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_EQUAL_STRING("\"abcdef\"", fits);

  MLN_TEST_INVALID(mln_style_expression_evaluate(
    expression, NULL, NULL, 1, &length, MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_INVALID(mln_style_expression_evaluate(
    expression, NULL, fits, sizeof(fits), NULL, MLN_TEST_DIAGNOSTIC
  ));
  mln_style_expression_release(expression);
}

static void absent_inputs_fail_the_expressions_that_need_them(void) {
  const char* readers[] = {
    "[\"step\", [\"zoom\"], 0, 1, 1]",
    "[\"heatmap-density\"]",
    "[\"get\", \"x\"]",
  };
  for (size_t index = 0; index < sizeof(readers) / sizeof(readers[0]);
       ++index) {
    const mln_style_expression expression = create(readers[index]);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT, evaluate_status(expression, NULL),
      readers[index]
    );
    TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "unavailable"));
    mln_style_expression_release(expression);
  }

  // Absent state reads as null.
  expect_evaluates("[\"feature-state\", \"x\"]", NULL, "null");
  expect_evaluates("[\"global-state\", \"x\"]", NULL, "null");
}

static void evaluation_rejects_invalid_inputs(void) {
  const mln_style_expression expression = create("1");
  mln_style_expression_inputs inputs = mln_style_expression_inputs_default();
  inputs.size = 4;
  MLN_TEST_INVALID(evaluate_status(expression, &inputs));

  inputs = mln_style_expression_inputs_default();
  inputs.fields = 1U << 20U;
  MLN_TEST_INVALID(evaluate_status(expression, &inputs));

  const uint32_t numbers[] = {
    MLN_STYLE_EXPRESSION_INPUT_ZOOM,
    MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER,
    MLN_STYLE_EXPRESSION_INPUT_ELEVATION,
  };
  for (size_t index = 0; index < sizeof(numbers) / sizeof(numbers[0]);
       ++index) {
    inputs = mln_style_expression_inputs_default();
    inputs.fields = numbers[index];
    inputs.zoom = NAN;
    inputs.color_ramp_parameter = INFINITY;
    inputs.elevation = -INFINITY;
    MLN_TEST_INVALID(evaluate_status(expression, &inputs));
    TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "must be finite"));
  }

  inputs = feature_inputs("{\"type\":\"Point\",\"coordinates\":[0,0]}");
  MLN_TEST_INVALID(evaluate_status(expression, &inputs));
  inputs = feature_inputs("{");
  MLN_TEST_INVALID(evaluate_status(expression, &inputs));

  static const char array[] = "[1]";
  inputs = mln_style_expression_inputs_default();
  inputs.fields = MLN_STYLE_EXPRESSION_INPUT_FEATURE_STATE;
  inputs.feature_state = MLN_BUFFER_LITERAL(array);
  MLN_TEST_INVALID(evaluate_status(expression, &inputs));
  TEST_ASSERT_NOT_NULL(
    strstr(mln_test_last_error(), "feature_state must contain a JSON object")
  );
  inputs.fields = MLN_STYLE_EXPRESSION_INPUT_GLOBAL_STATE;
  inputs.global_state = MLN_BUFFER_LITERAL(array);
  MLN_TEST_INVALID(evaluate_status(expression, &inputs));
  inputs.global_state = (mln_buffer_view){.data = NULL, .size = 0};
  MLN_TEST_INVALID(evaluate_status(expression, &inputs));

  inputs = mln_style_expression_inputs_default();
  inputs.fields = MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID;
  inputs.canonical_tile_id = (mln_canonical_tile_id){.z = 1, .x = 2, .y = 0};
  MLN_TEST_INVALID(evaluate_status(expression, &inputs));

  inputs = mln_style_expression_inputs_default();
  inputs.available_image_count = 1;
  MLN_TEST_INVALID(evaluate_status(expression, &inputs));
  const mln_buffer_view empty_id = {.data = NULL, .size = 0};
  inputs.available_images = &empty_id;
  MLN_TEST_INVALID(evaluate_status(expression, &inputs));

  mln_style_expression_release(expression);
}

static void a_released_expression_is_stale(void) {
  const mln_style_expression expression = create("1");
  mln_style_expression_release(expression);
  mln_style_expression_release(expression);
  mln_style_expression_release(MLN_HANDLE_NULL);

  MLN_TEST_INVALID_STATE(evaluate_status(expression, NULL));
  mln_style_expression_info info = {.size = sizeof(info)};
  MLN_TEST_INVALID_STATE(
    mln_style_expression_get_info(expression, &info, NULL)
  );
  MLN_TEST_INVALID(evaluate_status(MLN_HANDLE_NULL, NULL));
}

typedef struct {
  mln_style_expression expression;
  mln_status status;
  char json[32];
} expression_thread_probe;

static void evaluate_on_foreign_thread(void* argument) {
  expression_thread_probe* probe = argument;
  size_t length = 0;
  probe->status = mln_style_expression_evaluate(
    probe->expression, NULL, probe->json, sizeof(probe->json), &length, NULL
  );
  mln_style_expression_release(probe->expression);
}

static void an_expression_evaluates_and_releases_on_another_thread(void) {
  expression_thread_probe probe = {
    .expression = create("[\"concat\", \"a\", \"b\"]"),
    .status = MLN_STATUS_INVALID_STATE,
  };
  mln_test_thread* thread =
    mln_test_thread_start(evaluate_on_foreign_thread, &probe);
  mln_test_thread_join(thread);
  MLN_TEST_OK(probe.status);
  TEST_ASSERT_EQUAL_STRING("\"ab\"", probe.json);
  MLN_TEST_INVALID_STATE(evaluate_status(probe.expression, NULL));
}

MLN_TEST_GROUP {
  RUN_TEST(constants_evaluate_without_inputs);
  RUN_TEST(each_input_reaches_the_expression_that_reads_it);
  RUN_TEST(within_reads_the_canonical_tile);
  RUN_TEST(values_serialize_as_style_json);
  RUN_TEST(an_expected_type_coerces_strings_and_asserts_the_rest);
  RUN_TEST(parse_errors_name_their_location);
  RUN_TEST(creation_rejects_invalid_arguments);
  RUN_TEST(info_reports_zoom_and_feature_dependence);
  RUN_TEST(a_short_buffer_reports_the_length_it_needs);
  RUN_TEST(absent_inputs_fail_the_expressions_that_need_them);
  RUN_TEST(evaluation_rejects_invalid_inputs);
  RUN_TEST(a_released_expression_is_stale);
  RUN_TEST(an_expression_evaluates_and_releases_on_another_thread);
}
