// Style commands that refuse their input or their target: empty views,
// images without pixels, invalid style JSON, and sources whose ID is
// taken or whose kind does not take the command.

#include "support/style.h"
#include "support/test_support.h"

// Expects `expression`, which submits with `completion.descriptor`, to refuse
// the empty view `name` before it returns. The diagnostic names the view, and
// the completion stays with the caller without its callback running.
#define EXPECT_EMPTY_REJECTED(name, expression) \
  MLN_TEST_EXPECT_COMMAND_REJECTED(name " must not be empty", expression)

static const mln_lat_lng corners[4] = {{1, 1}, {1, 2}, {0, 2}, {0, 1}};

// Every ID, URL, and JSON input that must name at least one byte is
// refused at submission when empty, so no empty view reaches the map worker.
// One shared check guards every such input, so one ID and one other view stand
// for them.
static void empty_views_are_rejected_at_submission(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_buffer_view empty = MLN_BUFFER_LITERAL("");
  const mln_buffer_view id = MLN_BUFFER_LITERAL("id");
  const mln_buffer_view json = MLN_BUFFER_LITERAL("{}");

  EXPECT_EMPTY_REJECTED(
    "source_id", mln_map_add_style_source_json(
                   map, empty, json, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );
  EXPECT_EMPTY_REJECTED(
    "property_name",
    mln_map_set_style_layer_property(
      map, id, empty, json, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Some inputs are refused at submission and some by the command, so this reads
// the status and diagnostic from whichever stage refused `expression`, which
// submits with `completion.descriptor` and `&diagnostic`.
#define EXPECT_REFUSED(fragment, expression)                                \
  do {                                                                      \
    mln_test_completion completion = mln_test_completion_default(0);        \
    mln_diagnostic diagnostic = {.size = sizeof(mln_diagnostic)};           \
    mln_status status = (expression);                                       \
    const char* message = diagnostic.message;                               \
    if (status == MLN_STATUS_OK) {                                          \
      status = mln_test_completion_finish(&completion);                     \
      message = mln_test_completion_diagnostic(&completion);                \
    } else {                                                                \
      mln_test_completion_reject(&completion);                              \
    }                                                                       \
    TEST_ASSERT_EQUAL_INT_MESSAGE(                                          \
      MLN_STATUS_INVALID_ARGUMENT, status, #expression                      \
    );                                                                      \
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(message, (fragment)), #expression); \
    mln_test_completion_destroy(&completion);                               \
  } while (false)

static void style_commands_reject_invalid_inputs(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_style_serve(runtime, NULL, 0);
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  mln_premultiplied_rgba8_image no_pixels = {
    .size = sizeof(mln_premultiplied_rgba8_image)
  };
  no_pixels.width = 1;
  no_pixels.height = 1;
  no_pixels.stride = 4;
  no_pixels.byte_length = 4;
  EXPECT_REFUSED(
    "pixels must not be null", mln_map_set_style_image(
                                 map, MLN_BUFFER_LITERAL("marker"), &no_pixels,
                                 NULL, &completion.descriptor, &diagnostic
                               )
  );
  // Last: a style that fails to load leaves the map without its style.
  static const char nul_json[] = "{\0}";
  EXPECT_REFUSED(
    "must not contain embedded NUL",
    mln_map_set_style_json(
      map, mln_test_buffer_view(nul_json, sizeof(nul_json) - 1),
      &completion.descriptor, &diagnostic
    )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A command whose target is taken or of the wrong kind fails at commit with
// the reason.
static void source_commands_fail_on_a_taken_or_wrong_target(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_empty_style_json);
  static mln_test_style_route routes[] = {
    {.url = "fixture://taken.geojson", .body = "{}"},
  };
  mln_test_style_serve(runtime, routes, 1);
  const mln_buffer_view taken = MLN_BUFFER_LITERAL("taken");
  const mln_buffer_view png = MLN_BUFFER_LITERAL("fixture://image.png");
  const mln_buffer_view geojson_url =
    MLN_BUFFER_LITERAL("fixture://taken.geojson");
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, taken, MLN_BUFFER_LITERAL(MLN_TEST_EMPTY_GEOJSON_SOURCE),
    &completion.descriptor, NULL
  ));

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "style source",
    mln_map_add_style_source_json(
      map, MLN_BUFFER_LITERAL("unknown-type"),
      MLN_BUFFER_LITERAL("{\"type\":\"no-such-source-type\"}"),
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "source already exists",
    mln_map_add_geojson_source_url(
      map, taken, geojson_url, NULL, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "source already exists",
    mln_map_add_image_source_url(
      map, taken, corners, 4, png, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "not an image source",
    mln_map_set_image_source_url(map, taken, png, &completion.descriptor, NULL)
  );

  // A URL update and the synchronous tiling override belong to GeoJSON
  // sources alone. internal/geojson_tiling.cpp shows that the override
  // reaches the next frame.
  const mln_buffer_view image = MLN_BUFFER_LITERAL("image");
  MLN_TEST_AWAIT_OK(mln_map_add_image_source_url(
    map, image, corners, 4, png, &completion.descriptor, NULL
  ));
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "not a GeoJSON source",
    mln_map_set_geojson_source_url(
      map, image, geojson_url, &completion.descriptor, NULL
    )
  );
  const mln_buffer_view vector = MLN_BUFFER_LITERAL("vector");
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, vector,
    MLN_BUFFER_LITERAL(
      "{\"type\":\"vector\",\"tiles\":[\"fixture://tiles/{z}/{x}/{y}.mvt\"]}"
    ),
    &completion.descriptor, NULL
  ));
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "not a GeoJSON source",
    mln_map_set_geojson_source_synchronous_tiling(
      map, vector, true, &completion.descriptor, NULL
    )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(empty_views_are_rejected_at_submission);
  RUN_TEST(style_commands_reject_invalid_inputs);
  RUN_TEST(source_commands_fail_on_a_taken_or_wrong_target);
}
