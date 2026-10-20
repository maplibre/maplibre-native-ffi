#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "maplibre_native_c.h"

namespace mln::core {

// A glob compiled into a linear NFA. Each candidate byte visits each token
// once, so work is O(pattern length * candidate length). Scratch storage is
// bounded on the stack, so matching never allocates.
class CompiledGlob {
 public:
  static constexpr auto max_tokens = std::size_t{4096};
  [[nodiscard]] static auto token_count(std::string_view pattern) noexcept
    -> std::size_t;
  explicit CompiledGlob(std::string_view pattern);
  [[nodiscard]] auto matches(std::string_view candidate) const noexcept -> bool;

 private:
  enum class Kind : std::uint8_t { Literal, Any, Segment, Spanning };
  struct Token {
    Kind kind;
    char literal = 0;
  };
  std::vector<Token> tokens_;
};

// Whether a route or header rule with the kinds mask applies to a request of
// request_kind.
[[nodiscard]] constexpr auto matches_kind(
  std::uint32_t kinds, std::uint32_t request_kind
) noexcept -> bool {
  return request_kind < 32U && ((kinds >> request_kind) & 1U) != 0;
}

// One copied route or header rule pattern.
struct UrlPattern {
  UrlPattern(std::uint32_t kinds, std::uint32_t flags, std::string_view url);
  std::uint32_t kinds;
  std::string url;
  std::shared_ptr<CompiledGlob> glob;

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
    std::uint32_t kind, std::string_view resolved_url
  ) const noexcept -> bool;
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
auto copy_header_rules(
  const mln_http_header_rule* rules, std::size_t count, HeaderRuleTable& out
) -> mln_status;

// The header transform callback that applies a table, which user_data points
// to.
auto header_rules_callback(
  void* user_data, std::uint32_t kind, const char* url,
  mln_http_header_transform_response* out_response
) -> mln_status;

}  // namespace mln::core
