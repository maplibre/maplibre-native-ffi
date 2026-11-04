// Loading a style: when a style load completes, and how style edits wait for
// the style to load.
//
// The style URLs are served by a scripted provider, which holds a response
// until the case answers it, so a case controls when a load ends.

#include "support/resources.h"
#include "support/style.h"
#include "support/test_support.h"

#define SOURCE_STYLE(id)                                                  \
  "{\"version\":8,\"sources\":{\"" id "\":" MLN_TEST_EMPTY_GEOJSON_SOURCE \
  "},\"layers\":[]}"

static mln_status set_style_url(
  mln_map map, const char* url, mln_test_completion* completion
) {
  return mln_map_set_style_url(
    map, mln_buffer_view_from_cstr(url), &completion->descriptor, NULL
  );
}

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

static void add_source(mln_map map, const char* id) {
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, mln_buffer_view_from_cstr(id),
    MLN_BUFFER_LITERAL(MLN_TEST_EMPTY_GEOJSON_SOURCE), &completion.descriptor,
    NULL
  ));
}

// Expects a completion that the case already submitted to end as cancelled.
static void expect_cancelled(mln_test_completion* completion) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_CANCELLED, mln_test_completion_finish(completion)
  );
  mln_test_completion_destroy(completion);
}

// Expects each kind of style edit to fail while the style is not loaded.
static void expect_style_edits_rejected(mln_map map) {
  const char* const not_loaded = "style is not loaded";
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_STATE, not_loaded,
    mln_map_add_style_source_json(
      map, MLN_BUFFER_LITERAL("rejected"),
      MLN_BUFFER_LITERAL(MLN_TEST_EMPTY_GEOJSON_SOURCE), &completion.descriptor,
      NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_STATE, not_loaded,
    mln_map_add_style_layer_json(
      map, MLN_BUFFER_LITERAL("{\"id\":\"paper\",\"type\":\"background\"}"),
      MLN_BUFFER_LITERAL(""), &completion.descriptor, NULL
    )
  );
  const uint8_t pixel[4] = {0, 0, 0, 0};
  const mln_premultiplied_rgba8_image image = {
    .size = sizeof(mln_premultiplied_rgba8_image),
    .width = 1,
    .height = 1,
    .stride = 4,
    .pixels = pixel,
    .byte_length = sizeof(pixel),
  };
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_STATE, not_loaded,
    mln_map_set_style_image(
      map, MLN_BUFFER_LITERAL("rejected"), &image, NULL, &completion.descriptor,
      NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_STATE, not_loaded,
    mln_map_set_style_light_property(
      map, MLN_BUFFER_LITERAL("intensity"), MLN_BUFFER_LITERAL("0.25"),
      &completion.descriptor, NULL
    )
  );
  const mln_style_transition_options transition = {
    .size = sizeof(mln_style_transition_options),
    .fields = MLN_STYLE_TRANSITION_OPTION_DELAY,
    .delay_ms = 10.0,
  };
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_STATE, not_loaded,
    mln_map_set_style_transition_options(
      map, &transition, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_STATE, not_loaded,
    mln_map_set_global_state_property(
      map, MLN_BUFFER_LITERAL("theme"), MLN_BUFFER_LITERAL("true"),
      &completion.descriptor, NULL
    )
  );
  TEST_ASSERT_FALSE(source_exists(map, "rejected"));
}

static void a_new_map_takes_style_edits_once_a_style_loads(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  expect_style_edits_rejected(map);

  mln_test_load_style_and_wait(map, mln_test_empty_style_json);
  add_source(map, "accepted");
  TEST_ASSERT_TRUE(source_exists(map, "accepted"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_style_url_load_completes_after_its_style_parses(void) {
  static const char url[] = "custom://styles/parsed.json";
  static const mln_test_provided_resource resources[] = {
    {.url = url, .held = true},
  };
  mln_test_provider* provider = mln_test_provider_create(resources, 1);
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider_install(runtime, provider);
  mln_map map = mln_test_create_styled_map(runtime);
  add_source(map, "before");

  mln_test_completion load = mln_test_completion_default(0);
  MLN_TEST_OK(set_style_url(map, url, &load));
  TEST_ASSERT_TRUE(mln_test_provider_wait_for_requests(provider, url, 1));
  // These edits run after the load started, while the old style still shows,
  // and the parse would replace what they change.
  expect_style_edits_rejected(map);
  TEST_ASSERT_TRUE(source_exists(map, "before"));
  TEST_ASSERT_FALSE(mln_test_completion_poll(&load));

  const mln_resource_response parsed =
    mln_test_text_response(SOURCE_STYLE("from-style"));
  mln_test_request_answer(
    mln_test_provider_request_at(provider, url, 0)->handle, &parsed
  );
  MLN_TEST_OK(mln_test_completion_finish(&load));
  mln_test_completion_destroy(&load);
  TEST_ASSERT_TRUE(source_exists(map, "from-style"));
  TEST_ASSERT_FALSE(source_exists(map, "before"));
  add_source(map, "after");
  TEST_ASSERT_TRUE(source_exists(map, "after"));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

static void a_style_url_that_fails_to_load_fails_its_completion(void) {
  static const char missing_url[] = "custom://styles/missing.json";
  static const char invalid_url[] = "custom://styles/invalid.json";
  static const mln_test_provided_resource resources[] = {
    {.url = missing_url,
     .response =
       {
         .status = MLN_RESOURCE_RESPONSE_STATUS_ERROR,
         .error_reason = MLN_RESOURCE_ERROR_REASON_NOT_FOUND,
         .error_message = MLN_BUFFER_LITERAL("no such style"),
       }},
    {.url = invalid_url,
     .response = {
       .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
       .bytes = (const uint8_t*)"{\"version\":",
       .byte_count = sizeof("{\"version\":") - 1,
     }},
  };
  mln_test_provider* provider = mln_test_provider_create(resources, 2);
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider_install(runtime, provider);
  mln_map map = mln_test_create_styled_map(runtime);

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NATIVE_ERROR, "no such style",
    set_style_url(map, missing_url, &completion)
  );
  // A failed load leaves the map without a loaded style.
  expect_style_edits_rejected(map);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NATIVE_ERROR, "parse",
    set_style_url(map, invalid_url, &completion)
  );
  expect_style_edits_rejected(map);

  mln_test_load_style_and_wait(map, mln_test_empty_style_json);
  add_source(map, "recovered");
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

// Expects a completion that the case already submitted to have failed with
// MLN_STATUS_NATIVE_ERROR and a diagnostic that contains `fragment`.
static void expect_failed(
  mln_test_completion* completion, const char* fragment
) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NATIVE_ERROR, mln_test_completion_finish(completion)
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_completion_diagnostic(completion), fragment),
    mln_test_completion_diagnostic(completion)
  );
}

// MapLibre ignores a style response with no content or not modified, as one
// that confirms a style an earlier response delivered. A load whose first
// response is one of those has no style to parse, so it fails, once.
static void a_style_url_answered_without_a_style_fails_once(void) {
  static const char no_content_url[] = "custom://styles/no-content.json";
  static const char not_modified_url[] = "custom://styles/not-modified.json";
  static const char replacement_url[] = "custom://styles/replacement.json";
  static const mln_test_provided_resource resources[] = {
    {.url = no_content_url, .held = true},
    {.url = not_modified_url, .held = true},
    {.url = replacement_url,
     .response = {
       .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
       .bytes = (const uint8_t*)SOURCE_STYLE("replacement"),
       .byte_count = sizeof(SOURCE_STYLE("replacement")) - 1,
     }},
  };
  static const struct {
    const char* url;
    mln_resource_response_status status;
    const char* fragment;
  } cases[] = {
    {no_content_url, MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT, "no content"},
    {not_modified_url, MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED,
     "not modified"},
  };
  for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); index += 1) {
    const char* url = cases[index].url;
    mln_test_provider* provider = mln_test_provider_create(resources, 3);
    mln_runtime runtime = mln_test_create_runtime();
    mln_test_provider_install(runtime, provider);
    mln_map map = mln_test_create_map(runtime);

    mln_test_completion load = mln_test_completion_default(0);
    MLN_TEST_OK(set_style_url(map, url, &load));
    TEST_ASSERT_TRUE(mln_test_provider_wait_for_requests(provider, url, 1));
    const mln_resource_response answer = {
      .size = sizeof(mln_resource_response),
      .status = cases[index].status,
    };
    mln_test_request_answer(
      mln_test_provider_request_at(provider, url, 0)->handle, &answer
    );
    expect_failed(&load, cases[index].fragment);

    // Neither a newer load nor the map's release completes it again.
    MLN_TEST_AWAIT_OK(set_style_url(map, replacement_url, &completion));
    TEST_ASSERT_TRUE(source_exists(map, "replacement"));
    mln_test_destroy_map(map);
    expect_failed(&load, cases[index].fragment);
    mln_test_completion_destroy(&load);
    mln_test_destroy_runtime(runtime);
    mln_test_provider_destroy(provider);
  }
}

static void a_newer_style_load_cancels_the_pending_one(void) {
  static const char first_url[] = "custom://styles/first.json";
  static const char second_url[] = "custom://styles/second.json";
  static const char third_url[] = "custom://styles/third.json";
  static const mln_test_provided_resource resources[] = {
    {.url = first_url, .held = true},
    {.url = second_url, .held = true},
    {.url = third_url,
     .response = {
       .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
       .bytes = (const uint8_t*)SOURCE_STYLE("third"),
       .byte_count = sizeof(SOURCE_STYLE("third")) - 1,
     }},
  };
  mln_test_provider* provider = mln_test_provider_create(resources, 3);
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider_install(runtime, provider);
  mln_map map = mln_test_create_map(runtime);

  mln_test_completion first = mln_test_completion_default(0);
  MLN_TEST_OK(set_style_url(map, first_url, &first));
  TEST_ASSERT_TRUE(mln_test_provider_wait_for_requests(provider, first_url, 1));
  MLN_TEST_AWAIT_OK(mln_map_set_style_json(
    map, MLN_BUFFER_LITERAL(SOURCE_STYLE("inline")), &completion.descriptor,
    NULL
  ));
  expect_cancelled(&first);
  TEST_ASSERT_TRUE(source_exists(map, "inline"));

  mln_test_completion second = mln_test_completion_default(0);
  MLN_TEST_OK(set_style_url(map, second_url, &second));
  TEST_ASSERT_TRUE(
    mln_test_provider_wait_for_requests(provider, second_url, 1)
  );
  MLN_TEST_AWAIT_OK(set_style_url(map, third_url, &completion));
  expect_cancelled(&second);
  TEST_ASSERT_TRUE(source_exists(map, "third"));

  // The replaced loads no longer wait for their responses.
  const mln_resource_response late =
    mln_test_text_response(SOURCE_STYLE("late"));
  mln_test_request_answer(
    mln_test_provider_request_at(provider, first_url, 0)->handle, &late
  );
  mln_test_request_answer(
    mln_test_provider_request_at(provider, second_url, 0)->handle, &late
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

static void releasing_the_map_cancels_its_pending_style_load(void) {
  static const char url[] = "custom://styles/abandoned.json";
  static const mln_test_provided_resource resources[] = {
    {.url = url, .held = true},
  };
  mln_test_provider* provider = mln_test_provider_create(resources, 1);
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider_install(runtime, provider);
  mln_map map = mln_test_create_map(runtime);

  mln_test_completion load = mln_test_completion_default(0);
  MLN_TEST_OK(set_style_url(map, url, &load));
  TEST_ASSERT_TRUE(mln_test_provider_wait_for_requests(provider, url, 1));
  MLN_TEST_OK(mln_test_map_release(map));
  expect_cancelled(&load);

  const mln_resource_response late =
    mln_test_text_response(SOURCE_STYLE("late"));
  mln_test_request_answer(
    mln_test_provider_request_at(provider, url, 0)->handle, &late
  );
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

MLN_TEST_GROUP {
  RUN_TEST(a_new_map_takes_style_edits_once_a_style_loads);
  RUN_TEST(a_style_url_load_completes_after_its_style_parses);
  RUN_TEST(a_style_url_that_fails_to_load_fails_its_completion);
  RUN_TEST(a_style_url_answered_without_a_style_fails_once);
  RUN_TEST(a_newer_style_load_cancels_the_pending_one);
  RUN_TEST(releasing_the_map_cancels_its_pending_style_load);
}
