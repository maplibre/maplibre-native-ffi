// The URL language that provider routes and transform rules share: the glob
// patterns that mln_url_match_flag documents, which URL a route compares, and
// where a resource transform rule adds its query parameters. A loader cannot
// produce every candidate URL, so these cases call the matchers directly.

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "resources/url_rules.hpp"

#include "maplibre_native_c.h"
#include "support/test_support.h"

namespace {

struct GlobCase {
  std::string_view pattern;
  std::string_view url;
  bool matches;
};

constexpr auto style_url =
  std::string_view{"https://maps.localhost/style.json"};

constexpr auto glob_cases = std::array{
  // `*` spans one path segment, so a path segment that carries the host name
  // leaves a host pattern unmatched.
  GlobCase{"https://*.localhost/**", style_url, true},
  GlobCase{"https://*.localhost/**", "https://tiles.localhost/1/2/3.pbf", true},
  GlobCase{
    "https://*.localhost/**", "https://127.0.0.1/x.localhost/style.json", false
  },
  GlobCase{"https://*/style.json", style_url, true},
  GlobCase{"https://*.localhost/*", style_url, true},
  GlobCase{"https://*.localhost/*/*", style_url, false},
  GlobCase{"**", style_url, true},
  GlobCase{"*", style_url, false},
  // A pattern matches the complete URL, so a bare suffix or infix matches
  // nothing without a leading wildcard.
  GlobCase{"**.json", style_url, true},
  GlobCase{".json", style_url, false},
  GlobCase{"**.pbf", style_url, false},
  GlobCase{"https://maps**", style_url, true},
  // A pattern with no metacharacters compares byte for byte.
  GlobCase{"https://maps.localhost/style.json", style_url, true},
  GlobCase{"https://maps.localhost/Style.json", style_url, false},
  GlobCase{"", style_url, false},
  GlobCase{"", "", true},
  // `?` spans one character other than `/`.
  GlobCase{"?ttps://maps.localhost/style.json", style_url, true},
  GlobCase{"https://maps.localhost/style?json", style_url, true},
  GlobCase{"https:?**", style_url, false},
  // `\` makes the next character literal, and a trailing `\` matches itself.
  GlobCase{"https://maps.localhost/style\\?json", style_url, false},
  GlobCase{"https://star\\*.localhost/x", "https://star*.localhost/x", true},
  GlobCase{"https://star\\*.localhost/x", "https://starry.localhost/x", false},
  GlobCase{"custom://style\\", "custom://style\\", true},
  GlobCase{"custom://style\\", "custom://style", false},
  // A failed match after a wildcard run retries from a later position.
  GlobCase{"https://**/tiles/*", "https://host/tiles/a/b/tiles/x", true},
};

void globs_match_the_documented_pattern_language() {
  for (const auto& row : glob_cases) {
    const auto pattern = std::string{row.pattern};
    TEST_ASSERT_EQUAL_MESSAGE(
      row.matches, mln::core::CompiledGlob{row.pattern}.matches(row.url),
      pattern.c_str()
    );
  }
  const auto exact = mln::core::UrlPattern{MLN_RESOURCE_KIND_FLAG_ALL, 0, "**"};
  TEST_ASSERT_FALSE(exact.matches(MLN_RESOURCE_KIND_STYLE, style_url));
  TEST_ASSERT_TRUE(exact.matches(MLN_RESOURCE_KIND_STYLE, "**"));
}

auto table_of(mln_resource_route route) -> mln::core::RouteTable {
  auto table = mln::core::RouteTable{};
  MLN_TEST_OK(mln::core::copy_routes(&route, 1, table));
  return table;
}

void routes_match_resolved_urls_and_selected_kinds() {
  const auto resolved = table_of({
    .size = sizeof(mln_resource_route),
    .kinds = MLN_RESOURCE_KIND_FLAG_STYLE | MLN_RESOURCE_KIND_FLAG_SOURCE,
    .flags = 0,
    .url = MLN_BUFFER_LITERAL("https://maps.localhost/style.json"),
  });
  TEST_ASSERT_TRUE(resolved.matches(MLN_RESOURCE_KIND_STYLE, style_url));
  TEST_ASSERT_TRUE(resolved.matches(MLN_RESOURCE_KIND_SOURCE, style_url));
  TEST_ASSERT_FALSE(resolved.matches(MLN_RESOURCE_KIND_TILE, style_url));
  TEST_ASSERT_FALSE(
    resolved.matches(static_cast<mln_resource_kind>(UINT32_MAX), style_url)
  );

  const auto every_kind = table_of({
    .size = sizeof(mln_resource_route),
    .kinds = 0,
    .flags = 0,
    .url = MLN_BUFFER_LITERAL("https://maps.localhost/style.json"),
  });
  for (const auto kind :
       {MLN_RESOURCE_KIND_UNKNOWN, MLN_RESOURCE_KIND_STYLE,
        MLN_RESOURCE_KIND_TILE, MLN_RESOURCE_KIND_IMAGE}) {
    TEST_ASSERT_TRUE(every_kind.matches(kind, style_url));
  }
  TEST_ASSERT_TRUE(
    mln::core::RouteTable{}.matches(MLN_RESOURCE_KIND_STYLE, style_url)
  );
}

struct QueryCase {
  std::string_view url;
  std::string_view rewritten;
};

// Where the parameters go in each shape of URL. The transport never sends a
// fragment, so only these cases can see one.
constexpr auto query_cases = std::array{
  QueryCase{"https://maps.localhost/a", "https://maps.localhost/a?k=v"},
  QueryCase{"https://maps.localhost/a?x=1", "https://maps.localhost/a?x=1&k=v"},
  QueryCase{"https://maps.localhost/a?", "https://maps.localhost/a?k=v"},
  QueryCase{"https://maps.localhost/a?x&", "https://maps.localhost/a?x&k=v"},
  QueryCase{"https://maps.localhost/a#top", "https://maps.localhost/a?k=v#top"},
  QueryCase{
    "https://maps.localhost/a?x#y?z", "https://maps.localhost/a?x&k=v#y?z"
  },
};

void rules_add_parameters_before_the_fragment() {
  for (const auto& row : query_cases) {
    const auto url = std::string{row.url};
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
      std::string{row.rewritten}.c_str(),
      mln::core::append_query(row.url, "k=v").c_str(), url.c_str()
    );
  }
  TEST_ASSERT_EQUAL_STRING(
    "https://maps.localhost/a#top",
    mln::core::append_query("https://maps.localhost/a#top", "").c_str()
  );
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(globs_match_the_documented_pattern_language);
  RUN_TEST(routes_match_resolved_urls_and_selected_kinds);
  RUN_TEST(rules_add_parameters_before_the_fragment);
}
