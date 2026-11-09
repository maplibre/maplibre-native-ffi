// mln_map_options.fallback_font_faces gives every font stack fonts for the
// text that a style's own fonts don't draw.

#include "support/map.h"
#include "support/resources.h"
#include "support/test_support.h"

static const char fallback_font_url[] = "test://fonts/myanmar.ttf";

static const char fallback_font_faces[] =
  "[{\"url\":\"test://fonts/"
  "myanmar.ttf\",\"unicode-range\":[\"U+1000-109F\"]}]";

// A Myanmar label in a font stack that no font face names.
static const char myanmar_style_json[] =
  "{\"version\":8,\"glyphs\":\"test://fonts/{fontstack}/{range}.pbf\","
  "\"sources\":{\"points\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0,0]},\"properties\":{}}}},"
  "\"layers\":[{\"id\":\"label\",\"type\":\"symbol\",\"source\":\"points\","
  "\"layout\":{\"text-field\":\"\\u1019\\u103c\\u1014\\u103a\\u1019\\u102c\","
  "\"text-font\":[\"TestFont\"]}}]}";

static void a_map_loads_a_fallback_font_for_text_its_style_fonts_lack(void) {
  mln_runtime runtime = mln_test_create_runtime();
  // Every request fails as not found, but the provider records it.
  mln_test_provider* provider = mln_test_provider_create(NULL, 0);
  mln_test_provider_install(runtime, provider);

  mln_map_options options = mln_test_map_options();
  options.initial_extent =
    (mln_logical_extent){.width = 64, .height = 64, .scale_factor = 1.0};
  options.map_mode = MLN_MAP_MODE_STATIC;
  options.fallback_font_faces = MLN_BUFFER_LITERAL(fallback_font_faces);
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_test_load_style_and_wait(map, MLN_BUFFER_LITERAL(myanmar_style_json));

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  // The still image fails, since no font loads, after the map asks for one.
  (void)mln_test_render_still_image(&fixture, map);
  TEST_ASSERT_TRUE(
    mln_test_provider_wait_for_requests(provider, fallback_font_url, 1)
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

static void a_map_rejects_fallback_font_faces_that_are_not_font_files(void) {
  mln_runtime runtime = mln_test_create_runtime();
  static const char* const invalid[] = {
    "{}",
    "[{\"unicode-range\":[\"U+1000-109F\"]}]",
    "[{\"url\":\"test://fonts/a.ttf\",\"unicode-range\":[\"U+XYZ-FF\"]}]",
    "not json",
  };
  for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]);
       index += 1) {
    mln_map_options options = mln_test_map_options();
    options.fallback_font_faces =
      (mln_buffer_view){.data = invalid[index], .size = strlen(invalid[index])};
    mln_map map = MLN_HANDLE_NULL;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT,
      mln_test_map_create_status(runtime, &options, &map), invalid[index]
    );
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, map);
  }
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_map_loads_a_fallback_font_for_text_its_style_fonts_lack);
  RUN_TEST(a_map_rejects_fallback_font_faces_that_are_not_font_files);
}
