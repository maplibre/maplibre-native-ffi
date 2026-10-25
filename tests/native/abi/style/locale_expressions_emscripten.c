// Locale-sensitive style expressions in the browser, which evaluate them with
// the JavaScript engine's Intl API: number formatting follows the locale,
// currency, and fraction limits, and a collator follows the locale, its
// collation extensions, and the requested sensitivity, on every thread that
// evaluates it. The `_emscripten` suffix builds this file on browser targets
// only.

#include <stdio.h>
#include <string.h>

#include "support/map.h"
#include "support/style.h"
#include "support/test_support.h"

// A map of point features with the given names, ids `case-0`, `case-1`, and so
// on, and a `currency` and a `locale` property that are each malformed. The
// circle layer draws the features that pass `layer_filter`. The map has
// rendered once, so its source has loaded and its tiles have laid out.
typedef struct locale_map {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture;
} locale_map;

static void open_map(
  locale_map* map, const char* layer_filter, const char* const* names,
  size_t count
) {
  char features[1024] = "";
  size_t used = 0;
  for (size_t index = 0; index < count; index += 1) {
    const int written = snprintf(
      features + used, sizeof(features) - used,
      "%s{\"type\":\"Feature\",\"id\":\"case-%zu\",\"geometry\":"
      "{\"type\":\"Point\",\"coordinates\":[0,0]},\"properties\":"
      "{\"name\":\"%s\",\"currency\":\"US\",\"locale\":\"not a tag\"}}",
      index == 0 ? "" : ",", index, names[index]
    );
    TEST_ASSERT_GREATER_THAN_INT(0, written);
    TEST_ASSERT_LESS_THAN_size_t(sizeof(features) - used, (size_t)written);
    used += (size_t)written;
  }
  char style[2048];
  const int size = snprintf(
    style, sizeof(style),
    "{\"version\":8,\"sources\":{\"point\":{\"type\":\"geojson\",\"data\":"
    "{\"type\":\"FeatureCollection\",\"features\":[%s]}}},\"layers\":[{\"id\":"
    "\"point\",\"type\":\"circle\",\"source\":\"point\",\"filter\":%s}]}",
    features, layer_filter
  );
  TEST_ASSERT_GREATER_THAN_INT(0, size);
  TEST_ASSERT_LESS_THAN_INT(sizeof(style), size);

  map->runtime = mln_test_create_runtime();
  mln_map_options options = mln_test_map_options();
  options.initial_extent =
    (mln_logical_extent){.width = 64, .height = 64, .scale_factor = 1.0};
  options.map_mode = MLN_MAP_MODE_STATIC;
  map->map = mln_test_create_map_with_options(map->runtime, &options);
  mln_test_load_style_and_wait(
    map->map, mln_test_buffer_view(style, (size_t)size)
  );
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map->map, &map->fixture));
  // The still image waits for the source to load.
  MLN_TEST_OK(mln_test_render_still_image(&map->fixture, map->map));
}

static void open_point(locale_map* map) {
  static const char* const names[] = {"point"};
  open_map(map, "[\"all\"]", names, 1);
}

static void close_map(locale_map* map) {
  mln_test_render_fixture_destroy(&map->fixture);
  mln_test_destroy_map(map->map);
  mln_test_destroy_runtime(map->runtime);
}

// A mask of the features in `list`, by the index in their ids.
static uint32_t feature_mask(const mln_test_feature_list* list, size_t count) {
  MLN_TEST_OK(list->status);
  uint32_t mask = 0;
  for (size_t hit = 0; hit < list->count; hit += 1) {
    for (size_t index = 0; index < count; index += 1) {
      char id[32];
      snprintf(id, sizeof(id), "\"id\":\"case-%zu\"", index);
      if (strstr(list->features[hit].feature, id) != NULL) {
        mask |= 1U << index;
      }
    }
  }
  return mask;
}

// A mask of the features that pass `filter`, which leaves out a feature whose
// evaluation fails.
static uint32_t selected(
  const locale_map* map, const char* filter, size_t count
) {
  mln_source_feature_query_options query = {
    .size = sizeof(mln_source_feature_query_options)
  };
  query.filter = mln_buffer_view_from_cstr(filter);
  const mln_test_feature_list list =
    mln_test_style_query_source_with(&map->fixture, "point", &query);
  return feature_mask(&list, count);
}

static bool selects(const locale_map* map, const char* filter) {
  return selected(map, filter, 1) == 1;
}

#define EXPECT_SELECTS(map, filter) \
  TEST_ASSERT_TRUE_MESSAGE(selects((map), (filter)), (filter))

#define FORMATS(number, options, formatted)                    \
  "[\"==\",[\"number-format\"," number ",{\"locale\":" options \
  "}],\"" formatted "\"]"

static void number_format_follows_locale_currency_and_fraction_limits(void) {
  locale_map map;
  open_point(&map);
  EXPECT_SELECTS(&map, FORMATS("1234.5", "\"de-DE\"", "1.234,5"));
  EXPECT_SELECTS(
    &map, FORMATS("12.3", "\"en-US\",\"min-fraction-digits\":4", "12.3000")
  );
  // An explicit maximum wins over a minimum, and a half rounds to even.
  EXPECT_SELECTS(
    &map,
    FORMATS(
      "0.125", "\"en-US\",\"min-fraction-digits\":4,\"max-fraction-digits\":2",
      "0.12"
    )
  );
  // Currency takes the currency's digits unless a limit overrides them.
  EXPECT_SELECTS(
    &map,
    FORMATS("1234.5", "\"de-DE\",\"currency\":\"EUR\"", "1.234,50\\u00a0€")
  );
  EXPECT_SELECTS(
    &map,
    FORMATS(
      "12.3", "\"en-US\",\"currency\":\"USD\",\"max-fraction-digits\":0", "$12"
    )
  );
  EXPECT_SELECTS(
    &map, FORMATS("-12.3", "\"en-US\",\"currency\":\"USD\"", "-$12.30")
  );
  EXPECT_SELECTS(
    &map, FORMATS("1234.6", "\"ja-JP\",\"currency\":\"JPY\"", "￥1,235")
  );
  // A currency that is not an ISO 4217 code, or a locale that is not a
  // language tag, fails the evaluation.
  TEST_ASSERT_FALSE(selects(
    &map,
    "[\"!=\",[\"number-format\",1,{\"currency\":[\"get\",\"currency\"]}],\"\"]"
  ));
  TEST_ASSERT_FALSE(selects(
    &map,
    "[\"!=\",[\"number-format\",1,{\"locale\":[\"get\",\"locale\"]}],\"\"]"
  ));
  close_map(&map);
}

// A comparison of two strings under a collator for `locale`.
static bool compares(
  const locale_map* map, const char* comparison, const char* lhs,
  const char* rhs, const char* locale, bool case_sensitive,
  bool diacritic_sensitive
) {
  char filter[256];
  const int size = snprintf(
    filter, sizeof(filter),
    "[\"%s\",\"%s\",\"%s\",[\"collator\",{\"locale\":\"%s\","
    "\"case-sensitive\":%s,\"diacritic-sensitive\":%s}]]",
    comparison, lhs, rhs, locale, case_sensitive ? "true" : "false",
    diacritic_sensitive ? "true" : "false"
  );
  TEST_ASSERT_GREATER_THAN_INT(0, size);
  TEST_ASSERT_LESS_THAN_INT(sizeof(filter), size);
  return selects(map, filter);
}

static void collator_follows_locale_extensions_and_sensitivity(void) {
  locale_map map;
  open_point(&map);
  for (int sensitivity = 0; sensitivity < 4; sensitivity += 1) {
    const bool case_sensitive = (sensitivity & 1) != 0;
    const bool diacritic_sensitive = (sensitivity & 2) != 0;
    TEST_ASSERT_EQUAL(
      !case_sensitive,
      compares(
        &map, "==", "A", "a", "en-US", case_sensitive, diacritic_sensitive
      )
    );
    TEST_ASSERT_EQUAL(
      !diacritic_sensitive,
      compares(
        &map, "==", "\\u00e9", "e", "en-US", case_sensitive, diacritic_sensitive
      )
    );
  }
  // Swedish sorts ä after z, and German sorts it with a.
  TEST_ASSERT_TRUE(compares(&map, ">", "\\u00e4", "z", "sv-SE", false, false));
  TEST_ASSERT_TRUE(compares(&map, "<", "\\u00e4", "z", "de-DE", false, false));
  TEST_ASSERT_TRUE(
    compares(&map, "==", "\\u00e4", "ae", "de-DE-u-co-phonebk", false, false)
  );
  TEST_ASSERT_TRUE(compares(&map, "<", "2", "10", "en-US-u-kn", false, false));
  // The comparison reads past an embedded NUL.
  TEST_ASSERT_FALSE(
    compares(&map, "==", "a\\u0000b", "a\\u0000c", "en-US", false, false)
  );
  // A locale that is not a language tag fails the evaluation.
  TEST_ASSERT_FALSE(selects(
    &map,
    "[\"==\",\"a\",\"a\",[\"collator\",{\"locale\":[\"get\",\"locale\"]}]]"
  ));
  close_map(&map);
}

#define RESOLVED(locale) \
  "[\"resolved-locale\",[\"collator\",{\"locale\":\"" locale "\"}]]"

static void resolved_locale_reports_the_language_and_honored_extensions(void) {
  locale_map map;
  open_point(&map);
  // The engine may drop a region that its collation data does not need.
  EXPECT_SELECTS(&map, "[\"==\",[\"slice\"," RESOLVED("sv-SE") ",0,2],\"sv\"]");
  EXPECT_SELECTS(
    &map, "[\"in\",\"-u-co-phonebk-kn\"," RESOLVED("de-DE-u-co-phonebk-kn") "]"
  );
  // English has no phonebook collation.
  EXPECT_SELECTS(&map, "[\"==\"," RESOLVED("en-US-u-co-phonebk") ",\"en-US\"]");
  // An unknown language takes the default collation locale.
  EXPECT_SELECTS(
    &map,
    "[\"==\"," RESOLVED("xx-ZZ") ",[\"resolved-locale\",[\"collator\",{}]]]"
  );
  close_map(&map);
}

static void collator_compares_on_the_thread_that_lays_out_tiles(void) {
  // The style folds the constant collator when it parses the layer, and the
  // tile worker compares with it.
  static const char* const names[] = {"a", "\\u00e4", "\\u00f6", "z"};
  locale_map map;
  open_map(
    &map,
    "[\">\",[\"get\",\"name\"],\"z\",[\"collator\",{\"locale\":\"sv-SE\"}]]",
    names, 4
  );
  const mln_test_feature_list rendered =
    mln_test_style_query_rendered(&map.fixture);
  TEST_ASSERT_EQUAL_HEX32(0x6, feature_mask(&rendered, 4));
  close_map(&map);
}

MLN_TEST_GROUP {
  RUN_TEST(number_format_follows_locale_currency_and_fraction_limits);
  RUN_TEST(collator_follows_locale_extensions_and_sensitivity);
  RUN_TEST(resolved_locale_reports_the_language_and_honored_extensions);
  RUN_TEST(collator_compares_on_the_thread_that_lays_out_tiles);
}
