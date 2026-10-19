#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "maplibre_native_c.h"

namespace mln::core {

// Matches candidate against pattern in the glob language that
// mln_url_match_flags describes. Matching is iterative and allocates nothing,
// so it bounds stack use on MapLibre threads; the worst case costs the product
// of the two lengths.
[[nodiscard]] auto glob_matches(
  std::string_view pattern, std::string_view candidate
) noexcept -> bool;

// Compares candidate against url as flags select: exactly, or as a glob.
[[nodiscard]] auto url_matches(
  std::uint32_t flags, std::string_view url, std::string_view candidate
) noexcept -> bool;

// Whether a rule or route of kind applies to a request of request_kind.
[[nodiscard]] constexpr auto matches_kind(
  std::uint32_t kind, std::uint32_t request_kind
) noexcept -> bool {
  return kind == MLN_RESOURCE_KIND_ANY || kind == request_kind;
}

// One copied route, rule, or header rule pattern.
struct UrlPattern {
  std::uint32_t kind = 0;
  std::uint32_t flags = 0;
  std::string url;

  [[nodiscard]] auto matches(
    std::uint32_t request_kind, std::string_view url
  ) const noexcept -> bool;
};

// The routes of a resource provider, which owns their strings.
struct RouteTable {
  std::vector<UrlPattern> routes;

  // Whether a request reaches the provider. A table without routes claims
  // every request.
  [[nodiscard]] auto matches(
    std::uint32_t kind, std::string_view requested_url,
    std::string_view resolved_url
  ) const noexcept -> bool;
};

struct RewriteRuleTable {
  struct Rule {
    UrlPattern pattern;
    std::optional<std::string> replacement_url;
  };
  std::vector<Rule> rules;
};

struct HeaderRuleTable {
  struct Rule {
    UrlPattern pattern;
    std::vector<std::pair<std::string, std::string>> headers;
  };
  std::vector<Rule> rules;
};

// Validate and copy the caller's arrays. A rejected table records a
// diagnostic naming the index of the first invalid entry, and never a header
// value.
auto copy_routes(
  const mln_resource_route* routes, std::size_t count, RouteTable& out
) -> mln_status;
auto copy_rewrite_rules(
  const mln_resource_rewrite_rule* rules, std::size_t count,
  RewriteRuleTable& out
) -> mln_status;
auto copy_header_rules(
  const mln_http_header_rule* rules, std::size_t count, HeaderRuleTable& out
) -> mln_status;

// The transform callbacks that apply a table, which user_data points to.
auto rewrite_rules_callback(
  void* user_data, std::uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) noexcept -> mln_status;
auto header_rules_callback(
  void* user_data, std::uint32_t kind, const char* url,
  mln_http_header_transform_response* out_response
) noexcept -> mln_status;

}  // namespace mln::core
