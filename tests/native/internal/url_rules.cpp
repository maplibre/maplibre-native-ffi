// The URL language that provider routes and transform rules share: the glob
// patterns that mln_url_match_flags documents, and which URL a route
// compares. A loader cannot produce every candidate URL, so these cases call
// the matchers directly.

#include <array>
#include <atomic>
#include <barrier>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <string_view>
#include <thread>
#include <vector>

#include "resources/url_rules.hpp"

#include "internal/support/allocation_faults.hpp"
#include "maplibre_native_c.h"
#include "runtime/runtime.hpp"
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
  // Without the glob flag, metacharacters compare literally.
  const auto exact = mln::core::UrlPattern{
    MLN_RESOURCE_KIND_MASK_ALL, MLN_URL_MATCH_EXACT, "**"
  };
  TEST_ASSERT_FALSE(exact.matches(MLN_RESOURCE_KIND_STYLE, style_url));
  TEST_ASSERT_TRUE(exact.matches(MLN_RESOURCE_KIND_STYLE, "**"));
}

auto table_of(mln_resource_route route) -> mln::core::RouteTable {
  auto table = mln::core::RouteTable{};
  MLN_TEST_OK(mln::core::copy_routes(&route, 1, table));
  return table;
}

// A route compares the resolved URL unless it asks for the requested one. A
// route matches only the kinds that its mask names.
void routes_compare_the_url_stage_they_select() {
  constexpr auto alias = std::string_view{"maplibre://maps/style"};
  const auto resolved = table_of({
    .kinds = MLN_RESOURCE_KIND_MASK_STYLE | MLN_RESOURCE_KIND_MASK_SOURCE,
    .flags = MLN_URL_MATCH_EXACT,
    .url = MLN_BUFFER_LITERAL("https://maps.localhost/style.json"),
    .url_stage = MLN_RESOURCE_ROUTE_URL_RESOLVED,
  });
  TEST_ASSERT_TRUE(resolved.matches(MLN_RESOURCE_KIND_STYLE, alias, style_url));
  TEST_ASSERT_TRUE(
    resolved.matches(MLN_RESOURCE_KIND_SOURCE, alias, style_url)
  );
  TEST_ASSERT_FALSE(resolved.matches(MLN_RESOURCE_KIND_TILE, alias, style_url));
  // A kind past the mask's width never matches a route that names kinds.
  TEST_ASSERT_FALSE(resolved.matches(UINT32_MAX, alias, style_url));

  const auto requested = table_of({
    .kinds = MLN_RESOURCE_KIND_MASK_ALL,
    .flags = MLN_URL_MATCH_EXACT,
    .url = MLN_BUFFER_LITERAL("maplibre://maps/style"),
    .url_stage = MLN_RESOURCE_ROUTE_URL_REQUESTED,
  });
  TEST_ASSERT_TRUE(requested.matches(MLN_RESOURCE_KIND_TILE, alias, style_url));
  TEST_ASSERT_FALSE(
    requested.matches(MLN_RESOURCE_KIND_STYLE, style_url, style_url)
  );

  const auto requested_glob = table_of({
    .kinds = MLN_RESOURCE_KIND_MASK_ALL,
    .flags = MLN_URL_MATCH_GLOB,
    .url = MLN_BUFFER_LITERAL("maplibre://**"),
    .url_stage = MLN_RESOURCE_ROUTE_URL_REQUESTED,
  });
  TEST_ASSERT_TRUE(
    requested_glob.matches(MLN_RESOURCE_KIND_STYLE, alias, style_url)
  );

  // A table without routes claims every request.
  TEST_ASSERT_TRUE(
    mln::core::RouteTable{}.matches(MLN_RESOURCE_KIND_STYLE, alias, style_url)
  );
}

void pathological_globs_have_linear_work_in_candidate_length() {
  constexpr auto pattern = std::string_view{"https://x/**a*aaaaaaaaaaaaaaaab"};
  const auto compiled = mln::core::CompiledGlob{pattern};
  for (const auto length : {512U, 1024U, 2048U, 4096U, 8192U}) {
    const auto url = std::string{"https://x/"} + std::string(length, 'a');
    auto steps = std::size_t{0};
    TEST_ASSERT_FALSE(compiled.matches(url, &steps));
    TEST_ASSERT_TRUE(steps <= pattern.size() * url.size());
  }
}

void concurrent_matches_keep_independent_scratch() {
  const auto compiled =
    mln::core::CompiledGlob{"https://*.localhost/**/tile?.pbf"};
  auto start = std::barrier{8};
  auto valid = std::atomic_bool{true};
  auto threads = std::array<std::jthread, 8>{};
  for (auto& thread : threads) {
    thread = std::jthread{[&]() {
      start.arrive_and_wait();
      for (auto attempt = 0; attempt < 1000; ++attempt) {
        if (
          !compiled.matches("https://maps.localhost/a/b/tile1.pbf") ||
          compiled.matches("https://maps.localhost/a/b/tile12.pbf")
        ) {
          valid.store(false, std::memory_order_relaxed);
        }
      }
    }};
  }
  for (auto& thread : threads) thread.join();
  TEST_ASSERT_TRUE(valid.load(std::memory_order_relaxed));
}

auto literal_span(
  std::string_view pattern, std::size_t index, char candidate
) noexcept -> std::size_t {
  const auto element = pattern[index];
  if (element == '?') {
    return candidate == '/' ? 0 : 1;
  }
  if (element == '\\' && index + 1 < pattern.size()) {
    return pattern[index + 1] == candidate ? 2 : 0;
  }
  return element == candidate ? 1 : 0;
}

auto reference_glob_matches(
  std::string_view pattern, std::string_view candidate
) noexcept -> bool {
  auto pattern_index = std::size_t{0};
  auto candidate_index = std::size_t{0};

  struct WildcardCheckpoint {
    std::size_t pattern_index = std::string_view::npos;
    std::size_t candidate_index = 0;
  };
  auto segment_wildcard = WildcardCheckpoint{};
  auto spanning_wildcard = WildcardCheckpoint{};

  const auto backtrack = [&]() -> bool {
    if (segment_wildcard.pattern_index != std::string_view::npos) {
      if (
        segment_wildcard.candidate_index < candidate.size() &&
        candidate[segment_wildcard.candidate_index] != '/'
      ) {
        ++segment_wildcard.candidate_index;
        pattern_index = segment_wildcard.pattern_index;
        candidate_index = segment_wildcard.candidate_index;
        return true;
      }
      segment_wildcard = {};
    }
    if (
      spanning_wildcard.pattern_index == std::string_view::npos ||
      spanning_wildcard.candidate_index == candidate.size()
    ) {
      return false;
    }
    ++spanning_wildcard.candidate_index;
    segment_wildcard = {};
    pattern_index = spanning_wildcard.pattern_index;
    candidate_index = spanning_wildcard.candidate_index;
    return true;
  };

  while (true) {
    if (pattern_index < pattern.size() && pattern[pattern_index] == '*') {
      const auto run_start = pattern_index;
      while (pattern_index < pattern.size() && pattern[pattern_index] == '*') {
        ++pattern_index;
      }
      const auto checkpoint =
        WildcardCheckpoint{pattern_index, candidate_index};
      if (pattern_index - run_start > 1) {
        spanning_wildcard = checkpoint;
        segment_wildcard = {};
      } else {
        segment_wildcard = checkpoint;
      }
      continue;
    }
    if (candidate_index < candidate.size()) {
      const auto span =
        pattern_index < pattern.size()
          ? literal_span(pattern, pattern_index, candidate[candidate_index])
          : 0;
      if (span != 0) {
        pattern_index += span;
        ++candidate_index;
        continue;
      }
    } else if (pattern_index == pattern.size()) {
      return true;
    }
    if (!backtrack()) {
      return false;
    }
  }
}

auto words(std::string_view alphabet, std::size_t length)
  -> std::vector<std::string> {
  auto result = std::vector<std::string>{""};
  auto begin = std::size_t{0};
  for (auto size = std::size_t{0}; size < length; ++size) {
    const auto end = result.size();
    for (auto i = begin; i < end; ++i)
      for (auto ch : alphabet) result.push_back(result[i] + ch);
    begin = end;
  }
  return result;
}

void generated_globs_preserve_the_pattern_language() {
  const auto patterns = words("a/*?\\", 6);
  const auto candidates = words("ab/", 5);
  auto comparisons = std::size_t{0};
  for (const auto& pattern : patterns) {
    const auto compiled = mln::core::CompiledGlob{pattern};
    for (const auto& candidate : candidates) {
      const auto expected = reference_glob_matches(pattern, candidate);
      const auto actual = compiled.matches(candidate);
      if (expected != actual) {
        const auto diagnostic =
          "pattern=" + pattern + ", candidate=" + candidate;
        TEST_ASSERT_EQUAL_MESSAGE(expected, actual, diagnostic.c_str());
      }
      ++comparisons;
    }
  }
  TEST_ASSERT_EQUAL_size_t(7109284, comparisons);
}

void registration_bounds_glob_tokens_for_every_table() {
  for (const auto flags : {MLN_URL_MATCH_GLOB, MLN_URL_MATCH_EXACT}) {
    for (const auto length : {4096U, 4097U}) {
      const auto pattern = std::string(length, '?');
      const auto view = mln_buffer_view{pattern.data(), pattern.size()};
      const auto expected = flags == MLN_URL_MATCH_GLOB && length > 4096
                              ? MLN_STATUS_INVALID_ARGUMENT
                              : MLN_STATUS_OK;
      const auto route = mln_resource_route{
        .kinds = MLN_RESOURCE_KIND_MASK_ALL,
        .flags = flags,
        .url = view,
        .url_stage = MLN_RESOURCE_ROUTE_URL_RESOLVED,
      };
      auto routes = mln::core::RouteTable{};
      MLN_TEST_STATUS(expected, mln::core::copy_routes(&route, 1, routes));
      const auto rewrite = mln_resource_rewrite_rule{
        .kinds = MLN_RESOURCE_KIND_MASK_ALL,
        .flags = flags,
        .url = view,
      };
      auto rewrites = mln::core::RewriteRuleTable{};
      MLN_TEST_STATUS(
        expected, mln::core::copy_rewrite_rules(&rewrite, 1, rewrites)
      );
      const auto header = mln_http_header_rule{
        .kinds = MLN_RESOURCE_KIND_MASK_ALL,
        .flags = flags,
        .url = view,
      };
      auto headers = mln::core::HeaderRuleTable{};
      MLN_TEST_STATUS(
        expected, mln::core::copy_header_rules(&header, 1, headers)
      );
    }
  }
  // Escapes and wildcard runs consume one token each, regardless of byte count.
  auto pattern = std::string{};
  for (auto i = 0; i < 4095; ++i) pattern += "\\?";
  pattern += std::string(8192, '*');
  const auto route = mln_resource_route{
    .kinds = MLN_RESOURCE_KIND_MASK_ALL,
    .flags = MLN_URL_MATCH_GLOB,
    .url = {pattern.data(), pattern.size()},
    .url_stage = MLN_RESOURCE_ROUTE_URL_RESOLVED,
  };
  auto routes = mln::core::RouteTable{};
  MLN_TEST_OK(mln::core::copy_routes(&route, 1, routes));
  TEST_ASSERT_TRUE(routes.matches(
    MLN_RESOURCE_KIND_STYLE, "", std::string(4095, '?') + "/tail"
  ));
}

void long_provider_globs_match_without_allocating() {
  const auto pattern = std::string(4096, '?');
  const auto candidate = std::string(4096, 'a');
  const auto state = std::make_shared<mln::core::ResourceProviderState>();
  auto table = std::make_shared<mln::core::RouteTable>();
  table->routes.push_back(
    {{MLN_RESOURCE_KIND_MASK_ALL, MLN_URL_MATCH_GLOB, pattern},
     MLN_RESOURCE_ROUTE_URL_RESOLVED}
  );
  state->provider.routes = table;
  const auto lease = mln::core::ResourceProviderLease{
    state, std::shared_lock{state->mutex}, state->provider
  };
  const auto previous = std::set_terminate([] { std::_Exit(86); });
  auto matched = false;
  auto worker = std::thread{[&] {
    const auto faults = mln::native_tests::AllocationFaults{};
    matched = lease.matches(MLN_RESOURCE_KIND_STYLE, candidate, candidate);
  }};
  worker.join();
  std::set_terminate(previous);
  TEST_ASSERT_TRUE(matched);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(registration_bounds_glob_tokens_for_every_table);
  RUN_TEST(long_provider_globs_match_without_allocating);
  RUN_TEST(generated_globs_preserve_the_pattern_language);
  RUN_TEST(globs_match_the_documented_pattern_language);
  RUN_TEST(concurrent_matches_keep_independent_scratch);
  RUN_TEST(pathological_globs_have_linear_work_in_candidate_length);
  RUN_TEST(routes_compare_the_url_stage_they_select);
}
