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

[[nodiscard]] constexpr auto matches_kind(
  std::uint32_t kinds, mln_resource_kind request_kind
) noexcept -> bool {
  return request_kind < 32U && ((kinds >> request_kind) & 1U) != 0;
}

struct UrlPattern {
  UrlPattern(std::uint32_t kinds, std::uint32_t flags, std::string_view url);
  std::uint32_t kinds;
  std::string url;
  std::shared_ptr<CompiledGlob> glob;

  [[nodiscard]] auto matches(
    mln_resource_kind request_kind, std::string_view url
  ) const noexcept -> bool;
};

// The routes of a resource provider, which owns their strings.
struct RouteTable {
  std::vector<UrlPattern> routes;

  // Whether a request reaches the provider. A table without routes claims
  // every request.
  [[nodiscard]] auto matches(
    mln_resource_kind kind, std::string_view resolved_url
  ) const noexcept -> bool;
};

struct HeaderRuleTable {
  struct Rule {
    UrlPattern pattern;
    std::vector<std::pair<std::string, std::string>> headers;
  };
  std::vector<Rule> rules;
};

// The query parameters of a resource transform's rules, each rule's already
// percent-encoded and joined with `&`.
struct QueryRuleTable {
  struct Rule {
    UrlPattern pattern;
    std::string query;
  };
  std::vector<Rule> rules;

  // The URL with the first matching rule's parameters, or the URL unchanged.
  [[nodiscard]] auto apply(mln_resource_kind kind, const std::string& url) const
    -> std::string;
};

// Adds an encoded query to a URL, before any fragment.
[[nodiscard]] auto append_query(std::string_view url, std::string_view query)
  -> std::string;

// Validate and copy the caller's arrays. A rejected table records a
// diagnostic naming the index of the first invalid entry, and never a header
// or parameter value.
auto copy_routes(
  const mln_resource_route* routes, std::size_t count, RouteTable& out
) -> mln_status;
auto copy_header_rules(
  const mln_http_header_rule* rules, std::size_t count, HeaderRuleTable& out
) -> mln_status;
auto copy_query_rules(
  const mln_resource_transform_rule* rules, std::size_t count,
  QueryRuleTable& out
) -> mln_status;

// The header transform callback that applies a table, which user_data points
// to.
auto header_rules_callback(
  void* user_data, mln_resource_kind kind, mln_buffer_view url,
  mln_http_header_transform_response* out_response
) -> mln_status;

}  // namespace mln::core
