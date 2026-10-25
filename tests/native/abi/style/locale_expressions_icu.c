// Locale-sensitive style expressions on targets that evaluate them with ICU:
// Linux, with the CLDR data that ICU4X compiles into the library, and
// OpenHarmony and Windows, with the system's ICU4C. Number formatting follows
// the locale, currency, and fraction limits, and a collator follows the locale,
// its collation extensions, and the requested sensitivity. The `_icu` suffix
// builds this file on those targets only.

#include <stdio.h>

#include "support/map.h"
#include "support/style.h"
#include "support/test_support.h"

// A map with one point feature whose `currency` property is not an ISO 4217
// code, rendered once so that its source has loaded.
typedef struct locale_map {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture;
} locale_map;

static void open_map(locale_map* map) {
  map->runtime = mln_test_create_runtime();
  mln_map_options options = mln_test_map_options();
  options.initial_extent =
    (mln_logical_extent){.width = 64, .height = 64, .scale_factor = 1.0};
  options.map_mode = MLN_MAP_MODE_STATIC;
  map->map = mln_test_create_map_with_options(map->runtime, &options);
  mln_test_load_style_and_wait(
    map->map,
    MLN_BUFFER_LITERAL(
      "{\"version\":8,\"sources\":{\"point\":{\"type\":\"geojson\",\"data\":{"
      "\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\",\"coordinates\":"
      "[0,0]},\"properties\":{\"currency\":\"US$\"}}}},\"layers\":[{\"id\":"
      "\"point\",\"type\":\"circle\",\"source\":\"point\"}]}"
    )
  );
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map->map, &map->fixture));
  // The still image waits for the source to load.
  MLN_TEST_OK(mln_test_render_still_image(&map->fixture, map->map));
}

static void close_map(locale_map* map) {
  mln_test_render_fixture_destroy(&map->fixture);
  mln_test_destroy_map(map->map);
  mln_test_destroy_runtime(map->runtime);
}

// Whether the feature passes `filter`, which is false when evaluating the
// filter fails.
static bool selects(const locale_map* map, const char* filter) {
  mln_source_feature_query_options query =
    mln_source_feature_query_options_default();
  query.filter = mln_buffer_view_from_cstr(filter);
  const mln_test_feature_list list =
    mln_test_style_query_source_with(&map->fixture, "point", &query);
  MLN_TEST_OK(list.status);
  return list.count == 1;
}

#define EXPECT_SELECTS(map, filter) \
  TEST_ASSERT_TRUE_MESSAGE(selects((map), (filter)), (filter))

#define FORMATS(number, options, formatted)                    \
  "[\"==\",[\"number-format\"," number ",{\"locale\":" options \
  "}],\"" formatted "\"]"

static void number_format_follows_locale_currency_and_fraction_limits(void) {
  locale_map map;
  open_map(&map);
  EXPECT_SELECTS(&map, FORMATS("1234.5", "\"de-DE\"", "1.234,5"));
  EXPECT_SELECTS(
    &map, FORMATS("12.3", "\"en-US\",\"min-fraction-digits\":4", "12.3000")
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
  // A currency that is not an ISO 4217 code fails the evaluation.
  TEST_ASSERT_FALSE(selects(
    &map,
    "[\"!=\",[\"number-format\",1,{\"currency\":[\"get\",\"currency\"]}],\"\"]"
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
  open_map(&map);
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
  close_map(&map);
}

#define RESOLVES(locale, resolved)                                   \
  "[\"==\",[\"resolved-locale\",[\"collator\",{\"locale\":\"" locale \
  "\"}]]," resolved "]"

static void resolved_locale_reports_the_language_and_honored_extensions(void) {
  locale_map map;
  open_map(&map);
  EXPECT_SELECTS(&map, RESOLVES("sv-SE", "\"sv-SE\""));
  EXPECT_SELECTS(
    &map, RESOLVES("de-DE-u-co-phonebk-kn", "\"de-DE-u-co-phonebk-kn\"")
  );
  // English has no phonebook collation.
  EXPECT_SELECTS(&map, RESOLVES("en-US-u-co-phonebk", "\"en-US\""));
  // An unknown language takes the default collation locale.
  EXPECT_SELECTS(
    &map, RESOLVES("xx-ZZ", "[\"resolved-locale\",[\"collator\",{}]]")
  );
  close_map(&map);
}

MLN_TEST_GROUP {
  RUN_TEST(number_format_follows_locale_currency_and_fraction_limits);
  RUN_TEST(collator_follows_locale_extensions_and_sensitivity);
  RUN_TEST(resolved_locale_reports_the_language_and_honored_extensions);
}
