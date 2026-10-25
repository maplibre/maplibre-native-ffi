// Locale-sensitive style expressions on Apple platforms, which evaluate them
// with Foundation: number formatting keeps its defaults and honors explicit
// fraction limits, and a collator rejects malformed UTF-8 and keeps embedded
// NULs. The `_metal` suffix builds this file on Apple targets only.

#include "support/map.h"
#include "support/style.h"
#include "support/test_support.h"

// Sets the background layer's opacity to 0.25 when `formatted` is what
// `number-format` makes of 12.3 under `options`, and to 0.75 otherwise, and
// expects 0.25. The expression is constant, so the style folds it on the
// command and the property reads back folded.
static void expect_formatted(const char* options, const char* formatted) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    map, MLN_BUFFER_LITERAL(
           "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"background\","
           "\"type\":\"background\"}]}"
         )
  );
  char expression[256];
  const int size = snprintf(
    expression, sizeof(expression),
    "[\"case\",[\"==\",[\"number-format\",12.3,{\"locale\":\"en-US\"%s}],"
    "\"%s\"],0.25,0.75]",
    options, formatted
  );
  TEST_ASSERT_GREATER_THAN_INT(0, size);
  TEST_ASSERT_LESS_THAN_INT(sizeof(expression), size);
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("background"),
    MLN_BUFFER_LITERAL("background-opacity"),
    mln_test_buffer_view(expression, (size_t)size), &completion.descriptor, NULL
  ));

  mln_test_completion read = mln_test_completion_buffer_view();
  MLN_TEST_OK(mln_map_get_style_layer_property(
    map, MLN_BUFFER_LITERAL("background"),
    MLN_BUFFER_LITERAL("background-opacity"), &read.descriptor, NULL
  ));
  char opacity[64];
  MLN_TEST_OK(
    mln_test_style_finish_text(&read, opacity, sizeof(opacity), NULL)
  );
  TEST_ASSERT_EQUAL_STRING_MESSAGE("0.25", opacity, options);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void number_format_keeps_defaults_and_honors_fraction_limits(void) {
  expect_formatted("", "12.3");
  expect_formatted(
    ",\"currency\":\"USD\",\"min-fraction-digits\":0,"
    "\"max-fraction-digits\":3",
    "$12.3"
  );
  expect_formatted(",\"currency\":\"USD\",\"max-fraction-digits\":0", "$12");
}

// Renders one point feature per name in `names`, with ids `case-0`,
// `case-1`, and so on, queries the source under `filter`, and returns a mask
// of the indices it kept. A name and the filter may hold bytes that are not
// valid UTF-8.
static uint32_t selected_names(
  const char* filter, const char* const* names, size_t count
) {
  char features[1024] = "";
  size_t used = 0;
  for (size_t index = 0; index < count; index += 1) {
    const int written = snprintf(
      features + used, sizeof(features) - used,
      "%s{\"type\":\"Feature\",\"id\":\"case-%zu\",\"geometry\":"
      "{\"type\":\"Point\",\"coordinates\":[0,0]},\"properties\":"
      "{\"name\":\"%s\"}}",
      index == 0 ? "" : ",", index, names[index]
    );
    TEST_ASSERT_GREATER_THAN_INT(0, written);
    TEST_ASSERT_LESS_THAN_size_t(sizeof(features) - used, (size_t)written);
    used += (size_t)written;
  }
  char style[1536];
  const int style_size = snprintf(
    style, sizeof(style),
    "{\"version\":8,\"sources\":{\"point\":{\"type\":\"geojson\",\"data\":"
    "{\"type\":\"FeatureCollection\",\"features\":[%s]}}},\"layers\":[{\"id\":"
    "\"cases\",\"type\":\"circle\",\"source\":\"point\"}]}",
    features
  );
  TEST_ASSERT_GREATER_THAN_INT(0, style_size);
  TEST_ASSERT_LESS_THAN_INT(sizeof(style), style_size);

  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_test_map_options();
  options.initial_extent =
    (mln_logical_extent){.width = 64, .height = 64, .scale_factor = 1.0};
  options.map_mode = MLN_MAP_MODE_STATIC;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_test_load_style_and_wait(
    map, mln_test_buffer_view(style, (size_t)style_size)
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  // The still image waits for the source to load.
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));

  mln_source_feature_query_options query = {
    .size = sizeof(mln_source_feature_query_options)
  };
  query.filter = mln_buffer_view_from_cstr(filter);
  const mln_test_feature_list list =
    mln_test_style_query_source_with(&fixture, "point", &query);
  MLN_TEST_OK(list.status);
  uint32_t selected = 0;
  for (size_t hit = 0; hit < list.count; hit += 1) {
    for (size_t index = 0; index < count; index += 1) {
      char id[32];
      snprintf(id, sizeof(id), "\"id\":\"case-%zu\"", index);
      if (strstr(list.features[hit].feature, id) != NULL) {
        selected |= 1U << index;
      }
    }
  }

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  return selected;
}

#define INSENSITIVE_COLLATOR                                      \
  "[\"collator\",{\"locale\":\"en-US\",\"case-sensitive\":false," \
  "\"diacritic-sensitive\":false}]"

static void collation_rejects_malformed_utf8_and_keeps_embedded_nuls(void) {
  // A byte that is never valid UTF-8 compares equal to nothing, while the
  // diacritic-insensitive comparison still matches the accented name.
  static const char* const accents[] = {"cafe", "caf\xC3\xA9", "\xFF"};
  TEST_ASSERT_EQUAL_HEX32(
    0x3, selected_names(
           "[\"==\",[\"get\",\"name\"],\"cafe\"," INSENSITIVE_COLLATOR "]",
           accents, 3
         )
  );

  // The comparison reads past an embedded NUL.
  static const char* const nuls[] = {"a\\u0000b", "a\\u0000c"};
  TEST_ASSERT_EQUAL_HEX32(
    0x1, selected_names(
           "[\"==\",[\"get\",\"name\"],\"a\\u0000b\"," INSENSITIVE_COLLATOR "]",
           nuls, 2
         )
  );
}

MLN_TEST_GROUP {
  RUN_TEST(number_format_keeps_defaults_and_honors_fraction_limits);
  RUN_TEST(collation_rejects_malformed_utf8_and_keeps_embedded_nuls);
}
